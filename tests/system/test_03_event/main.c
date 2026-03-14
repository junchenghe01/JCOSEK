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
void Uart_Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%' && *(format + 1) == 'x') {
            uint32_t val = va_arg(args, uint32_t);
            Uart_SendHex(val); // 调用之前的十六进制打印
            format += 2;
        } else {
            Uart_SendChar(*format);
            if (*format == '\n') Uart_SendChar('\r');
            format++;
        }
    }
    va_end(args);
}
EventControlBlock events[2+1] = {0};
#define EVENT_0_MASK 0x01
#define EVENT_1_MASK 0x02
ResourceControlBlock resources[2+1] = {0};
TaskControlBlock tasks[3] = {0};
OsControlBlock ocb = {
    .appMode = 0,
    .Tasks = NULL,
    .TasksSize = 0,
    .pCurrentTask = NULL,
    .Resources = &resources,
    .ResourcesSize = 0,
    .ECB = &events,
    .EventsSize = 0
};
static int current = -1;

uint32_t *pSize = &(ocb.TasksSize);
extern void Os_ContextSwitch(TaskControlBlock* old_tcb, TaskControlBlock* new_tcb);

/* Os_Alarm.c (内部函数) */
void Os_Internal_TriggerAlarmAction(AlarmControlBlock* pAlarm) {
    // 根据 OIL 配置中的动作类型进行分发
    switch (pAlarm->Action) {
        
        case ALARM_ACTION_ACTIVATE_TASK:
            /* 场景 A：激活一个任务 */
            // 直接调用你之前 v0.0.2 实现的系统服务
            (void)ActivateTask(pAlarm->TargetTaskID);
            break;

        case ALARM_ACTION_SET_EVENT:
            /* 场景 B：为一个扩展任务设置事件 */
            // 调用你刚刚在 v0.0.3 实现的系统服务
            (void)SetEvent(pAlarm->TargetTaskID, pAlarm->TargetEvent);
            break;

        case ALARM_ACTION_ALARM_CALLBACK:
            /* 场景 C：执行一个回调函数 (Alarm-Callback) */
            // 这种模式通常运行在中断上下文，权限很高
            if (pAlarm->AlarmCallback != NULL) {
                pAlarm->AlarmCallback();
            }
            break;

        default:
            // 异常处理
            break;
    }
}
void Os_Internal_CheckAlarms(CounterType CounterID) {
    for (int i = 0; i < ocb.AlarmsSize; i++) {
        AlarmControlBlock* pAlarm = &ocb.ACB[i];
        CounterControlBlock* pCounter = GetCounterIdByCounterID(CounterID);

        // 只检查属于该 Counter 且处于激活状态的报警器
        if (pAlarm->IsActive && (pAlarm->CounterID == CounterID)) {
            
            // 4. 判断是否到期
            if (pCounter->CurrentValue == pAlarm->AlarmTime) {
                
                // 5. 执行报警动作 (Action)
                Os_Internal_TriggerAlarmAction(pAlarm);

                // 6. 处理周期性报警 (Cycle)
                if (pAlarm->CycleTime > 0) {
                    // 重新计算下一次到期时间 (逻辑同 SetRelAlarm)
                    pAlarm->AlarmTime = (pAlarm->AlarmTime + pAlarm->CycleTime) % (pCounter->maxallowedvalue + 1);
                } else {
                    pAlarm->IsActive = FALSE; // 单次报警执行后关闭
                }
            }
        }
    }
}
/**
 * @brief 增加计数器的值，并检查是否有绑定的报警器到期
 * @param CounterID 计数器索引 (通常系统只有一个 SystemCounter)
 */
