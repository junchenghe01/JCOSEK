// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os.h
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Task Management
 * - Scheduling Policy (Full/Mixed Preemptive Scheduling)
 * - Resource Management (Priority Ceiling Protocol)
 *
 * Conformance Class: ECC2 (Please update as per actual implementation)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org.
 *
 * @author Juncheng HE
 * @date   2026-02-13
 * @copyright Copyright (c) 2026 Juncheng HE. All rights reserved.
 */

/** ============================================================================
 * Includes
 * ========================================================================= */

/** Specification of Operating System R25-11 */

#ifndef OS_H
#define OS_H
#include "Platform_Types.h"
#include "Std_Types.h"
// #include "portable.h"
#include "portmacro.h"
/** ============================================================================
 * Macros & Constants
 * ========================================================================= */

/**
 * @brief  Identifier for an invalid OS-Application.
 * @details Used as a return value when no valid application context exists
 *          or during cross-core error handling.
 */
#define INVALID_OSAPPLICATION (-1)

/** --- OSEK/AUTOSAR Standard Error Codes --- */
/** @brief Access denied: Service called from a prohibited context or insufficient permissions. */
#define E_OS_ACCESS 1

/** @brief Call level error: Service called from an invalid context (e.g., waiting in an ISR). */
#define E_OS_CALLEVEL 2

/** @brief Invalid identifier: The provided ID (Task, Resource, Alarm, etc.) does not exist. */
#define E_OS_ID 3

/** @brief Limit exceeded: Specific limit reached (e.g., multiple activation limit for a task). */
#define E_OS_LIMIT 4

/** @brief Function not available: Current state prohibits the service (e.g., releasing unheld resource). */
#define E_OS_NOFUNC 5

/** @brief Resource exhaustion: Insufficient system resources to complete the request. */
#define E_OS_RESOURCE 6

/** @brief State error: Object is in an improper state for the requested service. */
#define E_OS_STATE 7

/** @brief Value error: Parameter value is out of the allowed range. */
#define E_OS_VALUE 8
/** --- Custom Kernel Error Codes --- */

/** @brief General system error: Used for internal kernel-level assertions or fatal failures. */
#define E_OS_SYS_ERROR 100

/** --- Scheduler Configuration --- */

/** @brief Maximum number of priority levels supported by the OS. */
#define OS_PRIO_LEVELS 256

/**
 * @brief  Size of the priority bitmap in 32-bit words.
 * @details Calculation: 256 levels / 32 bits per word = 8 uint32 elements.
 */
#define BITMAP_SIZE (OS_PRIO_LEVELS / 32)

/**
 * @brief Sets specific bits in a register or variable.
 * @details Sets all bits in @p Reg that are corresponding to the set bits in @p Mask.
 * @param Reg   The target register or variable.
 * @param Mask  The bitmask identifying which bits to set.
 */
#define OS_BIT_SET(Reg, Mask) ((Reg) |= (Mask))

/**
 * @brief Clears specific bits in a register or variable.
 * @details Resets all bits in @p Reg that are corresponding to the set bits in @p Mask.
 * @param Reg   The target register or variable.
 * @param Mask  The bitmask identifying which bits to clear.
 */
#define OS_BIT_CLEAR(Reg, Mask) ((Reg) &= ~(Mask))

/**
 * @brief Checks if any of the specified bits are set.
 * @details Evaluates to true if at least one bit in @p Mask matches a set bit in @p Reg.
 *          Commonly used for **WaitEvent** wake-up logic.
 * @param Reg   The register or variable to check.
 * @param Mask  The bitmask to test against.
 * @return Non-zero if any bit matches; zero otherwise.
 */
#define OS_BIT_CHECK_ANY(Reg, Mask) (((Reg) & (Mask)) != 0U)

/**
 * @brief Checks if all specified bits are set.
 * @details Evaluates to true only if all bits set in @p Mask are also set in @p Reg.
 * @param Reg   The register or variable to check.
 * @param Mask  The bitmask to test against.
 * @return Non-zero if all bits match; zero otherwise.
 */
#define OS_BIT_CHECK_ALL(Reg, Mask) (((Reg) & (Mask)) == (Mask))

/**
 * @brief Extracts specific bits from a register or variable.
 * @details Performs a bitwise AND to isolate the bits defined by @p Mask.
 * @param Reg   The target register or variable.
 * @param Mask  The bitmask used for extraction.
 * @return The value of the isolated bits.
 */
#define OS_BIT_GET(Reg, Mask) ((Reg) & (Mask))

/** ============================================================================
 * Local Types/Structs
 * ========================================================================= */

/**
 * @brief Application Mode identification.
 * Reference: [SWS_Os_91007] in AUTOSAR Classic Platform Specification.
 */
typedef uint8 AppModeType;

/**
 * [SWS_Os_91002] Definition of datatype TotalNumberOfCores
 * @todo: Implement TotalNumberOfCores to support multi-core configurations.
 */
// TODO (HE Juncheng/2026-03-15): TotalNumberOfCores
/**
 * [SWS_Os_00772] Definition of datatype ApplicationType
 * @brief  Identifier for an OS-Application.
 * @details Used to differentiate between different safety/functional partitions
 *          within the same CPU core or across multiple cores.
 */
typedef sint32 ApplicationType;

/**
 * [SWS_Os_00773] Definition of datatype ApplicationStateType
 * @brief  This data type identifies the lifecycle state of an OS-Application.
 */
typedef enum
{
    /** @brief The application is running and its resources are accessible. */
    APPLICATION_ACCESSIBLE,

    /** @brief The application is currently in the process of restarting. */
    APPLICATION_RESTARTING,

    /** @brief The application has been stopped and is no longer executable. */
    APPLICATION_TERMINATED
} ApplicationStateType;

/**
 * [SWS_Os_00774] Definition of datatype ApplicationStateRefType
 * @brief  Points to a memory location where an ApplicationStateType can be stored.
 * @details Typically used in GetApplicationState() to return the state via a pointer.
 */
typedef ApplicationStateType* ApplicationStateRefType;

/**
 * [SWS_Os_00775] Definition of datatype TrustedFunctionIndexType
 * @brief  Uniquely identifies a Trusted Function.
 * @details Trusted functions allow non-trusted applications to execute code
 *          in a privileged mode or with higher access rights.
 */
typedef uint16 TrustedFunctionIndexType;

/**
 * [SWS_Os_00776] Definition of datatype TrustedFunctionParameterRefType
 * @brief  Points to a structure which holds arguments for a trusted function call.
 * @details Uses void* to remain generic, as different trusted functions
 *          may have different argument structures.
 */
typedef void* TrustedFunctionParameterRefType;

/**
 * [SWS_Os_00777] Definition of datatype AccessType
 * @brief  Holds information on how a specific memory region can be accessed.
 * @details Usually a bitmask representing Read/Write/Execute permissions.
 */
typedef uint32 AccessType;

/**
 * [SWS_Os_00778] Definition of datatype ObjectAccessType
 * @brief  Identifies if an OS-Application has permissions to access a specific OS object.
 */
typedef enum
{
    /** @brief Access is permitted. */
    ACCESS,
    /** @brief Access is denied. */
    NO_ACCESS
} ObjectAccessType;

/**
 * [SWS_Os_00779] Definition of datatype ObjectTypeType
 * @brief  This data type uniquely identifies the category of an OS object.
 * @details Used by protection services to check permissions for specific
 *          system resources like tasks, alarms, or resources.
 */
typedef enum
{
    OBJECT_TASK,         /**< OS Task object */
    OBJECT_ISR,          /**< Interrupt Service Routine object */
    OBJECT_ALARM,        /**< Alarm object for time-based events */
    OBJECT_RESOURCE,     /**< Resource object for mutual exclusion */
    OBJECT_COUNTER,      /**< Counter object for time tracking */
    OBJECT_SCHEDULETABLE /**< Schedule Table object for synchronized execution */
} ObjectTypeType;

