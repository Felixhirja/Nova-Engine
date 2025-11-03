/**
 * Example: Integrating Actor Lifecycle Management with Nova Engine
 * 
 * This example shows how to integrate the lifecycle system with existing actors
 */

#include "engine/ActorLifecycle.h"
#include <iostream>
#include <memory>
#include <vector>

// Example: Adapter for existing Nova Engine actors
// Since existing actors may not directly implement IActorBase,
// we can create an adapter

// Owning adapter - takes ownership of the actor
template<typename ActorType>
class ActorLifecycleAdapterOwning : public IActorBase {
public:
    explicit ActorLifecycleAdapterOwning(ActorType* actor) 
        : actor_(actor) {}
    
    ~ActorLifecycleAdapterOwning() {
        delete actor_;
    }
    
    void Initialize() override {
        actor_->Initialize();
    }
    
    void Update(double dt) override {
        actor_->Update(dt);
    }
    
    std::string GetName() const override {
        return actor_->GetName();
    }
    
    ActorType* GetActor() const { return actor_; }
    
private:
    ActorType* actor_;
};

// Example game class integrating lifecycle management
class GameWithLifecycle {
public:
    void Initialize() {
        using namespace ActorLifecycle;
        auto& mgr = LifecycleManager::Instance();
        
        std::cout << "=== Initializing Actor Lifecycle System ===" << std::endl;
        
        // Setup logging hooks
        mgr.RegisterPostCreateHook([](ActorLifecycle::IActor* actor, LifecycleState state) {
            std::cout << "[Lifecycle] Created: " << actor->GetName() << std::endl;
        });
        
        mgr.RegisterPostInitializeHook([](ActorLifecycle::IActor* actor, LifecycleState state) {
            std::cout << "[Lifecycle] Initialized: " << actor->GetName() << std::endl;
        });
        
        mgr.RegisterStateChangeHook([](ActorLifecycle::IActor* actor, LifecycleState state) {
            std::cout << "[Lifecycle] " << actor->GetName() 
                      << " -> " << StateToString(state) << std::endl;
        });
        
        mgr.RegisterErrorHook([](ActorLifecycle::IActor* actor, LifecycleState state) {
            std::cerr << "[Lifecycle] ERROR in " << actor->GetName() << std::endl;
        });
        
        // Enable monitoring in debug builds
        #ifdef DEBUG
        mgr.EnablePerformanceTracking(true);
        LifecycleDebugger::EnableDetailedLogging(true);
        LifecycleMonitor::EnableAutoMonitoring(true, 1.0);
        std::cout << "[Lifecycle] Debug monitoring enabled" << std::endl;
        #else
        mgr.OptimizeForScenario("production");
        std::cout << "[Lifecycle] Production optimizations enabled" << std::endl;
        #endif
        
        std::cout << std::endl;
    }
    
    // Example: Creating actors with lifecycle tracking
    template<typename ActorType>
    void CreateActor(std::unique_ptr<ActorType> actor) {
        using namespace ActorLifecycle;
        auto& mgr = LifecycleManager::Instance();
        
        // Take ownership of the actor
        ActorType* rawActor = actor.release();
        
        // Create adapter for lifecycle tracking (adapter owns the actor)
        auto adapter = std::make_unique<ActorLifecycleAdapterOwning<ActorType>>(rawActor);
        
        // Track lifecycle
        ACTOR_LIFECYCLE_BEGIN(adapter.get());
        ACTOR_LIFECYCLE_INIT(adapter.get());
        
        // Store adapter (which owns the actor)
        adapters_.push_back(std::move(adapter));
    }
    
    void Update(double deltaTime) {
        // Update all actors with lifecycle tracking
        for (auto& adapter : adapters_) {
            ACTOR_LIFECYCLE_UPDATE(adapter.get(), deltaTime);
        }
    }
    
    void PauseActor(size_t index) {
        if (index < adapters_.size()) {
            auto& mgr = ActorLifecycle::LifecycleManager::Instance();
            mgr.OnActorPause(adapters_[index].get());
        }
    }
    
    void ResumeActor(size_t index) {
        if (index < adapters_.size()) {
            auto& mgr = ActorLifecycle::LifecycleManager::Instance();
            mgr.OnActorResume(adapters_[index].get());
        }
    }
    
