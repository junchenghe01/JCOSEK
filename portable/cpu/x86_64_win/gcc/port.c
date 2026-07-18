// SPDX-License-Identifier: MPL-2.0

/**
 * @file    port.c
 * @brief   x86_64 Windows (hosted) portable layer implementation for JCOSEK.
 *
 * @details This port runs the OSEK kernel *cooperatively* inside a normal
 *          Windows process — for simulation, unit/integration testing and
 *          virtual-ECU use cases. There is no real interrupt hardware and no
 *          register-level context switch:
 *
 *          - Tasks are plain C functions executed to completion on the host
 *            stack (no separate task stacks, no saved register contexts).
 *          - Os_ContextSwitch() does NOT transfer control immediately.
 *            It records the next task ("deferred dispatch") and returns, so
 *            kernel services like ActivateTask()/TerminateTask() unwind
 *            normally before the task body runs.
 *          - The dispatch loop then calls each pending task entry in kernel
 *            priority order until the kernel enters idle mode.
 *          - Idle (portWAIT_FOR_INTERRUPT) longjmps back to the innermost
 *            dispatch entry point, which returns control to the host:
 *              * StartOS() returns to the host at the first idle;
 *              * Os_Cpu_Dispatch() returns after all READY tasks finished.
 *
 *          The host drives logical time by calling IncrementCounter()
 *          (simulating the tick ISR) followed by Os_Cpu_Dispatch().
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org.
 *
 * @author Juncheng HE
 * @date   2026-07-17
 * @copyright Copyright (c) 2026 Juncheng HE. All rights reserved.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */
#include "Os.h"
#include "Os_Internal.h"

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Static Global Variables
 * ========================================================================= */

/** Return point back to the host (armed by StartFirstTask / Dispatch). */
static jmp_buf s_host_env;

/** TRUE while s_host_env holds a valid return point. */
static boolean s_host_env_armed = FALSE;

/** Task selected by the kernel but not yet executed (deferred dispatch). */
static TaskControlBlock *s_pending_task = NULL;

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */

/**
 * @brief  Run pending tasks to completion until the kernel goes idle.
 * @details Each executed task ends in TerminateTask(), whose Schedule() call
 *          either defers the next task (loop continues) or enters idle mode,
 *          which longjmps out of this loop back to the dispatch entry point.
 */
static void Os_Cpu_DispatchLoop(void)
{
    for (;;)
    {
        if (s_pending_task != NULL)
        {
            TaskControlBlock *pTask = s_pending_task;
            s_pending_task = NULL;
            if (pTask->TaskEntry != NULL)
            {
                pTask->TaskEntry(); /* run-to-completion */
            }
        }
        else
        {
            /* Nothing left to run → idle → longjmp back to the host */
            Os_Internal_EnterIdleMode();
        }
    }
}

/* ============================================================================
 * Public API Implementation — kernel port hooks
 * ========================================================================= */

/**
 * @brief  "Context switch": defer execution of the next task.
 * @details The kernel has already updated ocb.pCurrentTask = new_tcb.
 *          The old task's stack frame simply unwinds (run-to-completion
 *          model), and the dispatch loop will invoke the new task entry.
 *
 * @param[in] old_tcb  Previously running task (may be NULL on first dispatch).
 * @param[in] new_tcb  Task selected by the scheduler to run next.
 */
void Os_ContextSwitch(TaskControlBlock *old_tcb, TaskControlBlock *new_tcb)
{
    (void)old_tcb;
    if (new_tcb != NULL)
    {
        new_tcb->TaskState = TASK_STATE_RUNNING;
        s_pending_task = new_tcb;
    }
}

/**
 * @brief  No stack frame is needed — tasks share the host stack.
 *
 * @param[in] entry  Task entry function (unused on this port).
 * @param[in] stack  Base address of the task's stack array.
 * @param[in] size   Size of the stack (unused on this port).
 *
 * @return The stack base (kept non-NULL for TCB bookkeeping; never used).
 */
uint32 *Os_Cpu_StackInit(void (*entry)(void), uint32 *stack, uint32 size)
{
    (void)entry;
    (void)size;
    return stack;
}

/**
 * @brief  Start the first task (called from StartOS → Os_Internal_Start).
 * @details ocb.pCurrentTask was already set to the first task by the kernel.
 *          Unlike hardware ports, this function RETURNS once the kernel
 *          becomes idle — so on the hosted port StartOS() itself returns
 *          to the host after the auto-start tasks have completed.
 *
 * @param[in] pFirstStackPtr  First task's stack pointer (unused).
 */
void Os_Cpu_StartFirstTask(void *pFirstStackPtr)
{
    (void)pFirstStackPtr;
    s_pending_task = ocb.pCurrentTask;
    if (setjmp(s_host_env) == 0)
    {
        s_host_env_armed = TRUE;
        Os_Cpu_DispatchLoop(); /* exits via longjmp from Os_Cpu_Idle() */
    }
    s_host_env_armed = FALSE;
}

/**
 * @brief  Run all READY tasks to completion, then return (hosted extension).
 * @details See portmacro.h for the host usage pattern.
 */
void Os_Cpu_Dispatch(void)
{
    if (setjmp(s_host_env) == 0)
    {
        s_host_env_armed = TRUE;
        Os_Cpu_DispatchLoop(); /* exits via longjmp from Os_Cpu_Idle() */
    }
    s_host_env_armed = FALSE;
}

/**
 * @brief  Idle = end of the current dispatch window: return to the host.
 * @details Called by the kernel idle loop (portWAIT_FOR_INTERRUPT). Aborts
 *          if no dispatch window is active — that would mean the kernel went
 *          idle outside StartOS()/Os_Cpu_Dispatch(), a host integration bug.
 */
void Os_Cpu_Idle(void)
{
    if (s_host_env_armed == TRUE)
    {
        longjmp(s_host_env, 1);
    }

    (void)fprintf(stderr,
                  "jcosek x86_64_win port: kernel idled outside a dispatch "
                  "window (call StartOS/Os_Cpu_Dispatch first)\n");
    abort();
}

/* ============================================================================
 * Interrupt control — no-ops (single-threaded deterministic host execution)
 * ========================================================================= */

void Os_Cpu_EnableAllInterrupts(void)      {}
void Os_Cpu_DisableAllInterrupts(void)     {}
void Os_Cpu_SuspendAllInterrupts(void)     {}
void Os_Cpu_ResumeAllInterrupts(void)      {}
void Os_Cpu_SuspendOSInterrupts(void)      {}
void Os_Cpu_ResumeOSInterrupts(void)       {}
void Os_Cpu_ResetInterruptSuspension(void) {}

/* ============================================================================
 * Bitmap helper
 * ========================================================================= */

/**
 * @brief  Index of the most significant set bit (0-31), 0xFF if value == 0.
 */
uint8 GetMsbIndex(uint32 value)
{
    if (value == 0U)
    {
        return 0xFFU;
    }
    return (uint8)(31U - (uint32)__builtin_clz(value));
}
