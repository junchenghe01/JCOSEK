#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "Os.h"
#include "Os_Internal.h"
// #define MAX_TASKS 10

// typedef enum {
//     TASK_READY,
//     TASK_RUNNING
// } task_state_t;
// sizeof(tcb_t)        = 8    字节
// sizeof(uint32_t*)    = 4    字节（32位系统指针大小）
// sizeof(task_state_t) = 1    字节（枚举被优化为1字节）
// offsetof(tcb_t, sp)  = 0    字节（sp是第一个字段）
// offsetof(tcb_t, state)= 4   字节（state在sp后面，sp占4字节）
// tcb_t结构体在内存中的布局（共8字节）：
// 偏移量 | 字段      | 大小（字节） | 说明
// ------|-----------|-------------|----------------------
// 0x00  | sp        | 4           | 32位指针，4字节对齐
// 0x04  | state     | 1           | 枚举值，占用1字节
// 0x05  | padding   | 3           | 填充字节，用于4字节对齐
// typedef struct {
//     uint32_t *sp;
//     task_state_t state;
// } tcb_t;

// static tcb_t tasks[MAX_TASKS];
ResourceControlBlock resources[2+1] = {0};
TaskControlBlock tasks[2] = {0};
OsControlBlock ocb = {
    .appMode = 0,
    .Tasks = NULL,
    .TasksSize = 0,
    .pCurrentTask = NULL,
    .Resources = &resources,
    .ResourcesSize = 0
};
static int current = -1;
// static int task_count = 0;
uint32_t *pSize = &(ocb.TasksSize);
extern void context_switch(uint32_t **old_sp, uint32_t *new_sp);



/* ------------------ 任务栈 ------------------ */
#define STACK_SIZE 1024
static uint32_t task1_stack[STACK_SIZE]; // 4KB
static uint32_t task2_stack[STACK_SIZE]; // 4KB
static uint32_t task3_stack[STACK_SIZE]; // 4KB
static uint32_t task4_stack[STACK_SIZE]; // 4KB
static uint32_t task5_stack[STACK_SIZE]; // 4KB

/* ------------------ API ------------------ */
// __attribute__((naked)) void Os_Yield(void)
// {
//     __asm__ volatile(
//         "push {r4-r11, lr}\n"
//         "bl Os_Yield_Internal\n"
//         "pop {r4-r11, lr}\n"
//         "bx lr\n"
//     );
// }

// void Os_Yield_Internal(void)
// {
//     int next = current + 1;
//     if (next >= task_count) next = 0;

//     int prev = current;
//     current = next;

//     tasks[prev].state = TASK_READY;
//     tasks[current].state = TASK_RUNNING;

