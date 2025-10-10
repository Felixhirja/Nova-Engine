#include "CameraFollow.h"
#include "Camera.h"
#include <algorithm>

namespace CameraFollow {

void UpdateTargetLockCamera(Camera& camera,
                            CameraFollowState& state,
                            const CameraFollowConfig& config,
                            const CameraFollowInput& input,
                            double deltaTime) {
    const double PI = std::acos(-1.0);

    if (input.isTargetLocked != state.wasTargetLocked) {
        state.targetLockTransition = 0.0;
        state.wasTargetLocked = input.isTargetLocked;
    }

    if (input.isTargetLocked) {
        state.targetLockTransition = std::min(1.0, state.targetLockTransition + deltaTime * config.transitionSpeed);
    } else {
        state.targetLockTransition = std::max(0.0, state.targetLockTransition - deltaTime * config.transitionSpeed);
    }

    if (!(input.isTargetLocked || state.targetLockTransition > 0.0)) {
        return;
    }

    const double playerX = input.playerX;
    const double playerY = input.playerY;
    const double playerZ = input.playerZ;

    double orbitAngle = input.mouseLookYawOffset * 0.5;

    double targetCameraX = playerX + config.orbitDistance * std::sin(orbitAngle);
    double targetCameraY = playerY + config.orbitDistance * std::cos(orbitAngle);
    double targetCameraZ = playerZ + config.orbitHeight + input.mouseLookPitchOffset * 2.0;

    double camToPlayerX = targetCameraX - playerX;
    double camToPlayerY = targetCameraY - playerY;
    double camToPlayerZ = targetCameraZ - playerZ;
    double distanceFromPlayer = std::sqrt(camToPlayerX * camToPlayerX + camToPlayerY * camToPlayerY + camToPlayerZ * camToPlayerZ);

    if (distanceFromPlayer < config.minDistanceFromPlayer && distanceFromPlayer > 0.0) {
        double pullFactor = config.minDistanceFromPlayer / distanceFromPlayer;
        targetCameraX = playerX + camToPlayerX * pullFactor;
        targetCameraY = playerY + camToPlayerY * pullFactor;
        targetCameraZ = playerZ + camToPlayerZ * pullFactor;
    }

    if (targetCameraZ < config.groundLevel) {
        targetCameraZ = config.groundLevel;
    }

    double terrainFloor = config.terrainBuffer;
    if (targetCameraZ < terrainFloor) {
        targetCameraZ = terrainFloor;
    }

    double currentCameraX = camera.x();
    double currentCameraY = camera.y();
    double currentCameraZ = camera.z();

    double distanceToTarget = std::sqrt(
        (targetCameraX - currentCameraX) * (targetCameraX - currentCameraX) +
        (targetCameraY - currentCameraY) * (targetCameraY - currentCameraY) +
        (targetCameraZ - currentCameraZ) * (targetCameraZ - currentCameraZ)
    );

    double adaptiveFactor = std::min(1.0, distanceToTarget * 0.1);
    double positionLerpFactor = config.baseLerpFactor + adaptiveFactor * config.adaptiveLerpScale;

    double newCameraX = currentCameraX + (targetCameraX - currentCameraX) * positionLerpFactor;
    double newCameraY = currentCameraY + (targetCameraY - currentCameraY) * positionLerpFactor;
    double newCameraZ = currentCameraZ + (targetCameraZ - currentCameraZ) * positionLerpFactor;

    camera.SetPosition(newCameraX, newCameraY, newCameraZ);

    double dx = playerX - newCameraX;
    double dy = playerY - newCameraY;
    double dz = playerZ - newCameraZ;

    double targetYaw = std::atan2(dx, dy);
    double horizontalDistance = std::sqrt(dx * dx + dy * dy);
    double targetPitch = -std::atan2(dz, horizontalDistance) - 0.2;

    double currentYaw = camera.yaw();
    double currentPitch = camera.pitch();

    double yawDiff = targetYaw - currentYaw;
    while (yawDiff > PI) yawDiff -= 2 * PI;
    while (yawDiff < -PI) yawDiff += 2 * PI;

    double rotationLerpFactor = config.rotationLerpFactor;
    double newYaw = currentYaw + yawDiff * rotationLerpFactor;
    double newPitch = currentPitch + (targetPitch - currentPitch) * rotationLerpFactor;

    if (newPitch > PI/2 - 0.1) newPitch = PI/2 - 0.1;
    if (newPitch < -PI/2 + 0.1) newPitch = -PI/2 + 0.1;

    camera.SetOrientation(newPitch, newYaw);
}

} // namespace CameraFollow
