@echo off
REM Ultra-fast unity build script
REM Combines source files for maximum compilation speed

echo Building Nova Engine with Unity Build...
set UNITY_BUILD=1
set BUILD_MODE=fast
mingw32-make -j8 %*

if %ERRORLEVEL% equ 0 (
    echo Unity build completed successfully!
    echo Note: Unity builds compile faster but may have longer link times
) else (
    echo Unity build failed with exit code %ERRORLEVEL%
)