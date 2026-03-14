// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Lifecycle.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - System Lifecycle Management
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

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "Os.h"
#include "Os_Internal.h"

/* ============================================================================
 * Macros & Constants
 * ========================================================================= */
/**
 * @brief  Shutdown Strategy: Infinite Loop.
 * @details The system enters a safe 'hang' state, typically used during
 *          development to allow debugger attachment after a fatal error.
 */
#define SHUTDOWN_ACTION_INFINITE_LOOP 0

/**
 * @brief  Shutdown Strategy: Hardware Reboot.
 * @details Triggers a physical reset of the MCU via system control registers.
 */
#define SHUTDOWN_ACTION_HARDWARE_REBOOT 1

/**
 * @brief  Shutdown Strategy: Low Power Mode.
 * @details Configures the CPU to enter its deepest sleep state (e.g., Deep Sleep)
 *          to conserve energy after a controlled shutdown.
 */
#define SHUTDOWN_ACTION_LOW_POWER_MODE 2

/**
 * @brief  Hardware Register Mapping for System Control.
 * @details Define the base address and specific registers for platform-level
 *          operations such as reset and power management.
 */
#define SYSREGS_BASE 0x10000000

/** @brief System Control Register (Volatile access to prevent optimization) */
#define SYS_CTRL (*(volatile unsigned int*)(SYSREGS_BASE + 0x000))

/** @brief Bitmask for Software Reset (Assumption: Bit 0 triggers the reset) */
#define SYS_SOFTRESET 0x01
/* ============================================================================
 * Local Types/Structs
 * ========================================================================= */

/* ============================================================================
 * Static Global Variables
 * ========================================================================= */

/* ============================================================================
 * Static Function Prototypes
 * ========================================================================= */
static void Os_Internal_Hardware_Reset (void);
/* ============================================================================
 * Public API Implementation
 * ========================================================================= */

void Os_Start (void)
{
    /* Find the initial task to run based on priority bitmap */
    uint16 highest_prio = Os_Internal_GetHighestPriorityIndex ();

    /* Safety Check: If no task is ready, the system cannot start */
    if (highest_prio == 0xFFFF)
    {
        /* TODO (HE Juncheng/2026-03-15): Replace with a proper Kernel Panic or Idle Task */
        while (1)
            ;
    }

    /* Retrieve the TCB of the highest priority task */
    TaskControlBlock* pFirstTask = ocb.ReadyQueues[highest_prio];

    /* Update kernel state to reflect the first running task */
    pFirstTask->TaskState = TASK_STATE_RUNNING;
    ocb.pCurrentTask      = pFirstTask;

    // /**
    //  * Context Switch: Manually restore task context (ARM Architecture)
    //  * 1. mov sp, %0        : Load the saved stack pointer from TCB
    //  * 2. pop {r4-r11, lr}  : Restore callee-saved registers and Link Register
    //  * 3. bx lr             : Branch to task entry point (starts execution)
    //  */
    // __asm__ volatile (
    //     "mov sp, %0\n"
    //     "pop {r4-r11, lr}\n"
    //     "bx lr\n"
    //     :
    //     : "r"(pFirstTask->StackPtr));
    portSTART_FIRST_TASK (pFirstTask->StackPtr);
}

AppModeType GetActiveApplicationMode (void)
{
    return ocb.appMode;
}

__attribute__ ((weak)) void StartupHook (void)
{
    /* TODO (HE Juncheng/2026-03-15):*/
}

void StartOS (AppModeType Mode)
{
    /**
     * Step: Initialize Global Control Block.
     * Record the application mode for later retrieval via GetActiveApplicationMode().
     */
    ocb.appMode = Mode;

    /**
     * Step: Handle AUTOSTART Tasks.
     * Iterate through all tasks defined in the OIL file. If a task is
     * configured to auto-start in the current mode, move it to the READY state.
     */
    for (uint64 i = 0; i < ocb.TasksSize; i++)
    {
        /**
         * [TODO (HE Juncheng/2026-03-15)]: In a full implementation, check if isAutoStart matches the
         * current 'Mode' (some tasks may only auto-start in specific modes).
         */
        if (ocb.Tasks[i].isAutoStart == TRUE)
        {
            /* Add the task to the ready queue and update its internal state */
            Os_Internal_EnqueueReadyTask (&ocb.Tasks[i], ocb.Tasks[i].CurrentPriority, 0);
            ocb.Tasks[i].TaskState = TASK_STATE_READY;
        }
    }

    /**
     * Step: Handle AUTOSTART Alarms.
     * [UNIMPLEMENTED]: Iterate through ocb.Alarms and activate those with
     * autostart enabled for the current mode.
     */

    /**
     * Step: Invoke Startup Hooks.
     * All user interrupts (Category 2) are disabled during this call.
     * This is the last chance for the user to perform initialization
     * before the multi-tasking environment begins.
     */
    StartHooks ();

    /**
     * Step: Execute the first task.
     * Triggers the context switch to the highest priority task in the
     * ready queue. This call never returns.
     */
    Os_Start ();
}

__attribute__ ((weak)) void ShutdownHook (StatusType Error)
{
    // EcuM_Shutdown ();
}
/**
 * @brief  Shuts down the operating system.
 * @details This service aborts the overall OS execution. It is the final destination
 *          for the system, triggered either by a fatal error (e.g., stack overflow,
 *          internal inconsistency) or a controlled application request.
 *
 * @param[in] Error  The error code that caused the shutdown.
 *
 * @return void      This function NEVER returns to the caller.
 *
 * @note   Compliance: OSEK OS 2.2.3, Section 11.5
 * @warning Once ShutdownOS is called, all task scheduling and alarms are stopped.
 */
