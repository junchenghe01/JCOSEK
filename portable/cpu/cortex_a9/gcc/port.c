#include "Platform_Types.h"
#include "portmacro.h"

/**
 * @brief  Global storage for the CPU Status Register (CPSR).
 * @details Stores the CPU state (including interrupt mask bits) prior to
 *          disabling interrupts.
 *
 * @note   In accordance with OSEK/VDX requirements, nested calls to
 *         interrupt disabling services (e.g., DisableAllInterrupts) are
 *         not supported in this implementation. Therefore, a single
 *         static variable is sufficient to preserve the state.
 */
static uint32 saved_cpsr = 0;
/**
 * @brief  Nesting counter for interrupt suspension.
 * @details Tracks the depth of nested calls to SuspendAllInterrupts.
 *          Interrupts are only physically re-enabled when this counter
 *          reaches zero during ResumeAllInterrupts.
 */
static uint32 suspend_all_count = 0;
/**
 * @brief  Storage for the original CPU status (CPSR).
 * @details This variable captures the interrupt state only during the
 *          very first call to SuspendAllInterrupts (when counter is 0).
 *          Subsequent nested calls do not overwrite this value to preserve
 *          the original caller's context.
 */
static uint32 saved_cpsr_for_suspend = 0;
/**
 * @brief  Nesting counter for OS-level interrupt suspension.
 * @details Tracks the depth of nested calls to SuspendOSInterrupts.
 *          The OS will only physically restore the interrupt state when
 *          this counter returns to zero.
 */
static uint32 suspend_os_count = 0;
/**
 * @brief  Storage for the CPU status (CPSR) during OS interrupt suspension.
 * @details Specifically captures the state of the IRQ mask bit. Unlike
 *          'AllInterrupts' services, this is intended to mask only the
 *          interrupts managed by the OS kernel.
 */
static uint32 saved_cpsr_for_os = 0;
/* 初始化任务栈 */
uint32 *Os_Cpu_StackInit (void (*entry) (void), uint32 *stack, uint32 size)
{
    // 高地址 | 未初始化区域   | ← stack + size
    //       |--------------|
    //       | lr = entry   | ← sp初始指向这里，然后递减
    //       | r11 = 0      |
    //       | r10 = 0      |
    //       | r9  = 0      |
    //       | r8  = 0      |
    //       | r7  = 0      |
    //       | r6  = 0      |
    //       | r5  = 0      |
    //       | r4  = 0      | ← sp最终指向这里（栈顶）
    //       | ...          |
    // 低地址 | 栈底          | ← stack
    uint32 *sp = stack + size;                    // 栈顶 = 栈基址 + 大小, 得到的是栈顶（高地址），因为ARM使用满递减栈
    sp         = (uint32 *)((uintptr)sp & ~0x7);  // 8字节对齐

    /* 模拟 context_switch push {r4-r11, lr} */

    *(--sp) = (uint32)entry; /* lr - 任务入口地址 */
    *(--sp) = 0;             /* r11 */
    *(--sp) = 0;             /* r10 */
    *(--sp) = 0;             /* r9 */
    *(--sp) = 0;             /* r8 */
    *(--sp) = 0;             /* r7 */
    *(--sp) = 0;             /* r6 */
    *(--sp) = 0;             /* r5 */
    *(--sp) = 0;             /* r4 */
    return sp;               /* 返回新的栈顶指针 */
}
/**
 * @brief 启动第一个任务
 * @param pFirstStackPtr 第一任务的栈指针（通常从 TCB 中提取）
 *
 * 使用 __attribute__((naked)) 确保编译器不生成任何函数入口/出口指令
 */
