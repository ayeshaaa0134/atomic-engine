# 🖼️ AtomicTree Frontend Architecture & Telemetry Guide

This document explains the frontend components I have built for the AtomicTree project, their purpose, and how they bridge the gap between high-performance C++ code and a professional GUI.

## 🏁 Overview
The frontend is built using **React + Vite + TailwindCSS**. It is designed to look like a professional performance profiler (similar to Intel VTune or Google Chrome DevTools).

**CRITICAL:** Every view described below is currently being transitioned from "Static Simulation" to "Live Telemetry" via the `useAtomicData` hook.

---

## 🧩 Component Breakdown

### 1. `Sidebar.tsx` (The Command Center)
- **Depicts**: Controls for the engine (Start/Stop Profile, Run Benchmark, Reset).
- **Reality Check**: These buttons trigger commands in `extension.ts`, which in turn executes the G++ build and runs the executable.
- **Logic**: Uses VS Code's `postMessage` API to talk to the extension.

### 2. `TopNav.tsx` (Global Status)
- **Depicts**: Real-time summary metrics (Current Throughput, Active CPU context).
- **Reality Check**: This displays the *last known good* metric received from the backend JSON stream.

### 3. `PerformanceView.tsx` (Throughput & Latency)
- **Depicts**: A dynamic area chart showing **Ops/Sec** (Operations per Second) and **Latency spikes**.
- **Reality Check**: Uses `recharts`. The data is fed from the `throughputHistory` array in our React state. 
- **Features**: 
    - **Crash Recovery Marker**: A vertical red dashed line that appears automatically when the backend sends a `"type": "event"` message indicating a recovery operation took place.
    - **Phase Breakdown**: A "Flame Chart" style bar showing how much time is spent in `Alloc`, `Write Leaf`, `Flush`, etc.

### 4. `MemoryView.tsx` (NVM Heatmap)
- **Depicts**: A grid representing the physical/persistent memory blocks (blocks of 512B by default).
- **Reality Check**: This is a direct visualization of the AtomicTree's internal page table. 
    - **Green**: Free blocks.
    - **Blue**: Allocated/Valid Leaf/Internal nodes.
    - **Purple (Glow)**: Shadow nodes (Copy-on-Write versions waiting for Atomic Swap or GC).
- **Real Memory Integration**: It queries the backend for the current `used_bytes` vs `total_capacity` to scale the heatmap correctly.

### 5. `BottomPanel.tsx` (The "Black Box" Logs)
- **Depicts**: A terminal-like view of every action the AtomicTree engine performs.
- **Reality Check**: This intercepts `printf` and `std::cout` from your C++ code and prints it here for debugging without needing to check the standard terminal.

---

## 🛰️ Data Connectivity (How it's Real)

The "Simulation" parts are being replaced by this flow:
1. **C++ Backend**: Prints `{"type": "metric", "ops": 50000, ...}` to the terminal.
2. **VS Code Extension**: Intercepts the terminal output buffer.
3. **Webview Provider**: Parses that JSON and sends it to the React App.
4. **React Hook (`useAtomicData`)**: Updates the frontend state.
5. **Charts**: Automatically re-render with the new point on the graph.

> [!IMPORTANT]
> To see real memory of *your* specific laptop, the backend uses `GetProcessMemoryInfo` (Windows) to report actual RSS (Resident Set Size) into the JSON stream.
