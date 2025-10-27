#pragma once

#include "CameraFollow.h"
#include "physics/PhysicsEngine.h"

struct CameraMovementInput {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    double moveSpeed = 0.5;
    bool sprint = false;
    bool slow = false;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
};

class CameraFollowController {
public:
    CameraFollowController();

    void SetConfig(const CameraFollow::CameraFollowConfig& config);
    const CameraFollow::CameraFollowConfig& GetConfig() const { return config_; }

    const CameraFollow::CameraFollowState& GetState() const { return state_; }
    void ResetState();
    void SuppressNextUpdate() { suppressNextUpdate_ = true; }

    void Update(class Camera& camera,
                const CameraFollow::CameraFollowInput& followInput,
                const CameraMovementInput& movementInput,
                double deltaTime,
                physics::IPhysicsEngine* physicsEngine = nullptr);

private:
    CameraFollow::CameraFollowConfig config_;
    CameraFollow::CameraFollowState state_;
    bool suppressNextUpdate_ = false;

    void ApplyFreeLookRotation(class Camera& camera,
                               const CameraMovementInput& movementInput,
                               const CameraFollow::CameraFollowConfig& config,
                               double deltaTime);

    void ApplyFreeCameraMovement(class Camera& camera,
                                 const CameraFollow::CameraFollowInput& followInput,
                                 const CameraMovementInput& movementInput,
                                 const CameraFollow::CameraFollowConfig& config,
                                 double deltaTime);
};

