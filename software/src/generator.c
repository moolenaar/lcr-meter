/* generator.c
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

#include "generator.h"
#include <avr/interrupt.h>
#include <math.h>
#include "nvm.h"
#include "kernel.h"
#include "measurement.h"

//#define DATA_SIZE 5 * SIN_TABLE_SIZE
#define OVERSAMPLING1 1.0
#define OVERSAMPLING2 2.0
#define OVERSAMPLING4 4.0

int8_t frequencyIndex;

GeneratorStruct_t GeneratorArray[] =
{
   { 20000 - 1, 1, 50.0},     // index 0: 50 Hz
   { 10000 - 1, 1, 100.0},    // index 1: 100 Hz
   { 5000 - 1,  1, 200.0},    // index 2: 200 Hz
   { 2000 - 1,  1, 500.0},    // index 3: 500 Hz
   { 1000 - 1,  1, 1000.0},   // index 4: 1 KHz
   { 500 - 1,   1, 2000.0},   // index 5: 2 KHz
   { 200 - 1,   1, 5000.0},   // index 6: 5 KHz
   { 100 - 1,   1, 10000.0},  // index 7: 10 KHz
   { 50 - 1,    1, 20000.0},  // index 8: 20 KHz
   { 40 - 1,    2, 50000.0},  // index 9: 50 KHz
   { 40 - 1,    4, 100000.0}  // index 10: 100 KHz
};      

uint16_t SinTable[SIN_TABLE_SIZE];
int16_t ReferenceData[DATA_SIZE];
int16_t MeasuredData[DATA_SIZE];
volatile uint8_t DataAvailable;

float GetFrequency()
{
   return GeneratorArray[frequencyIndex].frequency;
}

float GetSampleRate()
{
   return GeneratorArray[frequencyIndex].frequency / GeneratorArray[frequencyIndex].NrPeriods * (float)SIN_TABLE_SIZE;
}

void SetAdcInput(uint8_t input)
{
   switch (input)
   {
      case 1:
         ADCA.CH0.MUXCTRL = 0;  // ADC0 = +, ADC4 = -
         break;

      case 2:
         ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS1_bm;  // ADC2 = +, ADC4 = -
         break;

      case 3:
         ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS1_bm | ADC_CH_MUXPOS0_bm;   // ADC3 = +, ADC4 = -
         break;
   }
}
   
void SetDcOutput(int16_t amplitude, int8_t adcInput)
{
   uint16_t i;
   
   TCD0.CTRLA = 0;              // stop timer
   
   frequencyIndex = -1;

   // create table with sinus data
   for (i = 0; i < SIN_TABLE_SIZE; i++)
   {
      SinTable[i] = amplitude + 2047 - Calibration.DacOffset;
   }

   // set the DAC clock
   TCD0.CNT = 0;                // reset count value
   
   TCD0.PER = GeneratorArray[FREQ_1KHZ].divider;

   SetAdcInput(adcInput);

   TCD0.CTRLA = TC0_CLKSEL0_bm; // clock = CLKper (32 MHz)
   TaskSleep(10);
}
   
void SetOutput(enum FrequencyIndex index, int16_t amplitude, int8_t adcInput)
{
   uint16_t i;
   
   TCD0.CTRLA = 0;              // stop timer
   
   frequencyIndex = index;

   // create table with sinus data
   for (i = 0; i < SIN_TABLE_SIZE; i++)
   {
      SinTable[i] = (int16_t)(sin((float)i / (float)SIN_TABLE_SIZE * 2.0 * M_PI * GeneratorArray[index].NrPeriods) * (float)amplitude) + 2047 - Calibration.DacOffset;
   }

   // set the DAC clock
   TCD0.CNT = 0;                // reset count value
   
   TCD0.PER = GeneratorArray[index].divider;

   SetAdcInput(adcInput);

   TCD0.CTRLA = TC0_CLKSEL0_bm; // clock = CLKper (32 MHz)
   TaskSleep(10);
}

void GeneratorSetup()
{
   uint16_t i;
   
   DataAvailable = true;
   
   // create table with sinus data
   for (i = 0; i < SIN_TABLE_SIZE; i++)
   {
      SinTable[i] = 2047; // 0 V output
   }
   
   // setup DMA channel 0 for the DAC
   DMA.CTRL = DMA_ENABLE_bm;

   DMA.CH0.ADDRCTRL  = DMA_CH_SRCRELOAD0_bm | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BURST_gc | DMA_CH_DESTDIR_INC_gc;
   DMA.CH0.TRIGSRC   = DMA_CH_TRIGSRC_EVSYS_CH0_gc;
   DMA.CH0.TRFCNT    = (uint8_t)(sizeof(SinTable));
   DMA.CH0.REPCNT    = 0;
   DMA.CH0.SRCADDR0  = ((uint16_t)SinTable >> 0) & 0xff;
   DMA.CH0.SRCADDR1  = ((uint16_t)SinTable >> 8) & 0xff;
   DMA.CH0.SRCADDR2  = 0;
   DMA.CH0.DESTADDR0 = ((uint16_t)&DACB.CH0DATA >> 0) & 0xff;
   DMA.CH0.DESTADDR1 = ((uint16_t)&DACB.CH0DATA >> 8) & 0xff;
   DMA.CH0.DESTADDR2 = 0;
   DMA.CH0.CTRLB     = 0; //DMA_CH_TRNINTLVL_HI_gc; // generate high level interrupt when transaction complete
   DMA.CH0.CTRLA     = DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc;
   
   // setup DMA 1 for ADC CH0
   DMA.CH1.ADDRCTRL  = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc;
   DMA.CH1.TRIGSRC   = DMA_CH_TRIGSRC_ADCA_CH1_gc;
   DMA.CH1.TRFCNT    = sizeof(MeasuredData);
   DMA.CH1.REPCNT    = 1;
   DMA.CH1.SRCADDR0  = ((uint16_t)&ADCA.CH0.RES >> 0) & 0xff;
   DMA.CH1.SRCADDR1  = ((uint16_t)&ADCA.CH0.RES >> 8) & 0xff;
   DMA.CH1.SRCADDR2  = 0;
   DMA.CH1.DESTADDR0 = ((uint16_t)MeasuredData >> 0) & 0xff;
   DMA.CH1.DESTADDR1 = ((uint16_t)MeasuredData >> 8) & 0xff;
   DMA.CH1.DESTADDR2 = 0;
   DMA.CH1.CTRLA = DMA_CH_REPEAT_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc;

   // setup DMA 2 for ADC CH1
   DMA.CH2.ADDRCTRL  = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc;
   DMA.CH2.TRIGSRC   = DMA_CH_TRIGSRC_ADCA_CH1_gc;
   DMA.CH2.TRFCNT    = sizeof(ReferenceData);
   DMA.CH2.REPCNT    = 1;
   DMA.CH2.SRCADDR0  = ((uint16_t)&ADCA.CH1.RES >> 0) & 0xff;
   DMA.CH2.SRCADDR1  = ((uint16_t)&ADCA.CH1.RES >> 8) & 0xff;
   DMA.CH2.SRCADDR2  = 0;
   DMA.CH2.DESTADDR0 = ((uint16_t)ReferenceData >> 0) & 0xff;
   DMA.CH2.DESTADDR1 = ((uint16_t)ReferenceData >> 8) & 0xff;
   DMA.CH2.DESTADDR2 = 0;
   DMA.CH2.CTRLB     = DMA_CH_TRNINTLVL_HI_gc; // generate high level interrupt when transaction complete
   DMA.CH2.CTRLA     = DMA_CH_REPEAT_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc;

   // setup DAC 0
   DACB.GAINCAL = NvmRead(0x33);
   DACB.OFFSETCAL = NvmRead(0x32);
   DACB.CTRLB = 0;
   DACB.CTRLC = DAC_REFSEL0_bm;
   DACB.TIMCTRL = DAC_CONINTVAL_8CLK_gc;
   DACB.CTRLA |= DAC_CH0EN_bm | DAC_ENABLE_bm;
   
   // setup event channel 0; TCD to the DMA channel 0
   EVSYS.CH0MUX = EVSYS_CHMUX_TCD0_CCA_gc;
   
   // setup event channel 1; timer trigger ADC CH0
   EVSYS.CH1MUX = EVSYS_CHMUX_TCD0_CCB_gc;
   
   // setup event channel 2; timer trigger ADC CH1
   EVSYS.CH2MUX = EVSYS_CHMUX_TCD0_CCB_gc;

   // setup timer D0
   TCD0.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm;
   TCD0.PER = 100 - 1;          // 100 = 10KHz, 1000 = 1KHz, 10000 = 100Hz, 50000 = 50Hz
   TCD0.CCA = 15;               // used for DAC output (DMA)
   TCD0.CCB = 2;                // used for ADC trigger; both CH0 (out) and CH1 (in) signals
   TCD0.CTRLA = TC0_CLKSEL0_bm; // clock = CLKper (32 MHz)

   // setup ADC
   ADCA.CALH        = NvmRead(0x21);
   ADCA.CALL        = NvmRead(0x20);
   ADCA.CTRLB       = /*ADC_RESOLUTION0_bm | ADC_RESOLUTION1_bm |*/ ADC_CONMODE_bm;
   ADCA.REFCTRL     = ADC_REFSEL0_bm;
   ADCA.EVCTRL      = ADC_SWEEP0_bm | ADC_EVSEL0_bm | ADC_EVACT1_bm; // event channel 1 = trigger CH0, event channel 2 = trigger CH1
   ADCA.PRESCALER   = ADC_PRESCALER1_bm;                             // 2 MHz ADC clock
