// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Scheduler.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Scheduling Policy (Full/Mixed Preemptive Scheduling)
 *
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
StatusType Schedule (void)
{
    StatusType status = E_NOT_OK;

    /* 1. Locate the highest priority level with ready tasks in O(1) or O(N_groups) */
    uint16 highest_prio = Os_Internal_GetHighestPriorityIndex ();

    /**
     * If no tasks are ready (even the Idle task is missing), this indicates a
     * severe kernel panic. Entering a debug-friendly hang state.
     */
    if (highest_prio == 0xFFFF)
    {
        while (1)
        {
            /**
             * [DEBUG] Check GIC Interrupt Acknowledge Register (IAR).
             * If IAR != 1023, an interrupt is pending but failed to trigger the vector.
             */
            uint32 iar = *(volatile uint32*)(0x1e00010c);
            if (iar != 1023)
            {
                /* Hardware has pending IRQ; check VBAR or Global Interrupt Enable */
            }
        }
    }

    /**
     * 2. Fetch the next task to run.
     * ReadyQueues[p] always points to the 'Head' of the circular list for that priority.
     */
    TaskControlBlock* pNextTask = ocb.ReadyQueues[highest_prio];

    /**
     * 3. Determine if a context switch is required.
     * Switch if:
     *   - The next task is different from the current task, AND
     *   - The current task is no longer in RUNNING state (e.g., Suspended/Waiting).
     */
    if (pNextTask != ocb.pCurrentTask && ocb.pCurrentTask->TaskState != TASK_STATE_RUNNING)
    {
        /* [TODO (HE Juncheng/2026-03-15)] Handle the self-switch scenario to prevent system hang */
        TaskControlBlock* pOldTask = ocb.pCurrentTask;

        /* Update global kernel state to the new task */
        ocb.pCurrentTask = pNextTask;

        /**
         * 4. Perform Architecture-specific Context Switch (Assembly).
         * [UNIMPLEMENTED] Handling Os_FirstStart if pOldTask is NULL.
         */
        Os_ContextSwitch (pOldTask, pNextTask);
        status = E_OK;
    }
    else
    {
        /**
         * Fallback: If the highest priority task is the current one, but it is
         * not in RUNNING state, the system must enter Idle mode.
         */
        if (ocb.pCurrentTask->TaskState != TASK_STATE_RUNNING)
        {
            Os_Internal_EnterIdleMode ();
        }
    }
    return status;
}

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Finds the highest priority task currently in the READY state.
 * @details Scans the priority bitmap from highest to lowest block.
 *          Uses the hardware-accelerated 'CLZ' (Count Leading Zeros) instruction
 *          to achieve O(1) time complexity within each 32-bit bitmap segment.
 *
 * @return uint16
 * @retval 0..N   The index of the highest priority task found.
 * @retval 0xFFFF Returned if no tasks are in the READY state.
 *
 * @note   This function is architecture-specific (requires ARM CLZ or equivalent).
 *         The priority is calculated as: (BlockIndex * 32) + (31 - LeadingZeros).
 */
uint16 Os_Internal_GetHighestPriorityIndex (void)
{
    /* Scan from the highest priority block down to 0 */
    for (int i = BITMAP_SIZE - 1; i >= 0; i--)
    {
        /* Check if any task is ready in the current 32-bit segment */
        if (ocb.ReadyBitmap[i] != 0)
        {
            uint32 temp = ocb.ReadyBitmap[i];
            uint32 leading_zeros;
            /* Hardware acceleration: Find the most significant bit set (MSB) */
            // __asm__ volatile ("clz %0, %1" /* Count leading zeros in 'temp' */
            //                   : "=r"(leading_zeros)
            //                   : "r"(temp));
            portGET_MSB_INDEX (leading_zeros, temp);
            /**
             * Convert bit position to global priority index:
             * i << 5: Multiply block index by 32
             * 31 - leading_zeros: Get the bit offset from the right (0-31)
             */
            return (uint16)((i << 5) + leading_zeros);
        }
    }
    /* No ready tasks found in any bitmap segment */
    return 0xFFFF;
}

