#pragma once

#include <chrono>
#include <deque>

/**
 * TimeManager - Advanced time step handling and delta time smoothing
 * Provides fixed timestep, variable timestep, and semi-fixed hybrid modes
 */
class TimeManager {
public:
    enum class TimeMode {
        Variable,      // Variable timestep (simple, can cause physics issues)
        Fixed,         // Fixed timestep with interpolation
        SemiFixed      // Fixed updates, variable render with interpolation
    };
    
    struct TimeState {
        double deltaTime = 0.0;           // Current frame delta (seconds)
        double smoothedDelta = 0.0;       // Smoothed delta for stability
        double fixedDelta = 0.016667;     // Fixed timestep (60 FPS)
        double accumulator = 0.0;         // Time accumulator for fixed updates
        double interpolation = 0.0;       // Interpolation factor [0,1]
        uint64_t updateCount = 0;         // Number of fixed updates this frame
        double timeScale = 1.0;           // Global time scale multiplier
        double unscaledTime = 0.0;        // Total unscaled time
        double scaledTime = 0.0;          // Total scaled time
    };
    
    TimeManager(TimeMode mode = TimeMode::SemiFixed, double fixedHz = 60.0);
    
    void BeginFrame();
    TimeState Update();
    
    void SetMode(TimeMode mode) { mode_ = mode; }
    TimeMode GetMode() const { return mode_; }
    
    void SetFixedHz(double hz);
    double GetFixedHz() const { return 1.0 / state_.fixedDelta; }
    
    void SetTimeScale(double scale) { state_.timeScale = scale; }
    double GetTimeScale() const { return state_.timeScale; }
    
    void SetMaxDeltaTime(double maxDelta) { maxDeltaTime_ = maxDelta; }
    
    void SetSmoothingWindow(int frames) { smoothingWindow_ = frames; }
    
    const TimeState& GetState() const { return state_; }
    
    void Pause();
    void Resume();
    bool IsPaused() const { return paused_; }
    
private:
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    
    TimeMode mode_;
    TimeState state_;
    
    time_point lastFrameTime_;
    time_point frameStartTime_;
    
    std::deque<double> deltaHistory_;
    int smoothingWindow_ = 10;
    
    double maxDeltaTime_ = 0.1;  // Cap at 100ms to prevent spiral of death
    bool paused_ = false;
    
    double CalculateRawDelta();
    double SmoothDelta(double rawDelta);
    void UpdateAccumulator(double deltaTime);
};