void Os_Internal_IncrementCounter(CounterType CounterID) {
    CounterControlBlock* pCounter = GetCounterIdByCounterID(CounterID);
    // const Os_CounterConfigType* pCfg = pCounter->Config;

    // 1. 物理计数值增加
    pCounter->CurrentValue++;

    // 2. 检查回绕 (Rollover)
    if (pCounter->CurrentValue > pCounter->maxallowedvalue) {
        pCounter->CurrentValue = 0;
    }

    // 3. 核心：遍历所有属于该计数器的报警器
    // 这是连接 Counter 和 Alarm 的纽带
    Os_Internal_CheckAlarms(CounterID);
}
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

    /* 3. 配置控制寄存器：
     * Bit 7: Timer Enable (1 = 开启)
     * Bit 6: Timer Mode (1 = Periodic 周期模式)
     * Bit 5: Interrupt Enable (1 = 开启中断)
     * Bit 1: Size (1 = 32-bit counter)
     */
    TIMER0_CONTROL = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1);

    /* 4. 清除可能存在的悬挂中断 */
    TIMER0_INTCLR = 1;
}
void Timer_ClearInterrupt(void) {
    /* 1. 向 INTCLR 寄存器写任意值以清除定时器内部标志 */
    TIMER0_INTCLR = 1; 

    /* 2. (可选) 通知 GIC 中断控制器该中断已处理 (End Of Interrupt) */
    /* 这一步通常在汇编的中断统一入口处完成，如果你的 C 代码里没管 GIC，这里只清硬件位即可 */
}
void Os_Internal_IsrExitSchedule(void) {
    // 1. 只有非嵌套中断才允许在退出时调度
    if (ocb.IntNestingCount == 1) { 
        
        uint16_t highestPrio = GetHighestPriorityIndex();
        
        // 2. 检查是否有更高优先级的任务在排队
        if (highestPrio > ocb.pCurrentTask->CurrentPriority) {
            
            // 3. 标记需要进行“中断后抢占”
            // 注意：这里不能直接调普通的 Schedule()，
            // 因为此时你还在硬件的中断模式（如 IRQ 模式）和中断栈里。
            
            TaskControlBlock* pNextTask = ocb.ReadyQueues[highestPrio];
            
            /* 
             * 触发汇编层面的中断上下文切换
             * 在 ARM Cortex-A9 中，这通常通过修改 LR_irq 或触发一个 PendSV/软件中断实现
             */
            // Os_Arch_TriggerIsrPreemption(); 
            Os_ContextSwitch(ocb.pCurrentTask, pNextTask);
        }
    }
}
// void C_Irq_Handler(void) {
//     Timer_ClearInterrupt();
//     Os_Internal_IncrementCounter(0);
    
//     // 在这里判断是否要调度，但内部会检查 Os_IntNestingCount
//     Os_Internal_IsrExitSchedule(); 

