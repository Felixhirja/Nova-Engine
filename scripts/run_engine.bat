@echo off
setlocal

rem Determine the repository root based on the script location.
set "SCRIPT_DIR=%~dp0"
if not defined SCRIPT_DIR (
    echo Failed to determine script directory.&goto :cleanup
)

pushd "%SCRIPT_DIR%.." >nul 2>&1
if errorlevel 1 (
    echo Error: Unable to change directory to the repository root.&goto :cleanup
)
set "PUSHED=1"

if exist nova-engine.exe (
    nova-engine.exe
) else (
    echo Error: nova-engine.exe not found. Build the project before running this script.
)

:cleanup
if defined PUSHED popd >nul 2>&1
pause
