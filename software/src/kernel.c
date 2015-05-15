/* kernel.c
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

#include "kernel.h"
#include <stddef.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/sleep.h>

// task stack structure:
// stack start address 0  return address low  (buffer address + buffer size - 1) 
//                     1  return address high
//                     2  R0
//                     3  SREG
//                     4  R31 - R1
//                    35  SPL
//                    36  SPH
//                    37  stack pointer value (is not used here so can be used elseware)


typedef struct 
{
   uint8_t* stackbuffer;
   volatile uint16_t StackPointer;
   uint16_t StackSize;
   uint8_t Handle;
   volatile enum TaskState_t TaskState;
   volatile uint16_t NextRunTime;
} TaskStruct_t;

volatile TaskStruct_t tasks[NRTASKS];

volatile uint8_t TaskIndex;
volatile uint8_t TaskIterator;
volatile uint16_t KernelTimerCounter;
volatile uint8_t nrTasks;

uint8_t IdleTaskStack[MINIMUMSTACKSIZE];

TaskFunction initFunction;

void IdleTask()
{
   set_sleep_mode(SLEEP_MODE_IDLE);
   if (initFunction != NULL)
   {
      initFunction();
   }

   while(true)
   {
      sleep_mode();
      TaskSleep(0);
   }
}

void KernelSetup()
{
   KernelTimerCounter = 0;
   TCC0_CTRLB = TC0_CCAEN_bm;
   TCC0_INTCTRLB = TC0_CCAINTLVL0_bm; // | TC0_CCAINTLVL1_bm;
   TCC0_PERBUF = 500 - 1;
   TCC0_PER = 500 - 1;
   PMIC.CTRL |= PMIC_RREN_bm;
   PMIC.CTRL |= PMIC_LOLVLEN_bm; // low level interrupt

   nrTasks = 0;
   InitTask(sizeof(IdleTaskStack), IdleTaskStack, IdleTask);
}

uint8_t InitTask(uint16_t stackSize, uint8_t* stackBuffer, TaskFunction function)
{
   uint8_t* stackptr = stackBuffer + stackSize - 1;
   
   *stackptr-- = (uint16_t)function & 0x00ff;
   *stackptr-- = (uint16_t)function >> 8;
   *stackptr-- = 0x00;                                         // register R0
   *stackptr-- = 0x00; nrTasks == 0 ? 0x00 : 0x80;             // status register; interrupt will be enabled later

   tasks[nrTasks].stackbuffer = stackBuffer;
   tasks[nrTasks].StackPointer = (uint16_t)stackptr - 31;      // store stack pointer
   tasks[nrTasks].StackSize = stackSize;
   tasks[nrTasks].Handle = nrTasks;
   tasks[nrTasks].TaskState = StateRunable;
   tasks[nrTasks].NextRunTime = 0;

   return nrTasks++;
}

void StartKernel(TaskFunction function)
{
   TaskIndex = 0;
   initFunction = function;
   
   EnableTaskSwitching();
   SP = tasks[TaskIndex].StackPointer;

   asm volatile(
      "jmp start \n\t"
   ::);
   // this function starts / returns the first task in the list
}

void TaskSleep(uint16_t time)
{
   // store context of current task
   asm volatile(
      "              cli               \n\t"
      "              push r0           \n\t" // r0
      "              in r0, 0x3f       \n\t" // SREG
      "              push r0           \n\t"
      "              push r1           \n\t"
      "              push r2           \n\t"
      "              push r3           \n\t"
      "              push r4           \n\t"
      "              push r5           \n\t"
      "              push r6           \n\t"
      "              push r7           \n\t"
      "              push r8           \n\t"
      "              push r9           \n\t"
      "              push r10          \n\t"
      "              push r11          \n\t"
      "              push r12          \n\t"
      "              push r13          \n\t"
      "              push r14          \n\t"
      "              push r15          \n\t"
      "              push r16          \n\t"
      "              push r17          \n\t"
      "              push r18          \n\t"
      "              push r19          \n\t"
      "              push r20          \n\t"
      "              push r21          \n\t"
      "              push r22          \n\t"
      "              push r23          \n\t"
      "              push r24          \n\t"
      "              push r25          \n\t"
      "              push r26          \n\t"
      "              push r27          \n\t"
      "              push r28          \n\t"
      "              push r29          \n\t"
      "              push r30          \n\t"
      "              push r31          \n\t"
      "              clr r1            \n\t" // reset the "zero" register
   ::);
   
   tasks[TaskIndex].StackPointer = SP;

   if (time > 0)
   {
      tasks[TaskIndex].TaskState = StateWaiting;
      tasks[TaskIndex].NextRunTime = KernelTimerCounter + time;
   }

   // find new task to run
   if (TaskIndex == 0) TaskIndex++;
   TaskIterator = TaskIndex;
   do
   {
      TaskIterator = TaskIterator >= nrTasks - 1 ? 1 : TaskIterator + 1;
   }
   while ((TaskIterator != TaskIndex) && (tasks[TaskIterator].TaskState != StateRunable));

   TaskIndex = tasks[TaskIterator].TaskState == StateRunable ? TaskIterator : 0;

   // retrieve context of next task to run
   SP = tasks[TaskIndex].StackPointer;

   asm volatile(
      "start:        pop r31          \n\t"
      "              pop r30          \n\t"
      "              pop r29          \n\t"
      "              pop r28          \n\t"
      "              pop r27          \n\t"
      "              pop r26          \n\t"
      "              pop r25          \n\t"
      "              pop r24          \n\t"
      "              pop r23          \n\t"
      "              pop r22          \n\t"
      "              pop r21          \n\t"
      "              pop r20          \n\t"
      "              pop r19          \n\t"
      "              pop r18          \n\t"
      "              pop r17          \n\t"
      "              pop r16          \n\t"
      "              pop r15          \n\t"
      "              pop r14          \n\t"
      "              pop r13          \n\t"
      "              pop r12          \n\t"
      "              pop r11          \n\t"
      "              pop r10          \n\t"
      "              pop r9           \n\t"
      "              pop r8           \n\t"
      "              pop r7           \n\t"
      "              pop r6           \n\t"
      "              pop r5           \n\t"
      "              pop r4           \n\t"
      "              pop r3           \n\t"
      "              pop r2           \n\t"
      "              pop r1           \n\t"
      "              pop r0           \n\t"
      "              out 0x3f, r0     \n\t" // SREG
      "              pop r0           \n\t" // r0
      "              sei              \n\t"
      "              ret              \n\t"
   ::);
}

void TaskStart(uint8_t index)
{
   ATOMIC_BLOCK(ATOMIC_FORCEON)
   {
      tasks[index].TaskState = StateRunable;
   }   
}

void TaskStop(uint8_t index)
{
   ATOMIC_BLOCK(ATOMIC_FORCEON)
   {
      tasks[index].TaskState = StateStopped;
   }
}

void DisableTaskSwitching()
{
   TaskSleep(0);
   TCC0_CTRLA = 0;
}

void EnableTaskSwitching()
{
   TCC0_CTRLA = TC0_CLKSEL2_bm | TC0_CLKSEL0_bm; // CLKper / 64
}

ISR (TCC0_CCA_vect)
{
   uint8_t i;
   
   KernelTimerCounter++;
   
   // update status of each task
   for (i = 0; i < nrTasks; i++)
   {
      if ((tasks[i].NextRunTime == KernelTimerCounter) &&
         (tasks[i].TaskState != StateStopped))
      {
         tasks[i].TaskState = StateRunable;
      }
   }
}
