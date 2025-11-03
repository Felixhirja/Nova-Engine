@echo off
REM Debug build script with debugging symbols
REM No optimization for easier debugging

echo Building Nova Engine in DEBUG mode...
set BUILD_MODE=debug
mingw32-make -j8 %*

if %ERRORLEVEL% equ 0 (
    echo Debug build completed successfully!
    echo Debugging symbols included, optimizations disabled
) else (
    echo Debug build failed with exit code %ERRORLEVEL%
)