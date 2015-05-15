/* uart.c
 * Copyright 2014 - 2015 Herman Moolenaar 
 * This file is part of LCR-Meter.
 *
 * LCR-Meter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LCR-Meter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LCR-Meter.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "uart.h"
#include <avr/interrupt.h>
#include "kernel.h"
#include <util/atomic.h>

// baud rate constants for a CPU frequency of 32 MHz
#define BSEL_1200    3331
#define BSEL_9600    3317
#define BSEL_19200   3301
#define BSEL_38400   3269
#define BSEL_115200  262

#define BSCALE_1200    -1
#define BSCALE_9600    -4
#define BSCALE_19200   -5
#define BSCALE_38400   -6
#define BSCALE_115200  -4

#define BUFFER_MASK 0x0f  // mask for a 32 byte transmit and receive buffer
volatile char rxBuffer[16];
volatile char txBuffer[16];

volatile uint8_t rxWriteIndex;
volatile uint8_t rxReadIndex;
volatile uint8_t txWriteIndex;
volatile uint8_t txReadIndex;
volatile uint8_t UartFinished;

void UartSetup()
{
   rxWriteIndex = 0;
   rxReadIndex = 0;
   txWriteIndex = 0;
   txReadIndex = 0;
   UartFinished = true;

   // set TX pin as output and high
   PORTE.OUT |= PIN3_bm;
   PORTE.DIR |= PIN3_bm;
   
   USARTE0.BAUDCTRLA = BSEL_115200 & 0xff;
   USARTE0.BAUDCTRLB = (BSCALE_115200 << 4) | (BSEL_115200 >> 8);
   USARTE0.CTRLA = USART_RXCINTLVL_LO_gc | USART_TXCINTLVL_LO_gc;
   USARTE0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
   USARTE0.CTRLC = USART_CHSIZE_8BIT_gc;
   PMIC.CTRL |= PMIC_LOLVLEN_bm; // enable low level interrupt
}

void UartTransmit()
{
   if (txReadIndex != txWriteIndex)
   {
      USARTE0.DATA = txBuffer[txReadIndex];
      txReadIndex = (txReadIndex + 1) & BUFFER_MASK;
   }
}

void UartStartTransmit()
{
   ATOMIC_BLOCK(ATOMIC_FORCEON)
   {
      if (UartFinished == true)
      {
         UartFinished = false;
         UartTransmit();
      }
   }
}

ISR (USARTE0_TXC_vect)
{
   if (txReadIndex != txWriteIndex)
   {
      UartTransmit();
   }
   else
   {
      UartFinished = true;
   }
}
   
void UartWrite(char* buffer)
{
   char *ptr = buffer;
   uint8_t test;

   while(*ptr != 0)
   {
      test = (txWriteIndex + 1) & BUFFER_MASK;

      while (test == txReadIndex)
      {
         // wait till space becomes available in the transmit buffer
         TaskSleep(2);
      }
      txBuffer[txWriteIndex] = *ptr++;
      txWriteIndex = test;
      UartStartTransmit();
   }
}

void int32ToUart(uint8_t before, int32_t value)
{
   char buffer[10];
   UNUSED(buffer);
   int8_t i;
   uint32_t divide = 1;
   
   for (i = 1; i < before; i++) divide *= 10;

   if (value < 0)
   {
      buffer[0] = '-';
      value = -value;
   }
   else
   {
      buffer[0] = ' ';
   }
   
   for (i = 1; i <= before; i++)
   {
      buffer[i] = '0' + value / divide;
      value %= divide;
      divide /= 10;
   }
   
   before++;
   buffer[before] = 0;
   
   UartWrite(buffer);
}

uint8_t UartRead(char* buffer, uint8_t size)
{
   uint32_t delay;
   uint8_t i, count;

   count = 0;
   for (i = 0; i < size; ++i)
   {
      delay = 0;
      while(rxReadIndex == rxWriteIndex)
      {
         // wait till receive buffer contains data
         TaskSleep(2);
         if (delay++ > 50)
         {
            // if it takes too long, return with the data we got
            return count;
         }
      }
      buffer[count] = rxBuffer[rxReadIndex];
      rxReadIndex = (rxReadIndex + 1) & BUFFER_MASK;
   }
   return count;
}

ISR (USARTE0_RXC_vect)
{
   uint8_t rxIndexPlusOne = (rxWriteIndex + 1) & BUFFER_MASK;
   uint8_t discard;
   UNUSED(discard);
   
   if (rxIndexPlusOne != rxReadIndex)
   {
      rxBuffer[rxWriteIndex] = USARTE0.DATA;
      rxWriteIndex = rxIndexPlusOne;
   }
   else
   {
      discard = USARTE0.DATA;
   }
}

