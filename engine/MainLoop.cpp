#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include "ResourceManager.h"
#include "Spaceship.h"
#include "EnergyManagementSystem.h"
#include "CameraSystem.h"
#include "GamepadManager.h"
#include "EngineBootstrap.h"
#include "FrameScheduler.h"
#include "Transform.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include "ecs/ECSInspector.h"
#include "VisualFeedbackSystem.h"
#include "AudioFeedbackSystem.h"
#include "HUDAlertSystem.h"
#include "ActorRegistry.h"
#include "Entities.h"  // Include ALL entities for automatic rendering
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <utility>
#include <cstdlib>
#include <cstdio>

// Simple replacement for Player::CameraViewState
// CameraViewState is defined in Player.h
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
    bool captureMouse = false;
    bool isRelativeMode = false;
    bool pendingRecenter = false;
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;
    bool hasLastCursorSample = false;
    // SDL-only: track previous function-key states for edge-trigger toggles
    bool sdlPrevF8Down = false;
    bool sdlPrevF9Down = false;
    bool sdlPrevF10Down = false;
    bool sdlPrevF11Down = false;
    FrameStageDurations lastStageDurations{};
    FrameTimingAverages rollingTimings{};
    double frameDurationSeconds = 0.0;
};

constexpr double kWarningPowerDeltaThreshold = 0.05;
constexpr double kWarningPercentThreshold = 0.01;
constexpr double kWarningTimerThreshold = 0.05;

bool HasSignificantDelta(double previous, double current, double threshold) {
    return std::abs(previous - current) > threshold;
}

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
    // std::cout << "DEBUG: MainLoop::Init() STARTED" << std::endl;
    // std::cerr << "DEBUG: MainLoop::Init() STARTED (cerr)" << std::endl;
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
            // std::cout << "GamepadManager: XInput available via "
            //           << gamepadManager.ActiveLibraryNameUtf8() << std::endl;
        } else {
            const std::string errorDescription = gamepadManager.LastError();
            if (!errorDescription.empty()) {
                // std::cout << "GamepadManager: XInput unavailable (" << errorDescription << ")" << std::endl;
            } else {
                // std::cout << "GamepadManager: XInput unavailable" << std::endl;
            }
        }
    }

    viewport = std::make_unique<Viewport3D>();
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Viewport3D constructed" << std::endl;
    }
    viewport->Init();
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Viewport3D::Init returned" << std::endl;
    }
    // std::cout << "Viewport3D::Init() completed" << std::endl;

    if (viewport) {
        viewport->ConfigureLayouts(Viewport3D::CreateDefaultLayouts());
        viewport->SetLayoutConfigPath("assets/config/viewport_layouts.json");
        viewport->SetFramePacingHint(framePacingController.IsVSyncEnabled(), framePacingController.TargetFPS());
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Layouts configured & frame pacing hint set" << std::endl;
    }

    CameraFollow::CameraFollowConfig cameraConfigDefaults;
    CameraFollow::CameraFollowConfig cameraConfig = cameraConfigDefaults;
    const char* cameraConfigProfileEnv = std::getenv("NOVA_CAMERA_PROFILE");
    if (cameraConfigProfileEnv && *cameraConfigProfileEnv) {
        const std::string profileName(cameraConfigProfileEnv);
        if (!CameraConfigLoader::LoadCameraFollowConfigProfile("assets/config/camera_follow.ini", profileName, cameraConfig)) {
            CameraConfigLoader::LoadCameraFollowConfig("assets/config/camera_follow.ini", cameraConfig);
        }
    } else {
        CameraConfigLoader::LoadCameraFollowConfig("assets/config/camera_follow.ini", cameraConfig);
    }
    cameraConfig.Validate();
    cameraFollowController.SetConfig(cameraConfig);
    cameraFollowController.ResetState();

    // Set up GLFW window resize callback
    // std::cout << "Setting up GLFW window resize callback" << std::endl;
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
        {
            std::ofstream log2("sdl_diag.log", std::ios::app);
            if (log2) log2 << "GLFW callbacks set" << std::endl;
        }
    }
#endif

    // Set GLFW window for input handling
    // std::cout << "About to set GLFW window for input" << std::endl;
