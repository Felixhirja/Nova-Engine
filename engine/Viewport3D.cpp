#include "Viewport3D.h"
#include "TextRenderer.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
#include <utility>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <functional>
#include <sstream>
#include <array>
#include "SVGSurfaceLoader.h"
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(USE_GLFW) || defined(USE_SDL)
#include <glad/glad.h>
#include "graphics/MeshSubmission.h"
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif
#ifdef USE_SDL
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#include "sdl_compat.h"
#elif defined(USE_SDL2)
#include <SDL2/SDL_syswm.h>
#include "sdl_compat.h"
#else
#include <SDL.h>
#include "sdl_compat.h"
#endif
#include "Camera.h"
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#include "Camera.h"
#endif
#include "VisualFeedbackSystem.h"
#if defined(USE_GLFW) || defined(USE_SDL)
#include "graphics/ParticleRenderer.h"
#include "graphics/PrimitiveMesh.h"
#endif

struct Viewport3D::PrimitiveBuffers {
    PrimitiveBuffers() = default;
    GLuint playerVBO = 0;
    GLsizei playerVertexCount = 0;
    GLuint cubeVBO = 0;
    GLsizei cubeVertexCount = 0;
};

namespace {

const ViewportLayout& DefaultViewportLayoutFallback() {
    static const ViewportLayout fallback = []() -> ViewportLayout {
        ViewportLayout layout;
        layout.name = "Single View";
        ViewportView primary;
        primary.name = "Primary";
        primary.normalizedX = 0.0;
        primary.normalizedY = 0.0;
        primary.normalizedWidth = 1.0;
        primary.normalizedHeight = 1.0;
        primary.role = ViewRole::Main;
        primary.overlay = false;
        layout.views.push_back(primary);
        return layout;
    }();
    return fallback;
}

const char* RenderBackendToString(RenderBackend backend) {
    switch (backend) {
    case RenderBackend::None: return "None";
    case RenderBackend::SDL_GL: return "SDL_GL";
    case RenderBackend::SDL_Renderer: return "SDL_Renderer";
    case RenderBackend::GLFW_GL: return "GLFW_GL";
    }
    return "Unknown";
}

std::string DescribeGlError(GLenum err) {
#if defined(GLU_VERSION_1_1) || defined(GLU_VERSION)
    const GLubyte* gluMessage = gluErrorString(err);
    if (gluMessage) {
        return reinterpret_cast<const char*>(gluMessage);
    }
#endif
    switch (err) {
#ifdef GL_NO_ERROR
    case GL_NO_ERROR: return "GL_NO_ERROR";
#endif
#ifdef GL_INVALID_ENUM
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
#endif
#ifdef GL_INVALID_VALUE
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
#endif
#ifdef GL_INVALID_OPERATION
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
#endif
#ifdef GL_STACK_OVERFLOW
    case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
    case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
#endif
#ifdef GL_OUT_OF_MEMORY
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#endif
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
    default:
        break;
    }
    std::ostringstream oss;
    oss << "0x" << std::hex << err;
    return oss.str();
}

struct SvgCacheHeader {
    char magic[4];
    std::uint32_t version;
    std::uint32_t width;
    std::uint32_t height;
};

constexpr std::uint32_t kSvgCacheVersion = 1;

std::filesystem::path BuildSvgCachePath(const std::filesystem::path& svgPath,
                                        const SvgRasterizationOptions& options) {
    namespace fs = std::filesystem;
    fs::path svgAbsolute = svgPath;
    fs::path cacheDir = svgAbsolute.parent_path() / "cache";
    std::ostringstream key;
    key << svgAbsolute.string()
        << "|w=" << options.targetWidth
        << "|h=" << options.targetHeight
        << "|scale=" << options.scale
        << "|aspect=" << (options.preserveAspectRatio ? 1 : 0);
    std::size_t hashValue = std::hash<std::string>{}(key.str());
    std::ostringstream fileName;
    fileName << svgAbsolute.stem().string() << "_" << std::hex << hashValue << ".rgba";
    return cacheDir / fileName.str();
}

bool TryLoadSvgCache(const std::filesystem::path& cachePath,
                     std::vector<std::uint8_t>& outPixels,
                     int& outWidth,
                     int& outHeight) {
    namespace fs = std::filesystem;
    std::ifstream in(cachePath, std::ios::binary);
    if (!in) {
        return false;
    }

    SvgCacheHeader header{};
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        return false;
    }

    if (std::memcmp(header.magic, "SVGC", 4) != 0 || header.version != kSvgCacheVersion) {
        return false;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(header.width) * static_cast<std::size_t>(header.height);
    const std::size_t expectedBytes = pixelCount * 4;

    in.seekg(0, std::ios::end);
    const std::streamoff fileSize = in.tellg();
    if (fileSize < static_cast<std::streamoff>(sizeof(header)) + static_cast<std::streamoff>(expectedBytes)) {
        return false;
    }
    in.seekg(sizeof(header), std::ios::beg);

    outPixels.resize(expectedBytes);
    if (!in.read(reinterpret_cast<char*>(outPixels.data()), static_cast<std::streamsize>(expectedBytes))) {
        return false;
    }

    outWidth = static_cast<int>(header.width);
    outHeight = static_cast<int>(header.height);
    return true;
}

void SaveSvgCache(const std::filesystem::path& cachePath,
                  const std::vector<std::uint8_t>& pixels,
                  int width,
                  int height) {
    namespace fs = std::filesystem;
    SvgCacheHeader header{{'S','V','G','C'}, kSvgCacheVersion,
                          static_cast<std::uint32_t>(width),
                          static_cast<std::uint32_t>(height)};

    const fs::path parentDir = cachePath.parent_path();
    if (!parentDir.empty()) {
        std::error_code ec;
        fs::create_directories(parentDir, ec);
    }

    std::ofstream out(cachePath, std::ios::binary);
    if (!out) {
        return;
    }

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!out) {
        return;
    }

    out.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
}

bool LoadSvgToRgbaCached(const std::string& svgPath,
                         std::vector<std::uint8_t>& outPixels,
                         int& outWidth,
                         int& outHeight,
                         SvgRasterizationOptions options) {
    namespace fs = std::filesystem;
    std::error_code absEc;
    fs::path svgAbsolute = fs::absolute(svgPath, absEc);
    if (absEc) {
        svgAbsolute = fs::path(svgPath);
    }
    const fs::path cachePath = BuildSvgCachePath(svgAbsolute, options);

    std::error_code svgEc;
    const auto svgWriteTime = fs::last_write_time(svgAbsolute, svgEc);

    std::error_code cacheEc;
    if (fs::exists(cachePath, cacheEc)) {
        const auto cacheWriteTime = fs::last_write_time(cachePath, cacheEc);
        if (!cacheEc && !svgEc && cacheWriteTime >= svgWriteTime) {
            if (TryLoadSvgCache(cachePath, outPixels, outWidth, outHeight)) {
                return true;
            }
        }
    }

    if (!LoadSvgToRgba(svgPath, outPixels, outWidth, outHeight, options)) {
        return false;
    }

    SaveSvgCache(cachePath, outPixels, outWidth, outHeight);
    return true;
}

} // namespace

// OpenGL debug callback function for GPU validation
#ifndef NDEBUG
void APIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const void* userParam) {
    (void)length; (void)userParam; // Suppress unused parameter warnings

    // Filter out some common non-critical messages to reduce noise
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // Buffer performance hints
    if (id == 131154) return; // Pixel-path performance warning
    // Filter out GL_STACK_OVERFLOW errors which appear to be false positives with debug groups
    if (type == GL_DEBUG_TYPE_ERROR && id == 1283) return; // GL_STACK_OVERFLOW

    // Convert enums to readable strings
    const char* sourceStr = "Unknown";
    switch (source) {
        case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER: sourceStr = "Other"; break;
    }

    const char* typeStr = "Unknown";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY: typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER: typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: typeStr = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER: typeStr = "Other"; break;
    }

    const char* severityStr = "Unknown";
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: severityStr = "High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severityStr = "Medium"; break;
        case GL_DEBUG_SEVERITY_LOW: severityStr = "Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Notification"; break;
    }

    // Output to console with appropriate stream
    if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR) {
        std::cerr << "[OpenGL Debug] " << severityStr << " " << typeStr << " from " << sourceStr
                  << " (ID: " << id << "): " << message << std::endl;
    } else {
        std::cout << "[OpenGL Debug] " << severityStr << " " << typeStr << " from " << sourceStr
                  << " (ID: " << id << "): " << message << std::endl;
    }
}
#endif

#ifndef NDEBUG
// Debug-draw configuration (file-scope, editable at runtime if you wire inputs)
namespace {
    bool g_ShowWorldAxes = true;              // Toggle world-origin axes in 3D
    bool g_ShowMiniAxesGizmo = false;         // Toggle 2D mini axes gizmo in HUD (default off)
    float g_WorldAxisLength = 10.0f;          // Length of world axes in scene units
    float g_WorldAxisLineWidth = 3.0f;        // Line width for world axes
    float g_MiniGizmoSize = 56.0f;            // Pixel length of mini gizmo X/Y arms
    float g_MiniGizmoThickness = 3.0f;        // Pixel thickness for mini gizmo arms
    float g_MiniGizmoMargin = 16.0f;          // Margin from screen corner
}

bool Viewport3D::IsWorldAxesShown() const { return g_ShowWorldAxes; }
bool Viewport3D::IsMiniAxesGizmoShown() const { return g_ShowMiniAxesGizmo; }
void Viewport3D::ToggleWorldAxes() { g_ShowWorldAxes = !g_ShowWorldAxes; }
void Viewport3D::ToggleMiniAxesGizmo() { g_ShowMiniAxesGizmo = !g_ShowMiniAxesGizmo; }
#endif

#if defined(USE_GLFW)
namespace {

struct Color4 {
    float r;
    float g;
    float b;
    float a;
};

inline Color4 MakeColor(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

Color4 StatusColor(double percent, bool recharging) {
    if (recharging) {
        return MakeColor(0.3f, 0.6f, 1.0f, 1.0f);
    }
    if (percent >= 0.75) {
        return MakeColor(0.2f, 0.85f, 0.4f, 1.0f);
    }
    if (percent >= 0.5) {
        return MakeColor(0.95f, 0.8f, 0.25f, 1.0f);
    }
    if (percent >= 0.25) {
        return MakeColor(0.95f, 0.55f, 0.1f, 1.0f);
    }
    return MakeColor(0.9f, 0.2f, 0.2f, 1.0f);
}

Color4 WarningColorForLabel(const std::string& warning) {
    if (warning.find("Power") != std::string::npos) {
        return MakeColor(0.9f, 0.25f, 0.25f, 1.0f);
    }
    if (warning.find("Shield") != std::string::npos) {
        return MakeColor(0.95f, 0.55f, 0.15f, 1.0f);
    }
    if (warning.find("Overload") != std::string::npos) {
        return MakeColor(0.95f, 0.8f, 0.25f, 1.0f);
    }
    return MakeColor(0.6f, 0.8f, 0.95f, 1.0f);
}

struct HudAnchorRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool valid = false;
};

void DrawQuad2D(UIBatcher* batch, float x, float y, float w, float h, const Color4& color) {
    if (!batch) return; // UIBatcher is required for 2D UI rendering
    batch->AddQuad(x, y, w, h, color.r, color.g, color.b, color.a);
}

void DrawBorder2D(UIBatcher* batch, float x, float y, float w, float h, const Color4& color, float thickness = 1.0f) {
    if (!batch) return; // UIBatcher is required for 2D UI rendering
    batch->AddRectOutline(x, y, w, h, thickness, color.r, color.g, color.b, color.a);
}

float Clamp01(double value) {
    if (value < 0.0) return 0.0f;
    if (value > 1.0) return 1.0f;
    return static_cast<float>(value);
}

void DrawFillBar(UIBatcher* batch, float x, float y, float w, float h, double fillAmount, const Color4& fillColor) {
    DrawQuad2D(batch, x, y, w, h, MakeColor(0.1f, 0.1f, 0.14f, 0.9f));
    float fill = Clamp01(fillAmount);
    if (fill > 0.0f) {
        DrawQuad2D(batch, x, y, w * fill, h, fillColor);
    }
    DrawBorder2D(batch, x, y, w, h, MakeColor(0.35f, 0.35f, 0.4f, 0.9f));
}

void RenderEnergyPanel(UIBatcher* batch,
                       const EnergyHUDTelemetry& telemetry,
                       int screenWidth,
                       int screenHeight,
                       const HudAnchorRect* anchor) {
    (void)screenHeight; // currently unused
    const float nativeWidth = 420.0f;
    const float nativeHeight = 300.0f;
    const float nativeMargin = 18.0f;

    float panelWidth = nativeWidth;
    float panelHeight = nativeHeight;
    float margin = nativeMargin;
    float panelX = static_cast<float>(screenWidth) - panelWidth - margin;
    float panelY = margin;
    bool useAnchor = anchor && anchor->valid;

    if (useAnchor) {
        const float anchorMargin = 12.0f;
        const float maxWidth = std::max(120.0f, anchor->width - anchorMargin * 2.0f);
        const float maxHeight = std::max(120.0f, anchor->height - anchorMargin * 2.0f);
        panelWidth = std::min(nativeWidth, maxWidth);
        panelHeight = std::min(nativeHeight, maxHeight);
        panelX = anchor->x + (anchor->width - panelWidth) * 0.5f;
        panelY = anchor->y + (anchor->height - panelHeight) * 0.5f;
        margin = nativeMargin;
    }

    const Color4 panelBg = useAnchor ? MakeColor(0.01f, 0.02f, 0.05f, 0.6f)
                                     : MakeColor(0.02f, 0.02f, 0.04f, 0.82f);
    const Color4 panelBorder = useAnchor ? MakeColor(0.25f, 0.45f, 0.75f, 0.75f)
                                         : MakeColor(0.45f, 0.55f, 0.75f, 0.8f);

    DrawQuad2D(batch, panelX, panelY, panelWidth, panelHeight, panelBg);
    DrawBorder2D(batch, panelX, panelY, panelWidth, panelHeight, panelBorder);

    TextRenderer::RenderText("SHIP STATUS HUD",
                             static_cast<int>(panelX + 18.0f),
                             static_cast<int>(panelY + 28.0f),
                             TextColor::Cyan(),
                             FontSize::Large);

    const float boxTop = panelY + 52.0f;
    const float boxHeight = 92.0f;
    const float boxGap = 12.0f;
    const float boxWidth = (panelWidth - (margin * 2.0f) - (boxGap * 2.0f)) / 3.0f;

    auto drawSubsystemBox = [&](float boxIndex,
                                const char* label,
                                double percent,
                                double delivered,
                                double requirement,
                                double value,
                                double valueMax,
                                const char* valueUnits,
                                double auxValue,
                                const char* auxLabel,
                                bool rechargingHighlight) {
        float bx = panelX + margin + boxIndex * (boxWidth + boxGap);
        float by = boxTop;
        DrawQuad2D(batch, bx, by, boxWidth, boxHeight, MakeColor(0.05f, 0.05f, 0.09f, 0.85f));
        DrawBorder2D(batch, bx, by, boxWidth, boxHeight, MakeColor(0.25f, 0.35f, 0.55f, 0.9f));

        TextRenderer::RenderText(label,
                                 static_cast<int>(bx + 12.0f),
                                 static_cast<int>(by + 20.0f),
                                 TextColor::White(),
                                 FontSize::Small);

        Color4 fillColor = StatusColor(percent, rechargingHighlight);
        DrawFillBar(batch, bx + 12.0f, by + 32.0f, boxWidth - 24.0f, 12.0f, percent, fillColor);
        TextRenderer::RenderTextF(static_cast<int>(bx + boxWidth - 60.0f),
                                  static_cast<int>(by + 32.0f),
                                  TextColor::Gray(0.9f),
                                  FontSize::Small,
                                  "%02.0f%%",
                                  percent * 100.0);

        if (valueUnits && valueUnits[0] != '\0') {
            TextRenderer::RenderTextF(static_cast<int>(bx + 12.0f),
                                      static_cast<int>(by + 58.0f),
                                      TextColor::Gray(0.85f),
                                      FontSize::Small,
                                      "%0.1f/%0.1f %s",
                                      value,
                                      valueMax,
                                      valueUnits);
        }

        if (requirement > 0.0) {
            TextRenderer::RenderTextF(static_cast<int>(bx + 12.0f),
                                      static_cast<int>(by + 74.0f),
                                      TextColor::Gray(0.7f),
                                      FontSize::Small,
                                      "%0.1f/%0.1f MW",
                                      delivered,
                                      requirement);
        }

        if (auxLabel && auxLabel[0] != '\0') {
            TextRenderer::RenderTextF(static_cast<int>(bx + 12.0f),
                                      static_cast<int>(by + 86.0f),
                                      TextColor::Gray(0.75f),
                                      FontSize::Small,
                                      "%s %0.1f",
                                      auxLabel,
                                      auxValue);
        }
    };

    drawSubsystemBox(0.0f,
                     "SHIELDS",
                     telemetry.shieldPercent,
                     telemetry.shieldDeliveredMW,
                     telemetry.shieldRequirementMW,
                     telemetry.shieldCapacityMJ,
                     telemetry.shieldCapacityMaxMJ,
                     "MJ",
                     telemetry.shieldRechargeRateMJ,
                     telemetry.warningRechargeDelay ? "RECH" : "+",
                     telemetry.shieldRechargeRemaining <= 0.0 && telemetry.shieldPercent < 1.0);

    drawSubsystemBox(1.0f,
                     "WEAPONS",
                     telemetry.weaponPercent,
                     telemetry.weaponDeliveredMW,
                     telemetry.weaponRequirementMW,
                     telemetry.weaponAmmoCurrent >= 0 ? static_cast<double>(telemetry.weaponAmmoCurrent) : telemetry.weaponDeliveredMW,
                     telemetry.weaponAmmoMax >= 0 ? static_cast<double>(telemetry.weaponAmmoMax) : telemetry.weaponRequirementMW,
                     telemetry.weaponAmmoMax >= 0 ? "AMMO" : "MW",
                     telemetry.weaponCooldownSeconds,
                     telemetry.weaponCooldownSeconds > 0.0 ? "CD" : "",
                     false);

    drawSubsystemBox(2.0f,
                     "THRUSTERS",
                     telemetry.thrusterPercent,
                     telemetry.thrusterDeliveredMW,
                     telemetry.thrusterRequirementMW,
                     telemetry.thrustToMass,
                     telemetry.thrustToMass,
                     telemetry.thrustToMass > 0.0 ? "T/M" : "MW",
                     telemetry.thrustToMass,
                     telemetry.thrustToMass > 0.0 ? "T/M" : "",
                     false);

    const float allocationTop = boxTop + boxHeight + 26.0f;
    const float warningColumnX = panelX + panelWidth - 150.0f;

    TextRenderer::RenderText("POWER ALLOCATION",
                             static_cast<int>(panelX + margin),
                             static_cast<int>(allocationTop - 8.0f),
                             TextColor::Gray(0.85f),
                             FontSize::Small);
    TextRenderer::RenderText("WARNINGS",
                             static_cast<int>(warningColumnX),
                             static_cast<int>(allocationTop - 8.0f),
                             TextColor::Gray(0.85f),
                             FontSize::Small);

    auto drawAllocationRow = [&](float rowIndex,
                                 const char* name,
                                 double allocation,
                                 double delivered,
                                 double requirement) {
        float rowY = allocationTop + rowIndex * 34.0f;
        TextRenderer::RenderText(name,
                                 static_cast<int>(panelX + margin),
                                 static_cast<int>(rowY),
                                 TextColor::White(),
                                 FontSize::Small);
        float barX = panelX + margin + 90.0f;
        float barWidth = warningColumnX - barX - 12.0f;
        DrawFillBar(batch, barX, rowY - 12.0f, barWidth, 12.0f, allocation, MakeColor(0.35f, 0.75f, 0.95f, 0.9f));
        TextRenderer::RenderTextF(static_cast<int>(barX + barWidth + 6.0f),
                                  static_cast<int>(rowY),
                                  TextColor::Gray(0.9f),
                                  FontSize::Small,
                                  "%02.0f%%",
                                  allocation * 100.0);
        if (requirement > 0.0) {
            TextRenderer::RenderTextF(static_cast<int>(barX),
                                      static_cast<int>(rowY + 12.0f),
                                      TextColor::Gray(0.7f),
                                      FontSize::Small,
                                      "%0.1f/%0.1f MW",
                                      delivered,
                                      requirement);
        }
    };

    drawAllocationRow(0.0f, "Shields", telemetry.shieldAllocation, telemetry.shieldDeliveredMW, telemetry.shieldRequirementMW);
    drawAllocationRow(1.0f, "Weapons", telemetry.weaponAllocation, telemetry.weaponDeliveredMW, telemetry.weaponRequirementMW);
    drawAllocationRow(2.0f, "Thrusters", telemetry.thrusterAllocation, telemetry.thrusterDeliveredMW, telemetry.thrusterRequirementMW);

    float warningY = allocationTop + 4.0f;
    if (telemetry.warnings.empty()) {
        TextRenderer::RenderText("All systems nominal",
                                 static_cast<int>(warningColumnX),
                                 static_cast<int>(warningY),
                                 TextColor::Gray(0.6f),
                                 FontSize::Small);
    } else {
        for (const auto& warning : telemetry.warnings) {
            Color4 warnColor = WarningColorForLabel(warning);
            TextRenderer::RenderText(warning,
                                     static_cast<int>(warningColumnX),
                                     static_cast<int>(warningY),
                                     TextColor{warnColor.r, warnColor.g, warnColor.b, warnColor.a},
                                     FontSize::Small);
            warningY += 18.0f;
        }
    }

    if (!telemetry.activePreset.empty()) {
        TextRenderer::RenderTextF(static_cast<int>(panelX + margin),
                                  static_cast<int>(allocationTop + 118.0f),
                                  TextColor::Gray(0.85f),
                                  FontSize::Small,
                                  "Preset: %s",
                                  telemetry.activePreset.c_str());
    }

    const double usedPower = telemetry.totalPowerOutputMW - telemetry.netPowerMW;
    const int netY = static_cast<int>(panelY + panelHeight - 42.0f);
    TextRenderer::RenderTextF(static_cast<int>(panelX + margin),
                              netY,
                              telemetry.netPowerMW < 0.0 ? TextColor::Red() : TextColor::White(),
                              FontSize::Medium,
                              "NET POWER: %.1f/%.1f MW",
                              std::max(0.0, usedPower),
                              telemetry.totalPowerOutputMW);
    TextRenderer::RenderTextF(static_cast<int>(panelX + margin),
                              netY + 18,
                              TextColor::Gray(0.85f),
                              FontSize::Small,
                              "EFFICIENCY: %.0f%%  DRAIN: %.1f MW",
                              telemetry.efficiencyPercent,
                              telemetry.drainRateMW);
}

// Lightweight player HUD elements (reticle + bottom status rail)
void DrawReticle2D(UIBatcher* batch, int screenWidth, int screenHeight, float scaleFactor) {
    if (!batch) return;
    const float cx = static_cast<float>(screenWidth) * 0.5f;
    const float cy = static_cast<float>(screenHeight) * 0.5f;
    const float clampedScale = std::clamp(scaleFactor, 0.6f, 1.6f);
    const float len = 14.0f * clampedScale;
    const float gap = 6.0f * clampedScale;
    const float thick = 2.0f * std::max(0.75f, clampedScale * 0.9f);
    const Color4 c = MakeColor(0.95f, 0.95f, 0.98f, 0.95f);
    // Horizontal
    DrawQuad2D(batch, cx - len - gap, cy - thick * 0.5f, len, thick, c);
    DrawQuad2D(batch, cx + gap,        cy - thick * 0.5f, len, thick, c);
    // Vertical
    DrawQuad2D(batch, cx - thick * 0.5f, cy - len - gap, thick, len, c);
    DrawQuad2D(batch, cx - thick * 0.5f, cy + gap,        thick, len, c);
}

void RenderPlayerStatusRail(UIBatcher* batch,
                            const EnergyHUDTelemetry* tel,
                            int screenWidth,
                            int screenHeight,
                            double speedUnitsPerSec,
                            int ammoCurrent,
                            int ammoMax,
                            const HudAnchorRect* anchor) {
    if (!batch) return;
    float margin = 10.0f;
    float railH = 56.0f;
    float railW = std::max(360.0f, static_cast<float>(screenWidth) * 0.6f);
    float railX = (static_cast<float>(screenWidth) - railW) * 0.5f;
    float railY = static_cast<float>(screenHeight) - railH - margin;
    Color4 railBackground = MakeColor(0.02f, 0.02f, 0.03f, 0.78f);
    Color4 railBorder = MakeColor(0.25f, 0.35f, 0.55f, 0.85f);

    if (anchor && anchor->valid) {
    const float anchorPadding = 24.0f;
    const float innerWidth = std::max(anchor->width - anchorPadding * 2.0f, 180.0f);
    const float innerHeight = std::max(anchor->height - anchorPadding * 2.0f, 48.0f);
    railW = std::min(std::max(railW, 420.0f), innerWidth);
    railH = std::min(std::max(railH, 48.0f), innerHeight);
    railX = anchor->x + (anchor->width - railW) * 0.5f;
    railY = anchor->y + anchor->height - railH - anchorPadding * 0.5f;
        margin = 12.0f;
        railBackground = MakeColor(0.01f, 0.02f, 0.05f, 0.6f);
        railBorder = MakeColor(0.2f, 0.35f, 0.55f, 0.7f);
    }

    // Background
    DrawQuad2D(batch, railX, railY, railW, railH, railBackground);
    DrawBorder2D(batch, railX, railY, railW, railH, railBorder, 1.2f);

    const float pad = (anchor && anchor->valid) ? 18.0f : 12.0f;
    const float colGap = (anchor && anchor->valid) ? 18.0f : 12.0f;
    const float colW = (railW - pad * 2.0f - colGap * 2.0f) / 3.0f;
    const float colY = railY + 8.0f;
    const float barH = railH - 26.0f;

    auto drawLabeledBar = [&](float idx, const char* label, double pct, const Color4& fill, const char* rightText){
        const float x = railX + pad + idx * (colW + colGap);
        TextRenderer::RenderText(label, static_cast<int>(x), static_cast<int>(railY + 18.0f), TextColor::Gray(0.85f), FontSize::Small);
        DrawFillBar(batch, x, colY + 10.0f, colW, barH - 8.0f, pct, fill);
        if (rightText && rightText[0] != '\0') {
            TextRenderer::RenderTextAligned(rightText,
                                            static_cast<int>(x + colW - 2.0f),
                                            static_cast<int>(railY + 18.0f),
                                            TextAlign::Right,
                                            TextColor::Gray(0.9f),
                                            FontSize::Small);
        }
    };

    // HEALTH from shields percent if available; otherwise full
    double healthPct = tel ? std::clamp(tel->shieldPercent, 0.0, 1.0) : 1.0;
    drawLabeledBar(0.0f, "HEALTH", healthPct, StatusColor(healthPct, false), "");

    // ENERGY from efficiency percent if available
    double energyPct = tel ? std::clamp(tel->efficiencyPercent / 100.0, 0.0, 1.0) : 1.0;
    char spd[32] = {0};
    snprintf(spd, sizeof(spd), "SPD %.1f", speedUnitsPerSec);
    drawLabeledBar(1.0f, "ENERGY", energyPct, MakeColor(0.3f, 0.75f, 0.95f, 0.95f), spd);

    // AMMO from weapon ammo if available
    double ammoPct = 1.0;
    char ammoTxt[32] = {0};
    if (ammoMax > 0) {
        ammoPct = std::clamp(static_cast<double>(ammoCurrent) / static_cast<double>(ammoMax), 0.0, 1.0);
    }
    if (ammoCurrent >= 0 && ammoMax > 0) {
        snprintf(ammoTxt, sizeof(ammoTxt), "%d/%d", ammoCurrent, ammoMax);
    } else if (ammoCurrent >= 0) {
        snprintf(ammoTxt, sizeof(ammoTxt), "%d", ammoCurrent);
    } else {
        snprintf(ammoTxt, sizeof(ammoTxt), "N/A");
    }
    drawLabeledBar(2.0f, "AMMO", ammoPct, MakeColor(0.95f, 0.75f, 0.25f, 0.95f), ammoTxt);
}

} // namespace
#endif // defined(USE_GLFW)

