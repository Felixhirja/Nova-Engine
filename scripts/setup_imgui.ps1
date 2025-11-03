# Setup ImGui for Nova Engine
# This script downloads ImGui and sets it up for the build system

param(
    [string]$Version = "v1.90.4"  # Latest stable version
)

$ErrorActionPreference = "Stop"

Write-Host "Setting up ImGui $Version for Nova Engine..." -ForegroundColor Green

# Paths
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ImGuiDir = Join-Path $ProjectRoot "lib\imgui"
$ImGuiInclude = Join-Path $ImGuiDir "include"
$ImGuiSrc = Join-Path $ImGuiDir "src"
$TempDir = Join-Path $env:TEMP "imgui_setup"

# Clean up any existing setup
if (Test-Path $TempDir) {
    Remove-Item $TempDir -Recurse -Force
}

# Create directories
@($ImGuiDir, $ImGuiInclude, $ImGuiSrc, $TempDir) | ForEach-Object {
    if (!(Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
    }
}

try {
    Write-Host "Downloading ImGui $Version..." -ForegroundColor Yellow
    
    # Download ImGui from GitHub
    $DownloadUrl = "https://github.com/ocornut/imgui/archive/refs/tags/$Version.zip"
    $ZipPath = Join-Path $TempDir "imgui.zip"
    
    Invoke-WebRequest -Uri $DownloadUrl -OutFile $ZipPath -ProgressAction SilentlyContinue
    
    Write-Host "Extracting ImGui..." -ForegroundColor Yellow
    
    # Extract the zip
    Expand-Archive -Path $ZipPath -DestinationPath $TempDir -Force
    
    # Find the extracted directory (usually imgui-1.90.4 or similar)
    $ExtractedDir = Get-ChildItem $TempDir -Directory | Where-Object { $_.Name -like "imgui-*" } | Select-Object -First 1
    
    if (!$ExtractedDir) {
        throw "Could not find extracted ImGui directory"
    }
    
    Write-Host "Installing ImGui headers..." -ForegroundColor Yellow
    
    # Copy core headers
    $CoreHeaders = @(
        "imgui.h",
        "imconfig.h", 
        "imgui_internal.h",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h"
    )
    
    foreach ($header in $CoreHeaders) {
        $srcPath = Join-Path $ExtractedDir.FullName $header
        if (Test-Path $srcPath) {
            Copy-Item $srcPath $ImGuiInclude -Force
            Write-Host "  Copied $header" -ForegroundColor Gray
        }
    }
    
    # Copy backend headers (for GLFW and OpenGL)
    $BackendDir = Join-Path $ExtractedDir.FullName "backends"
    if (Test-Path $BackendDir) {
        $BackendHeaders = @(
            "imgui_impl_glfw.h",
            "imgui_impl_opengl3.h"
        )
        
        foreach ($header in $BackendHeaders) {
            $srcPath = Join-Path $BackendDir $header
            if (Test-Path $srcPath) {
                Copy-Item $srcPath $ImGuiInclude -Force
                Write-Host "  Copied $header" -ForegroundColor Gray
            }
        }
    }
    
    Write-Host "Installing ImGui source files..." -ForegroundColor Yellow
    
    # Copy core source files
    $CoreSources = @(
        "imgui.cpp",
        "imgui_demo.cpp",
        "imgui_draw.cpp", 
        "imgui_tables.cpp",
        "imgui_widgets.cpp"
    )
    
    foreach ($source in $CoreSources) {
        $srcPath = Join-Path $ExtractedDir.FullName $source
        if (Test-Path $srcPath) {
            Copy-Item $srcPath $ImGuiSrc -Force
            Write-Host "  Copied $source" -ForegroundColor Gray
        }
    }
    
    # Copy backend sources
    if (Test-Path $BackendDir) {
        $BackendSources = @(
            "imgui_impl_glfw.cpp",
            "imgui_impl_opengl3.cpp"
        )
        
        foreach ($source in $BackendSources) {
            $srcPath = Join-Path $BackendDir $source
            if (Test-Path $srcPath) {
                Copy-Item $srcPath $ImGuiSrc -Force
                Write-Host "  Copied $source" -ForegroundColor Gray
            }
        }
    }
    
    # Create a simple imgui_stdlib.h for std::string support
    $StdLibHeader = @"
// imgui_stdlib.h
// Wrappers for C++ standard library types
#pragma once

#include <string>
#include "imgui.h"

namespace ImGui {
    bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
}
"@
    
    $StdLibHeaderPath = Join-Path $ImGuiInclude "imgui_stdlib.h"
    Set-Content -Path $StdLibHeaderPath -Value $StdLibHeader -Encoding UTF8
    Write-Host "  Created imgui_stdlib.h" -ForegroundColor Gray
    
    # Create the corresponding .cpp file
    $StdLibSource = @"
// imgui_stdlib.cpp
// Wrappers for C++ standard library types
#include "imgui_stdlib.h"
#include <string>

namespace ImGui {
    struct InputTextCallback_UserData {
        std::string* Str;
        ImGuiInputTextCallback ChainCallback;
        void* ChainCallbackUserData;
    };

    static int InputTextCallback(ImGuiInputTextCallbackData* data) {
        InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* str = user_data->Str;
            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        } else if (user_data->ChainCallback) {
            data->UserData = user_data->ChainCallbackUserData;
            return user_data->ChainCallback(data);
        }
        return 0;
    }

    bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = str;
        cb_user_data.ChainCallback = callback;
        cb_user_data.ChainCallbackUserData = user_data;
        return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
    }

    bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = str;
        cb_user_data.ChainCallback = callback;
        cb_user_data.ChainCallbackUserData = user_data;
        return InputTextMultiline(label, (char*)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
    }

    bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = str;
        cb_user_data.ChainCallback = callback;
        cb_user_data.ChainCallbackUserData = user_data;
        return InputTextWithHint(label, hint, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
    }
}
"@
    
    $StdLibSourcePath = Join-Path $ImGuiSrc "imgui_stdlib.cpp"
    Set-Content -Path $StdLibSourcePath -Value $StdLibSource -Encoding UTF8
    Write-Host "  Created imgui_stdlib.cpp" -ForegroundColor Gray
    
    # Create a version info file
    $VersionInfo = @"
ImGui Version: $Version
Downloaded: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Source: https://github.com/ocornut/imgui
"@
    
    $VersionPath = Join-Path $ImGuiDir "VERSION"
    Set-Content -Path $VersionPath -Value $VersionInfo -Encoding UTF8
    
    Write-Host "ImGui setup completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Files installed:" -ForegroundColor Cyan
    Write-Host "  Headers: $(Get-ChildItem $ImGuiInclude -Name | Out-String)".Trim() -ForegroundColor Gray
    Write-Host "  Sources: $(Get-ChildItem $ImGuiSrc -Name | Out-String)".Trim() -ForegroundColor Gray
    Write-Host ""
    Write-Host "You can now build Nova Engine with ImGui support using 'mingw32-make'" -ForegroundColor Green

} catch {
    Write-Error "Failed to setup ImGui: $_"
    exit 1
} finally {
    # Clean up temp directory
    if (Test-Path $TempDir) {
        Remove-Item $TempDir -Recurse -Force
    }
}