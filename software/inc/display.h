/* display.h
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


#ifndef DISPLAY_H
#define DISPLAY_H

#include "general.h"

enum LcdCommand_t {CMD_NONE, CMD_CLEAR, CMD_DISPLAY_INTRO, CMD_DISPLAY_MAIN_SCREEN, CMD_UPDATE_MEASUREMENT, CMD_DISPLAY_COUNTER, CMD_INPUT_OPEN, CMD_MEASUREMENT_ERROR, CMD_DISPLAY_BATTERY};

extern void DisplaySetup();
extern void LcdTask();
extern void LcdCommand(enum LcdCommand_t cmd);
extern char* floatToStr(char* buffer, uint8_t before, uint8_t after, float value);
extern char* int32ToStr(char* buffer, uint8_t before, int32_t value);

#endif /* DISPLAY_H */