void ParticleRendererDeleter::operator()(ParticleRenderer* ptr) const {
#if defined(USE_GLFW) || defined(USE_SDL)
    delete ptr;
#else
    (void)ptr;
#endif
}

Viewport3D::Viewport3D()
    : width(800)
    , height(600)
    , backend_(RenderBackend::None)
    , vsyncEnabled_(false)
    , frameRateLimitHint_(144.0)
    , debugLogging_(false)
    , aggressiveFocus_(false)
#ifdef USE_SDL
    , sdlWindow(nullptr)
    , sdlRenderer(nullptr)
    , sdlGLContext(nullptr)
    , spaceshipHudTexture_(nullptr)
    , spaceshipHudTextureWidth_(0)
    , spaceshipHudTextureHeight_(0)
    , spaceshipHudTextureFailed_(false)
#endif
#ifdef USE_GLFW
    , glfwWindow(nullptr)
#endif
    , activeLayoutIndex_(0)
    , uiBatcher_(nullptr)
#if defined(USE_GLFW) || defined(USE_SDL)
    , playerHudTextureGL_(0)
    , playerHudTextureGLWidth_(0)
    , playerHudTextureGLHeight_(0)
    , playerHudTextureGLFailed_(false)
    , playerMesh_()
    , playerMeshInitialized_(false)
#endif
{
    // ctor trace
    try {
        std::ofstream f("v3d_ctor.log", std::ios::app);
        if (f) f << "Viewport3D ctor begin" << std::endl;
    } catch (...) {}
}

Viewport3D::~Viewport3D() {
    try {
        std::ofstream f("v3d_ctor.log", std::ios::app);
        if (f) f << "Viewport3D dtor" << std::endl;
    } catch (...) {}
#if defined(USE_GLFW) || defined(USE_SDL)
    if (shaderManager_) {
        if (IsUsingGLBackend()) {
            shaderManager_->Clear();
        }
        shaderManager_.reset();
    }
#endif
}

void Viewport3D::EnsurePrimitiveBuffers() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
    if (!primitiveBuffers_) {
        primitiveBuffers_ = std::make_unique<PrimitiveBuffers>();
    }
    PrimitiveBuffers& buffers = *primitiveBuffers_;

    if (buffers.playerVBO == 0) {
        struct VertexPC {
            float px;
            float py;
            float pz;
            float r;
            float g;
            float b;
        };

        const VertexPC playerVertices[] = {
            {-1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
            { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
            {-1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f},

            {-1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f},
            {-1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f},
            {-1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f},

            {-1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f},
            {-1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f},
            { 1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f},
            {-1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f},

            {-0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
            { 0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
            { 0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
            {-0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
            { 0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
            {-0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f},
        };

        glGenBuffers(1, &buffers.playerVBO);
        glBindBuffer(GL_ARRAY_BUFFER, buffers.playerVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(playerVertices), playerVertices, GL_STATIC_DRAW);
        buffers.playerVertexCount = static_cast<GLsizei>(sizeof(playerVertices) / sizeof(playerVertices[0]));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (buffers.cubeVBO == 0) {
        struct VertexP {
            float px;
            float py;
            float pz;
        };

        const VertexP cubeVertices[] = {
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},

            {-0.5f, -0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},

            {-0.5f, -0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},

            { 0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f,  0.5f},

            {-0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},

            {-0.5f, -0.5f,  0.5f},
            {-0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f,  0.5f},
        };

        glGenBuffers(1, &buffers.cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, buffers.cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
        buffers.cubeVertexCount = static_cast<GLsizei>(sizeof(cubeVertices) / sizeof(cubeVertices[0]));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
#else
    (void)primitiveBuffers_;
#endif
}

void Viewport3D::EnsureCubePrimitive() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) return;
    if (cubePrimitive_ && cubePrimitive_->IsInitialized()) return;
    // Build simple position-only cube vertex list matching the legacy cube VBO topology
    std::vector<float> verts = {
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
    };

    std::vector<unsigned int> idx; // draw arrays; indices empty
    cubePrimitive_ = std::make_unique<PrimitiveMesh>();
    cubePrimitive_->Upload(verts, idx, 3 * static_cast<int>(sizeof(float)), false);
#else
    (void)cubePrimitive_;
#endif
}

void Viewport3D::EnsurePlayerPatchPrimitive() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) return;
    if (playerPatchPrimitive_ && playerPatchPrimitive_->IsInitialized()) return;
    // Build vertices matching the legacy playerVBO layout (px,py,pz,r,g,b)
    std::vector<float> verts = {
        -1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,

        -1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f,
        -1.0f,  0.8f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 1.01f, 1.0f, 0.0f, 0.0f,

        -1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 1.01f, 1.0f, 0.0f, 0.0f,
         1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f,
        -1.0f, -0.8f, 1.01f, 1.0f, 0.0f, 0.0f,

        -0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
         0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
         0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
        -0.2f, -0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
         0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
        -0.2f,  0.2f, 1.02f, 0.0f, 0.0f, 1.0f,
    };

    std::vector<unsigned int> idx; // draw arrays; indices empty
    playerPatchPrimitive_ = std::make_unique<PrimitiveMesh>();
    playerPatchPrimitive_->Upload(verts, idx, 6 * static_cast<int>(sizeof(float)), true);
#else
    (void)playerPatchPrimitive_;
#endif
}

void Viewport3D::EnsureHudTexturePrimitive(float x, float y, float width, float height) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
    if (!hudTexturePrimitive_) {
        hudTexturePrimitive_ = std::make_unique<PrimitiveMesh>();
        hudTexturePrimitiveDirty_ = true;
    }

    auto approximatelyEqual = [](float a, float b) {
        return std::fabs(a - b) <= 0.0005f;
    };

    if (!hudTexturePrimitiveDirty_ &&
        approximatelyEqual(x, hudTextureLastX_) &&
        approximatelyEqual(y, hudTextureLastY_) &&
        approximatelyEqual(width, hudTextureLastWidth_) &&
        approximatelyEqual(height, hudTextureLastHeight_)) {
        return;
    }

    std::vector<float> verts = {
        x,           y,            0.0f, 0.0f, 0.0f,
        x + width,   y,            0.0f, 1.0f, 0.0f,
        x + width,   y + height,   0.0f, 1.0f, 1.0f,
        x,           y + height,   0.0f, 0.0f, 1.0f,
    };

    std::vector<unsigned int> idx = {0, 1, 2, 0, 2, 3};

    hudTexturePrimitive_->Upload(
        verts,
        idx,
        5 * static_cast<int>(sizeof(float)),
        false,
        0,
        true,
        3 * static_cast<int>(sizeof(float)),
        2);

    hudTextureLastX_ = x;
    hudTextureLastY_ = y;
    hudTextureLastWidth_ = width;
    hudTextureLastHeight_ = height;
    hudTexturePrimitiveDirty_ = false;
#else
    (void)x; (void)y; (void)width; (void)height;
#endif
}

void Viewport3D::DestroyPrimitiveBuffers() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!primitiveBuffers_) {
        return;
    }
#if defined(USE_SDL)
    if (sdlWindow && sdlGLContext) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    else if (glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif

    PrimitiveBuffers& buffers = *primitiveBuffers_;
    if (buffers.playerVBO != 0) {
        glDeleteBuffers(1, &buffers.playerVBO);
        buffers.playerVBO = 0;
    }
    if (buffers.cubeVBO != 0) {
        glDeleteBuffers(1, &buffers.cubeVBO);
        buffers.cubeVBO = 0;
    }
    primitiveBuffers_.reset();
#else
    (void)primitiveBuffers_;
#endif
}

void Viewport3D::DrawPlayerPatchPrimitive() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
    // Prefer retained-mode PrimitiveMesh when available; fall back to legacy VBO
    EnsurePlayerPatchPrimitive();
    if (playerPatchPrimitive_ && playerPatchPrimitive_->IsInitialized()) {
        playerPatchPrimitive_->Draw();
        return;
    }
    // Legacy path
    EnsurePrimitiveBuffers();
    if (!primitiveBuffers_ || primitiveBuffers_->playerVBO == 0) {
        return;
    }
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveBuffers_->playerVBO);
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 6, reinterpret_cast<const void*>(0));
    glColorPointer(3, GL_FLOAT, sizeof(float) * 6, reinterpret_cast<const void*>(sizeof(float) * 3));
    glDrawArrays(GL_TRIANGLES, 0, primitiveBuffers_->playerVertexCount);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopClientAttrib();
#endif
}

void Viewport3D::DrawHudTextureOverlay(unsigned int texture, float x, float y, float width, float height) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend() || texture == 0) {
        return;
    }

    EnsureHudTexturePrimitive(x, y, width, height);
    if (!hudTexturePrimitive_ || !hudTexturePrimitive_->IsInitialized()) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    hudTexturePrimitive_->Draw();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
#else
    (void)texture; (void)x; (void)y; (void)width; (void)height;
#endif
}

