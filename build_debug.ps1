# Debug build script with debugging symbols
# No optimization for easier debugging

Write-Host "Building Nova Engine in DEBUG mode..." -ForegroundColor Yellow
$env:BUILD_MODE = "debug"
mingw32-make -j8 $args

if ($LASTEXITCODE -eq 0) {
    Write-Host "Debug build completed successfully!" -ForegroundColor Green
    Write-Host "Debugging symbols included, optimizations disabled" -ForegroundColor Yellow
} else {
    Write-Host "Debug build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}