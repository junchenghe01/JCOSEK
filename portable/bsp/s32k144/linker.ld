/**
 * @file    linker.ld
 * @brief   Linker script for NXP S32K144 (64 KB SRAM variant).
 *
 * @details Memory layout follows the NXP SDK S32K144_64_flash.ld:
 *
 *          Flash (512 KB total):
 *            0x00000000 - 0x00000400  m_interrupts    Vector table (1 KB)
 *            0x00000400 - 0x00000410  m_flash_config  FOPT/FSEC (16 B)
 *            0x00000410 - 0x00080000  m_text          Code + RO data (~511 KB)
 *
 *          SRAM_L (64 KB total, lower 32 KB used):
 *            0x1FFF8000 - 0x20000000  m_data          .data section (32 KB)
 *
 *          SRAM_U (64 KB total, lower 28 KB used):
 *            0x20000000 - 0x20007000  m_data_2        .bss + heap + stack (28 KB)
 *
 *          Stack is placed at the top of m_data_2 (0x20007000).
 */

ENTRY(Reset_Handler)

/* --------------------------------------------------------------------------
 * Configurable sizes (can be overridden via -defsym)
 * -------------------------------------------------------------------------- */
HEAP_SIZE  = DEFINED(__heap_size__)  ? __heap_size__  : 0x00000400;
STACK_SIZE = DEFINED(__stack_size__) ? __stack_size__ : 0x00001000; /* 4 KB for ISR/system stack */

/* If __flash_vector_table__=1, vector table stays in Flash (read-only).
 * Otherwise, space is reserved in SRAM for a runtime copy. */
M_VECTOR_RAM_SIZE = DEFINED(__flash_vector_table__) ? 0x0 : 0x0000; /* No RAM vector table by default */

/* --------------------------------------------------------------------------
 * Memory regions
 * -------------------------------------------------------------------------- */
MEMORY
{
    m_interrupts   (RX)  : ORIGIN = 0x00000000, LENGTH = 0x00000400
    m_flash_config (RX)  : ORIGIN = 0x00000400, LENGTH = 0x00000010
    m_text         (RX)  : ORIGIN = 0x00000410, LENGTH = 0x0007FBF0

    m_data         (RW)  : ORIGIN = 0x1FFF8000, LENGTH = 0x00008000  /* SRAM_L, 32 KB */
    m_data_2       (RW)  : ORIGIN = 0x20000000, LENGTH = 0x00007000  /* SRAM_U, 28 KB */
}

/* --------------------------------------------------------------------------
 * Output sections
 * -------------------------------------------------------------------------- */
SECTIONS
{
    /* ===== Vector table (must be first in Flash) ===== */
    .interrupts :
    {
        __VECTOR_TABLE = .;
        __interrupts_start__ = .;
        . = ALIGN(4);
        KEEP(*(.isr_vector))        /* Vector table from startup.c */
        __interrupts_end__ = .;
        . = ALIGN(4);
    } > m_interrupts

    /* ===== Flash Configuration Field (FCF) ===== */
    .flash_config :
    {
        . = ALIGN(4);
        KEEP(*(.FlashConfig))       /* FOPT/FSEC bytes */
        . = ALIGN(4);
    } > m_flash_config

    /* ===== Code and read-only data ===== */
    .text :
    {
        . = ALIGN(4);
        *(.text)                    /* Program code */
        *(.text*)                   /* Program code (linker-generated) */
        *(.rodata)                  /* Read-only data */
        *(.rodata*)                 /* Read-only data (linker-generated) */
        *(.init)                    /* crti.o sections */
        *(.fini)                    /* crti.o sections */
        *(.eh_frame)               /* Exception handling frames */
        . = ALIGN(4);
    } > m_text

    /* ARM exception index table (required by libgcc for unwind) */
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > m_text

    /* End of ROM image */
    __etext = .;
    __DATA_ROM = .;

    /* ===== Initialized data (.data) — copied from Flash to SRAM at startup ===== */
    .data : AT(__DATA_ROM)
    {
        . = ALIGN(4);
        __DATA_RAM = .;
        __data_start__ = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > m_data

    __DATA_END = __DATA_ROM + (__data_end__ - __data_start__);

    /* ===== Uninitialized data (.bss) — zeroed at startup ===== */
    .bss :
    {
        . = ALIGN(4);
        __BSS_START = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        __BSS_END = .;
    } > m_data_2

    /* ===== No-init section (preserved across resets) ===== */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        __noinit_start = .;
        *(.noinit)
        *(.noinit*)
        __noinit_end = .;
    } > m_data_2

    /* ===== Heap (optional) ===== */
    .heap :
    {
        . = ALIGN(8);
        __end__ = .;
        __heap_start__ = .;
        PROVIDE(end = .);
        PROVIDE(_end = .);
        PROVIDE(__end = .);
        __HeapBase = .;
        . += HEAP_SIZE;
        __HeapLimit = .;
        __heap_limit = .;
        __heap_end__ = .;
    } > m_data_2

    /* ===== ISR / System stack (MSP) =====
     * Placed at the top of m_data_2.  MSP is loaded with __StackTop
     * before calling main(), and the ISR / system stack grows downward
     * from this address. */
    __StackTop   = ORIGIN(m_data_2) + LENGTH(m_data_2);
    __StackLimit = __StackTop - STACK_SIZE;
    PROVIDE(__stack = __StackTop);
    __RAM_END = __StackTop;

    .stack __StackLimit :
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . += STACK_SIZE;
        __stack_end__ = .;
    } > m_data_2

    /* ===== ARM attributes (required by some debuggers) ===== */
    .ARM.attributes 0 : { *(.ARM.attributes) }

    /* ===== Memory validation ===== */
    ASSERT(__DATA_END <= (ORIGIN(m_text) + LENGTH(m_text)),
           "Flash overflow: .text + .data exceed m_text region!")
    ASSERT(__StackLimit >= __HeapLimit,
           "SRAM_U overflow: stack and heap overlap!")
}
