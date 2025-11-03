#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>
#include <mutex>
#include <iostream>

// Forward declarations
class IActor;

// Include headers needed for the full definitions
#include "ecs/EntityManager.h"
#include "ActorContext.h"

namespace lifecycle {

/**
 * Actor lifecycle state enumeration
 * Represents the current state of an actor in its lifecycle
 */
enum class ActorState {
    Created,      // Actor instance created but not initialized
    Initializing, // Actor is currently being initialized
    Initialized,  // Actor initialization complete
    Active,       // Actor is active and running
    Pausing,      // Actor is being paused
    Paused,       // Actor is paused
    Resuming,     // Actor is being resumed from pause
    Destroying,   // Actor is being destroyed
    Destroyed     // Actor has been destroyed
};

/**
 * Actor lifecycle event enumeration
 * Represents different lifecycle events that can occur
 */
enum class LifecycleEvent {
    PreCreate,      // Before actor creation
    PostCreate,     // After actor creation
    PreInitialize,  // Before actor initialization
    PostInitialize, // After actor initialization
    PreActivate,    // Before actor activation
    PostActivate,   // After actor activation
    PrePause,       // Before actor pause
    PostPause,      // After actor pause
    PreResume,      // Before actor resume
    PostResume,     // After actor resume
    PreDestroy,     // Before actor destruction
    PostDestroy     // After actor destruction
};

/**
 * Actor lifecycle statistics
 * Tracks performance and health metrics for actors
 */
struct LifecycleStats {
    std::chrono::high_resolution_clock::time_point creationTime;
    std::chrono::high_resolution_clock::time_point initializationTime;
    std::chrono::high_resolution_clock::time_point activationTime;
    std::chrono::high_resolution_clock::time_point destructionTime;
    
    size_t updateCallCount = 0;
    double totalUpdateTime = 0.0;
    double averageUpdateTime = 0.0;
    
    size_t pauseCount = 0;
    double totalPausedTime = 0.0;
    
    bool isAlive() const { return destructionTime == std::chrono::high_resolution_clock::time_point{}; }
    double getLifetime() const;
    double getInitializationDuration() const;
    double getActivationDuration() const;
};

/**
 * Actor lifecycle context
 * Contains information about the actor and its current state
 */
struct LifecycleContext {
    IActor* actor = nullptr;
    ActorContext* actorContext = nullptr;
    std::string actorName;
    std::string actorType;
    ActorState state = ActorState::Created;
    LifecycleStats stats;
    std::unordered_map<std::string, std::string> metadata;
    
    // Convenience methods
    bool IsInState(ActorState state) const { return this->state == state; }
    bool CanTransitionTo(ActorState newState) const;
    void SetMetadata(const std::string& key, const std::string& value) { metadata[key] = value; }
    std::string GetMetadata(const std::string& key, const std::string& defaultValue = "") const;
};

/**
 * Lifecycle hook function signature
 * Hooks receive the lifecycle context and can modify it
 */
using LifecycleHook = std::function<void(LifecycleContext&)>;

/**
 * Validation function signature
 * Returns true if the transition is valid, false otherwise
 */
using StateValidator = std::function<bool(const LifecycleContext&, ActorState newState)>;

/**
 * Performance optimizer function signature
 * Can batch or optimize lifecycle operations
 */
using PerformanceOptimizer = std::function<void(std::vector<LifecycleContext*>&)>;

/**
 * Actor Lifecycle Manager
 * Manages the complete lifecycle of actors with hooks, validation, and optimization
 */
class ActorLifecycleManager {
public:
    /**
     * Configuration for the lifecycle manager
     */
    struct Config {
        bool enableValidation = true;
        bool enableHooks = true;
        bool enablePerformanceOptimization = true;
        bool enableAnalytics = true;
        bool enableDebugLogging = false;
        size_t batchSize = 32;
        double validationTimeout = 5.0; // seconds
    };

    static ActorLifecycleManager& Instance();
    
    // Configuration
    void SetConfig(const Config& config) { config_ = config; }
    const Config& GetConfig() const { return config_; }

    // Lifecycle hooks registration
    void RegisterHook(LifecycleEvent event, const std::string& name, LifecycleHook hook);
    void UnregisterHook(LifecycleEvent event, const std::string& name);
    void ClearHooks(LifecycleEvent event);
    void ClearAllHooks();

