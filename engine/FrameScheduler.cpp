#include "FrameScheduler.h"

#include <thread>

FrameScheduler::FrameScheduler(FrameSchedulerConfig config) : config_(config) {}

void FrameScheduler::Run(const FrameSchedulerCallbacks& callbacks) {
    using clock = std::chrono::high_resolution_clock;

    const auto fixedDt = config_.fixedUpdateHz > 0.0
        ? std::chrono::duration<double>(1.0 / config_.fixedUpdateHz)
        : std::chrono::duration<double>(0.0);
    const auto minFrameTime = config_.maxRenderHz > 0.0
        ? std::chrono::duration<double>(1.0 / config_.maxRenderHz)
        : std::chrono::duration<double>(0.0);

    auto previous = clock::now();
    std::chrono::duration<double> lag(0.0);

    auto shouldContinue = [&]() {
        return !callbacks.shouldContinue || callbacks.shouldContinue();
    };

    while (true) {
        if (!shouldContinue()) {
            break;
        }

        auto frameStart = clock::now();
        auto elapsed = frameStart - previous;
        previous = frameStart;
        lag += elapsed;
        double deltaSeconds = elapsed.count();

        if (callbacks.onFrameStart) {
            callbacks.onFrameStart(deltaSeconds);
        }

        while (lag >= fixedDt) {
            if (callbacks.onFixedUpdate) {
                callbacks.onFixedUpdate(fixedDt.count());
            }
            lag -= fixedDt;
        }

        double interpolation = (fixedDt.count() > 0.0 && callbacks.onRender)
            ? lag.count() / fixedDt.count()
            : 0.0;

        if (callbacks.onRender) {
            callbacks.onRender(interpolation);
        }

        auto frameEnd = clock::now();
        if (callbacks.onFrameComplete) {
            callbacks.onFrameComplete(FrameSchedulerFrameInfo{deltaSeconds, frameStart, frameEnd});
        }

        if (config_.maxRenderHz > 0.0) {
            auto frameTime = frameEnd - frameStart;
            if (frameTime < minFrameTime) {
                std::this_thread::sleep_for(minFrameTime - frameTime);
            }
        }

        if (!shouldContinue()) {
            break;
        }
    }
}
