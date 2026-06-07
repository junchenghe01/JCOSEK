/**
 * @file    S32K144.h
 * @brief   Minimal register definitions for NXP S32K144 (Cortex-M4F).
 *
 * @details Provides only the registers needed by the JCOSEK kernel and
 *          startup code.  For full peripheral access, link with the NXP
 *          SDK device header (S32K144.h in platform/devices/).
 *
 *          This file is self-contained — no dependency on CMSIS or SDK.
 */

#ifndef S32K144_H_
#define S32K144_H_

#include <stdint.h>

/* ==========================================================================
 * ARM Cortex-M4 Core Peripherals (SCB, SysTick, NVIC)
 * ========================================================================= */

/* System Control Block base address */
#define SCB_BASE    (0xE000E008UL)

/* SCB registers used by the OS */
typedef struct
{
    volatile uint32_t ACTLR;        /* Offset 0x00: Auxiliary Control     */
    volatile uint32_t reserved0[829];
    volatile uint32_t CPUID;        /* Offset 0xD00: CPU ID               */
    volatile uint32_t ICSR;         /* Offset 0xD04: Int. Control State   */
    volatile uint32_t VTOR;         /* Offset 0xD08: Vector Table Offset  */
    volatile uint32_t AIRCR;        /* Offset 0xD0C: App. Int. Reset Ctrl.*/
    volatile uint32_t SCR;          /* Offset 0xD10: System Control       */
    volatile uint32_t CCR;          /* Offset 0xD14: Config. Control      */
    volatile uint32_t SHPR1;        /* Offset 0xD18: System Handlers 4-7  */
    volatile uint32_t SHPR2;        /* Offset 0xD1C: System Handlers 8-11 */
    volatile uint32_t SHPR3;        /* Offset 0xD20: System Handlers 12-15*/
    volatile uint32_t SHCRS;        /* Offset 0xD24: System Handler Ctrl  */
    volatile uint32_t CFSR;         /* Offset 0xD28: Config. Fault Status */
    volatile uint32_t HFSR;         /* Offset 0xD2C: HardFault Status     */
    volatile uint32_t DFSR;         /* Offset 0xD30: Debug Fault Status   */
    volatile uint32_t MMFAR;        /* Offset 0xD34: MemManage Fault Addr */
    volatile uint32_t BFAR;         /* Offset 0xD38: BusFault Address     */
    volatile uint32_t AFSR;         /* Offset 0xD3C: Aux. Fault Status    */
    volatile uint32_t reserved1[18];
    volatile uint32_t CPACR;        /* Offset 0xD88: Coprocessor Access   */
} SCB_Type;

#define SCB  ((SCB_Type *)SCB_BASE)

/* SCB_CPACR bit definitions */
#define SCB_CPACR_CP10_MASK  (0x3U << 20)
#define SCB_CPACR_CP11_MASK  (0x3U << 22)

/* SCB_ICSR bit definitions */
#define SCB_ICSR_PENDSVSET    (1UL << 28)
#define SCB_ICSR_PENDSVCLR    (1UL << 27)

/* SCB_AIRCR bit definitions */
#define SCB_AIRCR_VECTKEY     (0x05FAUL << 16)
#define SCB_AIRCR_SYSRESETREQ (1UL << 2)

/* ==========================================================================
 * Nested Vectored Interrupt Controller (NVIC)
 * ========================================================================= */

typedef struct
{
    volatile uint32_t ISER[8];      /* Interrupt Set-Enable             */
    volatile uint32_t reserved0[24];
    volatile uint32_t ICER[8];      /* Interrupt Clear-Enable           */
    volatile uint32_t reserved1[24];
    volatile uint32_t ISPR[8];      /* Interrupt Set-Pending            */
    volatile uint32_t reserved2[24];
    volatile uint32_t ICPR[8];      /* Interrupt Clear-Pending          */
    volatile uint32_t reserved3[24];
    volatile uint32_t IABR[8];      /* Interrupt Active Bit             */
    volatile uint32_t reserved4[56];
    volatile uint8_t  IP[240];      /* Interrupt Priority               */
    volatile uint32_t reserved5[644];
    volatile uint32_t STIR;         /* Software Trigger Interrupt       */
} NVIC_Type;

#define NVIC_BASE  (0xE000E100UL)
#define NVIC       ((NVIC_Type *)NVIC_BASE)

/* ==========================================================================
 * System Tick Timer (SysTick)
 * ========================================================================= */

typedef struct
{
    volatile uint32_t CSR;          /* Control and Status               */
    volatile uint32_t RVR;          /* Reload Value                     */
    volatile uint32_t CVR;          /* Current Value                    */
    volatile uint32_t CALIB;        /* Calibration                      */
} SysTick_Type;

#define SysTick_BASE (0xE000E010UL)
#define SysTick      ((SysTick_Type *)SysTick_BASE)

/* SysTick_CSR bits */
#define SysTick_CSR_ENABLE    (1UL << 0)
#define SysTick_CSR_TICKINT   (1UL << 1)
#define SysTick_CSR_CLKSOURCE (1UL << 2)
#define SysTick_CSR_COUNTFLAG (1UL << 16)

/* ==========================================================================
 * Watchdog Timer (WDOG)
 * ========================================================================= */