/**
 * @brief  Enqueues a task into the Ready Queue and updates the priority bitmap.
 * @details This function adds a task to the TAIL of the doubly-linked circular list
 *          for its current priority. It also handles priority re-queuing during
 *          resource management.
 *
 * @param[in] pTcb             Pointer to the Task Control Block (TCB) to enqueue.
 * @param[in] currentPriority  The priority level from which the task is being moved.
 * @param[in] flag             Special operation flag:
 *                             1: Move task from 'currentPriority' to 'pTcb->CurrentPriority'
 *                                (Resource Management / Priority Ceiling).
 *                             0: Standard task activation.
 */
void Os_Internal_EnqueueReadyTask (TaskControlBlock* pTcb, PriorityType currentPriority, uint8 flag)
{
    if (pTcb == NULL)
        return;

    uint8 prio = pTcb->CurrentPriority;

    /* --- STEP 1: Handle Priority Re-queuing (for Resource Management) --- */
    if (flag == 1)
    {
        if (ocb.ReadyQueues[currentPriority] == pTcb && pTcb->NextReady == pTcb)
        {
            /* Case A: Sole task at this priority. Clear the queue and the bitmap bit. */
            ocb.ReadyQueues[currentPriority] = NULL;
            uint32 group                     = currentPriority / 32;
            uint32 bit                       = currentPriority % 32;
            ocb.ReadyBitmap[group] &= ~(1 << bit);
        }
        else
        {
            /* Case B: Multiple tasks exist. Perform standard node removal. */
            pTcb->PrevReady->NextReady = pTcb->NextReady;
            pTcb->NextReady->PrevReady = pTcb->PrevReady;

            /* If the task was the head pointer, move the head to the next sibling. */
            if (ocb.ReadyQueues[currentPriority] == pTcb)
            {
                ocb.ReadyQueues[currentPriority] = pTcb->NextReady;
            }
        }
        /* Reset TCB pointers to ensure no dangling links before re-insertion. */
        pTcb->NextReady = pTcb;
        pTcb->PrevReady = pTcb;
    }

    /* --- STEP 2: Update Priority Bitmap (256-bit total, 8 groups of 32-bit) --- */
    uint32 group = prio / 32;
    uint32 bit   = prio % 32;

    /* Set the bit corresponding to the new priority level. */
    ocb.ReadyBitmap[group] |= (1 << bit);

    /* --- STEP 3: Insert into the Doubly-Linked Circular Queue (at the Tail) --- */
    if (ocb.ReadyQueues[prio] == NULL)
    {
        /* Case A: First task at this priority level. */
        ocb.ReadyQueues[prio] = pTcb;
        pTcb->NextReady       = pTcb;
        pTcb->PrevReady       = pTcb;
    }
    else
    {
        /* Case B: Tasks already present. Insert between Tail and Head. */
        TaskControlBlock* head = ocb.ReadyQueues[prio];
        TaskControlBlock* tail = head->PrevReady;

        pTcb->NextReady = head;
        pTcb->PrevReady = tail;
        tail->NextReady = pTcb;
        head->PrevReady = pTcb;

        /* Note: ocb.ReadyQueues[prio] remains pointing to the head. */
    }
}

/**
 * @brief  Yields the CPU to the next task at the same priority level.
 * @details If multiple tasks exist at the current priority, this function rotates
 *          the ready queue head to the next task in the circular list, moving the
 *          current task to the logical tail. It then invokes the scheduler.
 *
 * @note   In OSEK, this implements the behavior of a task relinquishing control
 *         to its "equal-priority" peers.
 */
