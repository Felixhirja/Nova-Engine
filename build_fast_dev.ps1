# Fast build script for Nova Engine development
# Uses minimal optimization for faster compilation

Write-Host "Building Nova Engine in FAST mode..." -ForegroundColor Green
$env:BUILD_MODE = "fast"
mingw32-make -j8 $args

if ($LASTEXITCODE -eq 0) {
    Write-Host "Fast build completed successfully!" -ForegroundColor Green
} else {
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}