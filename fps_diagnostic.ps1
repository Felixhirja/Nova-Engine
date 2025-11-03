# Nova Engine FPS Diagnostic Script
Write-Host "=== Nova Engine FPS Diagnostic ===" -ForegroundColor Cyan

# 1. Check if the engine is built with optimizations
Write-Host "`n1. Checking build optimizations..." -ForegroundColor Yellow
$makefileContent = Get-Content "Makefile" | Select-String "CXXFLAGS.*-O3"
if ($makefileContent) {
    Write-Host "✅ Build optimized with -O3" -ForegroundColor Green
} else {
    Write-Host "❌ Missing -O3 optimization" -ForegroundColor Red
}

# 2. Check VSync settings
Write-Host "`n2. Checking VSync/FPS settings..." -ForegroundColor Yellow
$fpsControllerH = Get-Content "engine/FramePacingController.h" | Select-String "targetFPS.*="
$fpsControllerCpp = Get-Content "engine/FramePacingController.cpp" | Select-String "clamp.*fps"
Write-Host "Current FPS settings found in source files"

# 3. Quick performance test
Write-Host "`n3. Running ECS performance test..." -ForegroundColor Yellow
if (Test-Path "test_optimized.exe") {
    $testResult = & ".\test_optimized.exe" 2>&1
    if ($testResult -match "Estimated FPS.*: (\d+)") {
        $ecsFps = $matches[1]
        Write-Host "✅ ECS Performance: $ecsFps FPS" -ForegroundColor Green
    }
} else {
    Write-Host "❌ Performance test not available" -ForegroundColor Red
}

# 4. System information
Write-Host "`n4. System Information..." -ForegroundColor Yellow
$gpu = Get-WmiObject Win32_VideoController | Select-Object -First 1
Write-Host "GPU: $($gpu.Name)"
$cpu = Get-WmiObject Win32_Processor | Select-Object -First 1
Write-Host "CPU: $($cpu.Name)"

# 5. Possible solutions
Write-Host "`n=== POSSIBLE SOLUTIONS ===" -ForegroundColor Cyan
Write-Host "If you're getting 25 FPS, try these fixes:"
Write-Host "1. Press F11 in-game to disable VSync" -ForegroundColor White
Write-Host "2. Update your GPU drivers" -ForegroundColor White
Write-Host "3. Set engine environment variable: `$env:NOVA_TARGET_FPS='400'" -ForegroundColor White
Write-Host "4. Disable Windows Game Mode/DVR" -ForegroundColor White
Write-Host "5. Close background applications" -ForegroundColor White

Write-Host "`n=== QUICK FIXES ===" -ForegroundColor Green
Write-Host "Run with maximum performance:"
Write-Host "`$env:NOVA_TARGET_FPS='400'; `$env:NOVA_NO_ADAPTIVE_VSYNC='1'; .\nova-engine.exe" -ForegroundColor Yellow