/**
 * [SWS_Os_00780] Definition of datatype MemoryStartAddressType
 * @brief  A generic pointer capable of pointing to any location in the MCU address space.
 */
typedef void* MemoryStartAddressType;

/**
 * [SWS_Os_00781] Definition of datatype MemorySizeType
 * @brief  Holds the size (in bytes) of a specific memory region.
 */
typedef uint32 MemorySizeType;

/**
 * [SWS_Os_00782] Definition of datatype ISRType
 * @brief  Uniquely identifies an Interrupt Service Routine (ISR).
 */
typedef sint16 ISRType;

/**
 * [SWS_Os_00783] Definition of datatype ScheduleTableType
 * @brief  Uniquely identifies a Schedule Table.
 * @details Schedule tables provide a mechanism for synchronized,
 *          statically defined execution of tasks or events.
 */
typedef uint16 ScheduleTableType;

/**
 * [SWS_Os_00784] Definition of datatype ScheduleTableStatusType
 * @brief  Describes the current state of a Schedule Table.
 * @details This status reflects the lifecycle and synchronization state
 *          of the table, supporting advanced scheduling and cross-core sync.
 */
typedef enum
{
    /** @brief The schedule table is not currently active or started. */
    SCHEDULETABLE_STOPPED,

    /**
     * @brief The schedule table is queued to start.
     * @details This occurs when the table is set via NextScheduleTable()
     *          to start immediately after the current table finishes.
     */
    SCHEDULETABLE_NEXT,

    /**
     * @brief Waiting for a global time synchronization event.
     * @details Used in explicit synchronization mode when the table
     *          has been started but is waiting for the global time source.
     */
    SCHEDULETABLE_WAITING,

    /**
     * @brief The schedule table is executing normally.
     * @details The table is running but is currently not synchronized
     *          to a global time source.
     */
    SCHEDULETABLE_RUNNING,

    /**
     * @brief The schedule table is running and fully synchronized.
     * @details The table is executing and is in sync with a global time source.
     */
    SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS
} ScheduleTableStatusType;

/**
 * [SWS_Os_00785] Definition of datatype ScheduleTableStatusRefType
 * @brief  Points to a variable of the data type ScheduleTableStatusType.
 * @details Used in GetScheduleTableStatus() to return the current status of a schedule table.
 */
typedef ScheduleTableStatusType* ScheduleTableStatusRefType;

/**
 * [SWS_Os_00787] Definition of datatype ProtectionReturnType
 * @brief  Identifies the action the OS should take after returning from the ProtectionHook.
 * @details When a protection violation (e.g., timing or memory) occurs, the ProtectionHook
 *          is called, and its return value determines the recovery strategy.
 */
typedef enum
{
    /** @brief Ignore the error and continue execution (use with extreme caution). */
    PRO_IGNORE,

    /** @brief Terminate the faulty Task or Category 2 ISR. */
    PRO_TERMINATETASKISR,

    /** @brief Terminate the entire OS-Application to which the faulty object belongs. */
    PRO_TERMINATEAPPL,

    /** @brief Shut down the entire OS. */
    PRO_SHUTDOWN,

    /** @brief Prevent the arrival of a task to handle rate-limiting violations. */
    PRO_PREVENT_ARRIVAL_RATE
} ProtectionReturnType;

/**
 * [SWS_Os_00788] Definition of datatype RestartType
 * @brief  Defines whether a Restart Task should be executed after an OS-Application is terminated.
 */
typedef enum
{
    /** @brief The OS-Application will be restarted by its configured Restart Task. */
    OS_OSAPPLICATION_RESTART,

    /** @brief The OS-Application will remain terminated and not restart. */
    OS_OSAPPLICATION_NO_RESTART
} RestartType;

/**
 * [SWS_Os_00789] Definition of datatype PhysicalTimeType
 * @brief  Represents time in physical units (e.g., nanoseconds, microseconds).
 * @details This type is typically used for values returned by conversion macros
 *          such as OS_TICKS2NS_CounterName() or OS_TICKS2US_CounterName().
 */
typedef uint32 PhysicalTimeType;

/**
 * [SWS_Os_00790] Definition of datatype CoreIdType
 * @brief  A scalar type used to identify a specific logical CPU core.
 * @details The ID represents a logical mapping, which may differ from the
 *          physical core index.
 */
typedef enum
{
    /** @brief Refers to the master core; typically responsible for system boot. */
    OS_CORE_ID_MASTER, /** may be an alias for OS_CORE_ID_<x> */
    OS_CORE_ID_0,

    /**
     * [TODO]: Add more cores (OS_CORE_ID_1 to OS_CORE_ID_x).
     * Consider using code generation/templates to determine the final enum list
     * based on the target hardware configuration.
     */
} CoreIdType;

/**
 * [SWS_Os_00791] Definition of datatype SpinlockIdType
 * @brief  Identifies a spinlock instance for multi-core synchronization.
 * @details Spinlocks are used by GetSpinlock, ReleaseSpinlock, and TryToGetSpinlock
 *          to manage shared resources across different cores.
 */
typedef enum
{
    INVALID_SPINLOCK = 0, /** represents an invalid spinlock instance */
    ID_0_SPINLOCK,
    ID_1_SPINLOCK,

    /**
     * [TODO]: Additional spinlock instances (ID_2 to ID_65535).
     * 0x01, 0x02, ...: identifies a specific spinlock instance.
     */
} SpinlockIdType;

/**
 * [SWS_Os_00792] Definition of datatype TryToGetSpinlockType
 * @brief  The TryToGetSpinlockType indicates if the spinlock has been occupied or not.
 */
typedef enum
{
    /** @brief Spinlock was successfully acquired. */
    TRYTOGETSPINLOCK_SUCCESS,
    /** @brief Spinlock was already occupied by another core. */
    TRYTOGETSPINLOCK_NOSUCCESS
} TryToGetSpinlockType;

/**
 * [SWS_Os_91000] Definition of datatype AreaIdType
 * @brief  Identifies a specific peripheral memory or register area.
 * @details Used by peripheral access APIs (Read/Write/ModifyPeripheral)
 *          to ensure controlled access to hardware.
 */
typedef uint32 AreaIdType;

/**
 * [SWS_Os_00786] Definition of datatype CounterType
 * @brief  Uniquely identifies a software or hardware counter.
 */
typedef uint32 CounterType;

/**
 * OSEK/VDX Operating System Specification
 * @brief  Represents the execution state of a task in the OS.
 */
typedef enum
{
    /** @brief Extended task is waiting for at least one event. */
    TASK_STATE_WAITING,

    /** @brief Task is ready to run and waiting for the scheduler to pick it. */
    TASK_STATE_READY,

    /** @brief Task is passive and cannot be executed until activated. */
    TASK_STATE_SUSPENDED,

    /** @brief Task is currently being executed by the CPU. */
    TASK_STATE_RUNNING
} TaskStateType;

/**
 * OSEK/VDX Operating System Specification
 * @brief  TaskStateRefType: points to a variable of TaskStateType
 */
typedef TaskStateType* TaskStateRefType;

/**
 * OSEK/VDX Operating System Specification
 * @brief  This data type identifies the state of a task.
 */
typedef uint8 StatusType;

/**
 * @brief  Enumeration of task models defined in OSEK/VDX.
 * @details Differentiates between tasks that support a waiting state and those that do not.
 */
