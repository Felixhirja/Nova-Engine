#include "ActorLifecycle.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

namespace ActorLifecycle {

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

const char* StateToString(LifecycleState state) {
    switch (state) {
        case LifecycleState::Uninitialized: return "Uninitialized";
        case LifecycleState::Initializing: return "Initializing";
        case LifecycleState::Active: return "Active";
        case LifecycleState::Paused: return "Paused";
        case LifecycleState::Destroying: return "Destroying";
        case LifecycleState::Destroyed: return "Destroyed";
        default: return "Unknown";
    }
}

bool IsValidTransition(LifecycleState from, LifecycleState to) {
    // Define valid state transitions
    switch (from) {
        case LifecycleState::Uninitialized:
            return to == LifecycleState::Initializing;
        
        case LifecycleState::Initializing:
            return to == LifecycleState::Active || to == LifecycleState::Destroyed;
        
        case LifecycleState::Active:
            return to == LifecycleState::Paused || to == LifecycleState::Destroying;
        
        case LifecycleState::Paused:
            return to == LifecycleState::Active || to == LifecycleState::Destroying;
        
        case LifecycleState::Destroying:
            return to == LifecycleState::Destroyed;
        
        case LifecycleState::Destroyed:
            return false; // No transitions from destroyed state
        
        default:
            return false;
    }
}

// ============================================================================
// LIFECYCLE MANAGER
// ============================================================================

LifecycleManager& LifecycleManager::Instance() {
    static LifecycleManager instance;
    return instance;
}

void LifecycleManager::RegisterPreCreateHook(LifecycleHook hook) {
    hooks_.onPreCreate.push_back(hook);
}

void LifecycleManager::RegisterPostCreateHook(LifecycleHook hook) {
    hooks_.onPostCreate.push_back(hook);
}

void LifecycleManager::RegisterPreInitializeHook(LifecycleHook hook) {
    hooks_.onPreInitialize.push_back(hook);
}

void LifecycleManager::RegisterPostInitializeHook(LifecycleHook hook) {
    hooks_.onPostInitialize.push_back(hook);
}

void LifecycleManager::RegisterPreDestroyHook(LifecycleHook hook) {
    hooks_.onPreDestroy.push_back(hook);
}

void LifecycleManager::RegisterPostDestroyHook(LifecycleHook hook) {
    hooks_.onPostDestroy.push_back(hook);
}

void LifecycleManager::RegisterStateChangeHook(LifecycleHook hook) {
    hooks_.onStateChange.push_back(hook);
}

void LifecycleManager::RegisterPauseHook(LifecycleHook hook) {
    hooks_.onPause.push_back(hook);
}

void LifecycleManager::RegisterResumeHook(LifecycleHook hook) {
    hooks_.onResume.push_back(hook);
}

void LifecycleManager::RegisterErrorHook(LifecycleHook hook) {
    hooks_.onError.push_back(hook);
}

void LifecycleManager::ClearAllHooks() {
    hooks_.Clear();
}

void LifecycleManager::OnActorCreate(IActor* actor) {
    if (!actor) return;
    
    auto startTime = std::chrono::steady_clock::now();
    
    ExecuteHooks(hooks_.onPreCreate, actor, LifecycleState::Uninitialized);
    
    // Initialize lifecycle data
    ActorLifecycleData data;
    data.currentState = LifecycleState::Uninitialized;
    data.creationTime = startTime;
    data.actorName = actor->GetName();
    
    actorData_[actor] = data;
    
    // Update metrics
    globalMetrics_.totalCreated++;
    globalMetrics_.currentActive++;
    
    auto& typeMetrics = metricsByType_[actor->GetName()];
    typeMetrics.totalCreated++;
    typeMetrics.currentActive++;
    
    ExecuteHooks(hooks_.onPostCreate, actor, LifecycleState::Uninitialized);
    
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime).count();
    UpdateMetrics(actor, duration, "create");
}

