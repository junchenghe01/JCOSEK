/**
 * @file    startup.c
 * @brief   Startup code and vector table for NXP S32K144 (Cortex-M4F).
 *
 * @details Defines the interrupt vector table in the .isr_vector section
 *          (NXP convention).  Implements Reset_Handler for system init.
 *
 *          All exception and interrupt handlers are weakly aliased to
 *          DefaultISR.  Override any handler by defining a function with
 *          the matching name in user code.
 *
 *          Used symbols from linker script:
 *            __StackTop      — initial MSP (end of system stack)
 *            __DATA_ROM      — Flash address where .data LMA starts
 *            __data_start__  — SRAM address where .data VMA starts
 *            __data_end__    — end of .data in SRAM
 *            __bss_start__   — start of .bss in SRAM
 *            __bss_end__     — end of .bss in SRAM
 *            __RAM_START     — start of SRAM (for ECC init)
 *            __RAM_END       — end of SRAM (for ECC init)
 */

#include <stdint.h>
#include "S32K144.h"

/* ==========================================================================
 * Linker-defined symbols
 * ========================================================================= */
extern uint32_t __StackTop;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __DATA_ROM;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

#ifdef ECC_RAM_INIT
extern uint32_t __RAM_START;
extern uint32_t __RAM_END;
#endif

/* ==========================================================================
 * Forward declarations (needed before vector table)
 * ========================================================================= */
extern int  main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void DefaultISR(void);

static void init_data_bss(void);

/* ==========================================================================
 * Fault capture variables — inspect these in the debugger to identify the
 * root cause when a fault or unexpected interrupt occurs.
 *
 * Usage in GDB:
 *   (gdb) p/x g_fault_cfsr
 *   (gdb) p/x g_fault_hfsr
 *   (gdb) p/x g_fault_mmfar
 *   (gdb) p/x g_fault_bfar
 *   (gdb) p/x g_default_isr_number
 * ========================================================================= */
static volatile uint32_t g_fault_cfsr;             /* Configurable Fault Status Register   */
static volatile uint32_t g_fault_hfsr;             /* HardFault Status Register            */
static volatile uint32_t g_fault_mmfar;            /* MemManage Fault Address Register     */
static volatile uint32_t g_fault_bfar;             /* BusFault Address Register            */
static volatile uint32_t g_fault_ufsr;             /* UsageFault Status Register (CFSR high)*/
static volatile uint32_t g_default_isr_number;     /* Exception # from ICSR.VECTACTIVE     */

/* SCB register base (Cortex-M4) */
#define SCB_ADDR  ((volatile uint32_t *)0xE000ED00U)
#define ICSR_OFF  (0x04U / 4U)   /* 0xE000ED04 — Interrupt Control and State */
#define CFSR_OFF  (0x28U / 4U)   /* 0xE000ED28 — Configurable Fault Status  */
#define HFSR_OFF  (0x2CU / 4U)   /* 0xE000ED2C — HardFault Status           */
#define MMFAR_OFF (0x34U / 4U)   /* 0xE000ED34 — MemManage Fault Address    */
#define BFAR_OFF  (0x38U / 4U)   /* 0xE000ED38 — BusFault Address           */

/* ==========================================================================
 * Default interrupt handler — captures the active exception number from
 * ICSR.VECTACTIVE so you can tell WHICH interrupt/exception fired.
 * ========================================================================= */
void DefaultISR(void)
{
    /* Read ICSR: bits [8:0] = VECTACTIVE (current exception number).
     * Exception numbers:
     *    3 = HardFault, 4 = MemManage, 5 = BusFault, 6 = UsageFault,
     *   11 = SVCall, 14 = PendSV, 15 = SysTick,
     *   16+ = peripheral IRQ (IRQ number = VECTACTIVE - 16)                */
    g_default_isr_number = SCB_ADDR[ICSR_OFF] & 0x1FFU;

    while (1) { }
}

/* ==========================================================================
 * Dedicated fault handlers — each captures its own status register(s)
 * before entering an infinite loop.  Now you can distinguish a HardFault
 * from a stray peripheral interrupt just by the symbol name in the debugger.
 * ========================================================================= */

/* ---- NMI — rare; keep as DefaultISR alias --------------------------------- */
void NMI_Handler(void)           __attribute__((weak, alias("DefaultISR")));
void DebugMon_Handler(void)      __attribute__((weak, alias("DefaultISR")));

/* ---- HardFault ----------------------------------------------------------- */
__attribute__((weak)) void HardFault_Handler(void)
{
    g_fault_hfsr = SCB_ADDR[HFSR_OFF];
    g_default_isr_number = 3; /* HardFault */
    while (1) { }
}

