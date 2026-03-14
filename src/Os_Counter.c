// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Counter.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Counter management
 *
 * Conformance Class: ECC2 (Please update as per actual implementation)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org.
 *
 * @author Juncheng HE
 * @date   2026-02-13
 * @copyright Copyright (c) 2026 Juncheng HE. All rights reserved.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */
#include "Os.h"
#include "Os_Internal.h"

/* ============================================================================
 * Macros & Constants
 * ========================================================================= */

/* ============================================================================
 * Local Types/Structs
 * ========================================================================= */

/* ============================================================================
 * Static Global Variables
 * ========================================================================= */

/* ============================================================================
 * Static Function Prototypes
 * ========================================================================= */
static void                 Os_Internal_IsrExitSchedule (void);
static CounterControlBlock* Os_Internal_GetCounterIdByCounterID (CounterType CounterID);
static void                 Os_Internal_CheckAlarms (CounterType CounterID);
/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
StatusType IncrementCounter (CounterType CounterID)
{
    /* 1. Validation of CounterID and Counter Type */
    /**
     * [NOTE]: Hardware counters (e.g., System Tick) are usually updated via
     * ISRs and cannot be incremented through this API.
     */
    if (CounterID >= ocb.CountersSize /* || Counter_Config[CounterID].IsHardware*/)
    {
        return E_OS_ID;
    }

    /* Retrieve the Counter Control Block */
    CounterControlBlock* pCounter = Os_Internal_GetCounterIdByCounterID (CounterID);

    /**
     * 2. Atomic Update: Protect data consistency during counter manipulation.
     * Use critical section to prevent race conditions with Alarms.
     */
    // OS_ENTER_CRITICAL();
    Os_Arch_DisableInterrupts ();

    /* Physical increment of the counter value */
    pCounter->CurrentValue++;

    /* 3. Handle Rollover: Reset to 0 if current value exceeds maxallowedvalue */
    if (pCounter->CurrentValue > pCounter->maxallowedvalue)
    {
        pCounter->CurrentValue = 0;
    }

    /**
     * 4. Alarm Processing: Check the alarm queue bound to this counter.
     * Typically, the OS maintains alarms in a list sorted by expiration time.
     */
    Os_Internal_CheckAlarms (CounterID);

    // OS_EXIT_CRITICAL();
    Os_Arch_EnableInterrupts ();

    /**
     * 5. Rescheduling Point.
     * If called from a task context and an alarm activates a higher priority task,
     * a reschedule is required.
     */
    // Os_Schedule();

    /**
     * Check if preemption is needed after the potential alarm actions.
     * Note: This internal helper handles ISR-level or Task-level rescheduling logic.
     */
    Os_Internal_IsrExitSchedule ();

    return E_OK;
}

StatusType GetCounterValue (CounterType CounterID, TickRefType Value)
{
    /* 1. Extended Status Validation */
    if (CounterID >= ocb.CountersSize)
        return E_OS_ID;

    /* Defensive programming: Ensure the destination pointer is valid */
    if (Value == NULL)
        return E_OS_VALUE;

    /* 2. Retrieve the Counter Control Block (CCB) */
    CounterControlBlock* counterCB = Os_Internal_GetCounterIdByCounterID (CounterID);

    /**
     * 3. Data Extraction.
     * [NOTE]: For 32-bit values on a 32-bit CPU, this read is typically atomic.
     * If the counter can be updated by hardware in the background,
     * consider using a volatile cast or a temporary critical section.
     */
    *Value = counterCB->CurrentValue;
    return E_OK;
}

