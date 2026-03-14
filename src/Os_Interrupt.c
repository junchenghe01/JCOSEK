// SPDX-License-Identifier: MPL-2.0

/**
 * @file  Os_Interrupt.c
 * @brief Core implementation of the OSEK/VDX Operating System kernel.
 *
 * @details
 * This file implements the core functionalities of the OSEK OS 2.2.3 specification, including:
 * - Interrupt Management
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

/* ============================================================================
 * Local Types/Structs
 * ========================================================================= */

/* ============================================================================
 * Static Global Variables
 * ========================================================================= */

/* ============================================================================
 * Static Function Prototypes
 * ========================================================================= */

/* ============================================================================
 * Public API Implementation
 * ========================================================================= */
void EnableAllInterrupts (void)
{
    // uint32 temp;

    // __asm__ volatile (
    //     /* 1. Load the previously saved CPSR value from memory */
    //     "ldr %[tmp], [%[storage]] \n\t"

    //     /* 2. Extract only the I (bit 7) and F (bit 6) mask bits from the saved value */
    //     "and %[tmp], %[tmp], #0xC0 \n\t"

    //     /**
    //      * 3. Atomically update the current CPSR interrupt bits:
    //      *    - mrs: Read the current status register
    //      *    - bic: Clear only the I and F bits (bit 6 and 7)
    //      *    - orr: Merge the saved I/F bits into the current state
    //      *    - msr: Write back to the Control field of the status register
    //      */
    //     "mrs r1, cpsr           \n\t"
    //     "bic r1, r1, #0xC0      \n\t" /* Clear current I/F bits */
    //     "orr r1, r1, %[tmp]     \n\t" /* Apply saved I/F bits */
    //     "msr cpsr_c, r1         \n\t" /* Write back to CPSR control field */
    //     : [tmp] "=&r"(temp)
    //     : [storage] "r"(&saved_cpsr)
    //     : "r1", "memory", "cc");
    portENABLE_ALL_INTERRUPTS ();
}

void DisableAllInterrupts (void)
{
    // uint32 temp;

    // __asm__ volatile (
    //     /* 1. Read the Current Program Status Register (CPSR) into a temporary register */
    //     "mrs %[tmp], cpsr      \n\t"

    //     /* 2. Save the captured CPSR (including current interrupt masks) to global storage */
    //     "str %[tmp], [%[storage]] \n\t"

    //     /**
    //      * 3. Efficiently disable interrupts:
    //      *    'cpsid' (Change Processor State - Interrupt Disable)
    //      *    'if' refers to both IRQ (i) and FIQ (f) bits.
    //      */
    //     "cpsid if              \n\t"
    //     : [tmp] "=&r"(temp)
    //     : [storage] "r"(&saved_cpsr)
    //     : "memory", "cc");
    portDISABLE_ALL_INTERRUPTS ();
}

void SuspendAllInterrupts (void)
{
    // uint32 current_cpsr;

    // /**
    //  * 1. Capture the Current Program Status Register (CPSR).
    //  *    This captures the state (including I/F bits) before any modifications.
    //  */
    // __asm__ volatile ("mrs %[tmp], cpsr" : [tmp] "=r"(current_cpsr));

    // /**
    //  * 2. Immediately disable interrupts (IRQ and FIQ).
    //  *    Using 'cpsid' for minimum latency to protect the nesting counter
    //  *    and global state from race conditions.
    //  */
    // __asm__ volatile ("cpsid if" : : : "memory");

    // /**
    //  * 3. Preserve the original state on the first nested call.
    //  *    If 'suspend_all_count' is 0, this is the outermost call;
    //  *    save the 'current_cpsr' to be restored later.
    //  */
    // if (suspend_all_count == 0)
    // {
    //     saved_cpsr_for_suspend = current_cpsr;
    // }

    // /**
    //  * 4. Increment the nesting counter.
    //  *    Ensures that the corresponding ResumeAllInterrupts knows the nesting depth.
    //  */
    // suspend_all_count++;
    portSUSPEND_ALL_INTERRUPTS ();
}

