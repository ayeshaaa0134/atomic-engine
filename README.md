# Atomic Tree Engine - Walkthrough

## Overview
The **Atomic Tree Engine** is a high-performance persistent memory visualization tool. It demonstrates how modern CPU micro-architecture (Store Buffers, Cache Hierarchies) interacts with Non-Volatile Memory (NVM) using advanced data structures like **NV-Tree** and **WORT**.

**Analogy**: It is a "Glass-Walled High-Speed Train System". You (the user) are in the Control Tower (VS Code), watching the tracks being laid (Flushes) and verified (Fences) milliseconds before the Train (Atomic Pointer Update) switches tracks.

## Implemented Components

### 1. Backend (C++)
*   **Primitives**: Low-level wrappers for `CLFLUSHOPT` and `SFENCE` with instruction tracing. 
    *   *See*: `backend/src/primitives.cpp`
*   **Allocator**: Bitmap-based Persistent Allocator using Memory Mapped Files (Win32).
    *   *See*: `backend/src/allocator.cpp`
*   **NV-Tree**: Implements "Atomic Split" (Shadow Paging) to ensure crash consistency.
    *   *See*: `backend/src/b_tree.cpp`
*   **WORT**: Write-Optimal Radix Tree using 8-byte failure-atomic updates.
    *   *See*: `backend/src/wort.cpp`
*   **Manager**: JSON-RPC Loop handling commands from VS Code.
    *   *See*: `backend/src/manager.cpp`

### 2. VS Code Extension
*   **Control Tower**: Spawns the backend process and routes telemetry.
*   **GitLens-Style UX**:
    *   Dedicated Activity Bar Icon.
    *   Rich Sidebars ("Dashboard", "Structure Explorer").
    *   Persistent Trace/Crash Buttons in View Title Bars.
*   **Metadata**: Published by **Ayesha Siddiqa**, Dev: **Ayesha Siddiqa**, Co-Dev: **Hadia Naveed**.

### 3. Frontend (React)
*   **Dashboard**: Real-time visualization of Throughput (IOPS) and Latency (P99).
*   **Radar**: "Out-of-Order Radar" visualizing the gap between CPU Store Buffers and NVM Persistence.
*   **Heatmap**: Grid view of the Persistent Heap.

## Build Instructions

### Prerequisites
*   Node.js & NPM
*   (Optional) MinGW/CMake if you want to rebuild the backend manually.

### Steps
1.  **Build Frontend & Package**:
    ```powershell
    cd frontend
    npm install && npm run build
    cd ../extension
    npm install && npm run package
    ```
2.  **Run Extension**:
    *   Open `d:/DSA-Project` in VS Code.
    *   Press `F5` to launch the Extension Host.
    *   *Note*: The backend `atomic-engine.exe` is pre-bundled in `extension/bin`. No compilation required!

## Verification
1.  Click the **Atomic Tree** icon in the Activity Bar.
2.  Click the **Play** button (Start Workload) in the "Dashboard" view title.
3.  Observe the real-time telemetry updates.
4.  Click the **Zap** button (Simulate Crash) to fail the backend and verify recovery.
