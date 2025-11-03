#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>

/**
 * LoopMetrics - Real-time metrics collection and analysis for the game loop
 */
class LoopMetrics {
public:
    struct FrameMetrics {
        double frameTime = 0.0;
        double updateTime = 0.0;
        double renderTime = 0.0;
        double inputTime = 0.0;
        double waitTime = 0.0;
        double deltaTime = 0.0;
        uint64_t frameNumber = 0;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    struct PerformanceStats {
        double avgFPS = 0.0;
        double minFPS = 0.0;
        double maxFPS = 0.0;
        double avgFrameTime = 0.0;
        double minFrameTime = 0.0;
        double maxFrameTime = 0.0;
        double percentile95 = 0.0;
        double percentile99 = 0.0;
        double stdDeviation = 0.0;
        int totalFrames = 0;
        int droppedFrames = 0;
        int stallCount = 0;
        double totalRuntime = 0.0;
    };
    
    LoopMetrics(size_t historySize = 300);
    
    void BeginFrame();
    void EndFrame(double deltaTime);
    
    void BeginUpdate();
    void EndUpdate();
    
    void BeginRender();
    void EndRender();
    
    void BeginInput();
    void EndInput();
    
    void RecordWaitTime(double waitTimeMs);
    
    PerformanceStats GetStats() const;
    
    const std::deque<FrameMetrics>& GetHistory() const { return history_; }
    
    uint64_t GetFrameNumber() const { return frameNumber_; }
    double GetInstantFPS() const;
    double GetAverageFPS(int sampleCount = 60) const;
    
    bool IsStalled() const { return currentStall_ > stallThresholdMs_; }
    double GetStallDuration() const { return currentStall_; }
    
    void SetStallThreshold(double thresholdMs) { stallThresholdMs_ = thresholdMs; }
    
    void Reset();
    
private:
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    
    std::deque<FrameMetrics> history_;
    size_t historySize_;
    
    FrameMetrics currentFrame_;
    uint64_t frameNumber_ = 0;
    
    time_point frameStart_;
    time_point updateStart_;
    time_point renderStart_;
    time_point inputStart_;
    
    double stallThresholdMs_ = 100.0;
    double currentStall_ = 0.0;
    int stallCount_ = 0;
    
    time_point firstFrame_;
    
    double ToMilliseconds(const clock::duration& duration) const;
};
