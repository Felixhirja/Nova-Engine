@echo off
echo ================================================
echo Nova Engine - Config System Integration Helper
echo ================================================
echo.

echo This script will help you integrate the Configuration
echo Management System into your Nova Engine game.
echo.

echo Step 1: Checking if main game executable exists...
if exist "nova-engine.exe" (
    echo   Found: nova-engine.exe
) else if exist "NovaEngine.exe" (
    echo   Found: NovaEngine.exe
) else (
    echo   No game executable found yet
    echo   You'll need to add config system to your build
)
echo.

echo Step 2: Checking build system...
if exist "Makefile" (
    echo   Found: Makefile
    echo   Add these lines to your Makefile:
    echo.
    echo   CONFIG_OBJS = engine/config/ConfigManager.o engine/config/ConfigEditor.o
    echo   nova-engine: $(OBJS) $(CONFIG_OBJS)
) else (
    echo   No Makefile found
)

if exist "build_vs.bat" (
    echo   Found: build_vs.bat
    echo   Add config files to your Visual Studio build
) else (
    echo   No build_vs.bat found
)
echo.

echo Step 3: Example integration code created at:
echo   GAME_INTEGRATION_GUIDE.md
echo.

echo Step 4: Quick test - validating example configs...
if exist "test_config_minimal.exe" (
    test_config_minimal.exe
    if %ERRORLEVEL% EQU 0 (
        echo   Config system is working!
    ) else (
        echo   Config system test failed
    )
) else (
    echo   Run build_config.bat first to test the system
)
echo.

echo ================================================
echo Integration Checklist:
echo ================================================
echo [ ] 1. Add ConfigManager.o and ConfigEditor.o to your build
echo [ ] 2. Add initialization code to your main.cpp
echo [ ] 3. Replace old config loading with ConfigManager
echo [ ] 4. Add validation tests for your configs
echo [ ] 5. Test with your game
echo.

echo Next steps:
echo   1. Read GAME_INTEGRATION_GUIDE.md
echo   2. Update your build system
echo   3. Add config initialization to main.cpp
echo   4. Run your game!
echo.

pause
