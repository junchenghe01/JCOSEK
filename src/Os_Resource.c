// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Resource.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Resource Management (Priority Ceiling Protocol)
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
static ResourceControlBlock* Os_Internal_GetResourceByResId (ResourceType ResID);
static void                  Os_Internal_EnqueueReadyTaskHead (TaskControlBlock* pTcb, PriorityType originalPriority);
/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
StatusType GetResource (ResourceType ResID)
{
    /* 1. Basic Parameter Validation */
    if (ResID >= RES_MAX || ResID == 0)
    {
        return E_OS_ID;
    }

    /* 2. Retrieve Resource Control Block */
    ResourceControlBlock* res = Os_Internal_GetResourceByResId (ResID);

    /**
     * 3. Check for Resource Occupancy.
     * [NOTE]: Per OSEK spec, a task should not be blocked here.
     * If the resource is already held by another task, it is an application error.
     */
    if (res->owner != NULL && res->owner != ocb.pCurrentTask->TaskID)
    {
        /**
         * [FIXME]: In OSEK, if the ceiling protocol is configured correctly,
         * this condition should technically never be met for tasks.
         */
        return E_OS_ACCESS;
    }

    /* 4. Acquire Resource and Save Pre-acquisition State */
    res->owner            = ocb.pCurrentTask->TaskID;
    res->originalPriority = ocb.pCurrentTask->CurrentPriority;
    res->nestingCount++;
    ocb.pCurrentTask->ResourcesCount++;

    /**
     * 5. Priority Promotion (Ceiling Protocol).
     * If the resource's ceiling priority is higher than the current task's priority,
     * promote the task immediately to prevent preemption by intermediate priority tasks.
     */
    if (res->ceilingPriority > ocb.pCurrentTask->CurrentPriority)
    {
        ocb.pCurrentTask->CurrentPriority = res->ceilingPriority;

        /**
         * Re-position the task in the ready queue.
         * It moves to the HEAD of the new priority level.
         */
        Os_Internal_EnqueueReadyTaskHead (ocb.pCurrentTask, res->originalPriority);
    }

    return E_OK;
}