//     context_switch(&tasks[prev].sp, tasks[current].sp);
// }
__attribute__((naked)) void Os_Yield(void)
{
    __asm__ volatile(
        // 保存当前寄存器
        "push {r4-r11, lr}\n"
        
        // 保存当前栈指针到任务控制块
        // int next = current + 1;
        "ldr r0, =current\n"        // 获取 currentr的地址，0 是 current的地址
        "ldr r0, [r0]\n"           // 获取 currentr的值， r0 是 current的值
        "add r0, r0, #1\n"       // 获取下一个任务， r0 是 current的值 + 1

        // if (next >= task_count) next = 0;
        "mov r1, r0\n"           // 2. 获取next的值， r1 是 next的值， r0 是 next的值
        "ldr r0, =pSize\n"   // 1. 获取指针变量 pSize 的地址
        "ldr r0, [r0]\n"     // 2. 获取 pSize 存的值（即 &ocb.TasksSize）
        "ldr r0, [r0]\n"     // 3. 获取该地址对应的值（即 TasksSize 的具体数值）
        "cmp r1, r0\n"    // 相当于执行: r1(next) - r0(task_count) >= 0
        "blt skip_reset\n"   // @ 如果 r1 < r0 (1 < 2)，跳过赋值逻辑
        "set_to_zero:\n"
        "mov r6, #0\n"             // 如果 > 0，则归零
        "mov r1, r6\n" 
        "skip_reset:\n"
        // int prev = current;
        "ldr r0, =current\n"
        "ldr r0, [r0]\n"           // 3. 获取task_count的值
        "mov r4, r0\n"              // r4 = prev
        // current = next;
        "ldr r0, =current\n"
        "mov r6, r1\n"           // r6 = next
        "str r6, [r0]\n"           // 更新current
        // typedef enum {
        //     TASK_STATE_WAITING = 0,
        //     TASK_STATE_READY = 1,
        //     TASK_STATE_SUSPENDED = 2,
        //     TASK_STATE_RUNNING = 3
        // } TaskStateType;
        // tasks[prev].state = TASK_READY;
        "ldr r1, =tasks\n"          // r1 = tasks基址
        "mov r5, #124\n"            // r5 = 元素大小（8字节）
        "mul r2, r4, r5\n"     // r2 = prev × 8
        "add r0, r1, r2\n"         // r2 = tasks + (prev × 8) = &tasks[prev]
        "add r0, r0, #8\n"         // 5. r0 = &tasks[current].state
        "ldr r0, [r0]\n"           // 6. 读取tasks[prev].state
        "mov r0, #2\n"         // 7. 设置为TASK_READY (TASK_STATE_SUSPENDED=2)
        "str r0, [r0]\n"         // 8. 保存tasks[prev].state

        // tasks[current].state = TASK_RUNNING;
        "ldr r0, =current\n"
        "ldr r0, [r0]\n"           // 1. 获取current的值
        "ldr r1, =tasks\n"          // 3. 获取tasks数组的起始地址
        "mov r5, #124\n"            // r5 = 元素大小（8字节）
        "mul r2, r0, r5\n"     // r2是tasks[current]的地址
        "add r0, r2, r1\n"         // 5. r0 = &tasks[current] + offset_of
        "add r0, r0, #8\n"         // 5. r0 = &tasks[current].state
        "ldr r0, [r0]\n"           // 6. 读取tasks[current].state
        "mov r0, #3\n"         // 7. 设置为TASK_RUNNING (TASK_STATE_RUNNING=3)
        "str r0, [r0]\n"         // 8. 保存tasks[current].state

        // // 禁用中断
        // "cpsid i\n"
        
        // 加载新任务的栈指针
        "ldr r0, =current\n"
        "ldr r0, [r0]\n"           // 1. 获取current的值
        "ldr r1, =tasks\n"          // 3. 获取tasks数组的起始地址
        /* 计算偏移：Offset = index * sizeof(tcb_t) */
        "mov r2, #124\n"            // @ 每项大小为 8 字节
        "mul r2, r0, r2\n"        // @ r2 = current * 8 (字节偏移)
        /* 计算最终成员地址：&tasks[current] */
        "add r3, r1, r2\n"       // @ r3 = tasks基址 + 偏移量
        "add r3, r3, #92\n"      // @ r3 = &tasks[current].StackPtr
        /* 访问具体成员，例如 .sp (假设 sp 在结构体偏移 0 处) */
        "ldr r4, [r3]\n"          // @ 从该 TCB 地址取出存好的 SP 值
        "mov sp, r4\n"            // @ 切换堆栈
        
        // // 启用中断
        // "cpsie i\n"
        
        // 恢复新任务寄存器并跳转
        "pop {r4-r11, lr}\n"
        "bx lr\n"
    );
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
void Os_CreateTask(void (*entry)(void), uint32_t *stack, uint32_t size, uint32_t prio)
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
    tasks[ocb.TasksSize].TaskID = ocb.TasksSize+1;
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

uint32 counter = 0;
/* ------------------ 示例任务 ------------------ */

void Task1(void)
{

    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask();
}

void Task2(void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask();
}
void Task3(void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask();
}
void Task4(void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask();
}
void Task5(void)
{
    /* 1. 进入临界区：提升优先级，禁止低优先级抢占 */
    GetResource(RES_USER_1);

    /* 2. 执行受保护的操作 */
    counter++;
    GetResource(RES_USER_2);
    if (counter == 50)
    {
        
        counter = 0;
    }
    ReleaseResource(RES_USER_2);

    /* 3. 退出临界区：恢复优先级，允许抢占 */
    ReleaseResource(RES_USER_1);

    /* 4. 任务结束 */
    TerminateTask();
}

int main(void)
{
    // uint32_t size_of_tcb = sizeof(OsControlBlock);//136
    // uint32_t size_of_sp = sizeof(uint32_t*);//4
    // uint32_t size_of_state = sizeof(TaskControlBlock);//124
    // uint32_t offset_of_sp = offsetof(TaskControlBlock, TaskState);//8
    // uint32_t offset_of_state = offsetof(OsControlBlock, Tasks);//4
    // uint32_t offset_of_StackPtr = offsetof(TaskControlBlock, StackPtr);// 92

    ocb.Resources = &resources;
    ocb.ResourcesSize = 2;
    ocb.Resources[0].resID = RES_SCHEDULER;// 默认资源
    ocb.Resources[0].ceilingPriority = 3;
    ocb.Resources[0].nestingCount = 0;
    ocb.Resources[0].owner = NULL;
    ocb.Resources[1].resID = RES_USER_1;
    ocb.Resources[1].ceilingPriority = 2;
    ocb.Resources[1].nestingCount = 0;
    ocb.Resources[1].owner = NULL;
    ocb.Resources[2].resID = RES_USER_2;
    ocb.Resources[2].ceilingPriority = 3;
    ocb.Resources[2].nestingCount = 0;
    ocb.Resources[2].owner = NULL;


    Os_InitializeScheduler(); // 先洗牌
    current = 0;
    ocb.TasksSize = 0;
    Os_CreateTask(Task1, task1_stack, STACK_SIZE, 1);
    Os_AddTaskToReadyQueue(&tasks[0]);
    Os_CreateTask(Task2, task2_stack, STACK_SIZE, 1);
    Os_AddTaskToReadyQueue(&tasks[1]);
    Os_CreateTask(Task3, task3_stack, STACK_SIZE, 1);
    Os_AddTaskToReadyQueue(&tasks[2]);
    Os_CreateTask(Task4, task4_stack, STACK_SIZE, 2);
    Os_AddTaskToReadyQueue(&tasks[3]);
    Os_CreateTask(Task5, task5_stack, STACK_SIZE, 3);
    Os_AddTaskToReadyQueue(&tasks[4]);

    ocb.Tasks = tasks;
    AppModeType mode = 0;
    // Os_Start();
    StartOS(mode);
    while (1);
}
