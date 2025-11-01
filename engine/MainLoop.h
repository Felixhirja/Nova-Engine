#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <string>
#include <memory>
#include <array>
#include <vector>

#include "CameraSystem.h"
#include "EngineStateMachine.h"
#include "ecs/EntityManager.h"
#include "ecs/ShipAssembly.h"
#include "FramePacingController.h"
#include "EnergyHUDTelemetry.h"
#include "EnergyManagementSystem.h"
#include "MainMenu.h"
#include "ActorContext.h"

// Forward declarations
class Viewport3D;
class Simulation;
class ResourceManager;
class EntityManager;
class VisualFeedbackSystem;
class AudioFeedbackSystem;
class HUDAlertSystem;
class ECSInspector;
struct CameraViewState;

class Camera;

class MainLoop {
public:
    MainLoop();
    ~MainLoop();

    void Init();
    // Runs the main loop. maxSeconds specifies how long the demo runs (0 = run once iteration only).
    void MainLoopFunc(int maxSeconds = 5);
    void Shutdown();

    std::string GetVersion() const;

    // Public getter for viewport (needed for GLFW callback)
    Viewport3D* GetViewport() { return viewport.get(); }

    // Request shutdown (used by GLFW window close callback)
    void RequestShutdown();

    bool IsInMainMenu() const;
    MainMenu& GetMainMenu() { return mainMenu_; }
    const MainMenu& GetMainMenu() const { return mainMenu_; }

#ifdef USE_GLFW
    void HandleKeyEvent(int key, int scancode, int action, int mods);
    void HandleMouseButtonEvent(int button, int action, int mods);
    void HandleCursorPosEvent(double xpos, double ypos);
#endif

private:
    bool running;
    std::string version;
    std::unique_ptr<Viewport3D> viewport;
    std::unique_ptr<Simulation> simulation;
    std::unique_ptr<ResourceManager> resourceManager;
    // Camera instance
    std::unique_ptr<Camera> camera;
    // Canonical ECS entity manager
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ECSInspector> ecsInspector;
    // Feedback systems
    std::unique_ptr<VisualFeedbackSystem> visualFeedbackSystem;
    std::unique_ptr<AudioFeedbackSystem> audioFeedbackSystem;
    std::unique_ptr<HUDAlertSystem> hudAlertSystem;
    ShipAssemblyResult hudShipAssembly;
    ActorContext bootstrapActorContext_;
    std::vector<std::string> registeredActorTypes_;
    EngineStateMachine stateMachine;
    bool thrustModeEnabled;

    std::unique_ptr<class EnergyManagementSystem> energyManagementSystem;
    int hudEnergyEntityId;
    EnergyHUDTelemetry energyTelemetry;
    double hudShieldCurrentMJ;
    double hudShieldRechargeTimer;
    double hudShieldRequirementMW;
    double hudWeaponRequirementMW;
    double hudThrusterRequirementMW;
    double hudOtherDrawMW;

    // Mouse look offsets for target lock mode
    double mouseLookYawOffset;
    double mouseLookPitchOffset;

    // Target lock transition smoothing
    CameraFollowController cameraFollowController;
    FramePacingController framePacingController;
    std::array<CameraPreset, 3> cameraPresets;

    enum class GameState {
        MAIN_MENU,
        PLAYING,
        PAUSED
    };

    GameState currentState_ = GameState::MAIN_MENU;
    MainMenu mainMenu_;

    struct EnergyWarningCache {
        bool powerDeficit = false;
        double netPowerAbs = 0.0;
        bool shieldCritical = false;
        double shieldPercent = 0.0;
        bool rechargeDelay = false;
        double rechargeTimer = 0.0;
        bool overloadRisk = false;
        std::vector<std::string> warnings;
    };

    EnergyWarningCache energyWarningCache_;

    void ApplyCameraPreset(size_t index);
    void ConfigureEnergyTelemetry();
    void UpdateEnergyTelemetry(double deltaSeconds);
    void StartNewGame();
    void LoadSavedGame();
};

#endif // MAIN_LOOP_H
