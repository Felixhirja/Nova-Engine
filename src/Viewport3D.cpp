#include "Viewport3D.h"
#include "TextRenderer.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <memory>
#include <algorithm>
#include <utility>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(USE_GLFW) || defined(USE_SDL)
#include <glad/glad.h>
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
#include "ResourceManager.h"
#include "SVGSurfaceLoader.h"
#include "Camera.h"
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#include "Camera.h"
#endif
#include "VisualFeedbackSystem.h"
#if defined(USE_GLFW) || defined(USE_SDL)
#include "graphics/ParticleRenderer.h"
#endif

struct Viewport3D::PrimitiveBuffers {
    PrimitiveBuffers() = default;
    GLuint playerVBO = 0;
    GLsizei playerVertexCount = 0;
    GLuint cubeVBO = 0;
    GLsizei cubeVertexCount = 0;
};

static const uint8_t tinyFont[][5] = {
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

static const uint8_t glyphV[5] = {0x11, 0x11, 0x0A, 0x0A, 0x04};
static const uint8_t glyphY[5] = {0x11, 0x0A, 0x04, 0x04, 0x04};
static const uint8_t glyphN[5] = {0x11, 0x19, 0x15, 0x13, 0x11};
static const uint8_t glyphC[5] = {0x0E, 0x10, 0x10, 0x10, 0x0E};
static const uint8_t glyphO[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t glyphF[5] = {0x1F, 0x10, 0x1E, 0x10, 0x10};
static const uint8_t glyphA[5] = {0x0E, 0x11, 0x1F, 0x11, 0x11};
static const uint8_t glyphP[5] = {0x1E, 0x11, 0x1E, 0x10, 0x10};
static const uint8_t glyphT[5] = {0x1F, 0x04, 0x04, 0x04, 0x04};
static const uint8_t glyphG[5] = {0x0E, 0x10, 0x17, 0x11, 0x0E};
static const uint8_t glyphLtrS[5] = {0x1F, 0x10, 0x1F, 0x01, 0x1F};

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

} // namespace

#if defined(USE_GLFW) || defined(USE_SDL)
// Legacy immediate-mode tiny font renderer removed; UI text pixels are batched via UIBatcher.
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

void RenderEnergyPanel(UIBatcher* batch, const EnergyHUDTelemetry& telemetry, int screenWidth, int screenHeight) {
    (void)screenHeight; // currently unused
    const float panelWidth = 420.0f;
    const float panelHeight = 300.0f;
    const float margin = 18.0f;
    const float panelX = static_cast<float>(screenWidth) - panelWidth - margin;
    const float panelY = margin;

    DrawQuad2D(batch, panelX, panelY, panelWidth, panelHeight, MakeColor(0.02f, 0.02f, 0.04f, 0.82f));
    DrawBorder2D(batch, panelX, panelY, panelWidth, panelHeight, MakeColor(0.45f, 0.55f, 0.75f, 0.8f));

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
                                 FontSize::Medium);

        Color4 statusColor = StatusColor(percent, rechargingHighlight);
    DrawFillBar(batch, bx + 12.0f,
                    by + 34.0f,
                    boxWidth - 24.0f,
                    14.0f,
                    percent,
                    statusColor);

        TextRenderer::RenderTextF(static_cast<int>(bx + boxWidth - 60.0f),
                                  static_cast<int>(by + 28.0f),
                                  TextColor::White(),
                                  FontSize::Small,
                                  "%3.0f%%",
                                  std::clamp(percent * 100.0, 0.0, 999.0));

        if (requirement > 0.0) {
            TextRenderer::RenderTextF(static_cast<int>(bx + 12.0f),
                                      static_cast<int>(by + 56.0f),
                                      TextColor::Gray(0.85f),
                                      FontSize::Small,
                                      "%0.1f/%0.1f MW",
                                      delivered,
                                      requirement);
        }

        if (valueMax > 0.0 && valueUnits) {
            TextRenderer::RenderTextF(static_cast<int>(bx + 12.0f),
                                      static_cast<int>(by + 72.0f),
                                      TextColor::Gray(0.9f),
                                      FontSize::Small,
                                      "%0.0f/%0.0f %s",
                                      value,
                                      valueMax,
                                      valueUnits);
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
{
}

Viewport3D::~Viewport3D() {}

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

void Viewport3D::DrawCubePrimitive(float r, float g, float b) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!IsUsingGLBackend()) {
        return;
    }
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
    }
    if (debugLogging_) {
        std::cout << "Viewport3D: render backend set to " << RenderBackendToString(backend_) << std::endl;
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
        double yaw = camera->yaw();
        double offsetX = 5.0 * sin(yaw);
        double offsetY = -5.0 * cos(yaw);
        gluLookAt(camera->x() + offsetX, camera->y() + offsetY, camera->z() + 5.0,
                  playerX, playerY, playerZ,
                  0.0, 0.0, 1.0);
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

void Viewport3D::DrawTinyChar2D(float x, float y, char c, float scale, float r, float g, float b) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (uiBatcher_) {
        auto addPixel = [&](float px, float py) {
            uiBatcher_->AddQuad(x + px * scale, y + py * scale, scale, scale, r, g, b, 1.0f);
        };

        auto drawGlyph = [&](const uint8_t* glyph) {
            for (int col = 0; col < 5; ++col) {
                uint8_t bits = glyph[col];
                for (int row = 0; row < 5; ++row) {
                    if (bits & (1 << (4 - row))) {
                        addPixel(static_cast<float>(col), static_cast<float>(row));
                    }
                }
            }
        };

        if (c >= '0' && c <= '9') {
            drawGlyph(tinyFont[c - '0']);
        } else if (c == '-') {
            addPixel(0.0f, 2.0f);
            addPixel(1.0f, 2.0f);
            addPixel(2.0f, 2.0f);
            addPixel(3.0f, 2.0f);
            addPixel(4.0f, 2.0f);
        } else if (c == '.') {
            addPixel(4.0f, 4.0f);
        } else if (c == 'V') {
            drawGlyph(glyphV);
        } else if (c == 'Y') {
            drawGlyph(glyphY);
        } else if (c == 'N') {
            drawGlyph(glyphN);
        } else if (c == 'C') {
            drawGlyph(glyphC);
        } else if (c == 'O') {
            drawGlyph(glyphO);
        } else if (c == 'F') {
            drawGlyph(glyphF);
        } else if (c == 'A') {
            drawGlyph(glyphA);
        } else if (c == 'P') {
            drawGlyph(glyphP);
        } else if (c == 'T') {
            drawGlyph(glyphT);
        } else if (c == 'G') {
            drawGlyph(glyphG);
        } else if (c == 'S') {
            drawGlyph(glyphLtrS);
        }
    } else {
        // No UIBatcher available; skip tiny char rendering.
    }
#else
    (void)x; (void)y; (void)c; (void)scale; (void)r; (void)g; (void)b;
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

    struct GLContextAttempt {
        int major;
        int minor;
        bool coreProfile;
        bool forwardCompatible;
        const char* description;
    };

    const std::vector<GLContextAttempt> contextAttempts = {
        {3, 3, false, false, "OpenGL 3.3 Compatibility"},
        {2, 1, false, false, "OpenGL 2.1 Compatibility"},
    };

    // Get primary monitor for fullscreen
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Update width and height to match fullscreen resolution
    width = mode->width;
    height = mode->height;

    // Create windowed window for development (easier to close when testing)
    // Comment out the fullscreen lines below and uncomment windowed lines for fullscreen
    /*
    // Get primary monitor for fullscreen
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Update width and height to match fullscreen resolution
    width = mode->width;
    height = mode->height;

    const GLContextAttempt* chosenAttempt = nullptr;
    for (const auto& attempt : contextAttempts) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, attempt.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, attempt.minor);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable resizing for fullscreen
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        if (attempt.coreProfile) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else if (attempt.major >= 3) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        } else {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        }
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, attempt.forwardCompatible ? GL_TRUE : GL_FALSE);

        if (debugLogging_) std::cout << "Viewport3D: Trying " << attempt.description << " context" << std::endl;
        glfwWindow = glfwCreateWindow(width, height, "Nova Engine", primaryMonitor, nullptr);
        if (glfwWindow) {
            chosenAttempt = &attempt;
            break;
        }

        std::cerr << "Viewport3D: GLFW window creation failed for " << attempt.description << std::endl;
    }
    */

    // Windowed mode for development
    // Set reasonable default window size (don't use fullscreen resolution for windowed mode)
    width = 1280;
    height = 720;
    const GLContextAttempt* chosenAttempt = nullptr;
    for (const auto& attempt : contextAttempts) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, attempt.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, attempt.minor);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE); // Explicitly request window to be visible
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE); // Request focus
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        if (attempt.coreProfile) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else if (attempt.major >= 3) {
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
    {
        std::ofstream f("glfw_diag.log", std::ios::app);
        if (f) f << "GLAD init succeeded; creating UIBatcher" << std::endl;
    }
    // Initialize UIBatcher after GLAD setup
    uiBatcher_ = std::make_unique<UIBatcher>();
    if (!uiBatcher_->Init()) {
        if (debugLogging_) std::cerr << "Viewport3D: UIBatcher::Init failed (GLFW path)" << std::endl;
        uiBatcher_.reset();
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
                        // Initialize UIBatcher after GLAD setup
                        uiBatcher_ = std::make_unique<UIBatcher>();
                        if (!uiBatcher_->Init()) {
                            if (debugLogging_) std::cerr << "Viewport3D: UIBatcher::Init failed (SDL_GL path)" << std::endl;
                            uiBatcher_.reset();
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

void Viewport3D::Render(const class Camera* camera, double playerX, double playerY, double playerZ, bool targetLocked) {
    if (debugLogging_) std::cout << "Viewport3D::Render() called with camera=" << (camera ? "valid" : "null") << std::endl;
    EnsureLayoutConfiguration();
    if (debugLogging_) std::cout << "Viewport3D::Render() - after EnsureLayoutConfiguration()" << std::endl;
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
}

void Viewport3D::Clear() {
    if (debugLogging_) std::cout << "Viewport3D::Clear() called" << std::endl;
    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        if (IsUsingSDLGL()) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            glViewport(0, 0, width, height);
            // Ensure default framebuffer is bound before clearing
#ifdef GL_FRAMEBUFFER
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL error in Clear(): " << err << " (GL_INVALID_OPERATION=" << GL_INVALID_OPERATION << ")" << std::endl;
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
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL error in Clear(): " << err << " (GL_INVALID_OPERATION=" << GL_INVALID_OPERATION << ")" << std::endl;
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

void Viewport3D::DrawPlayer(double x, double y, double z) {
    if (debugLogging_) std::cout << "Viewport3D::DrawPlayer() called at (" << x << ", " << y << ", " << z << ")" << std::endl;
    if (debugLogging_) std::cout << "Viewport3D::DrawPlayer() - backend=" << RenderBackendToString(backend_) << std::endl;
    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        if (IsUsingSDLGL()) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            glPushMatrix();
            glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
            DrawPlayerPatchPrimitive();
            glPopMatrix();
        } else {
            int px = static_cast<int>(((x + 5.0) / 10.0) * width);
            int py = height / 2;
            // Draw smaller, more visible player patch
            SDL_Rect mainRect{px - 6, py - 6, 12, 12};
            SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, 255); // Bright yellow
            compat_RenderFillRect(sdlRenderer, &mainRect);

            // Add red border for visibility
            SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 255);
            compat_RenderDrawRect(sdlRenderer, &mainRect);

            // Add blue center dot
            SDL_Rect centerDot{px - 2, py - 2, 4, 4};
            SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 255, 255);
            compat_RenderFillRect(sdlRenderer, &centerDot);
        }
#else
        (void)x; (void)y; (void)z;
#endif
    }
