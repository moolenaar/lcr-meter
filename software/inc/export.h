/* export.h
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


#ifndef EXPORT_H
#define EXPORT_H

#include "general.h"
#include "measurement.h"
#include "uart.h"

extern void ExportRawData();
extern void ExportMeasuredData(char* str, MeasurementStruct_t* meas);
extern void DcResponse();
extern void CalibrationValues();
extern void FrequencyResponse();
extern void ExportCorrectionData(char* str, float* correctionData);
extern void ExportZeroPoint(char* str, CalibrationStruct_t* cal);
extern void ExportInt16Data(char* str, int16_t* data, int16_t size);
extern void ExportFloatData(char* str, float* data, int16_t size);
extern void ExportInt32(char* str, int32_t data);
extern void ExportFloat(char* str, float data);

#endif /* EXPORT_H */

