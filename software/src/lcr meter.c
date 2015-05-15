/* lcr_meter.c
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

#include <avr/io.h>
#include <avr/fuse.h>

#include "kernel.h"
#include "measurement.h"
#include "display.h"
#include "uart.h"
#include "generator.h"

FUSES =
{
   .FUSEBYTE0 = 0xFF,
   .FUSEBYTE1 = 0x00,
   .FUSEBYTE2 = 0xFF,
   .FUSEBYTE4 = 0xFE,
   .FUSEBYTE5 = 0xFF
};

uint8_t MeasurementStack[MINIMUMSTACKSIZE + 120];
uint8_t LcdStack[MINIMUMSTACKSIZE + 120];

void cpuInit()
{
   // setup external crystal oscillator 4 MHz
   OSC.XOSCCTRL = OSC_FRQRANGE_2TO9_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
   CCP = CCP_IOREG_gc;
   OSC.CTRL = OSC_XOSCEN_bm | OSC_RC2MEN_bm;
   while(0 == (OSC.STATUS & OSC_XOSCRDY_bm));
   
   // setup PLL
   CCP = CCP_IOREG_gc;
   OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 8;
   CCP = CCP_IOREG_gc;
   OSC.CTRL = OSC_XOSCEN_bm | OSC_PLLEN_bm | OSC_RC2MEN_bm;
   while(0 == (OSC.STATUS & OSC_PLLRDY_bm));

   // switch over to PLL
   CCP = CCP_IOREG_gc;
   CLK.CTRL = CLK_SCLKSEL2_bm;

   // lock clock configuration
   CCP = CCP_IOREG_gc;
   CLK.LOCK = CLK_LOCK_bm;

   // switch off 2MHz RC oscillator used for booting
   CCP = CCP_IOREG_gc;
   OSC.CTRL = OSC_XOSCEN_bm | OSC_PLLEN_bm; 
}

void initFunc()
{
   UartWrite("LCR\n\r");
}

int main(void)
{
   cpuInit();
   DisplaySetup();
   KernelSetup();
   UartSetup();
   GeneratorSetup();
   MeasurementSetup();

   InitTask(sizeof(LcdStack), LcdStack, LcdTask);
   InitTask(sizeof(MeasurementStack), MeasurementStack, MeasurementTask);

   StartKernel(initFunc);
}
