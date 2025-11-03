@echo off
REM Fast parallel build script for Nova Engine
REM Uses all CPU cores for maximum speed

echo ============================================
echo Nova Engine - Fast Build
echo ============================================
echo.

REM Detect number of CPU cores
for /f "tokens=2 delims==" %%a in ('wmic cpu get NumberOfLogicalProcessors /value ^| find "="') do set CORES=%%a
echo Detected %CORES% CPU cores

echo.
echo Building with parallel compilation (-j%CORES%)...
echo.

REM Build with optimization and parallel jobs
mingw32-make -j%CORES% 2>&1

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo Build completed successfully!
    echo ============================================
    echo.
    echo To run: nova-engine.exe
    echo.
) else (
    echo.
    echo ============================================
    echo Build failed with error code %ERRORLEVEL%
    echo ============================================
    pause
)
