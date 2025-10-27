#include "CameraFollowController.h"

#include "Camera.h"
#include "CameraFollow.h"

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CameraFollowController::CameraFollowController() = default;

void CameraFollowController::SetConfig(const CameraFollow::CameraFollowConfig& config) {
    config_ = config;
}

void CameraFollowController::ResetState() {
    state_ = CameraFollow::CameraFollowState{};
}

void CameraFollowController::Update(Camera& camera,
                                    const CameraFollow::CameraFollowInput& followInput,
                                    const CameraMovementInput& movementInput,
                                    double deltaTime,
                                    physics::IPhysicsEngine* physicsEngine) {
    if (suppressNextUpdate_) {
        suppressNextUpdate_ = false;
        return;
    }

    // Handoff: zero velocity when entering target-lock
    if (followInput.isTargetLocked && !state_.wasTargetLocked) {
        state_.freeVelX = state_.freeVelY = state_.freeVelZ = 0.0;
        suppressNextUpdate_ = true; // absorb one frame of inputs if needed
    }

    UpdateTargetLockCamera(camera, state_, config_, followInput, deltaTime, physicsEngine);

    if (!followInput.isTargetLocked && state_.targetLockTransition <= 0.0) {
        ApplyFreeLookRotation(camera, movementInput, config_, deltaTime);
        ApplyFreeCameraMovement(camera, followInput, movementInput, config_, deltaTime);
    }

    state_.wasTargetLocked = followInput.isTargetLocked;
}

void CameraFollowController::ApplyFreeCameraMovement(Camera& camera,
                                                     const CameraFollow::CameraFollowInput& /*followInput*/,
                                                     const CameraMovementInput& movementInput,
                                                     const CameraFollow::CameraFollowConfig& config,
                                                     double deltaTime) {
    // Early out if no base speed
    if (movementInput.moveSpeed <= 0.0 &&
        config.moveSpeedHorizontal <= 0.0 &&
        config.moveSpeedVertical   <= 0.0) return;

    const double dt = std::clamp(deltaTime, 0.0, config.maxDeltaTimeClamp);

    // Sprint/slow factor
    const double speedFactor =
        (movementInput.sprint ? config.sprintMultiplier : 1.0) *
        (movementInput.slow   ? 0.5                      : 1.0);

    // --- Build camera basis (Y up, forward ≈ -Z) ---
    const double yaw   = camera.yaw();
    const double pitch = camera.pitch();

    const Camera::Basis basis = camera.BuildBasis(config.pitchAffectsForward);
    const double fwdX = basis.forwardX;
    const double fwdY = basis.forwardY;
    const double fwdZ = basis.forwardZ;
    const double rightX = basis.rightX;
    const double rightY = basis.rightY;
    const double rightZ = basis.rightZ;

    // Input axes (−1..+1)
    const int fwdIn   = (movementInput.moveForward ? 1 : 0) - (movementInput.moveBackward ? 1 : 0);
    const int rightIn = (movementInput.moveRight   ? 1 : 0) - (movementInput.moveLeft     ? 1 : 0);
    const int upIn    = (movementInput.moveUp      ? 1 : 0) - (movementInput.moveDown     ? 1 : 0);

    // --- Desired velocity (split horizontal XZ and vertical Y) ---
    // Horizontal from right/fwd (XZ only)
    double vxH = rightIn * rightX + fwdIn * fwdX;
    double vzH = rightIn * rightZ + fwdIn * fwdZ;

    // Normalize horizontal to avoid faster diagonals
    const double horizLen2 = vxH*vxH + vzH*vzH;
    if (horizLen2 > 0.0) {
        const double inv = 1.0 / std::sqrt(horizLen2);
        vxH *= inv; vzH *= inv;
    }

    // Vertical from world up (Y) — independent of horizontal magnitude
    const double vyV = static_cast<double>(upIn);

    // Select speeds
    const double baseH = (movementInput.moveSpeed > 0.0 ? movementInput.moveSpeed : config_.moveSpeedHorizontal);
    const double baseV = (movementInput.moveSpeed > 0.0 ? movementInput.moveSpeed : config_.moveSpeedVertical);

    // Scale by speeds and sprint/slow
    double desVelX = vxH * baseH * speedFactor;
    double desVelZ = vzH * baseH * speedFactor;
    double desVelY = vyV * baseV * speedFactor;

    // Exponential velocity smoothing
    auto expAlpha = [](double hz, double dtSec){
        const double lambda = std::max(0.0, hz);
        return 1.0 - std::exp(-lambda * std::max(0.0, dtSec));
    };
    const double velAlpha = std::clamp(expAlpha(config.freeAccelHz, dt), 0.0, 1.0);

    // If no input, gently damp toward zero to kill drift
    const bool noInput = (fwdIn == 0) && (rightIn == 0) && (upIn == 0);
    if (noInput) {
        state_.freeVelX += (-state_.freeVelX) * velAlpha;
        state_.freeVelY += (-state_.freeVelY) * velAlpha;
        state_.freeVelZ += (-state_.freeVelZ) * velAlpha;
    } else {
        state_.freeVelX += (desVelX - state_.freeVelX) * velAlpha;
        state_.freeVelY += (desVelY - state_.freeVelY) * velAlpha;
        state_.freeVelZ += (desVelZ - state_.freeVelZ) * velAlpha;
    }

    // Deadzone snap
    auto snapZero = [&](double& v){ if (std::abs(v) < config.freeVelDeadzone) v = 0.0; };
    snapZero(state_.freeVelX);
    snapZero(state_.freeVelY);
    snapZero(state_.freeVelZ);

    // Integrate
    camera.SetPosition(camera.x() + state_.freeVelX * dt,
                       camera.y() + state_.freeVelY * dt,
                       camera.z() + state_.freeVelZ * dt);
}

void CameraFollowController::ApplyFreeLookRotation(Camera& camera,
                                                    const CameraMovementInput& movementInput,
                                                    const CameraFollow::CameraFollowConfig& config,
                                                    double /*deltaTime*/) {
    // Sensitivity: radians per pixel
    const double sensYaw   = config.freeLookSensYaw;
    const double sensPitch = config.freeLookSensPitch;

    // Optional: tiny deadzone on mouse input to stop shimmer
    auto deadzone = [](double v, double dz){ return (std::abs(v) < dz) ? 0.0 : v; };
    const double dzPx = 0.2; // tweak
    double dx = deadzone(movementInput.mouseDeltaX, dzPx);
    double dy = deadzone(movementInput.mouseDeltaY, dzPx);

    // Get current orientation
    double yaw   = camera.yaw();
    double pitch = camera.pitch();

    // Apply mouse deltas (note: mouseDeltaY typically positive downward, so invert for pitch)
    const double yawSign   = config.invertFreeLookYaw   ? -1.0 : 1.0;
    const double pitchSign = config.invertFreeLookPitch ? -1.0 : 1.0;

    yaw   += yawSign   * dx * sensYaw;
    double pitchDelta = -dy * sensPitch;
    pitch += pitchSign * pitchDelta;

    // Wrap yaw to avoid unbounded growth
    yaw = std::remainder(yaw, 2.0 * M_PI);

    // Clamp pitch to prevent gimbal lock (approx -89° to +89°)
    const double maxPitch = 1.55334; // ~89° in radians
    pitch = std::clamp(pitch, -maxPitch, maxPitch);

    // Set new orientation
    camera.SetOrientation(pitch, yaw);
}



