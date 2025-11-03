#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <memory>
#include <optional>

// Actor interface - must provide these functions
struct IActorBase {
    virtual ~IActorBase() = default;
    virtual void Initialize() = 0;
    virtual void Update(double dt) = 0;
    virtual std::string GetName() const = 0;
};

/**
 * Actor Lifecycle Management System
 * Provides comprehensive lifecycle tracking, hooks, validation, and monitoring
 * 
 * Actors must provide:
 * - void Initialize()
 * - void Update(double dt)
 * - std::string GetName() const
 */

namespace ActorLifecycle {

using IActor = IActorBase;

// ============================================================================
// LIFECYCLE STATE MANAGEMENT
// ============================================================================

enum class LifecycleState {
    Uninitialized,  // Actor created but not initialized
    Initializing,   // Currently running Initialize()
    Active,         // Normal operation
    Paused,         // Update paused but actor still valid
    Destroying,     // Currently being destroyed
    Destroyed       // Fully destroyed
};

const char* StateToString(LifecycleState state);

// State transition validation
bool IsValidTransition(LifecycleState from, LifecycleState to);

// ============================================================================
// LIFECYCLE HOOKS
// ============================================================================

using LifecycleHook = std::function<void(IActor*, LifecycleState)>;

struct LifecycleHooks {
    std::vector<LifecycleHook> onPreCreate;
    std::vector<LifecycleHook> onPostCreate;
    std::vector<LifecycleHook> onPreInitialize;
    std::vector<LifecycleHook> onPostInitialize;
    std::vector<LifecycleHook> onPreDestroy;
    std::vector<LifecycleHook> onPostDestroy;
    std::vector<LifecycleHook> onStateChange;
    std::vector<LifecycleHook> onPause;
    std::vector<LifecycleHook> onResume;
    std::vector<LifecycleHook> onError;
    
    void Clear() {
        onPreCreate.clear();
        onPostCreate.clear();
        onPreInitialize.clear();
        onPostInitialize.clear();
        onPreDestroy.clear();
        onPostDestroy.clear();
        onStateChange.clear();
        onPause.clear();
        onResume.clear();
        onError.clear();
    }
};

// ============================================================================
// LIFECYCLE ANALYTICS
// ============================================================================

struct LifecycleMetrics {
    size_t totalCreated = 0;
    size_t totalDestroyed = 0;
    size_t currentActive = 0;
    size_t currentPaused = 0;
    size_t totalErrors = 0;
    size_t totalStateTransitions = 0;
    
    double totalInitTime = 0.0;
    double totalUpdateTime = 0.0;
    double totalDestroyTime = 0.0;
    
    double avgInitTime = 0.0;
    double avgUpdateTime = 0.0;
    double avgDestroyTime = 0.0;
    
    double maxInitTime = 0.0;
    double maxUpdateTime = 0.0;
    double maxDestroyTime = 0.0;
    
    void Reset() {
        *this = LifecycleMetrics{};
    }
};

struct ActorLifecycleData {
    LifecycleState currentState = LifecycleState::Uninitialized;
    LifecycleState previousState = LifecycleState::Uninitialized;
    
    std::chrono::steady_clock::time_point creationTime;
    std::chrono::steady_clock::time_point initializationTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    std::chrono::steady_clock::time_point destructionTime;
    
    double totalLifetime = 0.0;
    double totalActiveTime = 0.0;
    size_t updateCount = 0;
    size_t stateChangeCount = 0;
    
    std::string actorName;
    std::string lastError;
    
    bool isValid() const {
        return currentState != LifecycleState::Destroyed;
    }
};

// ============================================================================
// LIFECYCLE MANAGER
// ============================================================================

class LifecycleManager {
public:
    static LifecycleManager& Instance();
    
