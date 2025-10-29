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

void FramePacingController::UpdateFromTimings(const FrameTimingAverages& timing) {
    averageStageDurations_ = timing.stage;
    averageFrameDuration_ = timing.frameSeconds;
    averageSampleCount_ = timing.sampleCount;

    if (timing.sampleCount < 5) {
        return;
    }

    const double activeTime = averageStageDurations_.inputSeconds +
        averageStageDurations_.simulationSeconds +
        averageStageDurations_.renderPrepSeconds;
    const double frameDuration = averageFrameDuration_ > 0.0
        ? averageFrameDuration_
        : activeTime + averageStageDurations_.presentSeconds;

    if (!std::isfinite(activeTime) || !std::isfinite(frameDuration) || frameDuration <= 0.0) {
        return;
    }

    const double idleTime = std::max(0.0, frameDuration - activeTime);
    const double idleRatio = frameDuration > 0.0 ? idleTime / frameDuration : 0.0;

    if (settings_.targetFPS <= 0.0 && !settings_.vsyncEnabled) {
        return;
    }

    const double desiredDuration = DesiredFrameDuration().count();

    if (settings_.vsyncEnabled) {
        if (desiredDuration > 0.0 && activeTime > desiredDuration * 0.95 && idleRatio < 0.05) {
            settings_.vsyncEnabled = false;
        } else {
            if (frameDuration > 0.0 && std::isfinite(frameDuration)) {
                const double measuredFPS = 1.0 / frameDuration;
                const double smoothing = 0.1;
                const double clampedMeasured = std::clamp(measuredFPS, 30.0, 360.0);
                settings_.targetFPS = (1.0 - smoothing) * settings_.targetFPS + smoothing * clampedMeasured;
            }
            return;
        }
    } else {
        if (idleRatio > 0.25) {
            settings_.vsyncEnabled = true;
            if (frameDuration > 0.0 && std::isfinite(frameDuration)) {
                settings_.targetFPS = std::clamp(1.0 / frameDuration, 30.0, 360.0);
            }
            return;
        }
    }

    if (settings_.targetFPS <= 0.0) {
        const double fallback = frameDuration > 0.0 ? 1.0 / frameDuration : 60.0;
        settings_.targetFPS = std::clamp(fallback, 30.0, 360.0);
    }

    double recommendedDuration = activeTime * 1.10;
    if (desiredDuration > 0.0) {
        const double minClamp = desiredDuration * 0.5;
        const double maxClamp = desiredDuration * 1.5;
        recommendedDuration = std::clamp(recommendedDuration, minClamp, maxClamp);
    }

    if (!std::isfinite(recommendedDuration) || recommendedDuration <= 0.0) {
        return;
    }

    double recommendedFPS = 1.0 / recommendedDuration;
    recommendedFPS = std::clamp(recommendedFPS, 30.0, 360.0);

    const double smoothing = 0.15;
    settings_.targetFPS = (1.0 - smoothing) * settings_.targetFPS + smoothing * recommendedFPS;
}