typedef enum
{
    /**
     * @brief Basic Task model.
     * @details Only has states: RUNNING, READY, and SUSPENDED.
     *          Does not support Event mechanism or WAITING state.
     */
    TASK_KIND_BASIC_TASK = 0,

    /**
     * @brief Extended Task model.
     * @details Supports all states including WAITING.
     *          Able to wait for events during execution.
     */
    TASK_KIND_EXTENDED_TASK,
} TaskTypeEnum;

/**
 * OSEK/VDX Operating System Specification
 * @brief  This data type identifies a task.
 */
typedef uint16 TaskType;

/**
 * OSEK/VDX Operating System Specification
 * @brief  This data type points to a variable of TaskType.
 */
typedef TaskType* TaskRefType;

/**
 * [SWS_Os_91003] Definition of datatype PriorityType
 * @brief  This data type identifies the priority of a task.
 * @details Represented as a uint8, typically supporting 256 priority levels.
 *          In OSEK, a higher numerical value represents a higher priority.
 */
typedef uint8 PriorityType;

/**
 * OSEK/VDX Operating System Specification
 * @brief  Data type for a resource.
 */
typedef enum
{
    RES_NONE = 0,
    RES_SCHEDULER,
    RES_SHARED_MEMORY,
    RES_UART,
    RES_ECUM,
    /** ... Other user-defined resources ... */
    RES_USER_1,
    RES_USER_2,
    /** @brief Total count of resources (used for array sizing and validation). */
    RES_MAX
} ResourceType;

/**
 * @brief  Resource Control Block (RCB) structure.
 * @details This structure maintains the runtime state of a system resource.
 *          It is essential for the Priority Ceiling Protocol (PCP), managing
 *          priority promotion, task ownership, and nested access.
 */
typedef struct
{
    ResourceType resID;            /** @brief Unique identifier of the resource. */
    PriorityType ceilingPriority;  /** @brief Statically determined ceiling priority. */
    PriorityType originalPriority; /** @brief The task's priority before acquiring this resource. */
    TaskRefType  owner;            /** @brief Identifier of the task currently holding the resource (NULL if free). */
    uint8        nestingCount;     /** @brief Supports nested acquisition of the same resource. */

    /**
     * [OPTIONAL]: Pointer to a waiting list (linked list or priority queue)
     * to hold tasks blocked by this resource.
     */
    // struct WaitingTask* waitingList;
} ResourceControlBlock;

// extern ResourceControlBlock osResources[];  // Global array, generated by OIL
// extern const uint8 osNumResources;          // Total resources

/**
 * @brief  Defines the category of a task.
 * @details Used to distinguish between Basic Tasks and Extended Tasks.
 *          - Basic Tasks: Suitable for simple, non-blocking logic.
 *          - Extended Tasks: Support the event mechanism and 'Waiting' state.
 */
typedef uint8 TaskKindType; /** Task type: Basic or Extended */

/**
 * EventMaskType
 * @brief  Data type of the event mask. */
typedef uint32 EventMaskType;

/**
 * EventMaskRefType
 * @brief  Reference to an event mask. */
typedef EventMaskType* EventMaskRefType;

/**
 * @brief  Event Control Block (ECB) structure.
 * @details This structure manages the runtime event status for an Extended Task.
 *          It stores the set of events that have been signaled to the task but
 *          not yet cleared.
 */
typedef struct
{
    /**
     * @brief The current bitmask of set events.
     * @details Each bit represents a specific event (e.g., 0x01, 0x02, 0x04).
     *          The bitwise OR operation is used to set new events, and
     *          AND-NOT is used to clear them.
     */
    EventMaskType Events; /** The bit corresponding to this event (e.g., 0x01, 0x02) */
} EventControlBlock;

/**
 * @brief  ResourceMaskType stores the bitmask representing resources held by a task.
 * @details This type is used to track resource ownership or access rights.
 *          Assuming resources are represented using bitmasks for efficient
 *          lookup and nesting management.
 */
typedef uint8 ResourceMaskType; /** Assume resources are represented using bitmasks. */

/**
 * TickType
 * @brief  This data type represents count values in ticks.
 */
typedef uint32 TickType;

/**
 * TickRefType
 * @brief  This data type points to the data type TickType.
 */
typedef TickType* TickRefType;

/**
 * @brief  Counter Control Block (CCB) structure.
 * @details This structure defines the properties and current state of a counter.
 *          It acts as the time source for alarms and schedule tables.
 */
typedef struct
{
    /** @brief Unique identifier for the counter, statically assigned. */
    CounterType CounterID; /** Counter identifier */

    /**
     * @brief Maximum possible value for this counter.
     * @details When the counter reaches this value, it rolls over to zero on the next tick.
     */
    TickType maxallowedvalue; /** Counter upper limit (e.g., 0xFFFFFFFF) */

    /**
     * @brief Hardware dependency factor.
     * @details Specifies how many hardware ticks (or software increments)
     *          are required to increment the counter by one unit.
     */
    TickType ticksperbase; /** How many hardware ticks equal 1 Counter Tick? */

    /**
     * @brief Minimum cycle constraint for periodic alarms.
     * @details Prevents alarms from being set with an excessively high frequency
     *          that could starve the system.
     */
    TickType mincycle; /** Minimum allowable value for periodic alarms */

    /** @brief The current tick count of the counter. */
    TickType CurrentValue; /**Current tick count */
} CounterControlBlock;

/**
 * @brief  Structure for storage of counter characteristics.
 * @details [SWS_Os_00793] This data type represents a structure for storage of
 *          counter characteristics. It provides the essential physical constraints
 *          of the counter bound to an alarm.
 *
 * @note   All elements of the structure are of data type TickType.
 */
typedef struct
{
    /** @brief Maximum possible allowed count value in ticks.
     *  The counter rolls over to zero after reaching this value. */
    TickType maxallowedvalue; /** Maximum allowed count (tick) */

    /** @brief Number of ticks required to reach a counter-specific (significant) unit.
     *  Used to translate between hardware ticks and time units. */
    TickType ticksperbase; /** Number of ticks corresponding to each basic time unit */

    /** @brief Smallest allowed value for the cycle-parameter of SetRelAlarm/SetAbsAlarm.
     *  Only relevant for systems with extended status to prevent high-frequency interrupts. */
    TickType mincycle; /** Minimum cycle (tick) for periodic alarms */
} AlarmBaseType;

/**
 * AlarmBaseRefType
 * @brief  This data type points to the data type AlarmBaseType.
 */
typedef AlarmBaseType* AlarmBaseRefType;

/**
 * AlarmType
 * @brief  This data type represents an alarm object.
 */
typedef uint16 AlarmType;

/**
 * @brief  Defines the action to be taken when an alarm expires.
 * @details According to OSEK/VDX, an alarm can trigger various system events
 *          such as activating a task, setting an event, or calling a callback.
 */
typedef enum
{
    /** @brief The alarm activates a predefined task upon expiration. */
    ALARM_ACTION_ACTIVATE_TASK,

    /** @brief The alarm sets a specific event bit for an extended task. */
    ALARM_ACTION_SET_EVENT,

    /** @brief The alarm executes a user-defined callback function (AlarmCallback). */
    ALARM_ACTION_ALARM_CALLBACK
} ActionType;

/**
 * @brief  Alarm Control Block (ACB) structure.
 * @details This structure maintains the runtime and configuration state of an alarm.
 *          It defines which counter the alarm is bound to, when it expires,
 *          and what action (Task/Event/Callback) to perform upon expiration.
 */