void Viewport3D::EnsurePlayerMesh() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (playerMeshInitialized_) {
        return;
    }

    MeshBuilder builder(GL_TRIANGLES);
    builder.ReserveVertices(128);
    builder.ReserveIndices(192);

    auto addQuad = [&](const MeshVertex& a,
                       const MeshVertex& b,
                       const MeshVertex& c,
                       const MeshVertex& d) {
        const GLuint base = builder.CurrentIndex();
        builder.AddVertex(a);
        builder.AddVertex(b);
        builder.AddVertex(c);
        builder.AddVertex(d);
        builder.AddQuad(base, base + 1, base + 2, base + 3);
    };

    auto addTriangle = [&](const MeshVertex& a,
                           const MeshVertex& b,
                           const MeshVertex& c) {
        const GLuint base = builder.CurrentIndex();
        builder.AddVertex(a);
        builder.AddVertex(b);
        builder.AddVertex(c);
        builder.AddTriangle(base, base + 1, base + 2);
    };

    const GLfloat mainTopR = 0.24f;
    const GLfloat mainTopG = 0.78f;
    const GLfloat mainTopB = 0.98f;
    const GLfloat mainMidR = 0.18f;
    const GLfloat mainMidG = 0.60f;
    const GLfloat mainMidB = 0.92f;
    const GLfloat mainBottomR = 0.06f;
    const GLfloat mainBottomG = 0.30f;
    const GLfloat mainBottomB = 0.58f;
    const GLfloat accentBrightR = 0.95f;
    const GLfloat accentBrightG = 0.98f;
    const GLfloat accentBrightB = 1.00f;
    const GLfloat accentShadowR = 0.08f;
    const GLfloat accentShadowG = 0.20f;
    const GLfloat accentShadowB = 0.45f;
    const GLfloat eyeInnerR = 0.12f;
    const GLfloat eyeInnerG = 0.18f;
    const GLfloat eyeInnerB = 0.28f;
    const GLfloat eyeGlowR = 0.76f;
    const GLfloat eyeGlowG = 0.96f;
    const GLfloat eyeGlowB = 1.00f;

    const GLfloat halfWidth = 1.05f;
    const GLfloat halfHeight = 0.82f;
    const GLfloat halfDepth = 0.96f;
    const GLfloat visorInset = 0.72f;

    // Front face
    addQuad(
        MeshVertex(-halfWidth,  halfHeight, halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex( halfWidth,  halfHeight, halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex( halfWidth, -halfHeight, halfDepth, mainMidR, mainMidG, mainMidB),
        MeshVertex(-halfWidth, -halfHeight, halfDepth, mainMidR, mainMidG, mainMidB));

    // Back face
    addQuad(
        MeshVertex(-halfWidth,  halfHeight, -halfDepth, mainMidR, mainMidG, mainMidB),
        MeshVertex( halfWidth,  halfHeight, -halfDepth, mainMidR, mainMidG, mainMidB),
        MeshVertex( halfWidth, -halfHeight, -halfDepth, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex(-halfWidth, -halfHeight, -halfDepth, mainBottomR, mainBottomG, mainBottomB));

    // Left face
    addQuad(
        MeshVertex(-halfWidth,  halfHeight, -halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex(-halfWidth,  halfHeight,  halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex(-halfWidth, -halfHeight,  halfDepth, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex(-halfWidth, -halfHeight, -halfDepth, mainBottomR, mainBottomG, mainBottomB));

    // Right face
    addQuad(
        MeshVertex(halfWidth,  halfHeight,  halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex(halfWidth,  halfHeight, -halfDepth, mainTopR, mainTopG, mainTopB),
        MeshVertex(halfWidth, -halfHeight, -halfDepth, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex(halfWidth, -halfHeight,  halfDepth, mainBottomR, mainBottomG, mainBottomB));

    // Bottom face
    addQuad(
        MeshVertex(-halfWidth, -halfHeight,  halfDepth, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex( halfWidth, -halfHeight,  halfDepth, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex( halfWidth, -halfHeight, -halfDepth, accentShadowR, accentShadowG, accentShadowB),
        MeshVertex(-halfWidth, -halfHeight, -halfDepth, accentShadowR, accentShadowG, accentShadowB));

    // Rounded top cap (simple pyramid fan)
    const MeshVertex topCenter(0.0f, halfHeight + 0.52f, 0.0f, mainTopR, mainTopG, mainTopB);
    const MeshVertex topFront(-halfWidth * 0.65f, halfHeight, halfDepth * 0.78f, mainTopR, mainTopG, mainTopB);
    const MeshVertex topBack(-halfWidth * 0.65f, halfHeight, -halfDepth * 0.78f, mainMidR, mainMidG, mainMidB);
    const MeshVertex topFrontR(halfWidth * 0.65f, halfHeight, halfDepth * 0.78f, mainTopR, mainTopG, mainTopB);
    const MeshVertex topBackR(halfWidth * 0.65f, halfHeight, -halfDepth * 0.78f, mainMidR, mainMidG, mainMidB);
    addTriangle(topCenter, topFront, topFrontR);
    addTriangle(topCenter, topFrontR, topBackR);
    addTriangle(topCenter, topBackR, topBack);
    addTriangle(topCenter, topBack, topFront);

    // Visor recess frame
    addQuad(
        MeshVertex(-halfWidth * 0.78f, halfHeight * 0.45f, visorInset, mainMidR, mainMidG, mainMidB),
        MeshVertex( halfWidth * 0.78f, halfHeight * 0.45f, visorInset, mainMidR, mainMidG, mainMidB),
        MeshVertex( halfWidth * 0.88f, -halfHeight * 0.05f, visorInset, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex(-halfWidth * 0.88f, -halfHeight * 0.05f, visorInset, mainBottomR, mainBottomG, mainBottomB));

    addQuad(
        MeshVertex(-halfWidth * 0.88f, -halfHeight * 0.05f, visorInset, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex( halfWidth * 0.88f, -halfHeight * 0.05f, visorInset, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex( halfWidth * 0.70f, -halfHeight * 0.62f, visorInset, accentShadowR, accentShadowG, accentShadowB),
        MeshVertex(-halfWidth * 0.70f, -halfHeight * 0.62f, visorInset, accentShadowR, accentShadowG, accentShadowB));

    // Eyes (slight extrusion)
    const GLfloat eyeHalfWidth = 0.42f;
    const GLfloat eyeHalfHeight = 0.30f;
    const GLfloat eyeDepthOffset = halfDepth + 0.08f;
    const GLfloat eyeInsetDepth = halfDepth;

    auto addEye = [&](GLfloat centerX) {
        MeshVertex tl(centerX - eyeHalfWidth,  eyeHalfHeight, eyeDepthOffset, eyeGlowR, eyeGlowG, eyeGlowB);
        MeshVertex tr(centerX + eyeHalfWidth,  eyeHalfHeight, eyeDepthOffset, eyeGlowR, eyeGlowG, eyeGlowB);
        MeshVertex br(centerX + eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB);
        MeshVertex bl(centerX - eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB);
        addQuad(tl, tr, br, bl);

        // Side walls for each eye block
        addQuad(
            MeshVertex(centerX - eyeHalfWidth,  eyeHalfHeight, eyeInsetDepth, mainMidR, mainMidG, mainMidB),
            MeshVertex(centerX - eyeHalfWidth,  eyeHalfHeight, eyeDepthOffset, eyeGlowR, eyeGlowG, eyeGlowB),
            MeshVertex(centerX - eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB),
            MeshVertex(centerX - eyeHalfWidth, -eyeHalfHeight, eyeInsetDepth, mainBottomR, mainBottomG, mainBottomB));

        addQuad(
            MeshVertex(centerX + eyeHalfWidth,  eyeHalfHeight, eyeDepthOffset, eyeGlowR, eyeGlowG, eyeGlowB),
            MeshVertex(centerX + eyeHalfWidth,  eyeHalfHeight, eyeInsetDepth, mainMidR, mainMidG, mainMidB),
            MeshVertex(centerX + eyeHalfWidth, -eyeHalfHeight, eyeInsetDepth, mainBottomR, mainBottomG, mainBottomB),
            MeshVertex(centerX + eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB));

        addQuad(
            MeshVertex(centerX - eyeHalfWidth, -eyeHalfHeight, eyeInsetDepth, mainBottomR, mainBottomG, mainBottomB),
            MeshVertex(centerX + eyeHalfWidth, -eyeHalfHeight, eyeInsetDepth, mainBottomR, mainBottomG, mainBottomB),
            MeshVertex(centerX + eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB),
            MeshVertex(centerX - eyeHalfWidth, -eyeHalfHeight, eyeDepthOffset, eyeInnerR, eyeInnerG, eyeInnerB));
    };

    addEye(-0.55f);
    addEye(0.55f);

    // Lower accent ring
    addQuad(
        MeshVertex(-halfWidth * 0.60f, -halfHeight * 0.80f,  halfDepth * 0.70f, accentShadowR, accentShadowG, accentShadowB),
        MeshVertex( halfWidth * 0.60f, -halfHeight * 0.80f,  halfDepth * 0.70f, accentShadowR, accentShadowG, accentShadowB),
        MeshVertex( halfWidth * 0.45f, -halfHeight * 0.95f, -halfDepth * 0.10f, mainBottomR, mainBottomG, mainBottomB),
        MeshVertex(-halfWidth * 0.45f, -halfHeight * 0.95f, -halfDepth * 0.10f, mainBottomR, mainBottomG, mainBottomB));

    // Back thruster highlight
    addQuad(
        MeshVertex(-halfWidth * 0.55f, halfHeight * 0.15f, -halfDepth - 0.06f, accentBrightR, accentBrightG, accentBrightB),
        MeshVertex( halfWidth * 0.55f, halfHeight * 0.15f, -halfDepth - 0.06f, accentBrightR, accentBrightG, accentBrightB),
        MeshVertex( halfWidth * 0.35f, -halfHeight * 0.35f, -halfDepth - 0.12f, accentShadowR, accentShadowG, accentShadowB),
        MeshVertex(-halfWidth * 0.35f, -halfHeight * 0.35f, -halfDepth - 0.12f, accentShadowR, accentShadowG, accentShadowB));

    playerMesh_ = builder.Build(true);
    playerMesh_.SetAttributes(MeshAttribute_Position | MeshAttribute_Color);
    playerMeshInitialized_ = true;
    playerMeshPrimitiveDirty_ = true;
        for (auto& entry : entityMeshes_) {
            if (entry.second.primitive) {
                entry.second.primitive->Cleanup();
                entry.second.primitive.reset();
            }
            entry.second.primitiveDirty = true;
        }
#endif
}

PrimitiveMesh* Viewport3D::EnsureMeshPrimitive(const Mesh& mesh,
                                               std::unique_ptr<PrimitiveMesh>& cache,
                                               bool& dirtyFlag) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend() || mesh.Empty()) {
        return nullptr;
    }

    if (!cache) {
        cache = std::make_unique<PrimitiveMesh>();
        dirtyFlag = true;
    }

    if (dirtyFlag || !cache->IsInitialized()) {
        MeshSubmission submission = MeshSubmissionBuilder::FromMesh(mesh);
        cache->Upload(submission);
        dirtyFlag = false;
    }

    return cache.get();
#else
    (void)mesh;
    (void)cache;
    (void)dirtyFlag;
    return nullptr;
#endif
}

void Viewport3D::ToggleFullscreen() {
#if defined(USE_SDL)
    if (IsUsingSDLBackend() && sdlWindow) {
        const bool togglingToFullscreen = !isFullscreen_;
        if (togglingToFullscreen) {
            SDL_GetWindowPosition(sdlWindow, &windowedPosX_, &windowedPosY_);
            SDL_GetWindowSize(sdlWindow, &windowedWidth_, &windowedHeight_);
            if (SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
                isFullscreen_ = true;
            }
        } else {
            if (SDL_SetWindowFullscreen(sdlWindow, 0) == 0) {
                SDL_SetWindowPosition(sdlWindow, windowedPosX_, windowedPosY_);
                SDL_SetWindowSize(sdlWindow, windowedWidth_, windowedHeight_);
                isFullscreen_ = false;
            }
        }

        int newW = 0;
        int newH = 0;
        SDL_GetWindowSize(sdlWindow, &newW, &newH);
        Resize(newW, newH);
        return;
    }
#endif
#if defined(USE_GLFW)
    if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        if (!isFullscreen_) {
            glfwGetWindowPos(glfwWindow, &windowedPosX_, &windowedPosY_);
            glfwGetWindowSize(glfwWindow, &windowedWidth_, &windowedHeight_);
            GLFWmonitor* monitor = glfwGetWindowMonitor(glfwWindow);
            if (!monitor) {
                monitor = glfwGetPrimaryMonitor();
            }
            if (monitor) {
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                if (mode) {
                    glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    isFullscreen_ = true;
                    Resize(mode->width, mode->height);
                }
            }
        } else {
            glfwSetWindowMonitor(glfwWindow, nullptr, windowedPosX_, windowedPosY_, windowedWidth_, windowedHeight_, 0);
            isFullscreen_ = false;
            Resize(windowedWidth_, windowedHeight_);
        }
    }
#endif
}

void Viewport3D::DrawCubePrimitive(float r, float g, float b) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
    // Prefer retained-mode PrimitiveMesh when available; fall back to legacy VBO
    EnsureCubePrimitive();
    if (cubePrimitive_ && cubePrimitive_->IsInitialized()) {
        glColor3f(r, g, b);
        cubePrimitive_->Draw();
        return;
    }
    // Legacy path
    EnsurePrimitiveBuffers();
    if (!primitiveBuffers_ || primitiveBuffers_->cubeVBO == 0) {
        return;
    }
    glColor3f(r, g, b);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveBuffers_->cubeVBO);
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 3, reinterpret_cast<const void*>(0));
    glDrawArrays(GL_TRIANGLES, 0, primitiveBuffers_->cubeVertexCount);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopClientAttrib();
#else
    (void)r;
    (void)g;
    (void)b;
#endif
}

void Viewport3D::SetBackend(RenderBackend backend) {
    if (backend_ == backend) {
           return; // Early exit if the backend is already set
    }
    const bool wasGL = IsUsingGLBackend();
    backend_ = backend;
    if (wasGL && !IsUsingGLBackend()) {
        // Ensure a GL context is current before destroying GL resources
#if defined(USE_SDL)
        if (sdlWindow && sdlGLContext) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
        }
#endif
#if defined(USE_GLFW)
        if (glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
        }
#endif
        DestroyPrimitiveBuffers();
        if (uiBatcher_) { uiBatcher_->Cleanup(); uiBatcher_.reset(); }
        if (lineBatcher3D_) { lineBatcher3D_->Cleanup(); lineBatcher3D_.reset(); }
#if defined(USE_GLFW) || defined(USE_SDL)
        if (playerMeshInitialized_) {
            playerMesh_.Clear();
            playerMeshInitialized_ = false;
        }
        // Clean up any retained primitive meshes we created
        if (cubePrimitive_) {
            cubePrimitive_->Cleanup();
            cubePrimitive_.reset();
        }
        if (playerPatchPrimitive_) {
            playerPatchPrimitive_->Cleanup();
            playerPatchPrimitive_.reset();
        }
        if (hudTexturePrimitive_) {
            hudTexturePrimitive_->Cleanup();
            hudTexturePrimitive_.reset();
        }
        if (playerMeshPrimitive_) {
            playerMeshPrimitive_->Cleanup();
            playerMeshPrimitive_.reset();
        }
        playerMeshPrimitiveDirty_ = true;
        hudTexturePrimitiveDirty_ = true;
#endif
        if (shaderManager_) {
            shaderManager_->Clear();
            shaderManager_.reset();
        }
    }
    if (debugLogging_) {
        std::cout << "Viewport3D: render backend set to " << RenderBackendToString(backend_) << std::endl;
    }

    if (!wasGL && IsUsingGLBackend()) {
        InitializeShaderManager();
    }
}

bool Viewport3D::IsUsingSDLBackend() const {
    return backend_ == RenderBackend::SDL_GL || backend_ == RenderBackend::SDL_Renderer;
}

bool Viewport3D::IsUsingSDLGL() const {
    return backend_ == RenderBackend::SDL_GL;
}

bool Viewport3D::IsUsingSDLRenderer() const {
    return backend_ == RenderBackend::SDL_Renderer;
}

bool Viewport3D::IsUsingGLFWBackend() const {
    return backend_ == RenderBackend::GLFW_GL;
}

bool Viewport3D::IsUsingGLBackend() const {
    return backend_ == RenderBackend::SDL_GL || backend_ == RenderBackend::GLFW_GL;
}

void Viewport3D::EnsureLayoutConfiguration() {
    if (layouts_.empty()) {
        layouts_ = CreateDefaultLayouts();
    }
    layouts_.erase(std::remove_if(layouts_.begin(), layouts_.end(), [](const ViewportLayout& layout) {
        return layout.views.empty();
    }), layouts_.end());
    if (layouts_.empty()) {
        ViewportLayout fallback;
        fallback.name = "Single View";
        fallback.views.push_back({"Primary", 0.0, 0.0, 1.0, 1.0, ViewRole::Main, false});
        layouts_.push_back(fallback);
    }
    if (activeLayoutIndex_ >= layouts_.size()) {
        activeLayoutIndex_ = 0;
    }
}

void Viewport3D::ConfigureLayouts(std::vector<ViewportLayout> layouts) {
    layouts_ = std::move(layouts);
    activeLayoutIndex_ = 0;
    EnsureLayoutConfiguration();
}

void Viewport3D::CycleLayout() {
    EnsureLayoutConfiguration();
    if (!layouts_.empty()) {
        activeLayoutIndex_ = (activeLayoutIndex_ + 1) % layouts_.size();
    }
}

void Viewport3D::SetActiveLayout(size_t index) {
    EnsureLayoutConfiguration();
    if (layouts_.empty()) {
        activeLayoutIndex_ = 0;
        return;
    }
    activeLayoutIndex_ = std::min(index, layouts_.size() - 1);
}

const ViewportLayout& Viewport3D::GetActiveLayout() const {
    if (layouts_.empty() || activeLayoutIndex_ >= layouts_.size()) {
        return DefaultViewportLayoutFallback();
    }
    return layouts_[activeLayoutIndex_];
}

std::string Viewport3D::GetActiveLayoutName() const {
    const ViewportLayout& layout = GetActiveLayout();
    if (layout.name.empty()) {
        return std::string("Single View");
    }
    return layout.name;
}

std::vector<ViewportLayout> Viewport3D::CreateDefaultLayouts() {
    std::vector<ViewportLayout> defaults;

    ViewportLayout single;
    single.name = "Single View";
    single.views.push_back({"Primary", 0.0, 0.0, 1.0, 1.0, ViewRole::Main, false});
    defaults.push_back(single);

    ViewportLayout verticalSplit;
    verticalSplit.name = "Split Vertical";
    verticalSplit.views.push_back({"Left", 0.0, 0.0, 0.5, 1.0, ViewRole::Main, false});
    verticalSplit.views.push_back({"Right", 0.5, 0.0, 0.5, 1.0, ViewRole::Secondary, false});
    defaults.push_back(verticalSplit);

    ViewportLayout minimap;
    minimap.name = "Main + Minimap";
    minimap.views.push_back({"Main", 0.0, 0.0, 1.0, 1.0, ViewRole::Main, false});
    minimap.views.push_back({"Minimap", 0.7, 0.05, 0.28, 0.28, ViewRole::Minimap, true});
    defaults.push_back(minimap);

    return defaults;
}

size_t Viewport3D::GetActiveViewCount() const {
    if (layouts_.empty() || activeLayoutIndex_ >= layouts_.size()) {
        return 0;
    }
    return layouts_[activeLayoutIndex_].views.size();
}

ViewRole Viewport3D::GetViewRole(size_t viewIndex) const {
    if (layouts_.empty() || viewIndex >= layouts_[activeLayoutIndex_].views.size()) {
        return ViewRole::Main;
    }
    return layouts_[activeLayoutIndex_].views[viewIndex].role;
}

bool Viewport3D::IsOverlayView(size_t viewIndex) const {
    if (layouts_.empty() || viewIndex >= layouts_[activeLayoutIndex_].views.size()) {
        return false;
    }
    return layouts_[activeLayoutIndex_].views[viewIndex].overlay;
}

void Viewport3D::SetFramePacingHint(bool vsyncEnabled, double fps) {
    vsyncEnabled_ = vsyncEnabled;
    frameRateLimitHint_ = fps;
}

void Viewport3D::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
#if defined(USE_SDL)
    if (IsUsingSDLGL() && sdlWindow && sdlGLContext) {
        SDL_Window* previousWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext previousContext = SDL_GL_GetCurrentContext();
        const bool needRestore = (previousWindow != sdlWindow) || (previousContext != sdlGLContext);
        if (needRestore) {
            if (SDL_GL_MakeCurrent(sdlWindow, sdlGLContext) != 0) {
                if (debugLogging_) {
                    std::cerr << "Viewport3D::SetVSyncEnabled: SDL_GL_MakeCurrent failed: "
                              << SDL_GetError() << std::endl;
                }
                return;
            }
        }

        if (SDL_GL_SetSwapInterval(enabled ? 1 : 0) != 0 && debugLogging_) {
            std::cerr << "Viewport3D::SetVSyncEnabled: SDL_GL_SetSwapInterval failed: "
                      << SDL_GetError() << std::endl;
        }

        if (needRestore) {
            if (previousWindow && previousContext) {
                SDL_GL_MakeCurrent(previousWindow, previousContext);
            } else {
                SDL_GL_MakeCurrent(nullptr, nullptr);
            }
        }
    }
#endif
#if defined(USE_GLFW)
    if (IsUsingGLFWBackend() && glfwWindow) {
        GLFWwindow* previousContext = glfwGetCurrentContext();
        const bool needRestore = previousContext != glfwWindow;
        if (needRestore) {
            glfwMakeContextCurrent(glfwWindow);
        }

        if (glfwGetCurrentContext() == glfwWindow) {
            glfwSwapInterval(enabled ? 1 : 0);
        } else if (debugLogging_) {
            std::cerr << "Viewport3D::SetVSyncEnabled: failed to activate GLFW context for swap interval"
                      << std::endl;
        }

        if (needRestore) {
            glfwMakeContextCurrent(previousContext);
        }
    }
#endif
}

void Viewport3D::BeginFrame() {
    EnsureLayoutConfiguration();
#if defined(USE_SDL)
    if (IsUsingSDLGL() && sdlWindow) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif
    Clear();
}

void Viewport3D::FinishFrame() {
    ResetViewport();
}

void Viewport3D::ActivateView(const Camera* camera, double playerX, double playerY, double playerZ, size_t viewIndex) {
    EnsureLayoutConfiguration();
    if (layouts_.empty() || viewIndex >= layouts_[activeLayoutIndex_].views.size()) {
        return;
    }

    const ViewportView& view = layouts_[activeLayoutIndex_].views[viewIndex];

#if defined(USE_SDL)
    if (IsUsingSDLGL() && sdlWindow) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif

    if (IsUsingGLBackend()) {
        ActivateOpenGLView(view, camera, playerX, playerY, playerZ);
    } else if (IsUsingSDLBackend()) {
        ActivateSDLView(view);
    }
}

void Viewport3D::ApplyViewportView(const ViewportView& view) {
#if defined(USE_GLFW) || defined(USE_SDL)
    int viewportWidth = std::max(1, static_cast<int>(view.normalizedWidth * static_cast<double>(width)));
    int viewportHeight = std::max(1, static_cast<int>(view.normalizedHeight * static_cast<double>(height)));
    int viewportX = static_cast<int>(view.normalizedX * static_cast<double>(width));
    int viewportY = static_cast<int>(view.normalizedY * static_cast<double>(height));

    if (IsUsingGLBackend()) {
        int glViewportY = height - viewportY - viewportHeight;
        if (glViewportY < 0) {
            glViewportY = 0;
        }
        glViewport(viewportX, glViewportY, viewportWidth, viewportHeight);
    }
#if defined(USE_SDL)
    if (IsUsingSDLRenderer() && sdlRenderer) {
        SDL_Rect rect{viewportX, viewportY, viewportWidth, viewportHeight};
        SDL_RenderSetViewport(sdlRenderer, &rect);
    }
#endif
#else
    (void)view;
#endif
}

void Viewport3D::ResetViewport() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (IsUsingGLBackend()) {
        glViewport(0, 0, width, height);
    }
#endif
#if defined(USE_SDL)
    if (IsUsingSDLRenderer() && sdlRenderer) {
        SDL_RenderSetViewport(sdlRenderer, nullptr);
    }
#endif
}

void Viewport3D::ActivateOpenGLView(const ViewportView& view, const Camera* camera, double playerX, double playerY, double playerZ) {
#if defined(USE_GLFW) || defined(USE_SDL)
    ApplyViewportView(view);
    if (view.overlay) {
        GLint depthBits = 0;
        glGetIntegerv(GL_DEPTH_BITS, &depthBits);
        if (depthBits > 0) {
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
            GLint previousScissor[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_SCISSOR_BOX, previousScissor);

            glEnable(GL_SCISSOR_TEST);
            glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
            glClear(GL_DEPTH_BUFFER_BIT);

            if (scissorWasEnabled) {
                glScissor(previousScissor[0], previousScissor[1], previousScissor[2], previousScissor[3]);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
        }
    }
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int viewportWidth = std::max(1, static_cast<int>(view.normalizedWidth * static_cast<double>(width)));
    int viewportHeight = std::max(1, static_cast<int>(view.normalizedHeight * static_cast<double>(height)));
    double aspect = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
    double fov = camera ? camera->zoom() : 45.0;
    double clampedFov = std::clamp(fov, 20.0, 120.0);
    gluPerspective(clampedFov, aspect, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!camera) {
        return;
    }

    if (view.role == ViewRole::Minimap) {
        gluLookAt(playerX, playerY, playerZ + 25.0,
                  playerX, playerY, playerZ,
                  0.0, 1.0, 0.0);
    } else {
        camera->ApplyToOpenGL();
    }
#else
    (void)view;
    (void)camera;
    (void)playerX;
    (void)playerY;
    (void)playerZ;
#endif
}

void Viewport3D::ActivateSDLView(const ViewportView& view) {
#ifdef USE_SDL
    // For SDL renderer backend, just set the viewport; 2D drawing uses screen-space
    ApplyViewportView(view);
#else
    (void)view;
#endif
}


void Viewport3D::Init() {
    if (debugLogging_) std::cout << "Viewport3D::Init() starting" << std::endl;
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Viewport3D::Init start" << std::endl;
    }
    SetBackend(RenderBackend::None);
#ifdef USE_GLFW
    if (debugLogging_) std::cout << "USE_GLFW is defined, attempting GLFW initialization" << std::endl;
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Attempting glfwInit" << std::endl;
    }
#endif
#ifndef USE_GLFW
    if (debugLogging_) std::cout << "USE_GLFW is NOT defined, falling back to SDL or ASCII" << std::endl;
#endif
#ifdef USE_GLFW
    if (!glfwInit()) {
        std::cerr << "Viewport3D: GLFW initialization failed" << std::endl;
        {
            std::ofstream f("glfw_diag.log", std::ios::app);
            if (f) f << "glfwInit failed" << std::endl;
        }
        return;
    }
    if (debugLogging_) std::cout << "GLFW initialized successfully" << std::endl;
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "glfwInit succeeded" << std::endl;
    }
    // Try a few common OpenGL context configurations
    struct Attempt { int major; int minor; bool forwardCompatible; const char* description; };
    const Attempt attempts[] = {
        {3, 3, true,  "OpenGL 3.3 compat"},
        {3, 0, true,  "OpenGL 3.0 compat"},
        {2, 1, false, "OpenGL 2.1 any"},
        {0, 0, false, "Default profile"}
    };
    const Attempt* chosenAttempt = nullptr;
    for (const Attempt& attempt : attempts) {
        glfwDefaultWindowHints();
        if (attempt.major > 0) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, attempt.major);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, attempt.minor);
        }
        if (attempt.major >= 3) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        } else {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        }
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, attempt.forwardCompatible ? GL_TRUE : GL_FALSE);

        if (debugLogging_) std::cout << "Viewport3D: Trying " << attempt.description << " context (windowed)" << std::endl;
        {
            std::ofstream f("glfw_diag.log", std::ios::app);
            if (f) f << "Creating window " << attempt.description << " " << width << "x" << height << std::endl;
        }
        glfwWindow = glfwCreateWindow(width, height, "Nova Engine", nullptr, nullptr);
        if (glfwWindow) {
            chosenAttempt = &attempt;
            {
                std::ofstream f("glfw_diag.log", std::ios::app);
                if (f) f << "Window created" << std::endl;
            }
            break;
        }

        std::cerr << "Viewport3D: GLFW window creation failed for " << attempt.description << std::endl;
        {
            std::ofstream f("glfw_diag.log", std::ios::app);
            if (f) f << "Window creation failed for attempt" << std::endl;
        }
    }

    if (!glfwWindow) {
        std::cerr << "Viewport3D: Unable to create any OpenGL context" << std::endl;
        glfwTerminate();
        return;
    }

    if (debugLogging_) std::cout << "GLFW window created successfully using " << chosenAttempt->description << std::endl;

    // Make sure the window is visible
    glfwShowWindow(glfwWindow);
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Window shown" << std::endl;
    }

    // Make the OpenGL context current
    glfwMakeContextCurrent(glfwWindow);
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Made context current, loading GLAD" << std::endl;
    }
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Viewport3D: Failed to initialize GLAD" << std::endl;
        {
            std::ofstream f("glfw_diag.log", std::ios::app);
            if (f) f << "GLAD init failed" << std::endl;
        }
        glfwDestroyWindow(glfwWindow);
        glfwWindow = nullptr;
        glfwTerminate();
        return;
    }
    InitializeShaderManager();
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "GLAD init succeeded; creating UIBatcher" << std::endl;
    }

    // Enable OpenGL debug output for GPU validation (debug builds only)
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLDebugCallback, nullptr);
        // Enable all debug message types except notifications to reduce noise
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        if (debugLogging_) std::cout << "Viewport3D: OpenGL debug output enabled" << std::endl;
    } else {
        if (debugLogging_) std::cout << "Viewport3D: GL_KHR_debug extension not available" << std::endl;
    }
