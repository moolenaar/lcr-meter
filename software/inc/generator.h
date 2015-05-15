/* generator.h
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


#ifndef GENERATOR_H
#define GENERATOR_H

#include "general.h"

#define SIN_TABLE_SIZE 32
#define DATA_SIZE (5 * SIN_TABLE_SIZE)

typedef struct
{
   uint16_t divider;
   uint8_t NrPeriods;
   float frequency;
} GeneratorStruct_t;

enum FrequencyIndex { FREQ_50HZ, FREQ_100HZ, FREQ_200HZ, FREQ_500HZ, FREQ_1KHZ, FREQ_2KHZ, FREQ_5KHZ, FREQ_10KHZ, FREQ_20KHZ, FREQ_50KHZ, FREQ_100KHZ};

extern GeneratorStruct_t GeneratorArray[];

extern void GeneratorSetup();
extern void SetDcOutput(int16_t amplitude, int8_t adcInput);
extern void SetOutput(enum FrequencyIndex index, int16_t amplitude, int8_t adcInput);
extern void StartMeasurement();
extern uint8_t MeasurementReady();
extern float GetFrequency();
extern float GetSampleRate();
extern int16_t* GetReferenceData();
extern int16_t* GetMeasuredData();
extern uint8_t GetDataSize();

#endif /* GENERATOR_H */

