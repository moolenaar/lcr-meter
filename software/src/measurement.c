/* measurement.c
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

#include "measurement.h"
#include "kernel.h"
#include "display.h"
#include <math.h>
#include "export.h"

enum statevalues {STATE_FREQUENCY, STATE_INIT, STATE_WAIT_FOR_COMPONENT, STATE_MEASURE1, STATE_MEASURE2, STATE_ADJUST_RANGE_AND_FREQUENCY, STATE_CHECK_COIL, STATE_CHECK_RESISTOR, STATE_NEXT, STATE_CORRECT_DC};

const float ReferenceResistor[] = { 100.7, 2005.0, 20020.0, 199400.0, 1977000.0, 0.0, 0.0, 198.7};
float ReferenceCorrection[CORRECTION_VALUES];
float MeasureCorrection[CORRECTION_VALUES];

MeasurementStruct_t Measurement;
CalibrationStruct_t Calibration;

void SetRange(uint8_t range)
{
   uint8_t value = PORTD.OUT & ~0x07; 
   PORTD.OUT = value | (range & 0x07);
   TaskSleep(1);
}

void MeasurementSetup()
{
   int8_t i;
   
   // range selection
   PORTD.DIR |= 0x07;
   for (i = 0; i < CORRECTION_VALUES; i++)
   {
      ReferenceCorrection[i] = 1.0;
      MeasureCorrection[i] = 1.0;      
   }
}

MeasurementResult_t* GetMeasurementValue()
{
   static MeasurementResult_t result;
   
   result.Value = Measurement.Value;
   result.state = Measurement.state;
   result.Index = Measurement.Index;
   result.Component = Measurement.Component;
   
   return &result;
}

uint8_t GetBatteryValue()
{
   return 2;
}

void DoCalibrate(int16_t* data, int16_t size, int16_t offset)
{
   int16_t i;
   
   for (i=0; i < size; i++)
   {
      data[i] = data[i] - offset;
   }
}

float GoertzelFilter(int16_t* data)
{
   uint16_t i;
   float s1 = 0.0;
   float s2 = 0.0;
   float sine, cosine, coeff, real, imag, omega, s0;
   
   omega = 2.0 * M_PI / GetSampleRate() * GetFrequency();
   sine = sin(omega);
   cosine = cos(omega);
   coeff = 2.0 * cosine;
   
   for (i = 0; i < GetDataSize(); i++)
   {
      s0 = (float)data[i] + coeff * s1 - s2;
      s2 = s1;
      s1 = s0;
   }   
  
   real = s1 - s2 * cosine;
   imag = s2 * sine;

   return sqrtf(real * real + imag * imag);
}

float Resistance(float inputResult, float outputResult)
{
   float v;

   v =  outputResult / inputResult / 2.0;
   if (1.0 - v == 0)
   {
      Measurement.state = MeasurementStateError;
      v = 0.0;
   }
   else
   {
      if (v > 1.1)
      {
         Measurement.state = MeasurementStateOpen;
         v = 0.0;
      }
      else
      {
         Measurement.state = MeasurementStateValid;
         v = v * (ReferenceResistor[Measurement.MeasurementRange] + Calibration.ResistanceSerial) / (1.0 - v);
      }
   }
   return v;
}

float Capacitance(float inputResult, float outputResult)
{
   float v, a, b, w;

   v =  outputResult / inputResult / 2.0;
   
   Measurement.state = MeasurementStateValid;
   w = 2.0 * M_PI * GetFrequency();
   a = 1.0 / ((ReferenceResistor[Measurement.MeasurementRange] + Calibration.ResistanceSerial) * v * w);
   b = 1.0 / ((ReferenceResistor[Measurement.MeasurementRange] + Calibration.ResistanceSerial) * w);
   v = sqrt(a * a - b * b);
   return v;
}

float Inductance(float inputResult, float outputResult)
{
   float v, a, r, w;

   Measurement.state = MeasurementStateValid;

   v =  outputResult / inputResult / 2.0;
   v = v * v;
   r = ReferenceResistor[Measurement.MeasurementRange] + Calibration.ResistanceSerial;
   r = r * r;
   
   w = 2.0 * M_PI * GetFrequency();
   w = w * w;
   a = r / (w / v - w);
   v = sqrt(a);
   return v;
}

void MeasureAndWait()
{
   StartMeasurement();
   while (!MeasurementReady())
   {
      TaskSleep(1);
   }
}

void CorrectLinearity(int16_t* buffer, float* correction, int16_t offset)
{
   int16_t i, index;
   
   for (i = 0; i < GetDataSize(); i++)
   {
      index = buffer[i] / CORRECTION_STEP + CORRECTION_VALUES / 2;
      if (index < 0)
      {
         index = 0;
      }
      else if (index >= CORRECTION_VALUES)
      {
         index = CORRECTION_VALUES - 1;
      }
      buffer[i] = (int16_t)((float)(buffer[i] - offset) * correction[index]);
   }
}

void DoMeasurement(MeasurementStruct_t* meas)
{
   int16_t* reference = GetReferenceData();
   int16_t* measured = GetMeasuredData();
   
   MeasureAndWait();
   
   CorrectLinearity(reference, ReferenceCorrection, 0);
   CorrectLinearity(measured, MeasureCorrection, Calibration.AdcOffset);
//   ExportRawData();
   
   meas->Reference = GoertzelFilter(reference);
   meas->Measured = GoertzelFilter(measured);
   meas->average = 1;
}

void DoAveragedMeasurement(MeasurementStruct_t* meas, int8_t nrAverages)
{
   int8_t i;
   MeasurementStruct_t current;
   
   meas->Reference = 0.0;
   meas->Measured = 0.0;
   meas->average = 0;
   
   for (i = 0; i < nrAverages; i++)
   {
      DoMeasurement(&current);
      if (current.Reference == 0.0) return;

      meas->Reference += current.Reference;
      meas->Measured += current.Measured;
      meas->average += current.average;
   }
}

void ResetCalibrationValues()
{
   Calibration.ResistanceSerial = 0.0;
   Calibration.AdcOffset = 0;
   Calibration.DacOffset = 0;
}

void InitialiseDc()
{
   SetRange(2);
   SetDcOutput(0, 2);
   MeasureAndWait();
   TaskSleep(750);      
}

void measureDc(int16_t value, uint8_t input, float* ref, float* meas)
{
   int16_t n;
   float f;
   int16_t* reference = GetReferenceData();
   int16_t* measure = GetMeasuredData();

   SetDcOutput(value, input);
   MeasureAndWait();

   f = 0;
   for (n = 0; n < GetDataSize(); n++)
   {
      f += reference[n] - Calibration.AdcOffset;
   }
   *ref = f / GetDataSize();
   
   f = 0;
   for (n = 0; n < GetDataSize(); n++)
   {
      f += measure[n] - Calibration.AdcOffset;
   }
   *meas = f / GetDataSize();
}
   
void CalibrateAdcLinearity()
{
   int16_t i;
   float meas, offset = 0.0, factor;
   static float buffer[CORRECTION_VALUES];

   buffer[CORRECTION_VALUES / 2] = 0;
  
   for (i = 0; i < CORRECTION_VALUES / 2; i++)
   {
      buffer[CORRECTION_VALUES / 2 + i] =  i * CORRECTION_STEP;
      buffer[CORRECTION_VALUES / 2 - i] = -i * CORRECTION_STEP;
   }
   ExportFloatData("DAC input data\n\r", buffer, CORRECTION_VALUES);
   
   for(i = 0; i < CORRECTION_VALUES / 2; i++)
   {
      measureDc( i * CORRECTION_STEP, 2, &buffer[CORRECTION_VALUES / 2 + i], &meas);
      measureDc(-i * CORRECTION_STEP, 2, &buffer[CORRECTION_VALUES / 2 - i], &meas);
   }
   
   factor = 1.0;
   for(i = 0; i < CORRECTION_VALUES / 2; i++)
   {
      if (i == 0)
      {
         offset = buffer[i] / 2;
         buffer[CORRECTION_VALUES / 2] = 1.0;
      }
      else
      {
         buffer[CORRECTION_VALUES / 2 + i] -= offset;
         buffer[CORRECTION_VALUES / 2 - i] -= offset;
         buffer[CORRECTION_VALUES / 2 + i] /= (float)( i * CORRECTION_STEP);
         buffer[CORRECTION_VALUES / 2 - i] /= (float)(-i * CORRECTION_STEP);
         factor += buffer[CORRECTION_VALUES / 2 + i];
         factor += buffer[CORRECTION_VALUES / 2 - i];
      }
   }
   factor /= (float)((CORRECTION_VALUES / 2) * 2);
   ExportFloat("Factor", factor);
   
   ReferenceCorrection[CORRECTION_VALUES / 2] = 1.0;
   for(i = 1; i < CORRECTION_VALUES / 2; i++)
   {
      ReferenceCorrection[CORRECTION_VALUES / 2 + i] = buffer[CORRECTION_VALUES / 2 + i] / factor;
      ReferenceCorrection[CORRECTION_VALUES / 2 - i] = buffer[CORRECTION_VALUES / 2 - i] / factor;
      MeasureCorrection[CORRECTION_VALUES / 2 + i] = buffer[CORRECTION_VALUES / 2 + i] / factor;
      MeasureCorrection[CORRECTION_VALUES / 2 - i] = buffer[CORRECTION_VALUES / 2 - i] / factor;
   }   
   ExportFloatData("reference correction\n\r", ReferenceCorrection, CORRECTION_VALUES);
}

void CalibrateMuxLinearity()
{
   
}

void CalibrateZeroPoint()
{
   int16_t i, count;
   int32_t value;
   int16_t* reference;
   int16_t* measurement;
   
   // calibrate zero point of ADC by measuring differential with inputs tied together
   SetRange(6);
   SetDcOutput(0, 0);
   MeasureAndWait();
   measurement = GetMeasuredData();
   value = 0;
   for (i = 0; i < GetDataSize(); i++)
   {
      value += measurement[i];
   }
   value /= GetDataSize();
   Calibration.AdcOffset = value;
   
   // calibrate zero point of the output voltage
   SetRange(6);
   count = 0;
   Calibration.DacOffset = 0; 
   do
   {
      SetDcOutput(-Calibration.DacOffset, 2);
      MeasureAndWait();
      reference = GetReferenceData();
      value = 0;
      for (i = 0; i < GetDataSize(); i++)
      {
         value += reference[i];
      }
      value /= GetDataSize();
      Calibration.DacOffset = (int16_t)((float)(Calibration.DacOffset + value) / 2.0 + 0.5);
      count++;
   }
   while ((value != 0) && (count < 5));
   Calibration.DacOffset -= Calibration.AdcOffset;

   // correct ADC offset; add offset of input amplifier
   SetRange(7);
   count = 0;
   do
   {   
      SetDcOutput(0, 0);
      MeasureAndWait();
      measurement = GetMeasuredData();
      value = 0;
      for (i = 0; i < GetDataSize(); i++)
      {
         value += measurement[i];
      }
      value /= GetDataSize();
      Calibration.AdcOffset = value;
      count++;
   }
   while ((value != 0) && (count < 25));

   ExportInt32("DAC offset:", Calibration.DacOffset);
   ExportInt32("ADC offset:", Calibration.AdcOffset);
}

void resetMeasurement(MeasurementStruct_t* meas, enum FrequencyIndex freq)
{
   meas->Measured = 0.0;
   meas->Reference = 0.0;
   meas->average = 0;
   meas->Value = 0.0;
   meas->Index = freq;
   meas->MeasurementRange = 4;
   meas->Component = TypeUndetermined;
}

void CalibrateMuxResistance()
{
   // measure the resistance of the analog multiplexer
   MeasurementStruct_t current;
   
   SetRange(7);
   SetOutput(FREQ_200HZ, DAC_AMPLITUDE, 2);
   resetMeasurement(&current, FREQ_1KHZ);
   DoMeasurement(&current);
   DoAveragedMeasurement(&current, 10);
  
   Calibration.ResistanceSerial = ReferenceResistor[7] * current.Reference / current.Measured * 2.0 - ReferenceResistor[7] - 99.9;
   current.Value = Calibration.ResistanceSerial;
   ExportMeasuredData("Mux resistance: ", &current);
//   TaskSleep(500);
}

void Calibrate()
{
   UartWrite("Calibrating...\n\r");
   LcdCommand(CMD_DISPLAY_INTRO);   
   
   ResetCalibrationValues();

   InitialiseDc();
   CalibrateZeroPoint();
   CalibrateMuxResistance();
   CalibrateAdcLinearity();
   CalibrateMuxResistance();
   CalibrateMuxLinearity();
   CalibrateMuxResistance();
      
//   FrequencyResponse();
//   DcResponse();
//   CalibrationValues();
   
//   TaskSleep(2000);
   LcdCommand(CMD_DISPLAY_MAIN_SCREEN);
}

void FindRange()
{
   float v1;

   while (true)
   {
      DoMeasurement(&Measurement);
      v1 = Measurement.Measured / Measurement.Reference;

      if ((v1 < 0.1) && (Measurement.MeasurementRange > 0))
      {
         Measurement.MeasurementRange--;
         SetRange(Measurement.MeasurementRange);
      }
      else if ((v1 > 1.3) && (Measurement.MeasurementRange < 4))
      {
         Measurement.MeasurementRange++;
         SetRange(Measurement.MeasurementRange);
      }
      else if ((v1 < 0.1) && (Measurement.MeasurementRange == 0) && (Measurement.Index == FREQ_1KHZ))
      {
         Measurement.Index = FREQ_50HZ;
         SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
      }
      else if ((v1 < 0.051) && (Measurement.Index == FREQ_50HZ))
      {
         Measurement.Index = FREQ_50KHZ;
         SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
      }
      else
      {
         return;
      }
   }   
}

void Measure()
{
   static uint8_t state = STATE_INIT;
   static float v1 = 0.0, v2 = 0.0;
   enum FrequencyIndex oldIndex;
    
   while (true)
   {
      switch (state)
      {
         // set everything up
         case STATE_INIT:
            resetMeasurement(&Measurement, FREQ_1KHZ);
            SetRange(Measurement.MeasurementRange);
            SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
            UartWrite("STATE_INIT\n\r");
            state = STATE_WAIT_FOR_COMPONENT;
            break;
         
         // reset measurement frequency and range and wait for a unknown component to be connected to the test leads   
         case STATE_WAIT_FOR_COMPONENT:
            UartWrite("STATE_TEST\n\r");
            resetMeasurement(&Measurement, FREQ_1KHZ);
            SetRange(Measurement.MeasurementRange);
            SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
            DoMeasurement(&Measurement);
            v1 = Measurement.Measured / Measurement.Reference;
            if (v1 < 1.3)
            {
               state = STATE_ADJUST_RANGE_AND_FREQUENCY;
            }
            else
            {
               Measurement.Value = 0.0;
               Measurement.Component = TypeUndetermined;
               return;
            }
            break;
         
         // make measurement range within ADC range
         case STATE_ADJUST_RANGE_AND_FREQUENCY:
            UartWrite("STATE_RANGE\n\r");
            
            resetMeasurement(&Measurement, Measurement.Index);
            SetRange(Measurement.MeasurementRange);
            SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
            FindRange();
            v1 = Measurement.Measured / Measurement.Reference;

            if (Measurement.Index < FREQ_1KHZ)
            {
               Measurement.Component = TypeCapacitor;
            }
            else if (Measurement.Index > FREQ_1KHZ)
            {
                Measurement.Component = TypeInductor;
            }
            else
            {
               UartWrite("STATE_ADJUST_RANGE_AND_FREQUENCY\n\r");
               oldIndex = Measurement.Index;
               Measurement.Index = FREQ_200HZ;
               SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
               DoMeasurement(&Measurement);
               v2 = Measurement.Measured / Measurement.Reference;
               if (v2 < v1 - 0.01)
               {
                  Measurement.Component = TypeInductor;
               }
               else if (v2 > v1 + 0.01)
               {
                  Measurement.Component = TypeCapacitor;
               }
               else
               {
                   Measurement.Component = TypeResistor;
                   oldIndex = FREQ_200HZ;
               }
               Measurement.Index = oldIndex;
               SetOutput(Measurement.Index, DAC_AMPLITUDE, 3);
            }
            ExportInt32("index = ", Measurement.Index);
            ExportInt32("range = ", Measurement.MeasurementRange);
            state = STATE_MEASURE1;
            break;

         case STATE_MEASURE1:
            UartWrite("STATE_MEASURE1\n\r");
            DoAveragedMeasurement(&Measurement, 5);
            v2 = Measurement.Measured / Measurement.Reference;
            state = STATE_MEASURE2;
            Measurement.state = MeasurementStateValid;
            return;
            
         case STATE_MEASURE2:
            UartWrite("STATE_MEASURE2\n\r");
            DoAveragedMeasurement(&Measurement, 10);
            v1 = Measurement.Measured / Measurement.Reference;
            if (fabs(v2 - v1) > 0.1)
            {
               state = STATE_WAIT_FOR_COMPONENT;
            }
            return;
      }
      TaskSleep(1);
   }
}

void MeasureBattery()
{
   
}

void Calculate()
{
   switch (Measurement.Component)
   {
      case TypeResistor:
         Measurement.Value = Resistance(Measurement.Reference, Measurement.Measured);
         break;
         
      case TypeCapacitor:
         Measurement.Value = Capacitance(Measurement.Reference, Measurement.Measured);
         break;
         
      case TypeInductor:
         Measurement.Value = Inductance(Measurement.Reference, Measurement.Measured);
         break;

      default:
         Measurement.Value = 0.0;
         break;
   }
}

void Display()
{
   switch (Measurement.state)
   {
      case MeasurementStateError:
         LcdCommand(CMD_MEASUREMENT_ERROR);
         break;
    
      case MeasurementStateOpen:
         LcdCommand(CMD_INPUT_OPEN);
         break;
    
      case MeasurementStateValid:
         LcdCommand(CMD_UPDATE_MEASUREMENT);
         break;
   }        
}

void MeasurementTask()
{
   static uint8_t state = 0;
   static uint8_t count = 0;

   while(true)
   {
      switch(state)
      {
         case 0:
            Calibrate();
            LcdCommand(CMD_DISPLAY_MAIN_SCREEN);
            state++;
            break;
         
         case 1:
            Measure();
            state++;
            break;
         
         case 2:
            Calculate();
            state++;
            break;
         
         case 3:
            Display();
            state++;
            break;      
         
         case 4:
            //ExportRawData();
            ExportMeasuredData("", &Measurement);
            count++;
            state = count % 100 == 0 ? state + 1 : 1;
            state = 5;
            break;
            
         case 5:
            MeasureBattery();
//            if (count % 1000 == 0)
            {
               state++;
               count = 0;
            }
            //else
            //{
               //state = 1;
            //}               
            //break;
            
         case 6:
            LcdCommand(CMD_DISPLAY_BATTERY);
            state = 1;
            break;
         
         default:
            state = 0;
            break;
      }
      
      TaskSleep(10);
   } 
}
