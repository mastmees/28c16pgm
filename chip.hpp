#ifndef __chip_hpp__
#define __chip_hpp__

#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>

#define areset_high() PORTC|=0x20
#define areset_low() PORTC&=(~0x20)
#define aclock_high() PORTC|=0x10
#define aclock_low() PORTC&=(~0x10)
#define ce_high() PORTB|=0x04
#define ce_low() PORTB&=(~0x04)
#define oe_high() PORTB|=0x02
#define oe_low() PORTB&=(~0x02)
#define we_high() PORTB|=0x01
#define we_low() PORTB&=(~0x01)

extern volatile uint16_t busytimer;
extern void makebusy(void);

class Chip
{
uint16_t a;
public:
  Chip()
  {
    this->setadr(0);
    we_high();
    oe_high();
    ce_high();
  }
  
  void setadr(uint16_t adr)
  {
    if (!adr || adr<this->a) {
      areset_high();
      areset_low();
      this->a=0;
    }
    while (this->a<adr)
    {
      aclock_high();
      aclock_low();
      this->a++;
    }
  }

  uint16_t getadr(void) { return this->a; }
  
  uint16_t nextadr(void)
  {
    uint16_t b=this->a;
    b=b+1;
    setadr(b);
    return this->a;
  }

  uint8_t read(void)
  {
    uint8_t b;
    ce_low();
    _delay_us(1);
    oe_low();
    _delay_us(1);
    b=(PINC&0x0f)|(PIND&0xf0);
    oe_high();
    _delay_us(1);
    ce_high();
    _delay_us(1);
    return b;
  }

  uint8_t write(uint8_t b)
  {
  uint8_t x;
    makebusy();
    PORTD=(PORTD&0x0f)|(b&0xf0);
    PORTC=(PORTC&0xf0)|(b&0x0f);
    DDRD|=0xf0;
    DDRC|=0x0f;
    ce_low();
    DDRD&=0x0f;
    DDRC&=0xf0;
    _delay_us(1);
    we_low();
    _delay_us(1);
    we_high();
    DDRD&=0x0f;
    DDRC&=0xf0;
    PORTD|=0xf0;
    PORTC|=0x0f;
    wdt_reset();
    WDTCSR|=0x40;
    _delay_us(1);
    while (busytimer)
    {
      oe_low();
      _delay_us(1);
      x=PIND&0x80;
      oe_high();
      _delay_us(1);
      if (x==(b&0x80))
      {
        ce_high();
        return 1;
      }
    }
    ce_high();
    return 0;
  }
    
};

#endif