#endif

    // Initialize UIBatcher after GLAD setup
    uiBatcher_ = std::make_unique<UIBatcher>();
    if (!uiBatcher_->Init()) {
        if (debugLogging_) std::cerr << "Viewport3D: UIBatcher::Init failed (GLFW path)" << std::endl;
        uiBatcher_.reset();
    }

    // Initialize material system
    try {
        materialLibrary_ = std::make_unique<Nova::MaterialLibrary>();
        if (!materialLibrary_->Initialize()) {
            if (debugLogging_) std::cerr << "Viewport3D: MaterialLibrary::Initialize failed (GLFW path)" << std::endl;
            materialLibrary_.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Viewport3D: Exception in MaterialLibrary::Initialize: " << e.what() << std::endl;
        materialLibrary_.reset();
    }

    try {
        instancedRenderer_ = std::make_unique<Nova::InstancedMeshRenderer>();
        if (!instancedRenderer_->Initialize(shaderManager_.get())) {
            if (debugLogging_) std::cerr << "Viewport3D: InstancedMeshRenderer::Initialize failed (GLFW path)" << std::endl;
            instancedRenderer_.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Viewport3D: Exception in InstancedMeshRenderer::Initialize: " << e.what() << std::endl;
        instancedRenderer_.reset();
    }

    // Initialize ActorRenderer for ECS-based actor rendering
    try {
        actorRenderer_ = std::make_unique<ActorRenderer>();
        if (!actorRenderer_->Initialize()) {
            if (debugLogging_) std::cerr << "Viewport3D: ActorRenderer::Initialize failed (GLFW path)" << std::endl;
            actorRenderer_.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Viewport3D: Exception in ActorRenderer::Initialize: " << e.what() << std::endl;
        actorRenderer_.reset();
    }

    SetBackend(RenderBackend::GLFW_GL);
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Backend set to GLFW_GL; disabling vsync" << std::endl;
    }
    // Disable VSync to allow higher FPS by default
    SetVSyncEnabled(false);

    // Setup basic GL state
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable normal cursor mode for window interaction
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (debugLogging_) std::cout << "Viewport3D: Using GLFW with OpenGL for rendering." << std::endl;
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "Viewport3D::Init finished (GLFW path)" << std::endl;
    }
    return;
#endif
#ifndef USE_GLFW
    if (debugLogging_) std::cout << "USE_GLFW is NOT defined, falling back to SDL or ASCII" << std::endl;
#endif
#ifdef USE_SDL
    // Compute an absolute path for sdl_diag.log next to the running executable
    std::string diagLogPath = "sdl_diag.log";
#ifdef _WIN32
    {
        CHAR exePath[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, exePath, MAX_PATH);
        if (n > 0) {
            std::string full(exePath);
            size_t pos = full.find_last_of("\\/");
            if (pos != std::string::npos) full = full.substr(0, pos + 1);
            else full.clear();
            diagLogPath = full + "sdl_diag.log";
        }
    }
#endif
    auto writeLog = [&](const std::string &msg){
        std::ofstream f(diagLogPath, std::ios::app);
        if (f) f << msg << std::endl;
    };

    writeLog("Viewport3D::Init() started");

    int sdlInitRc = -1;
    // Attempt 1: with default driver
    writeLog("Viewport3D: SDL_Init attempt 1 (default driver)");
    sdlInitRc = SDL_Init(SDL_INIT_VIDEO);
    if (sdlInitRc != 0) {
        std::string err = SDL_GetError();
        writeLog(std::string("Viewport3D: SDL_Init attempt 1 failed: '") + err + "'");
        // Ensure any partial state is cleaned up
        SDL_Quit();

        // Attempt 2: try without video, just to see
        writeLog("Viewport3D: SDL_Init attempt 2 (no video)");
        sdlInitRc = SDL_Init(0);
        writeLog(std::string("Viewport3D: SDL_Init attempt 2 rc=") + std::to_string(sdlInitRc) + std::string(" err='") + SDL_GetError() + "'");
        if (sdlInitRc == 0) {
            // Try to init video separately
            writeLog("Viewport3D: SDL_InitSubSystem VIDEO");
            sdlInitRc = SDL_InitSubSystem(SDL_INIT_VIDEO);
            writeLog(std::string("Viewport3D: SDL_InitSubSystem rc=") + std::to_string(sdlInitRc) + std::string(" err='") + SDL_GetError() + "'");
        }
    } else {
        writeLog("Viewport3D: SDL_Init attempt 1 succeeded");
    }

        // Log SDL platform and whether SDL3.dll is currently loaded into the process
        {
            const char* platform = SDL_GetPlatform();
            if (platform) writeLog(std::string("SDL platform: ") + platform);
#ifdef _WIN32
            HMODULE h = GetModuleHandleA("SDL3.dll");
            if (h) {
                CHAR pathbuf[MAX_PATH];
                DWORD n = GetModuleFileNameA(h, pathbuf, MAX_PATH);
                if (n > 0) writeLog(std::string("Loaded SDL3.dll: ") + pathbuf);
                else writeLog(std::string("Loaded SDL3.dll but GetModuleFileNameA failed"));
            } else {
                writeLog(std::string("SDL3.dll module not found via GetModuleHandleA"));
            }
#endif
        }

        if (sdlInitRc == 0) {
            // Try OpenGL first for better compatibility
            writeLog("Viewport3D: Trying OpenGL path");
            auto setGLAttr = [&](SDL_GLattr attr, int value) {
                if (SDL_GL_SetAttribute(attr, value) != 0) {
                    writeLog(std::string("Viewport3D: SDL_GL_SetAttribute failed for attr ") + std::to_string(static_cast<int>(attr)) + ": " + SDL_GetError());
                }
            };
            setGLAttr(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            setGLAttr(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            setGLAttr(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
            setGLAttr(SDL_GL_DOUBLEBUFFER, 1);
            sdlWindow = compat_CreateWindow("Nova Engine", width, height, (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));
            if (sdlWindow) {
                writeLog("Viewport3D: SDL_CreateWindow (GL) succeeded");
                SDL_RaiseWindow(sdlWindow);
#ifdef _WIN32
                // On Windows, force the window to foreground and give it focus (only if aggressive focus is enabled)
                if (aggressiveFocus_) {
                    HWND hwnd = reinterpret_cast<HWND>(compat_GetWindowNativeHandle(sdlWindow));
                    if (hwnd) {
                        writeLog("Viewport3D: Setting window to foreground");
                        SetForegroundWindow(hwnd);
                        SetFocus(hwnd);
                        // Additional focus tricks
                        ShowWindow(hwnd, SW_RESTORE);
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        writeLog("Viewport3D: Window focus operations completed");
                    } else {
                        writeLog("Viewport3D: compat_GetWindowNativeHandle failed for focus");
                    }
                }
#endif
                writeLog("Viewport3D: Before SDL_GL_CreateContext");
                sdlGLContext = SDL_GL_CreateContext(sdlWindow);
                writeLog("Viewport3D: After SDL_GL_CreateContext");
                if (sdlGLContext) {
                    // Make the context current
                    writeLog("Viewport3D: Before SDL_GL_MakeCurrent");
                    if (SDL_GL_MakeCurrent(sdlWindow, sdlGLContext) != 0) {
                        writeLog(std::string("Viewport3D: SDL_GL_MakeCurrent failed: ") + SDL_GetError());
                        compat_GL_DeleteContext(sdlGLContext);
                        sdlGLContext = nullptr;
                        if (sdlWindow) { SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr; }
                    } else {
                        writeLog("Viewport3D: SDL_GL_MakeCurrent succeeded");
                        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
                            std::string msg = "Viewport3D: Failed to initialize GLAD";
                            std::cerr << msg << std::endl;
                            writeLog(msg);
                            compat_GL_DeleteContext(sdlGLContext);
                            sdlGLContext = nullptr;
                            if (sdlWindow) { SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr; }
                        } else {
                            InitializeShaderManager();
                        // Enable OpenGL debug output for GPU validation (debug builds only)
#ifndef NDEBUG
                        if (glDebugMessageCallback != nullptr) {
                            glEnable(GL_DEBUG_OUTPUT);
                            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                            glDebugMessageCallback(OpenGLDebugCallback, nullptr);
                            // Enable all debug message types except notifications to reduce noise
                            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
                            writeLog("OpenGL debug output enabled");
                        } else {
                            writeLog("GL_KHR_debug extension not available");
                        }
#endif
                        // Initialize UIBatcher after GLAD setup
                        uiBatcher_ = std::make_unique<UIBatcher>();
                        if (!uiBatcher_->Init()) {
                            if (debugLogging_) std::cerr << "Viewport3D: UIBatcher::Init failed (SDL_GL path)" << std::endl;
                            uiBatcher_.reset();
                        }

                        // Initialize ActorRenderer for ECS-based actor rendering
                        try {
                            actorRenderer_ = std::make_unique<ActorRenderer>();
                            if (!actorRenderer_->Initialize()) {
                                if (debugLogging_) std::cerr << "Viewport3D: ActorRenderer::Initialize failed (SDL_GL path)" << std::endl;
                                actorRenderer_.reset();
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Viewport3D: Exception in ActorRenderer::Initialize: " << e.what() << std::endl;
                            actorRenderer_.reset();
                        }

                        SetBackend(RenderBackend::SDL_GL);
                        // Setup basic GL state
                        glViewport(0, 0, width, height);
                        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        // Enable relative mouse mode for camera control
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        if (debugLogging_) std::cout << "Viewport3D: Using OpenGL for rendering." << std::endl;
                        return;
                        }
                    }
                } else {
                    // Failed to create GL context; destroy window and fall back
                    writeLog("Viewport3D: GL context is null, logging failure");
                    std::string msg = std::string("Viewport3D: SDL_GL_CreateContext failed: ") + SDL_GetError();
                    std::cerr << msg << std::endl; writeLog(msg);
                    if (sdlWindow) { SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr; }
                }
            } else {
                std::string msg = std::string("Viewport3D: SDL_CreateWindow (GL) failed: ") + SDL_GetError();
                std::cerr << msg << std::endl; writeLog(msg);
            }

            // If OpenGL fails, try SDL renderer
            writeLog("Viewport3D: Trying SDL renderer path");
            sdlWindow = compat_CreateWindow("Nova Engine", width, height, (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));
            if (sdlWindow) {
                writeLog("Viewport3D: SDL_CreateWindow (renderer) succeeded");
                SDL_RaiseWindow(sdlWindow);
#ifdef _WIN32
                if (aggressiveFocus_) {
                    // On Windows, force the window to foreground and give it focus
                    HWND hwnd = reinterpret_cast<HWND>(compat_GetWindowNativeHandle(sdlWindow));
                    if (hwnd) {
                        writeLog("Viewport3D: Setting renderer window to foreground");
                        SetForegroundWindow(hwnd);
                        SetFocus(hwnd);
                        // Additional focus tricks
                        ShowWindow(hwnd, SW_RESTORE);
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        writeLog("Viewport3D: Renderer window focus operations completed");
                    } else {
                        writeLog("Viewport3D: compat_GetWindowNativeHandle failed for renderer focus");
                    }
                }
#endif
                // SDL3: CreateRenderer takes (window, name). Pass NULL to let SDL pick the best renderer.
                writeLog("Viewport3D: Before SDL_CreateRenderer (accelerated)");
                sdlRenderer = compat_CreateRenderer(sdlWindow, NULL);
                writeLog("Viewport3D: After SDL_CreateRenderer");
                if (sdlRenderer) {
                    writeLog("Viewport3D: SDL_CreateRenderer succeeded");
                    SetBackend(RenderBackend::SDL_Renderer);
                    if (debugLogging_) std::cout << "Viewport3D: Using SDL renderer for rendering." << std::endl;
                    return;
                } else {
                    std::string msg = std::string("Viewport3D: SDL_CreateRenderer failed: ") + SDL_GetError();
                    std::cerr << msg << std::endl;
                    writeLog(msg);
#ifdef _WIN32
                    std::string wmsg = std::string("Viewport3D: Win32 GetLastError() = ") + std::to_string(GetLastError());
                    std::cerr << wmsg << std::endl;
                    writeLog(wmsg);
#endif
                    // Try software renderer as fallback
                    writeLog("Viewport3D: Trying software renderer");
                    sdlRenderer = SDL_CreateRenderer(sdlWindow, 0, SDL_RENDERER_SOFTWARE);
                    if (sdlRenderer) {
                        writeLog("Viewport3D: SDL_CreateRenderer (software) succeeded");
                        SetBackend(RenderBackend::SDL_Renderer);
                        if (debugLogging_) std::cout << "Viewport3D: Using SDL software renderer for rendering." << std::endl;
                        return;
                    } else {
                        std::string msg2 = std::string("Viewport3D: SDL_CreateRenderer (software) failed: ") + SDL_GetError();
                        std::cerr << msg2 << std::endl;
                        writeLog(msg2);
                    }
                }
            } else {
                std::string msg = std::string("Viewport3D: SDL_CreateWindow failed: ") + SDL_GetError();
                std::cerr << msg << std::endl;
                writeLog(msg);
#ifdef _WIN32
                std::string wmsg = std::string("Viewport3D: Win32 GetLastError() = ") + std::to_string(GetLastError());
                std::cerr << wmsg << std::endl;
                writeLog(wmsg);
#endif
            }

            if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
            if (sdlWindow) SDL_DestroyWindow(sdlWindow);
            SDL_Quit();
        } else {
            std::string msg = std::string("Viewport3D: SDL_Init failed (rc=") + std::to_string(sdlInitRc) + "): " + SDL_GetError();
            std::cerr << msg << std::endl;
            writeLog(msg);
#ifdef _WIN32
            std::string wmsg = std::string("Viewport3D: Win32 GetLastError() = ") + std::to_string(GetLastError());
            std::cerr << wmsg << std::endl;
            writeLog(wmsg);
#endif
    }
#endif
    if (debugLogging_) std::cout << "Viewport3D Initialized with size " << width << "x" << height << " (ASCII fallback)" << std::endl;
}

void Viewport3D::Render(const class Camera* camera, double playerX, double playerY, double playerZ, bool targetLocked, ecs::EntityManagerV2* entityManager) {
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "Viewport3D::Render");
    }
#endif
    if (debugLogging_) std::cout << "Viewport3D::Render() called with camera=" << (camera ? "valid" : "null") << std::endl;
    EnsureLayoutConfiguration();
    if (debugLogging_) std::cout << "Viewport3D::Render() - after EnsureLayoutConfiguration()" << std::endl;
#if defined(USE_GLFW) || defined(USE_SDL)
    if (IsUsingGLBackend()) {
        TickShaderHotReload();
    }
#endif
    // BeginFrame(); // Removed - Clear() is already called in MainLoop
    if (debugLogging_) std::cout << "Viewport3D::Render() - after BeginFrame()" << std::endl;

    int activeViewCount = GetActiveViewCount();
    if (debugLogging_) std::cout << "Viewport3D::Render() - active view count: " << activeViewCount << std::endl;
    if (activeViewCount == 0) {
        if (debugLogging_) std::cout << "Viewport3D::Render() - no active views" << std::endl;
        return;
    }

    ActivateView(camera, playerX, playerY, playerZ, 0);

#if defined(USE_GLFW) || defined(USE_SDL)
    if (IsUsingGLBackend() && camera) {
        if (debugLogging_) std::cout << "Viewport3D::Render() - drawing camera debug" << std::endl;
        DrawCameraDebug(camera, playerX, playerY, playerZ, ViewRole::Main, targetLocked);
    } else if (IsUsingSDLRenderer()) {
        // 2D fallback: nothing additional required here
        if (debugLogging_) std::cout << "Viewport3D::Render() - SDL 2D fallback" << std::endl;
    } else {
        if (debugLogging_) std::cout << "Viewport3D::Render() - no rendering (backend="
                                      << RenderBackendToString(backend_) << ", camera="
                                      << (camera ? "valid" : "null") << ")" << std::endl;
    }
#else
    (void)camera;
    (void)playerX;
    (void)playerY;
    (void)playerZ;
#endif
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPopDebugGroup();
    }
#endif
}

