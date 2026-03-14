// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Task.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Task Management
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
StatusType ActivateTask (TaskType TaskID)
{
    /* Retrieve the Task Control Block (TCB) for the given ID */
    TaskControlBlock* pTcb = Os_Internal_GetTaskControlBlockByTaskID (TaskID);

    /**
     * OSEK Rule: A task can only be activated if it is in the SUSPENDED state.
     * BCC1/ECC1 models do not support multiple activations (queuing).
     */
    if (pTcb->TaskState != TASK_STATE_SUSPENDED)
    {
        return E_OS_LIMIT;
    }

    /* Add the task to the ready queue at its current priority */
    Os_Internal_EnqueueReadyTask (pTcb, pTcb->CurrentPriority, 0);

    /* Transition the state to READY */
    pTcb->TaskState = TASK_STATE_READY;

    /**
     * Trigger the scheduler to ensure the highest priority task
     * (which might be this newly activated task) gets CPU time.
     */
    Schedule ();

    return E_OK;
}

// [UNIMPLEMENTED] ActivateTaskAsyn

StatusType TerminateTask (void)
{
    /* [UNIMPLEMENTED] Runtime Protection: Ensure current task context is valid */
    // if (ocb.pCurrentTask == NULL) return E_OS_SYS_ASSERTION;

    /* Transition current task to SUSPENDED to prevent it from being rescheduled */
    ocb.pCurrentTask->TaskState = TASK_STATE_SUSPENDED;

    /* Physically remove task from the ready queue/bitmap structure */
    Os_Internal_Task_RemoveFromReady (ocb.pCurrentTask);

    /**
     * [UNIMPLEMENTED] Os_Internal_Task_Yield: Optional pre-scheduling logic.
     * Currently bypassed to call the core Scheduler directly.
     */
    // Os_Internal_Task_Yield();

    /* Invoke scheduler to switch to the next highest priority READY task */
    Schedule ();

    /**
     * [CRITICAL] OSEK Compliance Gap:
     * According to the specification, TerminateTask must not return.
     * If execution reaches here, it implies a kernel failure or no other tasks are ready.
     *
     * [UNIMPLEMENTED] Error Handling:
     * 1. Call ErrorHook(E_OS_MISSINGEND)
     * 2. Call ShutdownOS()
     * 3. Fallback to infinite loop
     */
    // ShutdownOS(E_OS_SYS_ASSERTION);

    /* Fallback to prevent program counter from wandering into invalid memory */
    return E_OK; /* Formally required by function signature but unreachable */
}

StatusType ChainTask (TaskType TaskID)
{
    /* 1. Terminate current task: Remove the calling task from the ready queue */
    Os_Internal_Task_RemoveFromReady (ocb.pCurrentTask);

    /**
     * 2. Activate succeeding task: Locate the TCB for the target TaskID.
     * [TODO (HE Juncheng/2026-03-15)]: Add OSEK state check. TaskID must be in SUSPENDED state before chaining.
     */
    TaskControlBlock* pNextTask = Os_Internal_GetTaskControlBlockByTaskID (TaskID);

    /* 3. Perform Context Switch if the next task is different from the current one */
    /**
     * [FIXME]: If TaskID is the current task, this 'if' block might be bypassed,
     * causing the system to hang since the current task was already removed from ready.
     */
    if (pNextTask != ocb.pCurrentTask)
    {
        TaskControlBlock* pOldTask = ocb.pCurrentTask;

        /* Update the global pointer to the newly running task */
        ocb.pCurrentTask = pNextTask;

        /**
         * [UNIMPLEMENTED]: Handle the very first system start if pOldTask is NULL.
         * Currently assuming a valid pOldTask exists for context switching.
         */
        // if (pOldTask == NULL) { Os_FirstStart(pNextTask); } else {

        /* Perform hardware-specific context switch from old to new task */
        Os_ContextSwitch (pOldTask, pNextTask);
    }
    else
    {
        /**
         * Fallback Logic: If the next task is the current one but its state is invalid,
         * the system enters Idle/Panic mode to prevent CPU runaway.
         */
        if (ocb.pCurrentTask->TaskState != TASK_STATE_RUNNING)
        {
            Os_Internal_EnterIdleMode ();
        }
    }

    /**
     * According to the specification, ChainTask must not return.
     * If reached, it indicates a scheduling failure.
     */
    return E_OK;
}

StatusType GetTaskID (TaskRefType TaskID)
{
    /* 1. Safety Check: Ensure the provided pointer is valid */
    if (TaskID == NULL)
    {
        return E_OS_VALUE;
    }

    /* 2. Check if there is an active task context */
    if (ocb.pCurrentTask == NULL)
    {
        /**
         * Per OSEK requirement: Return INVALID_TASK if no task is running.
         * [NOTE]: Using 255 as the magic number for INVALID_TASK.
         */
        *TaskID = 255;
    }
    else
    {
        /**
         * Success: Store the TaskID of the current task into the
         * memory address pointed to by TaskID.
         */
        *TaskID = *(ocb.pCurrentTask->TaskID);
    }

    return E_OK;
}

StatusType GetTaskState (TaskType TaskID, TaskStateRefType State)
{
    /**
     * [TODO (HE Juncheng/2026-03-15)]: Add safety check for the 'State' pointer.
     * if (State == NULL) return E_OS_VALUE;
     */

    /* Store the state into the memory location pointed to by State */
    *State = Os_Internal_GetTaskControlBlockByTaskID (TaskID)->TaskState;
    return E_OK;
}

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Finds the Task Control Block (TCB) associated with a given TaskID.
 * @details Performs a linear search through the task container in the OSEK Control Block (OCB).
 *
 * @param[in]  TaskID  The unique identifier of the task to locate.
 * @return TaskControlBlock*
 * @retval Pointer to the found TCB if successful.
 * @retval NULL if no task matches the provided TaskID.
 *
 * @note   Complexity: O(n), where n is the number of tasks.
 *         Recommended to be used during initialization or non-critical paths.
 */
TaskControlBlock* Os_Internal_GetTaskControlBlockByTaskID (TaskType TaskID)
{
    TaskControlBlock* pTcb = NULL;

    /* Iterate through the tasks array to find the matching ID */
    for (uint64 i = 0; i < ocb.TasksSize; i++)
    {
        /* Compare the dereferenced TaskID pointer with the target TaskID */
        if (*(ocb.Tasks[i].TaskID) == TaskID)
        {
            pTcb = &ocb.Tasks[i];
            /* Found the TCB, exit loop early for better performance */
            break;
        }
    }
    return pTcb;
}
