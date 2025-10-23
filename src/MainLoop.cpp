#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include "ResourceManager.h"
#include "Spaceship.h"
#include "EnergyManagementSystem.h"
#include "Camera.h"
#include "CameraPresets.h"
#include "GamepadManager.h"
#include "EngineBootstrap.h"
#include "FrameScheduler.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include "ecs/ECSInspector.h"
#include "VisualFeedbackSystem.h"
#include "AudioFeedbackSystem.h"
#include "HUDAlertSystem.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <utility>
#include <cstdlib>
#include <string>
#include <sstream>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace {

struct FrameRuntimeContext {
    bool headlessMode = false;
    int maxFrames = -1;
    int frameCount = 0;
    bool requestExit = false;
    bool headlessNoticePrinted = false;
    int maxSeconds = 0;
    std::chrono::high_resolution_clock::time_point demoStart;
    std::chrono::high_resolution_clock::time_point fpsTimer;
    int framesThisSecond = 0;
    double currentFPS = 0.0;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
    int lastKey = -1;
    bool targetLocked = false;
};

} // namespace

MainLoop::MainLoop()
    : running(false)
    , version("1.0.0")
    , viewport(nullptr)
    , simulation(nullptr)
    , thrustModeEnabled(false)
    , mouseLookYawOffset(0.0)
    , mouseLookPitchOffset(0.0)
    , cameraPresets(GetDefaultCameraPresets()) {
    ecsInspector = std::make_unique<ECSInspector>();
    currentState_ = GameState::PLAYING;  // Start directly in playing mode for debugging
    mainMenu_.SetActive(false);  // Don't show main menu
    mainMenu_.ClearLastAction();
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
    stateMachine.TransitionTo(EngineState::Bootstrapping);

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

    if (viewport) {
        viewport->ConfigureLayouts(Viewport3D::CreateDefaultLayouts());
        viewport->SetFramePacingHint(framePacingController.IsVSyncEnabled(), framePacingController.TargetFPS());
    }

    cameraFollowController.SetConfig(CameraFollowConfig{});
    cameraFollowController.ResetState();

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
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
        glfwSetWindowUserPointer(glfwWindow, this);

        glfwSetKeyCallback(glfwWindow,
            [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                MainLoop* mainLoop = static_cast<MainLoop*>(glfwGetWindowUserPointer(window));
                if (mainLoop) {
                    mainLoop->HandleKeyEvent(key, scancode, action, mods);
                }
            });

        glfwSetMouseButtonCallback(glfwWindow,
            [](GLFWwindow* window, int button, int action, int mods) {
                MainLoop* mainLoop = static_cast<MainLoop*>(glfwGetWindowUserPointer(window));
                if (mainLoop) {
                    mainLoop->HandleMouseButtonEvent(button, action, mods);
                }
            });

        glfwSetCursorPosCallback(glfwWindow,
            [](GLFWwindow* window, double xpos, double ypos) {
                MainLoop* mainLoop = static_cast<MainLoop*>(glfwGetWindowUserPointer(window));
                if (mainLoop) {
                    mainLoop->HandleCursorPosEvent(xpos, ypos);
                }
            });

        glfwSetWindowCloseCallback(glfwWindow,
            [](GLFWwindow* window) {
                MainLoop* mainLoop = static_cast<MainLoop*>(glfwGetWindowUserPointer(window));
                if (mainLoop) {
                    mainLoop->RequestShutdown();
                }
            });
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
    camera = std::make_unique<Camera>(-8.0, 0.0, 6.0, -0.1, 1.5708, 45.0); // yaw = π/2 to look toward player

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
    // Bootstrap demo content and HUD assembly
    EngineBootstrap bootstrap;
    auto bootstrapResult = bootstrap.Run(*resourceManager, *entityManager);
    hudShipAssembly = std::move(bootstrapResult.hudAssembly);
    ConfigureEnergyTelemetry();

    stateMachine.TransitionTo(EngineState::Running);
}


