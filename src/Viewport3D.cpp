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
#include <GL/glu.h>
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
#include "Camera.h"
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#include "Camera.h"
#endif
#include "VisualFeedbackSystem.h"
#ifdef USE_GLFW
#include "TextRenderer.h"
#endif
#if defined(USE_GLFW) || defined(USE_SDL)
#include "graphics/ParticleRenderer.h"
#endif

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

#if defined(USE_GLFW) || defined(USE_SDL)
static void drawTinyCharGL(float x, float y, char c, float scale, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    auto drawPixel = [&](float px, float py) {
        glVertex2f(x + px * scale, y + py * scale);
        glVertex2f(x + (px + 1.0f) * scale, y + py * scale);
        glVertex2f(x + (px + 1.0f) * scale, y + (py + 1.0f) * scale);
        glVertex2f(x + px * scale, y + (py + 1.0f) * scale);
    };

    auto drawGlyph = [&](const uint8_t* glyph) {
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 5; ++row) {
                if (bits & (1 << (4 - row))) {
                    drawPixel(static_cast<float>(col), static_cast<float>(row));
                }
            }
        }
    };

    if (c >= '0' && c <= '9') {
        drawGlyph(tinyFont[c - '0']);
    } else if (c == '-') {
        drawPixel(0.0f, 2.0f);
        drawPixel(1.0f, 2.0f);
        drawPixel(2.0f, 2.0f);
        drawPixel(3.0f, 2.0f);
        drawPixel(4.0f, 2.0f);
    } else if (c == '.') {
        drawPixel(4.0f, 4.0f);
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
    glEnd();
}
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