void LifecycleManager::OnActorInitialize(IActor* actor) {
    if (!actor) return;
    
    auto startTime = std::chrono::steady_clock::now();
    
    ExecuteHooks(hooks_.onPreInitialize, actor, LifecycleState::Initializing);
    
    if (!TransitionState(actor, LifecycleState::Initializing)) {
        OnActorError(actor, "Invalid state transition to Initializing");
        return;
    }
    
    try {
        actor->Initialize();
        TransitionState(actor, LifecycleState::Active);
        
        if (actorData_.find(actor) != actorData_.end()) {
            actorData_[actor].initializationTime = std::chrono::steady_clock::now();
        }
    } catch (const std::exception& e) {
        OnActorError(actor, std::string("Initialization failed: ") + e.what());
        TransitionState(actor, LifecycleState::Destroyed);
    }
    
    ExecuteHooks(hooks_.onPostInitialize, actor, LifecycleState::Active);
    
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime).count();
    UpdateMetrics(actor, duration, "initialize");
}

void LifecycleManager::OnActorDestroy(IActor* actor) {
    if (!actor) return;
    
    auto startTime = std::chrono::steady_clock::now();
    
    ExecuteHooks(hooks_.onPreDestroy, actor, LifecycleState::Destroying);
    
    TransitionState(actor, LifecycleState::Destroying);
    
    // Finalize lifecycle data
    if (actorData_.find(actor) != actorData_.end()) {
        auto& data = actorData_[actor];
        data.destructionTime = std::chrono::steady_clock::now();
        data.totalLifetime = std::chrono::duration<double>(
            data.destructionTime - data.creationTime).count();
    }
    
    TransitionState(actor, LifecycleState::Destroyed);
    
    // Update metrics
    globalMetrics_.totalDestroyed++;
    if (globalMetrics_.currentActive > 0) {
        globalMetrics_.currentActive--;
    }
    
    auto& typeMetrics = metricsByType_[actor->GetName()];
    typeMetrics.totalDestroyed++;
    if (typeMetrics.currentActive > 0) {
        typeMetrics.currentActive--;
    }
    
    ExecuteHooks(hooks_.onPostDestroy, actor, LifecycleState::Destroyed);
    
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime).count();
    UpdateMetrics(actor, duration, "destroy");
    
    // Clean up lifecycle data after a delay (keep for debugging)
    // actorData_.erase(actor);
}

void LifecycleManager::OnActorUpdate(IActor* actor, double deltaTime) {
    if (!actor) return;
    
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) return;
    
    auto& data = it->second;
    if (data.currentState != LifecycleState::Active) return;
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        actor->Update(deltaTime);
        
        data.lastUpdateTime = std::chrono::steady_clock::now();
        data.updateCount++;
        
        if (trackPerformance_) {
            auto endTime = std::chrono::steady_clock::now();
            double duration = std::chrono::duration<double>(endTime - startTime).count();
            UpdateMetrics(actor, duration, "update");
        }
    } catch (const std::exception& e) {
        OnActorError(actor, std::string("Update failed: ") + e.what());
    }
}

void LifecycleManager::OnActorPause(IActor* actor) {
    if (!actor) return;
    
    ExecuteHooks(hooks_.onPause, actor, LifecycleState::Paused);
    
    if (TransitionState(actor, LifecycleState::Paused)) {
        globalMetrics_.currentPaused++;
        auto& typeMetrics = metricsByType_[actor->GetName()];
        typeMetrics.currentPaused++;
    }
}

void LifecycleManager::OnActorResume(IActor* actor) {
    if (!actor) return;
    
    ExecuteHooks(hooks_.onResume, actor, LifecycleState::Active);
    
    if (TransitionState(actor, LifecycleState::Active)) {
        if (globalMetrics_.currentPaused > 0) {
            globalMetrics_.currentPaused--;
        }
        auto& typeMetrics = metricsByType_[actor->GetName()];
        if (typeMetrics.currentPaused > 0) {
            typeMetrics.currentPaused--;
        }
    }
}

