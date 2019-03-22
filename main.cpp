/*
MIT License

Copyright (c) 2019 Madis Kaal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <util/delay.h>
#include <ctype.h>
#include "uart.hpp"
#include "chip.hpp"

#define led_on() PORTB&=(~0x08)
#define led_off() PORTB|=0x08
#define button_pressed() (!(PIND&8))

UART uart;
Chip chip;

volatile uint16_t busytimer;
uint8_t buf[128];
uint8_t idx;

void makebusy(void)
{
  TCNT0=0x0;
  busytimer=200;
  led_on();
}
 
void chiperase(void)
{
  makebusy();
  chip.setadr(0);
  while (chip.getadr()<2048)
  {
    chip.write(0xff);
    chip.nextadr();
    wdt_reset();
    WDTCSR|=0x40;
  }
}

void chipread(void)
{
uint16_t a,i;
uint8_t cs,b;
  makebusy();
  chip.setadr(0);
  uart.prints(":020000040000");
  uart.printx((~6)+1);
  uart.prints("\r\n");
  for (a=0;a<2048;) {
    makebusy();
    a=chip.getadr();
    cs=0x10;
    uart.prints(":10");
    uart.printx(a>>8);
    cs+=a>>8;
    uart.printx(a&0xff);
    cs+=a&255;
    uart.printx(0);
    for (i=0;i<16;i++) {
      b=chip.read();
      cs+=b;
      uart.printx(b);
      a=chip.nextadr();
    }
    uart.printx((~cs)+1);
    uart.prints("\r\n");
    wdt_reset();
    WDTCSR|=0x40;
  }
  uart.prints(":00000001FF\r\n");
}

uint8_t tobin(uint8_t c)
{
  if (c>='0' && c<='9')
    return c-'0';
  if (c>='A' && c<='F')
    return c-'0'-7;
  return 0;
}

uint8_t dehex(uint8_t *&s)
{
uint8_t b;
  if (!*s)
    return 0;
  b=tobin(*s)<<4;
  s++;
  if (!*s)
    return b;
  b=b|tobin(*s);
  s++;
  return b;
}

uint8_t validhex(void)
{
uint8_t *s=&buf[1];
uint8_t cs=0;
  while (*s) {
    cs+=dehex(s);
  }
  return cs==0;
}

void writehex(void)
{
uint8_t *s=&buf[1];
uint8_t nbytes,type;
uint16_t adr;
  if (validhex())
  {
    nbytes=dehex(s);
    adr=dehex(s);
    adr=(adr<<8);
    adr|=dehex(s);
    type=dehex(s);
    switch (type)
    {
      case 0: // data
        chip.setadr(adr);
        while (nbytes && adr<0x800) {
          type=dehex(s);
          if (!chip.write(type)) {
            uart.prints("write error at ");
            uart.printx(adr>>8);
            uart.printx(adr&0xff);
            uart.prints("\r\n");
          }
          adr=chip.nextadr();
          nbytes--;
        }
        break;
      case 4: // linear segment, assume its 0
        break;
      case 1: // end
        break;
      default:
        uart.prints("unsupported record type\r\n");
    }
  }
  else {
    uart.prints("bad checksum\r\n");
  }
}

uint8_t *token(void)
{
uint8_t *s=&buf[idx];
  while (buf[idx] && isspace(buf[idx]))
    idx++;
  s=&buf[idx];
  while (buf[idx] && (!isspace(buf[idx]))) {
    buf[idx]=tolower(buf[idx]);
    idx++;
  }
  if (buf[idx])
    buf[idx++]='\0';
  return s;
}

void execute(void)
{
uint8_t *s;
  if (*buf==':')
    writehex();
  else {
    idx=0;
    s=token();
    if (*s) {
      if (!strcmp((const char*)s,"read"))
        chipread();
      else if (!strcmp((const char*)s,"help") || !strcmp((const char*)s,"?"))
      {
        uart.prints("Commands:\r\nread\r\nhelp\r\nerase\r\n");
      }
      else if (!strcmp((const char*)s,"erase")) {
        chiperase();
      }
      else {
        uart.prints("?");
      }
    }
    uart.prints("\r\n>");
  }
}

void process(uint8_t c)
{
  if (c=='\n' || c=='\r')
  {
    uart.prints("\r\n");
    buf[idx]='\0';
    execute();
    idx=0;
    return;
  }
  if (c==8)
  {
    if (idx) {
      idx--;
      uart.prints("\x8 \x8");
    }
    return;
  }
  if (idx<(sizeof(buf)-1)) {
    buf[idx++]=c;
    uart.send(c);
  }
}

ISR(USART_RX_vect)
{
  uart.received(UDR0);
}

ISR(USART_UDRE_vect)
{
  uart.transmit();
}

ISR(TIMER0_OVF_vect)
{
  TCNT0=0x0;
  if (busytimer)
  {
    busytimer--;
    if (!busytimer)
      led_off();
  }
}

ISR(WDT_vect)
{
}

/*
I/O configuration
-----------------
I/O pin                               direction    DDR  PORT

PC0 D0                                input        0    1 
PC1 D1                                input        0    1
PC2 D2                                input        0    1
PC3 D3                                input        0    1
PC4 4040CLK                           output       1    0
PC5 4040RST                           output       1    0

PB0 /WE                               output       1    1
PB1 /OE                               output       1    1
PB2 /CE                               output       1    1
PB3 led                               output       1    1
PB4 unused                            input        0    1
PB5 unused                            input        0    1

PD0 RxD                               input        0    1
PD1 TxD                               output       1    1
PD2 unused                            input        0    1
PD3 button                            input        0    1
PD4 D4                                input        0    1
PD5 D5                                input        0    1
PD6 D6                                input        0    1
PD7 D7                                input        0    1
*/


int main(void)
{
  MCUSR=0;
  MCUCR=0;
  // I/O directions
  DDRC=0x30;
  DDRD=0x02;
  DDRB=0x0f;
  // initial state
  PORTC=0x0f;
  PORTD=0xff;
  PORTB=0x3f;
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  // configure watchdog to interrupt&reset, 4 sec timeout
  WDTCSR|=0x18;
  WDTCSR=0xe8;
  // configure timer0 for periodic interrupts
  TCCR0B=4; // timer0 clock prescaler to 256
  TCNT0=0;
  TIMSK0=1; // enable overflow interrupts
  //
  uart.initialize(9600);
  sei();
  // find zero position on both axis
  while (1) {
    sleep_cpu();   // watchdog or I/O interrupt wakes us up
    wdt_reset();
    WDTCSR|=0x40;
    if (button_pressed())
      chipread();
    if (uart.ready())
      process(uart.read());
  }
}

