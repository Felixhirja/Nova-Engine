#pragma once

#include "ecs/EntityManager.h"  // Include EntityManager first
#include "IActor.h"
#include "ActorLifecycleManager.h"
#include <memory>

/**
 * Enhanced Actor base class with automatic lifecycle management
 * Inherits from IActor and provides automatic integration with ActorLifecycleManager
 */
class LifecycleActor : public IActor {
public:
    LifecycleActor() = default;
    virtual ~LifecycleActor() = default;
    
    // Enhanced IActor interface with lifecycle management
    void AttachContext(ActorContext context) override {
        IActor::AttachContext(context);
        
        // Register with lifecycle manager
        lifecycle::ActorLifecycleManager::Instance().RegisterActor(this, &context_);
        
        // Initialize scoped lifecycle management
        scopedLifecycle_ = std::make_unique<lifecycle::ScopedActorLifecycle>(this, &context_);
    }
    
    void Initialize() override {
        // Transition to initializing state
        lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Initializing);
        
        // Call derived class initialization
        OnInitialize();
        
        // Transition to initialized state
        lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Initialized);
        
        // Transition to active state
        lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Active);
    }
    
    void Update(double dt) override {
        auto* context = lifecycle::ActorLifecycleManager::Instance().GetContext(this);
        if (context && context->IsInState(lifecycle::ActorState::Active)) {
            OnUpdate(dt);
        }
    }
    
    // Lifecycle control methods
    bool Pause() {
        return lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Pausing) &&
               lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Paused);
    }
    
    bool Resume() {
        return lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Resuming) &&
               lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Active);
    }
    
    bool Destroy() {
        return lifecycle::ActorLifecycleManager::Instance().TransitionTo(this, lifecycle::ActorState::Destroying);
    }
    
    // State queries
    lifecycle::ActorState GetLifecycleState() const {
        return lifecycle::ActorLifecycleManager::Instance().GetState(this);
    }
    
    bool IsActive() const {
        return GetLifecycleState() == lifecycle::ActorState::Active;
    }
    
    bool IsPaused() const {
        return GetLifecycleState() == lifecycle::ActorState::Paused;
    }
    
    bool IsDestroyed() const {
        return GetLifecycleState() == lifecycle::ActorState::Destroyed;
    }
    
    // Lifecycle statistics
    lifecycle::LifecycleStats GetLifecycleStats() const {
        return lifecycle::ActorLifecycleManager::Instance().GetStats(this);
    }
    
    // Lifecycle hooks (optional overrides for derived classes)
    virtual void OnLifecycleEvent(lifecycle::LifecycleEvent event, lifecycle::LifecycleContext& context) {
        (void)event;
        (void)context;
        // Default: no action
    }

protected:
    // Virtual methods for derived classes to implement
    virtual void OnInitialize() = 0;
    virtual void OnUpdate(double dt) = 0;
    
    // Optional lifecycle hooks for derived classes
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnDestroy() {}

private:
    std::unique_ptr<lifecycle::ScopedActorLifecycle> scopedLifecycle_;
};

/**
 * Utility class for registering lifecycle hooks
 */
class LifecycleHookRegistry {
public:
    static LifecycleHookRegistry& Instance() {
        static LifecycleHookRegistry instance;
        return instance;
    }
    
    // Register common lifecycle hooks
    void RegisterDefaultHooks() {
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Logging hooks
        manager.RegisterHook(lifecycle::LifecycleEvent::PostCreate, "logging", 
            [](lifecycle::LifecycleContext& context) {
                std::cout << "[Lifecycle] Actor '" << context.actorName << "' created" << std::endl;
            });
            
        manager.RegisterHook(lifecycle::LifecycleEvent::PostInitialize, "logging",
            [](lifecycle::LifecycleContext& context) {
                std::cout << "[Lifecycle] Actor '" << context.actorName << "' initialized" << std::endl;
            });
            
        manager.RegisterHook(lifecycle::LifecycleEvent::PostDestroy, "logging",
            [](lifecycle::LifecycleContext& context) {
                std::cout << "[Lifecycle] Actor '" << context.actorName << "' destroyed" << std::endl;
            });
        
        // Performance monitoring hook
        manager.RegisterHook(lifecycle::LifecycleEvent::PostInitialize, "performance",
            [](lifecycle::LifecycleContext& context) {
                double initTime = context.stats.getInitializationDuration();
                if (initTime > 0.1) { // Log slow initializations
                    std::cout << "[Lifecycle] WARNING: Slow initialization for '" 
                              << context.actorName << "': " << initTime << "s" << std::endl;
                }
            });
        
        // Validation hook
        manager.RegisterValidator("basic_validation", 
            [](const lifecycle::LifecycleContext& context, lifecycle::ActorState newState) {
                // Prevent transitions if actor is null
                return context.actor != nullptr;
            });
    }
    
    // Register performance optimization hooks
    void RegisterPerformanceHooks() {
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Batch creation optimizer
        manager.RegisterOptimizer("batch_creation",
            [](std::vector<lifecycle::LifecycleContext*>& contexts) {
                if (contexts.size() > 10) {
                    std::cout << "[Lifecycle] Optimizing batch of " << contexts.size() 
                              << " actors" << std::endl;
                    // Could implement actual optimizations here
                }
            });
    }
    