    // Hook registration
    void RegisterPreCreateHook(LifecycleHook hook);
    void RegisterPostCreateHook(LifecycleHook hook);
    void RegisterPreInitializeHook(LifecycleHook hook);
    void RegisterPostInitializeHook(LifecycleHook hook);
    void RegisterPreDestroyHook(LifecycleHook hook);
    void RegisterPostDestroyHook(LifecycleHook hook);
    void RegisterStateChangeHook(LifecycleHook hook);
    void RegisterPauseHook(LifecycleHook hook);
    void RegisterResumeHook(LifecycleHook hook);
    void RegisterErrorHook(LifecycleHook hook);
    void ClearAllHooks();
    
    // Lifecycle operations
    void OnActorCreate(IActor* actor);
    void OnActorInitialize(IActor* actor);
    void OnActorDestroy(IActor* actor);
    void OnActorUpdate(IActor* actor, double deltaTime);
    void OnActorPause(IActor* actor);
    void OnActorResume(IActor* actor);
    void OnActorError(IActor* actor, const std::string& error);
    
    // State management
    bool TransitionState(IActor* actor, LifecycleState newState);
    LifecycleState GetState(IActor* actor) const;
    std::optional<ActorLifecycleData> GetLifecycleData(IActor* actor) const;
    
    // Validation
    bool ValidateActor(IActor* actor) const;
    bool ValidateState(IActor* actor, LifecycleState expectedState) const;
    std::vector<std::string> ValidateActorIntegrity(IActor* actor) const;
    
    // Monitoring & Analytics
    const LifecycleMetrics& GetMetrics() const { return globalMetrics_; }
    LifecycleMetrics GetMetricsByType(const std::string& actorType) const;
    void ResetMetrics();
    
    // Debugging
    void DumpActorState(IActor* actor) const;
    void DumpAllActorStates() const;
    std::vector<IActor*> GetActorsByState(LifecycleState state) const;
    
    // Performance monitoring
    void EnablePerformanceTracking(bool enable) { trackPerformance_ = enable; }
    bool IsPerformanceTrackingEnabled() const { return trackPerformance_; }
    
    // Optimization
    void OptimizeForScenario(const std::string& scenario);
    void EnableBatching(bool enable) { batchingEnabled_ = enable; }
    
private:
    LifecycleManager() = default;
    ~LifecycleManager() = default;
    LifecycleManager(const LifecycleManager&) = delete;
    LifecycleManager& operator=(const LifecycleManager&) = delete;
    
    void ExecuteHooks(const std::vector<LifecycleHook>& hooks, IActor* actor, LifecycleState state);
    void UpdateMetrics(IActor* actor, double duration, const std::string& operation);
    void RecordStateTransition(IActor* actor, LifecycleState from, LifecycleState to);
    
    LifecycleHooks hooks_;
    std::unordered_map<IActor*, ActorLifecycleData> actorData_;
    LifecycleMetrics globalMetrics_;
    std::unordered_map<std::string, LifecycleMetrics> metricsByType_;
    
    bool trackPerformance_ = true;
    bool batchingEnabled_ = false;
};

// ============================================================================
// RAII LIFECYCLE GUARD
// ============================================================================

class LifecycleGuard {
public:
    LifecycleGuard(IActor* actor, LifecycleState targetState)
        : actor_(actor), initialState_(LifecycleManager::Instance().GetState(actor)) {
        LifecycleManager::Instance().TransitionState(actor_, targetState);
    }
    
    ~LifecycleGuard() {
        if (restoreOnDestroy_) {
            LifecycleManager::Instance().TransitionState(actor_, initialState_);
        }
    }
    
    void SetRestoreOnDestroy(bool restore) { restoreOnDestroy_ = restore; }
    
private:
    IActor* actor_;
    LifecycleState initialState_;
    bool restoreOnDestroy_ = false;
};

// ============================================================================
// LIFECYCLE VALIDATOR
// ============================================================================

class LifecycleValidator {
public:
    static bool ValidateStateTransition(LifecycleState from, LifecycleState to, std::string& error);
    static bool ValidateActorState(IActor* actor, std::string& error);
    static bool ValidateLifecycleIntegrity(IActor* actor, std::vector<std::string>& errors);
    static bool ValidatePerformance(IActor* actor, double maxInitTime, double maxUpdateTime, std::string& error);
};

// ============================================================================
// LIFECYCLE DEBUGGER
// ============================================================================

class LifecycleDebugger {
public:
    static void PrintActorState(IActor* actor);
    static void PrintActorHistory(IActor* actor);
    static void PrintGlobalMetrics();
    static void PrintMetricsByType();
    static void PrintActiveActors();
    static void PrintStateDistribution();
    
