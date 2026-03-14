#ifndef _ISR_H_
#define _ISR_H_
#include <stdint.h>

#include "Platform_Types.h"
/* ================== 硬件基地址 (修正为 QEMU VExpress-A9 匹配地址) ================== */
// A9MPCore Private Memory Region (init_cpus 函数中定义的 0x1e000000)
#define PERIPHBASE 0x1e000000
#define GIC_CPU_BASE (PERIPHBASE + 0x0100)   // 0x1e000100
#define GIC_DIST_BASE (PERIPHBASE + 0x1000)  // 0x1e001000
#define MPC_PRIV_BASE (PERIPHBASE + 0x0600)  // Private Timer
#define MPC_GLOB_BASE (PERIPHBASE + 0x0200)  // Global Timer

// Motherboard Peripherals (由 motherboard_legacy_map 定义)
#define SYS_SP804_BASE 0x10011000  // VE_TIMER01
#define SYS_RTC_BASE 0x10017000    // VE_RTC

/* ================== 中断号修正 ================== */
// GIC 内部中断 (PPI): 16-31
#define ID_GLOBAL_TIMER 27
#define ID_PRIVATE_TIMER 29
#define ID_WATCHDOG 30
// GIC 外部中断 (SPI): 32 + index
#define ID_RTC 36    // QEMU pic[4] -> 32 + 4 = 36
#define ID_SP804 34  // QEMU pic[2] -> 32 + 2 = 34 (注意：你原先写48)

/* ================== 寄存器操作 ================== */
#define GICC_IAR (*(volatile uint32 *)(GIC_CPU_BASE + 0x0C))
#define GICC_EOIR (*(volatile uint32 *)(GIC_CPU_BASE + 0x10))

// SP804 Timer 1 (系统心跳)
#define SP804_T1_LOAD (SYS_SP804_BASE + 0x00)
#define SP804_T1_VALUE (SYS_SP804_BASE + 0x04)
#define SP804_T1_CONTROL (SYS_SP804_BASE + 0x08)
#define SP804_T1_INTCLR (SYS_SP804_BASE + 0x0C)
#define SP804_T1_MIS (SYS_SP804_BASE + 0x10)  // 屏蔽后的中断状态

// SP804 Timer 2 (其他业务)
#define SP804_T2_LOAD (SYS_SP804_BASE + 0x20)
#define SP804_T2_VALUE (SYS_SP804_BASE + 0x24)
#define SP804_T2_CONTROL (SYS_SP804_BASE + 0x28)
#define SP804_T2_INTCLR (SYS_SP804_BASE + 0x2C)
#define SP804_T2_MIS (SYS_SP804_BASE + 0x30)  // 屏蔽后的中断状态
void gic_enable_int (uint32 id);

void gic_init ();

/* ================== 1. Private Timer (ID 29) ================== */
void init_private_timer (uint32 load);

/* ================== 2. Private Watchdog (ID 30) ================== */
void init_private_wdog (uint32 load);
/* ================== 3. Global Timer (ID 27) ================== */
void init_global_timer (uint32 delta);

/* ================== 4. Dual-Timer SP804 (ID 48) ================== */
void init_sp804_dual (uint32 load1, uint32 load2);

/* ================== 5. RTC PL031 (ID 34) ================== */
void init_rtc (uint32 seconds_from_now);

#endif /* _ISR_H_ */
