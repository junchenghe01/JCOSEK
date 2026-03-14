// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Alarm.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Alarm management
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
static AlarmControlBlock* Os_Internal_GetAlarmControlBlockByAlarmID (AlarmType AlarmID);
/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
StatusType GetAlarmBase (AlarmType AlarmID, AlarmBaseRefType Info)
{
    /**
     * Implementing GetAlarmBase is a fundamental service for the OSEK Alarm module.
     * Its core task is to inform the user about the physical characteristics
     * (e.g., max value, ticks) of the Counter bound to the alarm.
     */

    /* 1. Extended Status Validation */

    /**
     * Check if AlarmID is within a valid range.
     * Note: This assumes AlarmID corresponds directly to the array index.
     */
    if (AlarmID >= ocb.AlarmsSize)
    {
        return E_OS_ID;
    }

    /* Defensive programming: Ensure the destination pointer is valid */
    if (Info == NULL)
    {
        return E_OS_VALUE;
    }

    /* 2. Core Logic: Retrieve traits of the counter linked to the alarm */

    /**
     * Alarms are statically bound to a specific Counter during initialization.
     * Extract constant information from the counter's configuration.
     *
     * [TODO (HE Juncheng/2026-03-15)]: Optimization suggestion: Store the Counter pointer directly in
     * AlarmControlBlock to avoid repeated lookups.
     */
    CounterControlBlock* pCounter = Os_Internal_GetCounterIdByAlarmID (AlarmID);

    /* 3. Data Transfer (Copying static counter attributes) */

    /**
     * [NOTE]: These are typically static constants;
     * no interrupt masking is required for this copy operation.
     */
    Info->maxallowedvalue = pCounter->maxallowedvalue;
    Info->ticksperbase    = pCounter->ticksperbase;
    Info->mincycle        = pCounter->mincycle;

    return E_OK;
}

StatusType GetAlarm (AlarmType AlarmID, TickRefType Tick)
{
    /**
     * The core logic of GetAlarm is calculating the remaining time.
     * Per OSEK spec 13.6.2.2, this service returns the number of ticks
     * until the alarm expires.
     */

    /* 1. Basic Validation (Extended Status) */
    if (AlarmID >= ocb.AlarmsSize)
    {
        return E_OS_ID;
    }

    if (Tick == NULL)
    {
        return E_OS_VALUE;
    }

    /* Retrieve the Alarm Control Block (ACB) */
    AlarmControlBlock* pAlarmCB = Os_Internal_GetAlarmControlBlockByAlarmID (AlarmID);

    /* 2. State Verification (Standard/Extended Status) */
    /* If the alarm is not running, return E_OS_NOFUNC per specification */
    if (pAlarmCB->IsActive == FALSE)
    {
        return E_OS_NOFUNC;
    }

    /* 3. Core Calculation: Remaining Ticks */

    /**
     * [TODO (HE Juncheng/2026-03-15)]: Interrupts should be disabled during calculation to prevent
     * the Counter from incrementing (Tick-drift) mid-process.
     */
    Os_Arch_DisableInterrupts ();

    /* Retrieve the bound Counter Control Block */
    CounterControlBlock* pCounter    = Os_Internal_GetCounterIdByAlarmID (AlarmID);
    TickType             currentTick = pCounter->CurrentValue;
    TickType             expireTick  = pAlarmCB->AlarmTime;
    TickType             maxVal      = pCounter->maxallowedvalue;

    /**
     * Calculate the delta: expireTick - currentTick.
     * Logic must account for Counter Rollover (wrapping around zero).
     */
    if (expireTick >= currentTick)
    {
        /* Standard case: Expiration is within the current cycle */
        *Tick = expireTick - currentTick;
    }
    else
    {
        /**
         * Rollover case: Expiration has wrapped around the maxallowedvalue.
         * Remaining = (Max - Current) + Expire + 1
         */
        *Tick = (maxVal - currentTick) + expireTick + 1;
    }

    Os_Arch_EnableInterrupts ();

    return E_OK;
}

StatusType SetRelAlarm (AlarmType AlarmID, TickType increment, TickType cycle)
{
    /* 1. Extended Status Validation */
    if (AlarmID >= ocb.AlarmsSize)
    {
        return E_OS_ID;
    }

    AlarmControlBlock*   pAlarmCB = Os_Internal_GetAlarmControlBlockByAlarmID (AlarmID);
    CounterControlBlock* pCounter = Os_Internal_GetCounterIdByAlarmID (AlarmID);

    /* Requirement: Increment must not be zero or exceed the counter's max value */
    if ((increment == 0U) || (increment > pCounter->maxallowedvalue))
    {
        return E_OS_VALUE;
    }

    /* Requirement: If cycle is non-zero, it must be within [mincycle, maxallowedvalue] */
    if (cycle != 0U)
    {
        if ((cycle < pCounter->mincycle) || (cycle > pCounter->maxallowedvalue))
        {
            return E_OS_VALUE;
        }
    }

    /* 2. State Verification: Check if the alarm is already in use */
    if (pAlarmCB->IsActive == TRUE)
    {
        return E_OS_STATE;
    }

    /* 3. Core Logic: Determine Absolute Expiration Point (ExpireValue) */
    Os_Arch_DisableInterrupts ();

    // TickType now = pCounter->CurrentValue;
    // TickType max = pCounter->maxallowedvalue;

    // // Calculate absolute expiration time, handling overflow/rollover
    // // Formula: ExpireValue = (CurrentValue + increment) % (maxallowedvalue + 1)
    // if ((max - now) >= increment) {
    //     pAlarmCB->AlarmTime = now + increment;
    // } else {
    //     // Rollover: Current value is close to max, increment wraps around zero
    //     pAlarmCB->AlarmTime = increment - (max - now) - 1U;
    // }

    /**
     * 4. Update Control Block State
     * [NOTE]: In this implementation, AlarmTime represents "remaining ticks"
     * rather than an absolute point in time.
     */
    pAlarmCB->AlarmTime = increment;
    pAlarmCB->CycleTime = cycle;
    pAlarmCB->IsActive  = TRUE;

    Os_Arch_EnableInterrupts ();

    return E_OK;
}

