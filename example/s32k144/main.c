/**
 * @file    main.c
 * @brief   Minimal JCOSEK example for NXP S32K144.
 *
 * @details Creates 3 tasks (Init, High, Low) and runs them with the OSEK
 *          scheduler.  Uses SysTick as the system timer (1 ms tick).
 *
 *          Hardware setup:
 *            - Core clock: 48 MHz (FIRC)
 *            - SysTick:    1 ms period (48,000 cycles)
 *            - No UART or other peripherals (verified via debugger)
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Os.h"
#include "Os_Internal.h"
#include "S32K144.h"

/* ==========================================================================
 * OS Object Storage (configuration)
 * ========================================================================= */
#define NUM_TASKS 4
#define NUM_COUNTERS 1
#define NUM_ALARMS 1
#define NUM_RESOURCES 1
#define NUM_EVENTS 1
#define NUM_ISRS 1
#define STACK_SIZE 1024

static CounterControlBlock  counters[NUM_COUNTERS];
static AlarmControlBlock    alarms[NUM_ALARMS];
static EventControlBlock    events[NUM_EVENTS];
static ResourceControlBlock resources[NUM_RESOURCES];
static ISRControlBlock      isrs[NUM_ISRS];
static TaskControlBlock     tasks[NUM_TASKS];
OsControlBlock              ocb;

/* Task stacks */
static uint32 Task_Init_Stack[STACK_SIZE];
static uint32 Task_Idle_Stack[STACK_SIZE];
static uint32 Task_High_Stack[STACK_SIZE];
static uint32 Task_Low_Stack[STACK_SIZE];

/* Task IDs */
static TaskType Task_Init_id = 0;
static TaskType Task_Idle_id = 1;
static TaskType Task_High_id = 2;
static TaskType Task_Low_id  = 3;

volatile uint32 g_tick_count = 0;

/* ==========================================================================
 * OS_ContextSwitch declaration (from portasm.S)
 * ========================================================================= */
extern void Os_ContextSwitch (TaskControlBlock *old_tcb, TaskControlBlock *new_tcb);

/* ==========================================================================
 * Task functions
 * ========================================================================= */
void Task_Init (void)
{
    /* Initialize completes, terminate self */
    TerminateTask ();
}

void Task_Idle (void)
{
    while (1)
    {
        /* Idle loop — could call WFI here */
        __asm__ volatile ("wfi");
    }
}

void Task_High (void)
{
    /* High priority task — runs and terminates */
    TerminateTask ();
}

void Task_Low (void)
{
    /* Low priority task — runs and terminates */
    TerminateTask ();
}

/* ==========================================================================
 * SysTick Handler — System timer interrupt (1 ms tick).
 *
 * Replaces SP804 used on Cortex-A9.  Increments the OS counter and
 * handles alarm processing.
 * ========================================================================= */
void SysTick_Handler (void)
{
    g_tick_count++;

    /* Drive the OS counter (counter ID 0 = system tick) */
    IncrementCounter (0);
}

/* ==========================================================================
 * OS Initialization helpers
 * ========================================================================= */
void Os_InitializeScheduler (void)
{
    for (int i = 0; i < BITMAP_SIZE; i++)
    {
        ocb.ReadyBitmap[i] = 0;
    }
    for (int i = 0; i < OS_PRIO_LEVELS; i++)
    {
        ocb.ReadyQueues[i] = NULL;
    }
    ocb.pCurrentTask = NULL;
}

void Os_CreateTask (TaskRefType taskId, void (*entry) (void), uint32 *stack, uint32 size, uint32 prio,
                    TaskKindType kind, boolean isAutoStart)
{
    TaskControlBlock *pTcb = &tasks[ocb.TasksSize];

    pTcb->StackPtr         = Os_Cpu_StackInit (entry, stack, size);
    pTcb->StackSize        = size;
    pTcb->TaskState        = TASK_STATE_SUSPENDED;
    pTcb->StaticPriority   = prio;
    pTcb->CurrentPriority  = prio;
    pTcb->TaskEntry        = entry;
    pTcb->TaskID           = taskId;
    pTcb->TaskKind         = kind;
    pTcb->isAutoStart      = isAutoStart;
    pTcb->activation_count = isAutoStart ? 1 : 0;
    pTcb->Preemptable      = 1;
    pTcb->ResourcesCount   = 0;
    pTcb->WaitMask         = 0;
    pTcb->ActivationCount  = 0;
    pTcb->LastErrorStatus  = 0;
    pTcb->ExtendedData     = NULL;

    ocb.TasksSize++;
}

