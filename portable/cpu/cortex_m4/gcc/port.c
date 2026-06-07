/**
 * @file    port.c
 * @brief   Cortex-M4 portable layer implementation for JCOSEK.
 * @details Implements stack initialization, first task startup, interrupt
 *          control (PRIMASK/BASEPRI), idle (WFI), and the GetMsbIndex helper
 *          using the Cortex-M4 CLZ instruction.
 */

#include "Platform_Types.h"
#include <stdint.h>
#include "portmacro.h"

/* ==========================================================================
 * Static Variables for Interrupt Nesting
 * ========================================================================= */

/** Saved PRIMASK state before DisableAllInterrupts. */
static uint32 saved_primask = 0;

/** Nesting counter for SuspendAllInterrupts. */
static uint32 suspend_all_count = 0;

/** Original PRIMASK captured on the outermost SuspendAllInterrupts. */
static uint32 saved_primask_for_suspend = 0;

/** Nesting counter for SuspendOSInterrupts (BASEPRI-based). */
static uint32 suspend_os_count = 0;

/** Original BASEPRI captured on the outermost SuspendOSInterrupts. */
static uint32 saved_basepri_for_os = 0;

/* ==========================================================================
 * CMSIS-compatible inline helpers (avoid dependency on full CMSIS headers)
 * ========================================================================= */

/** Read PRIMASK register. */
static inline uint32 __get_PRIMASK(void)
{
    uint32 result;
    __asm__ volatile("mrs %0, primask" : "=r"(result));
    return result;
}

/** Set PRIMASK register. */
static inline void __set_PRIMASK(uint32 priMask)
{
    __asm__ volatile("msr primask, %0" : : "r"(priMask) : "memory");
}

/** Read BASEPRI register. */
static inline uint32 __get_BASEPRI(void)
{
    uint32 result;
    __asm__ volatile("mrs %0, basepri" : "=r"(result));
    return result;
}

/** Set BASEPRI register. */
static inline void __set_BASEPRI(uint32 basePri)
{
    __asm__ volatile("msr basepri, %0" : : "r"(basePri) : "memory");
}

/** Disable all maskable interrupts (set PRIMASK). */
static inline void __disable_irq(void)
{
    __asm__ volatile("cpsid i" : : : "memory");
}

/** Enable all maskable interrupts (clear PRIMASK). */
static inline void __enable_irq(void)
{
    __asm__ volatile("cpsie i" : : : "memory");
}

/** Wait For Interrupt. */
static inline void __WFI(void)
{
    __asm__ volatile("wfi" : : : "memory");
}

/** Count Leading Zeros. */
static inline uint32 __CLZ(uint32 value)
{
    uint32 result;
    __asm__ volatile("clz %0, %1" : "=r"(result) : "r"(value));
    return result;
}

/* ==========================================================================
 * Stack Initialization
 * ========================================================================= */

/**
 * @brief  Initialize a task's stack to look as if it was preempted by an
 *         exception. This matches the Cortex-M4 exception stacking order.
 *
 * @details The hardware automatically pushes 8 words on exception entry:
 *          xPSR, PC, LR, R12, R3, R2, R1, R0 (from high to low address).
 *          The PendSV handler manually saves the remaining 8 words: R4-R11.
 *
 *          Memory layout (high address → low address):
 *          +------------------+  ← stack + size (top)
 *          | xPSR = 0x01000000|  ← Thumb state bit set
 *          | PC  = entry      |
 *          | LR  = 0          |
 *          | R12 = 0          |
 *          | R3  = 0          |
 *          | R2  = 0          |
 *          | R1  = 0          |
 *          | R0  = 0          |
 *          | R11 = 0          |
 *          | R10 = 0          |
 *          | R9  = 0          |
 *          | R8  = 0          |
 *          | R7  = 0          |
 *          | R6  = 0          |
 *          | R5  = 0          |
 *          | R4  = 0          |  ← returned SP (PSP value)
 *          +------------------+  ← stack (bottom)
 *
 * @param[in]  entry  Task entry function pointer.
 * @param[in]  stack  Base address of the task's stack array.
 * @param[in]  size   Size of the stack in bytes (must be multiples of 4).
 *
 * @return     Pointer to the initial stack pointer value (PSP) to be stored
 *             in the TCB's StackPtr field.
 */