#ifdef USE_GLFW
    else if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        glPushMatrix();
        glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
        DrawPlayerPatchPrimitive();
        glPopMatrix();
    }
#endif
    else {
        if (debugLogging_) std::cout << "Drawing ASCII fallback for player at " << x << std::endl;
        const int widthChars = 40;
        double clamped = std::min(5.0, std::max(-5.0, x));
        int pos = static_cast<int>((clamped + 5.0) / 10.0 * (widthChars - 1));
        std::string line(widthChars, '-');
        line[pos] = 'P';
        std::cout << line << std::endl;
    }
}

void Viewport3D::DrawEntity(const Transform &t) {
    // Very simple: use the entity's x,y,z coordinates and draw like DrawPlayer
    DrawPlayer(t.x, t.y, t.z);
}

void Viewport3D::DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, int currentFrame) {
    // Forward to camera-aware overload (camera=nullptr)
    DrawEntity(t, textureHandle, resourceManager, nullptr, currentFrame);
}

void Viewport3D::DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, const class Camera* camera, int currentFrame) {
#if !defined(USE_SDL)
    (void)textureHandle;
    (void)resourceManager;
    (void)camera;
    (void)currentFrame;
#endif
    if (IsUsingSDLBackend()) {
#ifdef USE_SDL
        int px, py;
        if (camera) {
            camera->WorldToScreen(t.x, t.y, t.z, width, height, px, py);
        } else {
            px = static_cast<int>(((t.x + 5.0) / 10.0) * width);
            py = height / 2;
        }
        int w = 16, h = 16;
    SDL_Rect dst{px - w/2, py - h/2, w, h};

        // Try texture path first
        if (textureHandle != 0 && resourceManager) {
            void* texRaw = resourceManager->GetTexture(static_cast<void*>(sdlRenderer), textureHandle);
            if (texRaw) {
                SDL_Texture* tex = static_cast<SDL_Texture*>(texRaw);
                // If sprite info available, compute source rect
                ResourceManager::SpriteInfo info;
                SDL_Rect srcRect;
                bool haveSrc = false;
                if (resourceManager->GetSpriteInfo(textureHandle, info) && info.frameW > 0 && info.frameH > 0) {
                    int frameCount = info.sheetW > 0 ? std::max(1, info.sheetW / info.frameW) : 1;
                    int frame = frameCount > 0 ? currentFrame % frameCount : 0;
                    if (frame < 0) {
                        frame += frameCount;
                    }
                    srcRect.x = frame * info.frameW;
                    srcRect.y = 0;
                    srcRect.w = info.frameW;
                    srcRect.h = info.frameH;
                    haveSrc = true;
                }
                compat_RenderCopy(sdlRenderer, tex, haveSrc ? &srcRect : nullptr, &dst);
                return;
            }
        }

        // Fallback: draw an orange rectangle
    SDL_SetRenderDrawColor(sdlRenderer, 255, 128, 0, 255);
    compat_RenderFillRect(sdlRenderer, &dst);
#endif
#ifdef USE_GLFW
    } else if (IsUsingGLFWBackend() && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        glPushMatrix();
        glTranslatef((GLfloat)t.x, (GLfloat)t.y, (GLfloat)t.z);
        DrawCubePrimitive(1.0f, 0.5f, 0.0f);
        glPopMatrix();
#endif
    } else {
        DrawPlayer(t.x, t.y, t.z);
    }
}

