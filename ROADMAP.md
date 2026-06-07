# Project Roadmap - JCOSEK (OSEK/VDX OS)

## 🏗 High-Level Architectural Goals
This document outlines the development direction of the JCOSEK OS. Adhering to the **OSEK/VDX** specifications, we are dedicated to developing an open-source operating system that complies with OSEK/VDX standards.

---

## 🟢 Current Release: v0.1 (Base Kernel)
*Focus: Execution Environment & Basic Scheduling*
StartOS (System Startup)
Schedule (Explicit Scheduling Request)
TerminateTask (Task Termination)
ActivateTask (Task Activation)
### Core Kernel
- [x] **Bitmap Scheduler**: Supports an O(1) scheduler with 256 priority levels.
- [x] **Context Switching**: ARM Cortex-A9 Register Context Saving and Restoration
- [x] **FIFO Queuing**: Linked list management for tasks of the same priority.
- [x] **Startup Sequence**: `StartOS` Assembly Startup (OSEK/VDX-compliant)

### Arch/Arm
- [x] **SVC Mode**: The kernel runs in privileged mode.
- [x] **VExpress-A9**: Setting up a Basic QEMU Simulation Environment

---

## 🟢 Next Milestone: v0.0.2 (Standard Observability & Task Management)
*Focus: Enhancing System Introspection and Task Chaining (BCC1 Foundation)*

### 任务管理 (Task Management)
- [x] **Task Management**: 实现 `GetTaskState` / `ChainTask` 和 `GetTaskID`。

### 资源管理 (Resource Management)
- [x] **Resource Management**: 实现 `GetResource` / `ReleaseResource` / `ChainTask`。
- [x] **事件机制（若需扩展任务）**: 
- [x] **报警 + Counter**: 
- [x] **ISR Category 2 支持**: 
- [x] Legal: Adopt MPL-2.0 License for kernel files.。
---

## 🟡 Future Milestones (v0.3 & Beyond)
更新代码架构
服务API和服务逻辑拆分方便测试
审查需求，并构建单元测试代码

适配主流的其次
恩智浦 (NXP) - S32K & S32G 系列
英飞凌 (Infineon) - AURIX 系列
意法半导体 (ST) - Stellar & SPC5 系列
瑞萨 (Renesas) - RH850 系列
芯驰 (SemiDrive): E3 系列（高性能 MCU）。
旗宣 (Flagchip): FC7300 系列（基于 Cortex-M7）。
杰发科技 (AutoChips): AC7840x 系列。
---

## 📈 Long-Term Vision
*   **Safety**: 满足 ISO 26262 ASIL-D 标准的内核子集。
*   **Ecosystem**: 提供符合 AUTOSAR 标准的配置生成工具 (ARXML support)。

---

## 🤝 How to Contribute
我们欢迎针对 Roadmap 中标注为 [ ] 的任务提交 Pull Request。请在开始前查看 `CONTRIBUTING.md`。
