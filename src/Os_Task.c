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
static void       Os_Internal_EnqueueReadyTaskTail (TaskControlBlock* pTcb);
static StatusType Os_Internal_ActivateTask (TaskType TaskID);
static StatusType Os_Internal_TerminateTask (OsControlBlock* pOcb);
static StatusType Os_Internal_ChainTask (TaskType TaskID, OsControlBlock* pOcb);
static StatusType Os_Internal_GetTaskID (TaskRefType TaskID, OsControlBlock* pOcb);
static StatusType Os_Internal_GetTaskState (TaskType TaskID, TaskStateRefType State);
/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
StatusType ActivateTask (TaskType TaskID)
{
    StatusType status;
    SuspendOSInterrupts ();
    status = Os_Internal_ActivateTask (TaskID);
    ResumeOSInterrupts ();
    return E_OK;
}

// [UNIMPLEMENTED] ActivateTaskAsyn

StatusType TerminateTask (void)
{
    StatusType status;
    SuspendOSInterrupts ();
    status = Os_Internal_TerminateTask (&ocb);
    ResumeOSInterrupts ();
    return status;
}

StatusType ChainTask (TaskType TaskID)
{
    StatusType status;
    SuspendOSInterrupts ();
    status = Os_Internal_ChainTask (TaskID, &ocb);
    ResumeOSInterrupts ();
    return status;
}

StatusType GetTaskID (TaskRefType TaskID)
{
    StatusType status;
    SuspendOSInterrupts ();
    status = Os_Internal_GetTaskID (TaskID, &ocb);
    ResumeOSInterrupts ();
    return status;
}

StatusType GetTaskState (TaskType TaskID, TaskStateRefType State)
{
    StatusType status;
    SuspendOSInterrupts ();
    status = Os_Internal_GetTaskState (TaskID, State);
    ResumeOSInterrupts ();
    return status;
}

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Finds the Task Control Block (TCB) associated with a given TaskID.
 * @details Performs a linear search through the task container in the OSEK Control Block (OCB).
 *
 * @param[in]  TaskID  The unique identifier of the task to locate.
 * @param[out] pTcb    Pointer to a TaskControlBlock pointer where the result will be stored.
 *                      The caller should provide a valid pointer to store the result.
 * @return StatusType
 * @retval E_OK if the task is found.
 * @retval E_OS_ID if no task matches the provided TaskID.
 *
 * @note   Complexity: O(n), where n is the number of tasks.
 *         Recommended to be used during initialization or non-critical paths.
 */
StatusType Os_Internal_GetTaskControlBlockByTaskID (TaskType TaskID, TaskControlBlock* pTcb)
{
    StatusType status = E_OS_ID; /* Default to error state */
    pTcb              = NULL;    /* Initialize output pointer to NULL */
    /* Iterate through the tasks array to find the matching ID */
    for (uint64 i = 0; i < ocb.TasksSize; i++)
    {
        /* Compare the dereferenced TaskID pointer with the target TaskID */
        if (*(ocb.Tasks[i].TaskID) == TaskID)
        {
            pTcb   = &ocb.Tasks[i];
            status = E_OK;
            /* Found the TCB, exit loop early for better performance */
            break;
        }
    }
    return status;
}

/**
 * @brief  Enqueue the task at the tail of the ready queue.
 * @details Used for handling multiple activations following ActivateTask or TerminateTask.
 * Ensures that this task is scheduled last among tasks of the same priority.
 *
 * @param[in] pTcb  Pointer to the Task Control Block (TCB) to be enqueued
 */
static void Os_Internal_EnqueueReadyTaskTail (TaskControlBlock* pTcb)
{
    if (pTcb == NULL)
        return;

    uint8 prio = pTcb->CurrentPriority;

    /* --- STEP 1: Update Priority Bitmap --- */
    uint32 group = prio / 32;
    uint32 bit   = prio % 32;
    ocb.ReadyBitmap[group] |= (1 << bit);

    /* --- STEP 2: Insert into Doubly Circular Linked List (Tail Logic) --- */
    if (ocb.ReadyQueues[prio] == NULL)
    {
        /* Case A: The queue is empty; this task becomes the sole Head. */
        ocb.ReadyQueues[prio] = pTcb;
        pTcb->NextReady       = pTcb;
        pTcb->PrevReady       = pTcb;
    }
    else
    {
        /* Case B: The queue already contains tasks.
         * In a doubly circular linked list, the tail of the queue is effectively head->PrevReady.
         * We insert the new node between the tail and the head, but do not move the head pointer.
         */
        TaskControlBlock* head = ocb.ReadyQueues[prio];
        TaskControlBlock* tail = head->PrevReady;

        pTcb->NextReady = head;
        pTcb->PrevReady = tail;

        tail->NextReady = pTcb;
        head->PrevReady = pTcb;

        /* Note: ocb.ReadyQueues[prio] is not updated here;
         * consequently, the head pointer continues to point to the original first task,
         * and pTcb is naturally placed at the end.
         */
    }
}