typedef struct
{
    /** @brief Unique identifier for the alarm. */
    AlarmType AlarmID;

    /** @brief Identifier of the counter to which this alarm is bound. */
    CounterType CounterID;

    /** @brief The expiration time or remaining ticks until the alarm triggers. */
    TickType AlarmTime;

    /** @brief Status flag: TRUE if the alarm is currently running, FALSE otherwise. */
    uint8 IsActive;

    /** @brief The cycle period for recurring alarms. If 0, the alarm is single-shot. */
    TickType CycleTime;

    /** @brief Specifies the operation to perform (e.g., Activate Task) when the alarm expires. */
    ActionType Action;

    /** @brief The TaskID to be activated or receive an event (relevant for Task/Event actions). */
    TaskType TargetTaskID;

    /** @brief The event mask to be set (relevant only for SetEvent actions). */
    EventMaskType TargetEvent;

    /** @brief Function pointer to be executed (relevant only for Callback actions). */
    void (*AlarmCallback) (void);
} AlarmControlBlock;

/**
 * @brief  Interrupt Service Routine (ISR) Control Block.
 * @details This structure manages both the static configuration and dynamic
 *          runtime state of an ISR. It supports OSEK Category 1 and 2 models,
 *          handling nesting, resource acquisition, and hardware vector mapping.
 */
typedef struct
{
    /** --- Static Configuration Members --- */

    /** @brief Pointer to the user-defined ISR handler function. */
    void (*Handler) (void);

    /** @brief ISR Category: 1 (Hardware only) or 2 (OS-managed). */
    uint8 Category;

    /** @brief Unique identifier for the ISR. */
    uint32 ID;

    /** @brief Logic priority of the ISR, used for Priority Ceiling Protocol (PCP). */
    PriorityType Priority;

    /** @brief The hardware interrupt vector number (e.g., GIC ID or IRQ number). */
    uint32 HardwareVector;

    /** @brief List of resources accessible by this ISR. */
    ResourceType* Resources;
    // /** [TODO]: Add core binding, stack start address, etc. */
    // } ISRConfig; // Responsible for static configuration

    /** --- Dynamic Management Members --- */
    // typedef struct {
    /** @brief Current nesting depth of this specific ISR. */
    uint8 NestingLevel;

    /** @brief Stores the last error status that occurred within this ISR. */
    StatusType LastError;

    /** @brief Linked list or bitmask of resources currently held by the ISR. */
    ResourceType OccupiedRes;

    /** @brief Original interrupt mask/priority saved upon entering the ISR. */
    uint32 SavedInterruptMask;

    /** @brief Boolean flag indicating if the ISR is currently executing. */
    boolean IsActive;

    /**
     * Priority Mapping Table:
     * The OS needs to know the mapping between logical ISR IDs and hardware
     * priorities. This is used by services like SuspendOSInterrupts to
     * correctly set the processor's Priority Mask.
     */
} ISRControlBlock;

/**
 * @brief  Enumeration of CPU execution contexts.
 * @details This type identifies the current privilege and environment level.
 *          It is used by the kernel to validate API calls, as certain OSEK
 *          services are restricted to specific contexts.
 */
typedef enum
{
    /** @brief Execution within a basic or extended task. */
    CONTEXT_TYPE_TASK = 0,

    /**
     * @brief Execution within a Category 1 Interrupt Service Routine.
     * @note Cat1 ISRs are not managed by the OS and have minimal API access.
     */
    CONTEXT_TYPE_ISR_1,

    /**
     * @brief Execution within a Category 2 Interrupt Service Routine.
     * @note Cat2 ISRs are managed by the OS and can invoke specific system services.
     */
    CONTEXT_TYPE_ISR_2,
} ContextType;

/**
 * @brief  Type definition for task activation tracking.
 * @details Supports multiple activations of the same task (up to 255).
 */
typedef uint8 ActivationCountType;

/**
 * @brief  Task Control Block (TCB) Structure.
 * @details This is the central data structure for task management,
 *          containing all information required for scheduling, context
 *          switching, and resource/event management.
 */
typedef struct TaskControlBlock
{
    /** --- Task Identification and Type --- */
    /** @brief Reference to the unique task identifier. */
    TaskRefType TaskID;
    /** @brief Distinguishes between Basic and Extended task models. */
    TaskKindType TaskKind;

    /** --- Scheduling Information --- */
    /** @brief Fixed priority assigned during configuration. */
    PriorityType StaticPriority;
    /** @brief Dynamic priority reflecting current status (e.g., Priority Ceiling). */
    PriorityType CurrentPriority;
    /** @brief Configuration flag: 1 if the task can be preempted, 0 otherwise. */
    uint8 Preemptable;

    /**
     * [TODO]: Optimization: Priority Bitmap for O(1) scheduling.
     * uint32 readyPriorityBitmap;  // bit n set means priority n has ready tasks
     */

    /** @brief Current execution state (Ready, Running, Waiting, Suspended). */
    TaskStateType TaskState;

    /** --- Event Management (Extended Tasks Only) --- */
    // EventMaskType   Events;              /** Events currently set for the task */
    /** @brief Mask of events the task is currently blocked on. */
    EventMaskType WaitMask; /** 任务正在等待的事件 */

    /** --- Resource Management --- */
    /**
     * [TODO]: Deletion might affect assembly context switching.
     * ResourceMaskType OccupiedResources;  // Mask of occupied resources
     * ResourceMaskType InternalResources;  // Mask of internal resources
     * PriorityType     CeilingPriority;    // Internal resource ceiling priority
     */
    /** @brief Number of resources currently held by the task. */
    uint32 ResourcesCount;

    /** @brief Hardware-specific context (registers, PC, etc.). */
    ContextInfo Context;

    /** --- Stack and Entry Management --- */
    /** @brief Current top of stack or stack base pointer. */
    void* StackPtr;
    /** @brief Total stack space allocated to this task in bytes. */
    uint32 StackSize;
    /** @brief Pointer to the C function that implements the task. */
    void (*TaskEntry) (void);

    /** --- Activation and Queue Management --- */
    /** @brief Current number of pending activations (for multiple activation). */
    ActivationCountType ActivationCount;
    /** @brief Link to the next task in the circular ready list. */
    struct TaskControlBlock* NextReady;
    /** @brief Link to the previous task in the circular ready list. */
    struct TaskControlBlock* PrevReady;

    /** --- Debugging and Error Handling --- */
    /** @brief Stores the result of the last failed system service call. */
    uint8 LastErrorStatus;
    /** @brief Flag indicating if the task starts automatically at system boot. */
    boolean isAutoStart;

    /** --- Extension Fields (Optional) --- */
    /** @brief Pointer to user-defined metadata or extended attributes. */
    void* ExtendedData;
} TaskControlBlock;

/**
 * @brief  Global OS Control Block (OCB).
 * @details This is the central data structure of the kernel. It acts as the
 *          "Master Map" of the OS, maintaining references to all system objects,
 *          scheduling queues, and runtime execution contexts.
 */
