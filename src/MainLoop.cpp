#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

MainLoop::MainLoop() : running(false), version("1.0.0"), viewport(nullptr), simulation(nullptr) {}

MainLoop::~MainLoop() {
    Shutdown();
    // unique_ptr members will clean up automatically
}

void MainLoop::Init() {
    std::cout << "Star Engine Initializing..." << std::endl;
    running = true;
    Input::Init();

    viewport = std::make_unique<Viewport3D>();
    viewport->Init();

    // Camera
    camera = std::make_unique<Camera>(0.0, 0.0, 32.0); // zoom maps world units to pixels roughly

    // Create canonical ECS manager and initialize simulation with it
    entityManager = std::make_unique<EntityManager>();
    simulation = std::make_unique<Simulation>();
    simulation->Init(entityManager.get());

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

    EntityData demoEntity1;
    demoEntity1.name = "demo1";
    demoEntity1.transform.x = -2.0;
    demoEntity1.textureHandle = demoHandle1;
    // Register demo entity in legacy scene storage removed; create ECS entity instead
    // Also register this scene entity in the ECS so rendering can read components
    {
        Entity e = entityManager->CreateEntity();
        auto p = std::make_shared<Position>(); p->x = demoEntity1.transform.x; p->y = demoEntity1.transform.y;
        entityManager->AddComponent<Position>(e, p);
        auto s = std::make_shared<Sprite>(); s->textureHandle = demoHandle1; s->frame = 0;
        entityManager->AddComponent<Sprite>(e, s);
    }

    EntityData demoEntity2;
    demoEntity2.name = "demo2";
    demoEntity2.transform.x = 2.0;
    demoEntity2.textureHandle = demoHandle2;
    // Register demo entity in ECS
    {
        Entity e = entityManager->CreateEntity();
        auto p = std::make_shared<Position>(); p->x = demoEntity2.transform.x; p->y = demoEntity2.transform.y;
        entityManager->AddComponent<Position>(e, p);
        auto s = std::make_shared<Sprite>(); s->textureHandle = demoHandle2; s->frame = 0;
        entityManager->AddComponent<Sprite>(e, s);
    }

    // SceneManager removed; demo entities live in ECS directly.
}

void MainLoop::MainLoopFunc(int maxSeconds) {
    if (!running) { std::cout << "Engine not initialized!" << std::endl; return; }

    using clock = std::chrono::high_resolution_clock;
    const double updateHz = 60.0;
    const std::chrono::duration<double> fixedDt(1.0 / updateHz);
    const double maxFPS = 240.0;
    const std::chrono::duration<double> minFrameTime(1.0 / maxFPS);
    std::cout << "Star Engine Fixed-Timestep Main Loop (update @ " << updateHz << " Hz)" << std::endl;

    auto demoStart = clock::now();
    auto previous = demoStart;
    std::chrono::duration<double> lag(0);
    int frames = 0;
    auto fpsTimer = demoStart;
    double currentFPS = 0.0;

    bool paused = false;
    bool holdLeft = false;
    bool holdRight = false;

    while (true) {
        auto current = clock::now();
        auto elapsed = current - previous;
        previous = current;
        lag += elapsed;

        int key = Input::PollKey();
        if (key != -1) {
            if (key == 'q' || key == 'Q') break;
            if (key == ' ') paused = !paused;
            if (key == 'a' || key == 'A') holdLeft = !holdLeft;
            if (key == 'd' || key == 'D') holdRight = !holdRight;
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
        }

        if (simulation) simulation->SetPlayerInput(holdLeft, holdRight);

        if (!paused) {
            while (lag >= fixedDt) {
                if (simulation) simulation->Update(fixedDt.count());
                // SceneManager removed; no per-frame scene update needed here.
                lag -= fixedDt;
            }
        }

        if (viewport) {
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            // Clear once per frame
            viewport->Clear();
            viewport->DrawPlayer(playerX);
            // Render all entities from scene
            {
                // Advance animations and render
                // Smoothly update camera to follow player (both X and Y)
                if (camera && simulation) {
                    double px = simulation->GetPlayerX();
                    double py = 0.0; // simulation has no Y in this demo
                    // compute an alpha based on fixed timestep to create smoothing
                    double alpha = std::min(1.0, 5.0 * fixedDt.count());
                    camera->LerpTo(px, py, alpha);
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
                    Transform t; t.x = pos->x; t.y = pos->y; t.z = 0.0;
                    viewport->DrawEntity(t, th, resourceManager.get(), camera.get(), spr->frame);
                }
                // Draw camera debug marker on top and HUD
                if (viewport && camera) {
                    // Ensure smooth zoom is updated each frame
                    camera->UpdateZoom(fixedDt.count());
                    viewport->DrawCameraMarker(camera.get());
                    // Draw HUD with currentFPS
                    double hudPlayerX = simulation ? simulation->GetPlayerX() : 0.0;
                    viewport->DrawHUD(camera.get(), currentFPS, hudPlayerX);
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
            frames = 0;
            fpsTimer = current;
        }

        if (maxSeconds > 0) {
            if (std::chrono::duration<double>(current - demoStart).count() >= maxSeconds) break;
        } else {
            break;
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