typedef struct
{
    volatile uint32_t CS;           /* Control and Status               */
    volatile uint32_t CNT;          /* Counter (write unlock sequence)  */
    volatile uint32_t TOVAL;        /* Timeout Value                    */
    volatile uint32_t WIN;          /* Window                           */
} WDOG_Type;

#define WDOG_BASE (0x40052000UL)
#define WDOG      ((WDOG_Type *)WDOG_BASE)

/* WDOG_CS bit shifts */
#define WDOG_CS_CMD32EN_SHIFT  (14)
#define WDOG_CS_CLK_SHIFT      (8)
#define WDOG_CS_EN_SHIFT       (7)
#define WDOG_CS_UPDATE_SHIFT   (5)

/* WDOG_CS field macros */
#define WDOG_CS_CMD32EN(v)  (((uint32_t)(v) & 0x1U) << WDOG_CS_CMD32EN_SHIFT)
#define WDOG_CS_CLK(v)      (((uint32_t)(v) & 0x3U) << WDOG_CS_CLK_SHIFT)
#define WDOG_CS_EN(v)       (((uint32_t)(v) & 0x1U) << WDOG_CS_EN_SHIFT)
#define WDOG_CS_UPDATE(v)   (((uint32_t)(v) & 0x1U) << WDOG_CS_UPDATE_SHIFT)

/* ==========================================================================
 * LPIT (Low-Power Interrupt Timer) — Channel 0 for system tick
 * ========================================================================= */

typedef struct
{
    volatile uint32_t VERID;        /* Version ID                        */
    volatile uint32_t PARAM;        /* Parameter                         */
    volatile uint32_t reserved0[2];
    volatile uint32_t MCR;          /* Module Control                    */
    volatile uint32_t MSR;          /* Module Status                     */
    volatile uint32_t MIER;         /* Module Interrupt Enable           */
    volatile uint32_t SETTEN;       /* Set Timer Enable                  */
    volatile uint32_t CLRTEN;       /* Clear Timer Enable                */
    volatile uint32_t reserved1[56];
    struct {
        volatile uint32_t TVAL;     /* Timer Value                       */
        volatile uint32_t CVAL;     /* Current Timer Value               */
        volatile uint32_t TCTRL;    /* Timer Control                     */
        volatile uint32_t reserved;
    } CHANNEL[4];
} LPIT_Type;

#define LPIT0_BASE  (0x40037000UL)
#define LPIT0       ((LPIT_Type *)LPIT0_BASE)

/* LPIT_MCR */
#define LPIT_MCR_M_CEN (1UL << 0)

/* LPIT_MSR */
#define LPIT_MSR_TIF0  (1UL << 0)

/* LPIT_TCTRL */
#define LPIT_TCTRL_T_EN  (1UL << 0)
#define LPIT_TCTRL_CHAIN (1UL << 1)
#define LPIT_TCTRL_MODE  (1UL << 2) /* 0=32-bit counter, 1=16-bit */

/* ==========================================================================
 * SCG (System Clock Generator) — minimal for clock info
 * ========================================================================= */

typedef struct
{
    volatile uint32_t reserved0[2];
    volatile uint32_t CSR;          /* Clock Status Register             */
    volatile uint32_t RCCR;         /* Run Clock Control Register        */
    volatile uint32_t VCCR;         /* VLPR Clock Control Register       */
    volatile uint32_t HCCR;         /* HSRUN Clock Control Register      */
    volatile uint32_t CLKOUTCNFG;   /* SCG CLKOUT Configuration          */
    volatile uint32_t reserved1[5];
    volatile uint32_t SOSCCFG;      /* System OSC Configuration          */
    volatile uint32_t SOSCDIV;      /* System OSC Clock Divide           */
    volatile uint32_t SIRCCFG;      /* Slow IRC Configuration            */
    volatile uint32_t SIRCDIV;      /* Slow IRC Clock Divide             */
    volatile uint32_t FIRCCFG;      /* Fast IRC Configuration            */
    volatile uint32_t FIRCDIV;      /* Fast IRC Clock Divide             */
    volatile uint32_t SPLLCFG;      /* System PLL Configuration          */
    volatile uint32_t SPLLDIV;      /* System PLL Clock Divide           */
    volatile uint32_t SPLLCSR;      /* System PLL Clock Status           */
} SCG_Type;

#define SCG_BASE  (0x40064000UL)
#define SCG       ((SCG_Type *)SCG_BASE)

/* SCG_CSR fields */
#define SCG_CSR_SCS_MASK    (0xF0U)
#define SCG_CSR_SCS_SHIFT   (4)
#define SCG_CSR_DIVCORE_MASK (0xF0000000U)
#define SCG_CSR_DIVCORE_SHIFT (28)

/* SCG_SIRCCFG */
#define SCG_SIRCCFG_RANGE_MASK  (0x1U)
#define SCG_SIRCCFG_RANGE_SHIFT (0)

/* SCG_FIRCCFG */
#define SCG_FIRCCFG_RANGE_MASK  (0x1U)
#define SCG_FIRCCFG_RANGE_SHIFT (0)

/* SCG_SPLLCFG */
#define SCG_SPLLCFG_PREDIV_MASK  (0xE0000U)
#define SCG_SPLLCFG_PREDIV_SHIFT (17)
#define SCG_SPLLCFG_MULT_MASK    (0x1F0000U)
#define SCG_SPLLCFG_MULT_SHIFT   (16)

#endif /* S32K144_H_ */
