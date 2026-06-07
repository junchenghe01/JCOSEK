#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "Platform_Types.h"

/**
 * @brief  Context (register snapshot) structure for Cortex-M4.
 * @details The Cortex-M4 hardware automatically saves R0-R3, R12, LR, PC, and xPSR
 *          on exception entry. The software (PendSV handler) manually saves R4-R11.
 *
 *          To maintain TCB layout compatibility with existing code, the ContextInfo
 *          structure retains all 17 fields (68 bytes), ensuring that `StackPtr`
 *          remains at offset 88 within the TCB.
 *
 * Stack frame layout on Cortex-M4 (from high address to low):
 *   [Hardware-saved, 8 words]
 *     xPSR   (offset 64)  — 0x01000000 (Thumb state)
 *     PC     (offset 60)  — task entry function
 *     LR     (offset 56)  — 0 (initial) / return address
 *     R12    (offset 48)
 *     R3     (offset 44)
 *     R2     (offset 40)
 *     R1     (offset 36)
 *     R0     (offset 32)
 *   [Software-saved, 8 words]
 *     R11    (offset 28)
 *     R10    (offset 24)
 *     R9     (offset 20)
 *     R8     (offset 16)
 *     R7     (offset 12)
 *     R6     (offset 8)
 *     R5     (offset 4)
 *     R4     (offset 0)  ← PSP points here when task is not running
 */
typedef struct ContextInfo
{
    /** @name Software-saved (callee-saved) registers */
    uint32 ctx_r4;   /* Offset 0  */
    uint32 ctx_r5;   /* Offset 4  */
    uint32 ctx_r6;   /* Offset 8  */
    uint32 ctx_r7;   /* Offset 12 */
    uint32 ctx_r8;   /* Offset 16 */
    uint32 ctx_r9;   /* Offset 20 */
    uint32 ctx_r10;  /* Offset 24 */
    uint32 ctx_r11;  /* Offset 28 */
    /** @name Hardware-saved (caller-saved) registers — auto-stacked on exception entry */
    uint32 ctx_r0;   /* Offset 32 */
    uint32 ctx_r1;   /* Offset 36 */
    uint32 ctx_r2;   /* Offset 40 */
    uint32 ctx_r3;   /* Offset 44 */
    uint32 ctx_r12;  /* Offset 48 */
    /** @name Stack Pointer — the task's PSP value */
    uint32 ctx_sp;   /* Offset 52 — R13 (PSP) */
    /** @name Link Register — return address */
    uint32 ctx_lr;   /* Offset 56 — R14 (LR) */
    /** @name Program Counter — execution resume address */
    uint32 ctx_pc;   /* Offset 60 — R15 (PC) */
    /** @name Program Status Register */
    uint32 ctx_cpsr; /* Offset 64 — xPSR */
} ContextInfo;

/* --------------------------------------------------------------------------
 * Public API (must match Cortex-A9 interface exactly)
 * -------------------------------------------------------------------------- */

/* Stack initialization: create an exception stack frame for a new task.
 * Returns the PSP value that should be stored in the TCB. */
uint32 *Os_Cpu_StackInit(void (*entry)(void), uint32 *stack, uint32 size);

/* Start the very first task. Naked function — never returns. */
extern void Os_Cpu_StartFirstTask(uint32 *pFirstStackPtr);
#define portSTART_FIRST_TASK(pStack) Os_Cpu_StartFirstTask(pStack)

/* --------------------------------------------------------------------------
 * Interrupt Control
 * -------------------------------------------------------------------------- */

/* Enable all maskable interrupts (clear PRIMASK). */
extern void Os_Cpu_EnableAllInterrupts(void);
#define portENABLE_ALL_INTERRUPTS() Os_Cpu_EnableAllInterrupts()

/* Disable all maskable interrupts (set PRIMASK). */
extern void Os_Cpu_DisableAllInterrupts(void);
#define portDISABLE_ALL_INTERRUPTS() Os_Cpu_DisableAllInterrupts()

/* Suspend all interrupts with nesting support. */
extern void Os_Cpu_SuspendAllInterrupts(void);
#define portSUSPEND_ALL_INTERRUPTS() Os_Cpu_SuspendAllInterrupts()

/* Resume all interrupts (restore from outermost Suspend). */
extern void Os_Cpu_ResumeAllInterrupts(void);
#define portRESUME_ALL_INTERRUPTS() Os_Cpu_ResumeAllInterrupts()

/* Suspend OS-managed interrupts only (uses BASEPRI). */
extern void Os_Cpu_SuspendOSInterrupts(void);
#define portSUSPEND_OS_INTERRUPTS() Os_Cpu_SuspendOSInterrupts()

/* Resume OS-managed interrupts. */
extern void Os_Cpu_ResumeOSInterrupts(void);
#define portRESUME_OS_INTERRUPTS() Os_Cpu_ResumeOSInterrupts()

/* Reset interrupt suspension nesting (called before starting first task). */
extern void Os_Cpu_ResetInterruptSuspension(void);
#define portRESET_INTERRUPT_SUSPENSION() Os_Cpu_ResetInterruptSuspension()

/* Enter low-power idle (WFI). */
extern void Os_Cpu_Idle(void);
#define portWAIT_FOR_INTERRUPT() Os_Cpu_Idle()

/* --------------------------------------------------------------------------
 * Bitmap / Scheduling helpers
 * -------------------------------------------------------------------------- */

/* Get the index of the most significant set bit (0-31).
 * Returns 0xFF if value is 0.
 * Uses the CLZ instruction available on Cortex-M4. */
extern uint8 GetMsbIndex(uint32 value);
#define portGET_MSB_INDEX(result, value) ((result) = GetMsbIndex(value))

#endif /* PORTMACRO_H */