/* ==========================================================================
 * SysTick initialization
 *
 * Configures SysTick to fire every 1 ms:
 *   Reload = (48,000,000 / 1000) - 1 = 47999
 * ========================================================================= */
static void SysTick_Init (void)
{
    /* SysTick clock = core clock = 48 MHz, 1 ms period */
    SysTick->RVR = 47999U;
    SysTick->CVR = 0U;                                                               /* Clear current value */
    SysTick->CSR = SysTick_CSR_ENABLE | SysTick_CSR_TICKINT | SysTick_CSR_CLKSOURCE; /* Use core clock */
}

/* ==========================================================================
 * PendSV priority setup
 *
 * PendSV must have the lowest possible priority (0xFF for 4-bit NVIC).
 * SysTick should have a higher priority (lower number) to preempt.
 * ========================================================================= */
static void NVIC_Init (void)
{
    /* Set PendSV to lowest priority (255 → 0xFF shifted to 8-bit field) */
    SCB->SHPR3 = (SCB->SHPR3 & ~0xFF000000UL) | (0xFFUL << 16);

    /* Set SysTick to mid priority (128 → 0x80 shifted to 8-bit field) */
    SCB->SHPR3 = (SCB->SHPR3 & ~0x00FF0000UL) | (0x80UL << 8);

    /* Set priority grouping: 4 bits preemption priority, 0 bits sub-priority */
    /* (Default after reset is fine for our use) */
}

/* ==========================================================================
 * memcpy / memset — minimal implementations for freestanding environment
 * ========================================================================= */
void *memcpy (void *dest, const void *src, size_t n)
{
    char       *d = (char *)dest;
    const char *s = (const char *)src;
    while (n--)
    {
        *d++ = *s++;
    }
    return dest;
}

void *memset (void *s, int c, size_t n)
{
    char *p = (char *)s;
    while (n--)
    {
        *p++ = (char)c;
    }
    return s;
}

/* ==========================================================================
 * OS Hook functions
 * ========================================================================= */
void StartHooks (void)
{
}
void ErrorHook (StatusType error)
{
    (void)error;
}

/* ==========================================================================
 * main — System entry point.
 * ========================================================================= */
int main (void)
{
    /* Configure SysTick and NVIC priorities */
    SysTick_Init ();
    NVIC_Init ();

    /* Initialize scheduler data structures */
    Os_InitializeScheduler ();

    /* Configure system counter */
    counters[0].CounterID       = 0;
    counters[0].maxallowedvalue = 0xFFFFU;
    counters[0].ticksperbase    = 1;
    counters[0].mincycle        = 1;
    counters[0].CurrentValue    = 0;
    ocb.CCB                     = counters;
    ocb.CountersSize            = NUM_COUNTERS;

    /* Configure alarms */
    alarms[0].AlarmID       = 0;
    alarms[0].CounterID     = 0;
    alarms[0].AlarmTime     = 0;
    alarms[0].IsActive      = FALSE;
    alarms[0].CycleTime     = 0;
    alarms[0].Action        = ALARM_ACTION_ACTIVATE_TASK;
    alarms[0].TargetTaskID  = Task_Low_id;
    alarms[0].TargetEvent   = 0;
    alarms[0].AlarmCallback = NULL;
    ocb.ACB                 = alarms;
    ocb.AlarmsSize          = NUM_ALARMS;

    /* ISR table (empty for now) */
    ocb.ICB             = isrs;
    ocb.ISRsSize        = 0;
    ocb.IntNestingCount = 0;
    ocb.preempt_flag    = 0;

    /* Create tasks */
    ocb.TasksSize = 0;
    Os_CreateTask (&Task_Init_id, Task_Init, Task_Init_Stack, STACK_SIZE, 0, TASK_KIND_BASIC_TASK, TRUE);
    Os_CreateTask (&Task_Idle_id, Task_Idle, Task_Idle_Stack, STACK_SIZE, 1, TASK_KIND_BASIC_TASK, TRUE);
    Os_CreateTask (&Task_High_id, Task_High, Task_High_Stack, STACK_SIZE, 30, TASK_KIND_EXTENDED_TASK, TRUE);
    Os_CreateTask (&Task_Low_id, Task_Low, Task_Low_Stack, STACK_SIZE, 10, TASK_KIND_BASIC_TASK, TRUE);
    ocb.Tasks = tasks;

    /* Enable interrupts and start the OS */
    __asm__ volatile ("cpsie i" : : : "memory");

    AppModeType mode = 0;
    StartOS (mode);

    /* Never reached */
    while (1)
    {
    }
}