void Viewport3D::Clear() {
    if (debugLogging_) std::cout << "Viewport3D::Clear() called" << std::endl;
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Viewport3D::Clear");
    }
#endif
    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        if (IsUsingSDLGL()) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            glViewport(0, 0, width, height);
            // Ensure default framebuffer is bound before clearing
#ifdef GL_FRAMEBUFFER
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
            // Clear any previous GL error to avoid reporting unrelated issues
            (void)glGetError();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                // rate-limit to avoid spam if persistent
                static auto lastLog = std::chrono::steady_clock::time_point{};
                auto now = std::chrono::steady_clock::now();
                if (lastLog.time_since_epoch().count() == 0 || (now - lastLog) > std::chrono::seconds(1)) {
                    std::cerr << "OpenGL error in Clear(): " << err << " (" << DescribeGlError(err) << ")" << std::endl;
                    lastLog = now;
                }
                if (debugLogging_) {
                    // Capture a bit more GL state to help diagnose
                    GLint drawFbo = 0, readFbo = 0, viewportVals[4] = {0,0,0,0};
#ifdef GL_DRAW_FRAMEBUFFER_BINDING
                    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
#elif defined(GL_FRAMEBUFFER_BINDING)
                    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFbo);
#endif
#ifdef GL_READ_FRAMEBUFFER_BINDING
                    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
#endif
                    glGetIntegerv(GL_VIEWPORT, viewportVals);
                    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
                    std::cerr << "  GL state: drawFBO=" << drawFbo
                              << " readFBO=" << readFbo
                              << " viewport=" << viewportVals[0] << "," << viewportVals[1]
                              << " " << viewportVals[2] << "x" << viewportVals[3]
                              << " scissor=" << (scissor ? "on" : "off")
                              << std::endl;
                }
            }
        } else if (IsUsingSDLRenderer() && sdlRenderer) {
            SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
            // SDL3 keeps SDL_RenderClear as-is
            SDL_RenderClear(sdlRenderer);
        }
#endif
    } else if (IsUsingGLFWBackend()) {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
            glViewport(0, 0, width, height);
            // Ensure we're using the default framebuffer
            
#ifdef GL_FRAMEBUFFER
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
            // Clear any previous GL error to avoid reporting unrelated issues
            (void)glGetError();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                static auto lastLog = std::chrono::steady_clock::time_point{};
                auto now = std::chrono::steady_clock::now();
                if (lastLog.time_since_epoch().count() == 0 || (now - lastLog) > std::chrono::seconds(1)) {
                    std::cerr << "OpenGL error in Clear(): " << err << " (" << DescribeGlError(err) << ")" << std::endl;
                    lastLog = now;
                }
                if (debugLogging_) {
                    GLint drawFbo = 0, readFbo = 0, viewportVals[4] = {0,0,0,0};
#ifdef GL_DRAW_FRAMEBUFFER_BINDING
                    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
#elif defined(GL_FRAMEBUFFER_BINDING)
                    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFbo);
#endif
#ifdef GL_READ_FRAMEBUFFER_BINDING
                    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
#endif
                    glGetIntegerv(GL_VIEWPORT, viewportVals);
                    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
                    std::cerr << "  GL state: drawFBO=" << drawFbo
                              << " readFBO=" << readFbo
                              << " viewport=" << viewportVals[0] << "," << viewportVals[1]
                              << " " << viewportVals[2] << "x" << viewportVals[3]
                              << " scissor=" << (scissor ? "on" : "off")
                              << std::endl;
                }
            }
        }
#endif
    }
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPopDebugGroup();
    }
#endif
}

void Viewport3D::Present() {
    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        if (IsUsingSDLGL()) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            SDL_GL_SwapWindow(sdlWindow);
        } else if (IsUsingSDLRenderer() && sdlRenderer) {
            SDL_RenderPresent(sdlRenderer);
        }
#endif
    } else if (IsUsingGLFWBackend()) {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwSwapBuffers(glfwWindow);
        }
#endif
    }
}

void Viewport3D::DrawMeshAt(double x,
                            double y,
                            double z,
                            const Mesh* meshOverride,
                            EntityMeshBinding* overrideBinding,
                            float scale,
                            char asciiChar) {
    const Mesh* meshToDraw = meshOverride;
    if (overrideBinding) {
        if (!overrideBinding->mesh.Empty()) {
            meshToDraw = &overrideBinding->mesh;
        }
        scale = (scale > 0.0f) ? scale : overrideBinding->scale;
    }

    if (!meshToDraw || meshToDraw->Empty()) {
        EnsurePlayerMesh();
        meshToDraw = &playerMesh_;
        scale = 0.85f;
    } else if (scale <= 0.0f) {
        scale = 1.0f;
    }

    // Try to get a default material for rendering
    std::shared_ptr<Nova::Material> material;
    if (materialLibrary_) {
        material = materialLibrary_->LoadMaterial("hull_plate");
    }

    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        if (IsUsingSDLGL()) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            glPushMatrix();
            glTranslatef(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z));
            const GLfloat playerScale = static_cast<GLfloat>(scale);
            glScalef(playerScale, playerScale, playerScale);
            const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
            if (cullEnabled) {
                glDisable(GL_CULL_FACE);
            }

            // Apply material if available
            if (material) {
                // For now, use a simple shader program - this will need to be improved
                // TODO: Get appropriate shader program for material rendering
                material->Bind(nullptr); // nullptr for now, will need shader program
            }

            PrimitiveMesh* primitiveToDraw = nullptr;
            if (meshToDraw == &playerMesh_) {
                primitiveToDraw = EnsureMeshPrimitive(playerMesh_, playerMeshPrimitive_, playerMeshPrimitiveDirty_);
            } else if (overrideBinding) {
                primitiveToDraw = EnsureMeshPrimitive(overrideBinding->mesh,
                                                      overrideBinding->primitive,
                                                      overrideBinding->primitiveDirty);
            }

            if (primitiveToDraw && primitiveToDraw->IsInitialized()) {
                primitiveToDraw->Draw();
            } else if (meshToDraw) {
                meshToDraw->Draw();
            }

            // Unbind material
            if (material) {
                material->Unbind();
            }

            if (cullEnabled) {
                glEnable(GL_CULL_FACE);
            }
            glPopMatrix();
        } else {
            int px = static_cast<int>(((x + 5.0) / 10.0) * width);
            int py = height / 2;
            const bool hasOverride = meshToDraw && meshToDraw != &playerMesh_;
            const float patchScale = hasOverride ? std::max(0.2f, scale) : 0.85f;
            const int halfSize = std::max(3, static_cast<int>(std::round(6.0f * patchScale)));
            SDL_Rect mainRect{px - halfSize, py - halfSize, halfSize * 2, halfSize * 2};
            if (hasOverride) {
                SDL_SetRenderDrawColor(sdlRenderer, 0, 220, 255, 255);
                compat_RenderFillRect(sdlRenderer, &mainRect);
                SDL_SetRenderDrawColor(sdlRenderer, 0, 64, 255, 255);
                compat_RenderDrawRect(sdlRenderer, &mainRect);
            } else {
                SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, 255);
                compat_RenderFillRect(sdlRenderer, &mainRect);
                SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 255);
                compat_RenderDrawRect(sdlRenderer, &mainRect);
                SDL_Rect centerDot{px - 2, py - 2, 4, 4};
                SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 255, 255);
                compat_RenderFillRect(sdlRenderer, &centerDot);
            }
        }
#else
        (void)x; (void)y; (void)z; (void)meshOverride; (void)scale; (void)asciiChar;
#endif
    }
#ifdef USE_GLFW
    else if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        glPushMatrix();
        glTranslatef(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z));
        const GLfloat playerScale = static_cast<GLfloat>(scale);
        glScalef(playerScale, playerScale, playerScale);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        if (cullEnabled) {
            glDisable(GL_CULL_FACE);
        }

        // Apply material if available
        if (material) {
            // For now, use a simple shader program - this will need to be improved
            // TODO: Get appropriate shader program for material rendering
            material->Bind(nullptr); // nullptr for now, will need shader program
        }

        PrimitiveMesh* primitiveToDraw = nullptr;
        if (meshToDraw == &playerMesh_) {
            primitiveToDraw = EnsureMeshPrimitive(playerMesh_, playerMeshPrimitive_, playerMeshPrimitiveDirty_);
        } else if (overrideBinding) {
            primitiveToDraw = EnsureMeshPrimitive(overrideBinding->mesh,
                                                  overrideBinding->primitive,
                                                  overrideBinding->primitiveDirty);
        }

        if (primitiveToDraw && primitiveToDraw->IsInitialized()) {
            primitiveToDraw->Draw();
        } else if (meshToDraw) {
            meshToDraw->Draw();
        }

        // Unbind material
        if (material) {
            material->Unbind();
        }

        if (cullEnabled) {
            glEnable(GL_CULL_FACE);
        }
        glPopMatrix();
    }
#endif
    else {
        const int widthChars = 40;
        double clamped = std::min(5.0, std::max(-5.0, x));
        int pos = static_cast<int>((clamped + 5.0) / 10.0 * (widthChars - 1));
        std::string line(widthChars, '-');
        line[pos] = asciiChar;
        std::cout << line << std::endl;
    }
}

void Viewport3D::DrawPlayer(double x, double y, double z) {
    if (debugLogging_) {
        std::cout << "Viewport3D::DrawPlayer() called at (" << x << ", " << y << ", " << z << ")" << std::endl;
        std::cout << "Viewport3D::DrawPlayer() - backend=" << RenderBackendToString(backend_) << std::endl;
    }
    DrawMeshAt(x, y, z, nullptr, nullptr, 0.85f, 'P');
}

void Viewport3D::DrawEntity(const Transform &t) {
    DrawMeshAt(t.x, t.y, t.z, nullptr, nullptr, 0.85f, 'E');
}

void Viewport3D::DrawEntity(Entity entity, const Transform& t) {
    const Mesh* overrideMesh = nullptr;
    float scale = 1.0f;
    char asciiChar = 'E';
    auto binding = entityMeshes_.find(entity);
    EntityMeshBinding* bindingPtr = nullptr;
    if (binding != entityMeshes_.end()) {
        bindingPtr = &binding->second;
        if (!bindingPtr->mesh.Empty()) {
            overrideMesh = &bindingPtr->mesh;
        }
        scale = bindingPtr->scale;
        asciiChar = 'S';
    }
    if (debugLogging_) {
        std::cout << "Viewport3D::DrawEntity(entity=" << entity << ")";
        if (overrideMesh) {
            std::cout << " with custom mesh";
        }
        std::cout << std::endl;
    }
    DrawMeshAt(t.x, t.y, t.z, overrideMesh, bindingPtr, scale, asciiChar);
}

void Viewport3D::SetEntityMesh(Entity entity, Mesh mesh, float scale) {
    auto& binding = entityMeshes_[entity];
    binding.mesh = std::move(mesh);
    binding.scale = (scale > 0.0f) ? scale : 1.0f;
    binding.primitive.reset();
    binding.primitiveDirty = true;
}

void Viewport3D::ClearEntityMesh(Entity entity) {
    entityMeshes_.erase(entity);
}

void Viewport3D::ClearEntityMeshes() {
    for (auto& entry : entityMeshes_) {
        entry.second.primitive.reset();
    }
    entityMeshes_.clear();
}

Mesh Viewport3D::CreatePlayerAvatarMesh() {
    // Builds a stylized arrowhead mesh so the player silhouette stands out from generic cubes.
    constexpr GLenum kTriangles = 0x0004u; // GL_TRIANGLES
    MeshBuilder builder(kTriangles);
    builder.ReserveVertices(32);
    builder.ReserveIndices(96);

    auto addTriangle = [&](const MeshVertex& a, const MeshVertex& b, const MeshVertex& c) {
        const GLuint base = builder.CurrentIndex();
        builder.AddVertex(a);
        builder.AddVertex(b);
        builder.AddVertex(c);
        builder.AddTriangle(base, base + 1, base + 2);
    };

    const MeshVertex nose(0.0f, 0.0f, 1.35f, 0.92f, 0.96f, 1.0f, 1.0f);
    const MeshVertex tail(0.0f, 0.0f, -1.35f, 0.18f, 0.22f, 0.4f, 1.0f);
    const MeshVertex leftWing(-0.95f, 0.0f, 0.0f, 0.32f, 0.55f, 0.95f, 1.0f);
    const MeshVertex rightWing(0.95f, 0.0f, 0.0f, 0.32f, 0.55f, 0.95f, 1.0f);
    const MeshVertex dorsal(0.0f, 0.68f, 0.0f, 0.78f, 0.86f, 1.0f, 1.0f);
    const MeshVertex ventral(0.0f, -0.68f, 0.0f, 0.08f, 0.12f, 0.24f, 1.0f);

    // Primary hull (diamond / octahedron)
    addTriangle(nose, dorsal, leftWing);
    addTriangle(nose, rightWing, dorsal);
    addTriangle(nose, ventral, rightWing);
    addTriangle(nose, leftWing, ventral);
    addTriangle(tail, leftWing, dorsal);
    addTriangle(tail, dorsal, rightWing);
    addTriangle(tail, rightWing, ventral);
    addTriangle(tail, ventral, leftWing);

    // Dorsal fin for silhouette contrast
    const MeshVertex dorsalA(-0.25f, 0.68f, -0.42f, 0.42f, 0.85f, 1.0f, 1.0f);
    const MeshVertex dorsalB(0.25f, 0.68f, -0.42f, 0.42f, 0.85f, 1.0f, 1.0f);
    const MeshVertex dorsalTip(0.0f, 1.12f, -0.28f, 0.58f, 0.95f, 1.0f, 1.0f);
    addTriangle(dorsalA, dorsalTip, dorsalB);
    addTriangle(dorsalA, tail, dorsalTip);
    addTriangle(dorsalB, dorsalTip, tail);

    // Ventral stabilizer
    const MeshVertex ventralA(-0.22f, -0.68f, -0.36f, 0.16f, 0.32f, 0.58f, 1.0f);
    const MeshVertex ventralB(0.22f, -0.68f, -0.36f, 0.16f, 0.32f, 0.58f, 1.0f);
    const MeshVertex ventralTip(0.0f, -1.05f, -0.18f, 0.28f, 0.45f, 0.82f, 1.0f);
    addTriangle(ventralA, ventralB, ventralTip);
    addTriangle(ventralA, ventralTip, tail);
    addTriangle(ventralB, tail, ventralTip);

    // Engine ring around the tail for visual detail
    const MeshVertex engineTop(0.0f, 0.34f, -1.1f, 0.95f, 0.58f, 0.22f, 1.0f);
    const MeshVertex engineBottom(0.0f, -0.34f, -1.1f, 0.95f, 0.58f, 0.22f, 1.0f);
    const MeshVertex engineLeft(-0.34f, 0.0f, -1.1f, 0.85f, 0.48f, 0.2f, 1.0f);
    const MeshVertex engineRight(0.34f, 0.0f, -1.1f, 0.85f, 0.48f, 0.2f, 1.0f);

    addTriangle(tail, engineLeft, engineTop);
    addTriangle(tail, engineTop, engineRight);
    addTriangle(tail, engineRight, engineBottom);
    addTriangle(tail, engineBottom, engineLeft);
    addTriangle(engineLeft, engineTop, engineRight);
    addTriangle(engineLeft, engineRight, engineBottom);

    return builder.Build();
}

void Viewport3D::DrawStaticGrid() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }

    EnsureLineBatcher3D();
    if (!lineBatcher3D_) {
        return;
    }

    constexpr int kHalfSteps = 30;
    constexpr float kSpacing = 1.0f;
    constexpr float kMinorIntensity = 0.28f;
    constexpr float kMajorIntensity = 0.45f;
    constexpr float kOriginIntensity = 0.7f;
    constexpr float kMinorAlpha = 0.35f;
    constexpr float kMajorAlpha = 0.5f;
    constexpr float kOriginAlpha = 0.75f;

    const float halfSpan = static_cast<float>(kHalfSteps) * kSpacing;
    const float y = 0.0f;

    lineBatcher3D_->Begin();
    lineBatcher3D_->SetLineWidth(1.0f);

    auto addGridLine = [&](float x0, float y0, float z0, float x1, float y1, float z1, int index) {
        bool isOrigin = (index == 0);
        bool isMajor = (index % 5 == 0);
        float intensity = isOrigin ? kOriginIntensity : (isMajor ? kMajorIntensity : kMinorIntensity);
        float alpha = isOrigin ? kOriginAlpha : (isMajor ? kMajorAlpha : kMinorAlpha);
        lineBatcher3D_->AddLine(x0, y0, z0, x1, y1, z1, intensity, intensity, intensity, alpha);
    };

    for (int i = -kHalfSteps; i <= kHalfSteps; ++i) {
        float x = static_cast<float>(i) * kSpacing;
        addGridLine(x, y, -halfSpan, x, y, halfSpan, i);
    }

    for (int j = -kHalfSteps; j <= kHalfSteps; ++j) {
        float z = static_cast<float>(j) * kSpacing;
        addGridLine(-halfSpan, y, z, halfSpan, y, z, j);
    }

    lineBatcher3D_->Flush();
#endif
}

void Viewport3D::Resize(int w, int h) {
    width = w; height = h;
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!isFullscreen_) {
        windowedWidth_ = w;
        windowedHeight_ = h;
    }
#endif
    if (debugLogging_) std::cout << "Viewport3D Resized to " << width << "x" << height << std::endl;
}

