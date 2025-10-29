#include "../engine/Camera.h"
#include "../engine/CameraFollow.h"
#include "../engine/CameraFollowController.h"
#include <cmath>
#include <iostream>

using CameraFollow::CameraFollowConfig;
using CameraFollow::CameraFollowInput;
using CameraFollow::CameraFollowState;

namespace {

bool approxEqual(double a, double b, double tol) {
    return std::abs(a - b) <= tol;
}

void stepFrames(Camera& camera,
                CameraFollowState& state,
                const CameraFollowConfig& config,
                CameraFollowInput& input,
                double dt,
                int frames) {
    for (int i = 0; i < frames; ++i) {
        UpdateTargetLockCamera(camera, state, config, input, dt);
    }
}

bool verifyOffsets(const char* label,
                   const Camera& camera,
                   const CameraFollowInput& input,
                   double expectedPlanarDistance,
                   double expectedOrbitDistance,
                   double expectedHeight,
                   double tolerance) {
    double offsetX = camera.x() - input.playerX;
    double offsetY = camera.y() - input.playerY;
    double offsetZ = camera.z() - input.playerZ;

    double planarDistance = std::sqrt(offsetX * offsetX + offsetZ * offsetZ);

    const bool planarOk = approxEqual(planarDistance, expectedOrbitDistance, tolerance);
    const bool heightOk = (offsetY + tolerance >= expectedHeight);

    if (!planarOk || !heightOk) {
        std::cerr << label << " failed: planar distance=" << planarDistance
                  << " (expected " << expectedOrbitDistance << ") height=" << offsetY
                  << " (minimum " << expectedHeight << ")" << std::endl;
        return false;
    }
    (void)expectedPlanarDistance; // legacy parameter kept for compatibility
    return true;
}

} // namespace

int main() {
    Camera camera(-8.0, 0.0, 6.0, -0.1, Camera::kDefaultYawRadians, 12.0);
    CameraFollowState state;
    CameraFollowConfig config;
    CameraFollowInput input;
    input.isTargetLocked = true;

    const double dt = 1.0 / 60.0;

    // Warm up to allow transition to reach target lock fully
    stepFrames(camera, state, config, input, dt, 180);
    if (state.targetLockTransition < 0.99) {
        std::cerr << "Target lock transition did not reach steady state: " << state.targetLockTransition << std::endl;
        return 1;
    }

    const double tolerance = 0.6;

    if (!verifyOffsets("Initial follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 2;
    }

    // Move forward (positive Y)
    input.playerY += 5.0;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Forward follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 3;
    }

    // Move backward (negative Y)
    input.playerY = -5.0;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Backward follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 4;
    }

    // Strafe right (positive X)
    input.playerX = 4.0;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Right strafe follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 5;
    }

    // Strafe left (negative X)
    input.playerX = -4.0;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Left strafe follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 6;
    }

    // Move up (positive Z)
    input.playerZ = 2.5;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Upward follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 7;
    }

    // Move down (negative Z)
    input.playerZ = -1.5;
    stepFrames(camera, state, config, input, dt, 180);
    if (!verifyOffsets("Downward follow", camera, input, 0.0, config.orbitDistance, config.orbitHeight, tolerance)) {
        return 8;
    }

    // --- Free camera movement validation ---
    {
        CameraFollowController controller;
        CameraFollow::CameraFollowConfig freeConfig;
        freeConfig.transitionSpeed = 0.0;
        freeConfig.posResponsiveness = 0.0;
        freeConfig.rotResponsiveness = 0.0;
        freeConfig.minDistanceFromPlayer = 0.0;
        freeConfig.softGroundClamp = false;
        freeConfig.groundLevel = -1000.0;
        freeConfig.terrainBuffer = 0.0;
        freeConfig.moveSpeedHorizontal = 6.0;
        freeConfig.moveSpeedVertical = 6.0;
        freeConfig.freeAccelHz = 120.0;
        freeConfig.freeVelDeadzone = 0.0;
        freeConfig.pitchBias = 0.0;
        freeConfig.clampPitch = false;
        freeConfig.alwaysTickFreeMode = true;
        freeConfig.orbitDistance = 0.0;
        freeConfig.orbitHeight = 0.0;
        controller.SetConfig(freeConfig);
        controller.ResetState();

        Camera freeCamera(0.0, 0.0, 0.0, 0.0, Camera::kDefaultYawRadians, Camera::kDefaultFovDegrees);
        CameraFollow::CameraFollowInput freeInput;
        freeInput.isTargetLocked = false;

        CameraMovementInput moveInput{};
        moveInput.moveSpeed = 6.0;
        const double dt = 1.0 / 60.0;

        auto approx = [](double value, double expected, double tolerance) {
            return std::abs(value - expected) <= tolerance;
        };

        moveInput.moveForward = true;
        for (int i = 0; i < 120; ++i) {
            controller.Update(freeCamera, freeInput, moveInput, dt, nullptr);
        }
        if (!(freeCamera.x() > 5.0 && approx(freeCamera.z(), 0.0, 0.5) && approx(freeCamera.y(), 0.0, 0.25))) {
            std::cerr << "Free camera forward movement failed: position ("
                      << freeCamera.x() << ", " << freeCamera.y() << ", " << freeCamera.z() << ")" << std::endl;
            return 9;
        }

        controller.ResetState();
        freeCamera.SetPosition(0.0, 0.0, 0.0);
        freeCamera.SetOrientation(0.0, Camera::kDefaultYawRadians);

        moveInput.moveForward = false;
        moveInput.moveRight = true;
        for (int i = 0; i < 120; ++i) {
            controller.Update(freeCamera, freeInput, moveInput, dt, nullptr);
        }
        if (!(freeCamera.z() < -5.0 && approx(freeCamera.x(), 0.0, 0.5) && approx(freeCamera.y(), 0.0, 0.25))) {
            std::cerr << "Free camera strafe movement failed: position ("
                      << freeCamera.x() << ", " << freeCamera.y() << ", " << freeCamera.z() << ")" << std::endl;
            return 10;
        }
    }

    std::cout << "Camera follow tests passed" << std::endl;
    return 0;
}