StatusType ReleaseResource (ResourceType ResID)
{
    /* 1. Basic Parameter Validation */
    if (ResID >= RES_MAX || ResID == 0)
    {
        return E_OS_ID;
    }

    /* 2. Retrieve Resource Control Block */
    ResourceControlBlock* res = Os_Internal_GetResourceByResId (ResID);

    /* 3. Ownership and Nesting Verification */
    if (res->owner != ocb.pCurrentTask->TaskID)
    {
        return E_OS_NOFUNC; /* Task does not own this resource */
    }
    PriorityType _currentPriority = 0;

    if (res->nestingCount == 0)
    {
        return E_OS_NOFUNC; /* Internal Error: Nesting count underflow */
    }

    /* 4. Update Resource and Task Counters */
    res->nestingCount--;
    if (ocb.pCurrentTask->ResourcesCount == 0)
    {
        return E_OS_NOFUNC; /* Internal Error: Task resource count underflow */
    }
    ocb.pCurrentTask->ResourcesCount--;

    /* 5. Complete Release Logic */
    if (res->nestingCount == 0)
    {
        /* Fully release the resource ownership */
        res->owner = NULL;

        /**
         * [TODO (HE Juncheng/2026-03-15)]: Wake up waiting tasks if implementing a non-PCP blocking model.
         * Note: Standard OSEK PCP does not use waiting lists for resources.
         */
        // WakeHighestWaitingTask(res);

        /**
         * 6. Priority Restoration.
         * Demote the task's priority back to its original level.
         */
        _currentPriority                  = ocb.pCurrentTask->CurrentPriority;
        ocb.pCurrentTask->CurrentPriority = res->originalPriority;

        /**
         * Re-insert the task into the ready queue at the new (lower) priority.
         * The 'flag=1' indicates a priority-change-driven requeue operation.
         */
        Os_Internal_EnqueueReadyTask (ocb.pCurrentTask, _currentPriority, 1);
    }

    /**
     * 7. Reschedule immediately.
     * Since the task's priority has likely decreased, other higher-priority
     * tasks may now be eligible to preempt.
     */
    Schedule ();

    return E_OK;
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Finds the Resource Control Block (RCB) associated with a given ResID.
 * @details Performs a linear search through the resource container in the
 *          OSEK Control Block (OCB).
 *
 * @param[in]  ResID  The unique identifier of the resource to locate.
 *
 * @return ResourceControlBlock*
 * @retval Pointer to the found RCB if successful.
 * @retval NULL if no resource matches the provided ResID.
 */
static ResourceControlBlock* Os_Internal_GetResourceByResId (ResourceType ResID)
{
    /* Iterate through the resource array to find the matching ID */
    for (uint32 i = 0; i < ocb.ResourcesSize; i++)
    {
        if (ocb.Resources[i].resID == ResID)
        {
            /* Resource found, return its memory address */
            return &ocb.Resources[i];
        }
    }

    /**
     * [FIXME]: Function reaches here if ResID is not found.
     * Added an explicit NULL return to prevent undefined behavior.
     */
    return NULL;
}

/**
 * @brief  Enqueues a task at the HEAD of the ready queue and updates the priority bitmap.
 * @details Typically used during Resource Release or Priority Promotion (PCP).
 *          It ensures the task is the first to be scheduled within its priority level.
 *
 * @param[in] pTcb              Pointer to the Task Control Block (TCB) to be moved.
 * @param[in] originalPriority  The priority level the task is currently occupying.
 */
static void Os_Internal_EnqueueReadyTaskHead (TaskControlBlock* pTcb, PriorityType originalPriority)
{
    if (pTcb == NULL)
        return;

    uint8 prio = pTcb->CurrentPriority;

    /* --- STEP 1: Remove Task from the Original Priority Queue --- */
    if (ocb.ReadyQueues[originalPriority] == pTcb && pTcb->NextReady == pTcb)
    {
        /* Case A: Only one task exists at this priority. Clear queue and bitmap bit. */
        ocb.ReadyQueues[originalPriority] = NULL;
        uint32 group                      = originalPriority / 32;
        uint32 bit                        = originalPriority % 32;
        ocb.ReadyBitmap[group] &= ~(1 << bit);
    }
    else
    {
        /* Case B: Other tasks remain. Standard removal from the doubly-linked list. */
        pTcb->PrevReady->NextReady = pTcb->NextReady;
        pTcb->NextReady->PrevReady = pTcb->PrevReady;

        /* If the task was the head pointer, move the head to the next task. */
        if (ocb.ReadyQueues[originalPriority] == pTcb)
        {
            ocb.ReadyQueues[originalPriority] = pTcb->NextReady;
        }
    }

    /* Reset TCB pointers to avoid dangling references */
    pTcb->NextReady = pTcb;
    pTcb->PrevReady = pTcb;

    /* --- STEP 2: Insert Task into the New Priority Queue (at the Head) --- */

    /* 2.1 Update the Priority Bitmap for the target priority */
    uint32 group = prio / 32;
    uint32 bit   = prio % 32;
    ocb.ReadyBitmap[group] |= (1 << bit);

    /* 2.2 Insert into the doubly-linked circular list */
    if (ocb.ReadyQueues[prio] == NULL)
    {
        /* Case A: Target queue is empty. Task becomes the sole head. */
        ocb.ReadyQueues[prio] = pTcb;
        pTcb->NextReady       = pTcb;
        pTcb->PrevReady       = pTcb;
    }
    else
    {
        /* Case B: Target queue already has tasks. */
        TaskControlBlock* head = ocb.ReadyQueues[prio];
        TaskControlBlock* tail = head->PrevReady;

        /* Link TCB between the current tail and head */
        pTcb->NextReady = head;
        pTcb->PrevReady = tail;
        tail->NextReady = pTcb;
        head->PrevReady = pTcb;

        /**
         * [CORE LOGIC]: Force the priority head pointer to this new task.
         * This ensures it is the first task fetched by the scheduler at this level.
         */
        ocb.ReadyQueues[prio] = pTcb;
    }

    /* 2.3 Note: TaskState update (e.g., to READY) should be handled by the caller */
}
