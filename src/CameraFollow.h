#pragma once

#include <cmath>

class Camera;

struct CameraFollowConfig {
    double orbitDistance = 12.0;
    double orbitHeight = 3.0;
    double minDistanceFromPlayer = 2.0;
    double groundLevel = 0.5;
    double terrainBuffer = 1.0;
    double transitionSpeed = 3.0;
    double baseLerpFactor = 0.08;
    double adaptiveLerpScale = 0.05;
    double rotationLerpFactor = 0.15;
};

struct CameraFollowState {
    double targetLockTransition = 0.0;
    bool wasTargetLocked = false;
};

struct CameraFollowInput {
    double playerX = 0.0;
    double playerY = 0.0;
    double playerZ = 0.0;
    bool isTargetLocked = false;
    double mouseLookYawOffset = 0.0;
    double mouseLookPitchOffset = 0.0;
};

namespace CameraFollow {
    void UpdateTargetLockCamera(Camera& camera,
                                CameraFollowState& state,
                                const CameraFollowConfig& config,
                                const CameraFollowInput& input,
                                double deltaTime);
}