typedef struct
{
    /** @brief Currently active application mode (e.g., OSDEFAULTAPPMODE). */
    AppModeType appMode;

    /** @brief Pointer to the array of Task Control Blocks (TCBs). */
    TaskControlBlock* Tasks;
    /** @brief Total number of tasks configured in the system. */
    uint32 TasksSize;
    /** @brief Reference to the TCB of the currently running task. */
    TaskControlBlock* pCurrentTask;

    /** --- Scheduler & Ready System --- */

    /**
     * @brief Primary Priority Bitmap.
     * @details Each bit represents whether a priority level has ready tasks.
     *          Used for O(1) or O(log N) scheduling lookup.
     */
    uint32 ReadyBitmap[BITMAP_SIZE];

    /**
     * @brief Secondary Group Bitmap (Optional).
     * @details Optimization: Each bit represents a group of 32 priorities.
     *          Further accelerates the search for the highest priority.
     */
    uint8 GroupBitmap;

    /**
     * @brief Array of head pointers for the priority ready queues.
     * @details Each element points to the current active task for that priority level.
     */
    TaskControlBlock* ReadyQueues[OS_PRIO_LEVELS];

    /** --- Resource Management --- */

    /** @brief Pointer to the array of Resource Control Blocks (RCBs). */
    ResourceControlBlock* Resources;
    /** @brief Total number of resources defined in the system. */
    uint32 ResourcesSize;

    /** --- Event Control --- */

    /** @brief Pointer to the Event Control Block(s). */
    EventControlBlock* ECB;
    /** @brief Total number of events configured. */
    uint32 EventsSize;

    /** --- Counter & Alarm Management --- */

    /** @brief Pointer to the array of Counter Control Blocks (CCBs). */
    CounterControlBlock* CCB;
    /** @brief Total number of counters in the system. */
    uint32 CountersSize;

    /** @brief Pointer to the array of Alarm Control Blocks (ACBs). */
    AlarmControlBlock* ACB;
    /** @brief Total number of alarms in the system. */
    uint32 AlarmsSize;

    /** --- Interrupt Management --- */

    /** @brief Pointer to the array of ISR Control Blocks (ICBs). */
    ISRControlBlock* ICB;
    /** @brief Total number of ISRs configured. */
    uint32 ISRsSize;
    /** @brief Global interrupt nesting depth counter. */
    uint32 IntNestingCount;
    /** @brief Flag set by ISR to trigger a reschedule upon interrupt exit. */
    boolean preempt_flag;

    /** --- Execution Context --- */

    /**
     * @brief Current Execution Context Identifier.
     * @details Marks whether the CPU is in TASK, ISR1, or ISR2 context.
     *          Crucial for API service protection and validity checks.
     */
    ContextType CurrentContext;
    // Add other fields as needed
} OsControlBlock;

/** ============================================================================
 * Static Global Variables
 * ========================================================================= */

/** ============================================================================
 * Static Function Prototypes
 * ========================================================================= */

extern OsControlBlock ocb;
extern void           StartHooks (void);  // TODO (HE Juncheng/2026-03-15): 是否可以移除 | NULL
extern void           Os_Yield ();        // TODO (HE Juncheng/2026-03-15): 是否可以移除 | NULL
extern void           Os_ContextSwitch (TaskControlBlock* old_tcb,
                                        TaskControlBlock* new_tcb);  // TODO (HE Juncheng/2026-03-15): 是否可以移除 | NULL
void Task_RemoveFromReady (TaskControlBlock* pTcb);  // TODO (HE Juncheng/2026-03-15): 是否可以移除 | NULL
// void     Os_Internal_EnterIdleMode (void);
uint16 GetHighestPriorityIndex (void);  // TODO (HE Juncheng/2026-03-15): 是否可以移除 | NULL

/** ============================================================================
 * Public API Implementation
 * ========================================================================= */

/**
 * @brief  Returns the identifier of the currently active OS-Application.
 *
 * @details This service determines the OS-Application (a unique identifier has to be allocated to
 * each application) where the caller originally belongs to (was configured to).
 *
 * @return ApplicationType <identifier of running OS-Application> or INVALID_OSAPPLICATION
 */
ApplicationType GetApplicationID (void);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  User-defined hook called after OS initialization but before the scheduler starts.
 *
 * @details This hook routine is called by the operating system at the end of the operating system
 * initialisation and before the scheduler is running. At this time the application can initialise
 * device drivers etc.
 *
 * @particulars
 * See chapter 11.1 for general description of hook routines.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
__attribute__ ((weak)) void StartupHook (void);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  Starts the OSEK Operating System in a specific application mode.
 *
 * @details The user can call this system service to start the operating system in a specific mode,
 * see chapter 5, Application modes.
 *
 * @param[in] Mode application mode
 *
 * @return None
 *
 * @particulars
 * Only allowed outside of the operating system, therefore implementation specific restrictions
 * may apply. See also chapter 11.3, System start-up, especially with respect to systems where OSEK and
 * OSEKtime coexist. This call does not need to return.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void StartOS (AppModeType Mode);

/**
 * @brief  User-defined hook called during the system shutdown sequence.
 *
 * @details This hook routine is called by the operating system when the OS service ShutdownOS has been
 * called. This routine is called during the operating system shut down.
 *
 * @param[in] Error error occurred
 *
 * @particulars
 * ShutdownHook is a hook routine for user defined shutdown functionality, see chapter 11.4.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
__attribute__ ((weak)) void ShutdownHook (
    StatusType Error);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  Aborts the overall system execution.
 *
 * @details The user can call this system service to abort the overall system (e.g. emergency off).
 * The operating system also calls this function internally, if it has reached an undefined internal
 * state and is no longer ready to run.
 * If a ShutdownHook is configured the hook routine ShutdownHook is always called (with <Error> as argument)
 * before shutting down the operating system.
 * If ShutdownHook returns, further behaviour of ShutdownOS is implementation specific.
 * In case of a system where OSEK OS and OSEKtime OS coexist, ShutdownHook has to return.
 * <Error> needs to be a valid error code supported by OSEK OS. In case of a system where OSEK OS and OSEKtime
 * OS coexist, <Error> might also be a value accepted by OSEKtime OS. In this case, if enabled by an OSEKtime
 * configuration parameter, OSEKtime OS will be shut down after OSEK OS shutdown.
 *
 * @param[in] Error error occurred
 *
 * @particulars
 * After this service the operating system is shut down.
 * Allowed at task level, ISR level, in ErrorHook and StartupHook, and also called internally by the operating
 * system.
 * If the operating system calls ShutdownOS it never uses E_OK as the passed parameter value.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void ShutdownOS (StatusType Error);

/**
 * @brief  Activates a specific task to the ready state.
 *
 * @details The task <TaskID> is transferred from the suspended state into the ready state[14].
 * The operating system ensures that the task code is being executed from the first statement.
 * @param[in] TaskID Task reference
 *
 * @return StatusType
 * @retval E_OK        No error.
 * @retval E_OS_LIMIT  Too many task activations of @p TaskID.
 * @retval E_OS_ID     Task @p TaskID is invalid (Extended Status only).
 *
 * @particulars
 * The service may be called from interrupt level and from task level (see Figure 12-1).
 * Rescheduling after the call to ActivateTask depends on the place it is called from (ISR, non
 * preemptable task, preemptable task).
 * If E_OS_LIMIT is returned the activation is ignored.
 * When an extended task is transferred from suspended state into ready state all its events are cleared.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 *
 * @note 14 ActivateTask will not immediately change the state of the task in case of multiple
 * activation requests. If the task is not suspended, the activation will only be recorded and
 * performed later.
 */
StatusType ActivateTask (TaskType TaskID);

