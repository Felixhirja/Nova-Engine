Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Nova Engine - High Performance Mode" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Settings:" -ForegroundColor Yellow
Write-Host "  Target FPS: 400 (configurable)" -ForegroundColor White
Write-Host "  VSync: Disabled" -ForegroundColor White
Write-Host "  Optimizations: O3 + march=native + fast-math" -ForegroundColor White
Write-Host ""

# Set performance environment variables
$env:NOVA_TARGET_FPS = "400"
$env:NOVA_NO_ADAPTIVE_VSYNC = "1"

# Launch engine
Write-Host "Starting Nova Engine..." -ForegroundColor Green
.\nova-engine.exe

Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
