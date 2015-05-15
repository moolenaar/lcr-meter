/* measurement.h
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

#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include "general.h"
#include "generator.h"

#define CORRECTION_VALUES 32
#define CORRECTION_STEP 95
#define DAC_AMPLITUDE 1230

enum MeasurementStateType { MeasurementStateError = 0, MeasurementStateOpen, MeasurementStateValid };
enum ComponentType { TypeUndetermined, TypeResistor, TypeCapacitor, TypeInductor };

typedef struct 
{
   float ResistanceSerial;      // resistance in series with DUT
   int16_t AdcOffset;           // Offset of the ADC
   int16_t DacOffset;           // Offset of the DAC   
} CalibrationStruct_t;

typedef struct
{
   int8_t MeasurementRange;         // multiplexer value that selects the reference resistor
   enum FrequencyIndex Index;       // measurment frequency
   int average;                     // number of averages
   float Reference;                 // the reference (DAC output) amplitude
   float Measured;                  // the measured value after the voltage divider
   float Value;                     // calculated value based on the Reference and Measured values
   enum ComponentType Component;    // component type being measured
   enum MeasurementStateType state; // if the measurement is valid or not
} MeasurementStruct_t;

typedef struct
{
   float Value;                     // calculated value based on the Reference and Measured values
   enum ComponentType Component;    // component type being measured
   enum MeasurementStateType state; // if the measurement is valid or not
   enum FrequencyIndex Index;
} MeasurementResult_t;

extern CalibrationStruct_t Calibration;

extern void MeasurementSetup();
extern void MeasurementTask();
extern MeasurementResult_t* GetMeasurementValue();
extern uint8_t GetBatteryValue();
extern void DoAveragedMeasurement(MeasurementStruct_t* meas, int8_t nrAverages);
extern void DoMeasurement(MeasurementStruct_t* meas);

#endif