void MainLoop::MainLoopFunc(int maxSeconds) {
    std::cout << "MainLoopFunc started" << std::endl;
    if (!running) {
        std::cout << "Engine not initialized!" << std::endl;
        return;
    }

    using clock = std::chrono::high_resolution_clock;

    FrameRuntimeContext runtime;
    runtime.maxSeconds = maxSeconds;
    runtime.demoStart = clock::now();
    runtime.fpsTimer = runtime.demoStart;

    const double updateHz = 60.0;
    const double fixedDt = 1.0 / updateHz;
    const double maxFPS = 144.0;

    FrameSchedulerConfig schedulerConfig;
    schedulerConfig.fixedUpdateHz = updateHz;
    schedulerConfig.maxRenderHz = maxFPS;

    FrameScheduler scheduler(schedulerConfig);

    const char* headlessEnv = std::getenv("NOVA_ENGINE_HEADLESS");
    runtime.headlessMode = (headlessEnv != nullptr && std::string(headlessEnv) == "1");
    if (runtime.headlessMode) {
        const char* maxFramesEnv = std::getenv("NOVA_ENGINE_MAX_FRAMES");
        if (maxFramesEnv != nullptr) {
            runtime.maxFrames = std::atoi(maxFramesEnv);
            std::cout << "Headless mode: will run for " << runtime.maxFrames << " frames then exit" << std::endl;
        } else {
            runtime.maxFrames = 300;
            std::cout << "Headless mode: will run for " << runtime.maxFrames << " frames (default) then exit" << std::endl;
        }
    }

    FrameSchedulerCallbacks callbacks;

    callbacks.shouldContinue = [&]() {
        return running && !runtime.requestExit && !stateMachine.Is(EngineState::ShuttingDown);
    };

    callbacks.onFrameStart = [&](double deltaSeconds) {
        runtime.mouseDeltaX = 0.0;
        runtime.mouseDeltaY = 0.0;

#ifdef USE_GLFW
        // Poll GLFW events to process window input (mouse clicks, key presses, etc.)
        if (viewport && viewport->GetGLFWWindow()) {
            glfwPollEvents();
        }
#endif

        auto requestShutdown = [&]() {
            if (!runtime.requestExit) {
                runtime.requestExit = true;
                stateMachine.TransitionTo(EngineState::ShuttingDown);
            }
        };

        Input::UpdateKeyState();

        if (currentState_ == GameState::MAIN_MENU) {
            int menuKey = Input::PollKey();
            if (menuKey != -1) {
                mainMenu_.HandleKeyPress(menuKey);
            }

            mainMenu_.Update(deltaSeconds);

            MainMenu::Action action = mainMenu_.GetLastAction();
            if (action == MainMenu::Action::NewGame) {
                currentState_ = GameState::PLAYING;
                StartNewGame();
            } else if (action == MainMenu::Action::Continue) {
                currentState_ = GameState::PLAYING;
                LoadSavedGame();
            } else if (action == MainMenu::Action::Settings) {
                std::cout << "Settings menu not implemented yet." << std::endl;
                mainMenu_.ClearLastAction();
            } else if (action == MainMenu::Action::Quit) {
#ifdef USE_GLFW
                if (viewport && viewport->GetGLFWWindow()) {
                    glfwSetWindowShouldClose(static_cast<GLFWwindow*>(viewport->GetGLFWWindow()), GLFW_TRUE);
                }
#endif
                requestShutdown();
                mainMenu_.ClearLastAction();
            }

            return;
        }

#ifdef USE_GLFW
        if (viewport && viewport->GetGLFWWindow()) {
            GLFWwindow* window = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
            if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double centerX = width / 2.0;
                double centerY = height / 2.0;
                runtime.mouseDeltaX = x - centerX;
                runtime.mouseDeltaY = y - centerY;
                glfwSetCursorPos(window, centerX, centerY);
            }
        }
#endif
#ifdef USE_SDL
        if (viewport && viewport->GetSDLWindow()) {
            SDL_Window* window = static_cast<SDL_Window*>(viewport->GetSDLWindow());
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) {
                int sdlMouseDeltaX, sdlMouseDeltaY;
                SDL_GetRelativeMouseState(&sdlMouseDeltaX, &sdlMouseDeltaY);
                runtime.mouseDeltaX = sdlMouseDeltaX;
                runtime.mouseDeltaY = sdlMouseDeltaY;
            }
        }
