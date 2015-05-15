/* kernel.h
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

#ifndef KERNEL_H
#define KERNEL_H

#include "general.h"

#define NRTASKS 7
#define MINIMUMSTACKSIZE 80

typedef void (*TaskFunction)(void);

enum TaskState_t
{
   StateStopped = 0x10,
   StateWaiting = 0x20,
   StatePriority = 0x30,
   StateRunable = 0x40
};

extern void KernelSetup();
extern uint8_t InitTask(uint16_t stackSize, uint8_t* stackBuffer, TaskFunction function);
extern void StartKernel(TaskFunction function);
extern void TaskSleep(uint16_t time);
extern void TaskSleep();
extern void TaskStart(uint8_t index);
extern void TaskStop(uint8_t index);
//extern void DisableTaskSwitching();
extern void EnableTaskSwitching();

#endif // KERNEL_H
