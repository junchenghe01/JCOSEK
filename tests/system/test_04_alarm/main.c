#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "Os.h"
#include <stdbool.h>
#include <stdarg.h>
#include "Os_Internal.h"

#define UART0_BASE  0x10009000
#define UART_DR     (*(volatile uint32*)(UART0_BASE + 0x00))
#define UART_FR     (*(volatile uint32*)(UART0_BASE + 0x18))

// 发送原始字节
void Uart_SendChar(uint8 c) {
    // 等待 TX FIFO 不满 (TXFF 标志位)
    while (UART_FR & (1u << 5)); 
    UART_DR = c;
}

// 发送字符串 (最常用的调试手段)
void Uart_SendString(const char* str) {
    while (str && *str) {
        if (*str == '\n') Uart_SendChar('\r'); // 兼容 QEMU 终端换行
        Uart_SendChar((uint8)*str++);
    }
}
void Uart_SendHex(uint32 val) {
    const char hex_map[] = "0123456789ABCDEF";
    Uart_SendString("0x");
    for (int i = 28; i >= 0; i -= 4) {
        Uart_SendChar(hex_map[(val >> i) & 0x0F]);
    }
}
/**
 * @brief 极其简单的串口格式化打印，仅支持 %x 和 %s
 */
#include <stdarg.h>

/**
 * @brief 支持 %x 和 %d 的轻量级打印
 */
void Uart_Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++; // 跳过 '%'
            if (*format == 'x') {
                uint32_t val = va_arg(args, uint32_t);
                Uart_SendHex(val);
                format++;
            } 
            else if (*format == 'd') {
                int32_t val = va_arg(args, int32_t);
                
                // 处理负数
                if (val < 0) {
                    Uart_SendChar('-');
                    val = -val;
                }
                
                // 处理 0 的特殊情况
                if (val == 0) {
                    Uart_SendChar('0');
                } else {
                    // 简单的十进制拆解（逆序存入临时数组再正序打印）
                    char buf[11]; // int32 最大 10 位 + 结束符
                    int i = 0;
                    while (val > 0) {
                        buf[i++] = (val % 10) + '0';
                        val /= 10;
                    }
                    while (--i >= 0) {
                        Uart_SendChar(buf[i]);
                    }
                }
                format++;
            }
        } else {
            Uart_SendChar(*format);
            if (*format == '\n') Uart_SendChar('\r');
            format++;
        }
    }
    va_end(args);
}

CounterControlBlock counters[1] = {0};
AlarmControlBlock alarms[1] = {0};
EventControlBlock events[2] = {0};
#define EVENT_0_MASK 0x01
#define EVENT_1_MASK 0x02
ResourceControlBlock resources[2] = {0};
TaskControlBlock tasks[4] = {0};
OsControlBlock ocb = {0};
static int current = -1;

uint32_t *pSize = &(ocb.TasksSize);
extern void Os_ContextSwitch(TaskControlBlock* old_tcb, TaskControlBlock* new_tcb);


/* 定义寄存器地址 (以 Timer 0 为例) */
#define TIMER0_BASE      0x10011000
#define TIMER0_LOAD      (*(volatile uint32_t *)(TIMER0_BASE + 0x00))
#define TIMER0_VALUE     (*(volatile uint32_t *)(TIMER0_BASE + 0x04))
#define TIMER0_CONTROL   (*(volatile uint32_t *)(TIMER0_BASE + 0x08))
#define TIMER0_INTCLR    (*(volatile uint32_t *)(TIMER0_BASE + 0x0C))
void Os_Arch_Timer_Init(void) {
    /* 1. 先禁用定时器进行配置 */
    TIMER0_CONTROL = 0;

    /* 2. 设置加载值：决定 Tick 频率 */
    /* QEMU VExpress 时钟频率通常是 1MHz，设为 1000 则 1ms 触发一次中断 */
    TIMER0_LOAD = 1000;
    // TIMER0_LOAD    = 0x10000; // 适当加大计数值，防止触发过快

    /* 3. 配置控制寄存器：
     * Bit 7: Timer Enable (1 = 开启)
     * Bit 6: Timer Mode (1 = Periodic 周期模式)
     * Bit 5: Interrupt Enable (1 = 开启中断)
     * Bit 1: Size (1 = 32-bit counter)
     */
    TIMER0_CONTROL = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1);
    // /* 
    //  * 控制寄存器位定义:
    //  * [7] Enable: 1
    //  * [6] Mode: 1 (Periodic)
    //  * [5] IntEnable: 1
    //  * [3:2] Prescale: 00 (No division)
    //  * [1] Size: 1 (32-bit counter)
    //  * [0] One-shot: 0 (Wrapping)
    //  */
    // TIMER0_CONTROL = 0xE2; // 1110 0010
    /* 4. 清除可能存在的悬挂中断 */
    TIMER0_INTCLR = 1;
}
// 严格对应 QEMU vexpress-a9 映射
// https://gitlab.com/qemu-project/qemu/-/blob/master/hw/arm/vexpress.c
#define GIC_DIST_BASE  0x1e001000
#define GIC_CPU_BASE   0x1e000100  // 修正：从 0x000 改为 0x100

