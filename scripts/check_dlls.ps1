[CmdletBinding()]
param (
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$MsysBinDir
)

$requiredDlls = @(
    [PSCustomObject]@{ Name = 'glfw3.dll'; Optional = $false; Description = 'GLFW (graphics)' },
    [PSCustomObject]@{ Name = 'libfreeglut.dll'; Optional = $false; Description = 'FreeGLUT (OpenGL utility)' },
    [PSCustomObject]@{ Name = 'libgcc_s_seh-1.dll'; Optional = $false; Description = 'MinGW runtime' },
    [PSCustomObject]@{ Name = 'libstdc++-6.dll'; Optional = $false; Description = 'C++ standard library' },
    [PSCustomObject]@{ Name = 'libwinpthread-1.dll'; Optional = $false; Description = 'POSIX threads implementation' },
    [PSCustomObject]@{ Name = 'xinput1_4.dll'; Optional = $true; Description = 'XInput gamepad support' }
)

if (-not $MsysBinDir -or [string]::IsNullOrWhiteSpace($MsysBinDir)) {
    if ($env:MSYS2_MINGW64_BIN) {
        $MsysBinDir = $env:MSYS2_MINGW64_BIN
        Write-Verbose "Using MSYS2 bin directory from MSYS2_MINGW64_BIN: $MsysBinDir"
    }
    else {
        $MsysBinDir = 'C:\\msys64\\mingw64\\bin'
        Write-Verbose "MSYS2_MINGW64_BIN not set. Falling back to default: $MsysBinDir"
    }
}

if (-not (Test-Path -LiteralPath $ProjectRoot)) {
    throw "Project root '$ProjectRoot' does not exist."
}

if (-not (Test-Path -LiteralPath $MsysBinDir)) {
    Write-Warning "MSYS2 bin directory '$MsysBinDir' not found. Set the MSYS2_MINGW64_BIN environment variable or pass -MsysBinDir."
}

$missingDlls = @()
$copiedDlls = @()
$unresolvedDlls = @()

foreach ($dll in $requiredDlls) {
    $targetPath = Join-Path $ProjectRoot $dll.Name
    if (Test-Path -LiteralPath $targetPath) {
        Write-Verbose "Found $($dll.Name) in project directory."
        continue
    }

    $missingDlls += $dll
    $sourcePath = Join-Path $MsysBinDir $dll.Name

    if (Test-Path -LiteralPath $sourcePath) {
        try {
            Copy-Item -Path $sourcePath -Destination $targetPath -Force
            $copiedDlls += $dll
            Write-Host "Copied $($dll.Name) from MSYS2 to project directory." -ForegroundColor Green
        }
        catch {
            $unresolvedDlls += $dll
            Write-Warning "Failed to copy $($dll.Name): $($_.Exception.Message)"
        }
    }
    else {
        $unresolvedDlls += $dll
        $severity = if ($dll.Optional) { 'Warning' } else { 'Error' }
        Write-Warning "${severity}: $($dll.Name) not found in project directory or MSYS2 bin directory ($MsysBinDir)."
    }
}

if ($copiedDlls.Count -gt 0) {
    Write-Host "Successfully copied $($copiedDlls.Count) DLL(s): $([string]::Join(', ', $copiedDlls.Name))" -ForegroundColor Green
}

if ($missingDlls.Count -eq 0) {
    Write-Host 'All required DLLs are present.' -ForegroundColor Green
    exit 0
}

$requiredUnresolved = $unresolvedDlls | Where-Object { -not $_.Optional }
if ($requiredUnresolved.Count -gt 0) {
    Write-Error "Missing required DLLs: $([string]::Join(', ', $requiredUnresolved.Name)). Please install the corresponding MSYS2 packages or update -MsysBinDir."
    exit 1
}

if ($unresolvedDlls.Count -gt 0) {
    Write-Warning "Optional DLLs missing: $([string]::Join(', ', ($unresolvedDlls | Where-Object { $_.Optional }).Name))."
}

exit 0