/**
 * @brief  Terminates the calling task.
 *
 * @details This service causes the termination of the calling task. The calling task is transferred
 * from the running state into the suspended state[15].
 *
 * @param[in]  none
 *
 * @return StatusType
 * @par Standard Status:
 * No return to call level if successful.
 * @par Extended Status:
 * @retval E_OS_RESOURCE  Task still occupies resources.
 * @retval E_OS_CALLEVEL  Call at interrupt level.
 *
 * @particulars
 * An internal resource assigned to the calling task is automatically released. Other resources occupied
 * by the task shall have been released before the call to TerminateTask. If a resource is still occupied
 * in standard status the behaviour is undefined.
 * If the call was successful, TerminateTask does not return to the call level and the status can not be
 * evaluated.
 * If the version with extended status is used, the service returns in case of error, and provides a status
 * which can be evaluated in the application.
 * If the service TerminateTask is called successfully, it enforces a rescheduling.
 * Ending a task function without call to TerminateTask or ChainTask is strictly forbidden and may leave
 * the system in an undefined state.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType TerminateTask (void);

/**
 * @brief  Yields the processor to higher-priority tasks.
 *
 * @details If a higher-priority task is ready, the internal resource of the task is released, the current
 * task is put into the ready state, its context is saved and the higher-priority task is executed. Otherwise
 * the calling task is continued.
 *
 * @param[in]  none
 *
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_CALLEVEL  Call at interrupt level.
 * @retval E_OS_RESOURCE  Calling task still occupies explicit resources.
 *
 * @particulars
 * Rescheduling only takes place if the task an internal resource is assigned to the calling task[16] during system
 * generation. For these tasks, Schedule enables a processor assignment to other tasks with lower or equal priority
 * than the ceiling priority of the internal resource and higher priority than the priority of the calling task in
 * application-specific locations. When returning from Schedule, the internal resource has been taken again.
 * This service has no influence on tasks with no internal resource assigned (preemptable tasks).
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 *
 * @note 16 Non-preemptable tasks are seen as tasks with an internal resource of highest task priority assigned
 */
StatusType Schedule (void);

/**
 * @brief  Returns the identifier of the task which is currently running.
 *
 * @details GetTaskID returns the information about the TaskID of the task which is currently running.
 *
 * @param[out] TaskID Reference to the task which is currently running
 *
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OK        No error.
 *
 * @particulars
 * Allowed on task level, ISR level and in several hook routines (see Figure 12-1).
 * This service is intended to be used by library functions and hook routines.
 * If <TaskID> can’t be evaluated (no task currently running), the service returns INVALID_TASK as TaskType.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType GetTaskID (TaskRefType TaskID);

/**
 * @brief  Returns the state of a specific task.
 *
 * @details Returns the state of a task (running, ready, waiting, suspended) at the time of calling GetTaskState.
 *
 * @param[in] TaskID Task reference
 * @param[out] State Reference to the state of the task <TaskID>
 *
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Task @p TaskID is invalid.
 *
 * @particulars
 * The service may be called from interrupt service routines, task level, and some hook routines (see Figure 12-1).
 * When a call is made from a task in a full preemptive system, the result may already be incorrect at the time of
 * evaluation.
 * When the service is called for a task, which is activated more than once, the state is set to running if any instance
 * of the task is running.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType GetTaskState (TaskType TaskID, TaskStateRefType State);

/**
 * @brief  Restores the state saved by DisableAllInterrupts.
 * @details This service restores the state saved by DisableAllInterrupts.
 *
 * @param[in]  none
 * @param[out]  none
 * @return None
 *
 * @particulars
 * The service may be called from an ISR category 1 and category 2 and from the task level, but not from hook routines.
 * This service is a counterpart of DisableAllInterrupts service, which has to be called before, and its aim is the
 * completion of the critical section of code. No API service calls are allowed within this critical section.
 * The implementation should adapt this service to the target hardware providing a minimum overhead. Usually, this
 * service enables recognition of interrupts by the central processing unit.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void EnableAllInterrupts (void);

/**
 * @brief  Disables all hardware interrupts and saves the previous state.
 * @details This service disables all interrupts for which the hardware supports disabling. The state before is saved
 * for the EnableAllInterrupts call.
 *
 * @param[in]  none
 * @param[out]  none
 *
 * @particulars
 * The service may be called from an ISR category 1 and category 2 and from the task level, but not from hook routines.
 * This service is intended to start a critical section of the code.
 * This section shall be finished by calling the EnableAllInterrupts service. No API service calls are allowed within
 * this critical section. The implementation should adapt this service to the target hardware providing a minimum
 * overhead. Usually, this service disables recognition of interrupts by the central processing unit. Note that this
 * service does not support nesting. If nesting is needed for critical sections e.g. for libraries
 * SuspendOSInterrupts/ResumeOSInterrupts or SuspendAllInterrupt/ResumeAllInterrupts should be used.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void DisableAllInterrupts (void);

/**
 * @brief  Restores the interrupt recognition status saved by SuspendAllInterrupts.
 * @details This service restores the recognition status of all interrupts saved by the SuspendAllInterrupts service.
 *
 * @param[in]  none
 * @param[out]  none
 * @return None
 *
 * @particulars
 * The service may be called from an ISR category 1 and category 2, from alarm-callbacks and from the task level, but
 * not from all hook routines. This service is the counterpart of SuspendAllInterrupts service, which has to have been
 * called before, and its aim is the completion of the critical section of code. No API service calls beside
 * SuspendAllInterrupts/ResumeAllInterrupts pairs and SuspendOSInterrupts/ResumeOSInterrupts pairs are allowed within
 * this critical section. The implementation should adapt this service to the target hardware providing a minimum
 * overhead. SuspendAllInterrupts/ResumeAllInterrupts can be nested. In case of nesting pairs of the calls
 * SuspendAllInterrupts and ResumeAllInterrupts the interrupt recognition status saved by the first call of
 * SuspendAllInterrupts is restored by the last call of the ResumeAllInterrupts service.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void ResumeAllInterrupts (void);

/**
 * @brief  Saves the interrupt recognition status and disables all hardware interrupts.
 * @details This service saves the recognition status of all interrupts and disables all interrupts for which the
 * hardware supports disabling.
 *
 * @param[in]  none
 * @param[out]  none
 * @return None
 *
 * @particulars
 * The service may be called from an ISR category 1 and category 2, from alarm-callbacks and from the task level, but
 * not from all hook routines. This service is intended to protect a critical section of code from interruptions of any
 * kind. This section shall be finished by calling the ResumeAllInterrupts service. No API service calls beside
 * SuspendAllInterrupts/ResumeAllInterrupts pairs and SuspendOSInterrupts/ResumeOSInterrupts pairs are allowed within
 * this critical section. The implementation should adapt this service to the target hardware providing a minimum
 * overhead.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void SuspendAllInterrupts (void);

/**
 * @brief  Restores the interrupt recognition status saved by SuspendOSInterrupts.
 * @details This service restores the recognition status of interrupts saved by the SuspendOSInterrupts service.
 *
 * @param[in]  none
 * @param[out]  none
 * @return None
 *
 * @particulars
 * The service may be called from an ISR category 1 and category 2 and from the task level, but not from hook routines.
 * This service is the counterpart of SuspendOSInterrupts service, which has to have been called before, and its aim is
 * the completion of the critical section of code. No API service calls  beside SuspendAllInterrupts/ResumeAllInterrupts
 * pairs and SuspendOSInterrupts/ResumeOSInterrupts pairs are allowed within this critical section. The implementation
 * should adapt this service to the target hardware providing a minimum overhead. SuspendOSInterrupts/ResumeOSInterrupts
 * can be nested. In case of nesting pairs of the calls SuspendOSInterrupts and ResumeOSInterrupts the interrupt
 * recognition status saved by the first call of SuspendOSInterrupts is restored by the last call of the
 * ResumeOSInterrupts service.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void ResumeOSInterrupts (void);

/**
 * @brief  Saves the recognition status of OS-managed interrupts and disables them.
 * @details This service saves the recognition status of interrupts of category 2 and disables the recognition of these
 * interrupts.
 *
 * @param[in]  none
 * @param[out]  none
 * @return None
 *
 * @particulars
 * The service may be called from an ISR and from the task level, but not from hook routines.
 * This service is intended to protect a critical section of code. This section shall be finished by calling the
 * ResumeOSInterrupts service. No API service calls beside SuspendAllInterrupts/ResumeAllInterrupts pairs and
 * SuspendOSInterrupts/ResumeOSInterrupts pairs are allowed within this critical section. The implementation should
 * adapt this service to the target hardware providing a minimum overhead. It is intended only to disable interrupts of
 * category 2. However, if this is not possible in an efficient way more interrupts may be disabled.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
void SuspendOSInterrupts (void);

/**
 * @brief  Terminates the calling task and activates a succeeding task.
 * @details This service causes the termination of the calling task. After termination of the calling task a succeeding
 * task <TaskID> is activated. Using this service, it ensures that the succeeding task starts to run at the earliest
 * after the calling task has been terminated.
 *
 * @param[in] TaskID Reference to the sequential succeeding task to be activated.
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No return to call level if successful.
 * @retval E_OS_LIMIT  Too many task activations of @p TaskID.
 * @par Extended Status:
 * @retval E_OS_ID     Task @p TaskID is invalid.
 * @retval E_OS_RESOURCE  Calling task still occupies resources.
 * @retval E_OS_CALLEVEL  Call at interrupt level.
 *
 * @particulars
 * If the succeeding task is identical with the current task, this does not result in multiple requests. The task is not
 * transferred to the suspended state, but will immediately become ready again. An internal resource assigned to the
 * calling task is automatically released, even if the succeeding task is identical with the current task. Other
 * resources occupied by the calling shall have been released before ChainTask is called. If a resource is still
 * occupied in standard status the behaviour is undefined. If called successfully, ChainTask does not return to the call
 * level and the status can not be evaluated. In case of error the service returns to the calling task and provides a
 * status which can then be evaluated in the application. If the service ChainTask is called successfully, this enforces
 * a rescheduling. Ending a task function without call to TerminateTask or ChainTask is strictly forbidden and may leave
 * the system in an undefined state. If E_OS_LIMIT is returned the activation is ignored. When an extended task is
 * transferred from suspended state into ready state all its events are cleared.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 *
 * @note 15 In case of tasks with multiple activation requests, terminating the current instance of the task
 * automatically puts the next instance of the same task into the ready state.
 */