/* ---- MemManage (MPU fault) ----------------------------------------------- */
__attribute__((weak)) void MemManage_Handler(void)
{
    g_fault_cfsr  = SCB_ADDR[CFSR_OFF];
    g_fault_mmfar = SCB_ADDR[MMFAR_OFF];
    g_default_isr_number = 4; /* MemManage */
    while (1) { }
}

/* ---- BusFault (bus error) ------------------------------------------------ */
__attribute__((weak)) void BusFault_Handler(void)
{
    g_fault_cfsr = SCB_ADDR[CFSR_OFF];
    g_fault_bfar = SCB_ADDR[BFAR_OFF];
    g_default_isr_number = 5; /* BusFault */
    while (1) { }
}

/* ---- UsageFault (undefined instr, etc.) ---------------------------------- */
__attribute__((weak)) void UsageFault_Handler(void)
{
    g_fault_cfsr = SCB_ADDR[CFSR_OFF];
    g_default_isr_number = 6; /* UsageFault */
    while (1) { }
}

/* PendSV and SVC are provided by the OS port layer (portasm.S),
 * but declared weak here so the build succeeds without the kernel. */
void PendSV_Handler(void)        __attribute__((weak, alias("DefaultISR")));
void SVC_Handler(void)           __attribute__((weak, alias("DefaultISR")));
void SysTick_Handler(void)       __attribute__((weak, alias("DefaultISR")));

/* ==========================================================================
 * Peripheral IRQ handlers — weak aliases for IRQ 0–122.
 *
 * Naming follows the NXP SDK convention.  Override by defining the same
 * function name in user code.
 * ========================================================================= */
#define IRQ_WEAK(name) \
    void name(void) __attribute__((weak, alias("DefaultISR")))

