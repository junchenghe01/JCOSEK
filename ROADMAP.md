# Project Roadmap - JCOSEK (Heterogeneous Safety-Critical Arm RTOS)

## 🏗 High-Level Architectural Goals
本文档概述了 JCOSEK 内核的开发方向。我们遵循 **OSEK/VDX** 和 **AUTOSAR OS** 规范，致力于在 ARM 架构上提供极致的确定性。

---

## 🟢 Current Release: v0.1 (Base Kernel)
*Focus: Execution Environment & Basic Scheduling*
StartOS（系统启动）
Schedule（显式调度请求）
TerminateTask（任务终止）
ActivateTask（任务激活）
### 核心内核 (Core Kernel)
- [x] **Bitmap Scheduler**: 支持 256 优先级的 O(1) 调度器。
- [x] **Context Switching**: ARM Cortex-A9 寄存器现场保存与恢复。
- [x] **FIFO Queuing**: 同优先级任务的链表管理。
- [x] **Startup Sequence**: 符合 AUTOSAR 流程的 `StartOS` 汇编启动。

### 架构适配 (Arch/Arm)
- [x] **SVC Mode**: 内核运行在特权模式。
- [x] **VExpress-A9**: 基础 QEMU 模拟环境搭建。

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
*Focus: Resource Management and Safety*


- [ ] EcuM (ECU State Manager)：管理ECU的生命周期，包括启动、关闭和模式切换，协调电源和通信状态，但需注意多核下的主从模式。
- [ ] BswM (Basic Software Mode Manager)：协调BSW模块间的模式切换，促进模块间状态转换，但需小心模式仲裁以避免冲突。
- [ ] Tm (Time Service)：提供时间相关服务，如定时器和时间测量，支持应用和BSW的定时需求。

CAN
canif
pdur
com
asw
实现要给C状态机，配置可视化 Yakindu Statechart Tools
---

## 📈 Long-Term Vision
*   **Safety**: 满足 ISO 26262 ASIL-D 标准的内核子集。
*   **Ecosystem**: 提供符合 AUTOSAR 标准的配置生成工具 (ARXML support)。

---

## 🤝 How to Contribute
我们欢迎针对 Roadmap 中标注为 [ ] 的任务提交 Pull Request。请在开始前查看 `CONTRIBUTING.md`。



处理到EcuM_SetRelWakeupAlarm