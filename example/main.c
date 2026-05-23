#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Os.h"
#include "Os_Internal.h"
#include "isr.h"
#include "uart.h"

CounterControlBlock counters[1] = {0};
AlarmControlBlock   alarms[1]   = {0};
EventControlBlock   events[2]   = {0};
#define EVENT_0_MASK 0x01
#define EVENT_1_MASK 0x02
ResourceControlBlock resources[2] = {0};
ISRControlBlock      isrs[2]      = {0};
TaskControlBlock     tasks[4]     = {0};
OsControlBlock       ocb          = {0};
static int           current      = -1;

uint32*     pSize = &(ocb.TasksSize);
extern void Os_ContextSwitch (TaskControlBlock* old_tcb, TaskControlBlock* new_tcb);

#define STACK_SIZE 1024
static uint32 Task_Init_Stack[STACK_SIZE];  // 4KB
static uint32 Task_Idle_Stack[STACK_SIZE];  // 4KB
static uint32 Task_High_Stack[STACK_SIZE];  // 4KB
static uint32 Task_Low_Stack[STACK_SIZE];   // 4KB
TaskType      Task_Init_id = 0;
TaskType      Task_Idle_id = 1;
TaskType      Task_High_id = 2;
TaskType      Task_Low_id  = 3;
uint32        counter      = 0;
void          User_Request_PowerOff ()
{
    // 1. 执行应用层的保存（如写入 NVM/EEPROM）
    // Save_Mileage_Data(); // 保存里程等关键数据

    // 2. 调用 ShutdownOS，传入 E_OK 表示“正常关闭”
    ShutdownOS (E_OK);
}
void Task_Init (void)
{
    Uart_SendString ("[Task_Init] System Initialization Complete. Terminating Init Task...\n");
    TerminateTask ();
    // ChainTask(Task_Idle_id);
}
void Task_Idle (void)
{
    // static boolean activateTaskFlag = TRUE;
    while (1)
    {
        // 1. 周期性动作：比如喂看门狗
        // WDT_Feed();

        // 3. 或者只是简单地打印/闪灯（仅调试用）
        Uart_SendString ("Idle...\n");
        Uart_Printf ("AlarmTime Value: %d\n", ocb.ACB[0].AlarmTime);  // TODO (HE Juncheng/2026-03-15): 无任务后会跑飞

        // 2. 进入低功耗模式（可选）
        // __asm__ volatile ("wfi");
        counter++;
        if (counter > 150)
        {
            User_Request_PowerOff ();
        }
    }
}
// --- 任务1：扩展任务 (消费者) ---
void Task_High (void)
{
    // TickType now;
    // AlarmBaseType base;

    // Uart_SendString("\n--- [Alarm Test Start] ---\n");

    // // 1. 测试 GetAlarmBase
    // GetAlarmBase(0, &base);
    // Uart_Printf("Counter Max Value: %d\n", base.maxallowedvalue);

    // // 2. 获取当前计数器值
    // GetCounterValue(0, &now);
    // Uart_Printf("Current Tick: %d\n", now);

    // // 3. 设置一个相对报警器
    // // 500 ticks 后第一次触发，之后每 1000 ticks 触发一次
    // Uart_SendString("Setting RelAlarm: 500 start, 1000 cycle...\n");
    // // SetRelAlarm(0, 500, 1000);
    // SetRelAlarm(0,50,100);

    // // 4. 验证 GetAlarm (剩余时间)
    // TickType remaining;
    // GetAlarm(0, &remaining);
    // Uart_Printf("Ticks until expiry: %d\n", remaining);

    // Uart_SendString("[Task_High] Terminating, waiting for Alarm...\n");
    Uart_SendString ("[Task_High] Task is running...\n");
    TerminateTask ();
}