IRQ_WEAK(DMA0_IRQHandler);
IRQ_WEAK(DMA1_IRQHandler);
IRQ_WEAK(DMA2_IRQHandler);
IRQ_WEAK(DMA3_IRQHandler);
IRQ_WEAK(DMA4_IRQHandler);
IRQ_WEAK(DMA5_IRQHandler);
IRQ_WEAK(DMA6_IRQHandler);
IRQ_WEAK(DMA7_IRQHandler);
IRQ_WEAK(DMA8_IRQHandler);
IRQ_WEAK(DMA9_IRQHandler);
IRQ_WEAK(DMA10_IRQHandler);
IRQ_WEAK(DMA11_IRQHandler);
IRQ_WEAK(DMA12_IRQHandler);
IRQ_WEAK(DMA13_IRQHandler);
IRQ_WEAK(DMA14_IRQHandler);
IRQ_WEAK(DMA15_IRQHandler);
IRQ_WEAK(DMA_Error_IRQHandler);
IRQ_WEAK(MCM_IRQHandler);
IRQ_WEAK(FTFC_IRQHandler);
IRQ_WEAK(Read_Collision_IRQHandler);
IRQ_WEAK(LVD_LVW_IRQHandler);
IRQ_WEAK(FTFC_Fault_IRQHandler);
IRQ_WEAK(WDOG_EWM_IRQHandler);
IRQ_WEAK(RCM_IRQHandler);
IRQ_WEAK(LPI2C0_Master_IRQHandler);
IRQ_WEAK(LPI2C0_Slave_IRQHandler);
IRQ_WEAK(LPSPI0_IRQHandler);
IRQ_WEAK(LPSPI1_IRQHandler);
IRQ_WEAK(LPSPI2_IRQHandler);
IRQ_WEAK(Reserved45_IRQHandler);
IRQ_WEAK(Reserved46_IRQHandler);
IRQ_WEAK(LPUART0_RxTx_IRQHandler);
IRQ_WEAK(Reserved48_IRQHandler);
IRQ_WEAK(LPUART1_RxTx_IRQHandler);
IRQ_WEAK(Reserved50_IRQHandler);
IRQ_WEAK(LPUART2_RxTx_IRQHandler);
IRQ_WEAK(Reserved52_IRQHandler);
IRQ_WEAK(Reserved53_IRQHandler);
IRQ_WEAK(Reserved54_IRQHandler);
IRQ_WEAK(ADC0_IRQHandler);
IRQ_WEAK(ADC1_IRQHandler);
IRQ_WEAK(CMP0_IRQHandler);
IRQ_WEAK(Reserved58_IRQHandler);
IRQ_WEAK(Reserved59_IRQHandler);
IRQ_WEAK(ERM_single_fault_IRQHandler);
IRQ_WEAK(ERM_double_fault_IRQHandler);
IRQ_WEAK(RTC_IRQHandler);
IRQ_WEAK(RTC_Seconds_IRQHandler);
IRQ_WEAK(LPIT0_Ch0_IRQHandler);
IRQ_WEAK(LPIT0_Ch1_IRQHandler);
IRQ_WEAK(LPIT0_Ch2_IRQHandler);
IRQ_WEAK(LPIT0_Ch3_IRQHandler);
IRQ_WEAK(PDB0_IRQHandler);
IRQ_WEAK(Reserved69_IRQHandler);
IRQ_WEAK(Reserved70_IRQHandler);
IRQ_WEAK(Reserved71_IRQHandler);
IRQ_WEAK(Reserved72_IRQHandler);
IRQ_WEAK(SCG_IRQHandler);
IRQ_WEAK(LPTMR0_IRQHandler);
IRQ_WEAK(PORTA_IRQHandler);
IRQ_WEAK(PORTB_IRQHandler);
IRQ_WEAK(PORTC_IRQHandler);
IRQ_WEAK(PORTD_IRQHandler);
IRQ_WEAK(PORTE_IRQHandler);
IRQ_WEAK(SWI_IRQHandler);
IRQ_WEAK(Reserved81_IRQHandler);
IRQ_WEAK(Reserved82_IRQHandler);
IRQ_WEAK(Reserved83_IRQHandler);
IRQ_WEAK(PDB1_IRQHandler);
IRQ_WEAK(FLEXIO_IRQHandler);
IRQ_WEAK(Reserved86_IRQHandler);
IRQ_WEAK(Reserved87_IRQHandler);
IRQ_WEAK(Reserved88_IRQHandler);
IRQ_WEAK(Reserved89_IRQHandler);
IRQ_WEAK(Reserved90_IRQHandler);
IRQ_WEAK(Reserved91_IRQHandler);
IRQ_WEAK(Reserved92_IRQHandler);
IRQ_WEAK(Reserved93_IRQHandler);
IRQ_WEAK(CAN0_ORed_IRQHandler);
IRQ_WEAK(CAN0_Error_IRQHandler);
IRQ_WEAK(CAN0_Wake_Up_IRQHandler);
IRQ_WEAK(CAN0_ORed_0_15_MB_IRQHandler);
IRQ_WEAK(CAN0_ORed_16_31_MB_IRQHandler);
IRQ_WEAK(Reserved99_IRQHandler);
IRQ_WEAK(Reserved100_IRQHandler);
IRQ_WEAK(CAN1_ORed_IRQHandler);
IRQ_WEAK(CAN1_Error_IRQHandler);
IRQ_WEAK(Reserved103_IRQHandler);
IRQ_WEAK(CAN1_ORed_0_15_MB_IRQHandler);
IRQ_WEAK(Reserved105_IRQHandler);
IRQ_WEAK(Reserved106_IRQHandler);
IRQ_WEAK(Reserved107_IRQHandler);
IRQ_WEAK(CAN2_ORed_IRQHandler);
IRQ_WEAK(CAN2_Error_IRQHandler);
IRQ_WEAK(Reserved110_IRQHandler);
IRQ_WEAK(CAN2_ORed_0_15_MB_IRQHandler);
IRQ_WEAK(Reserved112_IRQHandler);
IRQ_WEAK(Reserved113_IRQHandler);
IRQ_WEAK(Reserved114_IRQHandler);
IRQ_WEAK(FTM0_Ch0_Ch1_IRQHandler);
IRQ_WEAK(FTM0_Ch2_Ch3_IRQHandler);
IRQ_WEAK(FTM0_Ch4_Ch5_IRQHandler);
IRQ_WEAK(FTM0_Ch6_Ch7_IRQHandler);
IRQ_WEAK(FTM0_Fault_IRQHandler);
IRQ_WEAK(FTM0_Ovf_Reload_IRQHandler);
IRQ_WEAK(FTM1_Ch0_Ch1_IRQHandler);
IRQ_WEAK(FTM1_Ch2_Ch3_IRQHandler);
IRQ_WEAK(FTM1_Ch4_Ch5_IRQHandler);
IRQ_WEAK(FTM1_Ch6_Ch7_IRQHandler);
IRQ_WEAK(FTM1_Fault_IRQHandler);
IRQ_WEAK(FTM1_Ovf_Reload_IRQHandler);
IRQ_WEAK(FTM2_Ch0_Ch1_IRQHandler);
IRQ_WEAK(FTM2_Ch2_Ch3_IRQHandler);
IRQ_WEAK(FTM2_Ch4_Ch5_IRQHandler);
IRQ_WEAK(FTM2_Ch6_Ch7_IRQHandler);
IRQ_WEAK(FTM2_Fault_IRQHandler);
IRQ_WEAK(FTM2_Ovf_Reload_IRQHandler);
IRQ_WEAK(FTM3_Ch0_Ch1_IRQHandler);
IRQ_WEAK(FTM3_Ch2_Ch3_IRQHandler);
IRQ_WEAK(FTM3_Ch4_Ch5_IRQHandler);
IRQ_WEAK(FTM3_Ch6_Ch7_IRQHandler);
IRQ_WEAK(FTM3_Fault_IRQHandler);
IRQ_WEAK(FTM3_Ovf_Reload_IRQHandler);

