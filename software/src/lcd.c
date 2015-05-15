/* lcd.c
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

#include "lcd.h"
#include <avr/interrupt.h>
#include "kernel.h"
#include <util/atomic.h>


volatile uint16_t LcdBuffer[16];
volatile uint8_t LcdWriteIndex;
volatile uint8_t LcdReadIndex;
#define LCD_MASK 0x0f;

uint8_t LcdDisplay[4][128];


void LcdTransmit()
{
   if (LcdBuffer[LcdReadIndex] & 0x0100)
   {
      PORTC.OUT &= ~PIN2_bm; // output control (CD = L)
   }
   else
   {
      PORTC.OUT |= PIN2_bm; // output data (CD = H)
   }

   SPIC.DATA = (uint8_t)(LcdBuffer[LcdReadIndex]);
   LcdReadIndex = (LcdReadIndex + 1) & LCD_MASK;
}

void LcdStartTransmit()
{
   ATOMIC_BLOCK(ATOMIC_FORCEON)
   {
      if (PORTC.OUT & PIN4_bm)
      {
         PORTC.OUT &= ~PIN4_bm; // enable UC1601 (CS0 = L)
         LcdTransmit();
      }
   }
}

void LcdWriteCode(uint8_t command, uint8_t code)
{
   uint8_t test = (LcdWriteIndex + 1) & LCD_MASK;

   while (LcdReadIndex == test)
   {
       TaskSleep(2);
   }
   LcdBuffer[LcdWriteIndex] = command == true ? 0x0100 + code : code;
   LcdWriteIndex = test;
   LcdStartTransmit();
}

void LcdWriteBuffer(uint8_t size, uint8_t* buffer)
{
   uint8_t i;
   uint8_t test;
   
   for (i = 0; i < size; i++)
   {
      test = (LcdWriteIndex + 1) & LCD_MASK;
      while (LcdReadIndex == test)
      {
          TaskSleep(2);
      }
      LcdBuffer[LcdWriteIndex] = buffer[i];
      LcdWriteIndex = test;
      LcdStartTransmit();
   }
}

void LcdSetPosition(uint8_t page, uint8_t horizontal)
{
   // set page address
   LcdWriteCode(true, 0xB0 + page);
   
   // set column address
   LcdWriteCode(true, 0x00 + (horizontal & 0x0f));
   LcdWriteCode(true, 0x10 + (horizontal >> 4));
}

void LcdBufferClear(int16_t x, int16_t y, int16_t width, uint32_t mask)
{
   uint8_t i, n;
   
   for (n = 0; n < 4; n++)
   {
      for (i = x; i < x + width; i++)
      {
         LcdDisplay[n][i] &= mask << (y + n);
      }
   }
}

void LcdClear()
{
//   uint8_t i, n;
   uint8_t i;
   LcdSetPosition(0, 0);
   
   for (i = 0; i < 67; i++)
   {
      LcdWriteBuffer(8, (uint8_t *)"\0\0\0\0\0\0\0\0");
   }
   //for (n = 0; n < 4; n++)
   //{
      //for (i = 0; i < 128; ++i)
      //{
         //LcdDisplay[n][i] = 0;
      //}
   //}
   LcdBufferClear(0, 0, 128, 0xffffffff);
}


void LcdSetup()
{
   uint8_t i;
   
   LcdWriteIndex = 0;
   LcdReadIndex = 0;
   
   for (i = 0; i < 128; ++i)
   {
      LcdDisplay[0][i] = 0;
      LcdDisplay[1][i] = 0;
      LcdDisplay[2][i] = 0;
      LcdDisplay[3][i] = 0;
   }
   
   // backlight on
   PORTC.DIR |= PIN3_bm;
   PORTC.OUT |= PIN3_bm;

   // nSS connected to CS0 of UC1601; nSS under software control
   // disable LCD; CS0 = H
   PORTC.DIR |= PIN4_bm;
   PORTC.OUT |= PIN4_bm;

   // CD pin; L = contol, H = data
   PORTC.DIR |= PIN2_bm;
   PORTC.OUT &= ~PIN2_bm;
   
   // MOSI and SCK pins
   PORTC.DIR |= PIN5_bm | PIN7_bm;
   
   // mode 3: MSB first data change on falling edge, read by UC1601 on rising edge, 1MHz
   SPIC.CTRL = SPI_CLK2X_bm | SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER1_bm | SPI_MODE1_bm | SPI_MODE0_bm;
   SPIC.INTCTRL = SPI_INTLVL1_bm; // not the same level as kernel
   PMIC.CTRL |= PMIC_MEDLVLEN_bm; // enable med interrupt level
}

ISR (SPIC_INT_vect)
{
   if (LcdReadIndex != LcdWriteIndex)
   {
      LcdTransmit();
   }
   else
   {
      PORTC.OUT |= PIN4_bm; // disable UC1601 (CS0 = H)
   }
}

void LcdInit()
{
   // now that the SPI controller is operational, setup the LCD controller
   TaskSleep(10);

   // system reset
   LcdWriteCode(true, 0xE2);
   
   TaskSleep(10);
   
   // set temperature compensation
   LcdWriteCode(true, 0x26);
   
   // set LCD power control
   LcdWriteCode(true, 0x2E);
   
   // set frame rate
   LcdWriteCode(true, 0xA0);
   
   // set COM end
   LcdWriteCode(true, 0xF1);
   LcdWriteCode(true, 32 - 1);
   
   // LCD bias ratio
   
   // LCD potmeter
   LcdWriteCode(true, 0x81);
   LcdWriteCode(true, 80);

   LcdClear();
   
   // enable LCD
   LcdWriteCode(true, 0xAF);
   TaskSleep(10);
   
   // set inverse display
   // LcdWriteCode(true, 0xa7);
}
