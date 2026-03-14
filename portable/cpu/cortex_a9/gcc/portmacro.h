#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "Platform_Types.h"
// 1. 基础数据类型重定义 (Type Definitions)
typedef struct ContextInfo
{
    uint32 ctx_r4;   /* Offset 0  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r5;   /* Offset 4  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r6;   /* Offset 8  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r7;   /* Offset 12  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r8;   /* Offset 16  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r9;   /* Offset 20  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r10;  /* Offset 24  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r11;  /* Offset 28  	通用数据，必须在切换时手动压栈/保存 */
    uint32 ctx_r0;   /* Offset 32    C 函数参数寄存器*/
    uint32 ctx_r1;   /* Offset 36    C 函数参数寄存器*/
    uint32 ctx_r2;   /* Offset 40    C 函数参数寄存器*/
    uint32 ctx_r3;   /* Offset 44    C 函数参数寄存器*/
    uint32 ctx_r12;  /* Offset 48	R12 (IP)	内部调用临时寄存器*/
    uint32 ctx_sp;   /* Offset 52	R13 (SP)	核心：指示任务当前的栈位置*/
    uint32 ctx_lr;   /* Offset 56	R14 (LR)	核心：函数返回的地址*/
    uint32 ctx_pc;   /* Offset 60	R15 (PC)	核心：断点地址（下次从哪跑）*/
    uint32 ctx_cpsr; /* Offset 64	CPSR	核心：CPU 模式与中断状态*/
} ContextInfo;

// typedef uint32 StackType_t;
// typedef uint32 TickType_t;
// typedef uint8  StatusType;

// #define portSTACK_GROWTH (-1)  // A9 堆栈向下增长
// #define portBYTE_ALIGNMENT 8   // A9 要求 8 字节对齐

// 2. 临界区管理 (Critical Sections)
// // 保存当前中断状态并关闭中断
// #define portENTER_CRITICAL() __asm volatile ("CPSID i" ::: "memory")

// // 恢复中断
// #define portEXIT_CRITICAL() __asm volatile ("CPSIE i" ::: "memory")

// // 更高级的实现通常会保存 CPSR 值，防止嵌套调用时提前开启中断

// 3. 任务调度触发 (Scheduler Bridging)
// // 对应 OSEK 的调度触发点
// #define portYIELD() __asm volatile ("SVC 0" ::: "memory")

// // 或者如果你的调度器直接跳转：
// extern void Os_ContextSwitch (void);
// #define portYIELD_WITHIN_API() Os_ContextSwitch ()

// 4. 编译器特有属性 (Compiler Attributes)
// #define portDONT_DISCARD __attribute__ ((used))
// #define portNAKED __attribute__ ((naked))
// #define portALIGN(x) __attribute__ ((aligned (x)))

// 5. 给你的 OSEK OS 特别增加：中断优先级过滤
// Cortex-A9 使用 GIC 中断控制器。如果你的 OSEK 支持 可抢占的中断 (ISR2)，你还需要在 portmacro.h 中定义操作 GIC
// 优先级屏蔽寄存器 (GICC_PMR) 的宏。

uint32     *Os_Cpu_StackInit (void (*entry) (void), uint32 *stack, uint32 size);
extern void Os_Cpu_StartFirstTask (uint32 *pFirstStackPtr);
/* 为了以后切换架构方便，可以定义一个别名宏 */
#define portSTART_FIRST_TASK(pStack) Os_Cpu_StartFirstTask (pStack)

extern void Os_Cpu_EnableAllInterrupts (void);

/* 映射到 OSEK 标准 API */
#define portENABLE_ALL_INTERRUPTS() Os_Cpu_EnableAllInterrupts ()

extern void Os_Cpu_DisableAllInterrupts (void);
#define portDISABLE_ALL_INTERRUPTS() Os_Cpu_DisableAllInterrupts ()
extern void Os_Cpu_SuspendAllInterrupts (void);
#define portSUSPEND_ALL_INTERRUPTS() Os_Cpu_SuspendAllInterrupts ()
extern void Os_Cpu_ResumeAllInterrupts (void);
#define portRESUME_ALL_INTERRUPTS() Os_Cpu_ResumeAllInterrupts ()
extern void Os_Cpu_SuspendOSInterrupts (void);
#define portSUSPEND_OS_INTERRUPTS() Os_Cpu_SuspendOSInterrupts ()
extern void Os_Cpu_ResumeOSInterrupts (void);
#define portRESUME_OS_INTERRUPTS() Os_Cpu_ResumeOSInterrupts ()
extern void Os_Cpu_Idle (void);
/* 也可以映射到一个更通用的名称 */
#define portWAIT_FOR_INTERRUPT() Os_Cpu_Idle ()
extern uint8 GetMsbIndex (uint32 value);
#define portGET_MSB_INDEX(result, value) ((result) = GetMsbIndex (value))
#endif /* PORTMACRO_H */