#endif

        UpdateEnergyTelemetry(deltaSeconds);

        int key = Input::PollKey();
        runtime.lastKey = key;
        if (key != -1) {
            std::cout << "Key pressed: " << key << " ('" << static_cast<char>(key) << "')" << std::endl;
        }

        if (key == 'q' || key == 'Q' || Input::IsKeyHeld('q') || Input::IsKeyHeld('Q') || key == 27) {
            std::cout << "Quit key detected, exiting..." << std::endl;
            requestShutdown();
            return;
        }

        if (key == 'p' || key == 'P') {
            if (stateMachine.TogglePause()) {
                std::cout << (stateMachine.Is(EngineState::Paused) ? "Engine paused" : "Engine resumed") << std::endl;
            }
        }

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

        if ((key == 't' || key == 'T') && simulation) {
            thrustModeEnabled = !thrustModeEnabled;
            simulation->SetUseThrustMode(thrustModeEnabled);
            std::cout << "Player vertical mode: " << (thrustModeEnabled ? "thrust" : "jump") << std::endl;
        }

        if (key == 9) {
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
        bool up = Input::IsKeyHeld(' ');
        bool down = Input::IsKeyHeld('c') || Input::IsKeyHeld('C');

#ifdef USE_GLFW
        bool cameraForward = Input::IsArrowKeyHeld(GLFW_KEY_UP);
        bool cameraBackward = Input::IsArrowKeyHeld(GLFW_KEY_DOWN);
        bool cameraLeft = Input::IsArrowKeyHeld(GLFW_KEY_LEFT);
        bool cameraRight = Input::IsArrowKeyHeld(GLFW_KEY_RIGHT);
        bool cameraUp = Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_UP);
        bool cameraDown = Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_DOWN);
#else
        bool cameraForward = false;
        bool cameraBackward = false;
        bool cameraLeft = false;
        bool cameraRight = false;
        bool cameraUp = false;
        bool cameraDown = false;