void LifecycleManager::OnActorError(IActor* actor, const std::string& error) {
    if (!actor) return;
    
    auto it = actorData_.find(actor);
    if (it != actorData_.end()) {
        it->second.lastError = error;
    }
    
    globalMetrics_.totalErrors++;
    auto& typeMetrics = metricsByType_[actor->GetName()];
    typeMetrics.totalErrors++;
    
    ExecuteHooks(hooks_.onError, actor, GetState(actor));
    
    if (LifecycleDebugger::IsDetailedLoggingEnabled()) {
        std::cerr << "[ActorLifecycle] Error in " << actor->GetName() 
                  << ": " << error << std::endl;
    }
}

bool LifecycleManager::TransitionState(IActor* actor, LifecycleState newState) {
    if (!actor) return false;
    
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) return false;
    
    auto& data = it->second;
    
    if (!IsValidTransition(data.currentState, newState)) {
        OnActorError(actor, "Invalid state transition from " + 
            std::string(StateToString(data.currentState)) + " to " + 
            std::string(StateToString(newState)));
        return false;
    }
    
    LifecycleState oldState = data.currentState;
    data.previousState = oldState;
    data.currentState = newState;
    data.stateChangeCount++;
    
    RecordStateTransition(actor, oldState, newState);
    ExecuteHooks(hooks_.onStateChange, actor, newState);
    
    return true;
}

LifecycleState LifecycleManager::GetState(IActor* actor) const {
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) {
        return LifecycleState::Uninitialized;
    }
    return it->second.currentState;
}

std::optional<ActorLifecycleData> LifecycleManager::GetLifecycleData(IActor* actor) const {
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool LifecycleManager::ValidateActor(IActor* actor) const {
    if (!actor) return false;
    
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) return false;
    
    return it->second.isValid();
}

bool LifecycleManager::ValidateState(IActor* actor, LifecycleState expectedState) const {
    return GetState(actor) == expectedState;
}

std::vector<std::string> LifecycleManager::ValidateActorIntegrity(IActor* actor) const {
    std::vector<std::string> errors;
    
    if (!actor) {
        errors.push_back("Actor is null");
        return errors;
    }
    
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) {
        errors.push_back("Actor not registered with lifecycle manager");
        return errors;
    }
    
    const auto& data = it->second;
    
    if (data.currentState == LifecycleState::Destroyed) {
        errors.push_back("Actor is in destroyed state");
    }
    
    // Note: We don't require updates immediately after initialization
    // An active actor with 0 updates is valid (just hasn't been updated yet)
    
    return errors;
}

LifecycleMetrics LifecycleManager::GetMetricsByType(const std::string& actorType) const {
    auto it = metricsByType_.find(actorType);
    if (it == metricsByType_.end()) {
        return LifecycleMetrics{};
    }
    return it->second;
}

void LifecycleManager::ResetMetrics() {
    globalMetrics_.Reset();
    metricsByType_.clear();
}

void LifecycleManager::DumpActorState(IActor* actor) const {
    if (!actor) {
        std::cout << "Actor is null" << std::endl;
        return;
    }
    
    auto it = actorData_.find(actor);
    if (it == actorData_.end()) {
        std::cout << "Actor not tracked" << std::endl;
        return;
    }
    
    const auto& data = it->second;
    std::cout << "Actor: " << data.actorName << std::endl;
    std::cout << "  State: " << StateToString(data.currentState) << std::endl;
    std::cout << "  Previous State: " << StateToString(data.previousState) << std::endl;
    std::cout << "  Update Count: " << data.updateCount << std::endl;
    std::cout << "  State Changes: " << data.stateChangeCount << std::endl;
    std::cout << "  Total Lifetime: " << data.totalLifetime << "s" << std::endl;
    
    if (!data.lastError.empty()) {
        std::cout << "  Last Error: " << data.lastError << std::endl;
    }
}

