#include "TimeManager.h"
#include <algorithm>
#include <numeric>

TimeManager::TimeManager(TimeMode mode, double fixedHz)
    : mode_(mode) {
    SetFixedHz(fixedHz);
    lastFrameTime_ = clock::now();
}

void TimeManager::BeginFrame() {
    frameStartTime_ = clock::now();
}

TimeManager::TimeState TimeManager::Update() {
    double rawDelta = CalculateRawDelta();
    
    if (paused_) {
        rawDelta = 0.0;
    }
    
    rawDelta = std::min(rawDelta, maxDeltaTime_);
    
    state_.deltaTime = rawDelta;
    state_.smoothedDelta = SmoothDelta(rawDelta);
    
    double effectiveDelta = state_.smoothedDelta * state_.timeScale;
    
    state_.unscaledTime += state_.smoothedDelta;
    state_.scaledTime += effectiveDelta;
    
    state_.updateCount = 0;
    state_.interpolation = 0.0;
    
    switch (mode_) {
        case TimeMode::Variable:
            state_.updateCount = 1;
            break;
            
        case TimeMode::Fixed:
            UpdateAccumulator(state_.smoothedDelta);
            break;
            
        case TimeMode::SemiFixed:
            UpdateAccumulator(effectiveDelta);
            break;
    }
    
    lastFrameTime_ = frameStartTime_;
    
    return state_;
}

void TimeManager::SetFixedHz(double hz) {
    if (hz > 0.0) {
        state_.fixedDelta = 1.0 / hz;
    }
}

void TimeManager::Pause() {
    paused_ = true;
}

void TimeManager::Resume() {
    paused_ = false;
    lastFrameTime_ = clock::now();
}

double TimeManager::CalculateRawDelta() {
    auto duration = frameStartTime_ - lastFrameTime_;
    return std::chrono::duration<double>(duration).count();
}

double TimeManager::SmoothDelta(double rawDelta) {
    deltaHistory_.push_back(rawDelta);
    
    if (static_cast<int>(deltaHistory_.size()) > smoothingWindow_) {
        deltaHistory_.pop_front();
    }
    
    if (deltaHistory_.empty()) {
        return rawDelta;
    }
    
    double sum = std::accumulate(deltaHistory_.begin(), deltaHistory_.end(), 0.0);
    return sum / deltaHistory_.size();
}

void TimeManager::UpdateAccumulator(double deltaTime) {
    state_.accumulator += deltaTime;
    
    const int maxUpdates = 5;
    int updates = 0;
    
    while (state_.accumulator >= state_.fixedDelta && updates < maxUpdates) {
        state_.accumulator -= state_.fixedDelta;
        state_.updateCount++;
        updates++;
    }
    
    if (state_.fixedDelta > 0.0) {
        state_.interpolation = state_.accumulator / state_.fixedDelta;
    }
}