#undef IRQ_WEAK

/* ==========================================================================
 * __isr_vector — Cortex-M4 Vector Table
 *
 * Size: 256 × 4 bytes = 1024 bytes, fitting exactly in m_interrupts.
 * Placed in .isr_vector per NXP convention.
 *
 * Format:
 *   [0]      Initial SP (loaded by hardware on reset)
 *   [1]      Reset_Handler
 *   [2–15]   System exceptions (NMI … SysTick)
 *   [16–255] External interrupts (IRQ 0–239)
 * ========================================================================= */
__attribute__((section(".isr_vector"), used, aligned(4)))
const uint32_t __isr_vector[256] =
{
    /* --- Initial Stack Pointer --- */
    (uint32_t)&__StackTop,

    /* --- System Exceptions (indices 1–15) --- */
    (uint32_t)Reset_Handler,            /*  1: Reset                     */
    (uint32_t)NMI_Handler,              /*  2: NMI                       */
    (uint32_t)HardFault_Handler,        /*  3: HardFault                 */
    (uint32_t)MemManage_Handler,        /*  4: MemManage                 */
    (uint32_t)BusFault_Handler,         /*  5: BusFault                  */
    (uint32_t)UsageFault_Handler,       /*  6: UsageFault                */
    0U,                                 /*  7: Reserved                  */
    0U,                                 /*  8: Reserved                  */
    0U,                                 /*  9: Reserved                  */
    0U,                                 /* 10: Reserved                  */
    (uint32_t)SVC_Handler,              /* 11: SVCall                    */
    (uint32_t)DebugMon_Handler,         /* 12: Debug Monitor             */
    0U,                                 /* 13: Reserved                  */
    (uint32_t)PendSV_Handler,           /* 14: PendSV                    */
    (uint32_t)SysTick_Handler,          /* 15: SysTick                   */

    /* --- External Interrupts 0–31 (indices 16–47) --- */
    (uint32_t)DMA0_IRQHandler,          /* IRQ  0 */
    (uint32_t)DMA1_IRQHandler,          /* IRQ  1 */
    (uint32_t)DMA2_IRQHandler,          /* IRQ  2 */
    (uint32_t)DMA3_IRQHandler,          /* IRQ  3 */
    (uint32_t)DMA4_IRQHandler,          /* IRQ  4 */
    (uint32_t)DMA5_IRQHandler,          /* IRQ  5 */
    (uint32_t)DMA6_IRQHandler,          /* IRQ  6 */
    (uint32_t)DMA7_IRQHandler,          /* IRQ  7 */
    (uint32_t)DMA8_IRQHandler,          /* IRQ  8 */
    (uint32_t)DMA9_IRQHandler,          /* IRQ  9 */
    (uint32_t)DMA10_IRQHandler,         /* IRQ 10 */
    (uint32_t)DMA11_IRQHandler,         /* IRQ 11 */
    (uint32_t)DMA12_IRQHandler,         /* IRQ 12 */
    (uint32_t)DMA13_IRQHandler,         /* IRQ 13 */
    (uint32_t)DMA14_IRQHandler,         /* IRQ 14 */
    (uint32_t)DMA15_IRQHandler,         /* IRQ 15 */
    (uint32_t)DMA_Error_IRQHandler,     /* IRQ 16 */
    (uint32_t)MCM_IRQHandler,           /* IRQ 17 */
    (uint32_t)FTFC_IRQHandler,          /* IRQ 18 */
    (uint32_t)Read_Collision_IRQHandler,/* IRQ 19 */
    (uint32_t)LVD_LVW_IRQHandler,       /* IRQ 20 */
    (uint32_t)FTFC_Fault_IRQHandler,    /* IRQ 21 */
    (uint32_t)WDOG_EWM_IRQHandler,      /* IRQ 22 */
    (uint32_t)RCM_IRQHandler,           /* IRQ 23 */
    (uint32_t)LPI2C0_Master_IRQHandler, /* IRQ 24 */
    (uint32_t)LPI2C0_Slave_IRQHandler,  /* IRQ 25 */
    (uint32_t)LPSPI0_IRQHandler,        /* IRQ 26 */
    (uint32_t)LPSPI1_IRQHandler,        /* IRQ 27 */
    (uint32_t)LPSPI2_IRQHandler,        /* IRQ 28 */
    (uint32_t)Reserved45_IRQHandler,    /* IRQ 29 */
    (uint32_t)Reserved46_IRQHandler,    /* IRQ 30 */
    (uint32_t)LPUART0_RxTx_IRQHandler,  /* IRQ 31 */

    /* --- External Interrupts 32–63 (indices 48–79) --- */
    (uint32_t)Reserved48_IRQHandler,    /* IRQ 32 */
    (uint32_t)LPUART1_RxTx_IRQHandler,  /* IRQ 33 */
    (uint32_t)Reserved50_IRQHandler,    /* IRQ 34 */
    (uint32_t)LPUART2_RxTx_IRQHandler,  /* IRQ 35 */
    (uint32_t)Reserved52_IRQHandler,    /* IRQ 36 */
    (uint32_t)Reserved53_IRQHandler,    /* IRQ 37 */
    (uint32_t)Reserved54_IRQHandler,    /* IRQ 38 */
    (uint32_t)ADC0_IRQHandler,          /* IRQ 39 */
    (uint32_t)ADC1_IRQHandler,          /* IRQ 40 */
    (uint32_t)CMP0_IRQHandler,          /* IRQ 41 */
    (uint32_t)Reserved58_IRQHandler,    /* IRQ 42 */
    (uint32_t)Reserved59_IRQHandler,    /* IRQ 43 */
    (uint32_t)ERM_single_fault_IRQHandler,/* IRQ 44 */
    (uint32_t)ERM_double_fault_IRQHandler,/* IRQ 45 */
    (uint32_t)RTC_IRQHandler,           /* IRQ 46 */
    (uint32_t)RTC_Seconds_IRQHandler,   /* IRQ 47 */
    (uint32_t)LPIT0_Ch0_IRQHandler,     /* IRQ 48 */
    (uint32_t)LPIT0_Ch1_IRQHandler,     /* IRQ 49 */
    (uint32_t)LPIT0_Ch2_IRQHandler,     /* IRQ 50 */
    (uint32_t)LPIT0_Ch3_IRQHandler,     /* IRQ 51 */
    (uint32_t)PDB0_IRQHandler,          /* IRQ 52 */
    (uint32_t)Reserved69_IRQHandler,    /* IRQ 53 */
    (uint32_t)Reserved70_IRQHandler,    /* IRQ 54 */
    (uint32_t)Reserved71_IRQHandler,    /* IRQ 55 */
    (uint32_t)Reserved72_IRQHandler,    /* IRQ 56 */
    (uint32_t)SCG_IRQHandler,           /* IRQ 57 */
    (uint32_t)LPTMR0_IRQHandler,        /* IRQ 58 */
    (uint32_t)PORTA_IRQHandler,         /* IRQ 59 */
    (uint32_t)PORTB_IRQHandler,         /* IRQ 60 */
    (uint32_t)PORTC_IRQHandler,         /* IRQ 61 */
    (uint32_t)PORTD_IRQHandler,         /* IRQ 62 */
    (uint32_t)PORTE_IRQHandler,         /* IRQ 63 */

    /* --- External Interrupts 64–95 (indices 80–111) --- */
    (uint32_t)SWI_IRQHandler,           /* IRQ 64 */
    (uint32_t)Reserved81_IRQHandler,    /* IRQ 65 */
    (uint32_t)Reserved82_IRQHandler,    /* IRQ 66 */
    (uint32_t)Reserved83_IRQHandler,    /* IRQ 67 */
    (uint32_t)PDB1_IRQHandler,          /* IRQ 68 */
    (uint32_t)FLEXIO_IRQHandler,        /* IRQ 69 */
    (uint32_t)Reserved86_IRQHandler,    /* IRQ 70 */
    (uint32_t)Reserved87_IRQHandler,    /* IRQ 71 */
    (uint32_t)Reserved88_IRQHandler,    /* IRQ 72 */
    (uint32_t)Reserved89_IRQHandler,    /* IRQ 73 */
    (uint32_t)Reserved90_IRQHandler,    /* IRQ 74 */
    (uint32_t)Reserved91_IRQHandler,    /* IRQ 75 */
    (uint32_t)Reserved92_IRQHandler,    /* IRQ 76 */
    (uint32_t)Reserved93_IRQHandler,    /* IRQ 77 */
    (uint32_t)CAN0_ORed_IRQHandler,     /* IRQ 78 */
    (uint32_t)CAN0_Error_IRQHandler,    /* IRQ 79 */
    (uint32_t)CAN0_Wake_Up_IRQHandler,  /* IRQ 80 */
    (uint32_t)CAN0_ORed_0_15_MB_IRQHandler,/* IRQ 81 */
    (uint32_t)CAN0_ORed_16_31_MB_IRQHandler,/* IRQ 82 */
    (uint32_t)Reserved99_IRQHandler,    /* IRQ 83 */
    (uint32_t)Reserved100_IRQHandler,   /* IRQ 84 */
    (uint32_t)CAN1_ORed_IRQHandler,     /* IRQ 85 */
    (uint32_t)CAN1_Error_IRQHandler,    /* IRQ 86 */
    (uint32_t)Reserved103_IRQHandler,   /* IRQ 87 */
    (uint32_t)CAN1_ORed_0_15_MB_IRQHandler,/* IRQ 88 */
    (uint32_t)Reserved105_IRQHandler,   /* IRQ 89 */
    (uint32_t)Reserved106_IRQHandler,   /* IRQ 90 */
    (uint32_t)Reserved107_IRQHandler,   /* IRQ 91 */
    (uint32_t)CAN2_ORed_IRQHandler,     /* IRQ 92 */
    (uint32_t)CAN2_Error_IRQHandler,    /* IRQ 93 */
    (uint32_t)Reserved110_IRQHandler,   /* IRQ 94 */
    (uint32_t)CAN2_ORed_0_15_MB_IRQHandler,/* IRQ 95 */

    /* --- External Interrupts 96–122 (indices 112–138) --- */
    (uint32_t)Reserved112_IRQHandler,   /* IRQ 96 */
    (uint32_t)Reserved113_IRQHandler,   /* IRQ 97 */
    (uint32_t)Reserved114_IRQHandler,   /* IRQ 98 */
    (uint32_t)FTM0_Ch0_Ch1_IRQHandler,  /* IRQ 99 */
    (uint32_t)FTM0_Ch2_Ch3_IRQHandler,  /* IRQ 100 */
    (uint32_t)FTM0_Ch4_Ch5_IRQHandler,  /* IRQ 101 */
    (uint32_t)FTM0_Ch6_Ch7_IRQHandler,  /* IRQ 102 */
    (uint32_t)FTM0_Fault_IRQHandler,    /* IRQ 103 */
    (uint32_t)FTM0_Ovf_Reload_IRQHandler,/* IRQ 104 */
    (uint32_t)FTM1_Ch0_Ch1_IRQHandler,  /* IRQ 105 */
    (uint32_t)FTM1_Ch2_Ch3_IRQHandler,  /* IRQ 106 */
    (uint32_t)FTM1_Ch4_Ch5_IRQHandler,  /* IRQ 107 */
    (uint32_t)FTM1_Ch6_Ch7_IRQHandler,  /* IRQ 108 */
    (uint32_t)FTM1_Fault_IRQHandler,    /* IRQ 109 */
    (uint32_t)FTM1_Ovf_Reload_IRQHandler,/* IRQ 110 */
    (uint32_t)FTM2_Ch0_Ch1_IRQHandler,  /* IRQ 111 */
    (uint32_t)FTM2_Ch2_Ch3_IRQHandler,  /* IRQ 112 */
    (uint32_t)FTM2_Ch4_Ch5_IRQHandler,  /* IRQ 113 */
    (uint32_t)FTM2_Ch6_Ch7_IRQHandler,  /* IRQ 114 */
    (uint32_t)FTM2_Fault_IRQHandler,    /* IRQ 115 */
    (uint32_t)FTM2_Ovf_Reload_IRQHandler,/* IRQ 116 */
    (uint32_t)FTM3_Ch0_Ch1_IRQHandler,  /* IRQ 117 */
    (uint32_t)FTM3_Ch2_Ch3_IRQHandler,  /* IRQ 118 */
    (uint32_t)FTM3_Ch4_Ch5_IRQHandler,  /* IRQ 119 */
    (uint32_t)FTM3_Ch6_Ch7_IRQHandler,  /* IRQ 120 */
    (uint32_t)FTM3_Fault_IRQHandler,    /* IRQ 121 */
    (uint32_t)FTM3_Ovf_Reload_IRQHandler,/* IRQ 122 */

    /* --- External Interrupts 123–239 (indices 139–255) ---
     *     All default to DefaultISR.  These cover reserved and unused
     *     interrupt slots on the S32K144.  The ISR name for each is
     *     auto-generated by IRQ_WEAK above and points to DefaultISR.   */
    [139] = (uint32_t)DefaultISR,    [140] = (uint32_t)DefaultISR,
    [141] = (uint32_t)DefaultISR,    [142] = (uint32_t)DefaultISR,
    [143] = (uint32_t)DefaultISR,    [144] = (uint32_t)DefaultISR,
    [145] = (uint32_t)DefaultISR,    [146] = (uint32_t)DefaultISR,
    [147] = (uint32_t)DefaultISR,    [148] = (uint32_t)DefaultISR,
    [149] = (uint32_t)DefaultISR,    [150] = (uint32_t)DefaultISR,
    [151] = (uint32_t)DefaultISR,    [152] = (uint32_t)DefaultISR,
    [153] = (uint32_t)DefaultISR,    [154] = (uint32_t)DefaultISR,
    [155] = (uint32_t)DefaultISR,    [156] = (uint32_t)DefaultISR,
    [157] = (uint32_t)DefaultISR,    [158] = (uint32_t)DefaultISR,
    [159] = (uint32_t)DefaultISR,    [160] = (uint32_t)DefaultISR,
    [161] = (uint32_t)DefaultISR,    [162] = (uint32_t)DefaultISR,
    [163] = (uint32_t)DefaultISR,    [164] = (uint32_t)DefaultISR,
    [165] = (uint32_t)DefaultISR,    [166] = (uint32_t)DefaultISR,
    [167] = (uint32_t)DefaultISR,    [168] = (uint32_t)DefaultISR,
    [169] = (uint32_t)DefaultISR,    [170] = (uint32_t)DefaultISR,
    [171] = (uint32_t)DefaultISR,    [172] = (uint32_t)DefaultISR,
    [173] = (uint32_t)DefaultISR,    [174] = (uint32_t)DefaultISR,
    [175] = (uint32_t)DefaultISR,    [176] = (uint32_t)DefaultISR,
    [177] = (uint32_t)DefaultISR,    [178] = (uint32_t)DefaultISR,
    [179] = (uint32_t)DefaultISR,    [180] = (uint32_t)DefaultISR,
    [181] = (uint32_t)DefaultISR,    [182] = (uint32_t)DefaultISR,
    [183] = (uint32_t)DefaultISR,    [184] = (uint32_t)DefaultISR,
    [185] = (uint32_t)DefaultISR,    [186] = (uint32_t)DefaultISR,
    [187] = (uint32_t)DefaultISR,    [188] = (uint32_t)DefaultISR,
    [189] = (uint32_t)DefaultISR,    [190] = (uint32_t)DefaultISR,
    [191] = (uint32_t)DefaultISR,    [192] = (uint32_t)DefaultISR,
    [193] = (uint32_t)DefaultISR,    [194] = (uint32_t)DefaultISR,
    [195] = (uint32_t)DefaultISR,    [196] = (uint32_t)DefaultISR,
    [197] = (uint32_t)DefaultISR,    [198] = (uint32_t)DefaultISR,
    [199] = (uint32_t)DefaultISR,    [200] = (uint32_t)DefaultISR,
    [201] = (uint32_t)DefaultISR,    [202] = (uint32_t)DefaultISR,
    [203] = (uint32_t)DefaultISR,    [204] = (uint32_t)DefaultISR,
    [205] = (uint32_t)DefaultISR,    [206] = (uint32_t)DefaultISR,
    [207] = (uint32_t)DefaultISR,    [208] = (uint32_t)DefaultISR,
    [209] = (uint32_t)DefaultISR,    [210] = (uint32_t)DefaultISR,
    [211] = (uint32_t)DefaultISR,    [212] = (uint32_t)DefaultISR,
    [213] = (uint32_t)DefaultISR,    [214] = (uint32_t)DefaultISR,
    [215] = (uint32_t)DefaultISR,    [216] = (uint32_t)DefaultISR,
    [217] = (uint32_t)DefaultISR,    [218] = (uint32_t)DefaultISR,
    [219] = (uint32_t)DefaultISR,    [220] = (uint32_t)DefaultISR,
    [221] = (uint32_t)DefaultISR,    [222] = (uint32_t)DefaultISR,
    [223] = (uint32_t)DefaultISR,    [224] = (uint32_t)DefaultISR,
    [225] = (uint32_t)DefaultISR,    [226] = (uint32_t)DefaultISR,
    [227] = (uint32_t)DefaultISR,    [228] = (uint32_t)DefaultISR,
    [229] = (uint32_t)DefaultISR,    [230] = (uint32_t)DefaultISR,
    [231] = (uint32_t)DefaultISR,    [232] = (uint32_t)DefaultISR,
    [233] = (uint32_t)DefaultISR,    [234] = (uint32_t)DefaultISR,
    [235] = (uint32_t)DefaultISR,    [236] = (uint32_t)DefaultISR,
    [237] = (uint32_t)DefaultISR,    [238] = (uint32_t)DefaultISR,
    [239] = (uint32_t)DefaultISR,    [240] = (uint32_t)DefaultISR,
    [241] = (uint32_t)DefaultISR,    [242] = (uint32_t)DefaultISR,
    [243] = (uint32_t)DefaultISR,    [244] = (uint32_t)DefaultISR,
    [245] = (uint32_t)DefaultISR,    [246] = (uint32_t)DefaultISR,
    [247] = (uint32_t)DefaultISR,    [248] = (uint32_t)DefaultISR,
    [249] = (uint32_t)DefaultISR,    [250] = (uint32_t)DefaultISR,
    [251] = (uint32_t)DefaultISR,    [252] = (uint32_t)DefaultISR,
    [253] = (uint32_t)DefaultISR,    [254] = (uint32_t)DefaultISR,
    [255] = (uint32_t)DefaultISR,     /* IRQ 239 */
};