__attribute__ ((naked)) void Os_Cpu_StartFirstTask (uint32 *pFirstStackPtr)
{
    __asm__ volatile (
        "mov sp, %0\n"       /* 1. 将传入的栈指针加载到 SP 寄存器 */
        "pop {r4-r11, lr}\n" /* 2. 从栈中弹出保存的寄存器 (对应 StackInit 时的顺序) */
        /* 注意：如果你的 StackInit 压入了 CPSR，这里需要额外的指令恢复 CPSR */
        "bx  lr\n" /* 3. 跳转到任务入口点 */
        :
        : "r"(pFirstStackPtr)
        : "memory");
}

/**
 * @brief 恢复之前保存的中断状态
 */
void Os_Cpu_EnableAllInterrupts (void)
{
    uint32 temp;

    __asm__ volatile (
        /* 1. Load the previously saved CPSR value from memory */
        "ldr %[tmp], [%[storage]] \n\t"

        /* 2. Extract only the I (bit 7) and F (bit 6) mask bits from the saved value */
        "and %[tmp], %[tmp], #0xC0 \n\t"

        /**
         * 3. Atomically update the current CPSR interrupt bits:
         *    - mrs: Read the current status register
         *    - bic: Clear only the I and F bits (bit 6 and 7)
         *    - orr: Merge the saved I/F bits into the current state
         *    - msr: Write back to the Control field of the status register
         */
        "mrs r1, cpsr           \n\t"
        "bic r1, r1, #0xC0      \n\t" /* Clear current I/F bits */
        "orr r1, r1, %[tmp]     \n\t" /* Apply saved I/F bits */
        "msr cpsr_c, r1         \n\t" /* Write back to CPSR control field */
        : [tmp] "=&r"(temp)
        : [storage] "r"(&saved_cpsr)
        : "r1", "memory", "cc");
}

void Os_Cpu_DisableAllInterrupts (void)
{
    uint32 temp;

    __asm__ volatile (
        /* 1. Read the Current Program Status Register (CPSR) into a temporary register */
        "mrs %[tmp], cpsr      \n\t"

        /* 2. Save the captured CPSR (including current interrupt masks) to global storage */
        "str %[tmp], [%[storage]] \n\t"

        /**
         * 3. Efficiently disable interrupts:
         *    'cpsid' (Change Processor State - Interrupt Disable)
         *    'if' refers to both IRQ (i) and FIQ (f) bits.
         */
        "cpsid if              \n\t"
        : [tmp] "=&r"(temp)
        : [storage] "r"(&saved_cpsr)
        : "memory", "cc");
}
void Os_Cpu_SuspendAllInterrupts (void)
{
    uint32 current_cpsr;

    /**
     * 1. Capture the Current Program Status Register (CPSR).
     *    This captures the state (including I/F bits) before any modifications.
     */
    __asm__ volatile ("mrs %[tmp], cpsr" : [tmp] "=r"(current_cpsr));

    /**
     * 2. Immediately disable interrupts (IRQ and FIQ).
     *    Using 'cpsid' for minimum latency to protect the nesting counter
     *    and global state from race conditions.
     */
    __asm__ volatile ("cpsid if" : : : "memory");

    /**
     * 3. Preserve the original state on the first nested call.
     *    If 'suspend_all_count' is 0, this is the outermost call;
     *    save the 'current_cpsr' to be restored later.
     */
    if (suspend_all_count == 0)
    {
        saved_cpsr_for_suspend = current_cpsr;
    }

    /**
     * 4. Increment the nesting counter.
     *    Ensures that the corresponding ResumeAllInterrupts knows the nesting depth.
     */
    suspend_all_count++;
}

