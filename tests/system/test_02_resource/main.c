#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "Os.h"
#include "Os_Internal.h"
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

uint32_t *pSize = &(ocb.TasksSize);
extern void context_switch(uint32_t **old_sp, uint32_t *new_sp);



/* ------------------ 任务栈 ------------------ */
#define STACK_SIZE 1024
static uint32_t task1_stack[STACK_SIZE]; // 4KB
static uint32_t task2_stack[STACK_SIZE]; // 4KB
static uint32_t task3_stack[STACK_SIZE]; // 4KB
static uint32_t task4_stack[STACK_SIZE]; // 4KB
static uint32_t task5_stack[STACK_SIZE]; // 4KB

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
void Os_CreateTask(TaskRefType taskId, void (*entry)(void), uint32_t *stack, uint32_t size, uint32_t prio)
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


// 模拟共享资源
int shared_counter = 0;
TaskType task1_id = 10;
TaskType task2_id = 20;
// --- 任务：低优先级 ---
void Task_Low(void) {
    // printf("[Task_Low] Try to GetResource(RES_UART)...\n");
    
    GetResource(RES_UART); // 获取资源后，Task_Low 的 CurrentPriority 应提升至 20
    // printf("[Task_Low] Resource Locked. CurrentPriority is %d\n", ocb.pCurrentTask->CurrentPriority);

    // 在持锁期间，手动激活一个中优先级任务
    // 按照逻辑，Task_Mid (Prio 20) 不应该能抢占当前任务，因为当前任务也是 Prio 20
    // printf("[Task_Low] Activating Task_Mid (Prio 20)...\n");
    ActivateTask(20); 

    for(volatile int i=0; i<1000000; i++); // 模拟耗时操作，确保不会立即执行完
    
    shared_counter++;
    // printf("[Task_Low] SharedCounter: %d. Releasing Resource...\n", shared_counter);
    
    ReleaseResource(RES_UART); // 释放后，优先级回落到 10，此时 Task_Mid 应该立即抢占
    
    // printf("[Task_Low] Terminating...\n");
    TerminateTask();
}

// --- 任务：中优先级 ---
void Task_Mid(void) {
    // printf("[Task_Mid] I am running! I finally preempted Task_Low.\n");
    shared_counter++;
    // printf("[Task_Mid] SharedCounter: %d. Terminating...\n", shared_counter);
    TerminateTask();
}

int main(void)
{
/*
这个示例模拟了两个任务竞争同一个串口资源（RES_UART），
测试核心是：低优先级任务持锁时，中优先级任务无法抢占它。
1. 预设条件
Task_Low (ID: 10, Prio: 10): 低优先级。
Task_Mid (ID: 20, Prio: 20): 中优先级。
RES_UART (ID: 1): 天花板优先级应设为 20（因为使用它的最高优先级任务是 Task_Mid）。
*/
    ocb.Resources = &resources;
    ocb.ResourcesSize = 2;
    ocb.Resources[0].resID = RES_SCHEDULER;// 默认资源
    ocb.Resources[0].ceilingPriority = 20;
    ocb.Resources[0].nestingCount = 0;
    ocb.Resources[0].owner = NULL;
    ocb.Resources[1].resID = RES_UART;
    ocb.Resources[1].ceilingPriority = 20;
    ocb.Resources[1].nestingCount = 0;
    ocb.Resources[1].owner = NULL;


    Os_InitializeScheduler(); // 先洗牌
    current = 0;
    ocb.TasksSize = 0;
    Os_CreateTask(&task1_id, Task_Low, task1_stack, STACK_SIZE, 10);
    Os_AddTaskToReadyQueue(&tasks[0]);
    Os_CreateTask(&task2_id, Task_Mid, task2_stack, STACK_SIZE, 20);
    Os_AddTaskToReadyQueue(&tasks[1]);

    ocb.Tasks = tasks;
    AppModeType mode = 0;
    // Os_Start();
    StartOS(mode);
    while (1);
}
