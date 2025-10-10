#include "../src/Camera.h"
#include "../src/CameraFollow.h"
#include <cmath>
#include <iostream>

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
        CameraFollow::UpdateTargetLockCamera(camera, state, config, input, dt);
    }
}

bool verifyOffsets(const char* label,
                   const Camera& camera,
                   const CameraFollowInput& input,
                   double expectedXOffset,
                   double expectedYOffset,
                   double expectedZOffset,
                   double tolerance) {
    double offsetX = camera.x() - input.playerX;
    double offsetY = camera.y() - input.playerY;
    double offsetZ = camera.z() - input.playerZ;

    if (!approxEqual(offsetX, expectedXOffset, tolerance) ||
        !approxEqual(offsetY, expectedYOffset, tolerance) ||
        !approxEqual(offsetZ, expectedZOffset, tolerance)) {
        std::cerr << label << " failed: got offsets ("
                  << offsetX << ", " << offsetY << ", " << offsetZ
                  << ") expected approx ("
                  << expectedXOffset << ", " << expectedYOffset << ", " << expectedZOffset
                  << ") with tolerance " << tolerance << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main() {
    Camera camera(-8.0, 0.0, 6.0, -0.1, 0.0, 12.0);
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

    std::cout << "Camera follow tests passed" << std::endl;
    return 0;
}
