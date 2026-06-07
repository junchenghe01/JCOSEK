# JCOSEK

An embedded real-time operating system kernel implemented based on the OSEK/VDX Operating System Specification (2.2.3).

## Supported Platforms

| Platform         | CPU          | Status   | Debugger        |
|------------------|-------------|----------|-----------------|
| VExpress-A9 QEMU | Cortex-A9   | ✅ Stable | GDB + QEMU      |
| NXP S32K144-Q100 | Cortex-M4F  | ✅ Ported | J-Link SWD      |
| STM32F103        | Cortex-M3   | 🚧 WIP   | —               |

## Quick Start

### Prerequisites

```bash
# ARM cross-compilation toolchain
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# For QEMU debugging
sudo apt install qemu-system-arm gdb-multiarch
```

### Build

#### S32K144 (NXP Evaluation Board)

```bash
mkdir -p build_s32k144 && cd build_s32k144
cmake -DTARGET_PLATFORM=s32k144 ..
make -j$(nproc)
# Output:
#   build_s32k144/JCOSEK_EXAMPLE.elf   — ELF executable
#   build_s32k144/JCOSEK_EXAMPLE.bin   — Raw binary
#   build_s32k144/JCOSEK_EXAMPLE.list  — objdump -S (disassembly with C source)
```

The `.list` file contains interleaved C source + assembly listing.  
Use it to verify struct offsets, check instruction selection, and debug low‑level issues.

#### VExpress-A9 QEMU (Default)

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
# Output: build/JCOSEK_EXAMPLE.elf
```

### Debug

#### S32K144 (J-Link)

```bash
# Terminal 1: Start J-Link GDB Server
JLinkGDBServer -device S32K144 -if SWD -speed 4000

# Terminal 2: Connect with GDB
arm-none-eabi-gdb build/JCOSEK_EXAMPLE.elf \
  -ex "target remote :2331" \
  -ex "monitor reset" \
  -ex "load" \
  -ex "break main" \
  -ex "continue"
```

Alternatively, use VS Code: select the **"Debug S32K144 (J-Link GDB Server)"** configuration (F5).

One-click flash without debugging:
```bash
cd build
JLinkExe -device S32K144 -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
```

#### VExpress-A9 QEMU

```bash
# Terminal 1: Start QEMU
qemu-system-arm -M vexpress-a9 -m 512M -kernel build/JCOSEK_EXAMPLE.elf -nographic -s -S

# Terminal 2: Connect GDB
gdb-multiarch build/JCOSEK_EXAMPLE.elf -ex "target remote :1234"
```

### Tests

```bash
cd build
cmake -DSYS_TEST=test_01_scheduling ..
make -j$(nproc)
```

Available system tests: `test_01_scheduling`, `test_02_resource`, `test_03_event`, `test_04_alarm`, `test_05_ISR`

## Architecture

```
src/                    # Kernel core (platform-independent)
├── Os_Lifecycle.c      # StartOS(), system startup
├── Os_Task.c           # ActivateTask, TerminateTask, ChainTask, GetTaskState
├── Os_Scheduler.c      # O(1) bitmap scheduler (256 priority levels)
├── Os_Resource.c       # Priority Ceiling Protocol
├── Os_Event.c          # Event mechanism (task synchronization)
├── Os_Alarm.c          # Alarm services
├── Os_Counter.c        # Counter services
└── Os_Interrupt.c      # ISR Category 2 support

include/                # Public API headers
└── Os.h                # OSEK/VDX standard services and error codes

portable/
├── cpu/                # CPU architecture abstraction
│   ├── cortex_a9/gcc/  # ARM Cortex-A9 (ARMv7-A, ARM instruction set)
│   └── cortex_m4/gcc/  # ARM Cortex-M4 (ARMv7E-M, Thumb-2, FPv4-SP-D16)
└── bsp/                # Board Support Package
    ├── vexpress_a9_qemu/
    └── s32k144/        # NXP S32K144-Q100 (512KB Flash, 128KB SRAM)
```

## Key Design Decisions

- **Scheduler**: O(1) bitmap-based, 256 priority levels, FIFO within same priority
- **Context switch on Cortex-M4**: PendSV exception with hardware auto-stacking (R0-R3,R12,LR,PC,xPSR)
- **Interrupt control on Cortex-M4**: PRIMASK (all interrupts) + BASEPRI (OS-level masking)
- **Kernel mode on Cortex-A9**: SVC (Supervisor) mode
- **FPU on S32K144**: Hard float ABI, lazy context save deferred (no FPU in kernel)
- **Linker**: freestanding environment (`-nostdlib -nostartfiles -ffreestanding`)

## Code Style

Uses `.clang-format` (Google-based, 4-space indent, 120 column width, Allman braces). Run `clang-format` before committing.

## Commit Convention

Follow [Conventional Commits](https://www.conventionalcommits.org/): `feat:`, `fix:`, `docs:`, `refactor:`, etc.

Must include a DCO sign-off: `Signed-off-by: Name <email>`

## License

This project is licensed under the Mozilla Public License 2.0 (MPL-2.0). See [LICENSE](LICENSE) for the full text.

Modified MPL-2.0 files must remain under MPL-2.0 when distributed.

## Contact

- Security: hejuncheng0@gmail.com
- General: hejuncheng0@gmail.com