#endif

        runtime.targetLocked = false;
        if (entityManager && simulation) {
            auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
            if (targetLock) {
                runtime.targetLocked = targetLock->isLocked;
            }
        }

        double playerInputYaw = 0.0;
        if (simulation) {
            simulation->SetPlayerInput(forward, backward, up, down, strafeLeft, strafeRight, playerInputYaw);
        }

        if (camera) {
            double cameraMoveSpeed = 0.5;
            double deltaTime = deltaSeconds > 0.0 ? deltaSeconds : fixedDt;

            CameraFollowInput followInput;
            followInput.playerX = simulation ? simulation->GetPlayerX() : 0.0;
            followInput.playerY = simulation ? simulation->GetPlayerY() : 0.0;
            followInput.playerZ = simulation ? simulation->GetPlayerZ() : 0.0;
            followInput.isTargetLocked = runtime.targetLocked;
            followInput.mouseLookYawOffset = mouseLookYawOffset;
            followInput.mouseLookPitchOffset = mouseLookPitchOffset;

            CameraMovementInput movementInput;
            movementInput.moveForward = cameraForward;
            movementInput.moveBackward = cameraBackward;
            movementInput.moveLeft = cameraLeft;
            movementInput.moveRight = cameraRight;
            movementInput.moveUp = cameraUp;
            movementInput.moveDown = cameraDown;
            movementInput.moveSpeed = cameraMoveSpeed;

            cameraFollowController.Update(*camera, followInput, movementInput, deltaTime);
        }
    };

    callbacks.onFixedUpdate = [&](double step) {
        if (currentState_ != GameState::PLAYING || !stateMachine.Is(EngineState::Running)) {
            return;
        }
        if (simulation) {
            simulation->Update(step);
        }
        if (visualFeedbackSystem) {
            visualFeedbackSystem->Update(step);
        }
    };

    callbacks.onRender = [&](double /*interpolation*/) {
        if (!viewport) {
            return;
        }

        if (currentState_ == GameState::MAIN_MENU) {
            viewport->Clear();
            auto renderData = mainMenu_.GetRenderData();
            viewport->RenderMenuOverlay(renderData);
            return;
        }

        double playerX = simulation ? simulation->GetPlayerX() : 0.0;
        double playerY = simulation ? simulation->GetPlayerY() : 0.0;
        double playerZ = simulation ? simulation->GetPlayerZ() : 0.0;

        viewport->Clear();
        viewport->Render(camera.get(), playerX, playerY, playerZ);
        viewport->DrawPlayer(playerX, playerY, playerZ);

        if (camera) {
            double wheelDelta = Input::GetMouseWheelDelta();
            if (wheelDelta != 0.0) {
                double zoomFactor = 1.0 + (wheelDelta * 0.1);
                double newZoom = camera->targetZoom() * zoomFactor;
                // Let Camera.cpp handle zoom clamping with its wider range (0.0001 to 10000.0)
                camera->SetTargetZoom(newZoom);
                Input::ResetMouseWheelDelta();
            }

            const double mouseDecay = 0.96;
            if (runtime.targetLocked) {
                const double targetLockSensitivity = 0.004;
                double accelerationFactor = 1.0;
                double mouseSpeed = std::sqrt(runtime.mouseDeltaX * runtime.mouseDeltaX +
                                              runtime.mouseDeltaY * runtime.mouseDeltaY);
                if (mouseSpeed > 5.0) {
                    accelerationFactor = 1.0 + (mouseSpeed - 5.0) * 0.02;
                }

                if (std::abs(runtime.mouseDeltaX) > 0.1 || std::abs(runtime.mouseDeltaY) > 0.1) {
                    mouseLookYawOffset += runtime.mouseDeltaX * targetLockSensitivity * accelerationFactor;
                    mouseLookPitchOffset += runtime.mouseDeltaY * targetLockSensitivity * accelerationFactor;

                    // Removed decay when mouse is actively moving for smoother camera control
                    // mouseLookYawOffset *= mouseDecay;
                    // mouseLookPitchOffset *= mouseDecay;
                } else {
                    mouseLookYawOffset *= mouseDecay;
                    mouseLookPitchOffset *= mouseDecay;
                }
            } else {
                mouseLookYawOffset *= mouseDecay;
                mouseLookPitchOffset *= mouseDecay;
            }

        }

        double hudPlayerX = simulation ? simulation->GetPlayerX() : 0.0;
        double hudPlayerY = simulation ? simulation->GetPlayerY() : 0.0;
        double hudPlayerZ = simulation ? simulation->GetPlayerZ() : 0.0;
        bool hudTargetLocked = runtime.targetLocked;
        const ShipAssemblyResult* hudAssemblyPtr = nullptr;
        if (hudShipAssembly.hull || !hudShipAssembly.diagnostics.errors.empty() || !hudShipAssembly.diagnostics.warnings.empty()) {
            hudAssemblyPtr = &hudShipAssembly;
        }

        viewport->DrawHUD(camera.get(), runtime.currentFPS, hudPlayerX, hudPlayerY, hudPlayerZ, hudTargetLocked, hudAssemblyPtr);
        if (visualFeedbackSystem) {
            viewport->RenderParticles(camera.get(), visualFeedbackSystem.get());
        }
        if (ecsInspector) {
            ecsInspector->Render(*viewport);
        }

        const char* cap = std::getenv("STAR_CAPTURE");
        if (cap && std::string(cap) == "1") {
            viewport->CaptureToBMP("/workspaces/Nova-Engine/renderer_capture.bmp");
        }

        viewport->FinishFrame();
        viewport->Present();
    };

    callbacks.onFrameComplete = [&](const FrameSchedulerFrameInfo& info) {
        runtime.frameCount++;
        runtime.framesThisSecond++;

        if (runtime.headlessMode && runtime.maxFrames > 0 && runtime.frameCount >= runtime.maxFrames && !runtime.headlessNoticePrinted) {
            std::cout << "Headless mode: reached " << runtime.frameCount << " frames, exiting..." << std::endl;
            runtime.headlessNoticePrinted = true;
            runtime.requestExit = true;
            stateMachine.TransitionTo(EngineState::ShuttingDown);
        }

        if (runtime.maxSeconds > 0) {
            if (std::chrono::duration<double>(info.frameEnd - runtime.demoStart).count() >= runtime.maxSeconds) {
                std::cout << "Reached max runtime of " << runtime.maxSeconds << " seconds, exiting..." << std::endl;
                runtime.requestExit = true;
                stateMachine.TransitionTo(EngineState::ShuttingDown);
            }
        }

        if (std::chrono::duration<double>(info.frameEnd - runtime.fpsTimer).count() >= 1.0) {
            double simPos = simulation ? simulation->GetPosition() : 0.0;
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            double cameraX = camera ? camera->x() : 0.0;
            double cameraY = camera ? camera->y() : 0.0;
            double cameraZ = camera ? camera->z() : 0.0;
            std::cout << "FPS: " << runtime.framesThisSecond << "  Simulation pos=" << simPos << "  Player x=" << playerX
                      << "  Camera: (" << cameraX << ", " << cameraY << ", " << cameraZ << ")"
                      << "  Zoom=" << (camera ? camera->zoom() : 1.0) << std::endl;
            runtime.currentFPS = runtime.framesThisSecond;
            runtime.framesThisSecond = 0;
            runtime.fpsTimer = info.frameEnd;
        }
    };

    scheduler.Run(callbacks);

    Input::Shutdown();
}