void Os_Cpu_ResumeAllInterrupts (void)
{
    /**
     * 1. Safety check: Only proceed if there is an active suspension.
     *    This prevents the counter from underflowing.
     */
    if (suspend_all_count > 0)
    {
        /* Decrement the nesting depth */
        suspend_all_count--;

        /**
         * 2. Physical Restore: Only perform hardware restoration when the
         *    outermost nesting level is reached (counter is 0).
         */
        if (suspend_all_count == 0)
        {
            uint32 temp_cpsr = saved_cpsr_for_suspend;

            __asm__ volatile (
                /* Read current status to modify only specific bits */
                "mrs r1, cpsr           \n\t"

                /* Clear the I (bit 7) and F (bit 6) bits from current status */
                "bic r1, r1, #0xC0      \n\t"

                /* Extract saved I and F bits from the temp variable into r0 */
                "and r0, %[saved], #0xC0 \n\t"

                /* Combine the original I/F bits with the current status */
                "orr r1, r1, r0         \n\t"

                /* Write back to the Control field of the CPSR */
                "msr cpsr_c, r1         \n\t"
                :
                : [saved] "r"(temp_cpsr)
                : "r0", "r1", "memory", "cc");
        }
    }
}

void Os_Cpu_SuspendOSInterrupts (void)
{
    uint32 current_cpsr;

    /**
     * 1. Capture the Current Program Status Register (CPSR).
     *    This stores the current interrupt mask state before modification.
     */
    __asm__ volatile ("mrs %[tmp], cpsr" : [tmp] "=r"(current_cpsr));

    /**
     * 2. Immediately disable IRQ (Interrupt Request).
     *    Using 'cpsid i' masks only standard interrupts.
     *    The 'f' bit (FIQ) is intentionally left unchanged to support Cat1 ISRs.
     */
    __asm__ volatile ("cpsid i" : : : "memory");

    /**
     * 3. Handle nesting logic:
     *    If this is the outermost call (count is 0), preserve the original CPSR.
     */
    if (suspend_os_count == 0)
    {
        saved_cpsr_for_os = current_cpsr;
    }

    /* Increment the nesting depth counter */
    suspend_os_count++;
}

void Os_Cpu_ResumeOSInterrupts (void)
{
    /**
     * 1. Underflow protection:
     *    Ensure there is an active OS suspension before proceeding.
     */
    if (suspend_os_count > 0)
    {
        /* Decrement the nesting depth */
        suspend_os_count--;

        /**
         * 2. Physical Restore:
         *    Perform hardware register update only at the outermost nesting level.
         */
        if (suspend_os_count == 0)
        {
            uint32 temp_cpsr = saved_cpsr_for_os;

            __asm__ volatile (
                /* Read the current status register into r1 */
                "mrs r1, cpsr           \n\t"

                /* Clear only the I-bit (bit 7) in the current status */
                "bic r1, r1, #0x80      \n\t"

                /* Extract the original I-bit from the saved CPSR into r0 */
                "and r0, %[saved], #0x80 \n\t"

                /* Merge the original I-bit back into the current status */
                "orr r1, r1, r0         \n\t"

                /* Apply the modification to the CPSR Control byte */
                "msr cpsr_c, r1         \n\t"
                :
                : [saved] "r"(temp_cpsr)
                : "r0", "r1", "memory", "cc");
        }
    }
}

/**
 * @brief 进入低功耗等待模式
 * 在 Cortex-A9 中，WFI 指令会挂起执行，直到中断发生。
 */
inline void Os_Cpu_Idle (void)
{
    __asm__ volatile ("wfi" ::: "memory");
}
/**
 * @brief 计算 32 位变量中最高位 (MSB) 的位置
 * @param value 输入的 32 位位图
 * @return 最高置位 (1) 的索引 (0-31)
 *
 * 在 ARMv7-A (Cortex-A9) 中，CLZ 计算领先的 0 的个数。
 * 索引 = 31 - CLZ(value)
 */
uint8 GetMsbIndex (uint32 value)
{
    uint32 leading_zeros;

    if (value == 0)
    {
        return 0xFF; /* 返回非法值表示位图为空 */
    }

    /* Hardware acceleration: Find the most significant bit set (MSB) */
    __asm__ volatile ("clz %0, %1" /* Count leading zeros in 'temp' */
                      : "=r"(leading_zeros)
                      : "r"(value));

    return (uint8)(31u - leading_zeros);
}
