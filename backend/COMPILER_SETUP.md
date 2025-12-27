# Quick Fix Guide - Compiler Not Found

## Problem
CMake can't find a C++ compiler on your system.

## Solution Options

### Option 1: Install MinGW (Recommended - Easiest)

1. **Download MinGW-w64**:
   - Go to: https://github.com/niXman/mingw-builds-binaries/releases
   - Download: `x86_64-14.2.0-release-win32-seh-ucrt-rt_v12-rev0.7z`
   
2. **Extract**:
   - Extract to: `C:\mingw64\`
   
3. **Add to PATH**:
   ```powershell
   # Run in PowerShell as Administrator
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\mingw64\bin", "Machine")
   ```

4. **Verify**:
   ```bash
   # Close and reopen terminal
   g++ --version
   ```

5. **Build**:
   ```bash
   cd C:\Users\Hanzalah\Desktop\atomic-engine\AtomicTree\backend
   .\build.bat
   ```

---

### Option 2: Use Visual Studio Build Tools

1. **Download**:
   https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022

2. **Install**:
   - Select "Desktop development with C++"
   - Takes ~10 minutes

3. **Build**:
   ```bash
   # Open "Developer Command Prompt for VS 2022"
   cd C:\Users\Hanzalah\Desktop\atomic-engine\AtomicTree\backend
   .\build.bat
   ```

---

### Option 3: Quick Build Without CMake (Temporary)

If you need to test quickly:

```powershell
cd C:\Users\Hanzalah\Desktop\atomic-engine\AtomicTree\backend

# Compile manually with MinGW
g++ -std=c++17 -o AtomicTree.exe main.cpp src/*.cpp -Iinclude

# OR with MSVC
cl /std:c++17 /EHsc /Iinclude main.cpp src\*.cpp
```

---

## After Installing Compiler

Run the updated build script:
```bash
cd backend
.\build.bat
```

The script will auto-detect MinGW or Visual Studio and configure appropriately!
