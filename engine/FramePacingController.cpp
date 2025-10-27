#include "FramePacingController.h"

#include <algorithm>
#include <cmath>

FramePacingController::FramePacingController() = default;

void FramePacingController::SetVSyncEnabled(bool enabled) {
    settings_.vsyncEnabled = enabled;
}

void FramePacingController::ToggleVSync() {
    settings_.vsyncEnabled = !settings_.vsyncEnabled;
}

void FramePacingController::SetTargetFPS(double fps) {
    if (std::isnan(fps) || std::isinf(fps)) {
        return;
    }

    const double clamped = std::clamp(fps, 0.0, 360.0);
    settings_.targetFPS = clamped;
}

void FramePacingController::AdjustTargetFPS(double delta) {
    SetTargetFPS(settings_.targetFPS + delta);
}

std::chrono::duration<double> FramePacingController::DesiredFrameDuration() const {
    if (settings_.targetFPS <= 0.0) {
        return std::chrono::duration<double>(0.0);
    }

    return std::chrono::duration<double>(1.0 / settings_.targetFPS);
}

