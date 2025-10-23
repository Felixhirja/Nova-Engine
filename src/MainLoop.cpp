#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Spaceship.h"
#include "Camera.h"
#include "CameraPresets.h"
#include "CameraFollow.h"
#include "GamepadManager.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include "ecs/ECSInspector.h"
#include "VisualFeedbackSystem.h"
#include "AudioFeedbackSystem.h"
#include "HUDAlertSystem.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <iomanip>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace {

ShipAssemblyResult BuildHudAssembly() {
    ShipAssemblyResult result;

    const ShipHullBlueprint* fighterHull = ShipHullCatalog::Find("fighter_mk1");
    if (!fighterHull) {
        std::cerr << "HUD assembly: failed to locate default fighter hull blueprint" << std::endl;
        return result;
    }

    ShipAssemblyRequest request;
    request.hullId = fighterHull->id;

    auto assignComponent = [](const HullSlot& slot) -> std::string {
        switch (slot.category) {
            case ComponentSlotCategory::PowerPlant:
                return "fusion_core_mk1";
            case ComponentSlotCategory::MainThruster:
                return "main_thruster_viper";
            case ComponentSlotCategory::ManeuverThruster:
                return "rcs_cluster_micro";
            case ComponentSlotCategory::Shield:
                return "shield_array_light";
            case ComponentSlotCategory::Weapon:
                return "weapon_cooling_cannon";
            case ComponentSlotCategory::Sensor:
                return "sensor_targeting_mk1";
            case ComponentSlotCategory::Support:
                return "support_life_pod";
            default:
                return std::string();
        }
    };

    for (const auto& slot : fighterHull->slots) {
        std::string componentId = assignComponent(slot);
        if (componentId.empty()) {
            std::cerr << "HUD assembly: missing default component mapping for slot "
                      << slot.slotId << " (" << ToString(slot.category) << ")" << std::endl;
            continue;
        }
        request.slotAssignments[slot.slotId] = componentId;
    }

    result = ShipAssembler::Assemble(request);

    std::cout << std::fixed << std::setprecision(2);
    if (!result.IsValid()) {
        std::cerr << "HUD assembly: assembly produced " << result.diagnostics.errors.size() << " error(s)" << std::endl;
        for (const auto& err : result.diagnostics.errors) {
            std::cerr << "  -> " << err << std::endl;
        }
    } else {
        std::cout << "HUD assembly: "
                  << (result.hull ? result.hull->displayName : std::string("Unknown Hull"))
                  << " | Net Power " << result.NetPowerMW() << " MW"
                  << " | Thrust/Mass " << result.ThrustToMassRatio()
                  << std::endl;
        if (!result.diagnostics.warnings.empty()) {
            std::cout << "  Warnings:" << std::endl;
            for (const auto& warn : result.diagnostics.warnings) {
                std::cout << "    * " << warn << std::endl;
            }
        }
    }
    std::cout.unsetf(std::ios_base::floatfield);
    return result;
}

} // namespace

MainLoop::MainLoop()
    : running(false)
    , version("1.0.0")
    , viewport(nullptr)
    , simulation(nullptr)
    , mouseLookYawOffset(0.0)
    , mouseLookPitchOffset(0.0)
    , cameraFollowConfig()
    , cameraFollowState()
    , cameraPresets(GetDefaultCameraPresets()) {
    ecsInspector = std::make_unique<ECSInspector>();
}

MainLoop::~MainLoop() {
    Shutdown();
    // unique_ptr members will clean up automatically
}

