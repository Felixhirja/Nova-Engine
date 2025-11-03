@echo off
REM Fast build script for Nova Engine development
REM Uses minimal optimization for faster compilation

echo Building Nova Engine in FAST mode...
set BUILD_MODE=fast
mingw32-make -j8 %*

if %ERRORLEVEL% equ 0 (
    echo Fast build completed successfully!
) else (
    echo Build failed with exit code %ERRORLEVEL%
)