StatusType GetElapsedValue (CounterType CounterID, TickRefType Value, TickRefType ElapsedValue)
{
    StatusType status = E_NOT_OK;

    /* 1. [SWS_Os_00381] Validate CounterID (Extended Status only) */
    if (CounterID >= ocb.CountersSize)
        return E_OS_ID;

    /* Retrieve the Counter Control Block and capture the current tick */
    CounterControlBlock* counterCB   = Os_Internal_GetCounterIdByCounterID (CounterID);
    TickType             currentTick = counterCB->CurrentValue;

    /* 2. [SWS_Os_00391] Ensure the input reference value is within allowed range */
    if (*Value > counterCB->maxallowedvalue)
    {
        return E_OS_VALUE;
    }

    /**
     * 3. [SWS_Os_00382] Calculate the delta (ElapsedValue).
     * Logic: Account for counter rollover/overflow.
     * Example: Max=1000, Previous=990, Current=10 -> Diff = (1000 - 990) + 10 + 1 = 21.
     * General Formula:
     * diff = (Current >= Previous) ? (Current - Previous) : (Max - Previous + Current + 1)
     */
    if (currentTick >= *Value)
    {
        /* Standard case: No rollover occurred since the last reference */
        *ElapsedValue = currentTick - *Value;
    }
    else
    {
        /**
         * Rollover case: The counter has wrapped around the zero point.
         * The '+ 1U' accounts for the tick transition from Max to 0.
         */
        *ElapsedValue = (counterCB->maxallowedvalue - *Value) + currentTick + 1U;
    }

    /* 4. [SWS_Os_00460] Update the 'Value' parameter with current tick for the next call */
    *Value = currentTick;

    /* 5. [SWS_Os_00382] Calculation complete */
    status = E_OK;

    return status;
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
CounterControlBlock* Os_Internal_GetCounterIdByAlarmID (AlarmType AlarmID)
{
    CounterType counterID = 0;

    /* Step 1: Search for the CounterID associated with the provided AlarmID */
    for (uint32 i = 0; i < ocb.AlarmsSize; i++)
    {
        if (ocb.ACB[i].AlarmID == AlarmID)
        {
            counterID = ocb.ACB[i].CounterID;
            break;
        }
    }

    /* Step 2: Search for the Counter Control Block using the CounterID found in Step 1 */
    for (uint32 i = 0; i < ocb.CountersSize; i++)
    {
        if (ocb.CCB[i].CounterID == counterID)
        {
            /* Return the pointer to the CCB */
            return &(ocb.CCB[i]);
            // break;
        }
    }

    /**
     * [FIXME]: Implicitly returns undefined if no match is found.
     * Added explicit return NULL for safety.
     */
    return NULL;
}

/**
 * @brief  Finds the Counter Control Block (CCB) associated with a specific CounterID.
 * @details Iterates through the counter container in the OSEK Control Block (OCB)
 *          to locate the matching CCB.
 *
 * @param[in] CounterID  The unique identifier of the counter to locate.
 *
 * @return CounterControlBlock*
 * @retval Pointer to the found CCB if successful.
 * @retval NULL if no counter matches the provided CounterID.
 */
static CounterControlBlock* Os_Internal_GetCounterIdByCounterID (CounterType CounterID)
{
    /* Iterate through the counter array to find the matching ID */
    for (uint32 i = 0; i < ocb.CountersSize; i++)
    {
        if (ocb.CCB[i].CounterID == CounterID)
        {
            /* Return the memory address of the corresponding CCB */
            return &(ocb.CCB[i]);
            // break;
        }
    }

    /**
     * [FIXME]: Function reaches here if CounterID is not found.
     * Recommended to return NULL to prevent undefined behavior in the caller.
     */
    return NULL;
}

/**
 * @brief  Updates and checks all alarms associated with a specific counter.
 * @details This function is invoked whenever a counter increments. It decrements
 *          the remaining time for each active alarm and triggers the configured
 *          action upon expiration.
 *
 * @param[in] CounterID  The identifier of the counter that has just incremented.
 *
 * @note   Execution Context: Usually called within the System Tick ISR or
 *         the IncrementCounter service.
 */
static void Os_Internal_CheckAlarms (CounterType CounterID)
{
    for (int i = 0; i < ocb.AlarmsSize; i++)
    {
        AlarmControlBlock*   pAlarm   = &ocb.ACB[i];
        CounterControlBlock* pCounter = Os_Internal_GetCounterIdByCounterID (CounterID);

        /* Only process active alarms that are bound to this specific counter */
        if (pAlarm->IsActive && (pAlarm->CounterID == CounterID))
        {
            /**
             * Core Logic: Relative Countdown.
             * Decrement the remaining ticks until expiration.
             */
            if (pAlarm->AlarmTime > 0)
            {
                pAlarm->AlarmTime--;
            }

            /**
             * [DEBUG NOTE]: Comparing with 'pCounter->CurrentValue == pAlarm->AlarmTime'
             * can be risky during heavy interrupt loads or task preemption.
             * Using the 'decrement to zero' approach is more robust for software timers.
             */
            // if (pCounter->CurrentValue == pAlarm->AlarmTime) {

            /* 2. Check if the alarm has expired (remaining ticks reached 0) */
            if (pAlarm->AlarmTime == 0)
            {
                /* 5. Trigger the configured Alarm Action (Task/Event/Callback) */
                Os_Internal_TriggerAlarmAction (pAlarm);

                /**
                 * [UNIMPLEMENTED]: Absolute time recalculation for cycle handling.
                 * This logic handles rollover manually without external math libraries.
                 */
                // if (pAlarm->CycleTime > 0) {
                //     // Recalculate the next expiration time (logic is the same as SetRelAlarm).
                //     // pAlarm->AlarmTime = (pAlarm->AlarmTime + pAlarm->CycleTime) % (pCounter->maxallowedvalue + 1);
                //     /* This syntax does not depend on any library and is applicable to any max value. */
                //     pAlarm->AlarmTime += pAlarm->CycleTime;
                //     if (pAlarm->AlarmTime > pCounter->maxallowedvalue) {
                //         // Handling wraparound: Subtract (max + 1)
                //         pAlarm->AlarmTime -= (pCounter->maxallowedvalue + 1);
                //     }
                // } else {
                //     pAlarm->IsActive = FALSE; // Close after single alarm execution
                // }

                /* 3. Handle periodic alarms (Cycle) */
                if (pAlarm->CycleTime > 0)
                {
                    /* Reset the countdown to the cycle value for the next period */
                    pAlarm->AlarmTime = pAlarm->CycleTime;
                }
                else
                {
                    /* Single-shot alarm: deactivate after the first trigger */
                    pAlarm->IsActive = FALSE;
                }
            }
        }
    }
}

/**
 * @brief  Handles task rescheduling upon exiting an Interrupt Service Routine (ISR).
 * @details This function implements the OSEK "Rescheduling at ISR end" logic.
 *          It checks if a higher priority task was readied during the ISR
 *          (e.g., via SetEvent or ActivateTask) and sets a preemption flag.
 *
 * @note   Compliance: OSEK OS 2.2.3, Section 4.6 (Rescheduling)
 * @warning This must be called only at the end of Category 2 ISRs.
 */
static void Os_Internal_IsrExitSchedule (void)
{
    /**
     * 1. Only allow rescheduling at the exit of the outermost interrupt.
     *    Nested interrupts must return to the previous interrupt level first.
     */
    if (ocb.IntNestingCount == 1)
    {
        /* Determine the highest priority task currently in the READY state */
        uint16 highestPrio = Os_Internal_GetHighestPriorityIndex ();

        /**
         * [PANIC]: If no tasks are ready (even the Idle task), this is a critical
         * kernel bug. Entering a debug hang state.
         */
        if (highestPrio == 0xFFFF)
        {
            // while(1); // Kernel crash infinite loop
            while (1)
            {
                /* [DEBUG]: Check GIC Interrupt Acknowledge Register (IAR) */
                uint32 iar = *(volatile uint32*)(0x1e00010c);
                if (iar != 1023)
                {
                    /* IRQ is pending but failed to jump to the vector table. */
                }
            }
        }

        /**
         * 2. Check if a higher priority task is waiting to preempt
         *    the currently interrupted task.
         */
        if (highestPrio > ocb.pCurrentTask->CurrentPriority)
        {
            /**
             * 3. Mark the system for "Post-ISR Preemption".
             *    Note: We cannot call the standard Schedule() here because the
             *    CPU is still in IRQ mode and using the IRQ stack.
             */
            TaskControlBlock* pNextTask = ocb.ReadyQueues[highestPrio];

            /**
             * [UNIMPLEMENTED]: Trigger architecture-specific preemption.
             * On ARM Cortex-A, this typically involves modifying LR_irq or
             * triggering a Software Interrupt (SWI/PendSV).
             */
            // Os_Arch_TriggerIsrPreemption();

            /* Set a global flag to be handled by the assembly ISR wrapper */
            ocb.preempt_flag = 1;
        }
    }
}