    void PrintMetrics() {
        using namespace ActorLifecycle;
        
        std::cout << "\n=== Lifecycle Metrics ===" << std::endl;
        LifecycleDebugger::PrintGlobalMetrics();
        
        std::cout << "\n=== Health Report ===" << std::endl;
        auto report = LifecycleMonitor::GenerateHealthReport();
        std::cout << "Total Actors: " << report.totalActors << std::endl;
        std::cout << "Healthy: " << report.healthyActors << std::endl;
        std::cout << "Warnings: " << report.warnings.size() << std::endl;
        std::cout << "Errors: " << report.errors.size() << std::endl;
        
        std::cout << "\n=== Optimization Recommendations ===" << std::endl;
        auto recs = LifecycleOptimizer::GetOptimizationRecommendations();
        if (recs.empty()) {
            std::cout << "No recommendations - system running optimally" << std::endl;
        } else {
            for (const auto& rec : recs) {
                std::cout << "  - " << rec << std::endl;
            }
        }
    }
    
    void ExportMetrics(const std::string& filename) {
        ActorLifecycle::LifecycleIntegration::ExportMetricsToFile(filename, "json");
        std::cout << "Metrics exported to: " << filename << std::endl;
    }
    
    void Shutdown() {
        using namespace ActorLifecycle;
        auto& mgr = LifecycleManager::Instance();
        
        std::cout << "\n=== Shutting Down ===" << std::endl;
        
        // Destroy all actors
        for (auto& adapter : adapters_) {
            ACTOR_LIFECYCLE_END(adapter.get());
        }
        
        // Clear (adapters will delete the actors)
        adapters_.clear();
        
        // Final metrics
        PrintMetrics();
        
        // Export for analysis
        ExportMetrics("lifecycle_final_report.json");
    }
    
private:
    std::vector<std::unique_ptr<IActorBase>> adapters_; // Adapters own the actors
};

// Example actor types (mocking Nova Engine actors)
class MockSpaceship {
public:
    void Initialize() {
        std::cout << "  [Spaceship] Initializing systems..." << std::endl;
    }
    
    void Update(double dt) {
        (void)dt;
        // Update logic
    }
    
    std::string GetName() const { return "Spaceship"; }
};

class MockStation {
public:
    void Initialize() {
        std::cout << "  [Station] Initializing docking bay..." << std::endl;
    }
    
    void Update(double dt) {
        (void)dt;
        // Update logic
    }
    
    std::string GetName() const { return "Station"; }
};

class MockProjectile {
public:
    void Initialize() {
        std::cout << "  [Projectile] Armed and ready..." << std::endl;
    }
    
    void Update(double dt) {
        (void)dt;
        // Update logic
    }
    
    std::string GetName() const { return "Projectile"; }
};

// Main example
int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "  Actor Lifecycle Integration Example      " << std::endl;
    std::cout << "============================================\n" << std::endl;
    
    GameWithLifecycle game;
    
    // Initialize lifecycle system
    game.Initialize();
    
    // Create various actors
    std::cout << "=== Creating Actors ===" << std::endl;
    game.CreateActor(std::make_unique<MockSpaceship>());
    game.CreateActor(std::make_unique<MockStation>());
    game.CreateActor(std::make_unique<MockProjectile>());
    
    // Simulate game loop
    std::cout << "\n=== Running Game Loop ===" << std::endl;
    for (int frame = 0; frame < 10; ++frame) {
        game.Update(0.016); // 60 FPS
        
        // Pause actor 1 on frame 5
        if (frame == 5) {
            std::cout << "\n[Game] Pausing actor 1" << std::endl;
            game.PauseActor(1);
        }
        
        // Resume actor 1 on frame 7
        if (frame == 7) {
            std::cout << "[Game] Resuming actor 1" << std::endl;
            game.ResumeActor(1);
        }
    }
    
    // Print metrics during gameplay
    game.PrintMetrics();
    
    // Shutdown
    game.Shutdown();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "  Example Complete                          " << std::endl;
    std::cout << "============================================" << std::endl;
    
    return 0;
}
