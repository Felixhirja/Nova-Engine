#pragma once

#include "IRenderer.h"
#include "Transform.h"
#include <string>
#include <iostream>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

// If SDL2 is available, this class will open a window and render a simple rectangle
// representing the player. Otherwise it falls back to ASCII console output.
class Viewport3D : public IRenderer {
public:
    Viewport3D();
    ~Viewport3D();

    void Init();
    void Render();
    void Resize(int width, int height);
    // Draw a player at world x coordinate (for simple ASCII demo)
    void DrawPlayer(double x);
    // Draw a generic entity given its transform (x,y,z)
    void DrawEntity(const Transform &t);
    // Draw entity with optional texture handle (0 == none). ResourceManager is optional and used only with SDL.
    void DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, int currentFrame = 0);
    // Draw entity with camera transform
    void DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, const class Camera* camera, int currentFrame = 0);
    void Shutdown();

    // Draw a small crosshair at camera center for debugging
    void DrawCameraMarker(const class Camera* camera);
    // Draw HUD (zoom, fps, player x)
    void DrawHUD(const class Camera* camera, double fps, double playerX);
    // Capture current renderer contents to a simple 24-bit BMP file (path)
    bool CaptureToBMP(const char* path);

    // IRenderer interface
    void Clear() override;
    void Present() override;

private:
    int width;
    int height;
    bool usingSDL;
#ifdef USE_SDL
    SDL_Window* sdlWindow;
    SDL_Renderer* sdlRenderer;
#endif
};
