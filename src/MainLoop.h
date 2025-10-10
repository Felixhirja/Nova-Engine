#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <string>
#include <memory>
#include <array>

#include "CameraFollow.h"
#include "CameraPresets.h"
#include "ShipAssembly.h"

class Viewport3D;
class Simulation;
class ResourceManager;
class EntityManager;
class VisualFeedbackSystem;
class AudioFeedbackSystem;
class HUDAlertSystem;

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
    // Feedback systems
    std::unique_ptr<VisualFeedbackSystem> visualFeedbackSystem;
    std::unique_ptr<AudioFeedbackSystem> audioFeedbackSystem;
    std::unique_ptr<HUDAlertSystem> hudAlertSystem;
    ShipAssemblyResult hudShipAssembly;
    
    // Mouse look offsets for target lock mode
    double mouseLookYawOffset;
    double mouseLookPitchOffset;
    
    // Target lock transition smoothing
    CameraFollowConfig cameraFollowConfig;
    CameraFollowState cameraFollowState;
    std::array<CameraPreset, 3> cameraPresets;

    void ApplyCameraPreset(size_t index);
};

#endif // MAIN_LOOP_H
