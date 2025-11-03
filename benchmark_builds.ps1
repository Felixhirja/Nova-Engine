# Build Speed Benchmark Script
# Measures compilation times for different build modes

Write-Host "Nova Engine Build Speed Benchmark" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan

# Clean build first
Write-Host "`nCleaning build artifacts..." -ForegroundColor Yellow
mingw32-make clean | Out-Null

# Benchmark Fast Mode
Write-Host "`n1. Testing FAST build mode..." -ForegroundColor Green
$fastStart = Get-Date
$env:BUILD_MODE = "fast"
mingw32-make -j8 | Out-Null
$fastEnd = Get-Date
$fastTime = ($fastEnd - $fastStart).TotalSeconds

# Clean for next test
mingw32-make clean | Out-Null

# Benchmark Unity Build
Write-Host "2. Testing UNITY build mode..." -ForegroundColor Green
$unityStart = Get-Date
$env:UNITY_BUILD = "1"
$env:BUILD_MODE = "fast"
mingw32-make -j8 | Out-Null
$unityEnd = Get-Date
$unityTime = ($unityEnd - $unityStart).TotalSeconds

# Clean for next test
mingw32-make clean | Out-Null
$env:UNITY_BUILD = ""

# Benchmark Release Mode
Write-Host "3. Testing RELEASE build mode..." -ForegroundColor Green
$releaseStart = Get-Date
$env:BUILD_MODE = ""
mingw32-make -j8 | Out-Null
$releaseEnd = Get-Date
$releaseTime = ($releaseEnd - $releaseStart).TotalSeconds

# Results
Write-Host "`nBuild Speed Results:" -ForegroundColor Cyan
Write-Host "===================" -ForegroundColor Cyan
Write-Host ("Release Mode: {0:F1} seconds (baseline)" -f $releaseTime) -ForegroundColor White
Write-Host ("Fast Mode:    {0:F1} seconds ({1:F1}x faster)" -f $fastTime, ($releaseTime / $fastTime)) -ForegroundColor Green
Write-Host ("Unity Build:  {0:F1} seconds ({1:F1}x faster)" -f $unityTime, ($releaseTime / $unityTime)) -ForegroundColor Green

Write-Host "`nRecommendations:" -ForegroundColor Yellow
Write-Host "- Use Fast Mode for daily development"
Write-Host "- Use Unity Build for clean builds"
Write-Host "- Use Release Mode for final testing"