void Viewport3D::Shutdown() {
#if defined(USE_GLFW) || defined(USE_SDL)
    // Ensure GL context is current prior to destroying GL resources
#if defined(USE_SDL)
    if (sdlWindow && sdlGLContext) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    if (glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif
    DestroyPrimitiveBuffers();
    if (lineBatcher3D_) { lineBatcher3D_->Cleanup(); lineBatcher3D_.reset(); }
    if (uiBatcher_) { uiBatcher_->Cleanup(); uiBatcher_.reset(); }
    if (particleRenderer_) {
        particleRenderer_->Cleanup();
        particleRenderer_.reset();
    }
    if (materialLibrary_) {
        materialLibrary_.reset();
    }
    if (instancedRenderer_) {
        instancedRenderer_->Clear();
        instancedRenderer_.reset();
    }
    if (playerHudTextureGL_ != 0) {
        glDeleteTextures(1, &playerHudTextureGL_);
        playerHudTextureGL_ = 0;
        playerHudTextureGLWidth_ = 0;
        playerHudTextureGLHeight_ = 0;
        playerHudTextureGLFailed_ = false;
    }
    if (playerMeshInitialized_) {
        playerMesh_.Clear();
        playerMeshInitialized_ = false;
    }
    if (cubePrimitive_) {
        cubePrimitive_->Cleanup();
        cubePrimitive_.reset();
    }
    if (playerPatchPrimitive_) {
        playerPatchPrimitive_->Cleanup();
        playerPatchPrimitive_.reset();
    }
    if (hudTexturePrimitive_) {
        hudTexturePrimitive_->Cleanup();
        hudTexturePrimitive_.reset();
    }
    hudTexturePrimitiveDirty_ = true;
    entityMeshes_.clear();
#endif
#ifdef USE_SDL
    if (IsUsingSDLBackend() || sdlWindow || sdlRenderer || sdlGLContext) {
        if (spaceshipHudTexture_) {
            SDL_DestroyTexture(spaceshipHudTexture_);
            spaceshipHudTexture_ = nullptr;
            spaceshipHudTextureWidth_ = 0;
            spaceshipHudTextureHeight_ = 0;
            spaceshipHudTextureFailed_ = false;
        }
        if (sdlRenderer) {
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = nullptr;
        }
        if (sdlGLContext) {
            compat_GL_DeleteContext(sdlGLContext);
            sdlGLContext = nullptr;
        }
        if (sdlWindow) {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = nullptr;
        }
        SDL_Quit();
    }
#endif
#ifdef USE_GLFW
    if (IsUsingGLFWBackend() || glfwWindow) {
        if (glfwWindow) {
            glfwDestroyWindow(glfwWindow);
            glfwWindow = nullptr;
        }
        glfwTerminate();
    }
#endif
    SetBackend(RenderBackend::None);
}

void Viewport3D::DrawCoordinateSystem() {
    // Optional toggle
#ifndef NDEBUG
    if (!g_ShowWorldAxes) {
        return;
    }
#endif
    if (IsUsingSDLGL()) {
#ifdef USE_SDL
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
#endif
    } else if (IsUsingGLFWBackend()) {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
        }
#endif
    }

    if (IsUsingGLBackend()) {
        // Save current matrices
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

    // Keep depth testing enabled so axes are drawn in world space and can be occluded
        // Draw coordinate system axes using the line batcher
        // Use current perspective projection instead of orthographic
    float axisLength = 10.0f;
#ifndef NDEBUG
    axisLength = g_WorldAxisLength;
#endif
        EnsureLineBatcher3D();
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
        float lw = 3.0f;
#ifndef NDEBUG
        lw = g_WorldAxisLineWidth;
#endif
        lineBatcher3D_->SetLineWidth(lw);
            // X axis (red)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
            // Y axis (green)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            // Z axis (blue)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 1.0f);
            lineBatcher3D_->Flush();
        }

        // Label axes so orientation is clear even when grid is dense
        const float labelOffset = axisLength + 0.35f;
        const float negativeOffset = -axisLength - 0.55f;
        const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);
        TextRenderer::RenderText3D("+X", labelOffset, 0.0f, 0.0f, TextColor::Red(), FontSize::Small);
        TextRenderer::RenderText3D("-X", negativeOffset, 0.0f, 0.0f, TextColor::Red(), FontSize::Small);
        TextRenderer::RenderText3D("+Y", 0.0f, labelOffset, 0.0f, TextColor::Green(), FontSize::Small);
        TextRenderer::RenderText3D("-Y", 0.0f, negativeOffset, 0.0f, TextColor::Green(), FontSize::Small);
        TextRenderer::RenderText3D("+Z", 0.0f, 0.0f, labelOffset, TextColor::Blue(), FontSize::Small);
        TextRenderer::RenderText3D("-Z", 0.0f, 0.0f, negativeOffset, TextColor::Blue(), FontSize::Small);
        if (depthWasEnabled) {
            glEnable(GL_DEPTH_TEST);
        }

    glLineWidth(1.0f);

        // Restore matrices
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
}

void Viewport3D::EnsureLineBatcher3D() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
    if (!lineBatcher3D_) {
        lineBatcher3D_ = std::make_unique<LineBatcher3D>();
        if (!lineBatcher3D_->Init()) {
            if (debugLogging_) {
                std::cerr << "Viewport3D::EnsureLineBatcher3D: Init failed (VBO creation)" << std::endl;
            }
            lineBatcher3D_.reset();
        }
    }
#endif
}

void Viewport3D::InitializeShaderManager() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!shaderManager_) {
        shaderManager_ = std::make_unique<Nova::ShaderManager>();
        if (debugLogging_) {
            std::cout << "Viewport3D: ShaderManager initialized" << std::endl;
        }
    }
#endif
}

void Viewport3D::TickShaderHotReload() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!enableShaderHotReload_ || !shaderManager_ || !IsUsingGLBackend()) {
        return;
    }

    const int reloaded = shaderManager_->ReloadModifiedShaders();
    if (reloaded > 0 && debugLogging_) {
        std::cout << "Viewport3D: hot-reloaded " << reloaded << " shader(s)" << std::endl;
    }
#endif
}

double Viewport3D::SampleSpeed(double x, double y, double z) {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    double speed = 0.0;
    if (haveHudSample_) {
        const double dt = std::chrono::duration<double>(now - lastHudTime_).count();
        if (dt > 1e-4) {
            const double dx = x - lastHudX_;
            const double dy = y - lastHudY_;
            const double dz = z - lastHudZ_;
            const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            speed = dist / dt; // world units per second
        }
    }
    lastHudX_ = x;
    lastHudY_ = y;
    lastHudZ_ = z;
    lastHudTime_ = now;
    haveHudSample_ = true;
    return speed;
}

void Viewport3D::DrawCameraVisual(const class Camera* camera, double playerX, double playerY, double playerZ, bool targetLocked) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!camera) return;

    (void)playerX;
    (void)playerY;
    (void)playerZ;

    auto drawCameraDebug = [&]() {
        glDisable(GL_DEPTH_TEST); // Draw on top

        struct Vec3 {
            double x;
            double y;
            double z;
        };

        auto normalize = [](const Vec3& v) {
            const double len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (len < 1e-6) {
                return Vec3{0.0, 0.0, 0.0};
            }
            return Vec3{v.x / len, v.y / len, v.z / len};
        };

        const Camera::Basis camBasis = camera->BuildBasis(true);
        Vec3 forward = normalize(Vec3{camBasis.forwardX, camBasis.forwardY, camBasis.forwardZ});
        Vec3 right = normalize(Vec3{camBasis.rightX, camBasis.rightY, camBasis.rightZ});
        Vec3 up = normalize(Vec3{camBasis.upX, camBasis.upY, camBasis.upZ});
        Vec3 cameraPos{camera->x(), camera->y(), camera->z()};

        auto localToWorld = [&](const Vec3& local) {
            return Vec3{
                cameraPos.x + local.x * right.x + local.y * up.x + local.z * forward.x,
                cameraPos.y + local.x * right.y + local.y * up.y + local.z * forward.y,
                cameraPos.z + local.x * right.z + local.y * up.z + local.z * forward.z};
        };

        EnsureLineBatcher3D();
        if (!lineBatcher3D_) {
            glEnable(GL_DEPTH_TEST);
            return;
        }

        auto addLineWorld = [&](const Vec3& a, const Vec3& b, float r, float g, float bcol, float acol = 1.0f) {
            lineBatcher3D_->AddLine(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z),
                                    static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z),
                                    r, g, bcol, acol);
        };

        auto addPointWorld = [&](const Vec3& p, float r, float g, float bcol, float acol = 1.0f) {
            lineBatcher3D_->AddPoint(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z),
                                     r, g, bcol, acol);
        };

        auto addLineLocal = [&](float ax, float ay, float az, float bx, float by, float bz,
                                float r, float g, float bcol, float acol = 1.0f) {
            const Vec3 worldA = localToWorld(Vec3{ax, ay, az});
            const Vec3 worldB = localToWorld(Vec3{bx, by, bz});
            addLineWorld(worldA, worldB, r, g, bcol, acol);
        };

        // Camera body (rectangular prism) - draw edges with line batcher (no immediate mode)
        const float bodyX0 = -0.4f, bodyX1 = 0.4f;
        const float bodyY0 = -0.2f, bodyY1 = 0.2f;
        const float bodyFront = 0.1f, bodyBack = -0.3f;
        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(2.0f);
        const float bodyR = 0.8f, bodyG = 0.8f, bodyB = 0.8f;
        addLineLocal(bodyX0, bodyY0, bodyFront, bodyX1, bodyY0, bodyFront, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY0, bodyFront, bodyX1, bodyY1, bodyFront, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY1, bodyFront, bodyX0, bodyY1, bodyFront, bodyR, bodyG, bodyB);
        addLineLocal(bodyX0, bodyY1, bodyFront, bodyX0, bodyY0, bodyFront, bodyR, bodyG, bodyB);
        addLineLocal(bodyX0, bodyY0, bodyBack, bodyX1, bodyY0, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY0, bodyBack, bodyX1, bodyY1, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY1, bodyBack, bodyX0, bodyY1, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX0, bodyY1, bodyBack, bodyX0, bodyY0, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX0, bodyY0, bodyFront, bodyX0, bodyY0, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY0, bodyFront, bodyX1, bodyY0, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX1, bodyY1, bodyFront, bodyX1, bodyY1, bodyBack, bodyR, bodyG, bodyB);
        addLineLocal(bodyX0, bodyY1, bodyFront, bodyX0, bodyY1, bodyBack, bodyR, bodyG, bodyB);
        lineBatcher3D_->Flush();

        const float lensOuter = 0.15f;
        const float lensInner = 0.10f;
        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(2.0f);
        addLineLocal(-lensOuter, -lensOuter, bodyFront + 0.001f,  lensOuter, -lensOuter, bodyFront + 0.001f, 0.2f, 0.2f, 0.2f);
        addLineLocal( lensOuter, -lensOuter, bodyFront + 0.001f,  lensOuter,  lensOuter, bodyFront + 0.001f, 0.2f, 0.2f, 0.2f);
        addLineLocal( lensOuter,  lensOuter, bodyFront + 0.001f, -lensOuter,  lensOuter, bodyFront + 0.001f, 0.2f, 0.2f, 0.2f);
        addLineLocal(-lensOuter,  lensOuter, bodyFront + 0.001f, -lensOuter, -lensOuter, bodyFront + 0.001f, 0.2f, 0.2f, 0.2f);
        addLineLocal(-lensInner, -lensInner, bodyFront + 0.002f,  lensInner, -lensInner, bodyFront + 0.002f, 0.9f, 0.9f, 1.0f);
        addLineLocal( lensInner, -lensInner, bodyFront + 0.002f,  lensInner,  lensInner, bodyFront + 0.002f, 0.9f, 0.9f, 1.0f);
        addLineLocal( lensInner,  lensInner, bodyFront + 0.002f, -lensInner,  lensInner, bodyFront + 0.002f, 0.9f, 0.9f, 1.0f);
        addLineLocal(-lensInner,  lensInner, bodyFront + 0.002f, -lensInner, -lensInner, bodyFront + 0.002f, 0.9f, 0.9f, 1.0f);
        lineBatcher3D_->Flush();

        // Coordinate system at camera position (world axes)
        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(2.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x + 1.5, cameraPos.y, cameraPos.z}, 1.0f, 0.0f, 0.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x, cameraPos.y + 1.5, cameraPos.z}, 0.0f, 1.0f, 0.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x, cameraPos.y, cameraPos.z + 1.5}, 0.0f, 0.0f, 1.0f);
        lineBatcher3D_->Flush();

        // Camera basis vectors (forward/right/up)
        const float vecLen = 2.5f;
        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(2.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x + forward.x * vecLen,
                                     cameraPos.y + forward.y * vecLen,
                                     cameraPos.z + forward.z * vecLen}, 1.0f, 1.0f, 0.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x + right.x * vecLen,
                                     cameraPos.y + right.y * vecLen,
                                     cameraPos.z + right.z * vecLen}, 0.0f, 1.0f, 1.0f);
        addLineWorld(cameraPos, Vec3{cameraPos.x + up.x * vecLen,
                                     cameraPos.y + up.y * vecLen,
                                     cameraPos.z + up.z * vecLen}, 1.0f, 0.0f, 1.0f);
        lineBatcher3D_->Flush();

        // Look-at target marker
        const double lookAtDistance = targetLocked ? 6.0 : 4.0;
        const Vec3 lookAtWorld{cameraPos.x + forward.x * lookAtDistance,
                               cameraPos.y + forward.y * lookAtDistance,
                               cameraPos.z + forward.z * lookAtDistance};
        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(2.0f);
        const float targetR = targetLocked ? 0.2f : 0.9f;
        const float targetG = targetLocked ? 1.0f : 0.7f;
        const float targetB = targetLocked ? 0.2f : 0.2f;
        lineBatcher3D_->SetPointSize(6.0f);
        addLineWorld(Vec3{lookAtWorld.x - 0.25, lookAtWorld.y, lookAtWorld.z},
                     Vec3{lookAtWorld.x + 0.25, lookAtWorld.y, lookAtWorld.z},
                     targetR, targetG, targetB);
        addLineWorld(Vec3{lookAtWorld.x, lookAtWorld.y - 0.25, lookAtWorld.z},
                     Vec3{lookAtWorld.x, lookAtWorld.y + 0.25, lookAtWorld.z},
                     targetR, targetG, targetB);
        addLineWorld(Vec3{lookAtWorld.x, lookAtWorld.y, lookAtWorld.z - 0.25},
                     Vec3{lookAtWorld.x, lookAtWorld.y, lookAtWorld.z + 0.25},
                     targetR, targetG, targetB);
        addPointWorld(lookAtWorld, targetR, targetG, targetB);
        lineBatcher3D_->Flush();

        // Camera frustum visualization in world space
        const double fovRadians = camera->zoom() * (std::acos(-1.0) / 180.0);
        const double aspect = (height != 0) ? static_cast<double>(width) / static_cast<double>(height) : 1.0;
        const double nearDist = 0.1;
        const double farDist = 5.0;
        const double halfTan = std::tan(fovRadians * 0.5);

        const Vec3 nearCenter{cameraPos.x + forward.x * nearDist,
                              cameraPos.y + forward.y * nearDist,
                              cameraPos.z + forward.z * nearDist};
        const Vec3 farCenter{cameraPos.x + forward.x * farDist,
                             cameraPos.y + forward.y * farDist,
                             cameraPos.z + forward.z * farDist};

        const Vec3 nearUp{up.x * halfTan * nearDist, up.y * halfTan * nearDist, up.z * halfTan * nearDist};
        const Vec3 nearRight{right.x * halfTan * nearDist * aspect,
                             right.y * halfTan * nearDist * aspect,
                             right.z * halfTan * nearDist * aspect};
        const Vec3 farUp{up.x * halfTan * farDist, up.y * halfTan * farDist, up.z * halfTan * farDist};
        const Vec3 farRight{right.x * halfTan * farDist * aspect,
                            right.y * halfTan * farDist * aspect,
                            right.z * halfTan * farDist * aspect};

        auto addVec = [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
        };
        auto subVec = [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
        };

        const Vec3 nearTL = addVec(subVec(nearCenter, nearRight), nearUp);
        const Vec3 nearTR = addVec(addVec(nearCenter, nearRight), nearUp);
        const Vec3 nearBL = subVec(subVec(nearCenter, nearRight), nearUp);
        const Vec3 nearBR = subVec(addVec(nearCenter, nearRight), nearUp);
        const Vec3 farTL = addVec(subVec(farCenter, farRight), farUp);
        const Vec3 farTR = addVec(addVec(farCenter, farRight), farUp);
        const Vec3 farBL = subVec(subVec(farCenter, farRight), farUp);
        const Vec3 farBR = subVec(addVec(farCenter, farRight), farUp);

        lineBatcher3D_->Begin();
        lineBatcher3D_->SetLineWidth(1.5f);
        const float fr = 1.0f, fg = 0.5f, fb = 0.0f;
        addLineWorld(nearTL, nearTR, fr, fg, fb);
        addLineWorld(nearTR, nearBR, fr, fg, fb);
        addLineWorld(nearBR, nearBL, fr, fg, fb);
        addLineWorld(nearBL, nearTL, fr, fg, fb);
        addLineWorld(farTL, farTR, fr, fg, fb);
        addLineWorld(farTR, farBR, fr, fg, fb);
        addLineWorld(farBR, farBL, fr, fg, fb);
        addLineWorld(farBL, farTL, fr, fg, fb);
        addLineWorld(nearTL, farTL, fr, fg, fb);
        addLineWorld(nearTR, farTR, fr, fg, fb);
        addLineWorld(nearBL, farBL, fr, fg, fb);
        addLineWorld(nearBR, farBR, fr, fg, fb);
        lineBatcher3D_->Flush();

        glEnable(GL_DEPTH_TEST);
    };

    if (IsUsingSDLGL()) {
#ifdef USE_SDL
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
        drawCameraDebug();
#endif
    } else if (IsUsingGLFWBackend()) {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
            drawCameraDebug();
        }
#endif
    }
#else
    (void)camera;
#endif
}

void Viewport3D::DrawCameraMarker(const class Camera* camera) {
    (void)camera;
    if (!IsUsingSDLBackend()) return;
#ifdef USE_SDL
    if (!sdlRenderer || !camera) return;
    // Draw a small cross at the center of the screen
    int cx = width / 2;
    int cy = height / 2;
    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 255, 255);
    compat_RenderDrawLine(sdlRenderer, cx - 8, cy, cx + 8, cy);
    compat_RenderDrawLine(sdlRenderer, cx, cy - 8, cx, cy + 8);
#endif
}

void Viewport3D::DrawCameraDebug(const class Camera* camera, double playerX, double playerY, double playerZ, ViewRole role, bool targetLocked) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!camera || role == ViewRole::Minimap) {
        return;
    }

    if (IsUsingGLBackend()) {
        glPushMatrix();
        DrawStaticGrid();
        // Draw world coordinate system at origin
        DrawCoordinateSystem();
        glPopMatrix();

        double camDistToPlayer = std::sqrt((camera->x() - playerX) * (camera->x() - playerX) +
                                           (camera->y() - playerY) * (camera->y() - playerY) +
                                           (camera->z() - playerZ) * (camera->z() - playerZ));
        if (camDistToPlayer > 3.0) {
            DrawCameraVisual(camera, playerX, playerY, playerZ, targetLocked);
        }
    }
#else
    (void)camera;
    (void)playerX;
    (void)playerY;
    (void)playerZ;
    (void)role;
#endif
}

#ifdef USE_SDL
// --- Minimal SDL bitmap text (5x5 glyphs) for non-GL renderer ---
struct SDLMiniFontGlyph { const uint8_t* cols; int w; int h; }; // 5x5

