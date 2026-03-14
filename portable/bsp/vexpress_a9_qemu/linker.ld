ENTRY(_start)

MEMORY
{
    /* VExpress-A9 的 DDR RAM 起始于 0x60000000 */
    RAM (rwx) : ORIGIN = 0x60000000, LENGTH = 128M
}

SECTIONS
{
    . = ORIGIN(RAM);

    /* 确保向量表处于镜像的最前端 */
    .text : {
        KEEP(*(.vectors))   /* 放置 startup.S 中定义的 .vectors 段 */
        *(.text*)
    } > RAM

    .rodata : {
        *(.rodata*)
    } > RAM

    .data : {
        *(.data*)
    } > RAM

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM
    /* --- 新增：NOINIT 段 --- */
    /* [SWS_Mcu_00254] 跨复位变量存放区 */
    .noinit (NOLOAD) : {
        . = ALIGN(4);
        __noinit_start = .;
        *(.noinit*)      /* 匹配 __attribute__((section(".noinit"))) */
        __noinit_end = .;
    } > RAM
    /* --- 堆栈分配区 --- */
    . = ALIGN(8);
    /* 1. IRQ 模式栈：专门用于执行 IRQ_Handler */
    . = . + 0x2000;         /* 分配 8KB 给 IRQ 栈 */
    /* TODO (HE Juncheng/2026-03-15): 中断栈 (ISR Stack)：
        为了防止任务栈溢出，OS 通常维护一个独立的 Os_ISRStack。在进入 ISR 2 时，硬件或软件需将堆栈指针（SP）切换到中断栈。*/
    __irq_stack_top = .;
    /* 2. 内核主栈 (SVC 模式)：用于 main 函数和任务初始化 */
    . = . + 0x4000;         /* 分配 16KB 给 SVC 栈 */
    __stack_top = .;

    /* 计算剩余内存给应用（可选） */
    __heap_start = ALIGN(8);
}
