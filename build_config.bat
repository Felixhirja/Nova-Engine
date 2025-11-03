@echo off
echo ========================================
echo Configuration Management Build Script
echo ========================================
echo.

REM Check for g++
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: g++ not found in PATH
    echo Please install MinGW or add g++ to your PATH
    exit /b 1
)

echo Building Configuration Management System...
echo.

REM Build ConfigEditor
echo [1/3] Compiling ConfigEditor.cpp...
g++ -std=c++17 -Wall -I. -c engine/config/ConfigEditor.cpp -o engine/config/ConfigEditor.o
if %ERRORLEVEL% NEQ 0 (
    echo Error: ConfigEditor compilation failed
    exit /b 1
)

REM Build SimpleJson
echo [2/3] Compiling SimpleJson.cpp...
g++ -std=c++17 -Wall -I. -c engine/SimpleJson.cpp -o engine/SimpleJson.o
if %ERRORLEVEL% NEQ 0 (
    echo Error: SimpleJson compilation failed
    exit /b 1
)

REM Build ConfigManager
echo [2.5/3] Compiling ConfigManager.cpp...
g++ -std=c++17 -Wall -I. -c engine/config/ConfigManager.cpp -o engine/config/ConfigManager.o
if %ERRORLEVEL% NEQ 0 (
    echo Error: ConfigManager compilation failed
    exit /b 1
)

REM Build test
echo [3/3] Building minimal test...
g++ -std=c++17 -Wall -I. tests/test_config_minimal.cpp engine/config/ConfigEditor.o engine/config/ConfigManager.o engine/SimpleJson.o -o test_config_minimal.exe
if %ERRORLEVEL% NEQ 0 (
    echo Error: Test build failed
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Run the test with:
echo   test_config_minimal.exe
echo.
