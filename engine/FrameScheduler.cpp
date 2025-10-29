#include "FrameScheduler.h"

#include <cmath>
#include <thread>

namespace {

std::size_t ResolveHistoryLimit(const FrameSchedulerConfig& config) {
    return config.timingHistorySize == 0 ? 1 : config.timingHistorySize;
}

} // namespace

FrameScheduler::FrameScheduler(FrameSchedulerConfig config) : config_(config) {}

void FrameScheduler::SetMaxRenderHz(double hz) {
    if (std::isnan(hz) || std::isinf(hz)) {
        return;
    }
    config_.maxRenderHz = hz < 0.0 ? 0.0 : hz;
}

void FrameScheduler::Run(const FrameSchedulerCallbacks& callbacks) {
    using clock = std::chrono::high_resolution_clock;

    timingHistory_.clear();
    frameDurationHistory_.clear();
    rollingStageSums_ = FrameStageDurations{};
    frameDurationSum_ = 0.0;
    rollingAverages_ = FrameTimingAverages{};
    lastStageDurations_ = FrameStageDurations{};
    lastFrameDurationSeconds_ = 0.0;

    auto previous = clock::now();
    std::chrono::duration<double> lag(0.0);

    const auto resolveFixedDt = [&]() {
        return config_.fixedUpdateHz > 0.0
            ? std::chrono::duration<double>(1.0 / config_.fixedUpdateHz)
            : std::chrono::duration<double>(0.0);
    };

    auto shouldContinue = [&]() {
        return !callbacks.shouldContinue || callbacks.shouldContinue();
    };

    while (true) {
        if (!shouldContinue()) {
            break;
        }

        const auto fixedDt = resolveFixedDt();
        const auto desiredFrameDuration = config_.maxRenderHz > 0.0
            ? std::chrono::duration<double>(1.0 / config_.maxRenderHz)
            : std::chrono::duration<double>(0.0);

        auto frameStart = clock::now();
        auto elapsed = frameStart - previous;
        previous = frameStart;
        lag += elapsed;
        double deltaSeconds = elapsed.count();

        FrameStageDurations stageDurations;

        if (callbacks.onFrameStart) {
            auto stageStart = clock::now();
            callbacks.onFrameStart(deltaSeconds);
            stageDurations.inputSeconds += std::chrono::duration<double>(clock::now() - stageStart).count();
        }

        if (fixedDt.count() > 0.0) {
            while (lag >= fixedDt) {
                if (callbacks.onFixedUpdate) {
                    auto stageStart = clock::now();
                    callbacks.onFixedUpdate(fixedDt.count());
                    stageDurations.simulationSeconds += std::chrono::duration<double>(clock::now() - stageStart).count();
                }
                lag -= fixedDt;
            }
        } else {
            lag = std::chrono::duration<double>::zero();
        }

        double interpolation = (fixedDt.count() > 0.0 && callbacks.onRender)
            ? lag.count() / fixedDt.count()
            : 0.0;

        if (callbacks.onRender) {
            auto stageStart = clock::now();
            callbacks.onRender(interpolation);
            stageDurations.renderPrepSeconds += std::chrono::duration<double>(clock::now() - stageStart).count();
        }

        auto afterRender = clock::now();

        std::chrono::duration<double> sleepDuration(0.0);
        if (desiredFrameDuration.count() > 0.0) {
            auto activeDuration = afterRender - frameStart;
            if (activeDuration < desiredFrameDuration) {
                sleepDuration = desiredFrameDuration - activeDuration;
                std::this_thread::sleep_for(sleepDuration);
            }
        }
        stageDurations.presentSeconds += sleepDuration.count();

        auto frameEnd = clock::now();
        double frameDurationSeconds = std::chrono::duration<double>(frameEnd - frameStart).count();

        lastStageDurations_ = stageDurations;
        lastFrameDurationSeconds_ = frameDurationSeconds;

        const std::size_t historyLimit = ResolveHistoryLimit(config_);
        timingHistory_.push_back(stageDurations);
        frameDurationHistory_.push_back(frameDurationSeconds);
        rollingStageSums_.inputSeconds += stageDurations.inputSeconds;
        rollingStageSums_.simulationSeconds += stageDurations.simulationSeconds;
        rollingStageSums_.renderPrepSeconds += stageDurations.renderPrepSeconds;
        rollingStageSums_.presentSeconds += stageDurations.presentSeconds;
        frameDurationSum_ += frameDurationSeconds;

        while (timingHistory_.size() > historyLimit) {
            const FrameStageDurations& oldest = timingHistory_.front();
            rollingStageSums_.inputSeconds -= oldest.inputSeconds;
            rollingStageSums_.simulationSeconds -= oldest.simulationSeconds;
            rollingStageSums_.renderPrepSeconds -= oldest.renderPrepSeconds;
            rollingStageSums_.presentSeconds -= oldest.presentSeconds;
            timingHistory_.pop_front();
        }
        while (frameDurationHistory_.size() > historyLimit) {
            frameDurationSum_ -= frameDurationHistory_.front();
            frameDurationHistory_.pop_front();
        }

        const std::size_t sampleCount = timingHistory_.size();
        if (sampleCount > 0) {
            const double invCount = 1.0 / static_cast<double>(sampleCount);
            rollingAverages_.stage.inputSeconds = rollingStageSums_.inputSeconds * invCount;
            rollingAverages_.stage.simulationSeconds = rollingStageSums_.simulationSeconds * invCount;
            rollingAverages_.stage.renderPrepSeconds = rollingStageSums_.renderPrepSeconds * invCount;
            rollingAverages_.stage.presentSeconds = rollingStageSums_.presentSeconds * invCount;
            if (!frameDurationHistory_.empty()) {
                rollingAverages_.frameSeconds = frameDurationSum_ / static_cast<double>(frameDurationHistory_.size());
            } else {
                rollingAverages_.frameSeconds = 0.0;
            }
            rollingAverages_.sampleCount = sampleCount;
        } else {
            rollingAverages_ = FrameTimingAverages{};
        }

        if (callbacks.onFrameComplete) {
            FrameSchedulerFrameInfo info{};
            info.deltaSeconds = deltaSeconds;
            info.frameStart = frameStart;
            info.frameEnd = frameEnd;
            info.stageDurations = stageDurations;
            info.rolling = rollingAverages_;
            info.frameDurationSeconds = frameDurationSeconds;
            callbacks.onFrameComplete(info);
        }

        if (!shouldContinue()) {
            break;
        }
    }
}
