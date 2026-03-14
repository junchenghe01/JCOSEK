/* =========================================================================
    Unity - A Test Framework for C
    ThrowTheSwitch.org
    Copyright (c) 2007-25 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT

    Dio_ReadChannel Function Unit Tests
    This file tests the Dio_ReadChannel function which reads the level of a DIO channel.
========================================================================= */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Os.h"
#include "TestMockOs.h"
#include "unity.h"

/* 有时候你可能想访问模块中的局部数据。例如：如果你计划通过引用传递，这可能很有用
 * 然而，通常应该避免这样做 */
extern int Counter;

// /* 声明外部模拟变量（在test_mocks.c中定义） */
ResourceControlBlock resources[2 + 1] = {0};
TaskControlBlock     tasks[2]         = {0};
OsControlBlock       ocb              = {
                       .appMode = 0, .Tasks = NULL, .TasksSize = 0, .pCurrentTask = NULL, .Resources = &resources, .ResourcesSize = 0};
static int current = -1;
// static int task_count = 0;
uint32*     pSize = &(ocb.TasksSize);
extern void context_switch (uint32** old_sp, uint32* new_sp);
/* ------------------ 任务栈 ------------------ */
#define STACK_SIZE 1024
static uint32 task1_stack[STACK_SIZE];  // 4KB
static uint32 task2_stack[STACK_SIZE];  // 4KB
static uint32 task3_stack[STACK_SIZE];  // 4KB
static uint32 task4_stack[STACK_SIZE];  // 4KB
static uint32 task5_stack[STACK_SIZE];  // 4KB
void          Os_InitializeScheduler (void)
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
    uint32 bit = 31 - (prio % 32);
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
void Os_CreateTask (TaskType taskId, void (*entry) (void), uint32* stack, uint32 size, uint32 prio)
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
    uint32* sp = stack + size;                     // 栈顶 = 栈基址 + 大小, 得到的是栈顶（高地址），因为ARM使用满递减栈
    sp         = (uint32*)((uintptr_t)sp & ~0x7);  // 8字节对齐

    /* 模拟 context_switch push {r4-r11, lr} */

    *(--sp)                              = (uint32)entry; /* lr - 任务入口地址 */
    *(--sp)                              = 0;             /* r11 */
    *(--sp)                              = 0;             /* r10 */
    *(--sp)                              = 0;             /* r9 */
    *(--sp)                              = 0;             /* r8 */
    *(--sp)                              = 0;             /* r7 */
    *(--sp)                              = 0;             /* r6 */
    *(--sp)                              = 0;             /* r5 */
    *(--sp)                              = 0;             /* r4 */
    tasks[ocb.TasksSize].StackPtr        = sp;
    tasks[ocb.TasksSize].StackSize       = size;
    tasks[ocb.TasksSize].TaskState       = TASK_STATE_SUSPENDED;
    tasks[ocb.TasksSize].StaticPriority  = prio;
    tasks[ocb.TasksSize].CurrentPriority = prio;
    tasks[ocb.TasksSize].TaskEntry       = entry;
    tasks[ocb.TasksSize].TaskID          = taskId;
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

uint32 counter = 0;
/* ------------------ 示例任务 ------------------ */

void Task1 (void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask ();
}

void Task2 (void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask ();
}
void Task3 (void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask ();
}
void Task4 (void)
{
    counter++;
    if (counter == 50)
    {
        counter = 0;
    }
    TerminateTask ();
}
void Task5 (void)
{
    /* 1. 进入临界区：提升优先级，禁止低优先级抢占 */
    GetResource (RES_USER_1);

    /* 2. 执行受保护的操作 */
    counter++;
    GetResource (RES_USER_2);
    if (counter == 50)
    {
        counter = 0;
    }
    ReleaseResource (RES_USER_2);

    /* 3. 退出临界区：恢复优先级，允许抢占 */
    ReleaseResource (RES_USER_1);

    /* 4. 任务结束 */
    TerminateTask ();
}

void setUp (void)
{
    // /* 在每个测试运行前重置模拟状态 */
    // reset_Dio_ReadPort_Mock();
    // reset_Dio_WritePort_Mock();
}

void tearDown (void)
{
    /* 在每个测试运行后执行，用于清理资源 (当前无需特殊清理) */
}

/* ==================== 测试用例分组 1: 基础功能测试（Happy Path） ==================== */
/* 正常输入，验证正确输出：用有效的、典型的参数调用函数，验证返回结果是否符合预期。 */
void test_Os_MainTest (void)
{
    ocb.Resources                    = &resources;
    ocb.ResourcesSize                = 2;
    ocb.Resources[0].resID           = RES_SCHEDULER;  // 默认资源
    ocb.Resources[0].ceilingPriority = 20;
    ocb.Resources[0].nestingCount    = 0;
    ocb.Resources[0].owner           = NULL;
    ocb.Resources[1].resID           = RES_UART;
    ocb.Resources[1].ceilingPriority = 20;
    ocb.Resources[1].nestingCount    = 0;
    ocb.Resources[1].owner           = NULL;

    Os_InitializeScheduler ();  // 先洗牌
    current       = 0;
    ocb.TasksSize = 0;
    Os_CreateTask (Task1, task1_stack, STACK_SIZE, 1);
    Os_AddTaskToReadyQueue (&tasks[0]);
    Os_CreateTask (Task2, task2_stack, STACK_SIZE, 1);
    Os_AddTaskToReadyQueue (&tasks[1]);

    ocb.Tasks        = tasks;
    AppModeType mode = 0;
    // Os_Start();
    StartOS (mode);
    while (1)
        ;
    StartOS (ocb.appMode);
}