    // Register analytics hooks
    void RegisterAnalyticsHooks() {
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Track actor type creation patterns
        manager.RegisterHook(lifecycle::LifecycleEvent::PostCreate, "analytics",
            [this](lifecycle::LifecycleContext& context) {
                creationCounts_[context.actorType]++;
                totalCreations_++;
                
                // Log milestone
                if (totalCreations_ % 100 == 0) {
                    std::cout << "[Analytics] Created " << totalCreations_ << " actors total" << std::endl;
                }
            });
        
        // Track long-lived actors
        manager.RegisterHook(lifecycle::LifecycleEvent::PostDestroy, "analytics",
            [this](lifecycle::LifecycleContext& context) {
                double lifetime = context.stats.getLifetime();
                if (lifetime > 60.0) { // Actors alive for more than a minute
                    longLivedActors_++;
                    std::cout << "[Analytics] Long-lived actor '" << context.actorName 
                              << "' destroyed after " << lifetime << "s" << std::endl;
                }
            });
    }
    
    // Analytics reporting
    void PrintAnalytics() const {
        std::cout << "\n=== Lifecycle Analytics Report ===" << std::endl;
        std::cout << "Total actors created: " << totalCreations_ << std::endl;
        std::cout << "Long-lived actors: " << longLivedActors_ << std::endl;
        std::cout << "Creation count by type:" << std::endl;
        for (const auto& [type, count] : creationCounts_) {
            std::cout << "  " << type << ": " << count << std::endl;
        }
        std::cout << "==================================\n" << std::endl;
    }

private:
    LifecycleHookRegistry() = default;
    
    // Analytics data
    std::unordered_map<std::string, size_t> creationCounts_;
    size_t totalCreations_ = 0;
    size_t longLivedActors_ = 0;
};

/**
 * Example enhanced actor using lifecycle management
 */
class ExampleLifecycleActor : public LifecycleActor {
public:
    std::string GetName() const override { return "ExampleLifecycleActor"; }

protected:
    void OnInitialize() override {
        std::cout << "[ExampleActor] Custom initialization logic" << std::endl;
        // Custom initialization here
    }
    
    void OnUpdate(double dt) override {
        // Custom update logic here
        (void)dt;
    }
    
    void OnPause() override {
        std::cout << "[ExampleActor] Paused" << std::endl;
    }
    
    void OnResume() override {
        std::cout << "[ExampleActor] Resumed" << std::endl;
    }
    
    void OnDestroy() override {
        std::cout << "[ExampleActor] Custom cleanup logic" << std::endl;
    }
};

/**
 * Lifecycle management utility functions
 */
namespace lifecycle_utils {
    /**
     * Initialize the complete lifecycle system
     */
    inline void InitializeLifecycleSystem() {
        auto& hookRegistry = LifecycleHookRegistry::Instance();
        hookRegistry.RegisterDefaultHooks();
        hookRegistry.RegisterPerformanceHooks();
        hookRegistry.RegisterAnalyticsHooks();
        
        // Configure lifecycle manager
        lifecycle::ActorLifecycleManager::Config config;
        config.enableValidation = true;
        config.enableHooks = true;
        config.enablePerformanceOptimization = true;
        config.enableAnalytics = true;
        config.enableDebugLogging = false; // Can be enabled for debugging
        
        lifecycle::ActorLifecycleManager::Instance().SetConfig(config);
        
        std::cout << "[Lifecycle] Lifecycle system initialized" << std::endl;
    }
    
    /**
     * Shutdown and report lifecycle analytics
     */
    inline void ShutdownLifecycleSystem() {
        LifecycleHookRegistry::Instance().PrintAnalytics();
        lifecycle::ActorLifecycleManager::Instance().PrintDebugInfo();
        lifecycle::ActorLifecycleManager::Instance().DestroyAllActors();
        std::cout << "[Lifecycle] Lifecycle system shutdown complete" << std::endl;
    }
    
    /**
     * Print current lifecycle state for all actors
     */
    inline void PrintLifecycleReport() {
        std::cout << lifecycle::ActorLifecycleManager::Instance().GetStateReport() << std::endl;
    }
    
    /**
     * Create performance report
     */
    inline void PrintPerformanceReport() {
        auto stats = lifecycle::ActorLifecycleManager::Instance().GetAllStats();
        std::cout << "\n=== Lifecycle Performance Report ===" << std::endl;
        std::cout << "Total actors tracked: " << stats.size() << std::endl;
        std::cout << "Active actors: " << lifecycle::utils::GetActiveActorCount(stats) << std::endl;
        std::cout << "Average lifetime: " << lifecycle::utils::GetAverageLifetime(stats) << "s" << std::endl;
        std::cout << "Average init time: " << lifecycle::utils::GetAverageInitTime(stats) << "s" << std::endl;
        std::cout << "====================================\n" << std::endl;
    }
}

// Convenience macros for lifecycle management
#define LIFECYCLE_ACTOR_IMPL(ClassName, ActorName) \
    public: \
        std::string GetName() const override { return ActorName; } \
    protected: \
        void OnInitialize() override; \
        void OnUpdate(double dt) override;

#define LIFECYCLE_ACTOR_FULL_IMPL(ClassName, ActorName) \
    LIFECYCLE_ACTOR_IMPL(ClassName, ActorName) \
    public: \
        void OnPause() override; \
        void OnResume() override; \
        void OnDestroy() override;
