// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Event.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Event Management
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

/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
StatusType SetEvent (TaskType TaskID, EventMaskType Mask)
{
    /* 1. Parameter and State Validation (Extended Status) */
    if (TaskID >= ocb.TasksSize)
    {
        return E_OS_ID;
    }

    TaskControlBlock* pTcb = Os_Internal_GetTaskControlBlockByTaskID (TaskID);

    /**
     * Requirement: Only Extended Tasks support the event mechanism.
     * Basic Tasks do not have the WAITING state.
     */
    if (pTcb->TaskKind != TASK_KIND_EXTENDED_TASK)
    {
        return E_OS_ACCESS;
    }

    /* Requirement: Events cannot be set for tasks in the SUSPENDED state. */
    if (pTcb->TaskState == TASK_STATE_SUSPENDED)
    {
        return E_OS_STATE;
    }

    /**
     * 2. Core Logic: Set the event bits.
     * [TODO (HE Juncheng/2026-03-15)]: Wrap this in a critical section (Disable/Enable Interrupts)
     * to ensure the read-modify-write operation is atomic.
     */
    Os_Arch_DisableInterrupts ();

    /* Update the task's event status by ORing the current events with the mask */
    ocb.ECB->Events |= Mask;

    /**
     * 3. Wake-up Logic: Check if the task should transition from WAITING to READY.
     * If the task is waiting and at least one of its awaited events is now set.
     */
    if (pTcb->TaskState == TASK_STATE_WAITING)
    {
        /* Check if the intersection of set events and awaited events is non-zero */
        if ((ocb.ECB->Events & pTcb->WaitMask) != 0)
        {
            /**
             * Transition the task to READY.
             * Use the standard enqueue function to update the scheduler bitmap
             * and append the task to its priority ready list.
             */
            Os_Internal_EnqueueReadyTask (pTcb, pTcb->CurrentPriority, 0);
            pTcb->TaskState = TASK_STATE_READY;
        }
    }

    Os_Arch_EnableInterrupts ();

    /**
     * 4. Rescheduling Point.
     * OSEK requires an immediate scheduling attempt if an event wakes up
     * a task with higher priority than the current one.
     * [TODO (HE Juncheng/2026-03-15)]: Ensure Schedule() is not called directly if inside an ISR context.
     */
    // if (Os_GetCurrentContext() != CONTEXT_ISR2) {
    Schedule ();
    // }

    return E_OK;
}

// [TODO (HE Juncheng/2026-03-15)]: SetEventAsyn

StatusType ClearEvent (EventMaskType Mask)
{
    /**
     * [TODO (HE Juncheng/2026-03-15)]: Add safety check for task kind (Extended Status).
     * Only Extended Tasks are allowed to use the event mechanism.
     * if (ocb.pCurrentTask->TaskKind != TASK_KIND_EXTENDED_TASK) return E_OS_ACCESS;
     */

    /**
     * Clear the bits defined in the Mask from the event status.
     * Performs a bitwise AND-NOT operation to reset specific event bits.
     */
    ocb.ECB->Events &= ~Mask;

    /**
     * [NOTE]: If each task had its own Event record in TCB, it would be:
     * ocb.pCurrentTask->Events &= ~Mask;
     */

    return E_OK;
}

StatusType GetEvent (TaskType TaskID, EventMaskRefType Event)
{
    /* 1. Extended Status Validation */

    /* Ensure TaskID is within valid range of the task array */
    if (TaskID >= ocb.TasksSize)
    {
        return E_OS_ID;
    }

    TaskControlBlock* pTcb = Os_Internal_GetTaskControlBlockByTaskID (TaskID);

    /* Requirement: Target task must be an Extended Task to have events */
    if (pTcb->TaskKind != TASK_KIND_EXTENDED_TASK || pTcb->WaitMask == 0)
    {
        return E_OS_ACCESS;
    }

    /* Requirement: Events are not available for tasks in the SUSPENDED state */
    if (pTcb->TaskState == TASK_STATE_SUSPENDED)
    {
        return E_OS_STATE;
    }

    /* Defensive Programming: Ensure the destination pointer is not NULL */
    if (Event == NULL)
    {
        return E_OS_VALUE;
    }

    /**
     * 2. Core Logic: Data Extraction.
     * [NOTE]: Using volatile casting to ensure the compiler reads the latest
     *         value from memory, protecting against race conditions with SetEvent.
     */
    /* Copy the current event bitmask into the user-provided memory location */
    Os_Arch_DisableInterrupts ();

    *(volatile uint32*)Event = (volatile uint32)ocb.ECB->Events;
    Os_Arch_EnableInterrupts ();

    /* 3. Finalization: GetEvent is a query-only service and not a scheduling point */
    return E_OK;
}

StatusType WaitEvent (EventMaskType Mask)
{
    TaskControlBlock* pCurTask = ocb.pCurrentTask;

    /* 1. Extended Status Validation */

    // // [TODO (HE Juncheng/2026-03-15)] Check calling context: WaitEvent must NOT be called from ISRs.
    // if (Os_GetCurrentContext() == CONTEXT_ISR2) {
    //     return E_OS_CALLEVEL;
    // }

    /* Requirement: Only Extended Tasks support the WAITING state. */
    if (pCurTask->TaskKind != TASK_KIND_EXTENDED_TASK /* || pCurTask->WaitMask == 0*/)
    {
        return E_OS_ACCESS;
    }

    /**
     * Requirement: A task must not hold any resources when calling WaitEvent.
     * This prevents potential deadlock scenarios.
     */
    if (pCurTask->ResourcesCount > 0)
    {
        return E_OS_RESOURCE;
    }

    /* 2. Core Logic: Event Self-Check */
    // [TODO (HE Juncheng/2026-03-15)]: Critical section protection needed for atomic event evaluation.
    Os_Arch_DisableInterrupts ();

    /* Update the task's WaitMask to reflect the events it is currently expecting */
    pCurTask->WaitMask |= Mask;

    /**
     * Check if any of the requested events in the 'Mask' are already set
     * in the task's "event inbox".
     */
    if ((ocb.ECB->Events & Mask) == 0)
    {
        /* --- Scenario: Condition NOT met, task must BLOCK --- */

        /**
         * 3. State Transition: Remove from Ready System.
         * Physically remove the TCB from the scheduler's bitmap and circular list.
         */
        Os_Internal_Task_RemoveFromReady (pCurTask);

        /* Update task state to WAITING */
        pCurTask->TaskState = TASK_STATE_WAITING;

        /**
         * 4. Invoke Scheduler: Switch to the next highest priority READY task.
         * Current task context is saved here via Os_ContextSwitch.
         */
        Schedule ();

        /**
         * Execution resumes here when the task is woken up by SetEvent()
         * and regains CPU control via the scheduler.
         */
    }
    else
    {
        /* --- Scenario: Condition already met, NO BLOCKING required --- */
        // pCurTask->eventCB->waitMask = 0;
    }

    Os_Arch_EnableInterrupts ();

    return E_OK;
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
