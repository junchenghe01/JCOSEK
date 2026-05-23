// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Internal.h
 * @brief Internal implementation details of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file contains internal definitions and helper functions.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org.
 *
 * @author Juncheng HE
 * @date   2026-02-13
 * @copyright Copyright (c) 2026 Juncheng HE. All rights reserved.
 */

/** ============================================================================
 * Includes
 * ========================================================================= */

/** Specification of Operating System R25-11 */

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

/** ============================================================================
 * Macros & Constants
 * ========================================================================= */

/** ============================================================================
 * Local Types/Structs
 * ========================================================================= */

/** ============================================================================
 * Static Global Variables
 * ========================================================================= */
extern OsControlBlock ocb;
/** ============================================================================
 * Internal Function Prototypes
 * ========================================================================= */
/**
 * @brief  Retrieves the Counter Control Block (CCB) associated with a given AlarmID.
 * @details Performs a two-stage lookup:
 *          1. Finds the CounterID linked to the specific AlarmID in the ACB (Alarm Control Block).
 *          2. Locates the corresponding CCB using the retrieved CounterID in the CCB array.
 *
 * @param[in] AlarmID  The unique identifier of the alarm.
 *
 * @return CounterControlBlock*
 * @retval Pointer to the found CCB if successful.
 * @retval NULL if no matching alarm or counter is found.
 */
CounterControlBlock* Os_Internal_GetCounterIdByAlarmID (AlarmType AlarmID);

void       Os_Internal_TriggerAlarmAction (AlarmControlBlock* pAlarm);
uint16     Os_Internal_GetHighestPriorityIndex (void);
StatusType Os_Internal_GetTaskControlBlockByTaskID (TaskType TaskID, TaskControlBlock* pTcb);
void       Os_Internal_EnqueueReadyTask (TaskControlBlock* pTcb, PriorityType currentPriority, uint8 flag);
void       Os_Internal_Task_RemoveFromReady (TaskControlBlock* pTcb);
void       Os_Internal_Task_Yield (void);
void       Os_Internal_EnterIdleMode (void);
/** ============================================================================
 * Public API Implementation
 * ========================================================================= */

/** ============================================================================
 * Internal Helper Functions
 * ========================================================================= */

/**
 * @brief  Enables global IRQ interrupts.
 * @details Utilizes the CPSIE (Change Processor State Interrupt Enable) instruction
 *          to clear the I-bit in the CPSR (Current Program Status Register),
 *          allowing the CPU to recognize external IRQ requests.
 *
 * @note   Architecture: ARMv7-A / ARMv7-R
 * @warning This function only enables standard IRQs. FIQs remain unaffected
 *          unless specifically handled.
 */
static inline void Os_Arch_EnableInterrupts (void)
{
    /**
     * "cpsie i" is specifically used to enable standard IRQ interrupts.
     * The "memory" clobber acts as a compiler barrier, preventing the compiler
     * from reordering memory access instructions across this boundary to ensure
     * that logic intended to be protected remains within the critical section.
     */
    __asm__ volatile ("cpsie i" : : : "memory");
}

/**
 * @brief  Disables global IRQ interrupts.
 * @details Utilizes the `cpsid i` (Change Processor State Interrupt Disable)
 *          instruction to set the I-bit in the CPSR. This prevents the CPU
 *          from responding to standard IRQ requests.
 *
 * @note   Architecture: ARMv7-A / ARMv7-R / ARMv6-M / ARMv7-M.
 * @warning This function only masks IRQs. FIQs (Fast Interrupt Requests)
 *          are not disabled by this specific instruction.
 */
static inline void Os_Arch_DisableInterrupts (void)
{
    /**
     * "cpsid i" provides an atomic way to disable IRQs.
     * The "memory" clobber serves as a compiler memory barrier, ensuring that
     * all memory operations planned by the compiler are completed before
     * the interrupts are disabled, and no subsequent operations are hoisted
     * above this instruction.
     */
    __asm__ volatile ("cpsid i" : : : "memory");
}

/**
 * @brief Performs a context switch to the next ready task based on the scheduling policy.
 * @details This function is called by the scheduler after determining the next task to run.
 *
 * @param[in] pOcb Pointer to the OS control block containing the current task and system state.
 * @return StatusType
 * @retval E_OK if the context switch was successful.
 * @retval E_OS_SYS_ASSERTION if the context switch failed due to a critical error.
 */
StatusType Os_Internal_Schedule (OsControlBlock* pOcb);

#endif /** OS_INTERNAL_H */
