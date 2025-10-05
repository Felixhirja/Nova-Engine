#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <cmath>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

MainLoop::MainLoop() : running(false), version("1.0.0"), viewport(nullptr), simulation(nullptr) {}

MainLoop::~MainLoop() {
    Shutdown();
    // unique_ptr members will clean up automatically
}

void MainLoop::Init() {
    std::cout << "Star Engine Initializing..." << std::endl;
    // Log to file
    std::ofstream log("sdl_diag.log", std::ios::app);
    log << "MainLoop::Init started" << std::endl;
    log.close();
    running = true;
    Input::Init();

    viewport = std::make_unique<Viewport3D>();
    viewport->Init();
    std::cout << "Viewport3D::Init() completed" << std::endl;

    // Set GLFW window for input handling
    Input::SetGLFWWindow(viewport->GetGLFWWindow());

    // Camera
    camera = std::make_unique<Camera>(0.0, 0.0, 5.0, 0.0, 0.0, 32.0); // fixed camera position at (0,0,5), zoom maps world units to pixels roughly

    // Create canonical ECS manager and initialize simulation with it
    entityManager = std::make_unique<EntityManager>();
    simulation = std::make_unique<Simulation>();
    simulation->Init(entityManager.get());
    std::cout << "Simulation::Init() completed" << std::endl;

    // Resource manager & demo entity (use unique_ptr ownership)
    resourceManager = std::make_unique<ResourceManager>();

    // Ensure assets directory exists and write tiny BMPs (16x16) - two colors
    system("mkdir -p assets/sprites >/dev/null 2>&1 || true");
    const char *demoPath1 = "assets/sprites/demo1.bmp";
    const char *demoPath2 = "assets/sprites/demo2.bmp";
    // Write two small 16x16 BMPs (24-bit) directly so we don't depend on external tools
    auto write_bmp = [](const char* path, unsigned char r, unsigned char g, unsigned char b){
        const int w = 16, h = 16;
        const int rowBytes = ((w*3 + 3) / 4) * 4;
        int imgSize = rowBytes * h;
        unsigned char header[54] = {0};
        // BMP header
        header[0] = 'B'; header[1] = 'M';
        int fileSize = 54 + imgSize;
        header[2] = fileSize & 0xFF; header[3] = (fileSize>>8) & 0xFF; header[4] = (fileSize>>16) & 0xFF; header[5] = (fileSize>>24) & 0xFF;
        header[10] = 54;
        // DIB header
        header[14] = 40;
        header[18] = w & 0xFF; header[19] = (w>>8) & 0xFF;
        header[22] = h & 0xFF; header[23] = (h>>8) & 0xFF;
        header[26] = 1; header[28] = 24;

        FILE* f = fopen(path, "wb");
        if (!f) return;
        fwrite(header, 1, 54, f);
        unsigned char* row = new unsigned char[rowBytes];
        for (int y = 0; y < h; ++y) {
            int idx = 0;
            for (int x = 0; x < w; ++x) {
                // BMP stores BGR
                row[idx++] = b;
                row[idx++] = g;
                row[idx++] = r;
            }
            // padding
            while (idx < rowBytes) row[idx++] = 0;
            fwrite(row, 1, rowBytes, f);
        }
        delete[] row;
        fclose(f);
    };

    // Create sprite-sheet images 32x16 (two 16x16 frames side-by-side)
    auto write_sheet = [&](const char* path, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2){
        const int fw = 16, fh = 16; const int w = fw*2, h = fh;
        const int rowBytes = ((w*3 + 3) / 4) * 4;
        int imgSize = rowBytes * h;
        unsigned char header[54] = {0};
        header[0] = 'B'; header[1] = 'M';
        int fileSize = 54 + imgSize;
        header[2] = fileSize & 0xFF; header[3] = (fileSize>>8) & 0xFF; header[4] = (fileSize>>16) & 0xFF; header[5] = (fileSize>>24) & 0xFF;
        header[10] = 54; header[14] = 40; header[18] = w & 0xFF; header[19] = (w>>8) & 0xFF; header[22] = h & 0xFF; header[23] = (h>>8) & 0xFF; header[26] = 1; header[28] = 24;
        FILE* f = fopen(path, "wb"); if (!f) return;
        fwrite(header, 1, 54, f);
        unsigned char* row = new unsigned char[rowBytes];
        for (int y = 0; y < h; ++y) {
            int idx = 0;
            for (int x = 0; x < w; ++x) {
                bool left = (x < fw);
                if (left) { row[idx++] = b1; row[idx++] = g1; row[idx++] = r1; }
                else { row[idx++] = b2; row[idx++] = g2; row[idx++] = r2; }
            }
            while (idx < rowBytes) row[idx++] = 0;
            fwrite(row, 1, rowBytes, f);
        }
        delete[] row; fclose(f);
    };

    write_sheet(demoPath1, 0x4a,0x90,0xe2, 0xff,0xff,0x00);
    write_sheet(demoPath2, 0xe9,0x4a,0x4a, 0x4a,0xff,0x4a);

    int demoHandle1 = resourceManager->Load(std::string(demoPath1));
    int demoHandle2 = resourceManager->Load(std::string(demoPath2));

    // Register sprite metadata: frameW=16, frameH=16, frames=2, fps=2
    ResourceManager::SpriteInfo info; info.frameW = 16; info.frameH = 16; info.frames = 2; info.fps = 2;
    resourceManager->RegisterSprite(demoHandle1, info);
    resourceManager->RegisterSprite(demoHandle2, info);

    // Register demo entities in ECS
    {
        Entity e = entityManager->CreateEntity();
        auto p = std::make_shared<Position>(); p->x = -2.0; p->y = 0.0; p->z = 0.0;
        entityManager->AddComponent<Position>(e, p);
        auto s = std::make_shared<Sprite>(); s->textureHandle = demoHandle1; s->frame = 0;
        entityManager->AddComponent<Sprite>(e, s);
    }

    {
        Entity e = entityManager->CreateEntity();
        auto p = std::make_shared<Position>(); p->x = 2.0; p->y = 0.0; p->z = 0.0;
        entityManager->AddComponent<Position>(e, p);
        auto s = std::make_shared<Sprite>(); s->textureHandle = demoHandle2; s->frame = 0;
        entityManager->AddComponent<Sprite>(e, s);
    }

    // SceneManager removed; demo entities live in ECS directly.
}