uint32 *Os_Cpu_StackInit(void (*entry)(void), uint32 *stack, uint32 size)
{
    /* Stack grows downward: start at the top, expressed in uint32 units */
    uint32 *sp = (uint32 *)((uint32 *)stack + (size / sizeof(uint32)));

    /* 8-byte alignment per AAPCS */
    sp = (uint32 *)((uintptr_t)sp & ~0x7U);

    /* --- Hardware-stacked registers (8 words) --- */
    /* xPSR: bit 24 (Thumb state) must be set for correct exception return */
    *(--sp) = 0x01000000U;
    /* PC: task entry point (LSB must be 1 for Thumb state) */
    *(--sp) = (uint32)entry;
    /* LR: 0 for initial task (not returning from a call) */
    *(--sp) = 0x00000000U;
    /* R12 */
    *(--sp) = 0x00000000U;
    /* R3 */
    *(--sp) = 0x00000000U;
    /* R2 */
    *(--sp) = 0x00000000U;
    /* R1 */
    *(--sp) = 0x00000000U;
    /* R0 */
    *(--sp) = 0x00000000U;

    /* --- Software-stacked registers (8 words, pushed by PendSV) --- */
    *(--sp) = 0x00000000U; /* R11 */
    *(--sp) = 0x00000000U; /* R10 */
    *(--sp) = 0x00000000U; /* R9  */
    *(--sp) = 0x00000000U; /* R8  */
    *(--sp) = 0x00000000U; /* R7  */
    *(--sp) = 0x00000000U; /* R6  */
    *(--sp) = 0x00000000U; /* R5  */
    *(--sp) = 0x00000000U; /* R4  */

    /* Return SP pointing at R4 (lowest saved register) */
    return sp;
}

/* ==========================================================================
 * First Task Startup
 * ========================================================================= */

/**
 * @brief  Start the very first task.
 * @details Naked function.  Sets up MSP for safety, enables interrupts, and
 *          triggers an SVC exception.  The SVC handler (in portasm.S) performs
 *          the actual context restore: it loads the first task's stack pointer
 *          from ocb.pCurrentTask->StackPtr, pops R4-R11, and exception-returns
 *          to Thread mode with PSP.
 *
 *          We must go through an exception (SVC) because the hardware
 *          exception-return mechanism is the only way to atomically switch
 *          stack pointers and restore the full register set on Cortex-M.
 *
 * @param[in]  pFirstStackPtr  The PSP value for the first task.  This has
 *                             already been stored in ocb.pCurrentTask->StackPtr
 *                             by the kernel before this call.
 */
__attribute__((naked)) void Os_Cpu_StartFirstTask(uint32 *pFirstStackPtr)
{
    (void)pFirstStackPtr; /* Already stored in ocb.pCurrentTask->StackPtr */

    __asm__ volatile(
        /* MSP already points to __StackTop (set by Reset_Handler).
         * DO NOT set MSP to the task stack — Handler mode always uses MSP,
         * and setting it to a task PSP would cause ISR exception frames
         * to corrupt task stack data. */
        /* Clear BASEPRI so PendSV (and other OS interrupts) can fire.
         * Os_Start() called SuspendOSInterrupts() which set BASEPRI=0x80,
         * and the matching ResumeOSInterrupts() is never reached.       */
        "mov r0, #0                      \n"
        "msr basepri, r0                 \n"
        /* Enable interrupts globally (clear PRIMASK) */
        "cpsie i                        \n"
        /* Trigger SVC to start the first task via exception return */
        "svc 0                          \n"
        /* Should never reach here */
        "b .                            \n"
        :
        :
        : "r0", "memory");
}

/* ==========================================================================
 * Interrupt Control
 * ========================================================================= */

/**
 * @brief  Enable all maskable interrupts.
 * @details Restores the previously saved PRIMASK state.
 */
void Os_Cpu_EnableAllInterrupts(void)
{
    __set_PRIMASK(saved_primask);
}

/**
 * @brief  Disable all maskable interrupts.
 * @details Saves the current PRIMASK state and disables IRQs.
 */
void Os_Cpu_DisableAllInterrupts(void)
{
    saved_primask = __get_PRIMASK();
    __disable_irq();
}