void Os_Internal_Task_Yield (void)
{
    /* Get the priority of the currently running task */
    uint8 prio = ocb.pCurrentTask->CurrentPriority;

    /**
     * Check if there are other tasks at the same priority level.
     * (NextReady != self) implies the circular list has more than one node.
     */
    if (ocb.ReadyQueues[prio]->NextReady != ocb.ReadyQueues[prio])
    {
        /**
         * Shift the head pointer to the next sibling.
         * Since the list is circular, the previous head naturally becomes the tail.
         */
        ocb.ReadyQueues[prio] = ocb.ReadyQueues[prio]->NextReady;
    }

    /**
     * Trigger the scheduler to perform a context switch.
     * The scheduler will now fetch the updated ReadyQueues[prio] as the new candidate.
     */
    Schedule ();
}
/**
 * @brief  Removes a task from the ready queue and updates the priority bitmap.
 * @details This function is called when a task transitions out of the READY state
 *          (e.g., Terminating, Waiting for an Event, or Suspending).
 *
 * @param[in] pTcb  Pointer to the Task Control Block to be removed.
 */
void Os_Internal_Task_RemoveFromReady (TaskControlBlock* pTcb)
{
    /* Get the current priority of the task.
     * TODO (HE Juncheng/2026-03-15): Investigate why task2 ID might be reported as 0 here. */
    uint8 prio = pTcb->CurrentPriority;

    /**
     * Case 1: The task is the only occupant of this priority level.
     * (NextReady points to itself).
     */
    if (pTcb->NextReady == pTcb)
    {
        /* Clear the queue head pointer for this priority */
        ocb.ReadyQueues[prio] = NULL;

        /* Update the Priority Bitmap: 'Turn off' the bit for this priority */
        uint32 group = prio / 32;
        uint32 bit   = prio % 32;

        /* Use 1UL to ensure unsigned 32-bit operation and prevent sign extension */
        ocb.ReadyBitmap[group] &= ~(1UL << bit);
    }
    /**
     * Case 2: There are other tasks remaining at this priority level.
     * Perform a standard removal from the doubly-linked circular list.
     */
    else
    {
        /* Re-link neighbors to bypass the current TCB */
        pTcb->PrevReady->NextReady = pTcb->NextReady;
        pTcb->NextReady->PrevReady = pTcb->PrevReady;

        /**
         * If the task being removed is the current head of the queue,
         * shift the head pointer to the next available task.
         */
        if (ocb.ReadyQueues[prio] == pTcb)
        {
            ocb.ReadyQueues[prio] = pTcb->NextReady;
        }
    }
}

/**
 * @brief  Enters the kernel's idle mode when no tasks are ready to run.
 * @details This function clears the current task context and enters a low-power
 *          infinite loop. The CPU will wait for an interrupt (WFI) to wake up
 *          and trigger the scheduler via an ISR.
 *
 * @note   This is the fallback state of the OS. It must ensure that interrupts
 *         are enabled so the system can be "awakened" by hardware events.
 */
void Os_Internal_EnterIdleMode (void)
{
    /**
     * 1. Clear the current task reference.
     *    This prevents the Scheduler from misidentifying a previously running
     *    task as active during the next scheduling cycle.
     */
    ocb.pCurrentTask = NULL;

    /**
     * 2. [CRITICAL] Enable global interrupts.
     *    The 'WFI' instruction below requires interrupts to be enabled to
     *    exit the sleep state. Without this, the system would hang indefinitely.
     */
    // Os_EnableAllInterrupts();

    /**
     * 3. Enter the Kernel Idle Loop.
     */
    while (1)
    {
        /**
         * [OPTIONAL] OSEK/AUTOSAR Idle Hook:
         * Allows user-defined low-priority background processing.
         */
        // Os_IdleHook();

        /**
         * ARM Power-Saving Instruction: Wait For Interrupt (WFI).
         * Suspends execution until a hardware interrupt occurs.
         */
        // __asm__ volatile ("wfi");
        portWAIT_FOR_INTERRUPT ();
    }
}