void MainLoop::ConfigureEnergyTelemetry() {
    energyTelemetry = EnergyHUDTelemetry{};
    hudShieldCurrentMJ = 0.0;
    hudShieldRechargeTimer = 0.0;
    hudShieldRequirementMW = 0.0;
    hudWeaponRequirementMW = 0.0;
    hudThrusterRequirementMW = 0.0;
    hudOtherDrawMW = 0.0;
    hudEnergyEntityId = 0;

    if (!simulation) {
        energyTelemetry.valid = false;
        energyManagementSystem.reset();
        return;
    }

    const bool hasAssembly = hudShipAssembly.hull != nullptr || !hudShipAssembly.components.empty();
    if (!hasAssembly) {
        energyTelemetry.valid = false;
        energyManagementSystem.reset();
        return;
    }

    energyTelemetry.valid = true;
    energyTelemetry.totalPowerOutputMW = hudShipAssembly.totalPowerOutputMW;
    energyTelemetry.drainRateMW = hudShipAssembly.totalPowerDrawMW;
    energyTelemetry.netPowerMW = energyTelemetry.totalPowerOutputMW - energyTelemetry.drainRateMW;
    if (energyTelemetry.totalPowerOutputMW > 0.0) {
        energyTelemetry.efficiencyPercent = std::clamp(
            (energyTelemetry.drainRateMW / energyTelemetry.totalPowerOutputMW) * 100.0,
            0.0,
            200.0);
    } else {
        energyTelemetry.efficiencyPercent = 0.0;
    }

    energyTelemetry.presets = {
        {"Balanced", 0.33, 0.33, 0.34},
        {"Offense", 0.20, 0.50, 0.30},
        {"Defense", 0.50, 0.25, 0.25},
        {"Speed", 0.25, 0.20, 0.55}
    };
    if (!energyTelemetry.presets.empty()) {
        const auto& preset = energyTelemetry.presets.front();
        energyTelemetry.activePreset = preset.name;
        energyTelemetry.shieldAllocation = preset.shields;
        energyTelemetry.weaponAllocation = preset.weapons;
        energyTelemetry.thrusterAllocation = preset.thrusters;
    }

    const SubsystemSummary* shieldSummary = hudShipAssembly.GetSubsystem(ComponentSlotCategory::Shield);
    const SubsystemSummary* weaponSummary = hudShipAssembly.GetSubsystem(ComponentSlotCategory::Weapon);
    const SubsystemSummary* mainThrusters = hudShipAssembly.GetSubsystem(ComponentSlotCategory::MainThruster);
    const SubsystemSummary* maneuverThrusters = hudShipAssembly.GetSubsystem(ComponentSlotCategory::ManeuverThruster);

    if (shieldSummary) {
        hudShieldRequirementMW = shieldSummary->totalPowerDrawMW;
    }
    if (weaponSummary) {
        hudWeaponRequirementMW = weaponSummary->totalPowerDrawMW;
    }
    if (mainThrusters) {
        hudThrusterRequirementMW += mainThrusters->totalPowerDrawMW;
    }
    if (maneuverThrusters) {
        hudThrusterRequirementMW += maneuverThrusters->totalPowerDrawMW;
    }

    hudOtherDrawMW = std::max(0.0, energyTelemetry.drainRateMW - (hudShieldRequirementMW + hudWeaponRequirementMW + hudThrusterRequirementMW));

    double totalShieldCapacity = 0.0;
    double totalShieldRecharge = 0.0;
    double maxShieldDelay = 0.0;
    int totalAmmoCapacity = 0;
    double maxWeaponFireRate = 0.0;

    for (const auto& component : hudShipAssembly.components) {
        const ShipComponentBlueprint* blueprint = component.blueprint;
        if (!blueprint) {
            continue;
        }

        switch (blueprint->category) {
            case ComponentSlotCategory::Shield:
                totalShieldCapacity += blueprint->shieldCapacityMJ;
                totalShieldRecharge += blueprint->shieldRechargeRateMJPerSec;
                maxShieldDelay = std::max(maxShieldDelay, blueprint->shieldRechargeDelaySeconds);
                break;
            case ComponentSlotCategory::Weapon:
                if (blueprint->weaponAmmoCapacity > 0) {
                    totalAmmoCapacity += blueprint->weaponAmmoCapacity;
                }
                if (blueprint->weaponFireRatePerSecond > 0.0) {
                    maxWeaponFireRate = std::max(maxWeaponFireRate, blueprint->weaponFireRatePerSecond);
                }
                break;
            default:
                break;
        }
    }

    energyTelemetry.shieldCapacityMaxMJ = totalShieldCapacity;
    hudShieldCurrentMJ = totalShieldCapacity > 0.0 ? totalShieldCapacity * 0.85 : 0.0;
    energyTelemetry.shieldCapacityMJ = hudShieldCurrentMJ;
    energyTelemetry.shieldPercent = totalShieldCapacity > 0.0 ? hudShieldCurrentMJ / totalShieldCapacity : 0.0;
    energyTelemetry.shieldRechargeRateMJ = totalShieldRecharge;
    energyTelemetry.shieldRechargeDelaySeconds = maxShieldDelay;
    energyTelemetry.shieldRechargeRemaining = 0.0;

    if (totalAmmoCapacity > 0) {
        energyTelemetry.weaponAmmoMax = totalAmmoCapacity;
        energyTelemetry.weaponAmmoCurrent = totalAmmoCapacity;
    } else {
        energyTelemetry.weaponAmmoMax = -1;
        energyTelemetry.weaponAmmoCurrent = -1;
    }
    if (maxWeaponFireRate > 0.0) {
        energyTelemetry.weaponCooldownSeconds = 1.0 / maxWeaponFireRate;
    }

    energyTelemetry.thrustToMass = hudShipAssembly.ThrustToMassRatio();

    hudEnergyEntityId = simulation ? static_cast<int>(simulation->GetPlayerEntity()) : 0;
    if (hudEnergyEntityId != 0) {
        energyManagementSystem = std::make_unique<EnergyManagementSystem>();
        energyManagementSystem->Initialize(hudEnergyEntityId,
                                           energyTelemetry.totalPowerOutputMW,
                                           hudShieldRequirementMW,
                                           hudWeaponRequirementMW,
                                           hudThrusterRequirementMW);
        energyManagementSystem->SetAllocation(hudEnergyEntityId,
                                              energyTelemetry.shieldAllocation,
                                              energyTelemetry.weaponAllocation,
                                              energyTelemetry.thrusterAllocation);
    } else {
        energyManagementSystem.reset();
        energyTelemetry.valid = false;
        return;
    }

    UpdateEnergyTelemetry(0.0);
}

