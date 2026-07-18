#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "Platform_Types.h"

/**
 * @brief  Context (register snapshot) structure for the x86_64 Windows port.
 * @details This is a *hosted* run-to-completion port: the kernel runs inside a
 *          normal Windows process and tasks execute as plain C function calls
 *          on the host stack. No register context is ever saved or restored.
 *          The structure only exists because the TCB embeds a ContextInfo
 *          field; its content is unused on this port.
 */
typedef struct ContextInfo
{
    uint64 ctx_unused; /* placeholder — no real context on the hosted port */
} ContextInfo;

/* --------------------------------------------------------------------------
 * Public API (must match Cortex-A9 / Cortex-M4 interface exactly)
 * -------------------------------------------------------------------------- */

/* Stack initialization: no exception frame is built on this port.
 * Returns the stack base so the TCB StackPtr field is non-NULL. */
uint32 *Os_Cpu_StackInit(void (*entry)(void), uint32 *stack, uint32 size);

/* Start the very first task. On this hosted port the function RETURNS once
 * the kernel becomes idle, so StartOS() returns to the host at first idle. */
extern void Os_Cpu_StartFirstTask(void *pFirstStackPtr);
#define portSTART_FIRST_TASK(pStack) Os_Cpu_StartFirstTask(pStack)

/* --------------------------------------------------------------------------
 * Interrupt Control
 * No hardware interrupts exist on the hosted port; execution is
 * single-threaded and deterministic, so all of these are no-op stubs.
 * -------------------------------------------------------------------------- */

/* Enable all maskable interrupts. */
extern void Os_Cpu_EnableAllInterrupts(void);
#define portENABLE_ALL_INTERRUPTS() Os_Cpu_EnableAllInterrupts()

/* Disable all maskable interrupts. */
extern void Os_Cpu_DisableAllInterrupts(void);
#define portDISABLE_ALL_INTERRUPTS() Os_Cpu_DisableAllInterrupts()

/* Suspend all interrupts with nesting support. */
extern void Os_Cpu_SuspendAllInterrupts(void);
#define portSUSPEND_ALL_INTERRUPTS() Os_Cpu_SuspendAllInterrupts()

/* Resume all interrupts (restore from outermost Suspend). */
extern void Os_Cpu_ResumeAllInterrupts(void);
#define portRESUME_ALL_INTERRUPTS() Os_Cpu_ResumeAllInterrupts()

/* Suspend OS-managed interrupts only. */
extern void Os_Cpu_SuspendOSInterrupts(void);
#define portSUSPEND_OS_INTERRUPTS() Os_Cpu_SuspendOSInterrupts()

/* Resume OS-managed interrupts. */
extern void Os_Cpu_ResumeOSInterrupts(void);
#define portRESUME_OS_INTERRUPTS() Os_Cpu_ResumeOSInterrupts()

/* Reset interrupt suspension nesting (called before starting first task). */
extern void Os_Cpu_ResetInterruptSuspension(void);
#define portRESET_INTERRUPT_SUSPENSION() Os_Cpu_ResetInterruptSuspension()

/* Enter idle: returns control to the host (end of a dispatch window). */
extern void Os_Cpu_Idle(void);
#define portWAIT_FOR_INTERRUPT() Os_Cpu_Idle()

/* --------------------------------------------------------------------------
 * Bitmap / Scheduling helpers
 * -------------------------------------------------------------------------- */

/* Get the index of the most significant set bit (0-31).
 * Returns 0xFF if value is 0.
 * Uses the compiler builtin (BSR/LZCNT instruction on x86_64). */
extern uint8 GetMsbIndex(uint32 value);
#define portGET_MSB_INDEX(result, value) ((result) = GetMsbIndex(value))

/* --------------------------------------------------------------------------
 * Hosted-port extension
 * -------------------------------------------------------------------------- */

/**
 * @brief  Run all READY tasks to completion, then return to the caller.
 * @details The host environment drives logical time by calling
 *          IncrementCounter() (simulating the tick interrupt) and then
 *          Os_Cpu_Dispatch() to execute every task that became READY.
 *          The function returns once the kernel enters idle mode.
 *
 *          Typical host loop:
 *            StartOS(0);                      — returns at first idle
 *            for (;;) {
 *                IncrementCounter(SYS_TICK);  — fire alarms / activate tasks
 *                Os_Cpu_Dispatch();           — run them, return when idle
 *            }
 */
extern void Os_Cpu_Dispatch(void);

#endif /* PORTMACRO_H */