    // State validation
    void RegisterValidator(const std::string& name, StateValidator validator);
    void UnregisterValidator(const std::string& name);
    bool ValidateTransition(const LifecycleContext& context, ActorState newState);

    // Performance optimization
    void RegisterOptimizer(const std::string& name, PerformanceOptimizer optimizer);
    void UnregisterOptimizer(const std::string& name);
    void OptimizeBatch(std::vector<LifecycleContext*>& contexts);

    // Actor lifecycle management
    void RegisterActor(IActor* actor, ActorContext* context);
    void UnregisterActor(IActor* actor);
    LifecycleContext* GetContext(IActor* actor);
    
    // State transitions
    bool TransitionTo(IActor* actor, ActorState newState);
    ActorState GetState(const IActor* actor) const;
    
    // Batch operations
    void BatchTransition(const std::vector<IActor*>& actors, ActorState newState);
    void BatchUpdate(double deltaTime);
    
    // Analytics and monitoring
    std::vector<LifecycleStats> GetAllStats() const;
    LifecycleStats GetStats(const IActor* actor) const;
    size_t GetActorCount() const { return actorContexts_.size(); }
    size_t GetActorCountInState(ActorState state) const;
    
    // Debug and diagnostics
    void PrintDebugInfo() const;
    void PrintActorState(IActor* actor) const;
    std::string GetStateReport() const;
    
    // Cleanup
    void DestroyAllActors();
    void GarbageCollect();

private:
    ActorLifecycleManager() = default;
    ~ActorLifecycleManager() = default;
    
    // Hook execution
    void ExecuteHooks(LifecycleEvent event, LifecycleContext& context);
    
    // State management
    bool IsValidTransition(ActorState current, ActorState target) const;
    void UpdateStats(LifecycleContext& context, ActorState newState);
    
    // Helper methods for lifecycle events
    LifecycleEvent GetPreEvent(ActorState state) const;
    LifecycleEvent GetPostEvent(ActorState state) const;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Configuration
    Config config_;
    
    // Actor management
    std::unordered_map<IActor*, std::unique_ptr<LifecycleContext>> actorContexts_;
    
    // Hooks
    std::unordered_map<LifecycleEvent, std::unordered_map<std::string, LifecycleHook>> hooks_;
    
    // Validators
    std::unordered_map<std::string, StateValidator> validators_;
    
    // Optimizers
    std::unordered_map<std::string, PerformanceOptimizer> optimizers_;
    
    // Batch processing
    std::vector<LifecycleContext*> batchQueue_;
};

/**
 * RAII helper for automatic actor lifecycle management
 * Ensures proper cleanup even if exceptions occur
 */
class ScopedActorLifecycle {
public:
    ScopedActorLifecycle(IActor* actor, ActorContext* context)
        : actor_(actor) {
        ActorLifecycleManager::Instance().RegisterActor(actor, context);
    }
    
    ~ScopedActorLifecycle() {
        if (actor_) {
            ActorLifecycleManager::Instance().UnregisterActor(actor_);
        }
    }
    
    // Non-copyable, movable
    ScopedActorLifecycle(const ScopedActorLifecycle&) = delete;
    ScopedActorLifecycle& operator=(const ScopedActorLifecycle&) = delete;
    
    ScopedActorLifecycle(ScopedActorLifecycle&& other) noexcept
        : actor_(other.actor_) {
        other.actor_ = nullptr;
    }
    
    ScopedActorLifecycle& operator=(ScopedActorLifecycle&& other) noexcept {
        if (this != &other) {
            actor_ = other.actor_;
            other.actor_ = nullptr;
        }
        return *this;
    }

private:
    IActor* actor_;
};

/**
 * Utility functions for lifecycle management
 */
namespace utils {
    std::string StateToString(ActorState state);
    std::string EventToString(LifecycleEvent event);
    ActorState StringToState(const std::string& str);
    LifecycleEvent StringToEvent(const std::string& str);
    
    // Performance helpers
    double GetAverageLifetime(const std::vector<LifecycleStats>& stats);
    double GetAverageInitTime(const std::vector<LifecycleStats>& stats);
    size_t GetActiveActorCount(const std::vector<LifecycleStats>& stats);
    
    // Validation helpers
    bool IsCreationState(ActorState state);
    bool IsActiveState(ActorState state);
    bool IsDestructionState(ActorState state);
    bool IsTransientState(ActorState state);
}

} // namespace lifecycle