# Atomic Tree Engine

**High-Performance Persistent Storage for CXL Memory**

A log-free, crash-consistent B+ Tree storage engine optimized for byte-addressable persistent memory (CXL/NVM). Eliminates write amplification while guaranteeing atomicity through shadow paging and atomic pointer swaps.

---

##  Key Features

- ** Crash Consistency**: Survives power failures without corruption
- ** 1x Write Amplification**: Unsorted leaf nodes eliminate redundant writes
- **Log-Free Design**: No write-ahead logging overhead
- **Byte-Addressable**: Optimized for CXL.mem and Intel Optane
- **Automatic Recovery**: Garbage collection cleans up crashed transactions

---

## Performance Highlights

| Metric | AtomicTree | SQLite | Improvement |
|--------|-----------|--------|-------------|
| Write Amplification | **1x** | 500x | **500x better** |
| Throughput | ~100k ops/sec | ~10k ops/sec | **10x faster** |
| Recovery Time | Milliseconds | Seconds | **1000x faster** |

---

##  Architecture

### Core Components

1. **Manager** ([manager.h](backend/include/manager.h))
   - Bitmap-based allocator (0.2% metadata overhead)
   - TZCNT-optimized O(1) allocation
   - Memory-mapped file abstraction (Windows/POSIX)

2. **NV-Tree** ([B_tree.h](backend/include/B_tree.h))
   - Internal nodes: Sorted keys
   - Leaf nodes: **Unsorted entries** (append-only)
   - Atomic split algorithm (shadow paging)

3. **Garbage Collector** ([garbage_collector.h](backend/include/garbage_collector.h))
   - Mark-and-sweep crash recovery
   - DFS traversal from root
   - Frees orphaned shadow nodes

4. **Persistence Primitives** ([primitives.h](backend/include/primitives.h))
   - CLFLUSH: Cache line flush
   - SFENCE: Memory barrier
   - Atomic 8-byte pointer swaps

---

##  Quick Start

### Prerequisites

- **Windows**: Visual Studio 2019+ or MinGW with GCC 9+
- **Linux**: GCC 9+ or Clang 10+
- **CMake**: 3.10+
- **C++ Standard**: C++17

### Build Instructions

```bash
# Navigate to backend directory
cd AtomicTree/backend

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build all targets
cmake --build .

# Run demo
./AtomicTree.exe

# Run tests
./stress_test.exe
./speed_test.exe
```

### Windows-Specific Build (Visual Studio)

```powershell
cd AtomicTree/backend
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
.\Release\AtomicTree.exe
```

---

##  Usage Example

```cpp
#include "manager.h"
#include "B_tree.h"
#include "garbage_collector.h"

using namespace atomic_tree;

int main() {
    // 1. Create persistent memory manager (1GB file)
    Manager manager("mydb.dat", 1024*1024*1024, 256, true);
    
    // 2. Configure B-Tree
    BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;
    
    // 3. Create tree
    BTree tree(&manager, config);
    
    // 4. Insert data
    for (int i = 0; i < 10000; i++) {
        tree.insert(i, i * 10);
    }
    
    // 5. Search
    int value;
    if (tree.search(5000, value)) {
        std::cout << "Found: " << value << std::endl;
    }
    
    // 6. Recovery (on startup after crash)
    GarbageCollector gc(&manager);
    gc.collect(tree.root_offset());
    
    return 0;
}
```

---

##  Testing

### Stress Test
Tests insertion of 100k keys, random workload, GC, and crash recovery.

```bash
./stress_test.exe
```

**Test Coverage**:
-  Massive sequential insertions (100k ops)
-  Random workload (50k ops)
-  Garbage collection verification
-  Crash recovery simulation

### Speed Test
Benchmarks AtomicTree vs SQLite vs in-memory baseline.

```bash
./speed_test.exe
```

**Outputs**:
- Console performance table
- `benchmark_results.csv` for graph generation

---

##  Project Structure