void MainLoop::UpdateEnergyTelemetry(double deltaSeconds) {
    if (!energyTelemetry.valid || !energyManagementSystem || hudEnergyEntityId == 0) {
        return;
    }

    const double totalOutput = energyTelemetry.totalPowerOutputMW;
    const double availablePower = totalOutput;

    energyManagementSystem->UpdateDemand(hudEnergyEntityId,
                                         totalOutput,
                                         availablePower,
                                         hudShieldRequirementMW,
                                         hudWeaponRequirementMW,
                                         hudThrusterRequirementMW);
    energyManagementSystem->Update(hudEnergyEntityId, static_cast<float>(deltaSeconds));

    const EnergyManagementState* state = energyManagementSystem->GetState(hudEnergyEntityId);
    if (!state) {
        return;
    }

    energyTelemetry.shieldAllocation = state->shieldAllocation;
    energyTelemetry.weaponAllocation = state->weaponAllocation;
    energyTelemetry.thrusterAllocation = state->thrusterAllocation;
    energyTelemetry.shieldDeliveredMW = state->shieldPowerMW;
    energyTelemetry.weaponDeliveredMW = state->weaponPowerMW;
    energyTelemetry.thrusterDeliveredMW = state->thrusterPowerMW;
    energyTelemetry.shieldRequirementMW = state->shieldRequirementMW;
    energyTelemetry.weaponRequirementMW = state->weaponRequirementMW;
    energyTelemetry.thrusterRequirementMW = state->thrusterRequirementMW;

    const double totalSubsystemDemand = state->shieldRequirementMW + state->weaponRequirementMW + state->thrusterRequirementMW;
    energyTelemetry.drainRateMW = hudOtherDrawMW + totalSubsystemDemand;
    energyTelemetry.netPowerMW = totalOutput - energyTelemetry.drainRateMW;

    if (energyTelemetry.weaponRequirementMW > 0.0) {
        energyTelemetry.weaponPercent = std::clamp(
            energyTelemetry.weaponDeliveredMW / energyTelemetry.weaponRequirementMW,
            0.0,
            1.2);
    } else {
        energyTelemetry.weaponPercent = 1.0;
    }
    if (energyTelemetry.thrusterRequirementMW > 0.0) {
        energyTelemetry.thrusterPercent = std::clamp(
            energyTelemetry.thrusterDeliveredMW / energyTelemetry.thrusterRequirementMW,
            0.0,
            1.2);
    } else {
        energyTelemetry.thrusterPercent = 1.0;
    }

    if (energyTelemetry.totalPowerOutputMW > 0.0) {
        energyTelemetry.efficiencyPercent = std::clamp(
            (energyTelemetry.drainRateMW / energyTelemetry.totalPowerOutputMW) * 100.0,
            0.0,
            200.0);
    }

    if (energyTelemetry.shieldCapacityMaxMJ > 0.0) {
        if (energyTelemetry.netPowerMW < 0.0) {
            double drain = std::max(0.0, -energyTelemetry.netPowerMW) * std::max(0.0, deltaSeconds) * 0.5;
            if (drain > 0.0) {
                hudShieldCurrentMJ = std::max(0.0, hudShieldCurrentMJ - drain);
                hudShieldRechargeTimer = energyTelemetry.shieldRechargeDelaySeconds;
            }
        } else {
            if (hudShieldRechargeTimer > 0.0) {
                hudShieldRechargeTimer = std::max(0.0, hudShieldRechargeTimer - deltaSeconds);
            } else if (energyTelemetry.shieldRechargeRateMJ > 0.0) {
                double recharge = energyTelemetry.shieldRechargeRateMJ * std::max(0.0, deltaSeconds);
                if (recharge > 0.0) {
                    hudShieldCurrentMJ = std::min(energyTelemetry.shieldCapacityMaxMJ, hudShieldCurrentMJ + recharge);
                }
            }
        }

        energyTelemetry.shieldRechargeRemaining = hudShieldRechargeTimer;
        energyTelemetry.shieldCapacityMJ = hudShieldCurrentMJ;
        energyTelemetry.shieldPercent = energyTelemetry.shieldCapacityMaxMJ > 0.0
            ? std::clamp(hudShieldCurrentMJ / energyTelemetry.shieldCapacityMaxMJ, 0.0, 1.0)
            : 0.0;
    }

    energyTelemetry.warningPowerDeficit = energyTelemetry.netPowerMW < 0.0;
    energyTelemetry.warningShieldCritical = energyTelemetry.shieldPercent < 0.25;
    energyTelemetry.warningRechargeDelay = hudShieldRechargeTimer > 0.0;
    energyTelemetry.warningOverloadRisk = state->overloadProtection &&
        (totalSubsystemDemand > state->totalPowerMW * state->overloadThreshold);

    energyTelemetry.warnings.clear();

    auto appendWarning = [&](const std::string& label) {
        if (!label.empty()) {
            energyTelemetry.warnings.push_back(label);
        }
    };

    if (energyTelemetry.warningPowerDeficit) {
        std::ostringstream oss;
        oss << "\u26A0 Power Deficit";
        if (energyTelemetry.netPowerMW < 0.0) {
            oss << " (" << std::fixed << std::setprecision(1)
                << std::abs(energyTelemetry.netPowerMW) << " MW)";
        }
        appendWarning(oss.str());
    }

    if (energyTelemetry.warningShieldCritical) {
        std::ostringstream oss;
        oss << "\u26A0 Shield Critical";
        oss << " (" << std::fixed << std::setprecision(0)
            << std::clamp(energyTelemetry.shieldPercent * 100.0, 0.0, 100.0) << "%)";
        appendWarning(oss.str());
    }

    if (energyTelemetry.warningRechargeDelay) {
        std::ostringstream oss;
        oss << "\u26A0 Shield Recharge";
        if (hudShieldRechargeTimer > 0.0) {
            oss << " (" << std::fixed << std::setprecision(1)
                << hudShieldRechargeTimer << "s)";
        }
        appendWarning(oss.str());
    }

    if (energyTelemetry.warningOverloadRisk) {
        appendWarning("\u26A0 Overload Risk");
    }
}