/**
 * @brief Enqueue the task at the head of the ready queue.
 * @details Used for handling the initial activation of a task. Ensures that this task is scheduled first among tasks of
 * the same priority.
 *
 * @param[in] TaskID The unique identifier of the task to be enqueued.
 * @return StatusType
 * @retval E_OK if the task is successfully enqueued.
 * @retval E_OS_ID if no task matches the provided TaskID.
 */
static StatusType Os_Internal_ActivateTask (TaskType TaskID)
{
    /* Retrieve the Task Control Block (TCB) for the given ID */
    TaskControlBlock* pTcb   = NULL;
    StatusType        status = Os_Internal_GetTaskControlBlockByTaskID (TaskID, &pTcb);
    if (status != E_OK)
    {
        return status;
    }

    /**
     * OSEK Rule: A task can only be activated if it is in the SUSPENDED state.
     * BCC1/ECC1 models do not support multiple activations (queuing).
     */
    if (pTcb->activation_count >= pTcb->max_activation)
    {
        return E_OS_LIMIT;
    }
    else
    {
        /* Increment the activation count for the task */
        pTcb->activation_count++;
    }

    if (pTcb->TaskState != TASK_STATE_SUSPENDED)
    {
        return E_NOT_OK; /* Task is already active, but we have recorded the activation for later */
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

/**
 * @brief Internal function to handle the logic of TerminateTask, including state transitions and scheduling.
 * @details This function is called by TerminateTask after disabling interrupts. It performs the following steps:
 * 1. Decrements the activation count of the current task.
 * 2. If there are still pending activations, it transitions the task to READY and
 *   re-enqueues it at the tail of the ready queue.
 *  If there are no pending activations, it transitions the task to SUSPENDED and removes it from the ready queue.
 * 3. Triggers the scheduler to switch to the next ready task.
 * 4. If the scheduler returns (which it should not), it indicates a critical failure, and appropriate error handling
 * should be performed.
 *
 * @param[in] pOcb Pointer to the OS control block containing the current task and system state.
 * @return StatusType
 * @retval E_OK if the task termination process completes successfully.
 * @retval E_OS_LIMIT if the activation count exceeds the maximum allowed.
 * @retval E_OS_SYS_ASSERTION if the current task context is invalid (e.g., no current task).
 */
static StatusType Os_Internal_TerminateTask (OsControlBlock* pOcb)
{
    /* [UNIMPLEMENTED] Runtime Protection: Ensure current task context is valid */
    // if (ocb.pCurrentTask == NULL) return E_OS_SYS_ASSERTION;

    /* Decrement the activation count of the current task */
    if (pOcb->pCurrentTask->activation_count > 0)
    {
        pOcb->pCurrentTask->activation_count--;
    }
    else
    {
        return E_OS_LIMIT; /* No pending activations to consume */
    }

    if (pOcb->pCurrentTask->activation_count > 0)
    {
        /* Transition current task to SUSPENDED to prevent it from being rescheduled */
        pOcb->pCurrentTask->TaskState = TASK_STATE_READY;

        /* Physically remove task from the ready queue/bitmap structure */
        // Os_Internal_Task_RemoveFromReady (pOcb->pCurrentTask);
        Os_Internal_EnqueueReadyTaskTail (pOcb->pCurrentTask);
    }
    else
    {
        /* Transition current task to SUSPENDED to prevent it from being rescheduled */
        pOcb->pCurrentTask->TaskState = TASK_STATE_SUSPENDED;

        /* Physically remove task from the ready queue/bitmap structure */
        Os_Internal_Task_RemoveFromReady (pOcb->pCurrentTask);
    }

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

/**
 * @brief Internal function to handle the logic of ChainTask, including state transitions and scheduling.
 * @details This function is called by ChainTask after disabling interrupts. It performs the following steps:
 * 1. Removes the current task from the ready queue to effectively terminate it.
 * 2. Activates the specified task by locating its TCB and transitioning it to READY
 * 3. If the next task is different from the current one, it performs a context switch to the new task.
 * 4. If the next task is the same as the current one but its state is
 * invalid (not RUNNING), it enters Idle/Panic mode to prevent CPU runaway.
 * 5. According to the specification, ChainTask must not return. If execution reaches the
 * end of this function, it indicates a critical failure, and appropriate error handling should be performed.
 *
 * @param[in] TaskID The unique identifier of the task to be activated and chained to.
 * @param[in] pOcb Pointer to the OS control block containing the current task and system state.
 * @return StatusType
 * @retval E_OK if the task chaining process completes successfully (though it should not return).
 * @retval E_OS_ID if no task matches the provided TaskID.
 * @retval E_OS_SYS_ASSERTION if the current task context is invalid (e.g., no current task).
 */
static StatusType Os_Internal_ChainTask (TaskType TaskID, OsControlBlock* pOcb)
{
    /* 1. Terminate current task: Remove the calling task from the ready queue */
    Os_Internal_Task_RemoveFromReady (pOcb->pCurrentTask);

    /**
     * 2. Activate succeeding task: Locate the TCB for the target TaskID.
     * [TODO (HE Juncheng/2026-03-15)]: Add OSEK state check. TaskID must be in SUSPENDED state before chaining.
     */
    TaskControlBlock* pNextTask = NULL;
    StatusType        status    = Os_Internal_GetTaskControlBlockByTaskID (TaskID, &pNextTask);
    if (status != E_OK)
    {
        return status;
    }

    /* 3. Perform Context Switch if the next task is different from the current one */
    /**
     * [FIXME]: If TaskID is the current task, this 'if' block might be bypassed,
     * causing the system to hang since the current task was already removed from ready.
     */
    if (pNextTask != pOcb->pCurrentTask)
    {
        TaskControlBlock* pOldTask = pOcb->pCurrentTask;

        /* Update the global pointer to the newly running task */
        pOcb->pCurrentTask = pNextTask;

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
        if (pOcb->pCurrentTask->TaskState != TASK_STATE_RUNNING)
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

/**
 * @brief Enters the idle mode of the operating system, typically called when no tasks are ready to run.
 * @details This function is invoked when the scheduler determines that there are no READY tasks to execute.
 *
 * @param[in] TaskID The unique identifier of the task to be enqueued.
 * @param[in] pOcb Pointer to the OS control block containing the current task and system state.
 * @return StatusType
 * @retval E_OK if the system successfully enters idle mode.
 * @retval E_OS_SYS_ASSERTION if the system fails to enter idle mode due to a critical error.
 */
static StatusType Os_Internal_GetTaskID (TaskRefType TaskID, OsControlBlock* pOcb)
{
    /* 1. Safety Check: Ensure the provided pointer is valid */
    if (TaskID == NULL)
    {
        return E_OS_VALUE;
    }

    /* 2. Check if there is an active task context */
    if (pOcb->pCurrentTask == NULL)
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
        *TaskID = *(pOcb->pCurrentTask->TaskID);
    }

    return E_OK;
}

/**
 * @brief Enters the idle mode of the operating system, typically called when no tasks are ready to run.
 * @details This function is invoked when the scheduler determines that there are no READY tasks to execute.
 *
 * @param[in] TaskID The unique identifier of the task whose state is to be retrieved.
 * @param[out] State Pointer to the memory location where the task state will be stored.
 * @return StatusType.
 * @retval E_OK if the task state is successfully retrieved and stored.
 * @retval E_OS_ID if no task matches the provided TaskID.
 * @retval E_OS_VALUE if the provided State pointer is NULL.
 */
static StatusType Os_Internal_GetTaskState (TaskType TaskID, TaskStateRefType State)
{
    /**
     * [TODO (HE Juncheng/2026-03-15)]: Add safety check for the 'State' pointer.
     * if (State == NULL) return E_OS_VALUE;
     */

    /* Store the state into the memory location pointed to by State */
    TaskControlBlock* pTcb   = NULL;
    StatusType        status = Os_Internal_GetTaskControlBlockByTaskID (TaskID, &pTcb);
    if (status != E_OK)
    {
        return status;
    }
    *State = pTcb->TaskState;
    return status;
}
