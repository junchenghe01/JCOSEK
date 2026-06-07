# Project Roadmap - JCOSEK (OSEK/VDX OS)

## 🏗 High-Level Architectural Goals
This document outlines the development direction of the JCOSEK OS. Adhering to the **OSEK/VDX** specifications, we are dedicated to developing an open-source operating system that complies with OSEK/VDX standards.

---

## 🟢 v0.0.1 — Base Kernel (Released)
*Focus: Execution Environment & Basic Scheduling*

### Core Kernel
- [x] **Bitmap Scheduler**: O(1) scheduler with 256 priority levels.
- [x] **Context Switching**: ARM Cortex-A9 register context save/restore.
- [x] **FIFO Queuing**: Linked-list management for tasks of the same priority.
- [x] **Startup Sequence**: `StartOS` assembly startup (OSEK/VDX-compliant).

### Arch/ARM
- [x] **SVC Mode**: Kernel runs in privileged mode.
- [x] **VExpress-A9**: Basic QEMU simulation environment.

---

## 🟢 v0.0.2 — Standard Services & Task Management (Completed)
*Focus: Resource/Event/Alarm/ISR + Licensing*

### Task Management
- [x] `GetTaskState` / `ChainTask` / `GetTaskID`

### Resource Management
- [x] `GetResource` / `ReleaseResource` (Priority Ceiling Protocol)

### Event Mechanism
- [x] `SetEvent` / `ClearEvent` / `GetEvent` / `WaitEvent`

### Alarm & Counter
- [x] `SetRelAlarm` / `SetAbsAlarm` / `CancelAlarm` / `GetAlarm`
- [x] Counter tick infrastructure

### Interrupt Handling
- [x] ISR Category 2 support

### Legal
- [x] Adopt MPL-2.0 License for all kernel files.

---

## 🟡 v0.2.0 — Architecture Refactoring & Multi-Platform (In Progress)
*Focus: Clean separation, testability, and expanded MCU support*

### Architecture Refactoring
- [x] Split monolithic OS files into per-module sources (`Os_Task.c`, `Os_Event.c`, `Os_Alarm.c`, etc.)
- [x] Separate service API from core logic for better testability
- [ ] Unit test framework integration (Unity submodule added)

### Platform Ports
- [x] **VExpress-A9 (QEMU)** — Cortex-A9, primary dev target
- [x] **STM32F103** — Cortex-M3 (Bluepill)
- [x] **NXP S32K144** — Cortex-M4 with FPU
- [ ] **NXP S32K144-EVB** — Evaluation board variant (directory created)
- [ ] **Infineon TC334** — TriCore (directory created)

### CI/CD
- [x] Multi-platform build matrix (vexpress_a9_qemu, stm32f103, s32k144)
- [x] Automated system tests on vexpress_a9_qemu
- [x] GitHub Release automation on version tags

### CPU Architecture Support
- [x] Cortex-A9 (ARM)
- [x] Cortex-M3 (Thumb)
- [x] Cortex-M4 (Thumb + FPU)
- [ ] Cortex-M0 (armcc/gcc directories created)
- [ ] TriCore (tasking/common directories created)

---

## 🔴 Future Milestones (v0.3 & Beyond)

### MCU Ecosystem Expansion
| Vendor | Series | Status |
|--------|--------|--------|
| NXP | S32K144 | ✅ Completed |
| NXP | S32K144-EVB | 🔲 Planned |
| NXP | S32G | 🔲 Planned |
| Infineon | AURIX TC334 | 🔲 Scaffold |
| Infineon | AURIX TC3xx/TC4x | 🔲 Planned |
| STMicro | Stellar / SPC5 | 🔲 Planned |
| Renesas | RH850 | 🔲 Planned |
| SemiDrive | E3 (High-Perf MCU) | 🔲 Planned |
| Flagchip | FC7300 (Cortex-M7) | 🔲 Planned |
| AutoChips | AC7840x | 🔲 Planned |

### Quality & Safety
- [ ] Unit tests for all kernel services
- [ ] MISRA-C compliance checking
- [ ] ISO 26262 ASIL-D capable kernel subset

### Ecosystem
- [ ] AUTOSAR-compatible configuration generator (ARXML support)
- [ ] Doxygen API documentation published
- [ ] Integration examples for each supported MCU

---

## 📈 Long-Term Vision
- **Safety**: Achieve ISO 26262 ASIL-D certification readiness for the kernel subset.
- **Ecosystem**: Provide AUTOSAR-compatible tooling (ARXML configuration generation).
- **Community**: Foster an open-source ecosystem around OSEK/VDX-compliant embedded OS.

---

## 🤝 How to Contribute
We welcome Pull Requests targeting tasks marked with `[ ]` in the roadmap. Please review `CONTRIBUTING.md` before starting work.