StatusType ChainTask (TaskType TaskID);

/**
 * @brief  Acquires a resource to enter a critical section.
 * @details This call serves to enter critical sections in the code that are assigned to the resource referenced by
 <ResID>.
 * A critical section shall always be left using ReleaseResource.
 *
 * @param[in] ResID Reference to resource
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Resource @p ResID is invalid.
 * @retval E_OS_ACCESS Attempt to get a resource which is already occupied, or the
 *                     calling task/ISR priority is higher than the ceiling priority.
 *
 * @particulars
 * The OSEK priority ceiling protocol for resource management is described in chapter 8.5.
 * Nested resource occupation is only allowed if the inner critical sections are completely executed within the
 surrounding
 * critical section (strictly stacked, see chapter 8.2, Restrictions when using resources). Nested occupation of one and
 the
 * same resource is also forbidden!
 * It is recommended that corresponding calls to GetResource and ReleaseResource appear within the same function.
 * It is not allowed to use services which are points of rescheduling for non preemptable tasks (TerminateTask,
 ChainTask,
 * Schedule and WaitEvent, see chapter 4.6.2) in critical sections. Additionally, critical sections are to be left
 before
 * completion of an interrupt service routine.
 * Generally speaking, critical sections should be short.
 * The service may be called from an ISR and from task level (see Figure 12-1).

 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType GetResource (ResourceType ResID);

/**
 * @brief  Releases a previously acquired resource to leave a critical section.
 * @details ReleaseResource is the counterpart of GetResource and serves to leave critical sections in the code that
 * are assigned to the resource referenced by <ResID>.
 *
 * @param[in] ResID Reference to resource
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Resource @p ResID is invalid.
 * @retval E_OS_NOFUNC Attempt to release a resource which is not occupied by
 *                     any task or ISR, or another resource shall be released
 *                     before (LIFO violation).
 * @retval E_OS_ACCESS Attempt to release a resource which has a lower ceiling
 *                     priority than the statically assigned priority of the
 *                     calling task or interrupt routine.
 *
 * @particulars
 * For information on nesting conditions, see particularities of GetResource.
 * The service may be called from an ISR and from task level (see Figure 12-1).
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType ReleaseResource (ResourceType ResID);

/**
 * @brief  Sets one or more events for a specific task.
 * @details The service may be called from an interrupt service routine and from the task level, but not from hook
 * routines. The events of task <TaskID> are set according to the event mask <Mask>. Calling SetEvent causes the task
 * <TaskID> to be transferred to the ready state, if it was waiting for at least one of the events specified in <Mask>.
 *
 * @param[in] TaskID Reference to the task for which one or several events are to be set.
 * @param[in] Mask Mask of the events to be set
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Task @p TaskID is invalid.
 * @retval E_OS_ACCESS Referenced task is not an extended task.
 * @retval E_OS_STATE  Events cannot be set as the referenced task is in the suspended state.
 *
 * @particulars
 * Any events not set in the event mask remain unchanged.
 *
 * @conformance
 * ECC1, ECC2
 */
StatusType SetEvent (TaskType TaskID, EventMaskType Mask);

/**
 * @brief  Clears the specified events for the calling task.
 * @details The events of the extended task calling ClearEvent are cleared according to the event mask <Mask>.
 *
 * @param[in] Mask Mask of the events to be cleared
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ACCESS The calling task is not an extended task.
 * @retval E_OS_CALLEVEL The service was called at interrupt level.
 *
 * @particulars
 * The system service ClearEvent is restricted to extended tasks which own the event.
 *
 * @conformance
 * ECC1, ECC2
 */
StatusType ClearEvent (EventMaskType Mask);

/**
 * @brief  Returns the current state of all event bits of a specific task.
 * @details This service returns the current state of all event bits of the task <TaskID>, not the events that
 * the task is waiting for.
 * The service may be called from interrupt service routines, task level and some hook routines (see Figure 12-1).
 * The current status of the event mask of task <TaskID> is copied to <Event>.
 *
 * @param[in] TaskID Task whose event mask is to be returned.
 * @param[out] Event Reference to the memory of the return data.
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Task @p TaskID is invalid.
 * @retval E_OS_ACCESS Referenced task @p TaskID is not an extended task.
 * @retval E_OS_STATE  Referenced task @p TaskID is in the suspended state.
 *
 * @particulars
 * The referenced task shall be an extended task.
 *
 * @conformance
 * ECC1, ECC2
 */
StatusType GetEvent (TaskType TaskID, EventMaskRefType Event);

/**
 * @brief  Allows the calling task to wait for specific events.
 * @details The state of the calling task is set to waiting, unless at least one of the events specified in <Mask>
 * has already been set.
 *
 * @param[in] Mask Mask of the events waited for.
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ACCESS The calling task is not an extended task.
 * @retval E_OS_RESOURCE The calling task still occupies resources.
 * @retval E_OS_CALLEVEL The service was called at interrupt level.
 *
 * @particulars
 * This call enforces rescheduling, if the wait condition occurs. If rescheduling takes place, the internal resource
 * of the task is released while the task is in the waiting state.
 * This service shall only be called from the extended task owning the event.
 *
 * @conformance
 * ECC1, ECC2
 */
StatusType WaitEvent (EventMaskType Mask);

