/**
 * @file    portasm.S
 * @brief   Cortex-M4 context switching: PendSV handler and Os_ContextSwitch.
 *
 * @details Architecture
 *          ============
 *          Cortex-M4 exception entry automatically pushes 8 words onto the
 *          current stack (PSP in Thread mode):
 *            [high] xPSR, PC, LR, R12, R3, R2, R1, R0 [low]
 *
 *          The PendSV handler only needs to save/restore the remaining
 *          callee-saved registers: R4-R11.
 *
 *          Calling convention
 *          ==================
 *          Os_ContextSwitch(R0=old_tcb, R1=new_tcb) is called from C code.
 *          Before the call, ocb.pCurrentTask has already been updated to
 *          point to the new TCB.  This function stores old_tcb in a global
 *          and triggers the PendSV exception.
 *
 *          PendSV_Handler then:
 *            1. Saves R4-R11 to the old task's stack via PSP.
 *            2. Stores PSP → old_tcb->StackPtr (offset 96).
 *            3. Loads new_tcb->StackPtr → PSP.
 *            4. Restores R4-R11 from the new task's stack.
 *            5. Exception-returns to the new task.
 *
 *          TCB layout
 *          ==========
 *          StackPtr is at offset 96 in TaskControlBlock (verified by
 *          disassembly — matches compiler's actual struct layout).
 *          The Cortex-M4 ContextInfo has 17 uint32 fields (68 bytes),
 *          placing StackPtr 8 bytes later than the Cortex-A9 port.
 *
 *          SCB (System Control Block) base: 0xE000ED00
 *          ICSR offset: 0x04, PENDSVSET = bit 28
 *          SHPR3 offset: 0x20 (PendSV priority at bits 23:16)
 */

    .syntax unified
    .cpu    cortex-m4
    .arch   armv7e-m
    .thumb

/* ==========================================================================
 * External references
 * ========================================================================= */
    .extern ocb

/* ==========================================================================
 * Global storage for TCB pointers communicated to PendSV.
 * ========================================================================= */
    .section .bss, "aw"
    .align  2
    .global g_old_tcb
    .type   g_old_tcb, %object
g_old_tcb:
    .space  4

    .global g_new_tcb
    .type   g_new_tcb, %object
g_new_tcb:
    .space  4

/* ==========================================================================
 * Os_ContextSwitch (C-callable)
 *
 * void Os_ContextSwitch(void *old_tcb, void *new_tcb);
 *
 * R0 = old_tcb, R1 = new_tcb (already equals ocb.pCurrentTask).
 *
 * Stores the parameters in globals so PendSV can find them, then triggers
 * PendSV.  PendSV fires after this function returns (lowest exception
 * priority).  Interrupts are not disabled here because PendSV will not
 * execute until all higher-priority exceptions complete.
 * ========================================================================= */

#define SCB_ICSR            0xE000ED04
#define SCB_ICSR_PENDSVSET  0x10000000

    .section .text, "ax"
    .align  2

    .global Os_ContextSwitch
    .type   Os_ContextSwitch, %function
    .thumb_func
Os_ContextSwitch:
    ldr     r2, =g_old_tcb
    str     r0, [r2]                /* g_old_tcb = old_tcb               */
    ldr     r2, =g_new_tcb
    str     r1, [r2]                /* g_new_tcb = new_tcb               */

    ldr     r2, =SCB_ICSR
    ldr     r3, =SCB_ICSR_PENDSVSET
    str     r3, [r2]                /* ICSR.PENDSVSET = 1                */
    dsb
    isb
    bx      lr
    .size   Os_ContextSwitch, . - Os_ContextSwitch


/* ==========================================================================
 * PendSV_Handler
 *
 * Executes at the lowest exception priority.  By the time this runs:
 *   - R0-R3, R12, LR, PC, xPSR of the old task are on its PSP stack.
 *   - g_old_tcb → the TCB being switched out.
 *   - ocb.pCurrentTask (== g_new_tcb) → the TCB being switched in.
 *
 * The handler uses MSP for its own stack frame (automatic in Handler mode).
 * ========================================================================= */

    .global PendSV_Handler
    .type   PendSV_Handler, %function
    .thumb_func
PendSV_Handler:
    /* 1.  Save the remaining callee-saved registers onto the old task. */
    mrs     r0, psp
    stmdb   r0!, {r4-r11}           /* push R4-R11; r0 → R4 (new top)   */

    /* 2.  Store the old PSP into old_tcb->StackPtr (offset 96). */
    ldr     r1, =g_old_tcb
    ldr     r1, [r1]                /* r1 = old_tcb                      */
    str     r0, [r1, #96]           /* old_tcb->StackPtr = PSP           */

    /* 3.  Restore the new PSP from new_tcb->StackPtr. */
    ldr     r2, =g_new_tcb
    ldr     r2, [r2]                /* r2 = new_tcb                      */
    ldr     r0, [r2, #96]           /* r0 = new_tcb->StackPtr            */

    /* 4.  Restore callee-saved registers from the new task. */
    ldmia   r0!, {r4-r11}
    msr     psp, r0                 /* PSP → just past R4-R11 (at hw frame) */

    /* 5.  Exception return.
     *     LR already holds an EXC_RETURN value (set by hardware on
     *     exception entry).  Typical value: 0xFFFFFFFD = return to
     *     Thread mode using PSP (no FPU).                              */
    bx      lr
    .size   PendSV_Handler, . - PendSV_Handler


/* ==========================================================================
 * SVC_Handler
 *
 * Used exclusively for starting the first task.  Os_Cpu_StartFirstTask
 * triggers 'svc 0', which enters this handler in Handler mode with MSP.
 *
 * The handler:
 *   1. Loads the first task's PSP from ocb.pCurrentTask->StackPtr.
 *   2. Restores R4-R11 from the task's stack.
 *   3. Sets CONTROL = 2 (Thread mode with PSP).
 *   4. Sets EXC_RETURN = 0xFFFFFFFD and triggers exception return.
 *
 * The hardware then unstacks R0-R3, R12, LR, PC, xPSR from the task's
 * stack and resumes execution in Thread mode using PSP.
 * ========================================================================= */

    .global SVC_Handler
    .type   SVC_Handler, %function
    .thumb_func
SVC_Handler:
    /* Load the first task's stack pointer */
    ldr     r1, =ocb
    ldr     r2, [r1, #12]           /* r2 = ocb.pCurrentTask            */
    ldr     r0, [r2, #96]           /* r0 = StackPtr (PSP value)        */

    /* Restore callee-saved registers from the task's stack */
    ldmia   r0!, {r4-r11}
    msr     psp, r0                 /* PSP → past R4-R11 (at hw frame)  */

    /* Switch from MSP to PSP for Thread mode */
    mov     r0, #2
    msr     control, r0
    isb                             /* ensure CONTROL takes effect       */

    /* Set EXC_RETURN: Thread mode + PSP + no FPU */
    mov     lr, #0xFFFFFFFD
    bx      lr                      /* Hardware unstacks → task runs    */
    .size   SVC_Handler, . - SVC_Handler

    .end
