#include "../engine/Camera.h"
#include "../engine/CameraFollow.h"
#include "../engine/Simulation.h"

using CameraFollow::CameraFollowConfig;
using CameraFollow::CameraFollowInput;
using CameraFollow::CameraFollowState;

#include <cmath>
#include <iostream>

namespace {

constexpr double kEpsilon = 1e-4;

bool approxLE(double value, double maxValue) {
    return value <= maxValue + kEpsilon;
}

bool approxGE(double value, double minValue) {
    return value >= minValue - kEpsilon;
}

bool isFiniteCamera(const Camera& camera) {
    return std::isfinite(camera.x()) && std::isfinite(camera.y()) &&
           std::isfinite(camera.z()) && std::isfinite(camera.pitch()) &&
           std::isfinite(camera.yaw()) && std::isfinite(camera.zoom());
}

} // namespace

int main() {
    bool allPassed = true;

    // --- Player boundary clamp test ---
    {
        Simulation sim;
        sim.Init();

        const double dt = 1.0 / 60.0;

        // Move player far in the positive direction using strafe right input
        sim.SetPlayerInput(false, false, false, false, false, true, 0.0);
        for (int i = 0; i < 600; ++i) {
            sim.Update(dt);
        }
        double xPos = sim.GetPlayerX();
        if (!approxGE(xPos, 5.0)) {
            std::cerr << "Unbounded movement test failed: expected x >= 5.0, got " << xPos << std::endl;
            allPassed = false;
        }

        // Move to the negative direction using strafe left input
        sim.SetPlayerInput(false, false, false, false, true, false, 0.0);
        for (int i = 0; i < 1200; ++i) {
            sim.Update(dt);
        }
        double xNeg = sim.GetPlayerX();
        if (!approxLE(xNeg, -5.0)) {
            std::cerr << "Unbounded movement test failed: expected x <= -5.0, got " << xNeg << std::endl;
            allPassed = false;
        }
    }

    // --- Rapid target lock toggling test ---
    {
        Camera camera(-8.0, 0.0, 6.0, -0.1, Camera::kDefaultYawRadians, 12.0);
        CameraFollowConfig config;
        CameraFollowState state;
        CameraFollowInput input;
        input.playerX = 0.0;
        input.playerY = 0.0;
        input.playerZ = 0.0;
        input.mouseLookYawOffset = 0.0;
        input.mouseLookPitchOffset = 0.0;

        const double dt = 1.0 / 120.0; // smaller step to stress-test transition smoothing

        for (int frame = 0; frame < 600; ++frame) {
            input.isTargetLocked = (frame % 3 != 0); // toggle rapidly: lock for two frames, unlock for one
            UpdateTargetLockCamera(camera, state, config, input, dt);

            if (state.targetLockTransition < -kEpsilon || state.targetLockTransition > 1.0 + kEpsilon) {
                std::cerr << "Target lock transition out of bounds at frame " << frame
                          << ": " << state.targetLockTransition << std::endl;
                allPassed = false;
                break;
            }

            if (!isFiniteCamera(camera)) {
                std::cerr << "Camera state became non-finite during rapid toggling at frame " << frame << std::endl;
                allPassed = false;
                break;
            }
        }
    }

    // --- Extreme zoom level handling test ---
    {
        Camera camera(0.0, 0.0, 1.0, 0.0, Camera::kDefaultYawRadians, 1.0);

        camera.SetTargetZoom(1e-8); // extremely small target
        camera.UpdateZoom(1.0 / 60.0);
        if (!approxGE(camera.zoom(), 1e-4)) {
            std::cerr << "Zoom lower clamp failed: zoom=" << camera.zoom() << std::endl;
            allPassed = false;
        }

        camera.SetTargetZoom(1e9); // extremely large target
        for (int i = 0; i < 600; ++i) {
            camera.UpdateZoom(1.0 / 60.0);
        }
        if (!approxLE(camera.zoom(), 10000.0)) {
            std::cerr << "Zoom upper clamp failed: zoom=" << camera.zoom() << std::endl;
            allPassed = false;
        }
    }

    if (!allPassed) {
        std::cerr << "Camera edge case tests FAILED" << std::endl;
        return 1;
    }

    std::cout << "Camera edge case tests passed" << std::endl;
    return 0;
}