// --- 任务2：基本任务 (生产者) ---
void Task_Low (void)
{
    // static uint32 count = 0;
    // TickType now;

    // GetCounterValue(0, &now);
    // count++;

    // Uart_Printf(" >>> [ALARM TRIGGERED] Count: %d at Tick: %d\n", count, now);

    // // if (count >= 3) { // TODO (HE Juncheng/2026-03-15): 数值太大就会跑飞
    // if(count >= 2){
    //     Uart_SendString("Test Finished, Cancelling Alarm...\n");
    //     CancelAlarm(0);
    // }
    Uart_SendString ("[Task_Low] Task is running...\n");
    TerminateTask ();
}

void Os_InitializeScheduler (void)
{
    // 1. 位图全部清零（表示所有优先级都没有就绪任务）
    for (int i = 0; i < BITMAP_SIZE; i++)
    {
        ocb.ReadyBitmap[i] = 0;
    }

    // 2. 优先级头指针数组清空
    for (int i = 0; i < OS_PRIO_LEVELS; i++)
    {
        ocb.ReadyQueues[i] = NULL;
    }

    ocb.pCurrentTask = NULL;
}
void Os_AddTaskToReadyQueue (TaskControlBlock* pTcb)
{
    uint8 prio = pTcb->CurrentPriority;  // 0-255

    // --- 第一步：更新位图 ---
    // 找到该优先级属于 8 个 uint32 中的哪一组
    uint32 group = prio / 32;
    // 找到组内的具体哪一位 (31 - 偏移，方便使用 CLZ 指令)
    // uint32 bit = 31 - (prio % 32);
    uint32 bit = prio % 32;  // 直接对应第 bit 位
    ocb.ReadyBitmap[group] |= (1 << bit);

    // --- 第二步：挂载到双向循环链表 ---
    if (ocb.ReadyQueues[prio] == NULL)
    {
        // 如果该优先级目前没任务，自己指向自己（循环链表特征）
        ocb.ReadyQueues[prio] = pTcb;
        pTcb->NextReady       = pTcb;
        pTcb->PrevReady       = pTcb;
    }
    else
    {
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
void Os_CreateTask (TaskRefType taskId, void (*entry) (void), uint32* stack, uint32 size, uint32 prio,
                    TaskKindType kind, boolean isAutoStart)
{
    tasks[ocb.TasksSize].StackPtr        = Os_Cpu_StackInit (entry, stack, size);
    tasks[ocb.TasksSize].StackSize       = size;
    tasks[ocb.TasksSize].TaskState       = TASK_STATE_SUSPENDED;
    tasks[ocb.TasksSize].StaticPriority  = prio;
    tasks[ocb.TasksSize].CurrentPriority = prio;
    tasks[ocb.TasksSize].TaskEntry       = entry;
    tasks[ocb.TasksSize].TaskID          = taskId;
    tasks[ocb.TasksSize].TaskKind        = kind;
    tasks[ocb.TasksSize].isAutoStart     = isAutoStart;
    if (isAutoStart)
    {
        tasks[ocb.TasksSize].activation_count = 1;
    }
    else
    {
        tasks[ocb.TasksSize].activation_count = 0;
    }
    ocb.TasksSize++;
}
void StartHooks (void) {

};
/**
 * 在内核开发中，如果你不想链接体积庞大的标准 C 库（避免引入不必要的依赖或 __errno 等麻烦），手动实现一个 memcpy
 * 是非常稳妥的做法。
 */
void* memcpy (void* dest, const void* src, size_t n)
{
    char*       d = (char*)dest;
    const char* s = (const char*)src;

    while (n--)
    {
        *d++ = *s++;
    }

    return dest;
}
void* memset (void* s, int c, size_t n)
{
    char* p = (char*)s;
    while (n--)
    {
        *p++ = (char)c;
    }
    return s;
}
/*
类型	            中断号 (ID)	    位宽	    作用范围	           主要用途
Private Timer	    29	            32-bit	    每个 CPU 核心私有	高精度延时、进程调度
Private Watchdog	30	            32-bit	    每个 CPU 核心私有	系统监控、辅助计时
Global Timer	    27	            64-bit	    所有 CPU 核心共享	全局同步、高精度时间戳
Dual-Timer (SP804)	48/49 等	    32-bit	    系统级共享外设	    传统的 OS 系统时钟
RTC (PL031)	        34	            32-bit	    系统级共享外设	    日历、墙上时间 (Wall Clock)
*/
/*
27: // Global Timer
*/
void Global_Timer_Handler (void)
{
    *(volatile uint32*)(MPC_GLOB_BASE + 0x0C) = 0x1;
}
/*
29: // Private Timer
*/
void Private_Timer_Handler (void)
{
    // 执行一类中断的高速逻辑，不得调用 OS API
    // Fast_Sampling_Process();
    Uart_SendString ("Private_Timer_Handler executed.\n");
    *(volatile uint32*)(MPC_PRIV_BASE + 0x0C) = 0x1;
}
/*
30: // Watchdog
*/
void Watchdog_Handler (void)
{
    *(volatile uint32*)(MPC_PRIV_BASE + 0x2C) = 0x1;
}
/*
34: // RTC
*/
void RTC_Handler (void)
{
    *(volatile uint32*)(SYS_RTC_BASE + 0x0C) = 0x1;
}
/*
48: // SP804
*/
void SP804_Handler (void)
{
    // 检查 Timer 1 是否触发
    if (*(volatile uint32*)SP804_T1_MIS != 0)
    {
        // 1. 硬件清除 Timer 1 中断 (写入任意值)
        *(volatile uint32*)SP804_T1_INTCLR = 0x1;

        // 2. 执行系统心跳逻辑
        // 例如：IncrementCounter(SystemCounter);
        IncrementCounter (0);  // 驱动 OS 计数器
        // 或者：ActivateTask(Task_SystemTick);
    }
    // 检查 Timer 2 是否触发
    if (*(volatile uint32*)SP804_T2_MIS != 0)
    {
        // 1. 硬件清除 Timer 2 中断
        *(volatile uint32*)SP804_T2_INTCLR = 0x1;

        // 2. 执行其他业务逻辑
        // 例如：ActivateTask(Task_OtherService);
        ActivateTask (Task_Low_id);
    }
    // IncrementCounter(0); // 驱动 OS 计数器
    // *(volatile uint32 *)(SYS_SP804_BASE + 0x0C) = 0x1;
}

ISRControlBlock* GetIsrControlBlockByIsrId (uint32 isrId)
{
    ISRControlBlock* pIsrCB = NULL;
    for (uint32 i = 0; i < ocb.ISRsSize; i++)
    {
        if (ocb.ICB[i].HardwareVector == isrId)
        {
            pIsrCB = &ocb.ICB[i];
            break;
        }
    }
    return pIsrCB;
}

void Os_Irq_Common_Handler (void)
{
    // 1. 从 GIC 读取中断号 (Acknowledge)
    uint32 irqID = GICC_IAR & 0x3FF;
    // 2. 检查无效中断 (Spurious Interrupt)
    if (irqID >= 1022)
    {
        return;
    }
    // 2. 增加嵌套计数
    ocb.IntNestingCount++;
    // 3. 根据 ID 查找 ISR 配置
    // Os_ISR_Table 是你定义的数组，包含 Category 信息和函数指针
    ISRControlBlock* pIsrCB = GetIsrControlBlockByIsrId (irqID);
    if (pIsrCB != NULL)
    {
        if (pIsrCB->Category == 2)
        {
            // 二类中断：允许调用 OS 服务
            pIsrCB->Handler ();
        }
        else if (pIsrCB->Category == 1)
        {
            // 一类中断：直接执行，不涉及调度逻辑
            pIsrCB->Handler ();
        }
        else
        {
            // 未知类别，忽略或记录错误
        }
    }
    else
    {
        // 如果是定时器中断（且未被映射到 ISR 表）
        // Timer_ISR_Handler();
        Uart_SendString ("isr is error.\n");
    }

    // 4. 确认中断结束 (EOI)
    GICC_EOIR = irqID;

    // 5. 判定是否触发调度
    // 准则：嵌套为 0 且 抢占标志被置位
    ocb.IntNestingCount--;
    if (ocb.IntNestingCount == 0)
    {
        ocb.preempt_flag = 1;  // 通知汇编执行 Os_IntContextSwitch
    }
}
/*
1. 测试用例场景设计
Category 1 测试：使用 Private Timer (ID 29)。每 500ms 触发一次，直接在硬件层翻转一个内存标志位或打印。
Category 2 测试：使用 SP804 (ID 34)。每 10ms 触发一次，调用 ActivateTask(Task_Heartbeat)，由任务来处理逻辑。

*/
int main (void)
{
    uint32 size_of_tcb               = sizeof (OsControlBlock);                     // 1124
    uint32 size_of_sp                = sizeof (uint32*);                            // 4
    uint32 size_of_state             = sizeof (TaskControlBlock);                   // 120
    uint32 offset_of_TaskState       = offsetof (TaskControlBlock, TaskState);      // 8
    uint32 offset_of_Tasks           = offsetof (OsControlBlock, Tasks);            // 4
    uint32 offset_of_StackPtr        = offsetof (TaskControlBlock, StackPtr);       // 88
    uint32 offset_of_IntNestingCount = offsetof (OsControlBlock, IntNestingCount);  // 1116
    uint32 offset_of_preempt_flag    = offsetof (OsControlBlock, preempt_flag);     // 1120
    counters[0].CounterID            = 0;                                           /* SystemCounter */
    counters[0].maxallowedvalue      = 0xFFFF;                                      /* 65535 后归零 */
    counters[0].ticksperbase         = 1;                                           /* 1:1 硬件/软件节拍 */
    counters[0].mincycle             = 10;                                          /* 周期不小于 10 ticks */
    counters[0].CurrentValue         = 0;                                           /* 初始计数值 */
    ocb.CCB                          = counters;
    ocb.CountersSize                 = 1;

    alarms[0].AlarmID       = 0;
    alarms[0].CounterID     = 0;     /* 绑定到上面的 SystemCounter */
    alarms[0].AlarmTime     = 0;     /* 初始为0，由 SetRelAlarm 计算 */
    alarms[0].IsActive      = FALSE; /* 初始不启动 */
    alarms[0].CycleTime     = 0;     /* 初始为0，由 SetRelAlarm 设置 */
    alarms[0].Action        = ALARM_ACTION_ACTIVATE_TASK;
    alarms[0].TargetTaskID  = Task_Low_id; /* 假设 Task_Low 的 ID 是 2 */
    alarms[0].TargetEvent   = 0;           /* 非事件动作，设为 0 */
    alarms[0].AlarmCallback = NULL;        /* 非回调动作，设为 NULL */
    ocb.ACB                 = alarms;
    ocb.AlarmsSize          = 2;
    ocb.preempt_flag        = 0;

    isrs[0].Handler            = SP804_Handler;
    isrs[0].Category           = 2;      // ISR 类型: 1 或 2
    isrs[0].ID                 = 0;      // ISR 标识符
    isrs[0].Priority           = 10;     // ISR 的逻辑优先级（用于资源保护）
    isrs[0].HardwareVector     = 34;     // 硬件中断向量号
    isrs[0].Resources          = NULL;   // 该 ISR 允许使用的资源列表
    isrs[0].NestingLevel       = 0;      // 当前嵌套深度
    isrs[0].LastError          = E_OK;   // 存储当前 ISR 发生的最后一个错误
    isrs[0].OccupiedRes        = 0;      // 当前 ISR 持有的资源链表（用于 ReleaseResource 检查）
    isrs[0].SavedInterruptMask = 0;      // 进入 ISR 前保存的中断优先级屏蔽位（用于嵌套恢复）
    isrs[0].IsActive           = FALSE;  // 该 ISR 当前是否正在运行

    isrs[1].Handler            = Private_Timer_Handler;
    isrs[1].Category           = 1;      // ISR 类型: 1 或 2
    isrs[1].ID                 = 1;      // ISR 标识符
    isrs[1].Priority           = 5;      // ISR 的逻辑优先级（用于资源保护）
    isrs[1].HardwareVector     = 29;     // 硬件中断向量号
    isrs[1].Resources          = NULL;   // 该 ISR 允许使用的资源列表
    isrs[1].NestingLevel       = 0;      // 当前嵌套深度
    isrs[1].LastError          = E_OK;   // 存储当前 ISR 发生的最后一个错误
    isrs[1].OccupiedRes        = 0;      // 当前 ISR 持有的资源链表（用于 ReleaseResource 检查）
    isrs[1].SavedInterruptMask = 0;      // 进入 ISR 前保存的中断优先级屏蔽位（用于嵌套恢复）
    isrs[1].IsActive           = FALSE;  // 该 ISR 当前是否正在运行
    ocb.ICB                    = isrs;
    ocb.ISRsSize               = 2;
    ocb.IntNestingCount        = 0;

    Os_InitializeScheduler ();  // 先洗牌
    current       = 0;
    ocb.TasksSize = 0;
    Os_CreateTask (&Task_Init_id, Task_Init, Task_Init_Stack, STACK_SIZE, 0, TASK_KIND_BASIC_TASK, TRUE);
    Os_CreateTask (&Task_Idle_id, Task_Idle, Task_Idle_Stack, STACK_SIZE, 1, TASK_KIND_BASIC_TASK, TRUE);
    Os_CreateTask (&Task_High_id, Task_High, Task_High_Stack, STACK_SIZE, 30, TASK_KIND_EXTENDED_TASK, TRUE);
    Os_CreateTask (&Task_Low_id, Task_Low, Task_Low_Stack, STACK_SIZE, 10, TASK_KIND_BASIC_TASK, TRUE);
    ocb.Tasks = tasks;
    gic_init ();

    /*
    定时器参数解析表 
    定时器类型          配置数值 (Hex)  配置数值 (Dec)     计算公式 (假设 100MHz)           预估触发时间
    Private Timer       0x100000      1,048,576\        (T=\frac{1,048,576}{10^{8}}\)   ~10.49 ms
    Private Watchdog    0x150000      1,376,256\        (T=\frac{1,376,256}{10^{8}}\)   ~13.76 ms
    Global Timer        0x200000      2,097,152\        (T=\frac{2,097,152}{10^{8}}\)   ~20.97 ms
    SP804 (Dual Timer)  0x250000      2,424,832\        (T=\frac{2,424,832}{10^{8}}\)   ~24.25 ms
    RTC (实时时钟)      5             5                 硬件逻辑直接计数                    5.00 s

    */

    // 初始化所有定时器 (数值根据 QEMU 频率调整)
    // 修改第29行：定时 500ms
    init_private_timer (0x2FAF080);
    // init_private_wdog(0x150000);
    // init_global_timer(0x200000);
    // 系统心跳（System Tick）通常设置为 10ms (100Hz) 或 1ms (1000Hz)。
    //  10ms (推荐，负载均衡)： 适合大多数嵌入式实验。
    //     计算：\(100,000,000\times 0.01=1,000,000\) \(\rightarrow \) 0xF4240
    // 1ms (高精度)： 适合对实时性要求极高的 RTOS。
    //     计算：\(100,000,000\times 0.001=100,000\) \(\rightarrow \) 0x186A0
    // 在 QEMU 模拟的 vexpress-a9 中，SP804 的时钟频率通常默认是 1MHz（即每秒 1,000,000 个 tick）。
    //  要配置 10ms 和 100ms，我们需要根据公式计算 load 值：
    // \(Load=\text{Frequency\ (Hz)}\times \text{Time\ (s)}\)
    // 1. 计算过程 假设时钟频率为 \(1\text{MHz}(1,000,000\text{Hz})\)：
    //  Timer 1 (10ms):\(1,000,000\times 0.010=10,000\text{\ (0x2710)}\)
    // Timer 2 (100ms):\(1,000,000\times 0.100=100,000\text{\ (0x186A0)}\) 
    init_sp804_dual (10000, 100000);
    // init_rtc(5); // 5秒后触发

    // 开启 CPU 总中断
    Os_Arch_EnableInterrupts ();
    AppModeType mode = 0;
    StartOS (mode);

    while (1)
        ;
}