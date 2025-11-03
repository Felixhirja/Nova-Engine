@echo off
echo ================================================
echo Nova Engine - High Performance Mode
echo ================================================
echo.
echo Settings:
echo   Target FPS: 400 (configurable)
echo   VSync: Disabled
echo   Optimizations: O3 + march=native + fast-math
echo.

REM Set performance environment variables
set NOVA_TARGET_FPS=400
set NOVA_NO_ADAPTIVE_VSYNC=1

REM Launch engine
echo Starting Nova Engine...
nova-engine.exe

pause
