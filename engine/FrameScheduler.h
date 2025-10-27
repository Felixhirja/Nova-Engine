#ifndef FRAME_SCHEDULER_H
#define FRAME_SCHEDULER_H

#include <functional>
#include <chrono>

struct FrameSchedulerConfig {
    double fixedUpdateHz = 60.0;
    double maxRenderHz = 60.0;
};

struct FrameSchedulerFrameInfo {
    double deltaSeconds;
    std::chrono::high_resolution_clock::time_point frameStart;
    std::chrono::high_resolution_clock::time_point frameEnd;
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

private:
    FrameSchedulerConfig config_;
};

#endif // FRAME_SCHEDULER_H