static const uint8_t kGlyphDigits[][5] = {
    {0x1F,0x11,0x11,0x11,0x1F}, // 0
    {0x04,0x06,0x04,0x04,0x07}, // 1
    {0x1F,0x01,0x1F,0x10,0x1F}, // 2
    {0x1F,0x01,0x1F,0x01,0x1F}, // 3
    {0x11,0x11,0x1F,0x01,0x01}, // 4
    {0x1F,0x10,0x1F,0x01,0x1F}, // 5
    {0x1F,0x10,0x1F,0x11,0x1F}, // 6
    {0x1F,0x01,0x02,0x04,0x04}, // 7
    {0x1F,0x11,0x1F,0x11,0x1F}, // 8
    {0x1F,0x11,0x1F,0x01,0x1F}, // 9
};
static const uint8_t kGlyphA[5] = {0x0E, 0x11, 0x1F, 0x11, 0x11};
static const uint8_t kGlyphC[5] = {0x0E, 0x10, 0x10, 0x10, 0x0E};
static const uint8_t kGlyphF[5] = {0x1F, 0x10, 0x1E, 0x10, 0x10};
static const uint8_t kGlyphG[5] = {0x0E, 0x10, 0x17, 0x11, 0x0E};
static const uint8_t kGlyphN[5] = {0x11, 0x19, 0x15, 0x13, 0x11};
static const uint8_t kGlyphO[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t kGlyphP[5] = {0x1E, 0x11, 0x1E, 0x10, 0x10};
static const uint8_t kGlyphS[5] = {0x1F, 0x10, 0x1F, 0x01, 0x1F};
static const uint8_t kGlyphT[5] = {0x1F, 0x04, 0x04, 0x04, 0x04};
static const uint8_t kGlyphV[5] = {0x11, 0x11, 0x0A, 0x0A, 0x04};
static const uint8_t kGlyphX[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
static const uint8_t kGlyphY[5] = {0x11, 0x0A, 0x04, 0x04, 0x04};
static const uint8_t kGlyphZ[5] = {0x1F, 0x02, 0x04, 0x08, 0x1F};

static SDLMiniFontGlyph SDLMiniFont_Get(char c) {
    if (c >= '0' && c <= '9') return {kGlyphDigits[c - '0'], 5, 5};
    switch (c) {
        case 'A': return {kGlyphA, 5, 5};
        case 'C': return {kGlyphC, 5, 5};
        case 'F': return {kGlyphF, 5, 5};
        case 'G': return {kGlyphG, 5, 5};
        case 'N': return {kGlyphN, 5, 5};
        case 'O': return {kGlyphO, 5, 5};
        case 'P': return {kGlyphP, 5, 5};
        case 'S': return {kGlyphS, 5, 5};
        case 'T': return {kGlyphT, 5, 5};
        case 'V': return {kGlyphV, 5, 5};
        case 'X': return {kGlyphX, 5, 5};
        case 'Y': return {kGlyphY, 5, 5};
        case 'Z': return {kGlyphZ, 5, 5};
        default: return {nullptr, 0, 0};
    }
}

static int SDLMiniFont_DrawText(SDL_Renderer* r, int x, int y, SDL_Color color, int scale, const char* text, bool draw) {
    if (!r || !text) return 0;
    const int glyphSpacing = 1; // 1 px between columns baseline
    int cx = x;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    auto drawGlyph = [&](const SDLMiniFontGlyph& g){
        if (!g.cols) { cx += (5 + glyphSpacing) * scale; return; }
        if (draw) {
            for (int col = 0; col < g.w; ++col) {
                uint8_t bits = g.cols[col];
                for (int row = 0; row < g.h; ++row) {
                    if (bits & (1 << (g.h - 1 - row))) {
                        SDL_Rect px{ cx + col*(scale), y + row*(scale), scale, scale };
                        compat_RenderFillRect(r, &px);
                    }
                }
            }
        }
        cx += (g.w + glyphSpacing) * scale;
    };
    for (const char* p = text; *p; ++p) {
        if (*p == ' ') { cx += (3 + glyphSpacing) * scale; continue; }
        if (*p == '.') {
            if (draw) {
                SDL_Rect px{ cx + 4*scale, y + 4*scale, scale, scale };
                compat_RenderFillRect(r, &px);
            }
            cx += (2 + glyphSpacing) * scale;
            continue;
        }
        if (*p == ':') {
            if (draw) {
                SDL_Rect d1{ cx + 2*scale, y + 1*scale, scale, scale };
                SDL_Rect d2{ cx + 2*scale, y + 3*scale, scale, scale };
                compat_RenderFillRect(r, &d1);
                compat_RenderFillRect(r, &d2);
            }
            cx += (2 + glyphSpacing) * scale;
            continue;
        }
        if (*p == '-') {
            if (draw) {
                SDL_Rect mid{ cx, y + 2*scale, 5*scale, scale };
                compat_RenderFillRect(r, &mid);
            }
            cx += (5 + glyphSpacing) * scale;
            continue;
        }
        SDLMiniFontGlyph g = SDLMiniFont_Get(static_cast<char>(std::toupper(*p)));
        drawGlyph(g);
    }
    return cx - x;
}

static int SDLMiniFont_RenderText(SDL_Renderer* r, int x, int y, SDL_Color color, int scale, const char* text) {
    return SDLMiniFont_DrawText(r, x, y, color, scale, text, true);
}

static int SDLMiniFont_MeasureText(SDL_Renderer* r, int scale, const char* text) {
    (void)r; return SDLMiniFont_DrawText(nullptr, 0, 0, SDL_Color{0,0,0,0}, scale, text, false);
}
#endif // USE_SDL

void Viewport3D::RenderMenuOverlay(const MainMenu::RenderData& menuData) {
#if defined(USE_GLFW) || defined(USE_SDL)
#ifdef USE_GLFW
    if (!IsUsingGLBackend() || width <= 0 || height <= 0) {
        return;
    }

    auto toTextColor = [](const MenuSystem::MenuStyle::Color& color, float alphaMultiplier = 1.0f) {
        return TextColor(
            static_cast<float>(color.r) / 255.0f,
            static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f,
            static_cast<float>(color.a) / 255.0f * alphaMultiplier);
    };

    auto toFontSize = [](float requestedSize) {
        if (requestedSize >= 56.0f) {
            return FontSize::Large;
        }
        if (requestedSize >= 28.0f) {
            return FontSize::Medium;
        }
        if (requestedSize >= 18.0f) {
            return FontSize::Fixed;
        }
        return FontSize::Small;
    };

    const FontSize titleFont = toFontSize(menuData.style.titleFontSize);
    const FontSize subtitleFont = toFontSize(menuData.style.subtitleFontSize);
    const FontSize itemFont = toFontSize(menuData.style.itemFontSize);
    const FontSize footerFont = toFontSize(menuData.style.footerFontSize);
    const FontSize descriptionFont = FontSize::Small;

    const int titleHeight = TextRenderer::GetFontHeight(titleFont);
    const int subtitleHeight = TextRenderer::GetFontHeight(subtitleFont);
    const int itemHeight = TextRenderer::GetFontHeight(itemFont);
    const int footerHeight = TextRenderer::GetFontHeight(footerFont);
    const int descriptionHeight = TextRenderer::GetFontHeight(descriptionFont);

    std::vector<const MenuSystem::MenuItem*> visibleItems;
    visibleItems.reserve(menuData.items.size());
    for (const auto& item : menuData.items) {
        if (item.visible) {
            visibleItems.push_back(&item);
        }
    }

    const MenuSystem::MenuItem* selectedItem = nullptr;
    if (menuData.selectedIndex >= 0 && menuData.selectedIndex < static_cast<int>(menuData.items.size())) {
        const auto& candidate = menuData.items[menuData.selectedIndex];
        if (candidate.visible) {
            selectedItem = &candidate;
        }
    }

    int maxLineWidth = 0;
    if (!menuData.title.empty()) {
        maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(menuData.title, titleFont));
    }
    if (!menuData.subtitle.empty()) {
        maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(menuData.subtitle, subtitleFont));
    }
    for (const auto* item : visibleItems) {
        maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(item->text, itemFont));
    }
    if (selectedItem && !selectedItem->description.empty()) {
        maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(selectedItem->description, descriptionFont));
    }
    if (!menuData.footer.empty()) {
        maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(menuData.footer, footerFont));
    }
    maxLineWidth = std::max(maxLineWidth, 320);

    const float centerX = static_cast<float>(width) * 0.5f;
    const float baseY = static_cast<float>(height) * 0.25f;

    float cursorY = baseY;
    float titleBaseline = 0.0f;
    float subtitleBaseline = 0.0f;
    std::vector<float> itemBaselines;
    itemBaselines.reserve(visibleItems.size());
    float descriptionBaseline = 0.0f;
    float footerBaseline = 0.0f;

    if (!menuData.title.empty()) {
        cursorY += static_cast<float>(titleHeight);
        titleBaseline = cursorY;
    }
    if (!menuData.subtitle.empty()) {
        cursorY += menuData.style.subtitleSpacing;
        cursorY += static_cast<float>(subtitleHeight);
        subtitleBaseline = cursorY;
    }

    cursorY += menuData.style.titleSpacing;

    for (size_t i = 0; i < visibleItems.size(); ++i) {
        cursorY += static_cast<float>(itemHeight);
        itemBaselines.push_back(cursorY);
        if (i + 1 < visibleItems.size()) {
            cursorY += menuData.style.itemSpacing;
        }
    }

    if (selectedItem && !selectedItem->description.empty()) {
        cursorY += std::max(menuData.style.itemSpacing * 0.5f, 24.0f);
        cursorY += static_cast<float>(descriptionHeight);
        descriptionBaseline = cursorY;
    }

    if (!menuData.footer.empty()) {
        cursorY += menuData.style.footerSpacing;
        cursorY += static_cast<float>(footerHeight);
        footerBaseline = cursorY;
    }

    const float backgroundWidth = static_cast<float>(maxLineWidth) + menuData.style.backgroundPadding * 2.0f;
    const float contentHeight = std::max(cursorY - baseY, static_cast<float>(itemHeight));
    const float backgroundHeight = contentHeight + menuData.style.backgroundPadding * 2.0f;
    const float backgroundLeft = centerX - backgroundWidth * 0.5f;
    const float backgroundTop = baseY - menuData.style.backgroundPadding;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Begin batched UI if available
    if (uiBatcher_) {
        uiBatcher_->Begin(width, height);
    }

    if (menuData.style.drawBackground) {
        const auto bg = menuData.style.backgroundColor;
        const float bgR = static_cast<float>(bg.r) / 255.0f;
        const float bgG = static_cast<float>(bg.g) / 255.0f;
        const float bgB = static_cast<float>(bg.b) / 255.0f;
        const float bgA = static_cast<float>(bg.a) / 255.0f;

        if (uiBatcher_) {
            uiBatcher_->AddQuad(backgroundLeft, backgroundTop, backgroundWidth, backgroundHeight,
                                 bgR, bgG, bgB, bgA);
            const float brA = std::min(1.0f, bgA + 0.15f);
            uiBatcher_->AddRectOutline(backgroundLeft, backgroundTop, backgroundWidth, backgroundHeight,
                                       1.5f, bgR, bgG, bgB, brA);
        }
    }

    if (titleBaseline > 0.0f && !menuData.title.empty()) {
        TextRenderer::RenderTextAligned(menuData.title,
                                        static_cast<int>(centerX),
                                        static_cast<int>(titleBaseline),
                                        TextAlign::Center,
                                        toTextColor(menuData.style.titleColor),
                                        titleFont);
    }

    if (subtitleBaseline > 0.0f && !menuData.subtitle.empty()) {
        TextRenderer::RenderTextAligned(menuData.subtitle,
                                        static_cast<int>(centerX),
                                        static_cast<int>(subtitleBaseline),
                                        TextAlign::Center,
                                        toTextColor(menuData.style.subtitleColor, 0.9f),
                                        subtitleFont);
    }

    for (size_t i = 0; i < visibleItems.size(); ++i) {
        const auto* item = visibleItems[i];
        const bool isSelected = (menuData.selectedIndex >= 0 &&
                                 menuData.selectedIndex < static_cast<int>(menuData.items.size()) &&
                                 &menuData.items[menuData.selectedIndex] == item);

        TextColor color;
        if (!item->enabled) {
            color = toTextColor(menuData.style.disabledColor, 0.75f);
        } else if (isSelected) {
            color = toTextColor(menuData.style.selectedColor, menuData.selectedItemAlpha);
        } else {
            color = toTextColor(menuData.style.normalColor);
        }

        const float baseline = itemBaselines[i];
        TextRenderer::RenderTextAligned(item->text,
                                        static_cast<int>(centerX),
                                        static_cast<int>(baseline),
                                        TextAlign::Center,
                                        color,
                                        itemFont);

        if (isSelected) {
            const float indicatorAlpha = std::clamp(menuData.selectedItemAlpha, 0.0f, 1.0f);
            const float indicatorHeight = static_cast<float>(itemHeight) * std::max(menuData.selectedItemScale, 1.0f);
            const float indicatorHalf = indicatorHeight * 0.5f;
            const float indicatorY = baseline - static_cast<float>(itemHeight) * 0.65f;
            const float leftX = centerX - backgroundWidth * 0.5f + 16.0f;
            const float rightX = centerX + backgroundWidth * 0.5f - 16.0f;
            const TextColor indicatorColor = toTextColor(menuData.style.selectedColor, indicatorAlpha);

            if (uiBatcher_) {
                uiBatcher_->AddTriangle(leftX, indicatorY - indicatorHalf,
                                        leftX + 12.0f, indicatorY,
                                        leftX, indicatorY + indicatorHalf,
                                        indicatorColor.r, indicatorColor.g, indicatorColor.b, indicatorColor.a);
                uiBatcher_->AddTriangle(rightX, indicatorY - indicatorHalf,
                                        rightX - 12.0f, indicatorY,
                                        rightX, indicatorY + indicatorHalf,
                                        indicatorColor.r, indicatorColor.g, indicatorColor.b, indicatorColor.a);
            }

            if (!item->shortcutHint.empty()) {
                const std::string hint = "[" + item->shortcutHint + "]";
                TextRenderer::RenderTextAligned(hint,
                                                static_cast<int>(centerX + backgroundWidth * 0.5f - 40.0f),
                                                static_cast<int>(baseline),
                                                TextAlign::Right,
                                                toTextColor(menuData.style.footerColor, 0.8f),
                                                FontSize::Small);
            }
        }
    }

    if (descriptionBaseline > 0.0f && selectedItem && !selectedItem->description.empty()) {
        const int wrapWidth = std::max(maxLineWidth - 60, 240);
        const int descriptionX = static_cast<int>(centerX) - wrapWidth / 2;
        const int descriptionTop = static_cast<int>(descriptionBaseline) - descriptionHeight;
        TextRenderer::RenderTextBlock(selectedItem->description,
                                      descriptionX,
                                      descriptionTop,
                                      wrapWidth,
                                      toTextColor(menuData.style.subtitleColor, 0.85f),
                                      descriptionFont,
                                      2);
    }

    if (footerBaseline > 0.0f && !menuData.footer.empty()) {
        TextRenderer::RenderTextAligned(menuData.footer,
                                        static_cast<int>(centerX),
                                        static_cast<int>(footerBaseline),
                                        TextAlign::Center,
                                        toTextColor(menuData.style.footerColor, 0.9f),
                                        footerFont);
    }

    // Flush batched UI if used
    if (uiBatcher_) {
        uiBatcher_->Flush();
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
#else
    (void)menuData;
#endif
#else
    (void)menuData;
#endif
}

#ifdef USE_SDL
void Viewport3D::EnsureSpaceshipHudTexture() {
    if (spaceshipHudTexture_ || spaceshipHudTextureFailed_) {
        return;
    }

    if (!sdlRenderer) {
        return;
    }

    SDL_Surface* surface = LoadSVGSurface("assets/ui/spaceship_hud.svg");
    if (!surface) {
        std::cerr << "Viewport3D: failed to load spaceship HUD SVG" << std::endl;
        spaceshipHudTextureFailed_ = true;
        return;
    }

    spaceshipHudTextureWidth_ = surface->w;
    spaceshipHudTextureHeight_ = surface->h;
    spaceshipHudTexture_ = compat_CreateTextureFromSurface(sdlRenderer, surface);
    compat_DestroySurface(surface);

    if (!spaceshipHudTexture_) {
        std::cerr << "Viewport3D: failed to create texture for spaceship HUD SVG: "
                  << SDL_GetError() << std::endl;
        spaceshipHudTextureFailed_ = true;
        spaceshipHudTextureWidth_ = 0;
        spaceshipHudTextureHeight_ = 0;
        return;
    }

    SDL_SetTextureBlendMode(spaceshipHudTexture_, SDL_BLENDMODE_BLEND);
}
#endif

void Viewport3D::EnsurePlayerHudTextureGL() {
    if (!IsUsingGLBackend()) {
        return;
    }

    if (playerHudTextureGL_ != 0 || playerHudTextureGLFailed_) {
        return;
    }
    SvgRasterizationOptions opts;
    opts.targetWidth = 1920;
    opts.targetHeight = 1080;
    opts.preserveAspectRatio = true;

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!LoadSvgToRgbaCached("assets/ui/player_hud.svg", pixels, width, height, opts)) {
        std::cerr << "Viewport3D: failed to load player HUD SVG" << std::endl;
        playerHudTextureGLFailed_ = true;
        return;
    }

    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0) {
        std::cerr << "Viewport3D: glGenTextures failed for player HUD" << std::endl;
        playerHudTextureGLFailed_ = true;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    playerHudTextureGL_ = textureId;
    playerHudTextureGLWidth_ = width;
    playerHudTextureGLHeight_ = height;
}

void Viewport3D::DrawHUD(const class Camera* camera,
                         double fps,
                         double playerX,
                         double playerY,
                         double playerZ,
                         const struct EnergyHUDTelemetry* energyTelemetry) {
    (void)playerZ; // only used in SDL path; silence unused param warning for GLFW-only builds
    if (debugLogging_) std::cout << "Viewport3D::DrawHUD() called" << std::endl;
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "Viewport3D::DrawHUD");
    }