/* ==========================================================================
 * Reset_Handler — C entry point after hardware reset.
 *
 * The Cortex-M4 hardware automatically:
 *   1. Reads __StackTop from address 0x00000000 → MSP
 *   2. Reads Reset_Handler from address 0x00000004 → PC
 *   3. Starts executing in Thread mode with MSP
 *
 * This handler completes system initialization and calls main().
 * ========================================================================= */
__attribute__((noreturn)) void Reset_Handler(void)
{
    /* 1. Mask interrupts during critical init */
    __asm__ volatile("cpsid i" : : : "memory");

#if defined(START_FROM_FLASH) && defined(ECC_RAM_INIT)
    /* 2. Initialize ECC SRAM by writing zeros */
    {
        volatile uint32_t *p = &__RAM_START;
        volatile uint32_t *e = &__RAM_END;
        while (p < e) { *p++ = 0U; }
    }
#endif

    /* 3. Reload MSP (hardware already did this, but be safe) */
    __asm__ volatile(
        "ldr r0, =__StackTop \n"
        "msr msp, r0         \n" : : : "r0");

    /* 4. SoC-level init: enable FPU, disable watchdog */
    SystemInit();

    /* 5. Copy .data from Flash to SRAM and zero .bss */
    init_data_bss();

    /* 6. Unmask interrupts */
    __asm__ volatile("cpsie i" : : : "memory");

    /* 7. Jump to application */
    main();

    /* Not reached */
    while (1) { }
}

