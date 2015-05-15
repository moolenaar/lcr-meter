/* lcd.h
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

#ifndef LCD_H
#define LCD_H

#include "general.h"
#include "font.h"

extern uint8_t LcdDisplay[4][128];

extern void LcdSetup();
extern void LcdInit();
extern void LcdClear();
extern void LcdSetPosition(uint8_t page, uint8_t horizontal);
extern void LcdWriteBuffer(uint8_t size, uint8_t* buffer);

#endif