void MainLoop::MainLoopFunc(int maxSeconds) {
    std::cout << "MainLoopFunc started" << std::endl;
    // Log to file - REMOVED: was causing FPS drops
    // std::ofstream log("sdl_diag.log", std::ios::app);
    // log << "MainLoopFunc started" << std::endl;
    // log.close();
    if (!running) { std::cout << "Engine not initialized!" << std::endl; return; }

    using clock = std::chrono::high_resolution_clock;
    const double updateHz = 60.0;
    const std::chrono::duration<double> fixedDt(1.0 / updateHz);
    const double maxFPS = 144.0;  // INCREASED to 144 to allow higher FPS
    const std::chrono::duration<double> minFrameTime(1.0 / maxFPS);
    std::cout << "Star Engine Fixed-Timestep Main Loop (update @ " << updateHz << " Hz)" << std::endl;

    auto demoStart = clock::now();
    auto previous = demoStart;
    std::chrono::duration<double> lag(0);
    int frames = 0;
    auto fpsTimer = demoStart;
    double currentFPS = 0.0;

    bool paused = false;
    const double turnSpeed = 2.0; // radians per second

    while (true) {
        // Log to file - REMOVED: was causing massive FPS drops
        // std::ofstream log("sdl_diag.log", std::ios::app);
        // log << "MainLoopFunc loop iteration" << std::endl;
        // log.close();
        auto current = clock::now();
        auto elapsed = current - previous;
        previous = current;
        lag += elapsed;

        // Get keyboard state using Input class
        // Get relative mouse movement for camera look
        double mouseDeltaX = 0.0, mouseDeltaY = 0.0;
#ifdef USE_GLFW
        if (viewport && viewport->GetGLFWWindow()) {
            GLFWwindow* window = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            double centerX = width / 2.0;
            double centerY = height / 2.0;
            // Calculate delta from center position
            mouseDeltaX = x - centerX;
            mouseDeltaY = y - centerY;
            // Always recenter cursor for proper relative movement
            glfwSetCursorPos(window, centerX, centerY);
        }
#endif
#ifdef USE_SDL
        int sdlMouseDeltaX, sdlMouseDeltaY;
        SDL_GetRelativeMouseState(&sdlMouseDeltaX, &sdlMouseDeltaY);
        mouseDeltaX = sdlMouseDeltaX;
        mouseDeltaY = sdlMouseDeltaY;
#endif

        // Update input state for hold-based controls
        Input::UpdateKeyState();

        // Check for quit key (still poll-based for immediate response)
        int key = Input::PollKey();
        if (key != -1) {
            std::cout << "Key pressed: " << key << " ('" << (char)key << "')" << std::endl;
        }
        if (key == 'q' || key == 'Q' || Input::IsKeyHeld('q') || Input::IsKeyHeld('Q') || key == 27) {
            std::cout << "Quit key detected, exiting..." << std::endl;
            break;
        }

        // Handle pause (still toggle-based)
        if (key == 'p' || key == 'P') paused = !paused;

        // Handle zoom keys (still poll-based for immediate response)
        if ((key == 'z' || key == 'Z') && camera) {
            double z = camera->targetZoom();
            z *= 1.1;
            if (z > 128.0) z = 128.0;
            camera->SetTargetZoom(z);
        }
        if ((key == 'x' || key == 'X') && camera) {
            double z = camera->targetZoom();
            z /= 1.1;
            if (z < 4.0) z = 4.0;
            camera->SetTargetZoom(z);
        }

        // Set player input based on held keys using Input class
        bool strafeLeft = Input::IsKeyHeld('a') || Input::IsKeyHeld('A');
        bool strafeRight = Input::IsKeyHeld('d') || Input::IsKeyHeld('D');
        bool forward = Input::IsKeyHeld('w') || Input::IsKeyHeld('W');
        bool backward = Input::IsKeyHeld('s') || Input::IsKeyHeld('S');
        bool up = Input::IsKeyHeld('e') || Input::IsKeyHeld('E');
        bool down = Input::IsKeyHeld('c') || Input::IsKeyHeld('C');

        // Camera movement with arrow keys
        bool cameraForward = Input::IsArrowKeyHeld(GLFW_KEY_UP);
        bool cameraBackward = Input::IsArrowKeyHeld(GLFW_KEY_DOWN);
        bool cameraLeft = Input::IsArrowKeyHeld(GLFW_KEY_LEFT);
        bool cameraRight = Input::IsArrowKeyHeld(GLFW_KEY_RIGHT);
        bool cameraUp = Input::IsKeyHeld(' ') && Input::IsKeyHeld(GLFW_KEY_UP); // Space + Up for camera up
        bool cameraDown = Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_DOWN); // Space + Down for camera down

        if (simulation) simulation->SetPlayerInput(forward, backward, up, down, strafeLeft, strafeRight, camera->yaw());

        // Apply camera movement
        if (camera) {
            double cameraMoveSpeed = 0.5; // units per second
            double deltaTime = fixedDt.count();
            double pi = acos(-1.0);
            
            if (cameraForward) {
                double dx = sin(camera->yaw()) * cameraMoveSpeed * deltaTime;
                double dy = -cos(camera->yaw()) * cameraMoveSpeed * deltaTime;
                camera->SetPosition(camera->x() + dx, camera->y() + dy, camera->z());
            }
            if (cameraBackward) {
                double dx = -sin(camera->yaw()) * cameraMoveSpeed * deltaTime;
                double dy = cos(camera->yaw()) * cameraMoveSpeed * deltaTime;
                camera->SetPosition(camera->x() + dx, camera->y() + dy, camera->z());
            }
            if (cameraLeft) {
                double dx = sin(camera->yaw() - pi/2) * cameraMoveSpeed * deltaTime;
                double dy = -cos(camera->yaw() - pi/2) * cameraMoveSpeed * deltaTime;
                camera->SetPosition(camera->x() + dx, camera->y() + dy, camera->z());
            }
            if (cameraRight) {
                double dx = sin(camera->yaw() + pi/2) * cameraMoveSpeed * deltaTime;
                double dy = -cos(camera->yaw() + pi/2) * cameraMoveSpeed * deltaTime;
                camera->SetPosition(camera->x() + dx, camera->y() + dy, camera->z());
            }
            if (cameraUp) {
                camera->SetPosition(camera->x(), camera->y(), camera->z() + cameraMoveSpeed * deltaTime);
            }
            if (cameraDown) {
                camera->SetPosition(camera->x(), camera->y(), camera->z() - cameraMoveSpeed * deltaTime);
            }
        }

        if (!paused) {
            while (lag >= fixedDt) {
                if (simulation) simulation->Update(fixedDt.count());
                // SceneManager removed; no per-frame scene update needed here.
                lag -= fixedDt;
            }
        }

        if (viewport) {
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            double playerY = simulation ? simulation->GetPlayerY() : 0.0;
            double playerZ = simulation ? simulation->GetPlayerZ() : 0.0;
            // Clear once per frame
            viewport->Clear();
            // Set up 3D rendering with camera looking at player
            viewport->Render(camera.get(), playerX, playerY, playerZ);
            // Draw player at its world position
            viewport->DrawPlayer(playerX, playerY, playerZ);
            // Render all entities from scene
            {
                // Advance animations and render
                // Free look camera: update yaw and pitch based on mouse movement
                if (camera) {
                    // Mouse look (always active in relative mode)
                    double mouseSensitivity = 0.002;
                    double currentYaw = camera->yaw();
                    double currentPitch = camera->pitch();
                    currentYaw += mouseDeltaX * mouseSensitivity;
                    currentPitch += mouseDeltaY * mouseSensitivity;
                    // Clamp pitch to prevent flipping
                    if (currentPitch > std::acos(-1.0)/2 - 0.1) currentPitch = std::acos(-1.0)/2 - 0.1;
                    if (currentPitch < -std::acos(-1.0)/2 + 0.1) currentPitch = -std::acos(-1.0)/2 + 0.1;
                    
                    camera->SetOrientation(currentPitch, currentYaw);
                }

                // Render from ECS: iterate all entities that have a Sprite component
                auto sprites = entityManager->GetAllWith<Sprite>();
                for (auto &kv : sprites) {
                    Entity ent = kv.first;
                    Sprite* spr = kv.second;
                    if (!spr) continue;
                    Position* pos = entityManager->GetComponent<Position>(ent);
                    if (!pos) continue;
                    int th = spr->textureHandle;
                    if (th != 0 && resourceManager) {
                        ResourceManager::SpriteInfo info;
                        if (resourceManager->GetSpriteInfo(th, info)) {
                            // advance frame by fixed timestep
                            spr->frame = (spr->frame + 1) % std::max(1, info.frames);
                            Transform t; t.x = pos->x; t.y = pos->y; t.z = 0.0;
                            viewport->DrawEntity(t, th, resourceManager.get(), camera.get(), spr->frame);
                            continue;
                        }
                    }
                    Transform t; t.x = pos->x; t.y = pos->y; t.z = pos->z;
                    viewport->DrawEntity(t, th, resourceManager.get(), camera.get(), spr->frame);
                }
                // Draw camera debug marker on top and HUD
                if (viewport && camera) {
                    // Ensure smooth zoom is updated each frame
                    camera->UpdateZoom(fixedDt.count());
                    viewport->DrawCameraMarker(camera.get());
                    // Draw HUD with currentFPS - RE-ENABLED
                    double hudPlayerX = simulation ? simulation->GetPlayerX() : 0.0;
                    double hudPlayerY = simulation ? simulation->GetPlayerY() : 0.0;
                    double hudPlayerZ = simulation ? simulation->GetPlayerZ() : 0.0;
                    viewport->DrawHUD(camera.get(), currentFPS, hudPlayerX, hudPlayerY, hudPlayerZ);
                    // If STAR_CAPTURE env var is set, dump the renderer contents to BMP for inspection
                    const char* cap = std::getenv("STAR_CAPTURE");
                    if (cap && std::string(cap) == "1") {
                        viewport->CaptureToBMP("/workspaces/Star-Engine/renderer_capture.bmp");
                    }
                    // present the final frame (marker and HUD included)
                    viewport->Present();
                }
            }
        }
        ++frames;

        if (std::chrono::duration<double>(current - fpsTimer).count() >= 1.0) {
            double simPos = simulation ? simulation->GetPosition() : 0.0;
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            std::cout << "FPS: " << frames << "  Simulation pos=" << simPos << "  Player x=" << playerX << "  Zoom=" << (camera?camera->zoom():1.0) << std::endl;
            currentFPS = frames;  // Update currentFPS for HUD display
            frames = 0;
            fpsTimer = current;
        }

        if (maxSeconds > 0) {
            if (std::chrono::duration<double>(current - demoStart).count() >= maxSeconds) break;
        }

        auto frameEnd = clock::now();
        auto frameTime = frameEnd - current;
        if (frameTime < minFrameTime) std::this_thread::sleep_for(minFrameTime - frameTime);
    }

    Input::Shutdown();
}

void MainLoop::Shutdown() {
    if (running) { std::cout << "Star Engine Shutting down..." << std::endl; running = false; }
    if (viewport) { viewport->Shutdown(); }
    // unique_ptr will free sceneManager
}

std::string MainLoop::GetVersion() const { return version; }
