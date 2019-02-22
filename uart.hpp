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
#ifndef __uart_hpp__
#define __uart_hpp__
#include <avr/io.h>

template <class T>
class queue
{
volatile T buf[32];
uint8_t head,tail,count;
public:
  queue(): head(0),tail(0),count(0) {}

  void clear()
  {
     count=0;
     tail=head;
  }
  
  void push(T c)
  {
    if (count<(sizeof(buf)/sizeof(buf[0]))) {
      buf[head++]=c;
      head=head%(sizeof(buf)/sizeof(buf[0]));
      count++;
    }
  }
  
  T pop()
  {
    T v=0;
    if (count) {
      v=buf[tail++];
      count--;
      tail=tail%(sizeof(buf)/sizeof(buf[0]));
    }
    return v;
  }
  
  uint8_t len()
  {
    return count;
  }
  
  uint8_t full()
  {
    return count==(sizeof(buf)/sizeof(buf[0]));
  }
  
  uint8_t empty()
  {
    return !count;
  }
};

class UART
{
  queue<uint8_t> rqueue,tqueue;
  
public:

  // called from interrupt handler to store received byte  
  void received(uint8_t c)
  {
    rqueue.push(c);
  }
  
  // called from interrupt handler to send byte from tx queue
  void transmit()
  {
    UDR0=tqueue.pop();
    if (tqueue.len()==0) {
      // last byte from buffer being sent, disable TX interrupt
      // and enable tx complete interrupt
      UCSR0B&=(~_BV(UDRIE0)); // disable tx interrupt
      UCSR0B|=_BV(TXCIE0); // enable completion interrupt
    }
  }
  
  // called from interrupt handler when transmitter register is
  // empty
  void transmit_complete()
  {
    UCSR0B&=(~_BV(TXCIE0)); // disable completion interrupt
  }
  
  uint8_t ready()
  {
    return rqueue.len();
  }
  
  uint8_t read()
  {
    return rqueue.pop();    
  }
  
  void send(uint8_t c)
  {
    UCSR0B&=(~_BV(TXCIE0)); // disable completion interrupt
    while (tqueue.full()) {
      wdt_reset();
      WDTCSR|=0x40;
    }
    tqueue.push(c);
    UCSR0B|=_BV(UDRIE0); // enable tx interrupt
  }
  
  uint8_t empty()
  {
    return tqueue.len()==0;
  }

  void initialize(uint32_t baudrate)
  {
    UBRR0H=((F_CPU/(16UL*baudrate))-1)>>8;
    UBRR0L=((F_CPU/(16UL*baudrate))-1)&0xff;
    UCSR0A=0;
    UCSR0B=_BV(RXEN0)|_BV(TXEN0)|_BV(RXCIE0);
  }

  void printx(uint8_t b)
  {
    uint8_t c;
    c=(b>>4)+'0';
    if (c>'9')
      c+=7;
    send(c);
    b=(b&0x0f)+'0';
    if (b>'9')
      b+=7;
    send(b);
  }
  
  void printn(int32_t n)
  {
    if (n<0)
    {
      send('-');
      n=-n;
    }
    if (n>9)
      printn(n/10);
    send((n%10)+'0');
  }
  
  void prints(const char* s)
  {
    while (s && *s)
      send(*s++);
  }  
  
};

#endif