/* ==========================================================================
 * Flash Configuration Field (FCF)
 *
 * Placed at address 0x400 in Flash.  The S32K144 reads this on reset
 * to determine flash security and protection settings.
 *
 *   Bytes 0-7:  Backdoor Comparison Key (all 0xFF = disabled)
 *   Bytes 8-11: Flash Protection (FDPROT:FEPROT:FOPT:FSEC)
 *               0xFFFF7FFE = unprotected, unsecured (FSEC=0xFE)
 * ========================================================================= */
__attribute__((section(".FlashConfig"), used, aligned(4)))
const uint32_t __flash_config[4] = {
    0xFFFFFFFFU,  /* Backdoor Key [31:0]  */
    0xFFFFFFFFU,  /* Backdoor Key [63:32] */
    0xFFFFFFFFU,  /* FDPROT:FEPROT:FOPT:FSEC (flash protection) */
    0xFFFF7FFEU,  /* FSEC=0xFE → unsecured */
};

/* ==========================================================================
 * init_data_bss — Initialize memory sections.
 * ========================================================================= */
static void init_data_bss(void)
{
    uint32_t *src;
    uint32_t *dst;
    uint32_t *end;

    /* Copy .data from Flash LMA to SRAM VMA */
    src = &__DATA_ROM;
    dst = &__data_start__;
    end = &__data_end__;
    while (dst < end) { *dst++ = *src++; }

    /* Zero .bss */
    dst = &__bss_start__;
    end = &__bss_end__;
    while (dst < end) { *dst++ = 0U; }
}