void LifecycleManager::DumpAllActorStates() const {
    std::cout << "=== All Actor States ===" << std::endl;
    for (const auto& [actor, data] : actorData_) {
        std::cout << data.actorName << ": " << StateToString(data.currentState) 
                  << " (updates: " << data.updateCount << ")" << std::endl;
    }
}

std::vector<IActor*> LifecycleManager::GetActorsByState(LifecycleState state) const {
    std::vector<IActor*> result;
    for (const auto& [actor, data] : actorData_) {
        if (data.currentState == state) {
            result.push_back(actor);
        }
    }
    return result;
}

void LifecycleManager::OptimizeForScenario(const std::string& scenario) {
    if (scenario == "high_frequency_updates") {
        trackPerformance_ = false;
        batchingEnabled_ = true;
    } else if (scenario == "debugging") {
        trackPerformance_ = true;
        batchingEnabled_ = false;
    } else if (scenario == "production") {
        trackPerformance_ = true;
        batchingEnabled_ = true;
    }
}

void LifecycleManager::ExecuteHooks(const std::vector<LifecycleHook>& hooks, 
                                     IActor* actor, LifecycleState state) {
    for (const auto& hook : hooks) {
        try {
            hook(actor, state);
        } catch (const std::exception& e) {
            OnActorError(actor, std::string("Hook execution failed: ") + e.what());
        }
    }
}

void LifecycleManager::UpdateMetrics(IActor* actor, double duration, 
                                      const std::string& operation) {
    if (!trackPerformance_) return;
    
    auto& globalMetrics = globalMetrics_;
    auto& typeMetrics = metricsByType_[actor->GetName()];
    
    if (operation == "initialize") {
        globalMetrics.totalInitTime += duration;
        typeMetrics.totalInitTime += duration;
        
        if (duration > globalMetrics.maxInitTime) {
            globalMetrics.maxInitTime = duration;
        }
        if (duration > typeMetrics.maxInitTime) {
            typeMetrics.maxInitTime = duration;
        }
        
        if (globalMetrics.totalCreated > 0) {
            globalMetrics.avgInitTime = globalMetrics.totalInitTime / globalMetrics.totalCreated;
        }
        if (typeMetrics.totalCreated > 0) {
            typeMetrics.avgInitTime = typeMetrics.totalInitTime / typeMetrics.totalCreated;
        }
    } else if (operation == "update") {
        globalMetrics.totalUpdateTime += duration;
        typeMetrics.totalUpdateTime += duration;
        
        if (duration > globalMetrics.maxUpdateTime) {
            globalMetrics.maxUpdateTime = duration;
        }
        if (duration > typeMetrics.maxUpdateTime) {
            typeMetrics.maxUpdateTime = duration;
        }
        
        auto it = actorData_.find(actor);
        if (it != actorData_.end() && it->second.updateCount > 0) {
            globalMetrics.avgUpdateTime = globalMetrics.totalUpdateTime / it->second.updateCount;
            typeMetrics.avgUpdateTime = typeMetrics.totalUpdateTime / it->second.updateCount;
        }
    } else if (operation == "destroy") {
        globalMetrics.totalDestroyTime += duration;
        typeMetrics.totalDestroyTime += duration;
        
        if (duration > globalMetrics.maxDestroyTime) {
            globalMetrics.maxDestroyTime = duration;
        }
        if (duration > typeMetrics.maxDestroyTime) {
            typeMetrics.maxDestroyTime = duration;
        }
        
        if (globalMetrics.totalDestroyed > 0) {
            globalMetrics.avgDestroyTime = globalMetrics.totalDestroyTime / globalMetrics.totalDestroyed;
        }
        if (typeMetrics.totalDestroyed > 0) {
            typeMetrics.avgDestroyTime = typeMetrics.totalDestroyTime / typeMetrics.totalDestroyed;
        }
    }
}

