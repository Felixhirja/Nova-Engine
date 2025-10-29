#pragma once

#include <chrono>

#include "FrameScheduler.h"

struct FramePacingSettings {
    bool vsyncEnabled = false;
    double targetFPS = 144.0;
};

class FramePacingController {
public:
    FramePacingController();

    const FramePacingSettings& Settings() const { return settings_; }

    void SetVSyncEnabled(bool enabled);
    void ToggleVSync();
    bool IsVSyncEnabled() const { return settings_.vsyncEnabled; }

    void SetTargetFPS(double fps);
    void AdjustTargetFPS(double delta);
    double TargetFPS() const { return settings_.targetFPS; }

    std::chrono::duration<double> DesiredFrameDuration() const;

    void UpdateFromTimings(const FrameTimingAverages& timing);

    const FrameStageDurations& AverageStageDurations() const { return averageStageDurations_; }
    double AverageFrameDuration() const { return averageFrameDuration_; }
    std::size_t AverageSampleCount() const { return averageSampleCount_; }

private:
    FramePacingSettings settings_;
    FrameStageDurations averageStageDurations_{};
    double averageFrameDuration_ = 0.0;
    std::size_t averageSampleCount_ = 0;
};

