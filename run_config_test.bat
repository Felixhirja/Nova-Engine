@echo off
echo Running Configuration Management Integration Test...
echo.
if exist test_config_minimal.exe (
    test_config_minimal.exe
) else (
    echo Error: test_config_minimal.exe not found
    echo Run build_config.bat first
    exit /b 1
)