StatusType SetAbsAlarm (AlarmType AlarmID, TickType start, TickType cycle)
{
    /* 1. Basic Validation (Extended Status) */
    if (AlarmID >= ocb.AlarmsSize)
    {
        return E_OS_ID;
    }

    /* Retrieve associated Control Blocks */
    AlarmControlBlock*   pAlarmCB = Os_Internal_GetAlarmControlBlockByAlarmID (AlarmID);
    CounterControlBlock* pCounter = Os_Internal_GetCounterIdByAlarmID (AlarmID);
    TickType             max      = pCounter->maxallowedvalue;

    /* Requirement: Start value must not exceed the counter's maximum allowed value */
    if (start > max)
    {
        return E_OS_VALUE;
    }

    /* Requirement: If cycle is non-zero, it must be within [mincycle, maxallowedvalue] */
    if (cycle != 0U)
    {
        if ((cycle < pCounter->mincycle) || (cycle > max))
        {
            return E_OS_VALUE;
        }
    }

    /* 2. State Verification: Check if the alarm is already in use */
    if (pAlarmCB->IsActive == TRUE)
    {
        return E_OS_STATE;
    }

    /* 3. Core Logic: Set the absolute expiration point */
    Os_Arch_DisableInterrupts ();

    /**
     * Per OSEK specification: If the 'start' value is already in the past,
     * the alarm should expire at the next occurrence of that value.
     *
     * In this design, since the underlying check compares (CurrentValue == ExpireValue),
     * we record the absolute 'start' value directly.
     */
    pAlarmCB->AlarmTime = start;
    pAlarmCB->CycleTime = cycle;
    pAlarmCB->IsActive  = TRUE;

    Os_Arch_EnableInterrupts ();

    return E_OK;
}

StatusType CancelAlarm (AlarmType AlarmID)
{
    /* 1. Extended Status Validation */
    if (AlarmID >= ocb.AlarmsSize)
    {
        return E_OS_ID;
    }

    /* Retrieve the Alarm Control Block (ACB) */
    AlarmControlBlock* pAlarmCB = Os_Internal_GetAlarmControlBlockByAlarmID (AlarmID);

    /* 2. State Verification (Standard Status) */
    /* Per OSEK spec: If the alarm is not running, return E_OS_NOFUNC */
    if (pAlarmCB->IsActive == FALSE)
    {
        return E_OS_NOFUNC;
    }

    /* 3. Core Logic: Cancel the Alarm (Atomic Operation) */
    /**
     * Interrupts must be disabled to prevent the Os_Counter_Tick
     * (or equivalent) from accessing the ACB while it's being cleared.
     */
    Os_Arch_DisableInterrupts ();

    /* Set the state to inactive */
    pAlarmCB->IsActive = FALSE;

    /**
     * Reset key values (Optional but recommended for safety and easier debugging)
     */
    pAlarmCB->AlarmTime = 0U;
    pAlarmCB->CycleTime = 0U;

    Os_Arch_EnableInterrupts ();

    return E_OK;
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Finds the Alarm Control Block (ACB) associated with a given AlarmID.
 * @details Performs a linear search through the alarm container within the
 *          OSEK Control Block (OCB).
 *
 * @param[in] AlarmID  The unique identifier of the alarm to locate.
 *
 * @return AlarmControlBlock*
 * @retval Pointer to the found ACB if successful.
 * @retval NULL if no alarm matches the provided AlarmID.
 */
static AlarmControlBlock* Os_Internal_GetAlarmControlBlockByAlarmID (AlarmType AlarmID)
{
    /* Iterate through the alarm array to find the matching ID */
    for (uint32 i = 0; i < ocb.AlarmsSize; i++)
    {
        if (ocb.ACB[i].AlarmID == AlarmID)
        {
            /* Return the memory address of the specific Alarm Control Block */
            return &ocb.ACB[i];
        }
    }

    /**
     * [FIXME]: Implicitly returns undefined if no match is found.
     * Added explicit return NULL to ensure kernel stability.
     */
    return NULL;
}