void DrawQuad2D(float x, float y, float w, float h, const Color4& color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void DrawBorder2D(float x, float y, float w, float h, const Color4& color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

float Clamp01(double value) {
    if (value < 0.0) return 0.0f;
    if (value > 1.0) return 1.0f;
    return static_cast<float>(value);
}

void DrawFillBar(float x, float y, float w, float h, double fillAmount, const Color4& fillColor) {
    DrawQuad2D(x, y, w, h, MakeColor(0.1f, 0.1f, 0.14f, 0.9f));
    float fill = Clamp01(fillAmount);
    if (fill > 0.0f) {
        DrawQuad2D(x, y, w * fill, h, fillColor);
    }
    DrawBorder2D(x, y, w, h, MakeColor(0.35f, 0.35f, 0.4f, 0.9f));
}

void RenderEnergyPanel(const EnergyHUDTelemetry& telemetry, int screenWidth, int screenHeight) {
    const float panelWidth = 420.0f;
    const float panelHeight = 300.0f;
    const float margin = 18.0f;
    const float panelX = static_cast<float>(screenWidth) - panelWidth - margin;
    const float panelY = margin;

    DrawQuad2D(panelX, panelY, panelWidth, panelHeight, MakeColor(0.02f, 0.02f, 0.04f, 0.82f));
    DrawBorder2D(panelX, panelY, panelWidth, panelHeight, MakeColor(0.45f, 0.55f, 0.75f, 0.8f));

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
        DrawQuad2D(bx, by, boxWidth, boxHeight, MakeColor(0.05f, 0.05f, 0.09f, 0.85f));
        DrawBorder2D(bx, by, boxWidth, boxHeight, MakeColor(0.25f, 0.35f, 0.55f, 0.9f));

        TextRenderer::RenderText(label,
                                 static_cast<int>(bx + 12.0f),
                                 static_cast<int>(by + 20.0f),
                                 TextColor::White(),
                                 FontSize::Medium);

        Color4 statusColor = StatusColor(percent, rechargingHighlight);
        DrawFillBar(bx + 12.0f,
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
        DrawFillBar(barX, rowY - 12.0f, barWidth, 12.0f, allocation, MakeColor(0.35f, 0.75f, 0.95f, 0.9f));
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
    , usingSDL(false)
    , useGL(false)
    , vsyncEnabled_(false)
    , frameRateLimitHint_(144.0)
#ifdef USE_SDL
    , sdlWindow(nullptr)
    , sdlRenderer(nullptr)
    , sdlGLContext(nullptr)
#endif
#ifdef USE_GLFW
    , glfwWindow(nullptr)
#endif
    , activeLayoutIndex_(0)
{
}

Viewport3D::~Viewport3D() {}

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
    return layouts_[activeLayoutIndex_];
}

std::string Viewport3D::GetActiveLayoutName() const {
    if (layouts_.empty()) {
        return std::string("Single View");
    }
    return layouts_[activeLayoutIndex_].name;
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
    if (layouts_.empty()) {
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
    if (usingSDL && useGL && sdlWindow) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }
#endif
#if defined(USE_GLFW)
    if (!usingSDL && useGL && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        glfwSwapInterval(enabled ? 1 : 0);
    }
#endif
}

void Viewport3D::BeginFrame() {
    EnsureLayoutConfiguration();
#if defined(USE_SDL)
    if (usingSDL && useGL && sdlWindow) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    if (!usingSDL && useGL && glfwWindow) {
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
    if (usingSDL && useGL && sdlWindow) {
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
    }
#endif
#if defined(USE_GLFW)
    if (!usingSDL && useGL && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif

    if (useGL) {
        ActivateOpenGLView(view, camera, playerX, playerY, playerZ);
    } else if (usingSDL) {
        ActivateSDLView(view);
    }
}

void Viewport3D::ApplyViewportView(const ViewportView& view) {
#if defined(USE_GLFW) || defined(USE_SDL)
    int viewportWidth = std::max(1, static_cast<int>(view.normalizedWidth * static_cast<double>(width)));
    int viewportHeight = std::max(1, static_cast<int>(view.normalizedHeight * static_cast<double>(height)));
    int viewportX = static_cast<int>(view.normalizedX * static_cast<double>(width));
    int viewportY = static_cast<int>(view.normalizedY * static_cast<double>(height));

    if (useGL) {
        int glViewportY = height - viewportY - viewportHeight;
        if (glViewportY < 0) {
            glViewportY = 0;
        }
        glViewport(viewportX, glViewportY, viewportWidth, viewportHeight);
    }
#if defined(USE_SDL)
    if (usingSDL && !useGL && sdlRenderer) {
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
    if (useGL) {
        glViewport(0, 0, width, height);
    }
#endif
#if defined(USE_SDL)
    if (usingSDL && !useGL && sdlRenderer) {
        SDL_RenderSetViewport(sdlRenderer, nullptr);
    }
#endif
}

void Viewport3D::ActivateOpenGLView(const ViewportView& view, const Camera* camera, double playerX, double playerY, double playerZ) {
#if defined(USE_GLFW) || defined(USE_SDL)
    ApplyViewportView(view);
    if (view.overlay) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int viewportWidth = std::max(1, static_cast<int>(view.normalizedWidth * static_cast<double>(width)));
    int viewportHeight = std::max(1, static_cast<int>(view.normalizedHeight * static_cast<double>(height)));
    double aspect = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
    gluPerspective(45.0, aspect, 0.1, 100.0);
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
    ApplyViewportView(view);
#else
    (void)view;
#endif
}

void Viewport3D::DrawMinimapOverlay(double playerX, double playerY, double playerZ) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (useGL) {
        const float extent = 8.0f;
        glDisable(GL_LIGHTING);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor3f(0.2f, 0.9f, 0.2f);
        glVertex3f(static_cast<float>(playerX - extent), static_cast<float>(playerY), static_cast<float>(playerZ));
        glVertex3f(static_cast<float>(playerX + extent), static_cast<float>(playerY), static_cast<float>(playerZ));
        glVertex3f(static_cast<float>(playerX), static_cast<float>(playerY - extent), static_cast<float>(playerZ));
        glVertex3f(static_cast<float>(playerX), static_cast<float>(playerY + extent), static_cast<float>(playerZ));
        glEnd();
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 0.3f, 0.3f);
        glVertex3f(static_cast<float>(playerX), static_cast<float>(playerY), static_cast<float>(playerZ));
        glEnd();
        glLineWidth(1.0f);
    }
#endif
#ifdef USE_SDL
    if (usingSDL && !useGL && sdlRenderer) {
        SDL_SetRenderDrawColor(sdlRenderer, 40, 200, 40, 255);
        const int extent = static_cast<int>(std::round(width * 0.05));
        int cx = width / 2;
        int cy = height / 2;
        compat_RenderDrawLine(sdlRenderer, cx - extent, cy, cx + extent, cy);
        compat_RenderDrawLine(sdlRenderer, cx, cy - extent, cx, cy + extent);
        SDL_SetRenderDrawColor(sdlRenderer, 255, 60, 60, 255);
        SDL_Rect dot{cx - 2, cy - 2, 4, 4};
        compat_RenderFillRect(sdlRenderer, &dot);
    }
#endif
#if !defined(USE_GLFW) && !defined(USE_SDL)
    (void)playerX;
    (void)playerY;
#endif
    (void)playerZ;
}

void Viewport3D::Init() {
    std::cout << "Viewport3D::Init() starting" << std::endl;
    usingSDL = false;
#ifdef USE_GLFW
    std::cout << "USE_GLFW is defined, attempting GLFW initialization" << std::endl;
#endif
#ifndef USE_GLFW
    std::cout << "USE_GLFW is NOT defined, falling back to SDL or ASCII" << std::endl;
#endif
#ifdef USE_GLFW
    if (!glfwInit()) {
        std::cerr << "Viewport3D: GLFW initialization failed" << std::endl;
        return;
    }
    std::cout << "GLFW initialized successfully" << std::endl;

    struct GLContextAttempt {
        int major;
        int minor;
        bool coreProfile;
        bool forwardCompatible;
        const char* description;
    };

    const std::vector<GLContextAttempt> contextAttempts = {
        {4, 6, true, true, "OpenGL 4.6 Core"},
        {4, 5, true, true, "OpenGL 4.5 Core"},
        {4, 3, true, true, "OpenGL 4.3 Core"},
        {3, 3, true, true, "OpenGL 3.3 Core"},
        {2, 1, false, false, "OpenGL 2.1 Compatibility"},
    };

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
        } else {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        }
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, attempt.forwardCompatible ? GL_TRUE : GL_FALSE);

        std::cout << "Viewport3D: Trying " << attempt.description << " context" << std::endl;
        glfwWindow = glfwCreateWindow(width, height, "Nova Engine", primaryMonitor, nullptr);
        if (glfwWindow) {
            chosenAttempt = &attempt;
            break;
        }

        std::cerr << "Viewport3D: GLFW window creation failed for " << attempt.description << std::endl;
    }

    if (!glfwWindow) {
        std::cerr << "Viewport3D: Unable to create any OpenGL context" << std::endl;
        glfwTerminate();
        return;
    }

    std::cout << "GLFW window created successfully using " << chosenAttempt->description << std::endl;

    // Make the OpenGL context current
    glfwMakeContextCurrent(glfwWindow);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Viewport3D: Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(glfwWindow);
        glfwWindow = nullptr;
        glfwTerminate();
        return;
    }
    // Disable VSync to allow higher FPS by default
    SetVSyncEnabled(false);
    useGL = true;

    // Setup basic GL state
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable relative mouse mode for camera control (GLFW equivalent)
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "Viewport3D: Using GLFW with OpenGL for rendering." << std::endl;
    return;
#endif
#ifndef USE_GLFW
    std::cout << "USE_GLFW is NOT defined, falling back to SDL or ASCII" << std::endl;
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
            sdlWindow = compat_CreateWindow("Nova Engine", width, height, (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));
            if (sdlWindow) {
                writeLog("Viewport3D: SDL_CreateWindow (GL) succeeded");
                SDL_RaiseWindow(sdlWindow);
#ifdef _WIN32
                // On Windows, force the window to foreground and give it focus
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                if (SDL_GetWindowWMInfo(sdlWindow, &wmInfo)) {
                    writeLog("Viewport3D: Setting window to foreground");
                    SetForegroundWindow(wmInfo.info.win.window);
                    SetFocus(wmInfo.info.win.window);
                    // Additional focus tricks
                    ShowWindow(wmInfo.info.win.window, SW_RESTORE);
                    SetWindowPos(wmInfo.info.win.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetWindowPos(wmInfo.info.win.window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    writeLog("Viewport3D: Window focus operations completed");
                } else {
                    writeLog("Viewport3D: SDL_GetWindowWMInfo failed for focus");
                }
#endif
                writeLog("Viewport3D: Before SDL_GL_CreateContext");
                sdlGLContext = SDL_GL_CreateContext(sdlWindow);
                writeLog("Viewport3D: After SDL_GL_CreateContext");
                if (sdlGLContext) {
                    // Successfully created an OpenGL context
                    useGL = true;
                    usingSDL = true;
                    // Make the context current
                    writeLog("Viewport3D: Before SDL_GL_MakeCurrent");
                    if (SDL_GL_MakeCurrent(sdlWindow, sdlGLContext) != 0) {
                        writeLog(std::string("Viewport3D: SDL_GL_MakeCurrent failed: ") + SDL_GetError());
                        compat_GL_DeleteContext(sdlGLContext);
                        sdlGLContext = nullptr;
                        useGL = false;
                        if (sdlWindow) { SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr; }
                    } else {
                        writeLog("Viewport3D: SDL_GL_MakeCurrent succeeded");
                        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
                            std::string msg = "Viewport3D: Failed to initialize GLAD";
                            std::cerr << msg << std::endl;
                            writeLog(msg);
                            compat_GL_DeleteContext(sdlGLContext);
                            sdlGLContext = nullptr;
                            useGL = false;
                            if (sdlWindow) { SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr; }
                        } else {
                        // Setup basic GL state
                        glViewport(0, 0, width, height);
                        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        // Enable relative mouse mode for camera control
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        std::cout << "Viewport3D: Using OpenGL for rendering." << std::endl;
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
                // On Windows, force the window to foreground and give it focus
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                if (SDL_GetWindowWMInfo(sdlWindow, &wmInfo)) {
                    writeLog("Viewport3D: Setting renderer window to foreground");
                    SetForegroundWindow(wmInfo.info.win.window);
                    SetFocus(wmInfo.info.win.window);
                    // Additional focus tricks
                    ShowWindow(wmInfo.info.win.window, SW_RESTORE);
                    SetWindowPos(wmInfo.info.win.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetWindowPos(wmInfo.info.win.window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    writeLog("Viewport3D: Renderer window focus operations completed");
                } else {
                    writeLog("Viewport3D: SDL_GetWindowWMInfo failed for renderer focus");
                }
#endif
                // SDL3: CreateRenderer takes (window, name). Pass NULL to let SDL pick the best renderer.
                writeLog("Viewport3D: Before SDL_CreateRenderer (accelerated)");
                sdlRenderer = compat_CreateRenderer(sdlWindow, NULL);
                writeLog("Viewport3D: After SDL_CreateRenderer");
                if (sdlRenderer) {
                    writeLog("Viewport3D: SDL_CreateRenderer succeeded");
                    usingSDL = true;
                    std::cout << "Viewport3D: Using SDL renderer for rendering." << std::endl;
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
                        usingSDL = true;
                        std::cout << "Viewport3D: Using SDL software renderer for rendering." << std::endl;
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
    std::cout << "Viewport3D Initialized with size " << width << "x" << height << " (ASCII fallback)" << std::endl;
}

void Viewport3D::Render(const class Camera* camera, double playerX, double playerY, double playerZ) {
    EnsureLayoutConfiguration();
    BeginFrame();

    if (GetActiveViewCount() == 0) {
        return;
    }

    ActivateView(camera, playerX, playerY, playerZ, 0);

#if defined(USE_GLFW) || defined(USE_SDL)
    if (useGL && camera) {
        DrawCameraDebug(camera, playerX, playerY, playerZ, ViewRole::Main);
    } else if (usingSDL && !useGL) {
        // 2D fallback: nothing additional required here
    }
#else
    (void)camera;
    (void)playerX;
    (void)playerY;
    (void)playerZ;
#endif
}

void Viewport3D::Clear() {
    if (usingSDL) {
#ifdef USE_SDL
        if (useGL) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
            SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
            // SDL3 keeps SDL_RenderClear as-is
            SDL_RenderClear(sdlRenderer);
        }
#endif
    } else {
#ifdef USE_GLFW
        if (useGL && glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
#endif
    }
}

void Viewport3D::Present() {
    if (usingSDL) {
#ifdef USE_SDL
        if (useGL) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            SDL_GL_SwapWindow(sdlWindow);
        } else {
            SDL_RenderPresent(sdlRenderer);
        }
#endif
    } else {
#ifdef USE_GLFW
        if (useGL && glfwWindow) {
            glfwSwapBuffers(glfwWindow);
        }
#endif
    }
}

void Viewport3D::DrawPlayer(double x, double y, double z) {
    if (usingSDL) {
#ifdef USE_SDL
        if (useGL) {
            SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix();
            glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
            glBegin(GL_QUADS);
            glVertex3f(-0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);
            glVertex3f(0.5f, -0.5f, -0.5f);
            glVertex3f(-0.5f, -0.5f, 0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, -0.5f, 0.5f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);
            glEnd();
            glPopMatrix();
        } else {
            int px = static_cast<int>(((x + 5.0) / 10.0) * width);
            int py = height / 2;
            SDL_Rect rect{px - 5, py - 5, 10, 10};
            SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, 255);
            compat_RenderFillRect(sdlRenderer, &rect);
        }
#else
        (void)x; (void)y; (void)z;
#endif
    }
#ifdef USE_GLFW
    else if (useGL && glfwWindow) {
        glfwMakeContextCurrent(glfwWindow);
        glColor3f(1.0f, 1.0f, 0.0f);
        glPushMatrix();
        glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
        glBegin(GL_QUADS);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glEnd();
        glPopMatrix();
    }
#endif
    else {
        std::cout << "Drawing ASCII fallback for player at " << x << std::endl;
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
    if (usingSDL) {
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
                if (resourceManager->GetSpriteInfo(textureHandle, info)) {
                    int frame = currentFrame;
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
    } else if (useGL && glfwWindow) {
        // For GLFW, draw 3D cube like DrawPlayer
        glfwMakeContextCurrent(glfwWindow);
        glColor3f(1.0f, 0.5f, 0.0f); // Orange for entities
        glPushMatrix();
        glTranslatef((GLfloat)t.x, (GLfloat)t.y, (GLfloat)t.z);
        // Draw cube
        glBegin(GL_QUADS);
        // Front
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        // Back
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        // Left
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        // Right
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        // Top
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        // Bottom
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glEnd();
        glPopMatrix();
#endif
    } else {
        DrawPlayer(t.x, t.y, t.z);
    }
}

void Viewport3D::Resize(int w, int h) {
    width = w; height = h;
    std::cout << "Viewport3D Resized to " << width << "x" << height << std::endl;
}

void Viewport3D::Shutdown() {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (particleRenderer_) {
        particleRenderer_->Cleanup();
        particleRenderer_.reset();
    }
#endif
    if (usingSDL) {
#ifdef USE_SDL
        if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
        if (useGL && sdlGLContext) {
            compat_GL_DeleteContext(sdlGLContext);
            sdlGLContext = nullptr;
            useGL = false;
        }
        if (sdlWindow) SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
#endif
        usingSDL = false;
    } else {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwDestroyWindow(glfwWindow);
            glfwWindow = nullptr;
        }
        glfwTerminate();
        useGL = false;
#endif
    }
}

void Viewport3D::DrawCoordinateSystem() {
    if (usingSDL && useGL) {
#ifdef USE_SDL
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
        glDisable(GL_DEPTH_TEST); // Draw on top
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        // X-axis: red
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(5.0f, 0.0f, 0.0f);
        // Y-axis: green
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 5.0f, 0.0f);
        // Z-axis: blue
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 5.0f);
        glEnd();
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
#endif
    } else if (!usingSDL && useGL) {
#ifdef USE_GLFW
        if (glfwWindow) {
            glfwMakeContextCurrent(glfwWindow);
            glDisable(GL_DEPTH_TEST); // Draw on top
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            // X-axis: red
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(5.0f, 0.0f, 0.0f);
            // Y-axis: green
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 5.0f, 0.0f);
            // Z-axis: blue
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 5.0f);
            glEnd();
            glLineWidth(1.0f);
            glEnable(GL_DEPTH_TEST);
        }
#endif
    }
}

void Viewport3D::DrawCameraVisual(const class Camera* camera) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!camera) return;

    auto drawCameraDebug = [&]() {
        glDisable(GL_DEPTH_TEST); // Draw on top
        glPushMatrix();
        glTranslatef(camera->x(), camera->y(), camera->z());

        // Draw a much better camera visual - a small camera icon
        glLineWidth(3.0f);

        // Calculate forward direction for lens
        double yaw = camera->yaw();
        double pitch = camera->pitch();
        double cosYaw = cos(yaw);
        double sinYaw = sin(yaw);
        double cosPitch = cos(pitch);
        double sinPitch = sin(pitch);
        double forwardX = cosYaw * cosPitch;
        double forwardY = sinYaw * cosPitch;
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

        // Camera body (rectangular prism)
        glColor3f(0.8f, 0.8f, 0.8f); // Light gray body
        glBegin(GL_QUADS);
        // Front face
        glVertex3f(-0.4f, -0.2f, 0.1f);
        glVertex3f(0.4f, -0.2f, 0.1f);
        glVertex3f(0.4f, 0.2f, 0.1f);
        glVertex3f(-0.4f, 0.2f, 0.1f);
        // Back face
        glVertex3f(-0.4f, -0.2f, -0.3f);
        glVertex3f(-0.4f, 0.2f, -0.3f);
        glVertex3f(0.4f, 0.2f, -0.3f);
        glVertex3f(0.4f, -0.2f, -0.3f);
        // Left face
        glVertex3f(-0.4f, -0.2f, 0.1f);
        glVertex3f(-0.4f, 0.2f, 0.1f);
        glVertex3f(-0.4f, 0.2f, -0.3f);
        glVertex3f(-0.4f, -0.2f, -0.3f);
        // Right face
        glVertex3f(0.4f, -0.2f, 0.1f);
        glVertex3f(0.4f, -0.2f, -0.3f);
        glVertex3f(0.4f, 0.2f, -0.3f);
        glVertex3f(0.4f, 0.2f, 0.1f);
        // Top face
        glVertex3f(-0.4f, 0.2f, 0.1f);
        glVertex3f(0.4f, 0.2f, 0.1f);
        glVertex3f(0.4f, 0.2f, -0.3f);
        glVertex3f(-0.4f, 0.2f, -0.3f);
        // Bottom face
        glVertex3f(-0.4f, -0.2f, 0.1f);
        glVertex3f(-0.4f, -0.2f, -0.3f);
        glVertex3f(0.4f, -0.2f, -0.3f);
        glVertex3f(0.4f, -0.2f, 0.1f);
        glEnd();

        // Lens (circular front)
        glColor3f(0.2f, 0.2f, 0.2f); // Dark gray lens
        glBegin(GL_QUADS);
        // Approximate circle with quad
        glVertex3f(-0.15f, -0.15f, 0.11f);
        glVertex3f(0.15f, -0.15f, 0.11f);
        glVertex3f(0.15f, 0.15f, 0.11f);
        glVertex3f(-0.15f, 0.15f, 0.11f);
        glEnd();

        // Lens glass (bright center)
        glColor3f(0.9f, 0.9f, 1.0f); // Light blue-white
        glBegin(GL_QUADS);
        glVertex3f(-0.1f, -0.1f, 0.12f);
        glVertex3f(0.1f, -0.1f, 0.12f);
        glVertex3f(0.1f, 0.1f, 0.12f);
        glVertex3f(-0.1f, 0.1f, 0.12f);
        glEnd();

        // Coordinate system at camera position (world axes)
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.5f, 0.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.5f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.5f);
        glEnd();

        // Camera basis vectors
        const float vecLen = 2.5f;
        glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 0.0f); // Forward - yellow
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(static_cast<float>(forward.x * vecLen),
                   static_cast<float>(forward.y * vecLen),
                   static_cast<float>(forward.z * vecLen));
        glColor3f(0.0f, 1.0f, 1.0f); // Right - cyan
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(static_cast<float>(right.x * vecLen),
                   static_cast<float>(right.y * vecLen),
                   static_cast<float>(right.z * vecLen));
        glColor3f(1.0f, 0.0f, 1.0f); // Up - magenta
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(static_cast<float>(up.x * vecLen),
                   static_cast<float>(up.y * vecLen),
                   static_cast<float>(up.z * vecLen));
        glEnd();

        // Look-at target marker (project forward)
        Vec3 lookAt = Vec3{forward.x * 5.0, forward.y * 5.0, forward.z * 5.0};
        glColor3f(0.6f, 1.0f, 0.2f);
        glBegin(GL_LINES);
        glVertex3f(static_cast<float>(lookAt.x - 0.2), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z));
        glVertex3f(static_cast<float>(lookAt.x + 0.2), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z));
        glVertex3f(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y - 0.2), static_cast<float>(lookAt.z));
        glVertex3f(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y + 0.2), static_cast<float>(lookAt.z));
        glVertex3f(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z - 0.2));
        glVertex3f(static_cast<float>(lookAt.x), static_cast<float>(lookAt.y), static_cast<float>(lookAt.z + 0.2));
        glEnd();

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

        glColor3f(1.0f, 0.5f, 0.0f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(static_cast<float>(nearTL.x), static_cast<float>(nearTL.y), static_cast<float>(nearTL.z));
        glVertex3f(static_cast<float>(nearTR.x), static_cast<float>(nearTR.y), static_cast<float>(nearTR.z));
        glVertex3f(static_cast<float>(nearBR.x), static_cast<float>(nearBR.y), static_cast<float>(nearBR.z));
        glVertex3f(static_cast<float>(nearBL.x), static_cast<float>(nearBL.y), static_cast<float>(nearBL.z));
        glEnd();

        glBegin(GL_LINE_LOOP);
        glVertex3f(static_cast<float>(farTL.x), static_cast<float>(farTL.y), static_cast<float>(farTL.z));
        glVertex3f(static_cast<float>(farTR.x), static_cast<float>(farTR.y), static_cast<float>(farTR.z));
        glVertex3f(static_cast<float>(farBR.x), static_cast<float>(farBR.y), static_cast<float>(farBR.z));
        glVertex3f(static_cast<float>(farBL.x), static_cast<float>(farBL.y), static_cast<float>(farBL.z));
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(static_cast<float>(nearTL.x), static_cast<float>(nearTL.y), static_cast<float>(nearTL.z));
        glVertex3f(static_cast<float>(farTL.x), static_cast<float>(farTL.y), static_cast<float>(farTL.z));
        glVertex3f(static_cast<float>(nearTR.x), static_cast<float>(nearTR.y), static_cast<float>(nearTR.z));
        glVertex3f(static_cast<float>(farTR.x), static_cast<float>(farTR.y), static_cast<float>(farTR.z));
        glVertex3f(static_cast<float>(nearBL.x), static_cast<float>(nearBL.y), static_cast<float>(nearBL.z));
        glVertex3f(static_cast<float>(farBL.x), static_cast<float>(farBL.y), static_cast<float>(farBL.z));
        glVertex3f(static_cast<float>(nearBR.x), static_cast<float>(nearBR.y), static_cast<float>(nearBR.z));
        glVertex3f(static_cast<float>(farBR.x), static_cast<float>(farBR.y), static_cast<float>(farBR.z));
        glEnd();

        glPopMatrix();
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
    };

    if (usingSDL) {
#ifdef USE_SDL
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
        drawCameraDebug();
#endif
    } else if (!usingSDL && useGL) {
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
    if (!usingSDL) return;
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

void Viewport3D::DrawCameraDebug(const class Camera* camera, double playerX, double playerY, double playerZ, ViewRole role) {
#if defined(USE_GLFW) || defined(USE_SDL)
    if (!camera || role == ViewRole::Minimap) {
        return;
    }

    if (useGL) {
        glPushMatrix();
        glTranslatef(10.0f - static_cast<float>(camera->x()),
                     -static_cast<float>(camera->y()),
                     -static_cast<float>(camera->z()));
        DrawCoordinateSystem();
        glPopMatrix();

        double camDistToPlayer = std::sqrt((camera->x() - playerX) * (camera->x() - playerX) +
                                           (camera->y() - playerY) * (camera->y() - playerY) +
                                           (camera->z() - playerZ) * (camera->z() - playerZ));
        if (camDistToPlayer > 3.0) {
            DrawCameraVisual(camera);
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
    } else if (c == ' ' || c == ':') {
        // leave blank for space/colon (handled below if needed)
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
    if (!useGL || width <= 0 || height <= 0) {
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

    if (menuData.style.drawBackground) {
        const auto bg = menuData.style.backgroundColor;
        const float bgR = static_cast<float>(bg.r) / 255.0f;
        const float bgG = static_cast<float>(bg.g) / 255.0f;
        const float bgB = static_cast<float>(bg.b) / 255.0f;
        const float bgA = static_cast<float>(bg.a) / 255.0f;

        glColor4f(bgR, bgG, bgB, bgA);
        glBegin(GL_QUADS);
        glVertex2f(backgroundLeft, backgroundTop);
        glVertex2f(backgroundLeft + backgroundWidth, backgroundTop);
        glVertex2f(backgroundLeft + backgroundWidth, backgroundTop + backgroundHeight);
        glVertex2f(backgroundLeft, backgroundTop + backgroundHeight);
        glEnd();

        glColor4f(bgR, bgG, bgB, std::min(1.0f, bgA + 0.15f));
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(backgroundLeft, backgroundTop);
        glVertex2f(backgroundLeft + backgroundWidth, backgroundTop);
        glVertex2f(backgroundLeft + backgroundWidth, backgroundTop + backgroundHeight);
        glVertex2f(backgroundLeft, backgroundTop + backgroundHeight);
        glEnd();
        glLineWidth(1.0f);
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

            glColor4f(indicatorColor.r, indicatorColor.g, indicatorColor.b, indicatorColor.a);
            glBegin(GL_TRIANGLES);
            glVertex2f(leftX, indicatorY - indicatorHalf);
            glVertex2f(leftX + 12.0f, indicatorY);
            glVertex2f(leftX, indicatorY + indicatorHalf);
            glVertex2f(rightX, indicatorY - indicatorHalf);
            glVertex2f(rightX - 12.0f, indicatorY);
            glVertex2f(rightX, indicatorY + indicatorHalf);
            glEnd();

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

void Viewport3D::DrawHUD(const class Camera* camera,
                         double fps,
                         double playerX,
                         double playerY,
                         double playerZ,
                         const struct EnergyHUDTelemetry* energyTelemetry) {
    (void)camera;
    (void)fps;
    (void)playerX;
    (void)playerY;
    (void)playerZ;
    (void)energyTelemetry;
    if (!usingSDL) {
#ifdef USE_GLFW
        if (useGL && glfwWindow) {
            // GLFW OpenGL HUD drawing
            glfwMakeContextCurrent(glfwWindow);
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

            // Draw background - SMALLER BOX for better performance
            glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
            glBegin(GL_QUADS);
            glVertex2f(10, 10);
            glVertex2f(10+340, 10);  // HUD width adjusted for frame pacing info
            glVertex2f(10+340, 10+120);  // HUD height adjusted for additional rows
            glVertex2f(10, 10+80);
            glEnd();

            // Border - SMALLER BOX
            glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(10, 10);
            glVertex2f(10+340, 10);  // HUD width adjusted for frame pacing info
            glVertex2f(10+340, 10+120);  // HUD height adjusted for additional rows
            glVertex2f(10, 10+80);
            glEnd();

            // Simple 7-segment style display for GLFW (similar to SDL version)
            auto drawRect = [](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
                glColor4f(r, g, b, a);
                glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + w, y);
                glVertex2f(x + w, y + h);
                glVertex2f(x, y + h);
                glEnd();
            };

            auto drawSevenSegDigitGL = [&](int x, int y, int segLen, int segThick, char c) {
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
                    glBegin(GL_QUADS);
                    glVertex2f(sx, sy);
                    glVertex2f(sx + w, sy);
                    glVertex2f(sx + w, sy + h);
                    glVertex2f(sx, sy + h);
                    glEnd();
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
                    drawRect(x + segThick + segLen + segThick / 2, y + 2 * (segThick + segLen) + segThick, segThick, segThick, 1, 1, 1);
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
            drawRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            drawRect(x, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 14;

            // FPS value
            char fbuf[16]; 
            snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
            glColor3f(1.0f, 0.9f, 0.5f);
            for (char* p = fbuf; *p; ++p) {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                x += spacing;
            }

            x += 12;
            // Zoom label "Z:"
            glColor3f(0.7f, 0.7f, 0.7f);
            drawRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            drawRect(x + segLen - 2, y + segThick, 4, segThick, 0.7f, 0.7f, 0.7f);
            drawRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;

            // Zoom value
            char zbuf[32];
            if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); 
            else snprintf(zbuf, sizeof(zbuf), "1.0");
            glColor3f(1.0f, 0.9f, 0.5f);
            for (char* p = zbuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.');
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
                drawTinyCharGL(vsyncX, vsyncY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
                vsyncX += glyphAdvance;
            }
            vsyncX += glyphScale * 2.0f;
            const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
            glColor3f(1.0f, 0.9f, 0.5f);
            for (const char* p = vsValue; *p; ++p) {
                drawTinyCharGL(vsyncX, vsyncY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
                vsyncX += glyphAdvance;
            }

            vsyncX += glyphScale * 2.0f;
            const char* capLabel = "CAP";
            glColor3f(0.7f, 0.7f, 0.7f);
            for (const char* p = capLabel; *p; ++p) {
                drawTinyCharGL(vsyncX, vsyncY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
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
                drawTinyCharGL(vsyncX, vsyncY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
                vsyncX += glyphAdvance;
            }

            // Second row - Position
            x = 18; y += 50;
            glColor3f(0.7f, 0.7f, 0.7f);
            // X label
            drawRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            drawRect(x + 6, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;
            
            // X value
            char xbuf[32]; 
            snprintf(xbuf, sizeof(xbuf), "%.1f", playerX);
            glColor3f(0.5f, 1.0f, 1.0f);
            for (char* p = xbuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                    x += spacing / 2;
                } else if (*p == '-') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '-');
                    x += spacing;
                }
            }

            x += 12;
            // Y label
            glColor3f(0.7f, 0.7f, 0.7f);
            drawRect(x, y, 4, segThick, 0.7f, 0.7f, 0.7f);
            drawRect(x + 6, y + segThick + 2, 4, segThick, 0.7f, 0.7f, 0.7f);
            x += 18;

            // Y value
            char ybuf[32]; 
            snprintf(ybuf, sizeof(ybuf), "%.1f", playerY);
            glColor3f(0.5f, 1.0f, 1.0f);
            for (char* p = ybuf; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                    x += spacing;
                } else if (*p == '.') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                    x += spacing / 2;
                } else if (*p == '-') {
                    drawSevenSegDigitGL(x, y, segLen, segThick, '-');
                    x += spacing;
                }
            }

            // Energy management overlay
#if defined(USE_GLFW)
            if (energyTelemetry && energyTelemetry->valid) {
                RenderEnergyPanel(*energyTelemetry, width, height);
            }
#endif

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
    if (!usingSDL) return;
#ifdef USE_SDL
    if (useGL) {
        // OpenGL HUD drawing
        SDL_GL_MakeCurrent(sdlWindow, sdlGLContext);
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

        // Draw background
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(8, 8);
        glVertex2f(8+380, 8);
        glVertex2f(8+380, 8+180);
        glVertex2f(8, 8+180);
        glEnd();

        // Border
        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(8, 8);
        glVertex2f(8+380, 8);
        glVertex2f(8+380, 8+180);
        glVertex2f(8, 8+180);
        glEnd();

        // Helper functions
        auto drawRect = [](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
            glColor4f(r, g, b, a);
            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x, y + h);
            glEnd();
        };

        auto drawSevenSegDigitGL = [&](int x, int y, int segLen, int segThick, char c) {
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
                glBegin(GL_QUADS);
                glVertex2f(sx, sy);
                glVertex2f(sx + w, sy);
                glVertex2f(sx + w, sy + h);
                glVertex2f(sx, sy + h);
                glEnd();
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
                drawRect(x + segThick + segLen + segThick / 2, y + 2 * (segThick + segLen) + segThick, segThick, segThick, 1, 1, 1);
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
        drawRect(x, y, 4, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x + segLen - 2, y + segThick, 4, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.5f, 0.5f, 0.5f);
        x += 24;

        // Zoom
        char zbuf[32];
        if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); else snprintf(zbuf, sizeof(zbuf), "0.0");
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = zbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                x += spacing / 2;
            }
        }

        x += 18;
        // FPS label
        drawRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x, y + 2 * (segThick + segLen), 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = fbuf; *p; ++p) {
            drawSevenSegDigitGL(x, y, segLen, segThick, *p);
            x += spacing;
        }

        float glyphScale = 4.0f;
        float glyphAdvance = (5.0f + 1.0f) * glyphScale;
        float infoX = static_cast<float>(x + 12);
        float infoY = static_cast<float>(y);
        const char* vsLabel = "VSYNC";
        glColor3f(0.7f, 0.7f, 0.7f);
        for (const char* p = vsLabel; *p; ++p) {
            drawTinyCharGL(infoX, infoY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
            infoX += glyphAdvance;
        }
        infoX += glyphScale * 2.0f;
        const char* vsValue = vsyncEnabled_ ? "ON" : "OFF";
        glColor3f(1.0f, 0.9f, 0.5f);
        for (const char* p = vsValue; *p; ++p) {
            drawTinyCharGL(infoX, infoY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
            infoX += glyphAdvance;
        }

        infoX += glyphScale * 2.0f;
        const char* capLabel = "CAP";
        glColor3f(0.7f, 0.7f, 0.7f);
        for (const char* p = capLabel; *p; ++p) {
            drawTinyCharGL(infoX, infoY, *p, glyphScale, 0.7f, 0.7f, 0.7f);
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
            drawTinyCharGL(infoX, infoY, *p, glyphScale, 1.0f, 0.9f, 0.5f);
            infoX += glyphAdvance;
        }

        x += 18;
        // Player X label
        drawRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x + 8, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = xbuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-');
                x += spacing;
            }
        }

        // Next row
        x = 18; y += 60;
        // Player Y label
        drawRect(x, y, 6, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x + 8, y + segThick + 2, 6, segThick, 0.5f, 0.5f, 0.5f);
        x += 18;
        char ybuf[32]; snprintf(ybuf, sizeof(ybuf), "%.2f", playerY);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = ybuf; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-');
                x += spacing;
            }
        }

        x += 18;
        // Player Z label
        drawRect(x, y, 4, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x + segLen - 2, y + segThick, 4, segThick, 0.5f, 0.5f, 0.5f);
        drawRect(x, y + 2 * (segThick + segLen), 4, segThick, 0.5f, 0.5f, 0.5f);
        x += 24;
        char zbuf2[32]; snprintf(zbuf2, sizeof(zbuf2), "%.2f", playerZ);
        glColor3f(1.0f, 0.9f, 0.5f);
        for (char* p = zbuf2; *p; ++p) {
            if (*p >= '0' && *p <= '9') {
                drawSevenSegDigitGL(x, y, segLen, segThick, *p);
                x += spacing;
            } else if (*p == '.') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '.');
                x += spacing / 2;
            } else if (*p == '-') {
                drawSevenSegDigitGL(x, y, segLen, segThick, '-');
                x += spacing;
            }
        }

        // Restore
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    } else {
        // SDL renderer HUD drawing
        if (!sdlRenderer) return;
        // Draw semi-transparent background box with border
        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 180);
        SDL_Rect bg{8, 8, 380, 180};
        compat_RenderFillRect(sdlRenderer, &bg);
        // white border
        SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 180);
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
}

bool Viewport3D::CaptureToBMP(const char* path) {
#ifdef USE_SDL
    if (!usingSDL || !sdlRenderer) return false;
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
    header[0] = 'B'; header[1] = 'M';
    int fileSize = 54 + imgSize;
    header[2] = (unsigned char)(fileSize & 0xFF); header[3] = (unsigned char)((fileSize>>8) & 0xFF);
    header[4] = (unsigned char)((fileSize>>16) & 0xFF); header[5] = (unsigned char)((fileSize>>24) & 0xFF);
    header[10] = 54; header[14] = 40;
    header[18] = (unsigned char)(w & 0xFF); header[19] = (unsigned char)((w>>8) & 0xFF);
    header[22] = (unsigned char)(h & 0xFF); header[23] = (unsigned char)((h>>8) & 0xFF);
    header[26] = 1; header[28] = 24;

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

    if (!useGL) {
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
