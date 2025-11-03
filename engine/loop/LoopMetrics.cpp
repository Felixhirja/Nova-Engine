#include "LoopMetrics.h"
#include <numeric>
#include <cmath>

LoopMetrics::LoopMetrics(size_t historySize)
    : historySize_(historySize) {
    firstFrame_ = clock::now();
}

void LoopMetrics::BeginFrame() {
    frameStart_ = clock::now();
    currentFrame_ = FrameMetrics();
    currentFrame_.frameNumber = ++frameNumber_;
    currentFrame_.timestamp = frameStart_;
}

void LoopMetrics::EndFrame(double deltaTime) {
    auto now = clock::now();
    currentFrame_.frameTime = ToMilliseconds(now - frameStart_);
    currentFrame_.deltaTime = deltaTime * 1000.0;
    
    if (currentFrame_.frameTime > stallThresholdMs_) {
        currentStall_ = currentFrame_.frameTime;
        stallCount_++;
    } else {
        currentStall_ = 0.0;
    }
    
    history_.push_back(currentFrame_);
    
    if (history_.size() > historySize_) {
        history_.pop_front();
    }
}

void LoopMetrics::BeginUpdate() {
    updateStart_ = clock::now();
}

void LoopMetrics::EndUpdate() {
    auto now = clock::now();
    currentFrame_.updateTime = ToMilliseconds(now - updateStart_);
}

void LoopMetrics::BeginRender() {
    renderStart_ = clock::now();
}

void LoopMetrics::EndRender() {
    auto now = clock::now();
    currentFrame_.renderTime = ToMilliseconds(now - renderStart_);
}

void LoopMetrics::BeginInput() {
    inputStart_ = clock::now();
}

void LoopMetrics::EndInput() {
    auto now = clock::now();
    currentFrame_.inputTime = ToMilliseconds(now - inputStart_);
}

void LoopMetrics::RecordWaitTime(double waitTimeMs) {
    currentFrame_.waitTime = waitTimeMs;
}

LoopMetrics::PerformanceStats LoopMetrics::GetStats() const {
    PerformanceStats stats;
    
    if (history_.empty()) return stats;
    
    stats.totalFrames = static_cast<int>(history_.size());
    
    std::vector<double> frameTimes;
    frameTimes.reserve(history_.size());
    
    double sum = 0.0;
    double minTime = std::numeric_limits<double>::max();
    double maxTime = 0.0;
    
    for (const auto& frame : history_) {
        frameTimes.push_back(frame.frameTime);
        sum += frame.frameTime;
        minTime = std::min(minTime, frame.frameTime);
        maxTime = std::max(maxTime, frame.frameTime);
    }
    
    stats.avgFrameTime = sum / history_.size();
    stats.minFrameTime = minTime;
    stats.maxFrameTime = maxTime;
    
    stats.avgFPS = (stats.avgFrameTime > 0.0) ? (1000.0 / stats.avgFrameTime) : 0.0;
    stats.minFPS = (maxTime > 0.0) ? (1000.0 / maxTime) : 0.0;
    stats.maxFPS = (minTime > 0.0) ? (1000.0 / minTime) : 0.0;
    
    if (!frameTimes.empty()) {
        std::sort(frameTimes.begin(), frameTimes.end());
        size_t p95_idx = static_cast<size_t>(frameTimes.size() * 0.95);
        size_t p99_idx = static_cast<size_t>(frameTimes.size() * 0.99);
        
        if (p95_idx < frameTimes.size()) stats.percentile95 = frameTimes[p95_idx];
        if (p99_idx < frameTimes.size()) stats.percentile99 = frameTimes[p99_idx];
    }
    
    double variance = 0.0;
    for (const auto& time : frameTimes) {
        double diff = time - stats.avgFrameTime;
        variance += diff * diff;
    }
    stats.stdDeviation = std::sqrt(variance / frameTimes.size());
    
    double dropThreshold = stats.avgFrameTime * 2.0;
    for (const auto& time : frameTimes) {
        if (time > dropThreshold) {
            stats.droppedFrames++;
        }
    }
    
    stats.stallCount = stallCount_;
    
    if (!history_.empty()) {
        auto lastFrame = history_.back().timestamp;
        auto duration = lastFrame - firstFrame_;
        stats.totalRuntime = ToMilliseconds(duration) / 1000.0;
    }
    
    return stats;
}

double LoopMetrics::GetInstantFPS() const {
    if (history_.empty()) return 0.0;
    double lastFrameTime = history_.back().frameTime;
    return (lastFrameTime > 0.0) ? (1000.0 / lastFrameTime) : 0.0;
}

double LoopMetrics::GetAverageFPS(int sampleCount) const {
    if (history_.empty()) return 0.0;
    
    int count = std::min(sampleCount, static_cast<int>(history_.size()));
    double sum = 0.0;
    
    auto it = history_.rbegin();
    for (int i = 0; i < count; ++i, ++it) {
        sum += it->frameTime;
    }
    
    double avgFrameTime = sum / count;
    return (avgFrameTime > 0.0) ? (1000.0 / avgFrameTime) : 0.0;
}

void LoopMetrics::Reset() {
    history_.clear();
    frameNumber_ = 0;
    stallCount_ = 0;
    currentStall_ = 0.0;
    firstFrame_ = clock::now();
}

double LoopMetrics::ToMilliseconds(const clock::duration& duration) const {
    return std::chrono::duration<double, std::milli>(duration).count();
}