#ifdef USE_GLFW
    void* glfwWindowPtr = viewport->GetGLFWWindow();
    // std::cout << "GLFW window pointer: " << glfwWindowPtr << std::endl;
    if (glfwWindowPtr != nullptr) {
        Input::SetGLFWWindow(glfwWindowPtr);
        // std::cout << "GLFW window set for input" << std::endl;
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Input set GLFW window" << std::endl;
    } else {
        // std::cout << "GLFW window is null (headless mode?), skipping input setup" << std::endl;
    }
#endif
    
    // Set SDL window for input handling (if available)
#ifdef USE_SDL
    Input::SetSDLWindow(viewport->GetSDLWindow());
#endif

    // std::cout << "About to create camera" << std::endl;
    // Camera
    // Position: behind player at (-8, 0, 6), looking toward origin (0,0,0)
    // Yaw of PI/2 (90 degrees) makes camera look in +X direction (toward player)
    camera = std::make_unique<Camera>(-8.0, 0.0, 6.0, -0.1, 1.5708, Camera::kDefaultFovDegrees); // yaw = π/2 to look toward player
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Camera created" << std::endl;
    }

    // std::cout << "About to create entity manager" << std::endl;
    // Create canonical ECS manager and initialize simulation with it
    entityManager = std::make_unique<EntityManager>();
    if (!ecsInspector) {
        ecsInspector = std::make_unique<ECSInspector>();
    }
    ecsInspector->SetEntityManager(entityManager.get());
    simulation = std::make_unique<Simulation>();
    // Performance optimization: disable advanced systems for better FPS
    simulation->SetEnableAdvancedSystems(false);
    // std::cout << "About to call simulation->Init()" << std::endl;
    simulation->Init(entityManager.get());
    // std::cout << "Simulation::Init() completed" << std::endl;
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Simulation initialized" << std::endl;
    }

    if (viewport && simulation) {
        Entity playerEntity = simulation->GetPlayerEntity();
        if (playerEntity != 0 && entityManager && entityManager->IsAlive(playerEntity)) {
            viewport->SetEntityMesh(playerEntity, Viewport3D::CreatePlayerAvatarMesh(), 1.1f);
        }
    }

    // Resource manager & demo entity (use unique_ptr ownership)
    resourceManager = std::make_unique<ResourceManager>();

    // Initialize feedback systems
    // std::cout << "Initializing feedback systems..." << std::endl;
    visualFeedbackSystem = std::make_unique<VisualFeedbackSystem>();
    audioFeedbackSystem = std::make_unique<AudioFeedbackSystem>();
    hudAlertSystem = std::make_unique<HUDAlertSystem>();
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "Feedback systems initialized" << std::endl;
    }
    // std::cout << "Feedback systems initialized" << std::endl;
    // Bootstrap demo content and HUD assembly
    EngineBootstrap bootstrap;
    auto* schedulerPtr = simulation ? simulation->GetSchedulerV2() : nullptr;
    auto bootstrapResult = bootstrap.Run(*resourceManager, *entityManager, schedulerPtr);
    hudShipAssembly = std::move(bootstrapResult.hudAssembly);
    bootstrapActorContext_ = bootstrapResult.actorContext;
    registeredActorTypes_ = std::move(bootstrapResult.actorTypes);

    // Player actor removed - entities are managed directly by ECS
    // playerActor_ = std::make_unique<Player>();
    // playerActor_->Initialize();
    ConfigureEnergyTelemetry();

    stateMachine.TransitionTo(EngineState::Running);
    {
        std::ofstream log2("sdl_diag.log", std::ios::app);
        if (log2) log2 << "MainLoop::Init finished" << std::endl;
    }
}