void Os_Arch_GIC_Init(void) {
    // 1. Distributor: 使能 ID 34
    // 每 32 个中断一个寄存器，ID 34 在 ISENABLER[1] 的第 2 位 (34-32=2)
    *((volatile uint32_t *)(GIC_DIST_BASE + 0x104)) = (1 << 2); 

    // 2. Distributor: 目标 CPU 分配 (ITARGETSR8)
    // ID 34 是第 8 个寄存器的第 16-23 位
    uint32_t target = *((volatile uint32_t *)(GIC_DIST_BASE + 0x820));
    target &= ~(0xFF << 16);
    target |= (0x01 << 16); // 目标指向 CPU0
    *((volatile uint32_t *)(GIC_DIST_BASE + 0x820)) = target;

    // 3. Distributor: 全局开启
    *((volatile uint32_t *)(GIC_DIST_BASE + 0x000)) = 0x1;

    // 4. CPU Interface: 优先级过滤 (PMR)
    // 设置为 0xF0，确保能通过 0xA0 优先级的中断
    *((volatile uint32_t *)(GIC_CPU_BASE + 0x004)) = 0xF0;

    // 5. CPU Interface: 全局开启
    *((volatile uint32_t *)(GIC_CPU_BASE + 0x000)) = 0x1;
}


#define GICC_IAR       (*(volatile uint32_t *)(GIC_CPU_BASE + 0x00C))
#define GICC_EOIR      (*(volatile uint32_t *)(GIC_CPU_BASE + 0x010))
void Timer_ISR_Handler(void) {
    // 1. 从 GIC 读取中断号 (Acknowledge)
    uint32_t iar = GICC_IAR;
    uint32_t irq_id = iar & 0x3FF;

    // 2. 判断是否为定时器中断 (ID 34)
    if (irq_id == 34) {
        // 清除定时器硬件位
        TIMER0_INTCLR = 1; 
        
        // 驱动 OS 计数器
        IncrementCounter(0); 
    }

    // 3. 必须通知 GIC 中断处理结束 (EOI)
    // 否则 CPU 永远不会接收下一次中断
    GICC_EOIR = iar;
}
// 这个测试用例将展示：Task_High 设置一个报警器，然后通过计数器的增长，自动在未来触发 Task_Low。
// 1. 预设条件
// SystemCounter: 最大值设为 2000（为了快速观察翻转）。
// Task_High (Prio 30): 负责启动报警器。
// Task_Low (Prio 10): 接受报警器激活，打印日志。
// Alarm_Cyclic (ID 0): 绑定到 Task_Low。
// 3. 预期运行现象 (判定标准)
// 启动：Task_High 打印当前 Tick（假设是 0）并设置报警器。
// 首次触发：当底层硬件中断驱动 Counter 增加到 500 时，Task_Low 第一次运行，输出 Count: 1 at Tick: 500。
// 周期触发：当 Counter 增加到 1500 时，输出 Count: 2 at Tick: 1500。
// 回绕触发：
// Counter 达到 2000 后归零。
// 下一次触发点应为 (1500 + 1000) % 2001 = 499。
// 当 Counter 到达 499 时，输出 Count: 3 at Tick: 499。
// 结束：CancelAlarm 被调用，之后不再有输出。
// 4. 关键检查点
// 硬件节拍：确保你的 Os_Arch_Timer_Init 已经运行，且 CPSIE i 开启了全局中断。
// 状态切换：在 Os_Internal_TriggerAlarmAction 处通过 VSCode 调试器 观察是否准确进入了 ActivateTask(Task_Low)。
// 精度：如果打印出的 Tick 值和预期的（500, 1500, 499）误差超过 1 个位，检查 Os_Counter_Tick 是否在判断后才增加计数，或者是 CancelAlarm 的原子性有问题。

#define STACK_SIZE 1024
static uint32_t Task_Init_Stack[STACK_SIZE]; // 4KB
static uint32_t Task_Idle_Stack[STACK_SIZE]; // 4KB
static uint32_t Task_High_Stack[STACK_SIZE]; // 4KB
static uint32_t Task_Low_Stack[STACK_SIZE]; // 4KB
TaskType Task_Init_id = 0;
TaskType Task_Idle_id = 1;
TaskType Task_High_id = 2;
TaskType Task_Low_id = 3;

