/* export.c
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

#include "export.h"
#include "display.h"

void ExportInt16Data(char* str, int16_t* data, int16_t size)
{
   uint16_t i;

   UartWrite(str);
   for (i = 0; i < size; i++)
   {
      int32ToUart(7, data[i]);
      UartWrite(" ");
   }
   UartWrite("\n\r");
};

void ExportFloatData(char* str, float* data, int16_t size)
{
   uint16_t i;
   char buffer[10];
   
   UartWrite(str);
   for (i = 0; i < size; i++)
   {
      UartWrite(floatToStr(buffer, 4, 3, data[i]));
      UartWrite(" ");
   }
   UartWrite("\n\r");
};

void ExportInt32(char* str, int32_t data)
{
   UartWrite(str);
   int32ToUart(7, data);
   UartWrite("\n\r");
}

void ExportFloat(char* str, float data)
{
   char buffer[10];

   UartWrite(str);
   UartWrite(floatToStr(buffer, 4, 3, data));
   UartWrite("\n\r");
}

void ExportCorrectionData(char* str, float* correctionData)
{
   uint16_t i;
   char buffer[12];

   UartWrite("\n\r");
   UartWrite(str);
   for (i = 0; i < CORRECTION_VALUES; i++)
   {
      UartWrite(floatToStr(buffer, 3, 3, correctionData[i]));
      UartWrite(" ");
   }
   UartWrite("\n\r");   
};

void ExportZeroPoint(char* str, CalibrationStruct_t* cal)
{
   UartWrite(str);
   int32ToUart(4, cal->DacOffset);
   UartWrite(" ");
   int32ToUart(4, cal->AdcOffset);
   UartWrite("\n\r");
}

void ExportRawData()
{
   uint16_t i;
   int16_t* ref = GetReferenceData();
   int16_t* data = GetMeasuredData();
   
   // export reference signal
   UartWrite("\n\rreference:\n\r");
   for (i = 0; i < GetDataSize(); i++)
   {
      int32ToUart(5, ref[i]);
      UartWrite(" ");
   }
   UartWrite("\n\r");
   
   // export data signal
   UartWrite("\n\rmeasurement:\n\r");
   for (i = 0; i < GetDataSize(); i++)
   {
      int32ToUart(5, data[i]);
      UartWrite(" ");
   }
   UartWrite("\n\r");
}

void ExportMeasuredData(char *str, MeasurementStruct_t* meas)
{
   char buffer[10];
   
   UartWrite(str);
   int32ToUart(2, meas->MeasurementRange);
   UartWrite(" ");
   int32ToUart(8, (int32_t)meas->Reference);
   UartWrite(" ");
   int32ToUart(8, (int32_t)meas->Measured);
   UartWrite(" ");
   UartWrite(floatToStr(buffer, 1, 3, meas->Measured / meas->Reference));
   UartWrite(" ");
   int32ToUart(6, (int32_t)(meas->Value));
   UartWrite("\n\r");
}

void DcResponse()
{
   //int16_t i, value;
//
   //UartWrite("\n\r DC response: \n\r");
   //UartWrite("DAC reference measurement\n\r");
   //UartWrite("-------------------------\n\r");
   //
   //SetRange(7);
   //value = -1792;
   //
   //for (i = 0; i < 32; i++)
   //{
      //SetDcOutput(value, 2);
      //StartMeasurement();
      //while (!MeasurementReady())
      //{
         //TaskSleep(1);
      //}
      //int32ToUart(8, (int32_t)value);
      //UartWrite("  ");
      //int32ToUart(8, (int32_t)GetReferenceData()[10]);
      //UartWrite("  ");
      //int32ToUart(8, (int32_t)GetMeasuredData()[10]);
      //UartWrite("\n\r");
      //
      //value += 112;
   //}
}

void CalibrationValues()
{
   UartWrite("\n\rDAC = (");
   int32ToUart(3, (int32_t)DACB.GAINCAL);
   UartWrite(", ");
   int32ToUart(3, (int32_t)DACB.OFFSETCAL);
   UartWrite(")\n\r");

   UartWrite("\n\rADC = (");
   int32ToUart(3, (int32_t)ADCA.CALH);
   UartWrite(", ");
   int32ToUart(3, (int32_t) ADCA.CALL);
   UartWrite(")\n\r");
}

void FrequencyResponse()
{
   //typedef struct
   //{
      //enum FrequencyIndex index;
      //char* txt;
   //} arr_t;
   //const arr_t arr[] = {
      //{FREQ_50HZ,   "50 Hz  "},
      //{FREQ_100HZ,  "100 Hz "},
      //{FREQ_200HZ,  "200 Hz "},
      //{FREQ_500HZ,  "500 Hz "},
      //{FREQ_1KHZ,   "1 KHz  "},
      //{FREQ_2KHZ,   "2 KHz  "},
      //{FREQ_5KHZ,   "5 KHz  "},
      //{FREQ_10KHZ,  "10 KHz "},
      //{FREQ_20KHZ,  "20 KHz "},
      //{FREQ_50KHZ,  "50 KHz "},
      //{FREQ_100KHZ, "100 KHz"}
   //};
   //uint8_t i, n;
   //float reference;
   //float measured;
   //float angle;
//
   //UartWrite("\n\rfrequency response: \n\r");
   //UartWrite("range frequency reference measured angle\n\r");
   //UartWrite("----------------------------------------\n\r");
//
   //for (i = 0; i < 5; i++)
   //{
      //SetRange(i);
      //for (n = 0; n < sizeof(arr) / sizeof(arr_t); n++)
      //{
         //SetOutput(arr[n].index, DAC_AMPLITUDE, 3);
         //DoMeasurement(&reference, &measured, &angle);
         //DoMeasurement(&reference, &measured, &angle);
         //
         //int32ToUart(2, i);
         //UartWrite("  ");
         //UartWrite(arr[n].txt);
         //UartWrite("  ");
         //int32ToUart(8, (int32_t)reference);
         //UartWrite("  ");
         //int32ToUart(8, (int32_t)measured);
         //UartWrite("  ");
         //int32ToUart(4, (int32_t)angle);
         //UartWrite("\n\r");
         //TaskSleep(10);
      //}
   //}
   //ExportRawData();
   //TaskSleep(2000);
//
   //SetRange(1);
   //SetOutput(FREQ_50KHZ, DAC_AMPLITUDE, 3);
//
   ////   TaskSleep(15000);
}