//   ADCA.PRESCALER = ADC_PRESCALER1_bm | ADC_PRESCALER0_bm;         // 1 MHz ADC clock
   ADCA.CH0.CTRL    = ADC_CH_INPUTMODE1_bm | ADC_CH_INPUTMODE0_bm;   // differential with 1x gain
   ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS1_bm;                             // ADC2 = +, ADC4 = -
   ADCA.CH1.CTRL    = ADC_CH_INPUTMODE1_bm;                          // differential without gain
   ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS0_bm;                             // ADC1 = +, ADC0 = -
   
   ADCA.CTRLA = ADC_DMASEL0_bm | ADC_ENABLE_bm;                      // start ADC using DMA   

   PMIC.CTRL |= PMIC_HILVLEN_bm;                                     // enable high level interrupt
}

void StartMeasurement()
{
   DataAvailable = false;
   DMA.CH0.CTRLB = DMA_CH_TRNINTLVL_HI_gc; // generate high level interrupt when transaction complete
}

// DMA channel 0; indicate that a sinus period of the DAC has finished
// this is used to start a new measurement
ISR (DMA_CH0_vect)
{
   DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;
   DMA.CH2.CTRLA |= DMA_CH_ENABLE_bm;

   DMA.CH0.CTRLB |= DMA_CH_TRNIF_bm;
   DMA.CH0.CTRLB = 0; // switch this interrupt off
}

uint8_t MeasurementReady()
{
   return DataAvailable == true;
}

// DMA channel 2; indicate that a measurement has finished
ISR (DMA_CH2_vect)
{
   DMA.CH2.CTRLB |= DMA_CH_TRNIF_bm;   
   DataAvailable = true;  
}

int16_t* GetReferenceData()
{
   return ReferenceData;
}

int16_t* GetMeasuredData()
{
   return MeasuredData;
}

uint8_t GetDataSize()
{
   return DATA_SIZE;
}
