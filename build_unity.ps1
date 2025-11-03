# Ultra-fast unity build script
# Combines source files for maximum compilation speed

Write-Host "Building Nova Engine with Unity Build..." -ForegroundColor Cyan
$env:UNITY_BUILD = "1"
$env:BUILD_MODE = "fast"
mingw32-make -j8 $args

if ($LASTEXITCODE -eq 0) {
    Write-Host "Unity build completed successfully!" -ForegroundColor Green
    Write-Host "Note: Unity builds compile faster but may have longer link times" -ForegroundColor Yellow
} else {
    Write-Host "Unity build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}