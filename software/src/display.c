/* display.c
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

#include "display.h"
#include "lcd.h"
#include "generator.h"
#include "measurement.h"
#include "kernel.h"

enum LcdCommand_t LcdCmd;


void WriteLcd(__flash const __flash uint8_t * const __flash *font, uint8_t x, uint8_t y, const char* buffer)
{
   uint8_t position = 0, fontIndex = 0;
   uint8_t i = 0, n, z = x;
   uint32_t value = 0;

   uint16_t heightBytes = font[0][0];
   uint32_t mask = (font[0][1] << (24 + y)) | (font[0][2] << (16 + y)) | (font[0][3] << (8 + y)) | (font[0][4] << y);
   
   while (buffer[position] != 0)
   {
      fontIndex = 1;
      while ((font[fontIndex][0] != ' ') && (font[fontIndex][0] != buffer[position])) fontIndex++;

      for (i = 0; i < font[fontIndex][1]; i++)
      {
         LcdDisplay[3][z] &= mask;
         LcdDisplay[2][z] &= mask >> 8;
         LcdDisplay[1][z] &= mask >> 16;
         LcdDisplay[0][z] &= mask >> 24;
         
         for (n = 0; n < heightBytes; n++)
         {
            value |= (uint32_t)font[fontIndex][2 + i * heightBytes + n]<<(y + n * 8);
         }

         LcdDisplay[3][z] |= value;
         LcdDisplay[2][z] |= value>>8;
         LcdDisplay[1][z] |= value>>16;
         LcdDisplay[0][z] |= value>>24;
         value = 0;
         
         z++;
      }
      position++;
   }
   
   for (i = 0; i < 4; i++)
   {
      TaskSleep(0);
      LcdSetPosition(3 - i, x);
      LcdWriteBuffer(z - x, &LcdDisplay[i][x]);
   }
}

char* int32ToStr(char* buffer, uint8_t before, int32_t value)
{
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

   buffer[before + 1] = 0;
   
   i = 0;
   do
   {
      i++;
   } while ((i < before) && (buffer[i] <= '0'));
   
   i--;
   
   while(i > 0)
   {
      buffer[i--] = buffer[0];
      buffer[0] = ' ';
   }
   
   return buffer;
}

char* floatToStr(char* buffer, uint8_t before, uint8_t after, float value)
{
   uint8_t i, pos, tmp;
   
   int32ToStr(buffer, before, (int32_t)value);
   pos = before + 1;

   if (after > 0)
   {
      buffer[pos++] = '.';
      
      value = value - (int32_t)value;
      if (value < 0)
      {
         value = -value;
      }

      float divide = 0.1;
      
      for (i = 0; i < after; i++)
      {
         tmp = (uint8_t)(value / divide);
         buffer[pos + i] = '0' + tmp;
         value -= tmp * divide;
         divide /= 10;
      }
   }
   buffer[pos + after] = 0;
   
   return buffer;
}

void LcdDisplayIntro()
{
   WriteLcd(font8x5, 1, 22, "LCR meter");
   //WriteLcd(font8x5, 1, 11, "Calibrating please wait.");
   WriteLcd(font8x5, 1, 0,  "version 0.1");
}

void LcdDisplayMainScreen()
{
   LcdClear();
   WriteLcd(font8x5, 20, 11, "wait...");
}

void LcdUpdateMeasurement()
{
   char buffer[10];
   char *ptr;
   MeasurementResult_t* meas = GetMeasurementValue();
   uint8_t before, after, index;
   float value;
   const char* unit[] = {"  ", "O ", "kO", "MO", "uF", "nF", "pF", "uH", "mH", "H "};

   switch (meas->Component)
   {
      case TypeResistor:
      WriteLcd(images, 0, 0, "R");
      index = 1;
      break;

      case TypeCapacitor:
      WriteLcd(images, 0, 0, "C");
      index = 4;
      break;

      case TypeInductor:
      WriteLcd(images, 0, 0, "L");
      index = 7;
      break;
         
      default:
      index = 0;
      WriteLcd(images, 0, 0, " ");
      break;
   }

   if (meas->Value > 1.0e6)
   {
      value = 1e6;
      index += 2;
   }
   else if (meas->Value > 1.0e3)
   {
      value = 1.0e3;
      index += 1;
   }
   else if (meas->Value > 1.0e0)
   {
      value = 1.0;
      index += 0;
   }
   else if (meas->Value > 1.0e-3)
   {
      value = 1.0e-3;
      index += 0;
   }
   else if (meas->Value > 1.0e-6)
   {
      value = 1.0e-6;
      index += 0;
   }
   else if (meas->Value > 1.0e-9)
   {
      value = 1.0e-9;
      index += 1;
   }
   else if (meas->Value > 1.0e-12)
   {
      value = 1.0e-12;
      index += 2;
   }
   else
   {
      value = 1.0;
      index = 0;
   }
   value = meas->Value / value;
   if (value > 1e3)
   {
      before = 4;
   }
   else if (value > 1e2)
   {
      before = 3;
   }
   else if (value > 1e1)
   {
      before = 2;
   }
   else
   {
      before = 1;
   }
   //before = (uint8_t)ceil(log(value));
   after = 4 - before;

   WriteLcd(font20x15, 15, 12, floatToStr(buffer, before, after, value));
   WriteLcd(font13x10, 91, 12, unit[index]);
      
   switch (meas->Index)
   {
      case FREQ_50HZ:   ptr = "50 Hz";   break;
      case FREQ_100HZ:  ptr = "100 Hz";  break;
      case FREQ_200HZ:  ptr = "200 Hz";  break;
      case FREQ_500HZ:  ptr = "500 Hz";  break;
      case FREQ_1KHZ:   ptr = "1 KHz";   break;
      case FREQ_2KHZ:   ptr = "2 KHz";   break;
      case FREQ_5KHZ:   ptr = "5 KHz";   break;
      case FREQ_10KHZ:  ptr = "10 KHz";  break;
      case FREQ_20KHZ:  ptr = "20 KHz";  break;
      case FREQ_50KHZ:  ptr = "50 KHz";  break;
      case FREQ_100KHZ: ptr = "100 KHz"; break;
      default: ptr = ""; break;
   }
   WriteLcd(font8x5, 77, 0, ptr);
}

void LcdDisplayBattery()
{
   uint8_t index = GetBatteryValue();
   char buf[2];
   buf[0] = '0' + index;
   buf[1] = 0; 
   WriteLcd(bat, 117, 0, buf);
}

void LcdDisplayOpen()
{
  WriteLcd(font8x5, 25, 15, "connect");
  WriteLcd(font8x5, 20, 7, "component");
}

void LcdDisplayCounter()
{
   char buffer[10];
   int16_t i = 0;
   
   UNUSED(buffer);
   
   WriteLcd(font20x15, 0, 4, int32ToStr(buffer, 4, i++));
   if (i > 9999) i = 0;
   TaskSleep(200);
}

void LcdCommand(enum LcdCommand_t cmd)
{
   while (LcdCmd != CMD_NONE) TaskSleep(1);
   LcdCmd = cmd;
   TaskSleep(0);
}

void DisplaySetup()
{
   LcdSetup();
   LcdCmd = CMD_NONE;
}

void LcdTask()
{
   LcdInit();

   while (true)
   {
      switch(LcdCmd)
      {
         case CMD_NONE:
         break;
         
         case CMD_CLEAR:
         LcdClear();
         break;

         case CMD_DISPLAY_INTRO:
         LcdDisplayIntro();
         break;

         case CMD_DISPLAY_MAIN_SCREEN:
         LcdDisplayMainScreen();
         break;

         case CMD_UPDATE_MEASUREMENT:
         LcdUpdateMeasurement();
         break;
         
         case CMD_DISPLAY_COUNTER:
         LcdDisplayCounter();
         break;
         
         case CMD_INPUT_OPEN:
         LcdDisplayOpen();
         break;
         
         case CMD_DISPLAY_BATTERY:
         LcdDisplayBattery();
         break;
         
         case CMD_MEASUREMENT_ERROR:
         break;
      }
      
      LcdCmd = CMD_NONE;
      TaskSleep(1);
   }
}