void LifecycleManager::RecordStateTransition(IActor* actor, 
                                               LifecycleState from, 
                                               LifecycleState to) {
    globalMetrics_.totalStateTransitions++;
    auto& typeMetrics = metricsByType_[actor->GetName()];
    typeMetrics.totalStateTransitions++;
}

// ============================================================================
// LIFECYCLE VALIDATOR
// ============================================================================

bool LifecycleValidator::ValidateStateTransition(LifecycleState from, 
                                                   LifecycleState to, 
                                                   std::string& error) {
    if (!IsValidTransition(from, to)) {
        error = std::string("Invalid transition from ") + StateToString(from) + 
                " to " + StateToString(to);
        return false;
    }
    return true;
}

bool LifecycleValidator::ValidateActorState(IActor* actor, std::string& error) {
    if (!actor) {
        error = "Actor is null";
        return false;
    }
    
    auto& manager = LifecycleManager::Instance();
    if (!manager.ValidateActor(actor)) {
        error = "Actor validation failed";
        return false;
    }
    
    return true;
}

bool LifecycleValidator::ValidateLifecycleIntegrity(IActor* actor, 
                                                      std::vector<std::string>& errors) {
    errors = LifecycleManager::Instance().ValidateActorIntegrity(actor);
    return errors.empty();
}

bool LifecycleValidator::ValidatePerformance(IActor* actor, 
                                               double maxInitTime, 
                                               double maxUpdateTime, 
                                               std::string& error) {
    auto data = LifecycleManager::Instance().GetLifecycleData(actor);
    if (!data) {
        error = "Actor not found in lifecycle manager";
        return false;
    }
    
    // Performance validation would require timing data per actor
    // This is a simplified version
    return true;
}

// ============================================================================
// LIFECYCLE DEBUGGER
// ============================================================================

bool LifecycleDebugger::detailedLogging_ = false;

void LifecycleDebugger::PrintActorState(IActor* actor) {
    LifecycleManager::Instance().DumpActorState(actor);
}

void LifecycleDebugger::PrintActorHistory(IActor* actor) {
    auto data = LifecycleManager::Instance().GetLifecycleData(actor);
    if (!data) {
        std::cout << "No history available for actor" << std::endl;
        return;
    }
    
    std::cout << "=== Actor History: " << data->actorName << " ===" << std::endl;
    std::cout << "State Changes: " << data->stateChangeCount << std::endl;
    std::cout << "Updates: " << data->updateCount << std::endl;
    std::cout << "Lifetime: " << data->totalLifetime << "s" << std::endl;
}

void LifecycleDebugger::PrintGlobalMetrics() {
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    
    std::cout << "=== Global Lifecycle Metrics ===" << std::endl;
    std::cout << "Total Created: " << metrics.totalCreated << std::endl;
    std::cout << "Total Destroyed: " << metrics.totalDestroyed << std::endl;
    std::cout << "Current Active: " << metrics.currentActive << std::endl;
    std::cout << "Current Paused: " << metrics.currentPaused << std::endl;
    std::cout << "Total Errors: " << metrics.totalErrors << std::endl;
    std::cout << "Total State Transitions: " << metrics.totalStateTransitions << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Avg Init Time: " << metrics.avgInitTime << "s" << std::endl;
    std::cout << "Avg Update Time: " << metrics.avgUpdateTime << "s" << std::endl;
    std::cout << "Avg Destroy Time: " << metrics.avgDestroyTime << "s" << std::endl;
    std::cout << "Max Init Time: " << metrics.maxInitTime << "s" << std::endl;
    std::cout << "Max Update Time: " << metrics.maxUpdateTime << "s" << std::endl;
    std::cout << "Max Destroy Time: " << metrics.maxDestroyTime << "s" << std::endl;
}

