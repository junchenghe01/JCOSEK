#include "isr.h"

void gic_enable_int (uint32 id)
{
    volatile uint32 *isenabler = (uint32 *)(GIC_DIST_BASE + 0x100 + (id / 32) * 4);
    *isenabler                 = (1 << (id % 32));
}

void gic_init ()
{
    *(volatile uint32 *)(GIC_DIST_BASE + 0x00) = 0x1;   // Enable Distributor
    *(volatile uint32 *)(GIC_CPU_BASE + 0x00)  = 0x1;   // Enable CPU Interface
    *(volatile uint32 *)(GIC_CPU_BASE + 0x04)  = 0xFF;  // Priority Mask
}

/* ================== 1. Private Timer (ID 29) ================== */
void init_private_timer (uint32 load)
{
    *(volatile uint32 *)(MPC_PRIV_BASE + 0x00) = load;
    *(volatile uint32 *)(MPC_PRIV_BASE + 0x08) = 0x7;  // Enable, Reload, IRQ
    gic_enable_int (ID_PRIVATE_TIMER);
}

/* ================== 2. Private Watchdog (ID 30) ================== */
void init_private_wdog (uint32 load)
{
    *(volatile uint32 *)(MPC_PRIV_BASE + 0x20) = load;
    *(volatile uint32 *)(MPC_PRIV_BASE + 0x28) = 0x7;  // Timer Mode
    gic_enable_int (ID_WATCHDOG);
}

/* ================== 3. Global Timer (ID 27) ================== */
void init_global_timer (uint32 delta)
{
    volatile uint32 *gt  = (uint32 *)MPC_GLOB_BASE;
    uint32           low = gt[0], high = gt[1];
    uint64           next = ((uint64)high << 32 | low) + delta;

    gt[2]       = (uint32)next;          // Comp Low
    gt[3]       = (uint32)(next >> 32);  // Comp High
    gt[0x8 / 4] = 0x0;                   // Set AUTO_INCREMENT to 0, and update it manually via the handler.
    gt[2]       = 0xF;                   // Enable, Comp, IRQ, Auto-inc
    gic_enable_int (ID_GLOBAL_TIMER);
}

/* ================== 4. Dual-Timer SP804 (ID 48) ================== */
// void init_sp804(uint32 load) {
//     *(volatile uint32 *)(SYS_SP804_BASE + 0x00) = load;
//     *(volatile uint32 *)(SYS_SP804_BASE + 0x08) = 0xE2; // 32bit, Periodic, IRQ, Enable
//     gic_enable_int(ID_SP804);
// }
void init_sp804_dual (uint32 load1, uint32 load2)
{
    // --- Timer 1 Configuration (System Heartbeat - Periodic) ---
    *(volatile uint32 *)SP804_T1_LOAD = load1;
    // 0xE2: Enable, Periodic, IntEnable, 32-bit
    *(volatile uint32 *)SP804_T1_CONTROL = 0xE2;

    // --- Timer 2 Configuration (Other Applications - e.g., One-shot Mode) ---
    *(volatile uint32 *)SP804_T2_LOAD = load2;
    // 0x62: Enable, One-shot(bit 0=0), IntEnable, 32-bit
    // If Periodic mode is also required, keep it at 0xE2.
    *(volatile uint32 *)SP804_T2_CONTROL = 0xE2;

    // Enable ID 34 in the GIC (two timers share this ID).
    gic_enable_int (ID_SP804);
}
/* ================== 5. RTC PL031 (ID 34) ================== */
void init_rtc (uint32 seconds_from_now)
{
    uint32 now                                = *(volatile uint32 *)(SYS_RTC_BASE + 0x00);
    *(volatile uint32 *)(SYS_RTC_BASE + 0x04) = now + seconds_from_now;  // Alarm
    *(volatile uint32 *)(SYS_RTC_BASE + 0x08) = 0x1;                     // Interrupt Mask Enable
    gic_enable_int (ID_RTC);
}