void MainLoop::MainLoopFunc(int maxSeconds) {
    // std::cout << "MainLoopFunc started" << std::endl;
    if (!running) {
        // std::cout << "Engine not initialized!" << std::endl;
        return;
    }

    using clock = std::chrono::high_resolution_clock;

    FrameRuntimeContext runtime;
    runtime.maxSeconds = maxSeconds;
    runtime.demoStart = clock::now();
    runtime.fpsTimer = runtime.demoStart;

    const double updateHz = 60.0;
    const double fixedDt = 1.0 / updateHz;
    FrameSchedulerConfig schedulerConfig;
    schedulerConfig.fixedUpdateHz = updateHz;
    schedulerConfig.maxRenderHz = framePacingController.TargetFPS();

    FrameScheduler scheduler(schedulerConfig);

    const char* headlessEnv = std::getenv("NOVA_ENGINE_HEADLESS");
    runtime.headlessMode = (headlessEnv != nullptr && std::string(headlessEnv) == "1");
    if (runtime.headlessMode) {
        const char* maxFramesEnv = std::getenv("NOVA_ENGINE_MAX_FRAMES");
        if (maxFramesEnv != nullptr) {
            runtime.maxFrames = std::atoi(maxFramesEnv);
            // std::cout << "Headless mode: will run for " << runtime.maxFrames << " frames then exit" << std::endl;
        } else {
            runtime.maxFrames = 300;
            // std::cout << "Headless mode: will run for " << runtime.maxFrames << " frames (default) then exit" << std::endl;
        }
        framePacingController.SetVSyncEnabled(false);
        framePacingController.SetTargetFPS(0.0);
        scheduler.SetMaxRenderHz(0.0);
    } else {
        scheduler.SetMaxRenderHz(framePacingController.TargetFPS());
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

#ifdef USE_SDL
        // SDL hotkeys: detect F8/F9 (debug) and F11 (fullscreen) presses with edge detection
        if (viewport && viewport->GetSDLWindow()) {
            const Uint8* sdlKeys = SDL_GetKeyboardState(nullptr);
            if (sdlKeys) {
                bool f8Down = sdlKeys[SDL_SCANCODE_F8] != 0;
                bool f9Down = sdlKeys[SDL_SCANCODE_F9] != 0;
                bool f10Down = sdlKeys[SDL_SCANCODE_F10] != 0;
                bool f11Down = sdlKeys[SDL_SCANCODE_F11] != 0;
#ifndef NDEBUG
                if (f8Down && !runtime.sdlPrevF8Down) {
                    viewport->ToggleWorldAxes();
                }
                if (f9Down && !runtime.sdlPrevF9Down) {
                    viewport->ToggleMiniAxesGizmo();
                }
                if (f10Down && !runtime.sdlPrevF10Down) {
                    viewport->ToggleStaticGrid();
                }
#endif
                if (f11Down && !runtime.sdlPrevF11Down) {
                    viewport->ToggleFullscreen();
                }
                runtime.sdlPrevF8Down = f8Down;
                runtime.sdlPrevF9Down = f9Down;
                runtime.sdlPrevF10Down = f10Down;
                runtime.sdlPrevF11Down = f11Down;
            }
        }
#endif

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
                // std::cout << "Settings menu not implemented yet." << std::endl;
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

        UpdateEnergyTelemetry(deltaSeconds);

        int key = Input::PollKey();
        runtime.lastKey = key;
        if (key != -1) {
            // std::cout << "Key pressed: " << key << " ('" << static_cast<char>(key) << "')" << std::endl;
        }

        if (key == 'q' || key == 'Q' || Input::IsKeyHeld('q') || Input::IsKeyHeld('Q') || key == 27) {
            // std::cout << "Quit key detected, exiting..." << std::endl;
            requestShutdown();
            return;
        }

        if (key == 'p' || key == 'P') {
            if (stateMachine.TogglePause()) {
                // std::cout << (stateMachine.Is(EngineState::Paused) ? "Engine paused" : "Engine resumed") << std::endl;
            }
        }

        if ((key == 'z' || key == 'Z') && camera) {
            constexpr double kFovStep = 5.0;
            camera->SetTargetZoom(camera->targetZoom() - kFovStep);
        }
        if ((key == 'x' || key == 'X') && camera) {
            constexpr double kFovStep = 5.0;
            camera->SetTargetZoom(camera->targetZoom() + kFovStep);
        }

        if (key == '1' || key == '2' || key == '3') {
            size_t presetIndex = static_cast<size_t>(key - '1');
            ApplyCameraPreset(presetIndex);
            // Clear input deltas so next tick doesn't rotate
            runtime.mouseDeltaX = 0.0;
            runtime.mouseDeltaY = 0.0;
            // Force target lock off for this frame
            runtime.targetLocked = false;
        }

        if ((key == 't' || key == 'T') && simulation) {
            thrustModeEnabled = !thrustModeEnabled;
            simulation->SetUseThrustMode(thrustModeEnabled);
            // std::cout << "Player vertical mode: " << (thrustModeEnabled ? "thrust" : "jump") << std::endl;
        }

        if (key == GLFW_KEY_TAB) {
            bool locked = false;
            if (entityManager && simulation) {
                auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
                if (targetLock) {
                    targetLock->isLocked = !targetLock->isLocked;
                    locked = targetLock->isLocked;
                }
            }
            if (locked) {
                mouseLookYawOffset = 0.0;
                mouseLookPitchOffset = 0.0;
            }
        }

        if ((key == 'b' || key == 'B') && viewport) {
            bool bloomEnabled = viewport->IsBloomEnabled();
            viewport->SetBloomEnabled(!bloomEnabled);
            // std::cout << "Bloom effect: " << (!bloomEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }

        if ((key == 'l' || key == 'L') && viewport) {
            bool letterboxEnabled = viewport->IsLetterboxEnabled();
            viewport->SetLetterboxEnabled(!letterboxEnabled);
            // std::cout << "Letterbox overlay: " << (!letterboxEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }

        if ((key == 'i' || key == 'I') && ecsInspector) {
            ecsInspector->Toggle();
            // std::cout << "ECS inspector: " << (ecsInspector->IsEnabled() ? "ENABLED" : "DISABLED") << std::endl;
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
        bool cameraForward = Input::IsArrowKeyHeld(GLFW_KEY_UP) || forward;
        bool cameraBackward = Input::IsArrowKeyHeld(GLFW_KEY_DOWN) || backward;
        bool cameraLeft = Input::IsArrowKeyHeld(GLFW_KEY_LEFT) || strafeLeft;
        bool cameraRight = Input::IsArrowKeyHeld(GLFW_KEY_RIGHT) || strafeRight;
        bool cameraUp = (Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_UP)) || up;
        bool cameraDown = (Input::IsKeyHeld(' ') && Input::IsArrowKeyHeld(GLFW_KEY_DOWN)) || down;
#else
        bool cameraForward = false;
        bool cameraBackward = false;
        bool cameraLeft = false;
        bool cameraRight = false;
        bool cameraUp = false;
        bool cameraDown = false;
#endif

        if (entityManager && simulation) {
            if (auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity())) {
                runtime.targetLocked = targetLock->isLocked;
            } else {
                runtime.targetLocked = false;
            }
        } else {
            runtime.targetLocked = false;
        }
        if (!runtime.targetLocked && entityManager && simulation) {
            auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity());
            if (targetLock) {
                runtime.targetLocked = targetLock->isLocked;
            }
        }

        runtime.captureMouse = !IsInMainMenu();

        bool wantRelative = runtime.captureMouse || runtime.targetLocked;
#ifdef USE_GLFW
        GLFWwindow* focusWindow = (viewport && viewport->GetGLFWWindow())
            ? static_cast<GLFWwindow*>(viewport->GetGLFWWindow())
            : nullptr;
        const bool hasFocus = focusWindow && glfwGetWindowAttrib(focusWindow, GLFW_FOCUSED);
        wantRelative = wantRelative && hasFocus;
        if (wantRelative != runtime.isRelativeMode) {
            if (focusWindow) {
                glfwSetInputMode(focusWindow, GLFW_CURSOR, wantRelative ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                if (wantRelative) {
                    int width = 0;
                    int height = 0;
                    glfwGetWindowSize(focusWindow, &width, &height);
                    glfwSetCursorPos(focusWindow, width / 2.0, height / 2.0);
                }
            }
            runtime.pendingRecenter = true;
            runtime.hasLastCursorSample = false;
            runtime.mouseDeltaX = 0.0;
            runtime.mouseDeltaY = 0.0;
            runtime.isRelativeMode = wantRelative;
        }
#else
        if (wantRelative != runtime.isRelativeMode) {
            runtime.isRelativeMode = wantRelative;
        }
#endif

        double playerInputYaw = 0.0;  // Player yaw will come from simulation entity
        if (simulation) {
            simulation->SetPlayerInput(forward, backward, up, down, strafeLeft, strafeRight, playerInputYaw);
        }

#if defined(USE_GLFW)
        if (wantRelative) {
            if (viewport && viewport->GetGLFWWindow()) {
                GLFWwindow* window = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
                if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
                    double cursorX = 0.0;
                    double cursorY = 0.0;
                    glfwGetCursorPos(window, &cursorX, &cursorY);

                    if (runtime.pendingRecenter || !runtime.hasLastCursorSample) {
                        runtime.mouseDeltaX = 0.0;
                        runtime.mouseDeltaY = 0.0;
                        runtime.pendingRecenter = false;
                        runtime.hasLastCursorSample = true;
                    } else {
                        runtime.mouseDeltaX = cursorX - runtime.lastCursorX;
                        runtime.mouseDeltaY = cursorY - runtime.lastCursorY;
                    }

                    runtime.lastCursorX = cursorX;
                    runtime.lastCursorY = cursorY;

                    const double deadzone = 0.25; // pixels
                    if (std::abs(runtime.mouseDeltaX) < deadzone) runtime.mouseDeltaX = 0.0;
                    if (std::abs(runtime.mouseDeltaY) < deadzone) runtime.mouseDeltaY = 0.0;
                } else {
                    runtime.mouseDeltaX = 0.0;
                    runtime.mouseDeltaY = 0.0;
                    runtime.pendingRecenter = true;
                    runtime.hasLastCursorSample = false;
                }
            } else {
                runtime.mouseDeltaX = 0.0;
                runtime.mouseDeltaY = 0.0;
                runtime.pendingRecenter = true;
                runtime.hasLastCursorSample = false;
                runtime.lastCursorX = 0.0;
                runtime.lastCursorY = 0.0;
            }
        } else {
            runtime.mouseDeltaX = 0.0;
            runtime.mouseDeltaY = 0.0;
            runtime.pendingRecenter = true;
            runtime.hasLastCursorSample = false;
        }
#elif defined(USE_SDL)
        if (viewport && viewport->GetSDLWindow()) {
            SDL_Window* window = static_cast<SDL_Window*>(viewport->GetSDLWindow());
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) {
                int sdlMouseDeltaX = 0;
                int sdlMouseDeltaY = 0;
                SDL_GetRelativeMouseState(&sdlMouseDeltaX, &sdlMouseDeltaY);
                runtime.mouseDeltaX = sdlMouseDeltaX;
                runtime.mouseDeltaY = sdlMouseDeltaY;
            } else {
                runtime.mouseDeltaX = 0.0;
                runtime.mouseDeltaY = 0.0;
            }
        }
#endif

        if (camera) {
            double cameraMoveSpeed = 10.0;  // Increased from 0.5 for better responsiveness
            double deltaTime = deltaSeconds > 0.0 ? deltaSeconds : fixedDt;
            // Process mouse input for camera offsets BEFORE camera update
            const double mouseDecay = 0.96;
            if (runtime.targetLocked) {
                const double targetLockSensitivity = 0.004;
                double accelerationFactor = 1.0;
                double mouseSpeed = std::sqrt(runtime.mouseDeltaX * runtime.mouseDeltaX +
                                              runtime.mouseDeltaY * runtime.mouseDeltaY);
                if (mouseSpeed > 5.0) {
                    accelerationFactor = 1.0 + (mouseSpeed - 5.0) * 0.02;
                }

                if (std::abs(runtime.mouseDeltaX) > 1.0 || std::abs(runtime.mouseDeltaY) > 1.0) {
                    // For target-lock mode, don't accumulate offsets - pass raw deltas
                    const double yawDelta = runtime.mouseDeltaX * targetLockSensitivity * accelerationFactor;
                    const double pitchDelta = -runtime.mouseDeltaY * targetLockSensitivity * accelerationFactor;
                    // CameraFollow applies invertLockYaw/invertLockPitch when consuming these offsets.

                    mouseLookYawOffset = yawDelta;
                    mouseLookPitchOffset = pitchDelta;

                    // Clamp to prevent extreme values
                    const double maxDelta = 0.5; // radians per frame
                    mouseLookYawOffset = std::clamp(mouseLookYawOffset, -maxDelta, maxDelta);
                    mouseLookPitchOffset = std::clamp(mouseLookPitchOffset, -maxDelta, maxDelta);
                } else {
                    mouseLookYawOffset = 0.0;
                    mouseLookPitchOffset = 0.0;
                }
            } else {
                mouseLookYawOffset *= mouseDecay;
                mouseLookPitchOffset *= mouseDecay;
            }

            // Check if we have an entity with CameraComponent to follow
            Entity cameraTargetEntity = 0;
            int highestPriority = -1;
            bool foundCameraTarget = false;
            
            if (entityManager) {
                // Find the highest priority entity with CameraComponent
                entityManager->ForEach<Position, CameraComponent>(
                    [&](Entity e, Position& /*pos*/, CameraComponent& cam) {
                        if (cam.isActive && cam.priority > highestPriority) {
                            cameraTargetEntity = e;
                            highestPriority = cam.priority;
                            foundCameraTarget = true;
                        }
                    }
                );
            }

            CameraViewState cameraAnchor;
            if (foundCameraTarget && entityManager) {
                // When we have a camera entity, use the camera's current position
                // as the anchor so it doesn't get pulled toward the entity position.
                // This allows free camera movement.
                cameraAnchor.worldX = camera->x();
                cameraAnchor.worldY = camera->y();
                cameraAnchor.worldZ = camera->z();
                cameraAnchor.isTargetLocked = false;
            } else {
                // Free camera mode - use camera's current position
                cameraAnchor.worldX = camera->x();
                cameraAnchor.worldY = camera->y();
                cameraAnchor.worldZ = camera->z();
                cameraAnchor.isTargetLocked = false;
            }
            runtime.targetLocked = cameraAnchor.isTargetLocked;

            CameraFollow::CameraFollowInput followInput;
            followInput.playerX = cameraAnchor.worldX;
            followInput.playerY = cameraAnchor.worldY;
            followInput.playerZ = cameraAnchor.worldZ;
            followInput.isTargetLocked = cameraAnchor.isTargetLocked;
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
            movementInput.mouseDeltaX = runtime.mouseDeltaX;
            movementInput.mouseDeltaY = runtime.mouseDeltaY;

            cameraFollowController.Update(*camera, followInput, movementInput, deltaTime, simulation ? simulation->GetActivePhysicsEngine().get() : nullptr);
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
        if (runtime.headlessMode) {
            return;
        }
        if (!viewport) {
            return;
        }

        if (currentState_ == GameState::MAIN_MENU) {
            viewport->Clear();
            auto renderData = mainMenu_.GetRenderData();
            viewport->RenderMenuOverlay(renderData);
            return;
        }

        // Show HUD hints for first few seconds
        using clock = std::chrono::high_resolution_clock;
        double secondsSinceStart = std::chrono::duration<double>(clock::now() - runtime.demoStart).count();
        if (viewport) {
            viewport->SetShowHudHints(secondsSinceStart < 5.0);
        }

        CameraViewState playerView;
        if (simulation) {
            playerView.worldX = simulation->GetPlayerX();
            playerView.worldY = simulation->GetPlayerY();
            playerView.worldZ = simulation->GetPlayerZ();
            playerView.isTargetLocked = runtime.targetLocked;
        }

        runtime.targetLocked = playerView.isTargetLocked;

        viewport->Clear();
        viewport->Render(camera.get(), playerView.worldX, playerView.worldY, playerView.worldZ, runtime.targetLocked);

        Transform playerTransform;
        playerTransform.x = playerView.worldX;
        playerTransform.y = playerView.worldY;
        playerTransform.z = playerView.worldZ;
        Entity playerEntity = simulation ? simulation->GetPlayerEntity() : 0;
        viewport->DrawEntity(playerEntity, playerTransform);

        // Render all entities with ViewportID component (fallback to cube if no model)
        if (entityManager) {
            entityManager->ForEach<Position, ViewportID>([&](Entity e, Position& pos, ViewportID& vp) {
                if (e != playerEntity && vp.viewportId == 0) {  // Skip player, render entities in viewport 0
                    Transform entityTransform;
                    entityTransform.x = pos.x;
                    entityTransform.y = pos.y;
                    entityTransform.z = pos.z;
                    viewport->DrawEntity(e, entityTransform);  // Will use cube as fallback
                }
            });
        }

        if (camera) {
            double wheelDelta = Input::GetMouseWheelDelta();
            if (wheelDelta != 0.0) {
                constexpr double kWheelStepDegrees = 3.0;
                double newFov = camera->targetZoom() - (wheelDelta * kWheelStepDegrees);
                camera->SetTargetZoom(newFov);
                Input::ResetMouseWheelDelta();
            }
        }

        double hudPlayerX = playerView.worldX;
        double hudPlayerY = playerView.worldY;
        double hudPlayerZ = playerView.worldZ;
        bool hudTargetLocked = playerView.isTargetLocked;
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

        runtime.lastStageDurations = info.stageDurations;
        runtime.rollingTimings = info.rolling;
        runtime.frameDurationSeconds = info.frameDurationSeconds;

        if (runtime.headlessMode) {
            scheduler.SetMaxRenderHz(0.0);
        } else {
            framePacingController.UpdateFromTimings(info.rolling);
            scheduler.SetMaxRenderHz(framePacingController.TargetFPS());
            if (viewport) {
                viewport->SetFramePacingHint(framePacingController.IsVSyncEnabled(), framePacingController.TargetFPS());
            }
        }

        if (runtime.headlessMode && runtime.maxFrames > 0 && runtime.frameCount >= runtime.maxFrames && !runtime.headlessNoticePrinted) {
            // std::cout << "Headless mode: reached " << runtime.frameCount << " frames, exiting..." << std::endl;
            runtime.headlessNoticePrinted = true;
            runtime.requestExit = true;
            stateMachine.TransitionTo(EngineState::ShuttingDown);
        }

        if (runtime.maxSeconds > 0) {
            if (std::chrono::duration<double>(info.frameEnd - runtime.demoStart).count() >= runtime.maxSeconds) {
                // std::cout << "Reached max runtime of " << runtime.maxSeconds << " seconds, exiting..." << std::endl;
                runtime.requestExit = true;
                stateMachine.TransitionTo(EngineState::ShuttingDown);
            }
        }

        if (std::chrono::duration<double>(info.frameEnd - runtime.fpsTimer).count() >= 1.0) {
            // std::cout << "FPS: " << runtime.framesThisSecond << std::endl;
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
    energyWarningCache_ = EnergyWarningCache{};

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

    const double netPowerAbs = std::abs(energyTelemetry.netPowerMW);
    bool refreshWarnings = false;

    if (energyTelemetry.warningPowerDeficit != energyWarningCache_.powerDeficit ||
        (energyTelemetry.warningPowerDeficit &&
         HasSignificantDelta(energyWarningCache_.netPowerAbs, netPowerAbs, kWarningPowerDeltaThreshold))) {
        refreshWarnings = true;
    }

    if (energyTelemetry.warningShieldCritical != energyWarningCache_.shieldCritical ||
        (energyTelemetry.warningShieldCritical &&
         HasSignificantDelta(energyWarningCache_.shieldPercent, energyTelemetry.shieldPercent, kWarningPercentThreshold))) {
        refreshWarnings = true;
    }

    if (energyTelemetry.warningRechargeDelay != energyWarningCache_.rechargeDelay ||
        (energyTelemetry.warningRechargeDelay &&
         HasSignificantDelta(energyWarningCache_.rechargeTimer, hudShieldRechargeTimer, kWarningTimerThreshold))) {
        refreshWarnings = true;
    }

    if (energyTelemetry.warningOverloadRisk != energyWarningCache_.overloadRisk) {
        refreshWarnings = true;
    }

    if (refreshWarnings) {
        auto& cache = energyWarningCache_;
        cache.powerDeficit = energyTelemetry.warningPowerDeficit;
        cache.netPowerAbs = netPowerAbs;
        cache.shieldCritical = energyTelemetry.warningShieldCritical;
        cache.shieldPercent = energyTelemetry.shieldPercent;
        cache.rechargeDelay = energyTelemetry.warningRechargeDelay;
        cache.rechargeTimer = hudShieldRechargeTimer;
        cache.overloadRisk = energyTelemetry.warningOverloadRisk;

        auto& warningsVec = cache.warnings;
        warningsVec.clear();
        warningsVec.reserve(4);

        char buffer[64];

        if (energyTelemetry.warningPowerDeficit) {
            if (energyTelemetry.netPowerMW < 0.0) {
                std::snprintf(buffer, sizeof(buffer), "\u26A0 Power Deficit (%.1f MW)", netPowerAbs);
            } else {
                std::snprintf(buffer, sizeof(buffer), "\u26A0 Power Deficit");
            }
            warningsVec.emplace_back(buffer);
        }

        if (energyTelemetry.warningShieldCritical) {
            const double shieldPercent = std::clamp(energyTelemetry.shieldPercent * 100.0, 0.0, 100.0);
            std::snprintf(buffer, sizeof(buffer), "\u26A0 Shield Critical (%.0f%%)", shieldPercent);
            warningsVec.emplace_back(buffer);
        }

        if (energyTelemetry.warningRechargeDelay) {
            if (hudShieldRechargeTimer > 0.0) {
                std::snprintf(buffer, sizeof(buffer), "\u26A0 Shield Recharge (%.1fs)", hudShieldRechargeTimer);
            } else {
                std::snprintf(buffer, sizeof(buffer), "\u26A0 Shield Recharge");
            }
            warningsVec.emplace_back(buffer);
        }

        if (energyTelemetry.warningOverloadRisk) {
            warningsVec.emplace_back("\u26A0 Overload Risk");
        }

        energyTelemetry.warnings = warningsVec;
    }
}

bool MainLoop::IsInMainMenu() const {
    return currentState_ == GameState::MAIN_MENU;
}

void MainLoop::StartNewGame() {
    mainMenu_.SetActive(false);
    mainMenu_.ClearLastAction();
    stateMachine.TransitionTo(EngineState::Running);
    // std::cout << "Starting new game from main menu." << std::endl;
}

void MainLoop::LoadSavedGame() {
    mainMenu_.SetActive(false);
    mainMenu_.ClearLastAction();
    stateMachine.TransitionTo(EngineState::Running);
    // std::cout << "Continuing game from main menu." << std::endl;
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

    // Runtime debug toggles (GLFW path)
    if (viewport) {
        if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
            viewport->ToggleFullscreen();
            return;
        }
#ifndef NDEBUG
        switch (key) {
        case GLFW_KEY_F8:
            viewport->ToggleWorldAxes();
            break;
        case GLFW_KEY_F9:
            viewport->ToggleMiniAxesGizmo();
            break;
        case GLFW_KEY_F10:
            viewport->ToggleStaticGrid();
            break;
        case GLFW_KEY_F11:
            viewport->ToggleCameraDebug();
            break;
        default:
            break;
        }
#endif
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
#if defined(DEBUG) || defined(_DEBUG)
        std::cerr << "ApplyCameraPreset: invalid index " << index << std::endl;
#endif
        return;
    }

    ApplyPresetToCamera(*camera, cameraPresets[index]);

    // Reset offsets & smoothing so the preset is exact this frame
    mouseLookYawOffset = 0.0;
    mouseLookPitchOffset = 0.0;
    cameraFollowController.ResetState();

    // Skip one controller update to prevent re-smoothing tug
    cameraFollowController.SuppressNextUpdate();

    // Clear target lock immediately
    if (entityManager && simulation) {
        if (auto* targetLock = entityManager->GetComponent<TargetLock>(simulation->GetPlayerEntity())) {
            targetLock->isLocked = false;
        }
    }

    // std::cout << "Camera preset " << (index + 1) << " applied" << std::endl;
}

void MainLoop::Shutdown() {
    if (!stateMachine.Is(EngineState::ShuttingDown)) {
        stateMachine.TransitionTo(EngineState::ShuttingDown);
    }

    if (!running && !viewport) {
        return;
    }

    running = false;

#ifdef USE_GLFW
    // Clear GLFW callbacks only once while the window is still alive.
    if (viewport && viewport->GetGLFWWindow()) {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(viewport->GetGLFWWindow());
        glfwSetWindowSizeCallback(glfwWindow, nullptr);
        glfwSetKeyCallback(glfwWindow, nullptr);
        glfwSetMouseButtonCallback(glfwWindow, nullptr);
        glfwSetCursorPosCallback(glfwWindow, nullptr);
    }
    // Drop the input hook before the window/context disappear to avoid calling back into a dead GLFW window.
    Input::SetGLFWWindow(nullptr);
#endif

#ifdef USE_SDL
    Input::SetSDLWindow(nullptr);
#endif

    if (viewport) {
        viewport->Shutdown();
        viewport.reset();
    }

    if (ecsInspector) {
        ecsInspector->SetEntityManager(nullptr);
    }
}

void MainLoop::RequestShutdown() {
    Shutdown();
}

std::string MainLoop::GetVersion() const { return version; }