void Viewport3D::Resize(int w, int h) {
    width = w; height = h;
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

        // Disable depth test so axes draw on top
        glDisable(GL_DEPTH_TEST);
        // Draw coordinate system axes using the line batcher
        // Use current perspective projection instead of orthographic
        const float axisLength = 10.0f; // Longer axes for better visibility
        EnsureLineBatcher3D();
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(3.0f);
            // X axis (red)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
            // Y axis (green)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            // Z axis (blue)
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f, 1.0f, 1.0f);
            lineBatcher3D_->Flush();
        }

        // Draw axis labels using TextRenderer so we don't rely on raw GLUT calls here
        TextRenderer::RenderText3D("X",
                                   axisLength + 0.5f,
                                   0.0f,
                                   0.0f,
                                   TextColor::White(),
                                   FontSize::Medium);
        TextRenderer::RenderText3D("Y",
                                   0.0f,
                                   axisLength + 0.5f,
                                   0.0f,
                                   TextColor::White(),
                                   FontSize::Medium);
        TextRenderer::RenderText3D("Z",
                                   0.0f,
                                   0.0f,
                                   axisLength + 0.5f,
                                   TextColor::White(),
                                   FontSize::Medium);

    glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);

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

void Viewport3D::DrawCameraVisual(const class Camera* camera, double playerX, double playerY, double playerZ, bool targetLocked) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!camera) return;

    auto drawCameraDebug = [&]() {
        glDisable(GL_DEPTH_TEST); // Draw on top
        glPushMatrix();
        // Position the camera marker based on target lock state
        double camYaw = camera->yaw();
        double camCosYaw = cos(camYaw);
        double camSinYaw = sin(camYaw);
        
        double markerX, markerY, markerZ;
        
        if (targetLocked) {
            // When target locked: position 3 units in front along camera's forward direction
            double camPitch = camera->pitch();
            double camCosPitch = cos(camPitch);
            double camSinPitch = sin(camPitch);
            double forwardX = camCosYaw * camCosPitch;
            double forwardY = camSinYaw * camCosPitch;
            double forwardZ = camSinPitch;
            
            markerX = playerX + forwardX * 3.0;
            markerY = playerY + forwardY * 3.0;
            markerZ = playerZ + forwardZ * 3.0;
        } else {
            // When not target locked: position 5 units behind player along camera's forward direction
            double camPitch = camera->pitch();
            double camCosPitch = cos(camPitch);
            double camSinPitch = sin(camPitch);
            double forwardX = camCosYaw * camCosPitch;
            double forwardY = camSinYaw * camCosPitch;
            double forwardZ = camSinPitch;
            
            markerX = playerX - forwardX * 5.0;
            markerY = playerY - forwardY * 5.0;
            markerZ = playerZ - forwardZ * 5.0;
        }
        
        glTranslatef(markerX, markerY, markerZ);
        
        // Rotate to match camera orientation
        glRotatef(-camera->pitch() * 180.0f / 3.141592653589793f, 1.0f, 0.0f, 0.0f);
        glRotatef(-camera->yaw() * 180.0f / 3.141592653589793f, 0.0f, 1.0f, 0.0f);

        // Draw a much better camera visual - a small camera icon
        glLineWidth(3.0f);

        // Calculate forward direction for lens
        double pitch = camera->pitch();
        double cosPitch = cos(pitch);
        double sinPitch = sin(pitch);
        double forwardX = camCosYaw * cosPitch;
        double forwardY = camSinYaw * cosPitch;
        double forwardZ = sinPitch;

        struct Vec3 {
            double x;
            double y;
            double z;
        };

        auto cross = [](const Vec3& a, const Vec3& b) {
            return Vec3{a.y * b.z - a.z * b.y,
                        a.z * b.x - a.x * b.z,
                        a.x * b.y - a.y * b.x};
        };

        auto normalize = [](const Vec3& v) {
            double len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (len < 1e-6) {
                return Vec3{0.0, 0.0, 0.0};
            }
            return Vec3{v.x / len, v.y / len, v.z / len};
        };

        Vec3 forward = normalize(Vec3{forwardX, forwardY, forwardZ});
        Vec3 worldUp{0.0, 0.0, 1.0};
        Vec3 right = cross(forward, worldUp);
        if (std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z) < 1e-5) {
            right = Vec3{1.0, 0.0, 0.0};
        } else {
            right = normalize(right);
        }
        Vec3 up = cross(right, forward);
        up = normalize(up);
        if (std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z) < 1e-5) {
            up = worldUp;
        }

        // Camera body (rectangular prism) - draw edges with line batcher (no immediate mode)
        EnsureLineBatcher3D();
        if (lineBatcher3D_) {
            const float br = 0.8f, bg = 0.8f, bb = 0.8f;
            // 8 corners of the prism
            const float x0 = -0.4f, x1 = 0.4f;
            const float y0 = -0.2f, y1 = 0.2f;
            const float zf = 0.1f, zb = -0.3f;
            auto addEdge = [&](float ax, float ay, float az, float bx, float by, float bz) {
                lineBatcher3D_->AddLine(ax, ay, az, bx, by, bz, br, bg, bb);
            };
            // Front rectangle
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(2.0f);
            addEdge(x0, y0, zf, x1, y0, zf);
            addEdge(x1, y0, zf, x1, y1, zf);
            addEdge(x1, y1, zf, x0, y1, zf);
            addEdge(x0, y1, zf, x0, y0, zf);
            // Back rectangle
            addEdge(x0, y0, zb, x1, y0, zb);
            addEdge(x1, y0, zb, x1, y1, zb);
            addEdge(x1, y1, zb, x0, y1, zb);
            addEdge(x0, y1, zb, x0, y0, zb);
            // Connectors
            addEdge(x0, y0, zf, x0, y0, zb);
            addEdge(x1, y0, zf, x1, y0, zb);
            addEdge(x1, y1, zf, x1, y1, zb);
            addEdge(x0, y1, zf, x0, y1, zb);
            lineBatcher3D_->Flush();

            // Lens outline squares
            const float lr = 0.2f, lg = 0.2f, lb = 0.2f; // dark gray
            auto addLensEdge = [&](float ax, float ay, float az, float bx, float by, float bz) {
                lineBatcher3D_->AddLine(ax, ay, az, bx, by, bz, lr, lg, lb);
            };
            const float L1 = 0.15f; // outer
            const float L2 = 0.10f; // inner (glass)
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(2.0f);
            // Outer square at front (zf + epsilon)
            addLensEdge(-L1, -L1, zf + 0.001f,  L1, -L1, zf + 0.001f);
            addLensEdge( L1, -L1, zf + 0.001f,  L1,  L1, zf + 0.001f);
            addLensEdge( L1,  L1, zf + 0.001f, -L1,  L1, zf + 0.001f);
            addLensEdge(-L1,  L1, zf + 0.001f, -L1, -L1, zf + 0.001f);
            // Inner square (glass tint as lighter)
            const float gr = 0.9f, gg = 0.9f, gb = 1.0f;
            auto addGlassEdge = [&](float ax, float ay, float az, float bx, float by, float bz) {
                lineBatcher3D_->AddLine(ax, ay, az, bx, by, bz, gr, gg, gb);
            };
            addGlassEdge(-L2, -L2, zf + 0.002f,  L2, -L2, zf + 0.002f);
            addGlassEdge( L2, -L2, zf + 0.002f,  L2,  L2, zf + 0.002f);
            addGlassEdge( L2,  L2, zf + 0.002f, -L2,  L2, zf + 0.002f);
            addGlassEdge(-L2,  L2, zf + 0.002f, -L2, -L2, zf + 0.002f);
            lineBatcher3D_->Flush();
        }

    // Coordinate system at camera position (world axes) - batched
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(2.0f);
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, 1.0f, 0.0f);
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, 1.0f);
            lineBatcher3D_->Flush();
        }

        // Camera basis vectors - batched
        const float vecLen = 2.5f;
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(2.0f);
            // Forward - yellow
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f,
                                    static_cast<float>(forward.x * vecLen),
                                    static_cast<float>(forward.y * vecLen),
                                    static_cast<float>(forward.z * vecLen),
                                    1.0f, 1.0f, 0.0f);
            // Right - cyan
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f,
                                    static_cast<float>(right.x * vecLen),
                                    static_cast<float>(right.y * vecLen),
                                    static_cast<float>(right.z * vecLen),
                                    0.0f, 1.0f, 1.0f);
            // Up - magenta
            lineBatcher3D_->AddLine(0.0f, 0.0f, 0.0f,
                                    static_cast<float>(up.x * vecLen),
                                    static_cast<float>(up.y * vecLen),
                                    static_cast<float>(up.z * vecLen),
                                    1.0f, 0.0f, 1.0f);
            lineBatcher3D_->Flush();
        }

        // Look-at target marker (project forward) - batched
        Vec3 lookAt = Vec3{forward.x * 5.0, forward.y * 5.0, forward.z * 5.0};
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(2.0f);
            const float lr = 0.6f, lg = 1.0f, lb = 0.2f;
            lineBatcher3D_->AddLine(static_cast<float>(lookAt.x - 0.2), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z),
                                    static_cast<float>(lookAt.x + 0.2), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z),
                                    lr, lg, lb);
            lineBatcher3D_->AddLine(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y - 0.2), static_cast<float>(lookAt.z),
                                    static_cast<float>(lookAt.x), static_cast<float>(lookAt.y + 0.2), static_cast<float>(lookAt.z),
                                    lr, lg, lb);
            lineBatcher3D_->AddLine(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z - 0.2f),
                                    static_cast<float>(lookAt.x), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z + 0.2f),
                                    lr, lg, lb);
            lineBatcher3D_->Flush();
        }

        // Camera frustum visualization
        double fovRadians = (45.0 * std::acos(-1.0)) / 180.0;
        double aspect = (height != 0) ? static_cast<double>(width) / static_cast<double>(height) : 1.0;
        double nearDist = 0.5;
        double farDist = 5.0;
        double halfTan = std::tan(fovRadians / 2.0);

        double nearHeight = halfTan * nearDist;
        double nearWidth = nearHeight * aspect;
        double farHeight = halfTan * farDist;
        double farWidth = farHeight * aspect;

        auto scale = [](const Vec3& v, double s) {
            return Vec3{v.x * s, v.y * s, v.z * s};
        };

        Vec3 nearCenter = scale(forward, nearDist);
        Vec3 farCenter = scale(forward, farDist);
        Vec3 nearUp = scale(up, nearHeight);
        Vec3 nearRight = scale(right, nearWidth);
        Vec3 farUp = scale(up, farHeight);
        Vec3 farRight = scale(right, farWidth);

        auto add = [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
        };
        auto sub = [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
        };

        Vec3 nearTL = add(sub(nearCenter, nearRight), nearUp);
        Vec3 nearTR = add(add(nearCenter, nearRight), nearUp);
        Vec3 nearBL = sub(sub(nearCenter, nearRight), nearUp);
        Vec3 nearBR = sub(add(nearCenter, nearRight), nearUp);
        Vec3 farTL = add(sub(farCenter, farRight), farUp);
        Vec3 farTR = add(add(farCenter, farRight), farUp);
        Vec3 farBL = sub(sub(farCenter, farRight), farUp);
        Vec3 farBR = sub(add(farCenter, farRight), farUp);

        // Camera frustum visualization - batched
        if (lineBatcher3D_) {
            lineBatcher3D_->Begin();
            lineBatcher3D_->SetLineWidth(1.5f);
            const float fr = 1.0f, fg = 0.5f, fb = 0.0f;
            // Near rectangle (loop)
            lineBatcher3D_->AddLine(static_cast<float>(nearTL.x), static_cast<float>(nearTL.y), static_cast<float>(nearTL.z),
                                    static_cast<float>(nearTR.x), static_cast<float>(nearTR.y), static_cast<float>(nearTR.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearTR.x), static_cast<float>(nearTR.y), static_cast<float>(nearTR.z),
                                    static_cast<float>(nearBR.x), static_cast<float>(nearBR.y), static_cast<float>(nearBR.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearBR.x), static_cast<float>(nearBR.y), static_cast<float>(nearBR.z),
                                    static_cast<float>(nearBL.x), static_cast<float>(nearBL.y), static_cast<float>(nearBL.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearBL.x), static_cast<float>(nearBL.y), static_cast<float>(nearBL.z),
                                    static_cast<float>(nearTL.x), static_cast<float>(nearTL.y), static_cast<float>(nearTL.z), fr, fg, fb);
            // Far rectangle (loop)
            lineBatcher3D_->AddLine(static_cast<float>(farTL.x), static_cast<float>(farTL.y), static_cast<float>(farTL.z),
                                    static_cast<float>(farTR.x), static_cast<float>(farTR.y), static_cast<float>(farTR.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(farTR.x), static_cast<float>(farTR.y), static_cast<float>(farTR.z),
                                    static_cast<float>(farBR.x), static_cast<float>(farBR.y), static_cast<float>(farBR.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(farBR.x), static_cast<float>(farBR.y), static_cast<float>(farBR.z),
                                    static_cast<float>(farBL.x), static_cast<float>(farBL.y), static_cast<float>(farBL.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(farBL.x), static_cast<float>(farBL.y), static_cast<float>(farBL.z),
                                    static_cast<float>(farTL.x), static_cast<float>(farTL.y), static_cast<float>(farTL.z), fr, fg, fb);
            // Connect near to far corners
            lineBatcher3D_->AddLine(static_cast<float>(nearTL.x), static_cast<float>(nearTL.y), static_cast<float>(nearTL.z),
                                    static_cast<float>(farTL.x), static_cast<float>(farTL.y), static_cast<float>(farTL.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearTR.x), static_cast<float>(nearTR.y), static_cast<float>(nearTR.z),
                                    static_cast<float>(farTR.x), static_cast<float>(farTR.y), static_cast<float>(farTR.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearBL.x), static_cast<float>(nearBL.y), static_cast<float>(nearBL.z),
                                    static_cast<float>(farBL.x), static_cast<float>(farBL.y), static_cast<float>(farBL.z), fr, fg, fb);
            lineBatcher3D_->AddLine(static_cast<float>(nearBR.x), static_cast<float>(nearBR.y), static_cast<float>(nearBR.z),
                                    static_cast<float>(farBR.x), static_cast<float>(farBR.y), static_cast<float>(farBR.z), fr, fg, fb);
            lineBatcher3D_->Flush();
        }

        glPopMatrix();
        glLineWidth(1.0f);
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
        // Draw world coordinate system at origin
        // DrawCoordinateSystem(); // Commented out - debug axis visualization removed
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
// Larger tiny font renderer with a few extra glyphs (Z, F, X, =)
static void drawTinyCharSDL(SDL_Renderer* r, int x, int y, char c) {
    if (!r) return;
    const int scale = 8; // increase for much better visibility
    // helper to draw a 5x5 glyph stored as 5 columns
    auto drawGlyph = [&](const uint8_t* glyph){
        for (int col = 0; col < 5; ++col) {
            uint8_t colBits = glyph[col];
            for (int row = 0; row < 5; ++row) {
                if (colBits & (1 << (4 - row))) {
                    SDL_Rect px{ x + col*(scale+1), y + row*(scale+1), scale, scale };
                    compat_RenderFillRect(r, &px);
                }
            }
        }
    };

    if (c >= '0' && c <= '9') {
        const uint8_t* glyph = tinyFont[c - '0'];
        drawGlyph(glyph);
    } else if (c == '-') {
        SDL_Rect px{ x, y + 2 * (scale + 1), 5 * (scale + 1), scale };
        compat_RenderFillRect(r, &px);
    } else if (c == '.') {
        SDL_Rect px{ x + 4 * (scale + 1), y + 4 * (scale + 1), scale, scale };
        compat_RenderFillRect(r, &px);
    } else if (c == '=') {
        SDL_Rect top{ x, y + 1 * (scale + 1), 5 * (scale + 1), scale };
        SDL_Rect bot{ x, y + 3 * (scale + 1), 5 * (scale + 1), scale };
        compat_RenderFillRect(r, &top);
        compat_RenderFillRect(r, &bot);
    } else if (c == 'Z') {
        const uint8_t glyphZ[5] = {0x1F, 0x02, 0x04, 0x08, 0x1F};
        drawGlyph(glyphZ);
    } else if (c == 'F') {
        drawGlyph(glyphF);
    } else if (c == 'P') {
        drawGlyph(glyphP);
    } else if (c == 'S') {
        drawGlyph(glyphLtrS);
    } else if (c == 'X') {
        const uint8_t glyphX[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
        drawGlyph(glyphX);
    } else if (c == ':') {
        // draw two dots for colon
        SDL_Rect d1{ x + 2 * (scale + 1), y + 1 * (scale + 1), scale / 2, scale / 2 };
        SDL_Rect d2{ x + 2 * (scale + 1), y + 3 * (scale + 1), scale / 2, scale / 2 };
        compat_RenderFillRect(r, &d1);
        compat_RenderFillRect(r, &d2);
    } else if (c == 'V') {
        drawGlyph(glyphV);
    } else if (c == 'Y') {
        drawGlyph(glyphY);
    } else if (c == 'N') {
        drawGlyph(glyphN);
    } else if (c == 'C') {
        drawGlyph(glyphC);
    } else if (c == 'O') {
        drawGlyph(glyphO);
    } else if (c == 'A') {
        drawGlyph(glyphA);
    } else if (c == 'T') {
        drawGlyph(glyphT);
    } else if (c == 'G') {
        drawGlyph(glyphG);
    } else if (c == ' ') {
        // leave blank for space
    } else {
        // unknown: leave blank
    }
}

// Draw a single seven-segment style digit at (x,y).
static void drawSevenSegDigit(SDL_Renderer* r, int x, int y, int segLen, int segThick, char c) {
    if (!r) return;
    // segment bitmask: bit0=a(top), bit1=b(upper-right), bit2=c(lower-right), bit3=d(bottom), bit4=e(lower-left), bit5=f(upper-left), bit6=g(middle)
    static const uint8_t segMap[10] = {
        // a b c d e f g
        0b0111111, // 0: a b c d e f
        0b0000110, // 1: b c
        0b1011011, // 2: a b g e d
        0b1001111, // 3: a b g c d
        0b1100110, // 4: f g b c
        0b1101101, // 5: a f g c d
        0b1111101, // 6: a f g e c d
        0b0000111, // 7: a b c
        0b1111111, // 8: all
        0b1101111  // 9: a b c d f g
    };

    auto drawSeg = [&](int sx, int sy, int w, int h){ SDL_Rect rct{sx, sy, w, h}; compat_RenderFillRect(r, &rct); };

    int a = x + segThick, ay = y, aw = segLen, ah = segThick;
    int f_x = x, f_y = y + segThick, f_w = segThick, f_h = segLen;
    int b_x = x + segThick + segLen, b_y = y + segThick, b_w = segThick, b_h = segLen;
    int g_x = x + segThick, g_y = y + segThick + segLen, g_w = segLen, g_h = segThick;
    int e_x = x, e_y = y + 2*segThick + segLen, e_w = segThick, e_h = segLen;
    int c_x = x + segThick + segLen, c_y = y + 2*segThick + segLen, c_w = segThick, c_h = segLen;
    int d_x = x + segThick, d_y = y + 2*(segThick + segLen), d_w = segLen, d_h = segThick;

    if (c == '-') {
        drawSeg(g_x, g_y, g_w, g_h);
        return;
    }
    if (c == '.') {
        // small dot at bottom-right
    SDL_Rect dot{ x + segThick + segLen + segThick/2, y + 2*(segThick+segLen) + segThick, segThick, segThick };
    compat_RenderFillRect(r, &dot);
        return;
    }

    if (c < '0' || c > '9') return;
    uint8_t bits = segMap[c - '0'];
    if (bits & 0x01) drawSeg(a, ay, aw, ah); // a
    if (bits & 0x02) drawSeg(b_x, b_y, b_w, b_h); // b
    if (bits & 0x04) drawSeg(c_x, c_y, c_w, c_h); // c
    if (bits & 0x08) drawSeg(d_x, d_y, d_w, d_h); // d
    if (bits & 0x10) drawSeg(e_x, e_y, e_w, e_h); // e
    if (bits & 0x20) drawSeg(f_x, f_y, f_w, f_h); // f
    if (bits & 0x40) drawSeg(g_x, g_y, g_w, g_h); // g
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

void Viewport3D::DrawHUD(const class Camera* camera,
                         double fps,
                         double playerX,
                         double playerY,
                         double playerZ,
                         const struct EnergyHUDTelemetry* energyTelemetry) {
    (void)playerZ; // only used in SDL path; silence unused param warning for GLFW-only builds
    if (debugLogging_) std::cout << "Viewport3D::DrawHUD() called" << std::endl;
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
                std::cout << "OpenGL error before DrawHUD: " << error << std::endl;
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
                uiBatcher_->AddQuad(10, 10, 340, 110, 0.2f, 0.2f, 0.2f, 0.8f);
                uiBatcher_->AddRectOutline(10, 10, 340, 120, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f);
            }

            // Simple 7-segment style display for GLFW (similar to SDL version)
            auto addRect = [this](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
                if (uiBatcher_) {
                    uiBatcher_->AddQuad(x, y, w, h, r, g, b, a);
                }
            };

            auto drawSevenSegDigitGL = [&](int x, int y, int segLen, int segThick, char c, float r, float g, float b) {
                static const uint8_t segMap[10] = {
                    0b0111111, // 0
                    0b0000110, // 1
                    0b1011011, // 2
                    0b1001111, // 3
                    0b1100110, // 4
                    0b1101101, // 5
                    0b1111101, // 6
                    0b0000111, // 7
                    0b1111111, // 8
                    0b1101111  // 9
                };
                auto drawSegGL = [&](float sx, float sy, float w, float h) {
                    if (uiBatcher_) {
                        uiBatcher_->AddQuad(sx, sy, w, h, r, g, b, 1.0f);
                    }
                };
                int a = x + segThick, ay = y, aw = segLen, ah = segThick;
                int f_x = x, f_y = y + segThick, f_w = segThick, f_h = segLen;
                int b_x = x + segThick + segLen, b_y = y + segThick, b_w = segThick, b_h = segLen;
                int g_x = x + segThick, g_y = y + segThick + segLen, g_w = segLen, g_h = segThick;
                int e_x = x, e_y = y + 2 * segThick + segLen, e_w = segThick, e_h = segLen;
                int c_x = x + segThick + segLen, c_y = y + 2 * segThick + segLen, c_w = segThick, c_h = segLen;
                int d_x = x + segThick, d_y = y + 2 * (segThick + segLen), d_w = segLen, d_h = segThick;
                if (c == '-') {
                    drawSegGL(g_x, g_y, g_w, g_h);
                    return;
                }
                if (c == '.') {
                    addRect(x + segThick + segLen + segThick / 2, y + 2 * (segThick + segLen) + segThick, segThick, segThick, 1, 1, 1);
                    return;
                }
                if (c < '0' || c > '9') return;
                uint8_t bits = segMap[c - '0'];
                if (bits & 0x01) drawSegGL(a, ay, aw, ah);
                if (bits & 0x02) drawSegGL(b_x, b_y, b_w, b_h);
                if (bits & 0x04) drawSegGL(c_x, c_y, c_w, c_h);
                if (bits & 0x08) drawSegGL(d_x, d_y, d_w, d_h);
                if (bits & 0x10) drawSegGL(e_x, e_y, e_w, e_h);
                if (bits & 0x20) drawSegGL(f_x, f_y, f_w, f_h);
                if (bits & 0x40) drawSegGL(g_x, g_y, g_w, g_h);
            };

            // Layout
            int segLen = 12;
            int segThick = 4;
            int spacing = segLen + segThick + 6;
            int x = 18, y = 25;

            // Label "FPS:"
            glColor3f(0.7f, 0.7f, 0.7f);
            addRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            addRect(x, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 14;

            // FPS value
            char fbuf[16]; 
            snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
            glColor3f(1.0f, 0.9f, 0.5f);
            for (char* p = fbuf; *p; ++p) {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                x += spacing;
            }

            x += 12;
            // Zoom label "Z:"
            glColor3f(0.7f, 0.7f, 0.7f);
            addRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            addRect(x + segLen - 2, y + segThick, 4, segThick, 0.7f, 0.7f, 0.7f);
            addRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;

            // Zoom value
            char zbuf[32];
            if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); 
            else snprintf(zbuf, sizeof(zbuf), "1.0");
            glColor3f(1.0f, 0.9f, 0.5f);
            for (char* p = zbuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.', 1.0f, 0.9f, 0.5f);
                    x += spacing / 2;
                }
            }

            float glyphScale = 4.0f;
            float glyphAdvance = (5.0f + 1.0f) * glyphScale;
            float vsyncX = static_cast<float>(x + 12);
            float vsyncY = static_cast<float>(y);
            const char* vsLabel = "VSYNC";
            glColor3f(0.7f, 0.7f, 0.7f);
            for (const char* p = vsLabel; *p; ++p) {
                DrawTinyChar2D(vsyncX, vsyncY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
                vsyncX += glyphAdvance;
            }
            vsyncX += glyphScale * 2.0f;
            const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
            glColor3f(1.0f, 0.9f, 0.5f);
            for (const char* p = vsValue; *p; ++p) {
                DrawTinyChar2D(vsyncX, vsyncY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
                vsyncX += glyphAdvance;
            }

            vsyncX += glyphScale * 2.0f;
            const char* capLabel = "CAP";
            glColor3f(0.7f, 0.7f, 0.7f);
            for (const char* p = capLabel; *p; ++p) {
                DrawTinyChar2D(vsyncX, vsyncY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
                vsyncX += glyphAdvance;
            }

            vsyncX += glyphScale * 2.0f;
            char capBuf[16];
            if (frameRateLimitHint_ <= 0.0) {
                snprintf(capBuf, sizeof(capBuf), "INF");
            } else {
                snprintf(capBuf, sizeof(capBuf), "%.0f", frameRateLimitHint_);
            }
            glColor3f(1.0f, 0.9f, 0.5f);
            for (char* p = capBuf; *p; ++p) {
                DrawTinyChar2D(vsyncX, vsyncY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
                vsyncX += glyphAdvance;
            }

            // Second row - Position
            x = 18; y += 50;
            glColor3f(0.7f, 0.7f, 0.7f);
            // X label
            addRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            addRect(x + 6, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;
            
            // X value
            char xbuf[32]; 
            snprintf(xbuf, sizeof(xbuf), "%.1f", playerX);
            glColor3f(0.5f, 1.0f, 1.0f);
            for (char* p = xbuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p, 0.5f, 1.0f, 1.0f);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.', 0.5f, 1.0f, 1.0f);
                    x += spacing / 2;
                } else if (*p == '-') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '-', 0.5f, 1.0f, 1.0f);
                    x += spacing;
                }
            }

            x += 12;
            // Y label
            glColor3f(0.7f, 0.7f, 0.7f);
            addRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            addRect(x + 6, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;

            // Y value
            char ybuf[32]; 
            snprintf(ybuf, sizeof(ybuf), "%.1f", playerY);
            glColor3f(0.5f, 1.0f, 1.0f);
            for (char* p = ybuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p, 0.5f, 1.0f, 1.0f);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.', 0.5f, 1.0f, 1.0f);
                    x += spacing / 2;
                } else if (*p == '-') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '-', 0.5f, 1.0f, 1.0f);
                    x += spacing;
                }
            }

            // Energy management overlay
#if defined(USE_GLFW)
            if (energyTelemetry && energyTelemetry->valid) {
                RenderEnergyPanel(uiBatcher_.get(), *energyTelemetry, width, height);
            }
#endif

            // Flush batched UI rendering
            if (uiBatcher_) {
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
            std::cout << "OpenGL error before DrawHUD: " << error << std::endl;
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

        // Helper functions
        auto addRect = [this](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
            if (uiBatcher_) {
                uiBatcher_->AddQuad(x, y, w, h, r, g, b, a);
            }
        };

        auto drawSevenSegDigitGL = [&](int x, int y, int segLen, int segThick, char c, float r, float g, float b) {
            static const uint8_t segMap[10] = {
                0b0111111, // 0
                0b0000110, // 1
                0b1011011, // 2
                0b1001111, // 3
                0b1100110, // 4
                0b1101101, // 5
                0b1111101, // 6
                0b0000111, // 7
                0b1111111, // 8
                0b1101111  // 9
            };
            auto drawSegGL = [&](float sx, float sy, float w, float h) {
                if (uiBatcher_) {
                    uiBatcher_->AddQuad(sx, sy, w, h, r, g, b, 1.0f);
                }
            };
            int a = x + segThick, ay = y, aw = segLen, ah = segThick;
            int f_x = x, f_y = y + segThick, f_w = segThick, f_h = segLen;
            int b_x = x + segThick + segLen, b_y = y + segThick, b_w = segThick, b_h = segLen;
            int g_x = x + segThick, g_y = y + segThick + segLen, g_w = segLen, g_h = segThick;
            int e_x = x, e_y = y + 2 * segThick + segLen, e_w = segThick, e_h = segLen;
            int c_x = x + segThick + segLen, c_y = y + 2 * segThick + segLen, c_w = segThick, c_h = segLen;
            int d_x = x + segThick, d_y = y + 2 * (segThick + segLen), d_w = segLen, d_h = segThick;
            if (c == '-') {
                drawSegGL(g_x, g_y, g_w, g_h);
                return;
            }
            if (c == '.') {
                addRect(x + segThick + segLen + segThick / 2, y + 2 * (segThick + segLen) + segThick, segThick, segThick, 1, 1, 1);
                return;
            }
            if (c < '0' || c > '9') return;
            uint8_t bits = segMap[c - '0'];
            if (bits & 0x01) drawSegGL(a, ay, aw, ah);
            if (bits & 0x02) drawSegGL(b_x, b_y, b_w, b_h);
            if (bits & 0x04) drawSegGL(c_x, c_y, c_w, c_h);
            if (bits & 0x08) drawSegGL(d_x, d_y, d_w, d_h);
            if (bits & 0x10) drawSegGL(e_x, e_y, e_w, e_h);
            if (bits & 0x20) drawSegGL(f_x, f_y, f_w, f_h);
            if (bits & 0x40) drawSegGL(g_x, g_y, g_w, g_h);
        };

        // Layout
        int segLen = 16;
        int segThick = 6;
        int spacing = segLen + segThick + 8;
        int x = 18, y = 18;

        // Label "Z:"
        addRect(x, y, 4, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x + segLen - 2, y + segThick, 4, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.5f, 0.5f, 0.5f);
        x += 24;

        // Zoom
        char zbuf[32];
        if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); else snprintf(zbuf, sizeof(zbuf), "0.0");
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = zbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.', 1.0f, 0.9f, 0.5f);
                x += spacing / 2;
            }
        }

        x += 18;
        // FPS label
        addRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x, y + 2 * (segThick + segLen), 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = fbuf; *p; ++p) {
            drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
            x += spacing;
        }

        float glyphScale = 4.0f;
        float glyphAdvance = (5.0f + 1.0f) * glyphScale;
        float infoX = static_cast<float>(x + 12);
        float infoY = static_cast<float>(y);
        const char* vsLabel = "VSYNC";
        glColor3f(0.7f, 0.7f, 0.7f);
        for (const char* p = vsLabel; *p; ++p) {
            DrawTinyChar2D(infoX, infoY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
            infoX += glyphAdvance;
        }
        infoX += glyphScale * 2.0f;
        const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
        glColor3f(1.0f, 0.9f, 0.5f);
        for (const char* p = vsValue; *p; ++p) {
            DrawTinyChar2D(infoX, infoY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
            infoX += glyphAdvance;
        }

        infoX += glyphScale * 2.0f;
        const char* capLabel = "CAP";
        glColor3f(0.7f, 0.7f, 0.7f);
        for (const char* p = capLabel; *p; ++p) {
            DrawTinyChar2D(infoX, infoY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
            infoX += glyphAdvance;
        }

        infoX += glyphScale * 2.0f;
        char capBuf[16];
        if (frameRateLimitHint_ <= 0.0) {
            snprintf(capBuf, sizeof(capBuf), "INF");
        } else {
            snprintf(capBuf, sizeof(capBuf), "%.0f", frameRateLimitHint_);
        }
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = capBuf; *p; ++p) {
            DrawTinyChar2D(infoX, infoY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
            infoX += glyphAdvance;
        }

        x += 18;
        // Player X label
        addRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x + 8, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = xbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.', 1.0f, 0.9f, 0.5f);
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-', 1.0f, 0.9f, 0.5f);
                x += spacing;
            }
        }

        // Next row
        x = 18; y += 60;
        // Player Y label
        addRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x + 8, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char ybuf[32]; snprintf(ybuf, sizeof(ybuf), "%.2f", playerY);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = ybuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.', 1.0f, 0.9f, 0.5f);
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-', 1.0f, 0.9f, 0.5f);
                x += spacing;
            }
        }

        x += 18;
        // Player Z label
        addRect(x, y, 4, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x + segLen - 2, y + segThick, 4, segThick, 0.5f, 0.5f, 0.5f);
        addRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.5f, 0.5f, 0.5f);
        x += 24;
        char zbuf2[32]; snprintf(zbuf2, sizeof(zbuf2), "%.2f", playerZ);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = zbuf2; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p, 1.0f, 0.9f, 0.5f);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.', 1.0f, 0.9f, 0.5f);
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-', 1.0f, 0.9f, 0.5f);
                x += spacing;
            }
        }

        // Flush batched UI rendering
        if (uiBatcher_) {
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

        // HUD text color: bright white for digits
        SDL_SetRenderDrawColor(sdlRenderer, 245, 245, 245, 255);

        // Layout for seven-seg digits (larger for clarity)
        int segLen = 16;
        int segThick = 6;
        int spacing = segLen + segThick + 8;
        int x = 18, y = 18;

        // Draw label "Z:" using small rectangles (simple readable glyph)
        SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
        // draw 'Z' as a simple diagonal-ish using rects
        SDL_Rect rz1{ x, y, 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz1);
        SDL_Rect rz2{ x + segLen - 2, y + segThick, 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz2);
        SDL_Rect rz3{ x, y + 2*(segThick + segLen), 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz3);
        x += 24;

        // Draw Zoom numeric value (one digit before decimal, one after) as "xx.x"
        char zbuf[32];
        if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); else snprintf(zbuf, sizeof(zbuf), "0.0");
        SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
        for (char* p = zbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
                x += spacing/2;
            }
        }

        x += 18; // gap before FPS
        // FPS label and digits
        SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
        // draw "FPS:" label as three small rects
        SDL_Rect rf1{ x, y, 6, segThick }; compat_RenderFillRect(sdlRenderer, &rf1);
        SDL_Rect rf2{ x, y + segThick + 2, 6, segThick }; compat_RenderFillRect(sdlRenderer, &rf2);
        SDL_Rect rf3{ x, y + 2*(segThick+segLen), 6, segThick }; compat_RenderFillRect(sdlRenderer, &rf3);
        x += 18;
        char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
        SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
        for (char* p = fbuf; *p; ++p) {
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
            x += spacing;
        }

        x += 18;
        // Player X label
        SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
        SDL_Rect rx1{ x, y, 6, segThick }; compat_RenderFillRect(sdlRenderer, &rx1);
        SDL_Rect rx2{ x + 8, y + segThick + 2, 6, segThick }; compat_RenderFillRect(sdlRenderer, &rx2);
        x += 18;
        // Player X numeric with 2 decimals (we draw only digits and decimal point)
        char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
        SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
        for (char* p = xbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
                x += spacing/2;
            } else if (*p == '-') {
                // draw minus sign as middle seg
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '-');
                x += spacing;
            }
        }

        // Next row for Y and Z
        x = 18; y += 60;
        // Player Y label
        SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
        SDL_Rect ry1{ x, y, 6, segThick }; compat_RenderFillRect(sdlRenderer, &ry1);
        SDL_Rect ry2{ x + 8, y + segThick + 2, 6, segThick }; compat_RenderFillRect(sdlRenderer, &ry2);
        x += 18;
        char ybuf[32]; snprintf(ybuf, sizeof(ybuf), "%.2f", playerY);
        SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
        for (char* p = ybuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
                x += spacing/2;
            } else if (*p == '-') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '-');
                x += spacing;
            }
        }

        x += 18;
        // Player Z label
        SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
        SDL_Rect rz1_z{ x, y, 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz1_z);
        SDL_Rect rz2_z{ x + segLen - 2, y + segThick, 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz2_z);
        SDL_Rect rz3_z{ x, y + 2*(segThick + segLen), 4, segThick }; compat_RenderFillRect(sdlRenderer, &rz3_z);
        x += 24;
        char zbuf2[32]; snprintf(zbuf2, sizeof(zbuf2), "%.2f", playerZ);
        SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
        for (char* p = zbuf2; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
                x += spacing/2;
            } else if (*p == '-') {
                drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '-');
                x += spacing;
            }
        }
    }
#endif
    if (IsUsingSDLGL()) {
        GLenum error = glGetError();
        if (debugLogging_ && error != GL_NO_ERROR) {
            std::cout << "OpenGL error after DrawHUD: " << error << std::endl;
        }
    }
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
        if (!particleRenderer_->Init()) {
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