/**
 * @brief  Suspend all interrupts with nesting support.
 * @details On the first call (outermost), saves the current PRIMASK.
 *          Disables interrupts unconditionally. Supports nested calls.
 */
void Os_Cpu_SuspendAllInterrupts(void)
{
    uint32 current_primask = __get_PRIMASK();

    __disable_irq();

    if (suspend_all_count == 0U)
    {
        saved_primask_for_suspend = current_primask;
    }

    suspend_all_count++;
}

/**
 * @brief  Resume all interrupts (inverse of SuspendAllInterrupts).
 * @details Decrements the nesting counter. Only restores the original PRIMASK
 *          when the outermost nesting level is reached.
 */
void Os_Cpu_ResumeAllInterrupts(void)
{
    if (suspend_all_count > 0U)
    {
        suspend_all_count--;

        if (suspend_all_count == 0U)
        {
            __set_PRIMASK(saved_primask_for_suspend);
        }
    }
}

/**
 * @brief  Suspend OS-managed interrupts.
 * @details Uses BASEPRI to mask interrupts at or below the OS interrupt
 *          priority threshold. This allows high-priority (Category 1) ISRs
 *          to still fire while blocking lower-priority OS interrupts.
 *
 *          On Cortex-M4: BASEPRI masks all interrupts with priority >= value.
 *          We use a threshold of 0x80 (priority 8-15 in 4-bit NVIC).
 *          Higher numeric value = lower logical priority.
 *
 *          configOS_INTERRUPT_PRIORITY_THRESHOLD = 0x80
 *          This means interrupts with priority 0-7 can still preempt,
 *          while priorities 8-15 are blocked.
 */
#define OS_INTERRUPT_PRIORITY_THRESHOLD 0x80U

void Os_Cpu_SuspendOSInterrupts(void)
{
    uint32 current_basepri = __get_BASEPRI();

    __set_BASEPRI(OS_INTERRUPT_PRIORITY_THRESHOLD);

    if (suspend_os_count == 0U)
    {
        saved_basepri_for_os = current_basepri;
    }

    suspend_os_count++;
}

/**
 * @brief  Resume OS-managed interrupts.
 * @details Decrements the nesting counter. Restores the original BASEPRI
 *          only at the outermost level.
 */
void Os_Cpu_ResumeOSInterrupts(void)
{
    if (suspend_os_count > 0U)
    {
        suspend_os_count--;

        if (suspend_os_count == 0U)
        {
            __set_BASEPRI(saved_basepri_for_os);
        }
    }
}

/* ==========================================================================
 * Idle / Low-Power
 * ========================================================================= */

/**
 * @brief  Enter CPU idle state until the next interrupt.
 * @details Executes the WFI (Wait For Interrupt) instruction.
 *          The CPU will wake up on any enabled interrupt.
 */
void Os_Cpu_Idle(void)
{
    __WFI();
}

/* ==========================================================================
 * First-Task Startup Helper
 * ========================================================================= */

/**
 * @brief  Reset the interrupt suspension nesting state.
 * @details Must be called before Os_Cpu_StartFirstTask() to clear the
 *          SuspendOSInterrupts() nesting left over from the boot sequence.
 *          The outermost SuspendOSInterrupts() in Os_Start() has no matching
 *          ResumeOSInterrupts() (Os_Internal_Start never returns), so we
 *          must explicitly reset the counters and BASEPRI.
 */
void Os_Cpu_ResetInterruptSuspension(void)
{
    suspend_all_count        = 0U;
    saved_primask_for_suspend = 0U;
    suspend_os_count         = 0U;
    saved_basepri_for_os     = 0U;
    __set_BASEPRI(0U);
}

/* ==========================================================================
 * Bitmap Helper
 * ========================================================================= */

/**
 * @brief  Get the index of the most significant set bit in a 32-bit value.
 * @details Uses the CLZ (Count Leading Zeros) instruction available on
 *          Cortex-M4. Converts leading-zero count to bit index: 31 - CLZ(v).
 *
 * @param[in]  value  The 32-bit bitmap to scan.
 * @return     uint8  Bit index of the MSB (0–31). Returns 0xFF if value is 0.
 */
uint8 GetMsbIndex(uint32 value)
{
    if (value == 0U)
    {
        return 0xFFU;
    }

    return (uint8)(31U - __CLZ(value));
}