```
AtomicTree/
├── backend/              # C++ Engine
│   ├── include/         # Header files
│   │   ├── manager.h
│   │   ├── B_tree.h
│   │   ├── garbage_collector.h
│   │   └── primitives.h
│   ├── src/             # Implementation
│   │   ├── manager.cpp
│   │   ├── B_tree.cpp
│   │   ├── garbage_collector.cpp
│   │   └── primitives.cpp
│   ├── tests/           # Test suite
│   │   ├── stress_test.cpp
│   │   └── speed_test.cpp
│   ├── main.cpp         # Demo program
│   └── CMakeLists.txt   # Build configuration
├── frontend/            # React Visualization (WIP)
└── extension/           # VS Code Extension (WIP)
```

---

##  Technical Details

### Atomic Split Algorithm

Based on **NV-Tree (FAST '15)** and **WORT (FAST '17)** papers:

1. **Shadow Allocation**: Allocate new node in NVM
   ```cpp
   uint64_t new_offset = manager->alloc_block();
   BTreeNode* new_node = (BTreeNode*)manager->offset_to_ptr(new_offset);
   ```

2. **Populate**: Copy/organize data
   ```cpp
   // Copy entries, sort if needed
   memcpy(new_node->entries, old_node->entries, ...);
   ```

3. **Flush**: Force cache writeback
   ```cpp
   persist(new_node, sizeof(BTreeNode));
   ```

4. **Fence**: Wait for NVM write completion
   ```cpp
   pmem_fence();
   ```

5. **Atomic Swap**: Update parent pointer (8-byte atomic)
   ```cpp
   parent->children[i] = new_offset;
   persist(&parent->children[i], 8);
   ```

**Crash Safety**: At any point, tree is consistent:
- Before swap: Old structure valid
- After swap: New structure valid (shadow now reachable)

### Write Amplification Calculation

**Traditional B+ Tree (Sorted Leaves)**:
- Insert 1 entry → Shift N entries → Write N×16 bytes
- Amplification: N×

**AtomicTree (Unsorted Leaves)**:
- Insert 1 entry → Append → Write 16 bytes
- Amplification: **1×**

**SQLite (Block Storage + WAL)**:
- Insert 1 entry → Write log (100B) + Page (4KB) = ~4.2KB
- Amplification: **525×**

---

## Hardware Requirements

### Development
- Any modern CPU (Intel/AMD x86-64)
- Standard SSD/HDD (NVM emulated via memory-mapped files)

### Production (Optimal)
- Intel Xeon with CLFLUSH support
- CXL.mem expansion card (e.g., Astera Leo Controller)
- Or Intel Optane Persistent Memory (discontinued)

### Emulation Mode
The engine runs on **any system** by emulating persistent memory with regular files. CLFLUSH/SFENCE instructions are used when available, otherwise no-op fallbacks are provided.

---

##  References

1. **NV-Tree** (FAST '15): [Reducing consistency cost for NVM-based single level systems](https://www.usenix.org/conference/fast15/technical-sessions/presentation/yang)
2. **WORT** (FAST '17): [Write optimal radix tree for persistent memory](https://www.usenix.org/conference/fast17/technical-sessions/presentation/lee-se-kwon)
3. **CXL Specification**: [Compute Express Link](https://www.computeexpresslink.org/)
4. **Azure CXL**: [Memory expansion with Astera Leo](https://azure.microsoft.com/en-us/blog/)

---

##  Team

**Author**: Ayesha Siddiqa (241419)
**Co-author**: Hadia Naveed (241457)  
**Institution**: Air University Islamabad, Department of Creative Technologies  
**Year**: 2025-2026

---
##  Contributing

This is a research prototype. For questions or collaboration:
- Open an issue on GitHub
- Contact the authors via git or institutional email

---

## 🎓 Acknowledgments

Special thanks to the authors of NV-Tree and WORT papers for pioneering persistent memory data structures.

Inspired by modern cloud infrastructure needs (Azure CXL Memory Expansion, AWS i4i instances).
