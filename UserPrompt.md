# 大模型提示词


qemu模拟的MCU是vexpress-a9 Cortex-A9 
任务控制块如下：
/* 任务控制块（TCB）结构体 */
typedef struct TaskControlBlock {
    /* 任务标识与类型 */
    TaskRefType     TaskID;                 /* 任务唯一标识符 */
    TaskKindType    TaskKind;               /* 基本任务或扩展任务 */
    
    /* 调度相关 */
    PriorityType    StaticPriority;         /* 静态优先级 */
    PriorityType    CurrentPriority;        /* 当前优先级（考虑优先级天花板） */
    uint8           Preemptable;            /* 是否可抢占（0/1） */
    TaskStateType   TaskState;              /* 当前任务状态 */
    
    /* 事件管理（仅扩展任务使用） */
    EventMaskType   Events;                 /* 当前已设置的事件 */
    EventMaskType   WaitingEvents;          /* 任务正在等待的事件 */
    
    /* 资源管理 */
    ResourceMaskType OccupiedResources;     /* 占用的资源位掩码 */
    ResourceMaskType InternalResources;     /* 占用的内部资源位掩码 */
    PriorityType     CeilingPriority;       /* 内部资源的天花板优先级 */
    
    /* 上下文信息 */
    void*           StackPointer;           /* 当前栈指针 */
    void*           ProgramCounter;         /* 程序计数器（可能由硬件保存） */
	uint32          (* TaskEntry)(void);    /* 任务入口函数指针 */
    /* 其他寄存器内容可根据硬件架构扩展 */
    
    /* 激活与队列管理 */
    ActivationCountType ActivationCount;    /* 当前激活次数（用于多次激活） */
    struct TaskControlBlock* NextReady;     /* 就绪队列中的下一个任务 */
    struct TaskControlBlock* PrevReady;     /* 就绪队列中的前一个任务 */
    
    /* 调试与错误处理 */
    uint8           LastErrorStatus;        /* 最后一次系统调用错误状态 */
    
    /* 扩展字段（可选） */
    void*           ExtendedData;           /* 用户自定义扩展数据指针 */
} TaskControlBlock;
帮我实现
context.S：实现上下文切换
portable.h：给调度器提供如下可移植层接口：
SaveTaskContext
RestoreTaskContext
PerformContextSwitch



我的任务控制块