void MainLoop::Init() {
    std::cout << "DEBUG: MainLoop::Init() STARTED" << std::endl;
    std::cerr << "DEBUG: MainLoop::Init() STARTED (cerr)" << std::endl;
    // Log to file
    std::ofstream log("sdl_diag.log", std::ios::app);
    log << "MainLoop::Init started" << std::endl;
    log.close();
    running = true;
    Input::Init();

    {
        auto& gamepadManager = GamepadManager::Instance();
        bool xinputReady = gamepadManager.EnsureInitialized();

        std::ofstream diagLog("sdl_diag.log", std::ios::app);
        if (diagLog) {
            diagLog << "GamepadManager: attempt="
                    << (gamepadManager.HasAttemptedInitialization() ? "true" : "false")
                    << ", available=" << (xinputReady ? "true" : "false");
            if (xinputReady) {
                diagLog << ", library=" << gamepadManager.ActiveLibraryNameUtf8();
            } else if (!gamepadManager.LastError().empty()) {
                diagLog << ", error=" << gamepadManager.LastError();
            }
            diagLog << std::endl;
        }

        if (xinputReady) {
            std::cout << "GamepadManager: XInput available via "
                      << gamepadManager.ActiveLibraryNameUtf8() << std::endl;
        } else {
            const std::string errorDescription = gamepadManager.LastError();
            if (!errorDescription.empty()) {
                std::cout << "GamepadManager: XInput unavailable (" << errorDescription << ")" << std::endl;
            } else {
                std::cout << "GamepadManager: XInput unavailable" << std::endl;
            }
        }
    }

    viewport = std::make_unique<Viewport3D>();
    viewport->Init();
    std::cout << "Viewport3D::Init() completed" << std::endl;

    // Set up GLFW window resize callback
    std::cout << "Setting up GLFW window resize callback" << std::endl;
#ifdef USE_GLFW
    if (viewport->GetGLFWWindow()) {
        glfwSetWindowSizeCallback(static_cast<GLFWwindow*>(viewport->GetGLFWWindow()), 
            [](GLFWwindow* window, int width, int height) {
                // Get the viewport instance from the MainLoop (stored in user pointer)
                MainLoop* mainLoop = static_cast<MainLoop*>(glfwGetWindowUserPointer(window));
                if (mainLoop) {
                    Viewport3D* viewport = mainLoop->GetViewport();
                    if (viewport) {
                        viewport->Resize(width, height);
                    }
                }
            });
        // Store a pointer to this MainLoop instance in the GLFW window for the callback
        glfwSetWindowUserPointer(static_cast<GLFWwindow*>(viewport->GetGLFWWindow()), this);
    }
#endif

    // Set GLFW window for input handling
    std::cout << "About to set GLFW window for input" << std::endl;
#ifdef USE_GLFW
    void* glfwWindowPtr = viewport->GetGLFWWindow();
    std::cout << "GLFW window pointer: " << glfwWindowPtr << std::endl;
    if (glfwWindowPtr != nullptr) {
        Input::SetGLFWWindow(glfwWindowPtr);
        std::cout << "GLFW window set for input" << std::endl;
    } else {
        std::cout << "GLFW window is null (headless mode?), skipping input setup" << std::endl;
    }
#endif
    
    // Set SDL window for input handling (if available)
#ifdef USE_SDL
    Input::SetSDLWindow(viewport->GetSDLWindow());
#endif

    std::cout << "About to create camera" << std::endl;
    // Camera
    // Position: behind player at (-8, 0, 6), looking toward origin (0,0,0)
    // Yaw of PI/2 (90 degrees) makes camera look in +X direction (toward player)
    camera = std::make_unique<Camera>(-8.0, 0.0, 6.0, -0.1, 1.5708, 12.0); // yaw = π/2 to look toward player

    std::cout << "About to create entity manager" << std::endl;
    // Create canonical ECS manager and initialize simulation with it
    entityManager = std::make_unique<EntityManager>();
    if (!ecsInspector) {
        ecsInspector = std::make_unique<ECSInspector>();
    }
    ecsInspector->SetEntityManager(entityManager.get());
    simulation = std::make_unique<Simulation>();
    std::cout << "About to call simulation->Init()" << std::endl;
    simulation->Init(entityManager.get());
    std::cout << "Simulation::Init() completed" << std::endl;

    // Resource manager & demo entity (use unique_ptr ownership)
    resourceManager = std::make_unique<ResourceManager>();

    // Initialize feedback systems
    std::cout << "Initializing feedback systems..." << std::endl;
    visualFeedbackSystem = std::make_unique<VisualFeedbackSystem>();
    audioFeedbackSystem = std::make_unique<AudioFeedbackSystem>();
    hudAlertSystem = std::make_unique<HUDAlertSystem>();
    std::cout << "Feedback systems initialized" << std::endl;

    // Preload spaceship taxonomy for downstream systems and log summary
    const auto& shipClasses = SpaceshipCatalog::All();
    std::cout << "SpaceshipCatalog: loaded " << shipClasses.size() << " class definitions" << std::endl;
    for (const auto& def : shipClasses) {
        std::cout << "  -> " << def.displayName << ": " << def.conceptSummary.elevatorPitch << std::endl;
    }

    hudShipAssembly = BuildHudAssembly();

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
    std::cout << "Nova Engine Fixed-Timestep Main Loop (update @ " << updateHz << " Hz)" << std::endl;

    auto demoStart = clock::now();
    auto previous = demoStart;
    std::chrono::duration<double> lag(0);
    int frames = 0;
    auto fpsTimer = demoStart;
    double currentFPS = 0.0;

    bool paused = false;
    const double turnSpeed = 2.0; // radians per second

    // Check for headless mode with frame limit
    const char* headlessEnv = std::getenv("NOVA_ENGINE_HEADLESS");
    bool headlessMode = (headlessEnv != nullptr && std::string(headlessEnv) == "1");
    int maxFrames = -1; // -1 means unlimited
    if (headlessMode) {
        const char* maxFramesEnv = std::getenv("NOVA_ENGINE_MAX_FRAMES");
        if (maxFramesEnv != nullptr) {
            maxFrames = std::atoi(maxFramesEnv);
            std::cout << "Headless mode: will run for " << maxFrames << " frames then exit" << std::endl;
        } else {
            maxFrames = 300; // Default: 10 seconds at 30fps
            std::cout << "Headless mode: will run for " << maxFrames << " frames (default) then exit" << std::endl;
        }
    }
    int frameCount = 0;

    while (true) {
        // Check frame limit in headless mode
        if (headlessMode && maxFrames > 0 && frameCount >= maxFrames) {
            std::cout << "Headless mode: reached " << frameCount << " frames, exiting..." << std::endl;
            break;
        }
        frameCount++;
        
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
            // Only process mouse input if window has focus
            if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
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
        }
#endif
#ifdef USE_SDL
        // SDL relative mouse mode should work regardless of focus, but let's be safe
        if (viewport && viewport->GetSDLWindow()) {
            SDL_Window* window = static_cast<SDL_Window*>(viewport->GetSDLWindow());
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) {
                int sdlMouseDeltaX, sdlMouseDeltaY;
                SDL_GetRelativeMouseState(&sdlMouseDeltaX, &sdlMouseDeltaY);
                mouseDeltaX = sdlMouseDeltaX;
                mouseDeltaY = sdlMouseDeltaY;
            }
        }
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

        if (key == '1' || key == '2' || key == '3') {
            size_t presetIndex = static_cast<size_t>(key - '1');
            ApplyCameraPreset(presetIndex);
        }

        static bool thrustModeEnabled = false;
        if ((key == 't' || key == 'T') && simulation) {
            thrustModeEnabled = !thrustModeEnabled;
            simulation->SetUseThrustMode(thrustModeEnabled);
            std::cout << "Player vertical mode: " << (thrustModeEnabled ? "thrust" : "jump") << std::endl;
        }

        // Handle target lock toggle (Tab key)
        if (key == 9) { // Tab key
            // Toggle target lock on the player entity
            if (entityManager && simulation) {
                auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
                if (targetLock) {
                    targetLock->isLocked = !targetLock->isLocked;
                    if (targetLock->isLocked) {
                        std::cout << "Target lock enabled" << std::endl;
                    } else {
                        std::cout << "Target lock disabled" << std::endl;
                    }
                }
            }
        }

        // Handle post-process effect toggles
        if ((key == 'b' || key == 'B') && viewport) {
            bool bloomEnabled = viewport->IsBloomEnabled();
            viewport->SetBloomEnabled(!bloomEnabled);
            std::cout << "Bloom effect: " << (!bloomEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }

        if ((key == 'l' || key == 'L') && viewport) {
            bool letterboxEnabled = viewport->IsLetterboxEnabled();
            viewport->SetLetterboxEnabled(!letterboxEnabled);
            std::cout << "Letterbox overlay: " << (!letterboxEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }

        if ((key == 'i' || key == 'I') && ecsInspector) {
            ecsInspector->Toggle();
            std::cout << "ECS inspector: " << (ecsInspector->IsEnabled() ? "ENABLED" : "DISABLED") << std::endl;
        }

        if (ecsInspector && ecsInspector->IsEnabled()) {
            if (key == '[' || key == '{') {
                ecsInspector->PreviousFilter();
            } else if (key == ']' || key == '}') {
                ecsInspector->NextFilter();
            } else if (key == '0' || key == ')') {
                ecsInspector->ClearFilter();
            }
        }

        // Set player input based on held keys using Input class
        bool strafeLeft = Input::IsKeyHeld('a') || Input::IsKeyHeld('A');
        bool strafeRight = Input::IsKeyHeld('d') || Input::IsKeyHeld('D');
        bool forward = Input::IsKeyHeld('w') || Input::IsKeyHeld('W');
        bool backward = Input::IsKeyHeld('s') || Input::IsKeyHeld('S');
        bool up = Input::IsKeyHeld(' '); // Space for up
        bool down = Input::IsKeyHeld('c') || Input::IsKeyHeld('C'); // C for down

        // Camera movement with arrow keys
#ifdef USE_GLFW
        bool cameraForward = Input::IsArrowKeyHeld(GLFW_KEY_UP);
        bool cameraBackward = Input::IsArrowKeyHeld(GLFW_KEY_DOWN);
        bool cameraLeft = Input::IsArrowKeyHeld(GLFW_KEY_LEFT);
        bool cameraRight = Input::IsArrowKeyHeld(GLFW_KEY_RIGHT);
        bool cameraUp = Input::IsKeyHeld(' ') && Input::IsKeyHeld(GLFW_KEY_UP); // Space + Up for camera up
        bool cameraDown = Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_DOWN); // Space + Down for camera down
#else
        bool cameraForward = false;
        bool cameraBackward = false;
        bool cameraLeft = false;
        bool cameraRight = false;
        bool cameraUp = false;
        bool cameraDown = false;
#endif

        // Check if target lock is enabled on the player
        bool isTargetLocked = false;
        if (entityManager && simulation) {
            auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
            if (targetLock) {
                isTargetLocked = targetLock->isLocked;
            }
        }

        // Set player input - always use world-relative movement (yaw = 0.0)
        double playerInputYaw = 0.0;
        if (simulation) simulation->SetPlayerInput(forward, backward, up, down, strafeLeft, strafeRight, playerInputYaw);

        // Apply camera movement or target lock
        if (camera) {
            double cameraMoveSpeed = 0.5; // units per second
            double deltaTime = fixedDt.count();

            CameraFollowInput followInput;
            followInput.playerX = simulation ? simulation->GetPlayerX() : 0.0;
            followInput.playerY = simulation ? simulation->GetPlayerY() : 0.0;
            followInput.playerZ = simulation ? simulation->GetPlayerZ() : 0.0;
            followInput.isTargetLocked = isTargetLocked;
            followInput.mouseLookYawOffset = mouseLookYawOffset;
            followInput.mouseLookPitchOffset = mouseLookPitchOffset;

            CameraFollow::UpdateTargetLockCamera(*camera, cameraFollowState, cameraFollowConfig, followInput, deltaTime);

            // Free camera mode (only when fully out of target lock)
            if (!isTargetLocked && cameraFollowState.targetLockTransition <= 0.0) {
                // Free camera mode: handle arrow key movement (world space)
                double newCameraX = camera->x();
                double newCameraY = camera->y();
                double newCameraZ = camera->z();
                
                if (cameraForward) {
                    newCameraY += cameraMoveSpeed * deltaTime;
                }
                if (cameraBackward) {
                    newCameraY -= cameraMoveSpeed * deltaTime;
                }
                if (cameraLeft) {
                    newCameraX -= cameraMoveSpeed * deltaTime;
                }
                if (cameraRight) {
                    newCameraX += cameraMoveSpeed * deltaTime;
                }
                if (cameraUp) {
                    newCameraZ += cameraMoveSpeed * deltaTime;
                }
                if (cameraDown) {
                    newCameraZ -= cameraMoveSpeed * deltaTime;
                }
                
                // Apply collision avoidance to free camera
                double playerX = followInput.playerX;
                double playerY = followInput.playerY;
                double playerZ = followInput.playerZ;
                
                // Minimum distance from player in free camera mode
                double minDistanceFromPlayer = 1.0;
                double freeCamToPlayerX = newCameraX - playerX;
                double freeCamToPlayerY = newCameraY - playerY;
                double freeCamToPlayerZ = newCameraZ - playerZ;
                double distanceFromPlayer = sqrt(freeCamToPlayerX * freeCamToPlayerX + freeCamToPlayerY * freeCamToPlayerY + freeCamToPlayerZ * freeCamToPlayerZ);
                
                if (distanceFromPlayer < minDistanceFromPlayer) {
                    // Push camera away from player
                    double pushFactor = minDistanceFromPlayer / distanceFromPlayer;
                    newCameraX = playerX + freeCamToPlayerX * pushFactor;
                    newCameraY = playerY + freeCamToPlayerY * pushFactor;
                    newCameraZ = playerZ + freeCamToPlayerZ * pushFactor;
                }
                
                // Ground collision for free camera
                double groundLevel = 0.5;
                if (newCameraZ < groundLevel) {
                    newCameraZ = groundLevel;
                }
                
                camera->SetPosition(newCameraX, newCameraY, newCameraZ);
            }
        }

        if (!paused) {
            while (lag >= fixedDt) {
                if (simulation) simulation->Update(fixedDt.count());
                // Update visual feedback system (particle physics)
                if (visualFeedbackSystem) {
                    visualFeedbackSystem->Update(fixedDt.count());
                }
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
                    // Enhanced mouse controls with configurable sensitivity
                    double baseMouseSensitivity = 0.002;
                    double mouseSensitivity = baseMouseSensitivity;
                    
                    // Handle mouse wheel for zoom
                    double wheelDelta = Input::GetMouseWheelDelta();
                    if (wheelDelta != 0.0) {
                        double zoomFactor = 1.0 + (wheelDelta * 0.1);
                        double newZoom = camera->targetZoom() * zoomFactor;
                        // Clamp zoom to reasonable limits
                        if (newZoom > 128.0) newZoom = 128.0;
                        if (newZoom < 4.0) newZoom = 4.0;
                        camera->SetTargetZoom(newZoom);
                        Input::ResetMouseWheelDelta(); // Reset after processing
                    }
                    
                    if (isTargetLocked) {
                        // Enhanced target lock mouse controls with improved sensitivity curves
                        double targetLockSensitivity = 0.004;
                        double mouseDecay = 0.96; // Slightly faster decay for more responsive feel
                        double accelerationFactor = 1.0;
                        
                        // Adaptive sensitivity based on mouse movement speed
                        double mouseSpeed = sqrt(mouseDeltaX * mouseDeltaX + mouseDeltaY * mouseDeltaY);
                        if (mouseSpeed > 5.0) {
                            accelerationFactor = 1.0 + (mouseSpeed - 5.0) * 0.02; // Accelerate for fast movements
                        }
                        
                        // Apply mouse input with improved acceleration
                        if (abs(mouseDeltaX) > 0.1 || abs(mouseDeltaY) > 0.1) {
                            mouseLookYawOffset += mouseDeltaX * targetLockSensitivity * accelerationFactor;
                            mouseLookPitchOffset += mouseDeltaY * targetLockSensitivity * accelerationFactor;
                        } else {
                            // Smooth decay when mouse is still
                            mouseLookYawOffset *= mouseDecay;
                            mouseLookPitchOffset *= mouseDecay;
                        }
                        
                        // Improved clamping with orbital limits and smooth boundaries
                        const double PI = acos(-1.0);
                        const double maxYawOffset = PI * 2.0; // Allow full orbit
                        const double maxPitchOffset = PI / 3.0; // 60 degrees up
                        const double minPitchOffset = -PI / 2.5; // -72 degrees down
                        
                        // Smooth clamping to prevent harsh stops
                        double clampFactor = 0.1;
                        if (mouseLookYawOffset > maxYawOffset) {
                            mouseLookYawOffset = maxYawOffset + (mouseLookYawOffset - maxYawOffset) * (1.0 - clampFactor);
                        }
                        if (mouseLookYawOffset < -maxYawOffset) {
                            mouseLookYawOffset = -maxYawOffset + (mouseLookYawOffset + maxYawOffset) * (1.0 - clampFactor);
                        }
                        if (mouseLookPitchOffset > maxPitchOffset) {
                            mouseLookPitchOffset = maxPitchOffset + (mouseLookPitchOffset - maxPitchOffset) * (1.0 - clampFactor);
                        }
                        if (mouseLookPitchOffset < minPitchOffset) {
                            mouseLookPitchOffset = minPitchOffset + (mouseLookPitchOffset - minPitchOffset) * (1.0 - clampFactor);
                        }
                    } else {
                        // Free camera mode: direct mouse look with enhanced sensitivity
                        double currentYaw = camera->yaw();
                        double currentPitch = camera->pitch();
                        
                        // Adaptive sensitivity for free camera
                        double freeCameraSensitivity = mouseSensitivity * 1.2; // Slightly more sensitive in free camera
                        double freeAccelerationFactor = 1.0;
                        
                        double mouseSpeed = sqrt(mouseDeltaX * mouseDeltaX + mouseDeltaY * mouseDeltaY);
                        if (mouseSpeed > 3.0) {
                            freeAccelerationFactor = 1.0 + (mouseSpeed - 3.0) * 0.015;
                        }
                        
                        currentYaw += mouseDeltaX * freeCameraSensitivity * freeAccelerationFactor;
                        currentPitch += mouseDeltaY * freeCameraSensitivity * freeAccelerationFactor;
                        
                        // Smooth pitch clamping to prevent flipping
                        const double PI = acos(-1.0);
                        double maxPitch = PI/2 - 0.1;
                        double minPitch = -PI/2 + 0.1;
                        
                        if (currentPitch > maxPitch) currentPitch = maxPitch;
                        if (currentPitch < minPitch) currentPitch = minPitch;
                        
                        camera->SetOrientation(currentPitch, currentYaw);
                        
                        // Reset mouse look offsets when not in target lock
                        mouseLookYawOffset *= 0.95; // Gradual reset
                        mouseLookPitchOffset *= 0.95;
                    }
                }

                // Ensure sprite metadata buffer is uploaded once when updated
#ifdef USE_GLFW
                if (resourceManager) {
                    resourceManager->SyncSpriteMetadataGPU();
                }
#endif

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
                    // Get target lock state from player component
                    bool hudTargetLocked = isTargetLocked;
                    const ShipAssemblyResult* hudAssemblyPtr = nullptr;
                    if (hudShipAssembly.hull || !hudShipAssembly.diagnostics.errors.empty() || !hudShipAssembly.diagnostics.warnings.empty()) {
                        hudAssemblyPtr = &hudShipAssembly;
                    }
                    viewport->DrawHUD(camera.get(), currentFPS, hudPlayerX, hudPlayerY, hudPlayerZ, hudTargetLocked, hudAssemblyPtr);
                    // Render particle effects (sparks, explosions, shield impacts)
                    if (visualFeedbackSystem) {
                        viewport->RenderParticles(camera.get(), visualFeedbackSystem.get());
                    }
                    if (ecsInspector) {
                        ecsInspector->Render(*viewport);
                    }
                    // If STAR_CAPTURE env var is set, dump the renderer contents to BMP for inspection
                    const char* cap = std::getenv("STAR_CAPTURE");
                    if (cap && std::string(cap) == "1") {
                        viewport->CaptureToBMP("/workspaces/Nova-Engine/renderer_capture.bmp");
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
            double cameraX = camera ? camera->x() : 0.0;
            double cameraY = camera ? camera->y() : 0.0;
            double cameraZ = camera ? camera->z() : 0.0;
            std::cout << "FPS: " << frames << "  Simulation pos=" << simPos << "  Player x=" << playerX 
                      << "  Camera: (" << cameraX << ", " << cameraY << ", " << cameraZ << ")" 
                      << "  Zoom=" << (camera?camera->zoom():1.0) << std::endl;
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

void MainLoop::ApplyCameraPreset(size_t index) {
    if (!camera || index >= cameraPresets.size()) {
        return;
    }

    ApplyPresetToCamera(*camera, cameraPresets[index]);

    // Reset offsets and target lock smoothing so preset takes effect immediately
    mouseLookYawOffset = 0.0;
    mouseLookPitchOffset = 0.0;
    cameraFollowState.targetLockTransition = 0.0;
    cameraFollowState.wasTargetLocked = false;

    // Ensure target lock component is disabled when jumping to a preset
    if (entityManager && simulation) {
        auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
        if (targetLock) {
            targetLock->isLocked = false;
        }
    }

    std::cout << "Camera preset " << (index + 1) << " applied" << std::endl;
}

void MainLoop::Shutdown() {
    if (running) { std::cout << "Nova Engine Shutting down..." << std::endl; running = false; }
    if (viewport) { viewport->Shutdown(); }
    if (ecsInspector) {
        ecsInspector->SetEntityManager(nullptr);
    }
    // unique_ptr will free sceneManager
}

std::string MainLoop::GetVersion() const { return version; }
