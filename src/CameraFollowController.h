#pragma once

#include "CameraFollow.h"

struct CameraMovementInput {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    double moveSpeed = 0.5;
};

class CameraFollowController {
public:
    CameraFollowController();

    void SetConfig(const CameraFollowConfig& config);
    const CameraFollowConfig& GetConfig() const { return config_; }

    const CameraFollowState& GetState() const { return state_; }
    void ResetState();

    void Update(class Camera& camera,
                const CameraFollowInput& followInput,
                const CameraMovementInput& movementInput,
                double deltaTime);

private:
    CameraFollowConfig config_;
    CameraFollowState state_;

    void ApplyFreeCameraMovement(class Camera& camera,
                                 const CameraFollowInput& followInput,
                                 const CameraMovementInput& movementInput,
                                 double deltaTime);
};

