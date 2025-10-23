#pragma once

#include "IRenderer.h"
#include "Transform.h"
#include <string>
#include <iostream>
#include <memory>
#include <vector>

class ParticleRenderer;
struct ParticleRendererDeleter {
    void operator()(ParticleRenderer* ptr) const;
};
#ifdef USE_SDL
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#elif defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#endif
#ifdef USE_GLFW
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#endif

enum class ViewRole {
    Main,
    Secondary,
    Minimap
};

struct ViewportView {
    std::string name;
    double normalizedX = 0.0;
    double normalizedY = 0.0;
    double normalizedWidth = 1.0;
    double normalizedHeight = 1.0;
    ViewRole role = ViewRole::Main;
    bool overlay = false;
};

struct ViewportLayout {
    std::string name;
    std::vector<ViewportView> views;
};

// If SDL2 is available, this class will open a window and render a simple rectangle
// representing the player. Otherwise it falls back to ASCII console output.
class Viewport3D : public IRenderer {
public:
    Viewport3D();
    ~Viewport3D();

    void Init();
    void Render(const class Camera* camera, double playerX, double playerY, double playerZ);
    void BeginFrame();
    void FinishFrame();
    void ActivateView(const class Camera* camera, double playerX, double playerY, double playerZ, size_t viewIndex);
    size_t GetActiveViewCount() const;
    ViewRole GetViewRole(size_t viewIndex) const;
    bool IsOverlayView(size_t viewIndex) const;
    void DrawMinimapOverlay(double playerX, double playerY, double playerZ);
    void Resize(int width, int height);
    // Draw a player at world x coordinate (for simple ASCII demo)
    void DrawPlayer(double x, double y = 0.0, double z = 0.0);
    // Draw a generic entity given its transform (x,y,z)
    void DrawEntity(const Transform &t);
    // Draw entity with optional texture handle (0 == none). ResourceManager is optional and used only with SDL.
    void DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, int currentFrame = 0);
    // Draw entity with camera transform
    void DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, const class Camera* camera, int currentFrame = 0);
    void Shutdown();
    
    // If OpenGL is in use this flag will be true and an OpenGL context will be owned
    bool usingGL() const { return useGL; }

    // Get the SDL window (returns nullptr if SDL is unavailable)
#ifdef USE_SDL
    SDL_Window* GetSDLWindow() const { return sdlWindow; }
#else
    void* GetSDLWindow() const { return nullptr; }
#endif

    // Get the GLFW window (returns nullptr if not using GLFW)
#ifdef USE_GLFW
    GLFWwindow* GetGLFWWindow() const { return glfwWindow; }
#else
    void* GetGLFWWindow() const { return nullptr; }
#endif

    // Draw 3D coordinate system axes
    void DrawCoordinateSystem();
    // Draw visual representation of camera position and orientation
    void DrawCameraVisual(const class Camera* camera);
    // Draw a small crosshair at camera center for debugging
    void DrawCameraMarker(const class Camera* camera);
    void DrawCameraDebug(const class Camera* camera, double playerX, double playerY, double playerZ, ViewRole role);
    // Draw HUD (zoom, fps, player x)
    void DrawHUD(const class Camera* camera, double fps, double playerX, double playerY, double playerZ);
    // Extended HUD with additional parameters
    void DrawHUD(const class Camera* camera, double fps, double playerX, double playerY, double playerZ, bool, const class ShipAssemblyResult*);

    // Bloom and letterbox settings (stubs for now)
    bool IsBloomEnabled() const { return false; }
    void SetBloomEnabled(bool) {}
    bool IsLetterboxEnabled() const { return false; }
    void SetLetterboxEnabled(bool) {}

    void SetVSyncEnabled(bool enabled);
    bool IsVSyncEnabled() const { return vsyncEnabled_; }
    void SetFramePacingHint(bool vsyncEnabled, double fps);
    double FrameRateLimitHint() const { return frameRateLimitHint_; }

    // Render particles
    void RenderParticles(const class Camera*, const class VisualFeedbackSystem*);

    // Capture current renderer contents to a simple 24-bit BMP file (path)
    bool CaptureToBMP(const char* path);

    // IRenderer interface
    void Clear() override;
    void Present() override;

    void ConfigureLayouts(std::vector<ViewportLayout> layouts);
    void CycleLayout();
    void SetActiveLayout(size_t index);
    const ViewportLayout& GetActiveLayout() const;
    std::string GetActiveLayoutName() const;
    static std::vector<ViewportLayout> CreateDefaultLayouts();

private:
    int width;
    int height;
    bool usingSDL;
    bool useGL;
    bool vsyncEnabled_;
    double frameRateLimitHint_;
#ifdef USE_SDL
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    // OpenGL context (SDL_GLContext is a typedef provided by SDL for OpenGL)
    SDL_GLContext sdlGLContext;
#endif
#ifdef USE_GLFW
    GLFWwindow* glfwWindow;
#endif
    std::unique_ptr<ParticleRenderer, ParticleRendererDeleter> particleRenderer_;
    std::vector<ViewportLayout> layouts_;
    size_t activeLayoutIndex_;

    void EnsureLayoutConfiguration();
    void ApplyViewportView(const ViewportView& view);
    void ResetViewport();
    void RenderOpenGLViews(const class Camera* camera, double playerX, double playerY, double playerZ);
    void ActivateOpenGLView(const ViewportView& view, const class Camera* camera, double playerX, double playerY, double playerZ);
    void ActivateSDLView(const ViewportView& view);
};
