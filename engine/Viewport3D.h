#pragma once

#include "IRenderer.h"
#include "MainMenu.h"
#include "Transform.h"
#include "EnergyHUDTelemetry.h"
#include "ecs/EntityManager.h"
#include "graphics/UIBatcher.h"
#include "graphics/LineBatcher3D.h"
#include "graphics/MaterialLibrary.h"
#include "graphics/InstancedMeshRenderer.h"
#include "graphics/ActorRenderer.h"
#include "Mesh.h"
#if (defined(USE_GLFW) || defined(USE_SDL))
#include "graphics/PrimitiveMesh.h"
#include "graphics/MeshSubmission.h"
#endif
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>

enum class RenderBackend {
    None,
    SDL_GL,
    SDL_Renderer,
    GLFW_GL
};

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
    struct EntityMeshBinding {
        Mesh mesh;
        float scale = 1.0f;
        std::unique_ptr<PrimitiveMesh> primitive;
        bool primitiveDirty = true;
    };

    Viewport3D();
    ~Viewport3D();

    void Init();
    void Render(const class Camera* camera, double playerX, double playerY, double playerZ, bool targetLocked = false, ecs::EntityManagerV2* entityManager = nullptr);
    void BeginFrame();
    void FinishFrame();
    void ActivateView(const class Camera* camera, double playerX, double playerY, double playerZ, size_t viewIndex);
    void ConfigureLayouts(std::vector<ViewportLayout> layouts);
    void CycleLayout();
    void SetActiveLayout(size_t index);
    const ViewportLayout& GetActiveLayout() const;
    std::string GetActiveLayoutName() const;
    static std::vector<ViewportLayout> CreateDefaultLayouts();
    size_t GetActiveViewCount() const;
    ViewRole GetViewRole(size_t viewIndex) const;
    bool IsOverlayView(size_t viewIndex) const;
    void DrawMinimapOverlay(double playerX, double playerY, double playerZ);
    void Resize(int width, int height);
    // Draw a player at world x coordinate (for simple ASCII demo)
    void DrawPlayer(double x, double y = 0.0, double z = 0.0);
    // Draw a generic entity given its transform (x,y,z)
    void DrawEntity(const Transform &t);
    void DrawEntity(Entity entity, const Transform& t);
    // Allow callers to override the mesh/scale used for a specific entity ID
    void SetEntityMesh(Entity entity, Mesh mesh, float scale = 1.0f);
    void ClearEntityMesh(Entity entity);
    void ClearEntityMeshes();
    // Factory for the default stylized player avatar mesh
    static Mesh CreatePlayerAvatarMesh();
    void Shutdown();
    
    // If OpenGL is in use this flag will be true and an OpenGL context will be owned
    bool usingGL() const { return IsUsingGLBackend(); }

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
    void DrawCameraVisual(const class Camera* camera, double playerX = 0.0, double playerY = 0.0, double playerZ = 0.0, bool targetLocked = false);
    // Draw a small crosshair at camera center for debugging
    void DrawCameraMarker(const class Camera* camera);
    void DrawCameraDebug(const class Camera* camera, double playerX, double playerY, double playerZ, ViewRole role, bool targetLocked = false);
    // Draw HUD (zoom, fps, player x)
    void DrawHUD(const class Camera* camera,
                 double fps,
                 double playerX,
                 double playerY,
                 double playerZ,
                 const struct EnergyHUDTelemetry* energyTelemetry = nullptr);
    // Extended HUD with additional parameters
    void DrawHUD(const class Camera* camera,
                 double fps,
                 double playerX,
                 double playerY,
                 double playerZ,
                 bool,
                 const class ShipAssemblyResult*,
                 const struct EnergyHUDTelemetry* energyTelemetry = nullptr);

    void ToggleFullscreen();
    bool IsFullscreen() const { return isFullscreen_; }

    // Overlay rendering
    void RenderMenuOverlay(const MainMenu::RenderData& menuData);

    // Bloom and letterbox settings (stubs for now)
    bool IsBloomEnabled() const { return false; }
    void SetBloomEnabled(bool) {}
    bool IsLetterboxEnabled() const { return false; }
    void SetLetterboxEnabled(bool) {}

    // Small helper: show brief HUD usage hints (F8/F9)
    void SetShowHudHints(bool v) { showHudHints_ = v; }

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

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    // Expose UI batcher for external overlays (read-only pointer; may be null if not using GL)
    UIBatcher* GetUIBatcher() const { return uiBatcher_.get(); }

#ifndef NDEBUG
    // Debug toggles for axes rendering (no-op in Release)
    void ToggleWorldAxes();
    void ToggleMiniAxesGizmo();
    bool IsWorldAxesShown() const;
    bool IsMiniAxesGizmoShown() const;
#endif