void Task_Init(void) {
    Uart_SendString("[Task_Init] System Initialization Complete. Terminating Init Task...\n");
    TerminateTask();
    // ChainTask(Task_Idle_id);
}
void Task_Idle(void) {
    // static boolean activateTaskFlag = TRUE;
    while(1) {
        // 1. 周期性动作：比如喂看门狗
        // WDT_Feed();

        // 2. 进入低功耗模式（可选）
        __asm__ volatile ("wfi"); 

        // 3. 或者只是简单地打印/闪灯（仅调试用）
        Uart_SendString("Idle...\n");
        Uart_Printf("AlarmTime Value: %d\n", ocb.ACB[0].AlarmTime); // TODO: 无任务后会跑飞
        // if (activateTaskFlag == TRUE)
        // {
        //     activateTaskFlag = FALSE;
        //     ActivateTask(Task_High_id);
        // }
        
    }
}
// --- 任务1：扩展任务 (消费者) ---
void Task_High(void) {
    TickType now;
    AlarmBaseType base;

    Uart_SendString("\n--- [Alarm Test Start] ---\n");

    // 1. 测试 GetAlarmBase
    GetAlarmBase(0, &base);
    Uart_Printf("Counter Max Value: %d\n", base.maxallowedvalue);

    // 2. 获取当前计数器值
    GetCounterValue(0, &now);
    Uart_Printf("Current Tick: %d\n", now);

    // 3. 设置一个相对报警器
    // 500 ticks 后第一次触发，之后每 1000 ticks 触发一次
    Uart_SendString("Setting RelAlarm: 500 start, 1000 cycle...\n");
    // SetRelAlarm(0, 500, 1000);
    SetRelAlarm(0,50,100);

    // 4. 验证 GetAlarm (剩余时间)
    TickType remaining;
    GetAlarm(0, &remaining);
    Uart_Printf("Ticks until expiry: %d\n", remaining);

    Uart_SendString("[Task_High] Terminating, waiting for Alarm...\n");
    TerminateTask();
}

// --- 任务2：基本任务 (生产者) ---
void Task_Low(void) {
    static uint32_t count = 0;
    TickType now;
    
    GetCounterValue(0, &now);
    count++;

    Uart_Printf(" >>> [ALARM TRIGGERED] Count: %d at Tick: %d\n", count, now);

    // if (count >= 3) { // TODO: 数值太大就会跑飞
    if(count >= 2){
        Uart_SendString("Test Finished, Cancelling Alarm...\n");
        CancelAlarm(0);
    }
    
    TerminateTask();
}

void Os_InitializeScheduler(void) {
    // 1. 位图全部清零（表示所有优先级都没有就绪任务）
    for (int i = 0; i < BITMAP_SIZE; i++) {
        ocb.ReadyBitmap[i] = 0;
    }
    
    // 2. 优先级头指针数组清空
    for (int i = 0; i < OS_PRIO_LEVELS; i++) {
        ocb.ReadyQueues[i] = NULL;
    }

    ocb.pCurrentTask = NULL;
}
void Os_AddTaskToReadyQueue(TaskControlBlock* pTcb) {
    uint8_t prio = pTcb->CurrentPriority; // 0-255

    // --- 第一步：更新位图 ---
    // 找到该优先级属于 8 个 uint32 中的哪一组
    uint32_t group = prio / 32;
    // 找到组内的具体哪一位 (31 - 偏移，方便使用 CLZ 指令)
    // uint32_t bit = 31 - (prio % 32);
    uint32_t bit = prio % 32; //直接对应第 bit 位
    ocb.ReadyBitmap[group] |= (1 << bit);

    // --- 第二步：挂载到双向循环链表 ---
    if (ocb.ReadyQueues[prio] == NULL) {
        // 如果该优先级目前没任务，自己指向自己（循环链表特征）
        ocb.ReadyQueues[prio] = pTcb;
        pTcb->NextReady = pTcb;
        pTcb->PrevReady = pTcb;
    } else {
        // 如果已有任务，插入到末尾（即头节点的前面）
        TaskControlBlock* head = ocb.ReadyQueues[prio];
        TaskControlBlock* tail = head->PrevReady;

        pTcb->NextReady = head;
        pTcb->PrevReady = tail;
        tail->NextReady = pTcb;
        head->PrevReady = pTcb;
    }
}
/* 初始化任务栈 */
void Os_CreateTask(TaskRefType taskId, void (*entry)(void), uint32_t *stack, 
                    uint32_t size, uint32_t prio, TaskKindType kind, boolean isAutoStart)
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
    uint32_t *sp = stack + size;      // 栈顶 = 栈基址 + 大小, 得到的是栈顶（高地址），因为ARM使用满递减栈
    sp = (uint32_t *)((uintptr_t)sp & ~0x7);  // 8字节对齐


    /* 模拟 context_switch push {r4-r11, lr} */

    *(--sp) = (uint32_t)entry; /* lr - 任务入口地址 */
    *(--sp) = 0; /* r11 */
    *(--sp) = 0; /* r10 */
    *(--sp) = 0; /* r9 */
    *(--sp) = 0; /* r8 */
    *(--sp) = 0; /* r7 */
    *(--sp) = 0; /* r6 */
    *(--sp) = 0; /* r5 */
    *(--sp) = 0; /* r4 */
    tasks[ocb.TasksSize].StackPtr = sp;
    tasks[ocb.TasksSize].StackSize = size;
    tasks[ocb.TasksSize].TaskState = TASK_STATE_SUSPENDED;
    tasks[ocb.TasksSize].StaticPriority = prio;
    tasks[ocb.TasksSize].CurrentPriority = prio;
    tasks[ocb.TasksSize].TaskEntry = entry;
    tasks[ocb.TasksSize].TaskID = taskId;
    tasks[ocb.TasksSize].TaskKind = kind;
    tasks[ocb.TasksSize].isAutoStart = isAutoStart;
    ocb.TasksSize++;
}
void StartHooks(void)
{

};
/**
 * 在内核开发中，如果你不想链接体积庞大的标准 C 库（避免引入不必要的依赖或 __errno 等麻烦），手动实现一个 memcpy 是非常稳妥的做法。
 */
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}
void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    while (n--) {
        *p++ = (char)c;
    }
    return s;
}

