# How to Use AtomicTree Extension with Your Own C++ Code

## Overview
Write custom C++ code using AtomicTree and automatically visualize performance metrics in VS Code!

---

## Step 1: Create Your C++ File

Create any `.cpp` file in the `AtomicTree/examples/` folder (or anywhere in the project).

### Example Code Template:

```cpp
#include <iostream>
#include "../backend/include/manager.h"
#include "../backend/include/B_tree.h"
#include "../backend/include/garbage_collector.h"

using namespace atomic_tree;

int main() {
    // Initialize metrics output
    std::cout << R"({"type":"init","name":"MyApp"})" << std::endl;
    
    // Create your database
    Manager manager("my_data.dat", 256*1024*1024, 256, true);
    
    BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;
    
    BTree tree(&manager, config);
    
    // YOUR CUSTOM LOGIC HERE
    for (int i = 0; i < 10000; i++) {
        tree.insert(i, i * 2);
        
        // Output metrics for visualization (every 100 ops)
        if (i % 100 == 0) {
            std::cout << R"({"type":"metric","name":"throughput","value":)" 
                      << (100.0 / (i+1)) << "}" << std::endl;
        }
    }
    
    // Cleanup
    GarbageCollector gc(&manager);
    gc.collect(tree.root_offset());
    
    std::cout << R"({"type":"complete"})" << std::endl;
    return 0;
}
```

---

## Step 2: Build and Run with Extension

### Method 1: Using Keyboard Shortcut (Easiest)

1. Open your `.cpp` file in VS Code
2. Press **`Ctrl+Shift+B`** (build)
3. Press **`F5`** (run with visualization)
4. Extension automatically opens and shows live metrics!

### Method 2: Using Command Palette

1. Press **`Ctrl+Shift+P`**
2. Type: `AtomicTree: Run Current File`
3. Extension launches automatically

### Method 3: Using Tasks

1. Press **`Ctrl+Shift+P`**
2. Type: `Tasks: Run Task`
3. Select: **"Run AtomicTree with Extension"**

---

## Step 3: View Real-Time Metrics

Once running, the extension shows:

### Performance Tab:
- **Throughput** (operations/sec)
- **Latency** (microseconds)
- **Write Amplification** (1.0x target)
- **Real-time chart** updating live

### Memory Tab:
- **Bitmap heatmap** (green = free, blue = allocated)
- **GC statistics** (nodes marked, blocks freed)
- **Memory table** (internal/leaf/shadow nodes)

---

## JSON Metrics Format

Your C++ code should output JSON lines to `stdout` for the extension to parse:

### Metrics:
```json
{"type":"metric","name":"throughput","value":150000}
{"type":"metric","name":"latency","value":12.5}
{"type":"metric","name":"write_amp","value":1.0}
```

### Events:
```json
{"type":"event","name":"crash","recovered":true}
{"type":"event","name":"gc_complete","marked":156,"freed":12}
{"type":"event","name":"split","node_type":"leaf"}
```

### Logs:
```json
{"type":"log","level":"info","message":"Operation complete"}
```

---

## Example: Complete Working Program

See: [`examples/my_custom_code.cpp`](file:///c:/Users/Hanzalah/Desktop/atomic-engine/AtomicTree/examples/my_custom_code.cpp)

```cpp
// This example shows:
// - Creating a database
// - Inserting 10k records
// - Outputting metrics
// - Running GC
// - Extension visualizing everything live!
```

---

## How It Works

```
Your C++ Code (my_code.cpp)
    ↓
    Outputs JSON to stdout
    ↓
VS Code Extension (listening to terminal)
    ↓
    Parses JSON metrics
    ↓
Webview Dashboard (React)
    ↓
Updates charts/heatmaps in real-time
```

---

## Quick Start Workflow

1. **Create** `examples/my_test.cpp`:
   ```cpp
   #include "../backend/include/manager.h"
   #include "../backend/include/B_tree.h"
   
   int main() {
       Manager mgr("test.dat", 64*1024*1024, 256, true);
       BTreeConfig cfg{16, 8, 32};
       BTree tree(&mgr, cfg);
       
       for(int i=0; i<1000; i++) {
           tree.insert(i, i*10);
       }
       
       return 0;
   }
   ```

2. **Open** in VS Code

3. **Press** `Ctrl+Shift+B` to build

4. **Press** `F5` to run with visualization

5. **Watch** extension show live metrics!

---

## Troubleshooting

### Extension doesn't show metrics?
- Check your code outputs JSON to `stdout`
- Use `std::endl` to flush output
- Ensure extension is installed (`F5` in extension folder)

### Build fails?
- Verify MinGW is installed: `g++ --version`
- Check includes point to `../backend/include/`
- Ensure all source files are linked (see `tasks.json`)

### Can't see charts?
- Click **AtomicTree icon** in VS Code sidebar
- Switch to **"Live Metrics"** tab
- Click **"Run Profile"** if not auto-started

---

## Advanced: Custom Metrics

Add your own metrics:

```cpp
// Custom metric
std::cout << R"({"type":"metric","name":"cache_hits","value":)" 
          << cache_hit_count << "}" << std::endl;

// Custom event
std::cout << R"({"type":"event","name":"custom_split","details":"..."})" 
          << std::endl;
```

The extension will display these in the dashboard automatically!

---

## Summary

✅ Write C++ code using AtomicTree
✅ Output JSON metrics to stdout
✅ Press F5 in VS Code
✅ Extension automatically visualizes everything!

Now you can **code, run, and visualize** all in one workflow! 🚀
