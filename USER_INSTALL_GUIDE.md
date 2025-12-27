# For Users: Installing AtomicTree Extension

## Super Simple Installation

### Method 1: From .vsix File (Recommended for Students)

1. Get the `.vsix` file from your instructor/classmate
2. Open VS Code
3. Press `Ctrl+Shift+P`
4. Type: `Extensions: Install from VSIX`
5. Select the `.vsix` file
6. ✅ Done! Extension installed

### Method 2: From VS Code Marketplace (If Published)

1. Open VS Code
2. Click Extensions icon (or press `Ctrl+Shift+X`)
3. Search: **"AtomicTree Engine"**
4. Click **Install**
5. ✅ Done!

---

## Using the Extension

### Once Installed:

1. **Open any C++ project** with AtomicTree code
2. **Click the AtomicTree icon** (⚛️) in the left sidebar
3. You'll see two panels:
   - **Configuration** - Settings and controls
   - **Live Metrics** - Real-time charts

### To Visualize Your Code:

1. Open your `.cpp` file
2. Press **`F5`** to run
3. Extension automatically shows:
   - Performance metrics
   - Memory usage
   - Live charts

**That's it - no configuration needed!**

---

## Example: First Time Use

```cpp
// 1. Write simple code (examples/simple_loop.cpp)
#include "../backend/include/manager.h"
#include "../backend/include/B_tree.h"

int main() {
    Manager mgr("test.dat", 64*1024*1024, 256, true);
    BTreeConfig cfg{16, 8, 32};
    BTree tree(&mgr, cfg);
    
    for(int i = 0; i < 1000; i++) {
        tree.insert(i, i*2);
    }
    return 0;
}
```

```powershell
# 2. Press F5 in VS Code
# 3. Extension opens automatically showing metrics!
```

---

## Requirements

**What you need**:
- ✅ VS Code (free: https://code.visualstudio.com/)
- ✅ MinGW compiler (if writing code: https://www.mingw-w64.org/)

**What you DON'T need**:
- ❌ Node.js
- ❌ CMake (unless building from source)
- ❌ Any other tools!

---

## Troubleshooting

### Extension doesn't show metrics?
- Make sure your code outputs JSON to console
- Check AtomicTree icon in sidebar is clicked

### Can't build C++ code?
- Install MinGW: https://www.mingw-w64.org/downloads/
- Add to PATH: `C:\mingw64\bin`

### Extension not showing in sidebar?
- Reload VS Code: `Ctrl+Shift+P` → "Reload Window"

---

## That's It!

**3 Steps Total**:
1. Install extension (1 click)
2. Open C++ code
3. Press F5

**Extension does everything automatically!** 🎉