void LifecycleDebugger::PrintMetricsByType() {
    std::cout << "=== Metrics By Actor Type ===" << std::endl;
    // Implementation would iterate through type-specific metrics
}

void LifecycleDebugger::PrintActiveActors() {
    auto actors = LifecycleManager::Instance().GetActorsByState(LifecycleState::Active);
    std::cout << "=== Active Actors (" << actors.size() << ") ===" << std::endl;
    for (auto* actor : actors) {
        std::cout << "  - " << actor->GetName() << std::endl;
    }
}

void LifecycleDebugger::PrintStateDistribution() {
    std::cout << "=== State Distribution ===" << std::endl;
    for (int i = 0; i < 6; ++i) {
        auto state = static_cast<LifecycleState>(i);
        auto actors = LifecycleManager::Instance().GetActorsByState(state);
        std::cout << StateToString(state) << ": " << actors.size() << std::endl;
    }
}

// ============================================================================
// LIFECYCLE MONITOR
// ============================================================================

bool LifecycleMonitor::autoMonitoring_ = false;
double LifecycleMonitor::monitorInterval_ = 1.0;

LifecycleMonitor::HealthReport LifecycleMonitor::GenerateHealthReport() {
    HealthReport report;
    
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    report.totalActors = metrics.currentActive + metrics.currentPaused;
    report.avgUpdateTime = metrics.avgUpdateTime;
    report.maxUpdateTime = metrics.maxUpdateTime;
    
    // Check for performance issues
    if (metrics.avgUpdateTime > 0.016) { // 16ms frame budget
        report.warnings.push_back("Average update time exceeds 16ms frame budget");
    }
    
    if (metrics.totalErrors > 0) {
        report.errors.push_back("Errors detected: " + std::to_string(metrics.totalErrors));
    }
    
    report.healthyActors = report.totalActors - report.unhealthyActors;
    
    return report;
}

bool LifecycleMonitor::CheckActorHealth(IActor* actor, std::vector<std::string>& issues) {
    issues = LifecycleManager::Instance().ValidateActorIntegrity(actor);
    return issues.empty();
}

void LifecycleMonitor::MonitorPerformance(double threshold) {
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    
    if (metrics.maxUpdateTime > threshold) {
        std::cerr << "[LifecycleMonitor] Performance warning: Max update time " 
                  << metrics.maxUpdateTime << "s exceeds threshold " 
                  << threshold << "s" << std::endl;
    }
}

void LifecycleMonitor::DetectStalls(double timeout) {
    // Implementation would check for actors that haven't updated in timeout seconds
}

// ============================================================================
// LIFECYCLE OPTIMIZER
// ============================================================================

bool LifecycleOptimizer::poolingEnabled_ = false;
bool LifecycleOptimizer::batchingEnabled_ = false;
bool LifecycleOptimizer::cachingEnabled_ = false;

void LifecycleOptimizer::OptimizeInitialization() {
    // Enable pooling for frequently created actors
    poolingEnabled_ = true;
}

void LifecycleOptimizer::OptimizeUpdates() {
    // Enable batching for similar actors
    batchingEnabled_ = true;
}

void LifecycleOptimizer::OptimizeDestruction() {
    // Defer destruction to end of frame
}

void LifecycleOptimizer::OptimizeMemory() {
    // Enable caching for reusable data
    cachingEnabled_ = true;
}

LifecycleOptimizer::OptimizationReport LifecycleOptimizer::GenerateOptimizationReport() {
    OptimizationReport report;
    
    if (poolingEnabled_) {
        report.recommendations.push_back("Actor pooling is enabled");
    } else {
        report.recommendations.push_back("Consider enabling actor pooling");
    }
    
    if (batchingEnabled_) {
        report.recommendations.push_back("Update batching is enabled");
    } else {
        report.recommendations.push_back("Consider enabling update batching");
    }
    
    return report;
}