#endif
    if (IsUsingGLFWBackend()) {
#ifdef USE_GLFW
    if (glfwWindow) {
            // GLFW OpenGL HUD drawing
            glfwMakeContextCurrent(glfwWindow);
            if (glfwGetCurrentContext() != glfwWindow) {
                return;
            }
            GLenum error = glGetError();
            if (debugLogging_ && error != GL_NO_ERROR) {
                std::cout << "OpenGL error before DrawHUD: " << error << " (" << DescribeGlError(error) << ")" << std::endl;
            }
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            gluOrtho2D(0, width, height, 0); // 0,0 top-left
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            HudAnchorRect energyAnchor{};
            HudAnchorRect telemetryAnchor{};
            HudAnchorRect statusAnchor{};
            float hudScale = 1.0f;
            bool haveHudTexture = false;

#if defined(USE_GLFW) || defined(USE_SDL)
            EnsurePlayerHudTextureGL();
            if (playerHudTextureGL_ != 0 && playerHudTextureGLWidth_ > 0 && playerHudTextureGLHeight_ > 0) {
                haveHudTexture = true;
                const float texW = static_cast<float>(playerHudTextureGLWidth_);
                const float texH = static_cast<float>(playerHudTextureGLHeight_);
                hudScale = std::min(static_cast<float>(width) / texW,
                                    static_cast<float>(height) / texH);
                const float destW = texW * hudScale;
                const float destH = texH * hudScale;
                const float offsetX = (static_cast<float>(width) - destW) * 0.5f;
                const float offsetY = (static_cast<float>(height) - destH) * 0.5f;

                DrawHudTextureOverlay(playerHudTextureGL_, offsetX, offsetY, destW, destH);

                energyAnchor = {offsetX + 80.0f * hudScale,
                                offsetY + 60.0f * hudScale,
                                460.0f * hudScale,
                                280.0f * hudScale,
                                true};
                telemetryAnchor = {offsetX + 1380.0f * hudScale,
                                   offsetY + 60.0f * hudScale,
                                   460.0f * hudScale,
                                   280.0f * hudScale,
                                   true};
                statusAnchor = {offsetX + 80.0f * hudScale,
                                offsetY + 860.0f * hudScale,
                                1760.0f * hudScale,
                                180.0f * hudScale,
                                true};
            }
#endif

            if (uiBatcher_) {
                uiBatcher_->Begin(width, height);
                if (!haveHudTexture) {
                    uiBatcher_->AddQuad(10, 10, 340, 110, 0.2f, 0.2f, 0.2f, 0.8f);
                    uiBatcher_->AddRectOutline(10, 10, 340, 120, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f);
                }
            }

            // Layout
            const float telemetryBaseX = haveHudTexture ? telemetryAnchor.x + 24.0f * hudScale : 18.0f;
            const float telemetryBaseY = haveHudTexture ? telemetryAnchor.y + 40.0f * hudScale : 25.0f;
            int x = static_cast<int>(std::lround(telemetryBaseX));
            int y = static_cast<int>(std::lround(telemetryBaseY));

            // Label "FPS:"
            TextRenderer::RenderText("FPS:", x, y, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            x += TextRenderer::MeasureText("FPS:", FontSize::Large) + 8;

            // FPS value (use TextRenderer instead of seven-seg)
            char fbuf[16];
            snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
            TextRenderer::RenderText(fbuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
            x += TextRenderer::MeasureText(fbuf, FontSize::Large) + 12;

            x += 12;
            // Zoom label "Z:"
            TextRenderer::RenderText("Z:", x, y, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            x += TextRenderer::MeasureText("Z:", FontSize::Large) + 8;

            // Zoom value (TextRenderer)
            char zbuf[32];
            if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom());
            else snprintf(zbuf, sizeof(zbuf), "1.0");
            TextRenderer::RenderText(zbuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
            x += TextRenderer::MeasureText(zbuf, FontSize::Large) + 12;

            int vsyncX = x + 12;
            int vsyncY = y;
            TextRenderer::RenderText("VSYNC", vsyncX, vsyncY, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            vsyncX += TextRenderer::MeasureText("VSYNC", FontSize::Large) + 12;
            const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
            TextRenderer::RenderText(vsValue, vsyncX, vsyncY, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
            vsyncX += TextRenderer::MeasureText(vsValue, FontSize::Large) + 12;
            TextRenderer::RenderText("CAP", vsyncX, vsyncY, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            vsyncX += TextRenderer::MeasureText("CAP", FontSize::Large) + 12;
            char capBuf[16];
            if (frameRateLimitHint_ <= 0.0) {
                snprintf(capBuf, sizeof(capBuf), "INF");
            } else {
                snprintf(capBuf, sizeof(capBuf), "%.0f", frameRateLimitHint_);
            }
            TextRenderer::RenderText(capBuf, vsyncX, vsyncY, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);

            // Second row - Position
            const int rowSpacing = haveHudTexture ? static_cast<int>(std::lround(46.0f * hudScale)) : 50;
            x = static_cast<int>(std::lround(haveHudTexture ? telemetryAnchor.x + 24.0f * hudScale : 18.0f));
            y += rowSpacing;
            // X label
            TextRenderer::RenderText("X", x, y, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            x += TextRenderer::MeasureText("X", FontSize::Large) + 8;
            
            // X value (TextRenderer)
            char xbuf[32];
            snprintf(xbuf, sizeof(xbuf), "%.1f", playerX);
            TextRenderer::RenderText(xbuf, x, y, TextColor(0.5f, 1.0f, 1.0f), FontSize::Large);
            x += TextRenderer::MeasureText(xbuf, FontSize::Large) + 12;

            x += 12;
            // Y label
            TextRenderer::RenderText("Y", x, y, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
            x += TextRenderer::MeasureText("Y", FontSize::Large) + 8;

            // Y value (TextRenderer)
            char ybuf[32];
            snprintf(ybuf, sizeof(ybuf), "%.1f", playerY);
            TextRenderer::RenderText(ybuf, x, y, TextColor(0.5f, 1.0f, 1.0f), FontSize::Large);
            x += TextRenderer::MeasureText(ybuf, FontSize::Large) + 12;

            // Energy management overlay
#if defined(USE_GLFW)
            if (energyTelemetry && energyTelemetry->valid) {
                RenderEnergyPanel(uiBatcher_.get(), *energyTelemetry, width, height,
                                  haveHudTexture ? &energyAnchor : nullptr);
            }
#endif

            // Reticle at screen center (always on for now)
            if (uiBatcher_) {
                DrawReticle2D(uiBatcher_.get(), width, height, haveHudTexture ? hudScale : 1.0f);
            }

            // Bottom status rail (health/energy/ammo + speed)
            {
                const double speed = SampleSpeed(playerX, playerY, playerZ);
                int ammoCur = -1, ammoMax = -1;
                if (energyTelemetry && energyTelemetry->valid) {
                    ammoCur = energyTelemetry->weaponAmmoCurrent;
                    ammoMax = energyTelemetry->weaponAmmoMax;
                }
                RenderPlayerStatusRail(uiBatcher_.get(), energyTelemetry, width, height, speed, ammoCur, ammoMax,
                                       haveHudTexture ? &statusAnchor : nullptr);
            }

            // 2D mini axes gizmo (HUD) — draws in screen space using quads
#ifndef NDEBUG
            if (uiBatcher_ && g_ShowMiniAxesGizmo) {
                const float originX = g_MiniGizmoMargin;
                const float originY = static_cast<float>(height) - g_MiniGizmoMargin; // bottom-left corner origin
                const float L = g_MiniGizmoSize;
                const float T = std::max(1.0f, g_MiniGizmoThickness);
                // X axis (red) to the right
                uiBatcher_->AddQuad(originX, originY, L, T, 1.0f, 0.0f, 0.0f, 1.0f);
                // Y axis (green) upwards (negative screen Y)
                uiBatcher_->AddQuad(originX, originY - L, T, L, 0.0f, 1.0f, 0.0f, 1.0f);
                // Z axis (blue) marker as a small square at origin
                const float ZS = std::max(T * 1.6f, 4.0f);
                uiBatcher_->AddQuad(originX - ZS * 0.5f, originY - ZS * 0.5f, ZS, ZS, 0.0f, 0.6f, 1.0f, 1.0f);
                // Labels via TextRenderer
                TextRenderer::RenderText("X", static_cast<int>(originX + L + 6.0f), static_cast<int>(originY - 6.0f), TextColor(1.0f, 0.6f, 0.6f), FontSize::Medium);
                TextRenderer::RenderText("Y", static_cast<int>(originX - 10.0f), static_cast<int>(originY - L - 12.0f), TextColor(0.6f, 1.0f, 0.6f), FontSize::Medium);
                TextRenderer::RenderText("Z", static_cast<int>(originX + 8.0f), static_cast<int>(originY + 6.0f), TextColor(0.6f, 0.8f, 1.0f), FontSize::Medium);
            }
#endif

            // Flush batched UI rendering
            if (uiBatcher_) {
                // Optional: small HUD hint for debug hotkeys
#ifndef NDEBUG
                if (showHudHints_) {
                    const char* hint = "F8: World Axes   F9: Mini Gizmo";
                    const float hx = 14.0f;
                    const float hy = 140.0f;
                    const int hintW = TextRenderer::MeasureText(hint, FontSize::Medium);
                    // subtle background sized to text
                    uiBatcher_->AddQuad(hx - 6.0f, hy - 10.0f, static_cast<float>(hintW + 12), 20.0f, 0.0f, 0.0f, 0.0f, 0.35f);
                    TextRenderer::RenderText(hint, static_cast<int>(hx), static_cast<int>(hy), TextColor(0.9f, 0.9f, 0.9f), FontSize::Medium);
                }
#endif
                uiBatcher_->Flush();
            }

            // Restore
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }
#endif
        return;
    }
    if (!IsUsingSDLBackend()) return;
#ifdef USE_SDL
    if (IsUsingSDLGL()) {
        // OpenGL HUD drawing
        if (SDL_GL_MakeCurrent(sdlWindow, sdlGLContext) != 0) {
            return;
        }
        GLenum error = glGetError();
        if (debugLogging_ && error != GL_NO_ERROR) {
            std::cout << "OpenGL error before DrawHUD: " << error << " (" << DescribeGlError(error) << ")" << std::endl;
        }
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, width, height, 0); // 0,0 top-left
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Begin batched UI rendering and draw background/border
        if (uiBatcher_) {
            uiBatcher_->Begin(width, height);
            uiBatcher_->AddQuad(8, 8, 380, 180, 0.0f, 0.0f, 0.0f, 0.7f);
            uiBatcher_->AddRectOutline(8, 8, 380, 180, 1.0f, 1.0f, 1.0f, 1.0f, 0.7f);
        }

        // Layout
        int x = 18, y = 18;

        // Label "Z:" via TextRenderer
        TextRenderer::RenderText("Z:", x, y, TextColor(0.5f, 0.5f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText("Z:", FontSize::Large) + 12;

        // Zoom (TextRenderer)
        char zbuf[32];
        if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); else snprintf(zbuf, sizeof(zbuf), "0.0");
        TextRenderer::RenderText(zbuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText(zbuf, FontSize::Large) + 12;

        x += 18;
    // FPS label via TextRenderer
    TextRenderer::RenderText("FPS:", x, y, TextColor(0.5f, 0.5f, 0.5f), FontSize::Large);
    x += TextRenderer::MeasureText("FPS:", FontSize::Large) + 12;
        char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
        TextRenderer::RenderText(fbuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText(fbuf, FontSize::Large) + 12;

        int infoX = x + 12;
        int infoY = y;
        TextRenderer::RenderText("VSYNC", infoX, infoY, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
        infoX += TextRenderer::MeasureText("VSYNC", FontSize::Large) + 12;
        const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
        TextRenderer::RenderText(vsValue, infoX, infoY, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        infoX += TextRenderer::MeasureText(vsValue, FontSize::Large) + 12;
        TextRenderer::RenderText("CAP", infoX, infoY, TextColor(0.7f, 0.7f, 0.7f), FontSize::Large);
        infoX += TextRenderer::MeasureText("CAP", FontSize::Large) + 12;
        char capBuf[16];
        if (frameRateLimitHint_ <= 0.0) {
            snprintf(capBuf, sizeof(capBuf), "INF");
        } else {
            snprintf(capBuf, sizeof(capBuf), "%.0f", frameRateLimitHint_);
        }
        TextRenderer::RenderText(capBuf, infoX, infoY, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);

    x += 18;
    // Player X label via TextRenderer
    TextRenderer::RenderText("X", x, y, TextColor(0.5f, 0.5f, 0.5f), FontSize::Large);
    x += TextRenderer::MeasureText("X", FontSize::Large) + 12;
        char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
        TextRenderer::RenderText(xbuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText(xbuf, FontSize::Large) + 12;

    // Next row
    x = 18; y += 60;
    // Player Y label via TextRenderer
    TextRenderer::RenderText("Y", x, y, TextColor(0.5f, 0.5f, 0.5f), FontSize::Large);
    x += TextRenderer::MeasureText("Y", FontSize::Large) + 12;
        char ybuf[32]; snprintf(ybuf, sizeof(ybuf), "%.2f", playerY);
        TextRenderer::RenderText(ybuf, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText(ybuf, FontSize::Large) + 12;

    x += 18;
    // Player Z label via TextRenderer
    TextRenderer::RenderText("Z", x, y, TextColor(0.5f, 0.5f, 0.5f), FontSize::Large);
    x += TextRenderer::MeasureText("Z", FontSize::Large) + 12;
        char zbuf2[32]; snprintf(zbuf2, sizeof(zbuf2), "%.2f", playerZ);
        TextRenderer::RenderText(zbuf2, x, y, TextColor(1.0f, 0.9f, 0.5f), FontSize::Large);
        x += TextRenderer::MeasureText(zbuf2, FontSize::Large) + 12;

        // Reticle
        if (uiBatcher_) {
            DrawReticle2D(uiBatcher_.get(), width, height, 1.0f);
        }

        // Bottom status rail
        {
            const double speed = SampleSpeed(playerX, playerY, playerZ);
            int ammoCur = -1, ammoMax = -1;
            if (energyTelemetry && energyTelemetry->valid) {
                ammoCur = energyTelemetry->weaponAmmoCurrent;
                ammoMax = energyTelemetry->weaponAmmoMax;
            }
                RenderPlayerStatusRail(uiBatcher_.get(), energyTelemetry, width, height, speed, ammoCur, ammoMax, nullptr);
        }

    // 2D mini axes gizmo (HUD) — draws in screen space using quads
#ifndef NDEBUG
    if (uiBatcher_ && g_ShowMiniAxesGizmo) {
        const float originX = g_MiniGizmoMargin;
        const float originY = static_cast<float>(height) - g_MiniGizmoMargin; // bottom-left corner origin
        const float L = g_MiniGizmoSize;
        const float T = std::max(1.0f, g_MiniGizmoThickness);
        // X axis (red) to the right
        uiBatcher_->AddQuad(originX, originY, L, T, 1.0f, 0.0f, 0.0f, 1.0f);
        // Y axis (green) upwards (negative screen Y)
        uiBatcher_->AddQuad(originX, originY - L, T, L, 0.0f, 1.0f, 0.0f, 1.0f);
        // Z axis (blue) marker as a small square at origin
        const float ZS = std::max(T * 1.6f, 4.0f);
        uiBatcher_->AddQuad(originX - ZS * 0.5f, originY - ZS * 0.5f, ZS, ZS, 0.0f, 0.6f, 1.0f, 1.0f);
        // Labels via TextRenderer
        TextRenderer::RenderText("X", static_cast<int>(originX + L + 6.0f), static_cast<int>(originY - 6.0f), TextColor(1.0f, 0.6f, 0.6f), FontSize::Medium);
        TextRenderer::RenderText("Y", static_cast<int>(originX - 10.0f), static_cast<int>(originY - L - 12.0f), TextColor(0.6f, 1.0f, 0.6f), FontSize::Medium);
        TextRenderer::RenderText("Z", static_cast<int>(originX + 8.0f), static_cast<int>(originY + 6.0f), TextColor(0.6f, 0.8f, 1.0f), FontSize::Medium);
    }
#endif

    // Flush batched UI rendering
        if (uiBatcher_) {
            // Optional: small HUD hint for debug hotkeys
#ifndef NDEBUG
            if (showHudHints_) {
                const char* hint = "F8: World Axes   F9: Mini Gizmo";
                const float hx = 14.0f;
                const float hy = 200.0f;
                const int hintW = TextRenderer::MeasureText(hint, FontSize::Medium);
                uiBatcher_->AddQuad(hx - 6.0f, hy - 10.0f, static_cast<float>(hintW + 12), 20.0f, 0.0f, 0.0f, 0.0f, 0.35f);
                TextRenderer::RenderText(hint, static_cast<int>(hx), static_cast<int>(hy), TextColor(0.9f, 0.9f, 0.9f), FontSize::Medium);
            }
#endif
            uiBatcher_->Flush();
        }

        // Restore
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    } else if (IsUsingSDLRenderer()) {
        // SDL renderer HUD drawing
        if (!sdlRenderer) return;

        bool drewSpaceshipHud = false;
#ifdef USE_SDL
        EnsureSpaceshipHudTexture();
        if (spaceshipHudTexture_) {
            drewSpaceshipHud = true;
            const float scaleX = static_cast<float>(width) /
                                 static_cast<float>(std::max(1, spaceshipHudTextureWidth_));
            const float scaleY = static_cast<float>(height) /
                                 static_cast<float>(std::max(1, spaceshipHudTextureHeight_));
            const float scale = std::min(scaleX, scaleY);
            const int destW = static_cast<int>(spaceshipHudTextureWidth_ * scale);
            const int destH = static_cast<int>(spaceshipHudTextureHeight_ * scale);
            SDL_Rect dest{
                (width - destW) / 2,
                (height - destH) / 2,
                destW,
                destH
            };
            compat_RenderCopy(sdlRenderer, spaceshipHudTexture_, nullptr, &dest);
        }
#endif

        // Draw semi-transparent background box with border for telemetry readout
        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
        const Uint8 backgroundAlpha = drewSpaceshipHud ? 140 : 180;
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, backgroundAlpha);
        SDL_Rect bg{8, 8, 380, 180};
        compat_RenderFillRect(sdlRenderer, &bg);
        // white border
        SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, backgroundAlpha);
        compat_RenderDrawRect(sdlRenderer, &bg);

        int x = 18, y = 18;
        const int scaleLabel = 4;
        const int scaleValue = 4;
        SDL_Color labelColor{128, 128, 128, 255};
        SDL_Color valueColor{255, 230, 120, 255};

        // Z: label and value
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, labelColor, scaleLabel, "Z:") + 12;
        char zbuf[32];
        if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom());
        else snprintf(zbuf, sizeof(zbuf), "0.0");
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, valueColor, scaleValue, zbuf) + 18;

        // FPS: label and value
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, labelColor, scaleLabel, "FPS:") + 12;
        char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, valueColor, scaleValue, fbuf) + 18;

        // X: value
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, labelColor, scaleLabel, "X") + 12;
        char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, valueColor, scaleValue, xbuf);

        // Next row for Y and Z
        x = 18; y += 60;
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, labelColor, scaleLabel, "Y") + 12;
        char ybuf[32]; snprintf(ybuf, sizeof(ybuf), "%.2f", playerY);
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, valueColor, scaleValue, ybuf) + 18;

        x += SDLMiniFont_RenderText(sdlRenderer, x, y, labelColor, scaleLabel, "Z") + 12;
        char zbuf2[32]; snprintf(zbuf2, sizeof(zbuf2), "%.2f", playerZ);
        x += SDLMiniFont_RenderText(sdlRenderer, x, y, valueColor, scaleValue, zbuf2);
    }
#endif
    if (IsUsingSDLGL()) {
        GLenum error = glGetError();
        if (debugLogging_ && error != GL_NO_ERROR) {
            std::cout << "OpenGL error after DrawHUD: " << error << " (" << DescribeGlError(error) << ")" << std::endl;
        }
    }
#ifndef NDEBUG
    if (glDebugMessageCallback != nullptr) {
        glPopDebugGroup();
    }
#endif
}

bool Viewport3D::CaptureToBMP(const char* path) {
#ifdef USE_SDL
    if (!IsUsingSDLRenderer() || !sdlRenderer) return false;
    // Read pixels from current render target
    int w = width, h = height;
    int pitch = w * 3;
    std::vector<unsigned char> pixels(pitch * h);
    // Ensure render target is the default
    if (compat_RenderReadPixels(sdlRenderer, NULL, SDL_PIXELFORMAT_RGB24, pixels.data(), pitch) != 0) {
        std::cerr << "Viewport3D::CaptureToBMP: SDL_RenderReadPixels failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // BMP 24-bit header
    int rowBytes = ((w * 3 + 3) / 4) * 4;
    int imgSize = rowBytes * h;
    unsigned char header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    header[10] = 54;
    header[14] = 40;
    auto put_le32 = [](unsigned char* dst, int off, uint32_t value) {
        dst[off + 0] = static_cast<unsigned char>(value & 0xFF);
        dst[off + 1] = static_cast<unsigned char>((value >> 8) & 0xFF);
        dst[off + 2] = static_cast<unsigned char>((value >> 16) & 0xFF);
        dst[off + 3] = static_cast<unsigned char>((value >> 24) & 0xFF);
    };

    put_le32(header, 2, static_cast<uint32_t>(54 + imgSize));
    put_le32(header, 18, static_cast<uint32_t>(w));
    put_le32(header, 22, static_cast<uint32_t>(h));
    header[26] = 1;
    header[27] = 0;
    header[28] = 24;
    header[29] = 0;
    put_le32(header, 34, static_cast<uint32_t>(imgSize));
    put_le32(header, 38, 3780); // ~96 DPI
    put_le32(header, 42, 3780);

    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite(header, 1, 54, f);
    // BMP stores rows bottom-up
    std::vector<unsigned char> row(rowBytes);
    for (int y = h - 1; y >= 0; --y) {
        unsigned char* src = pixels.data() + y * pitch;
        int idx = 0;
        for (int x = 0; x < w; ++x) {
            // source is RGB; BMP needs BGR
            row[idx++] = src[x*3 + 2];
            row[idx++] = src[x*3 + 1];
            row[idx++] = src[x*3 + 0];
        }
        while (idx < rowBytes) row[idx++] = 0;
        fwrite(row.data(), 1, rowBytes, f);
    }
    fclose(f);
    return true;
#else
    (void)path;
    return false;
#endif
}

void Viewport3D::DrawHUD(const class Camera* camera,
                         double fps,
                         double playerX,
                         double playerY,
                         double playerZ,
                         bool,
                         const class ShipAssemblyResult*,
                         const EnergyHUDTelemetry* energyTelemetry) {
    // Call the existing DrawHUD
    DrawHUD(camera, fps, playerX, playerY, playerZ, energyTelemetry);
}

void Viewport3D::RenderParticles(const class Camera* camera, const class VisualFeedbackSystem* visualFeedback) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!visualFeedback) {
        return;
    }

    if (!IsUsingGLBackend()) {
        // No active OpenGL context; nothing to render.
        return;
    }

    if (!particleRenderer_) {
        particleRenderer_.reset(new ParticleRenderer());
        if (!particleRenderer_->Init(shaderManager_.get())) {
            std::cerr << "Viewport3D: Failed to initialize ParticleRenderer" << std::endl;
            particleRenderer_.reset();
            return;
        }
    }

    particleRenderer_->Render(visualFeedback->GetParticles(), camera);
#else
    (void)camera;
    (void)visualFeedback;
#endif
}
