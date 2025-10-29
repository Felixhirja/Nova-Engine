#ifndef FRAME_SCHEDULER_H
#define FRAME_SCHEDULER_H

#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>

struct FrameSchedulerConfig {
    double fixedUpdateHz = 60.0;
    double maxRenderHz = 60.0;
    std::size_t timingHistorySize = 120;
};

struct FrameStageDurations {
    double inputSeconds = 0.0;
    double simulationSeconds = 0.0;
    double renderPrepSeconds = 0.0;
    double presentSeconds = 0.0;
};

struct FrameTimingAverages {
    FrameStageDurations stage;
    double frameSeconds = 0.0;
    std::size_t sampleCount = 0;
};

struct FrameSchedulerFrameInfo {
    double deltaSeconds;
    std::chrono::high_resolution_clock::time_point frameStart;
    std::chrono::high_resolution_clock::time_point frameEnd;
    FrameStageDurations stageDurations;
    FrameTimingAverages rolling;
    double frameDurationSeconds = 0.0;
};

struct FrameSchedulerCallbacks {
    std::function<bool()> shouldContinue;
    std::function<void(double)> onFrameStart;
    std::function<void(double)> onFixedUpdate;
    std::function<void(double)> onRender;
    std::function<void(const FrameSchedulerFrameInfo&)> onFrameComplete;
};

class FrameScheduler {
public:
    explicit FrameScheduler(FrameSchedulerConfig config);

    void Run(const FrameSchedulerCallbacks& callbacks);

    void SetMaxRenderHz(double hz);

    const FrameStageDurations& LastStageDurations() const { return lastStageDurations_; }
    const FrameTimingAverages& RollingAverages() const { return rollingAverages_; }
    std::size_t TimingSampleCount() const { return rollingAverages_.sampleCount; }
    double LastFrameDurationSeconds() const { return lastFrameDurationSeconds_; }

private:
    FrameSchedulerConfig config_;
    FrameStageDurations lastStageDurations_{};
    FrameTimingAverages rollingAverages_{};
    std::deque<FrameStageDurations> timingHistory_;
    std::deque<double> frameDurationHistory_;
    FrameStageDurations rollingStageSums_{};
    double frameDurationSum_ = 0.0;
    double lastFrameDurationSeconds_ = 0.0;
};

#endif // FRAME_SCHEDULER_H