void ResumeAllInterrupts (void)
{
    // /**
    //  * 1. Safety check: Only proceed if there is an active suspension.
    //  *    This prevents the counter from underflowing.
    //  */
    // if (suspend_all_count > 0)
    // {
    //     /* Decrement the nesting depth */
    //     suspend_all_count--;

    //     /**
    //      * 2. Physical Restore: Only perform hardware restoration when the
    //      *    outermost nesting level is reached (counter is 0).
    //      */
    //     if (suspend_all_count == 0)
    //     {
    //         uint32 temp_cpsr = saved_cpsr_for_suspend;

    //         __asm__ volatile (
    //             /* Read current status to modify only specific bits */
    //             "mrs r1, cpsr           \n\t"

    //             /* Clear the I (bit 7) and F (bit 6) bits from current status */
    //             "bic r1, r1, #0xC0      \n\t"

    //             /* Extract saved I and F bits from the temp variable into r0 */
    //             "and r0, %[saved], #0xC0 \n\t"

    //             /* Combine the original I/F bits with the current status */
    //             "orr r1, r1, r0         \n\t"

    //             /* Write back to the Control field of the CPSR */
    //             "msr cpsr_c, r1         \n\t"
    //             :
    //             : [saved] "r"(temp_cpsr)
    //             : "r0", "r1", "memory", "cc");
    //     }
    // }
    portRESUME_ALL_INTERRUPTS ();
}

void SuspendOSInterrupts (void)
{
    // uint32 current_cpsr;

    // /**
    //  * 1. Capture the Current Program Status Register (CPSR).
    //  *    This stores the current interrupt mask state before modification.
    //  */
    // __asm__ volatile ("mrs %[tmp], cpsr" : [tmp] "=r"(current_cpsr));

    // /**
    //  * 2. Immediately disable IRQ (Interrupt Request).
    //  *    Using 'cpsid i' masks only standard interrupts.
    //  *    The 'f' bit (FIQ) is intentionally left unchanged to support Cat1 ISRs.
    //  */
    // __asm__ volatile ("cpsid i" : : : "memory");

    // /**
    //  * 3. Handle nesting logic:
    //  *    If this is the outermost call (count is 0), preserve the original CPSR.
    //  */
    // if (suspend_os_count == 0)
    // {
    //     saved_cpsr_for_os = current_cpsr;
    // }

    // /* Increment the nesting depth counter */
    // suspend_os_count++;
    portSUSPEND_OS_INTERRUPTS ();
}

void ResumeOSInterrupts (void)
{
    // /**
    //  * 1. Underflow protection:
    //  *    Ensure there is an active OS suspension before proceeding.
    //  */
    // if (suspend_os_count > 0)
    // {
    //     /* Decrement the nesting depth */
    //     suspend_os_count--;

    //     /**
    //      * 2. Physical Restore:
    //      *    Perform hardware register update only at the outermost nesting level.
    //      */
    //     if (suspend_os_count == 0)
    //     {
    //         uint32 temp_cpsr = saved_cpsr_for_os;

    //         __asm__ volatile (
    //             /* Read the current status register into r1 */
    //             "mrs r1, cpsr           \n\t"

    //             /* Clear only the I-bit (bit 7) in the current status */
    //             "bic r1, r1, #0x80      \n\t"

    //             /* Extract the original I-bit from the saved CPSR into r0 */
    //             "and r0, %[saved], #0x80 \n\t"

    //             /* Merge the original I-bit back into the current status */
    //             "orr r1, r1, r0         \n\t"

    //             /* Apply the modification to the CPSR Control byte */
    //             "msr cpsr_c, r1         \n\t"
    //             :
    //             : [saved] "r"(temp_cpsr)
    //             : "r0", "r1", "memory", "cc");
    //     }
    // }
    portRESUME_OS_INTERRUPTS ();
}
/* ============================================================================
 * Internal Helper Functions
 * ========================================================================= */