//     // 临退出前，深度 -1
//     ocb.IntNestingCount--; 
// }
/* portable/vexpress_a9/timer.c */
void Timer_ISR_Handler(void) {
    // 1. 清除硬件中断标志位
    Timer_ClearInterrupt();

    // 2. 驱动 OS 计数器
    Os_Internal_IncrementCounter(0); // 0 代表 SystemCounter

    // 3. 在中断退出前触发调度 (如果是抢占式内核)
    Os_Internal_IsrExitSchedule();
    // 临退出前，深度 -1
    ocb.IntNestingCount--; 
}
// 这个用例模拟了一个典型的“生产者-消费者”模型： Task_High（扩展任务）等待信号， Task_Low（基本任务）负责发送信号。
// 1. 预设条件
// Task_High (ID: 0, Prio: 30, Extended): 优先级高，负责处理数据。
// Task_Low (ID: 1, Prio: 10, Basic): 优先级低，负责产生事件。
// EVENT_DATA_READY (0x01): 数据准备就绪事件。
// EVENT_STOP_SYSTEM (0x02): 停止系统事件。
// 3. 预期运行顺序（判定标准）
// 如果你的位图和事件逻辑正确，QEMU 串口输出顺序应如下：
// [Task_High] Started. Calling WaitEvent...
// [Task_Low] Started. I will set EVENT_DATA_READY...
// [Task_High] Woken up! (证明 SetEvent 成功触发了抢占)
// [Task_High] Current Events: 0x1 (证明 GetEvent 拿到了正确位)
// [Task_High] After Clear, Events: 0x0 (证明 ClearEvent 抹掉了位图)
// [Task_High] All Event tests passed. Terminating...
// [Task_Low] Resumed. Terminating...
// 4. 调试重点 (GDB)
// WaitEvent 时：在 Schedule() 处打断点。观察 ocb.ReadyBitmap，此时优先级 30 的位应该是 0，优先级 10 的位是 1。
// SetEvent 时：观察 tasks[1].eventCB->setMask 是否从 0 变成了 1。
// 栈检查：确保扩展任务在 WaitEvent 阻塞时，其上下文（R4-R11）被正确保存在了它自己的私有栈中。
#define STACK_SIZE 1024
static uint32_t Task_Init_Stack[STACK_SIZE]; // 4KB
static uint32_t Task_High_Stack[STACK_SIZE]; // 4KB
static uint32_t Task_Low_Stack[STACK_SIZE]; // 4KB
TaskType Task_Init_id = 0;
TaskType Task_High_id = 1;
TaskType Task_Low_id = 2;
#define EVENT_DATA_READY (0x01U)
#define EVENT_STOP_SYSTEM (0x02U)
void Task_Init(void) {
    Uart_SendString("[Task_Init] System Initialization Complete. Terminating Init Task...\n");
    TerminateTask();
}
// --- 任务1：扩展任务 (消费者) ---
void Task_High(void) {
    EventMaskType currentEvents;
    StatusType status;

    Uart_SendString("[Task_High] Started. Calling WaitEvent(0x01 | 0x02)...\n");

    // 1. 测试 WaitEvent: 此时没有事件，Task_High 应该进入 WAITING，切换到 Task_Low
    status = WaitEvent(EVENT_DATA_READY | EVENT_STOP_SYSTEM);
    // OS_ASSERT(status == E_OK, "WaitEvent Failed");

    // --- 此时 Task_High 应该在这里“复活” ---
    Uart_SendString("[Task_High] Woken up!\n");

    // 2. 测试 GetEvent: 检查是谁唤醒了我
    status = GetEvent(Task_High_id, &currentEvents); // ID 1 是 Task_High
    // OS_ASSERT(status == E_OK, "GetEvent Failed");
    Uart_Printf("[Task_High] Current Events: 0x%x\n", currentEvents);

    if (OS_BIT_CHECK_ANY(currentEvents, EVENT_DATA_READY)) {
        // printf("[Task_High] Handling DATA_READY...\n");
        
        // 3. 测试 ClearEvent: 清除已处理的事件
        ClearEvent(EVENT_DATA_READY);
        
        GetEvent(Task_High_id, &currentEvents);
        Uart_Printf("[Task_High] After Clear, Events: 0x%x\n", currentEvents);
        // OS_ASSERT(!OS_BIT_CHECK_ANY(currentEvents, EVENT_DATA_READY), "ClearEvent Failed");
    }

    Uart_SendString("[Task_High] All Event tests passed. Terminating...\n");
    TerminateTask();
}

// --- 任务2：基本任务 (生产者) ---
void Task_Low(void) {
    Uart_SendString("[Task_Low] Started. I will set EVENT_DATA_READY for Task_High.\n");

    // 4. 测试 SetEvent: 唤醒 Task_High
    // 由于 Task_High 优先级高，调用完 SetEvent 后会立即发生抢占
    StatusType status = SetEvent(Task_High_id, EVENT_DATA_READY); 
    // OS_ASSERT(status == E_OK, "SetEvent Failed");

    // --- 只有等 Task_High 运行完或阻塞，才会回到这里 ---
    Uart_SendString("[Task_Low] Resumed. Terminating...\n");
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
    uint32_t size_of_tcb = sizeof(OsControlBlock);//136
    uint32_t size_of_sp = sizeof(uint32_t*);//4
    uint32_t size_of_state = sizeof(TaskControlBlock);//124
    uint32_t offset_of_sp = offsetof(TaskControlBlock, TaskState);//8
    uint32_t offset_of_state = offsetof(OsControlBlock, Tasks);//4
    uint32_t offset_of_StackPtr = offsetof(TaskControlBlock, StackPtr);// 92
    events->Events = 0;
    ocb.ECB = &events;
    ocb.EventsSize = 2;
    Os_Arch_Timer_Init();
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
    Os_CreateTask(&Task_High_id, 
        Task_High, 
        Task_High_Stack, 
        STACK_SIZE, 
        30,
        TASK_KIND_EXTENDED_TASK,
        TRUE);
    // Os_AddTaskToReadyQueue(&tasks[0]);
    Os_CreateTask(&Task_Low_id, 
        Task_Low, 
        Task_Low_Stack, 
        STACK_SIZE, 
        10,
        TASK_KIND_BASIC_TASK,
        TRUE);
    // Os_AddTaskToReadyQueue(&tasks[1]);
    ocb.Tasks = tasks;

    AppModeType mode = 0;
    StartOS(mode);

    while (1);
}