void ShutdownOS (StatusType Error)
{
    /**
     * Step 1: Disable all interrupts.
     * To ensure atomicity during the shutdown process and prevent any new
     * hardware events or task activations from occurring.
     */
    // Os_Arch_DisableAllInterrupts();

    /**
     * Step 2: Invoke the ShutdownHook.
     * This user-defined hook allows for "last words" logic:
     * - Logging error codes to non-volatile memory (EEPROM/Flash).
     * - Putting external hardware (actuators/motors) into a safe state.
     */
    ShutdownHook (Error);

    /**
     * Step 3: Stop all active objects.
     * Forcefully transition all tasks to the SUSPENDED state and cancel
     * pending alarms/resources to clean up the internal OS state.
     */
    for (uint32 i = 0; i < ocb.TasksSize; i++)
    {
        ocb.Tasks[i].TaskState = TASK_STATE_SUSPENDED;
    }

    /**
     * Step 4: Final Implementation-Defined Action.
     * Depending on the system configuration (ShutdownFlag), the CPU will
     * either hang, reboot, or enter a low-power sleep state.
     */
    uint32 ShutdownFlag = 1; /* [TODO (HE Juncheng/2026-03-15)]: This should be a configurable parameter */
    switch (ShutdownFlag)
    {
        case SHUTDOWN_ACTION_INFINITE_LOOP:
            /* Safe hang for debugging purposes */
            while (1)
                ;
            break;
        case SHUTDOWN_ACTION_HARDWARE_REBOOT:
            /* Trigger physical reset via system control registers */
            Os_Internal_Hardware_Reset ();
            break;
        case SHUTDOWN_ACTION_LOW_POWER_MODE:
            /* Enter sleep mode to save energy */
            while (1)
            {
                // __asm__ volatile ("wfi");
                portWAIT_FOR_INTERRUPT ();
            }
            break;
        default:
            /* Fallback: Secure idle state */
            while (1)
            {
                // __asm__ volatile ("wfi");
                portWAIT_FOR_INTERRUPT ();
            }
            break;
    }

    /**
     * [NOTE]: Standard OSEK shutdown does not guarantee full cleanup of
     * all resources; it is a terminal operation meant for fatal scenarios.
     */
}

ApplicationType GetApplicationID (void)
{
    /**
     * [SWS_Os_00261]
     * GetApplicationID shall return the application identifier to which
     * the executing Task/Cat2 ISR/hook was configured.
     * Note: Context for Category 1 ISR is undefined.
     *
     * [SWS_Os_00262]
     * If no OS-Application is running, GetApplicationID shall return INVALID_OSAPPLICATION.
     *
     * [SWS_Os_00514]
     * Availability: Scalability Classes 3 and 4 and in multi-core systems.
     */

    /* Default implementation: Currently returning invalid as OS-Apps are not yet defined */
    return INVALID_OSAPPLICATION;
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
/**
 * @brief  Performs a hardware-level system reset.
 * @details This function triggers a physical reboot of the microcontroller
 *          by writing the reset bitmask to the System Control Register.
 *          This is the final action in the shutdown sequence.
 *
 * @note   Platform Dependent: The base address and reset bit (SYS_CTRL)
 *         must be correctly mapped to the target hardware/SoC.
 * @warning This function does not return, as the CPU state is cleared immediately.
 */
static void Os_Internal_Hardware_Reset (void)
{
    /**
     * In some hardware versions, writing a specific value to the SYS_CTRL
     * register triggers a system-wide software reset.
     */
    SYS_CTRL = SYS_SOFTRESET;
}

/**
 * @brief  Dispatches the configured action when an alarm expires.
 * @details This internal function is triggered when an alarm's counter reaches
 *          its expiration point. It executes the specific action defined in
 *          the OIL configuration for that alarm.
 *
 * @param[in] pAlarm  Pointer to the Alarm Control Block (ACB) that has expired.
 *
 * @note   Execution Context: Typically called within the System Tick ISR or
 *         IncrementCounter context (Category 2 ISR).
 */
void Os_Internal_TriggerAlarmAction (AlarmControlBlock* pAlarm)
{
    /* Dispatch the action based on the OIL configuration */
    switch (pAlarm->Action)
    {
        case ALARM_ACTION_ACTIVATE_TASK:
            /**
             * Scenario A: Activate a task.
             * Directly invokes the standard ActivateTask service.
             */
            (void)ActivateTask (pAlarm->TargetTaskID);
            break;

        case ALARM_ACTION_SET_EVENT:
            /**
             * Scenario B: Set an event for an extended task.
             * Invokes the SetEvent service defined in the Event Control module.
             */
            (void)SetEvent (pAlarm->TargetTaskID, pAlarm->TargetEvent);
            break;

        case ALARM_ACTION_ALARM_CALLBACK:
            /**
             * Scenario C: Execute an Alarm-Callback function.
             * This usually runs in the interrupt context with high privileges.
             */
            if (pAlarm->AlarmCallback != NULL)
            {
                pAlarm->AlarmCallback ();
            }
            break;

        default:
            /* Error Handling: Unsupported or undefined alarm action */
            break;
    }
}
