#include "isr.h"



void gic_enable_int(uint32_t id) {
    // GICD_ISENABLERn: 每 32 个中断占用一个寄存器
    volatile uint32_t *isenabler = (uint32_t *)(GIC_DIST_BASE + 0x100 + (id / 32) * 4);
    *isenabler = (1 << (id % 32));
}

void gic_init() {
    *(volatile uint32_t *)(GIC_DIST_BASE + 0x00) = 0x1; // Enable Distributor
    *(volatile uint32_t *)(GIC_CPU_BASE + 0x00)  = 0x1; // Enable CPU Interface
    *(volatile uint32_t *)(GIC_CPU_BASE + 0x04)  = 0xFF; // Priority Mask
}

/* ================== 1. Private Timer (ID 29) ================== */
void init_private_timer(uint32_t load) {
    *(volatile uint32_t *)(MPC_PRIV_BASE + 0x00) = load;
    *(volatile uint32_t *)(MPC_PRIV_BASE + 0x08) = 0x7; // Enable, Reload, IRQ
    gic_enable_int(ID_PRIVATE_TIMER);
}

/* ================== 2. Private Watchdog (ID 30) ================== */
void init_private_wdog(uint32_t load) {
    *(volatile uint32_t *)(MPC_PRIV_BASE + 0x20) = load;
    *(volatile uint32_t *)(MPC_PRIV_BASE + 0x28) = 0x7; // 定时器模式
    gic_enable_int(ID_WATCHDOG);
}

/* ================== 3. Global Timer (ID 27) ================== */
void init_global_timer(uint32_t delta) {
    volatile uint32_t *gt = (uint32_t *)MPC_GLOB_BASE;
    uint32_t low = gt[0], high = gt[1];
    uint64_t next = ((uint64_t)high << 32 | low) + delta;
    
    gt[2] = (uint32_t)next;         // Comp Low
    gt[3] = (uint32_t)(next >> 32);  // Comp High
    gt[0x8/4] = 0x0;                // Auto-increment 设为0，由handler手动更新
    gt[2] = 0xF;                    // Enable, Comp, IRQ, Auto-inc
    gic_enable_int(ID_GLOBAL_TIMER);
}

/* ================== 4. Dual-Timer SP804 (ID 48) ================== */
// void init_sp804(uint32_t load) {
//     *(volatile uint32_t *)(SYS_SP804_BASE + 0x00) = load;
//     *(volatile uint32_t *)(SYS_SP804_BASE + 0x08) = 0xE2; // 32bit, Periodic, IRQ, Enable
//     gic_enable_int(ID_SP804);
// }
void init_sp804_dual(uint32_t load1, uint32_t load2) {
    // --- Timer 1 配置 (系统心跳 - Periodic) ---
    *(volatile uint32_t *)SP804_T1_LOAD    = load1;
    // 0xE2: Enable, Periodic, IntEnable, 32-bit
    *(volatile uint32_t *)SP804_T1_CONTROL = 0xE2; 

    // --- Timer 2 配置 (其他业务 - 例如 One-shot 模式) ---
    *(volatile uint32_t *)SP804_T2_LOAD    = load2;
    // 0x62: Enable, One-shot(bit 0=0), IntEnable, 32-bit
    // 如果也要 Periodic 模式，则保持 0xE2
    *(volatile uint32_t *)SP804_T2_CONTROL = 0xE2; 

    // 开启 GIC 中的 ID 34 (两个定时器共享此 ID)
    gic_enable_int(ID_SP804); 
}
/* ================== 5. RTC PL031 (ID 34) ================== */
void init_rtc(uint32_t seconds_from_now) {
    uint32_t now = *(volatile uint32_t *)(SYS_RTC_BASE + 0x00);
    *(volatile uint32_t *)(SYS_RTC_BASE + 0x04) = now + seconds_from_now; // Alarm
    *(volatile uint32_t *)(SYS_RTC_BASE + 0x08) = 0x1; // Interrupt Mask Enable
    gic_enable_int(ID_RTC);
}