std::vector<std::string> LifecycleOptimizer::GetOptimizationRecommendations() {
    std::vector<std::string> recommendations;
    
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    
    if (metrics.avgInitTime > 0.001) {
        recommendations.push_back("High initialization time - consider actor pooling");
    }
    
    if (metrics.avgUpdateTime > 0.001) {
        recommendations.push_back("High update time - consider update batching");
    }
    
    if (metrics.currentActive > 1000) {
        recommendations.push_back("Large number of active actors - consider LOD system");
    }
    
    return recommendations;
}

// ============================================================================
// LIFECYCLE INTEGRATION
// ============================================================================

static std::unordered_map<std::string, std::function<void(const LifecycleMetrics&)>> externalMonitors;

void LifecycleIntegration::RegisterExternalMonitor(
    const std::string& name,
    std::function<void(const LifecycleMetrics&)> callback) {
    externalMonitors[name] = callback;
}

void LifecycleIntegration::UnregisterExternalMonitor(const std::string& name) {
    externalMonitors.erase(name);
}

std::string LifecycleIntegration::ExportMetricsJSON() {
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"totalCreated\": " << metrics.totalCreated << ",\n";
    json << "  \"totalDestroyed\": " << metrics.totalDestroyed << ",\n";
    json << "  \"currentActive\": " << metrics.currentActive << ",\n";
    json << "  \"currentPaused\": " << metrics.currentPaused << ",\n";
    json << "  \"totalErrors\": " << metrics.totalErrors << ",\n";
    json << "  \"totalStateTransitions\": " << metrics.totalStateTransitions << ",\n";
    json << "  \"avgInitTime\": " << metrics.avgInitTime << ",\n";
    json << "  \"avgUpdateTime\": " << metrics.avgUpdateTime << ",\n";
    json << "  \"avgDestroyTime\": " << metrics.avgDestroyTime << ",\n";
    json << "  \"maxInitTime\": " << metrics.maxInitTime << ",\n";
    json << "  \"maxUpdateTime\": " << metrics.maxUpdateTime << ",\n";
    json << "  \"maxDestroyTime\": " << metrics.maxDestroyTime << "\n";
    json << "}";
    
    return json.str();
}

std::string LifecycleIntegration::ExportMetricsCSV() {
    const auto& metrics = LifecycleManager::Instance().GetMetrics();
    
    std::ostringstream csv;
    csv << "Metric,Value\n";
    csv << "TotalCreated," << metrics.totalCreated << "\n";
    csv << "TotalDestroyed," << metrics.totalDestroyed << "\n";
    csv << "CurrentActive," << metrics.currentActive << "\n";
    csv << "CurrentPaused," << metrics.currentPaused << "\n";
    csv << "TotalErrors," << metrics.totalErrors << "\n";
    csv << "TotalStateTransitions," << metrics.totalStateTransitions << "\n";
    csv << "AvgInitTime," << metrics.avgInitTime << "\n";
    csv << "AvgUpdateTime," << metrics.avgUpdateTime << "\n";
    csv << "AvgDestroyTime," << metrics.avgDestroyTime << "\n";
    csv << "MaxInitTime," << metrics.maxInitTime << "\n";
    csv << "MaxUpdateTime," << metrics.maxUpdateTime << "\n";
    csv << "MaxDestroyTime," << metrics.maxDestroyTime << "\n";
    
    return csv.str();
}

void LifecycleIntegration::ExportMetricsToFile(const std::string& filename, 
                                                 const std::string& format) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    if (format == "json") {
        file << ExportMetricsJSON();
    } else if (format == "csv") {
        file << ExportMetricsCSV();
    }
    
    file.close();
}

bool LifecycleIntegration::ImportConfiguration(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Implementation would parse configuration file
    return true;
}

void LifecycleIntegration::ApplyConfiguration(const std::string& config) {
    // Implementation would parse and apply configuration string
}

} // namespace ActorLifecycle
