#include "CameraFollowController.h"

#include "Camera.h"

#include <algorithm>
#include <cmath>

CameraFollowController::CameraFollowController() = default;

void CameraFollowController::SetConfig(const CameraFollowConfig& config) {
    config_ = config;
}

void CameraFollowController::ResetState() {
    state_ = CameraFollowState{};
}

void CameraFollowController::Update(Camera& camera,
                                    const CameraFollowInput& followInput,
                                    const CameraMovementInput& movementInput,
                                    double deltaTime) {
    CameraFollow::UpdateTargetLockCamera(camera, state_, config_, followInput, deltaTime);

    if (!followInput.isTargetLocked && state_.targetLockTransition <= 0.0) {
        ApplyFreeCameraMovement(camera, followInput, movementInput, deltaTime);
    }
}

void CameraFollowController::ApplyFreeCameraMovement(Camera& camera,
                                                     const CameraFollowInput& followInput,
                                                     const CameraMovementInput& movementInput,
                                                     double deltaTime) {
    if (movementInput.moveSpeed <= 0.0) {
        return;
    }

    double newCameraX = camera.x();
    double newCameraY = camera.y();
    double newCameraZ = camera.z();

    const double displacement = movementInput.moveSpeed * deltaTime;
    if (movementInput.moveForward) {
        newCameraY += displacement;
    }
    if (movementInput.moveBackward) {
        newCameraY -= displacement;
    }
    if (movementInput.moveLeft) {
        newCameraX -= displacement;
    }
    if (movementInput.moveRight) {
        newCameraX += displacement;
    }
    if (movementInput.moveUp) {
        newCameraZ += displacement;
    }
    if (movementInput.moveDown) {
        newCameraZ -= displacement;
    }

    const double playerX = followInput.playerX;
    const double playerY = followInput.playerY;
    const double playerZ = followInput.playerZ;

    const double minDistanceFromPlayer = config_.minDistanceFromPlayer;
    const double freeCamToPlayerX = newCameraX - playerX;
    const double freeCamToPlayerY = newCameraY - playerY;
    const double freeCamToPlayerZ = newCameraZ - playerZ;
    const double distanceFromPlayer = std::sqrt(freeCamToPlayerX * freeCamToPlayerX +
                                               freeCamToPlayerY * freeCamToPlayerY +
                                               freeCamToPlayerZ * freeCamToPlayerZ);

    if (distanceFromPlayer < minDistanceFromPlayer && distanceFromPlayer > 0.0) {
        const double pushFactor = minDistanceFromPlayer / distanceFromPlayer;
        newCameraX = playerX + freeCamToPlayerX * pushFactor;
        newCameraY = playerY + freeCamToPlayerY * pushFactor;
        newCameraZ = playerZ + freeCamToPlayerZ * pushFactor;
    }

    const double groundLevel = config_.groundLevel;
    if (newCameraZ < groundLevel) {
        newCameraZ = groundLevel;
    }

    camera.SetPosition(newCameraX, newCameraY, newCameraZ);
}