bool MainLoop::IsInMainMenu() const {
    return currentState_ == GameState::MAIN_MENU;
}

void MainLoop::StartNewGame() {
    mainMenu_.SetActive(false);
    mainMenu_.ClearLastAction();
    stateMachine.TransitionTo(EngineState::Running);
    std::cout << "Starting new game from main menu." << std::endl;
}

void MainLoop::LoadSavedGame() {
    mainMenu_.SetActive(false);
    mainMenu_.ClearLastAction();
    stateMachine.TransitionTo(EngineState::Running);
    std::cout << "Continuing game from main menu." << std::endl;
}

#ifdef USE_GLFW
void MainLoop::HandleKeyEvent(int key, int /*scancode*/, int action, int mods) {
    (void)mods;
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    if (IsInMainMenu()) {
        mainMenu_.HandleKeyPress(key);
        return;
    }
}

void MainLoop::HandleMouseButtonEvent(int button, int action, int /*mods*/) {
    if (!IsInMainMenu()) {
        return;
    }

    if (action != GLFW_PRESS || button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    if (!viewport) {
        return;
    }

    double xpos = 0.0;
    double ypos = 0.0;
    GLFWwindow* window = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
    if (!window) {
        return;
    }

    glfwGetCursorPos(window, &xpos, &ypos);

    mainMenu_.HandleMouseClick(
        static_cast<int>(xpos),
        static_cast<int>(ypos),
        viewport->GetWidth(),
        viewport->GetHeight());
}

void MainLoop::HandleCursorPosEvent(double xpos, double ypos) {
    if (!IsInMainMenu() || !viewport) {
        return;
    }

    mainMenu_.HandleMouseMove(
        static_cast<int>(xpos),
        static_cast<int>(ypos),
        viewport->GetWidth(),
        viewport->GetHeight());
}
#endif

void MainLoop::ApplyCameraPreset(size_t index) {
    if (!camera || index >= cameraPresets.size()) {
        return;
    }

    ApplyPresetToCamera(*camera, cameraPresets[index]);

    // Reset offsets and target lock smoothing so preset takes effect immediately
    mouseLookYawOffset = 0.0;
    mouseLookPitchOffset = 0.0;
    cameraFollowController.ResetState();

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
    if (!stateMachine.Is(EngineState::ShuttingDown)) {
        stateMachine.TransitionTo(EngineState::ShuttingDown);
    }
    if (running) { std::cout << "Nova Engine Shutting down..." << std::endl; running = false; }
    if (viewport) { viewport->Shutdown(); }
    if (ecsInspector) {
        ecsInspector->SetEntityManager(nullptr);
    }
    // unique_ptr will free sceneManager
}

void MainLoop::RequestShutdown() {
    std::cout << "Window close requested, shutting down..." << std::endl;
    Shutdown();
}

std::string MainLoop::GetVersion() const { return version; }