/**
 * @brief  Retrieves the characteristics of the counter bound to an alarm.
 * @details The system service GetAlarmBase reads the alarm base characteristics. The return value <Info> is a
 * structure in which the information of data type AlarmBaseType is stored.
 *
 * @param[in] AlarmID Reference to alarm
 * @param[out] Info Reference to structure with constants of the alarm base.
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @par Extended Status:
 * @retval E_OS_ID     Alarm @p AlarmID is invalid.
 *
 * @particulars
 * Allowed on task level, ISR, and in several hook routines (see Figure 12-1).
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType GetAlarmBase (AlarmType AlarmID, AlarmBaseRefType Info);

/**
 * @brief  Returns the relative value in ticks before the alarm expires.
 * @details The system service GetAlarm returns the relative value in ticks before the alarm <AlarmID> expires.
 *
 * @param[in] AlarmID Reference to an alarm
 * @param[out] Tick Relative value in ticks before the alarm <AlarmID> expires.
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @retval E_OS_NOFUNC Alarm @p AlarmID is not in use (inactive).
 * @par Extended Status:
 * @retval E_OS_ID     Alarm @p AlarmID is invalid.
 *
 * @particulars
 * It is up to the application to decide whether for example a CancelAlarm may still be useful.
 * If <AlarmID> is not in use, <Tick> is not defined.
 * Allowed on task level, ISR, and in several hook routines (see Figure 12-1).
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType GetAlarm (AlarmType AlarmID, TickRefType Tick);

/**
 * @brief  Activates an alarm with a relative start time.
 * @details The system service occupies the alarm <AlarmID> element. After <increment> ticks have elapsed, the
 * task assigned to the alarm <AlarmID> is activated or the assigned event (only for extended tasks) is set or
 * the alarm-callback routine is called.
 * @param[in] AlarmID Reference to the alarm element
 * @param[in] increment Relative value in ticks
 * @param[in] cycle Cycle value in case of cyclic alarm. In case of single alarms, cycle shall be zero.
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @retval E_OS_STATE  Alarm @p AlarmID is already in use.
 * @par Extended Status:
 * @retval E_OS_ID     Alarm @p AlarmID is invalid.
 * @retval E_OS_VALUE  @p increment or @p cycle value outside of admissible counter limits.
 *
 * @particulars
 * The behaviour of <increment> equal to 0 is up to the implementation.
 * If the relative value <increment> is very small, the alarm may expire, and the task may become ready or the
 * alarm-callback may be called before the system service returns to the user.
 * If <cycle> is unequal zero, the alarm element is logged on again immediately after expiry with the relative
 * value <cycle>.
 * The alarm <AlarmID> must not already be in use.
 * To change values of alarms already in use the alarm shall be cancelled first.
 * If the alarm is already in use, this call will be ignored and the error E_OS_STATE is returned.
 * Allowed on task level and in ISR, but not in hook routines.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2; Events only ECC1, ECC2
 */
StatusType SetRelAlarm (AlarmType AlarmID, TickType increment, TickType cycle);

/**
 * @brief  Activates an alarm with an absolute start time.
 * @details The system service occupies the alarm <AlarmID> element. When <start> ticks are reached, the task
 * assigned to the alarm <AlarmID> is activated or the assigned event (only for extended tasks) is set or the
 * alarm-callback routine is called.
 * @param[in] AlarmID Reference to the alarm element
 * @param[in] start Absolute value in ticks
 * @param[in] cycle Cycle value in case of cyclic alarm. In case of single alarms, cycle shall be zero.
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @retval E_OS_STATE  Alarm @p AlarmID is already in use.
 * @par Extended Status:
 * @retval E_OS_ID     Alarm @p AlarmID is invalid.
 * @retval E_OS_VALUE  @p start or @p cycle value outside of admissible counter limits.
 *
 * @particulars
 * If the absolute value <start> is very close to the current counter value, the alarm may expire, and the task
 * may become ready or the alarm-callback may be called before the system service returns to the user.
 * If the absolute value <start> already was reached before the system call, the alarm shall only expire when the
 * absolute value <start> is reached again, i.e. after the next overrun of the counter.
 * If <cycle> is unequal zero, the alarm element is logged on again immediately after expiry with the relative
 * value <cycle>.
 * The alarm <AlarmID> shall not already be in use.
 * To change values of alarms already in use the alarm shall be cancelled first.
 * If the alarm is already in use, this call will be ignored and the error E_OS_STATE is returned.
 * Allowed on task level and in ISR, but not in hook routines.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2; Events only ECC1, ECC2
 */
StatusType SetAbsAlarm (AlarmType AlarmID, TickType start, TickType cycle);

/**
 * @brief  Cancels an active alarm.
 * @details The system service cancels the alarm <AlarmID>.
 *
 * @param[in] AlarmID Reference to an alarm
 * @param[out]  none
 * @return StatusType
 * @par Standard Status:
 * @retval E_OK        No error.
 * @retval E_OS_NOFUNC Alarm @p AlarmID is not in use (already inactive).
 * @par Extended Status:
 * @retval E_OS_ID     Alarm @p AlarmID is invalid.
 *
 * @particulars
 * Allowed on task level and in ISR, but not in hook routines. Status:
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
StatusType CancelAlarm (AlarmType AlarmID);

/**
 * @brief  Increments a software counter.
 * @details This service increments a software counter.
 * [SWS_Os_00399] Definition of API function IncrementCounter
 *
 * @param[in] CounterID The Counter to be incremented
 * @param[in,out] None
 * @param[out] None
 * @return StatusType
 * @retval E_OK: No errors
 * @retval E_OS_ID (only in EXTENDED status): The CounterID was not valid or counter is implemented in hardware and can
 * not be incremented by software
 *
 * @service_id
 * [hex]0x0f
 *
 * @sync_async
 * Synchronous
 *
 * @reentrancy
 * Reentrant
 *
 * @available_via
 * Os.h
 */
StatusType IncrementCounter (CounterType CounterID);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  Reads the current count value of a counter.
 * @details  This service reads the current count value of a counter (returning either the hardware timer ticks if
 * counter is driven by hardware or the software ticks when user drives counter). [SWS_Os_00383] Definition of API
 * function GetCounterValue ⌈
 *
 * @param[in] CounterID The Counter which tick value should be read
 * @param[in,out] None
 * @param[out] Value Contains the current tick value of the counter
 * @return  StatusType
 * @retval E_OK: No errors
 * @retval E_OS_ID (only in EXTENDED status): The <CounterID> was not valid
 *
 * @service_id
 * [hex] 0x10
 *
 * @sync_async
 * Synchronous
 *
 * @reentrancy
 * Reentrant
 *
 * @available_via
 * Os.h
 */
StatusType GetCounterValue (CounterType CounterID,
                            TickRefType Value);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  Calculates the elapsed ticks since a previous reading.
 * @details  This service gets the number of ticks between the current tick value and a previously read tick value.
 * [SWS_Os_00392] Definition of API function GetElapsedValue ⌈
 *
 * @param[in] CounterID The Counter to be read
 * @param[in,out] Value in: the previously read tick value of the counter, out: the current tick value of the counter
 * @param[out] ElapsedValue The difference to the previous read value
 * @return StatusType
 * @retval E_OK: No errors
 * @retval E_OS_ID (only in EXTENDED status): The CounterID was not valid
 * @retval E_OS_VALUE (only in EXTENDED status): The given Value was not valid
 *
 * @service_id
 * [hex] 0x11
 *
 * @sync_async
 * Synchronous
 *
 * @reentrancy
 * Reentrant
 *
 * @available_via
 * Os.h
 */
StatusType GetElapsedValue (CounterType CounterID, TickRefType Value,
                            TickRefType ElapsedValue);  // TODO (HE Juncheng/2026-03-15): AutoSAR接口可以移除 | NULL

/**
 * @brief  get active application mode.
 * @details This service returns the current application mode. It may be used to write mode dependent code.
 *
 * @param[in] none
 * @param[out]  none
 * @return AppModeType
 *
 * @particulars
 * See chapter 5 for a general description of application modes.
 * Allowed for task, ISR and all hook routines.
 *
 * @conformance
 * BCC1, BCC2, ECC1, ECC2
 */
AppModeType GetActiveApplicationMode (void);
#endif /** OS_H */