/* ==========================================================================
 * SystemInit — Weak SoC initialization.
 *
 * Override by linking with the NXP SDK's system_S32K144.c, or customize
 * with compile-time defines:
 *   -DENABLE_FPU=1 / 0   (default: 1)
 *   -DDISABLE_WDOG=1 / 0 (default: 1)
 * ========================================================================= */
#ifndef ENABLE_FPU
#define ENABLE_FPU 1
#endif
#ifndef DISABLE_WDOG
#define DISABLE_WDOG 1
#endif

__attribute__((weak)) void SystemInit(void)
{
#if ENABLE_FPU
    /* Enable CP10 and CP11 for full FPU access */
    SCB->CPACR |= (SCB_CPACR_CP10_MASK | SCB_CPACR_CP11_MASK);
#endif

#if DISABLE_WDOG
    /* Unlock watchdog */
    WDOG->CNT = 0xD928C520U;
    (void)WDOG->CNT;  /* Dummy read to ensure write completes */

    /* Disable watchdog: 32-bit unlock, LPO clock, disabled, allow updates */
    WDOG->CS = (WDOG_CS_CMD32EN(1) |
                WDOG_CS_CLK(2)     |
                WDOG_CS_EN(0)      |
                WDOG_CS_UPDATE(1));

    WDOG->TOVAL = 0xFFFFU;
#endif
}
