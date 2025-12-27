# 🌲 AtomicTree Engine: Professional Project Handbook

Welcome to the **AtomicTree Engine** handbook. This document provides a comprehensive overview of the project, its core architecture, professional features, and the engineering principles that make it a state-of-the-art solution for Non-Volatile Memory (NVM) data management.

---

## 1. Project Overview: What is AtomicTree?

The **AtomicTree Engine** is a high-performance, persistent B+ Tree implementation optimized specifically for modern storage hardware like **NVMe SSDs** and **Intel Optane (Persistent Memory)**. 

### The Problem
Traditional databases are designed for slow HDDs or volatile RAM. When using high-speed persistent storage, the overhead of "logging" (WAL) and constant disk flushing creates a massive performance bottleneck.

### The Solution: "Direct Persist"
AtomicTree eliminates the need for heavy write-ahead logs by using **Atomic Pointer Swaps** and **Shadow Paging**. It treats your storage like memory, allowing for microsecond-level latency while ensuring that if the power fails, your data remains perfectly consistent.

---

## 2. Professional Features

The PRO version of AtomicTree includes advanced engineering refinements:

### 🛡️ NVM Integrity Verification
Every time the engine starts, it performs a structural health check. It uses a custom **checksum algorithm** to verify that the persistent region hasn't been corrupted by external processes or hardware failure.
- **Magic Byte Validation**: Identifies the specific AtomicTree format.
- **Probabilistic Health Checks**: Real-time integrity status displayed on the dashboard.

### 🔍 Deep Hardware Detection
The VS Code extension doesn't just show generic stats; it performs deep system inspection:
- **Storage Type**: Automatically detects if you are running on a high-speed **NVMe/SSD** or a legacy **HDD**.
- **Physical Model**: Displays the exact drive model (e.g., "Samsung 980 PRO").
- **CPU Arch**: Optimizes B+ Tree split logic based on your CPU core count and architecture.

### 📊 Real-Time Visualizer (Glassmorphism UI)
A premium, dark-mode dashboard provides instant transparency into the engine's internals:
- **IOPS throughput**: Live graph of operations per second.
- **Write Amplification (WAF)**: A critical "x-factor" metric showing the ratio of Physical to Logical writes.
- **Memory Map**: A pixel-perfect hex visualizer showing exactly which blocks of the NVM region are allocated or free.

---

## 3. Architecture & Code Breakdown

The project is unified at the repository root across three distinct layers:

### ⚙️ Backend (C++ 17) - `backend/`
The high-performance core.
- **`src/manager.cpp`**: The persistent memory allocator. It maps a file (e.g., `nvm.dat`) into memory and manages blocks using a high-speed bitmap.
- **`src/B_tree.cpp`**: Implements the B+ Tree logic. Features shadow-paging for crash-consistent updates without logging.
- **`include/primitives.h`**: Defines the fundamental data structures used throughout the engine.

### 🔌 Extension (TS/VS Code) - `extension/`
The bridge between VS Code and the Engine.
- **`src/extension.ts`**: Handles the VS Code lifecycle, registers commands, and broadcasts system metrics (CPU/RAM/Disk).
- **`src/webview/main.tsx`**: The React-based frontend. Uses the **VS Code Webview UI Toolkit** for a seamless, professional experience.

### 📂 Documentation & Guides - `docs/`
- **`ARCHITECTURE.md`**: Deep dive into the B+ Tree split and merge algorithms.
- **`QA_TEST_PLAN.md`**: Outlines the stress-testing and integrity validation suites.

---

## 4. How the "Magic" Works (Logic Flow)

1. **Activation**: When you open a C++ file, the extension "wakes up."
2. **Build**: The `AtomicTree Build` task compiles the C++ backend using G++.
3. **Execution**: The executable runs in the terminal. As it processes data, it prints JSON telemetry to `stdout`.
4. **Capture**: The VS Code extension "sniffs" the terminal output in real-time, parses the JSON, and sends it to the Dashboard.
5. **Visualization**: The React UI updates instantly, showing you the "pulse" of your persistent memory engine.

---

## 5. Summary for the User

**This project is a bridge between low-level systems engineering and high-level developer experience.** It allows you to build data structures that are fast as RAM but permanent as a Disk, all while watching the engine work in a beautiful, real-time visual environment.

> [!TIP]
> Always look at the **Write Amplification** metric. A value close to **1.0x** means the engine is perfectly optimized for your hardware!