    static void EnableDetailedLogging(bool enable) { detailedLogging_ = enable; }
    static bool IsDetailedLoggingEnabled() { return detailedLogging_; }
    
private:
    static bool detailedLogging_;
};

// ============================================================================
// LIFECYCLE MONITOR
// ============================================================================

class LifecycleMonitor {
public:
    struct HealthReport {
        size_t totalActors = 0;
        size_t healthyActors = 0;
        size_t unhealthyActors = 0;
        size_t stalledActors = 0;
        double avgUpdateTime = 0.0;
        double maxUpdateTime = 0.0;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    
    static HealthReport GenerateHealthReport();
    static bool CheckActorHealth(IActor* actor, std::vector<std::string>& issues);
    static void MonitorPerformance(double threshold);
    static void DetectStalls(double timeout);
    
    static void EnableAutoMonitoring(bool enable, double interval = 1.0) {
        autoMonitoring_ = enable;
        monitorInterval_ = interval;
    }
    
private:
    static bool autoMonitoring_;
    static double monitorInterval_;
};

// ============================================================================
// LIFECYCLE OPTIMIZER
// ============================================================================

class LifecycleOptimizer {
public:
    struct OptimizationReport {
        size_t pooledActors = 0;
        size_t batchedOperations = 0;
        double timeSaved = 0.0;
        double memorySaved = 0.0;
        std::vector<std::string> recommendations;
    };
    
    static void OptimizeInitialization();
    static void OptimizeUpdates();
    static void OptimizeDestruction();
    static void OptimizeMemory();
    
    static OptimizationReport GenerateOptimizationReport();
    static std::vector<std::string> GetOptimizationRecommendations();
    
    static void EnablePooling(bool enable) { poolingEnabled_ = enable; }
    static void EnableBatching(bool enable) { batchingEnabled_ = enable; }
    static void EnableCaching(bool enable) { cachingEnabled_ = enable; }
    
private:
    static bool poolingEnabled_;
    static bool batchingEnabled_;
    static bool cachingEnabled_;
};

// ============================================================================
// LIFECYCLE INTEGRATION
// ============================================================================

class LifecycleIntegration {
public:
    // External tool integration
    static void RegisterExternalMonitor(const std::string& name, 
        std::function<void(const LifecycleMetrics&)> callback);
    static void UnregisterExternalMonitor(const std::string& name);
    
    // Export data for external tools
    static std::string ExportMetricsJSON();
    static std::string ExportMetricsCSV();
    static void ExportMetricsToFile(const std::string& filename, const std::string& format = "json");
    
    // Import configuration
    static bool ImportConfiguration(const std::string& filename);
    static void ApplyConfiguration(const std::string& config);
};

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define ACTOR_LIFECYCLE_BEGIN(actor) \
    ActorLifecycle::LifecycleManager::Instance().OnActorCreate(actor)

#define ACTOR_LIFECYCLE_INIT(actor) \
    ActorLifecycle::LifecycleManager::Instance().OnActorInitialize(actor)

#define ACTOR_LIFECYCLE_UPDATE(actor, dt) \
    ActorLifecycle::LifecycleManager::Instance().OnActorUpdate(actor, dt)

#define ACTOR_LIFECYCLE_END(actor) \
    ActorLifecycle::LifecycleManager::Instance().OnActorDestroy(actor)

#define ACTOR_LIFECYCLE_VALIDATE(actor) \
    ActorLifecycle::LifecycleManager::Instance().ValidateActor(actor)

} // namespace ActorLifecycle