private:
    int width;
    int height;
    RenderBackend backend_;
    bool vsyncEnabled_;
    double frameRateLimitHint_;
    bool debugLogging_; // Control verbose debug output
    bool aggressiveFocus_; // Control aggressive window focus behavior on Windows
#ifdef USE_SDL
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
    // OpenGL context (SDL_GLContext is a typedef provided by SDL for OpenGL)
    SDL_GLContext sdlGLContext;
    SDL_Texture* spaceshipHudTexture_;
    int spaceshipHudTextureWidth_;
    int spaceshipHudTextureHeight_;
    bool spaceshipHudTextureFailed_;
#endif
#ifdef USE_GLFW
    GLFWwindow* glfwWindow;
#endif
    std::unique_ptr<ParticleRenderer, ParticleRendererDeleter> particleRenderer_;
    std::unique_ptr<ActorRenderer> actorRenderer_;
    std::vector<ViewportLayout> layouts_;
    size_t activeLayoutIndex_;
    std::unique_ptr<UIBatcher> uiBatcher_;
#if defined(USE_GLFW) || defined(USE_SDL)
    unsigned int playerHudTextureGL_;
    int playerHudTextureGLWidth_;
    int playerHudTextureGLHeight_;
    bool playerHudTextureGLFailed_;
    Mesh playerMesh_;
    bool playerMeshInitialized_ = false;
    bool isFullscreen_ = false;
    int windowedPosX_ = 100;
    int windowedPosY_ = 100;
    int windowedWidth_ = 800;
    int windowedHeight_ = 600;
#endif
    std::unique_ptr<LineBatcher3D> lineBatcher3D_;
    struct PrimitiveBuffers;
    std::unique_ptr<PrimitiveBuffers> primitiveBuffers_;
    // Material system
    std::unique_ptr<Nova::MaterialLibrary> materialLibrary_;
    std::unique_ptr<Nova::InstancedMeshRenderer> instancedRenderer_;
    // Retained primitive meshes (migration target for immediate-mode draws)
    std::unique_ptr<PrimitiveMesh> cubePrimitive_;
    std::unique_ptr<PrimitiveMesh> playerPatchPrimitive_;
    std::unique_ptr<PrimitiveMesh> hudTexturePrimitive_;
    std::unique_ptr<PrimitiveMesh> playerMeshPrimitive_;
    bool playerMeshPrimitiveDirty_ = true;
    float hudTextureLastX_ = 0.0f;
    float hudTextureLastY_ = 0.0f;
    float hudTextureLastWidth_ = 0.0f;
    float hudTextureLastHeight_ = 0.0f;
    bool hudTexturePrimitiveDirty_ = true;
    bool showHudHints_ = false;

    void EnsureLayoutConfiguration();
    void ApplyViewportView(const ViewportView& view);
    void ResetViewport();
    void RenderOpenGLViews(const class Camera* camera, double playerX, double playerY, double playerZ);
    void ActivateOpenGLView(const ViewportView& view, const class Camera* camera, double playerX, double playerY, double playerZ);
    void ActivateSDLView(const ViewportView& view);
#ifdef USE_SDL
    void EnsureSpaceshipHudTexture();
#endif
#if defined(USE_GLFW) || defined(USE_SDL)
    void EnsurePlayerHudTextureGL();
#endif
    void SetBackend(RenderBackend backend);
    bool IsUsingSDLBackend() const;
    bool IsUsingSDLGL() const;
    bool IsUsingSDLRenderer() const;
    bool IsUsingGLFWBackend() const;
    bool IsUsingGLBackend() const;
    void EnsurePrimitiveBuffers();
    void EnsureCubePrimitive();
    void EnsurePlayerPatchPrimitive();
    void EnsureHudTexturePrimitive(float x, float y, float width, float height);
    void DestroyPrimitiveBuffers();
    void DrawPlayerPatchPrimitive();
    void DrawCubePrimitive(float r, float g, float b);
    void DrawHudTextureOverlay(unsigned int texture, float x, float y, float width, float height);
    void EnsurePlayerMesh();
    PrimitiveMesh* EnsureMeshPrimitive(const Mesh& mesh,
                                       std::unique_ptr<PrimitiveMesh>& cache,
                                       bool& dirtyFlag);
    void DrawMeshAt(double x,
                    double y,
                    double z,
                    const Mesh* meshOverride,
                    EntityMeshBinding* overrideBinding,
                    float scale,
                    char asciiChar);
        void DrawStaticGrid();

    // Ensure line batcher exists and initialized
    void EnsureLineBatcher3D();

    // HUD sampling state for velocity estimation
    double lastHudX_ = 0.0;
    double lastHudY_ = 0.0;
    double lastHudZ_ = 0.0;
    std::chrono::steady_clock::time_point lastHudTime_{};
    bool haveHudSample_ = false;

    // Compute speed estimate in world units per second; stores last sample
    double SampleSpeed(double x, double y, double z);

    std::unordered_map<Entity, EntityMeshBinding> entityMeshes_;
};