int main(void)
{
    uint32_t size_of_tcb = sizeof(OsControlBlock);//1112
    uint32_t size_of_sp = sizeof(uint32_t*);//4
    uint32_t size_of_state = sizeof(TaskControlBlock);//120
    uint32_t offset_of_TaskState = offsetof(TaskControlBlock, TaskState);//8
    uint32_t offset_of_Tasks = offsetof(OsControlBlock, Tasks);//4
    uint32_t offset_of_StackPtr = offsetof(TaskControlBlock, StackPtr);// 88
    uint32_t offset_of_IntNestingCount = offsetof(OsControlBlock, IntNestingCount);//1108
    uint32_t offset_of_preempt_flag = offsetof(OsControlBlock, preempt_flag);//1112
    counters[0].CounterID = 0; /* SystemCounter */
    counters[0].maxallowedvalue = 0xFFFF;    /* 65535 后归零 */
    counters[0].ticksperbase = 1;            /* 1:1 硬件/软件节拍 */
    counters[0].mincycle = 10;               /* 周期不小于 10 ticks */
    counters[0].CurrentValue = 0;             /* 初始计数值 */
    ocb.CCB = &counters;
    ocb.CountersSize = 1;

    alarms[0].AlarmID = 0;
    alarms[0].CounterID = 0;               /* 绑定到上面的 SystemCounter */
    alarms[0].AlarmTime = 0;               /* 初始为0，由 SetRelAlarm 计算 */
    alarms[0].IsActive = FALSE;            /* 初始不启动 */
    alarms[0].CycleTime = 0;               /* 初始为0，由 SetRelAlarm 设置 */
    alarms[0].Action = ALARM_ACTION_ACTIVATE_TASK;
    alarms[0].TargetTaskID = Task_Low_id;            /* 假设 Task_Low 的 ID 是 2 */
    alarms[0].TargetEvent = 0;             /* 非事件动作，设为 0 */
    alarms[0].AlarmCallback = NULL;         /* 非回调动作，设为 NULL */
    ocb.ACB = &alarms;
    ocb.AlarmsSize = 2;
    ocb.preempt_flag = 0;

    Os_InitializeScheduler(); // 先洗牌
    current = 0;
    ocb.TasksSize = 0;
    Os_CreateTask(&Task_Init_id, 
        Task_Init, 
        Task_Init_Stack, 
        STACK_SIZE, 
        0,
        TASK_KIND_BASIC_TASK,
        TRUE);
    Os_CreateTask(&Task_Idle_id, 
        Task_Idle, 
        Task_Idle_Stack, 
        STACK_SIZE, 
        1,
        TASK_KIND_BASIC_TASK,
        TRUE);
    Os_CreateTask(&Task_High_id, 
        Task_High, 
        Task_High_Stack, 
        STACK_SIZE, 
        30,
        TASK_KIND_EXTENDED_TASK,
        TRUE);
    Os_CreateTask(&Task_Low_id, 
        Task_Low, 
        Task_Low_Stack, 
        STACK_SIZE, 
        10,
        TASK_KIND_BASIC_TASK,
        TRUE);
    ocb.Tasks = tasks;
    Os_Arch_GIC_Init();
    Os_Arch_Timer_Init();
    Os_Arch_EnableInterrupts();

    AppModeType mode = 0;
    StartOS(mode);

    while (1);
}