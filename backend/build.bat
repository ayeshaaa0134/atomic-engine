@echo off
REM Atomic Tree Engine - Fixed Build Script
REM Automatically detects available compiler

echo ============================================
echo   Atomic Tree Engine - Build Script
echo ============================================
echo.

REM Check if CMake is installed
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found!
    echo Please install CMake from: https://cmake.org/download/
    pause
    exit /b 1
)

echo [1/6] CMake found: 
cmake --version
echo.

echo [2/6] Creating build directory...
if exist build rmdir /s /q build
mkdir build
cd build
echo.

echo [3/6] Detecting compiler...

REM Try MinGW first (most common)
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found MinGW/GCC - Using MinGW Makefiles
    cmake .. -G "MinGW Makefiles"
    goto :build
)

REM Try Visual Studio 2022
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found Visual Studio - Using Visual Studio 17 2022
    cmake .. -G "Visual Studio 17 2022"
    goto :build
)

REM Try Visual Studio 2019
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" (
    echo Found Visual Studio 2019 - Using Visual Studio 16 2019
    cmake .. -G "Visual Studio 16 2019"
    goto :build
)

REM No compiler found
echo [ERROR] No C++ compiler found!
echo.
echo Please install one of the following:
echo 1. MinGW-w64: https://www.mingw-w64.org/downloads/
echo 2. Visual Studio 2022 Community: https://visualstudio.microsoft.com/downloads/
echo.
echo Recommended: MinGW (lightweight)
pause
exit /b 1

:build
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)
echo.

echo [4/6] Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
echo.

echo [5/6] Build complete!
echo.
echo ============================================
echo   Build Successful!
echo ============================================
echo.
echo Executables created in: build\Release\ (or build\ for MinGW)
echo.
echo Run tests with:
echo   cd build
if exist Release (
    echo   Release\stress_test.exe
    echo   Release\speed_test.exe
) else (
    echo   stress_test.exe
    echo   speed_test.exe
)
echo.
pause
