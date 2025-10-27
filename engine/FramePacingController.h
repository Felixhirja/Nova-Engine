#pragma once

#include <chrono>

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

private:
    FramePacingSettings settings_;
};

