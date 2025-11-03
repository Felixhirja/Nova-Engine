/**
 * Comprehensive Actor Lifecycle Testing
 * Tests all aspects of the lifecycle management system
 */

#include "engine/ActorLifecycle.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <string>
#include <memory>

using namespace ActorLifecycle;

// Mock actor for testing
class TestActor : public IActorBase {
public:
    TestActor(const std::string& name = "TestActor") : name_(name) {}
    
    void Initialize() override {
        initialized_ = true;
    }
    
    void Update(double dt) override {
        updateCount_++;
        lastDeltaTime_ = dt;
    }
    
    std::string GetName() const override {
        return name_;
    }
    
    bool IsInitialized() const { return initialized_; }
    size_t GetUpdateCount() const { return updateCount_; }
    double GetLastDeltaTime() const { return lastDeltaTime_; }
    
private:
    std::string name_;
    bool initialized_ = false;
    size_t updateCount_ = 0;
    double lastDeltaTime_ = 0.0;
};

// Test lifecycle hooks
void TestLifecycleHooks() {
    std::cout << "\n=== Testing Lifecycle Hooks ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    
    bool preCreateCalled = false;
    bool postCreateCalled = false;
    bool preInitCalled = false;
    bool postInitCalled = false;
    bool stateChangeCalled = false;
    
    manager.RegisterPreCreateHook([&](IActor* actor, LifecycleState state) {
        preCreateCalled = true;
        std::cout << "Pre-create hook called for " << actor->GetName() << std::endl;
    });
    
    manager.RegisterPostCreateHook([&](IActor* actor, LifecycleState state) {
        postCreateCalled = true;
        std::cout << "Post-create hook called for " << actor->GetName() << std::endl;
    });
    
    manager.RegisterPreInitializeHook([&](IActor* actor, LifecycleState state) {
        preInitCalled = true;
        std::cout << "Pre-initialize hook called" << std::endl;
    });
    
    manager.RegisterPostInitializeHook([&](IActor* actor, LifecycleState state) {
        postInitCalled = true;
        std::cout << "Post-initialize hook called" << std::endl;
    });
    
    manager.RegisterStateChangeHook([&](IActor* actor, LifecycleState state) {
        stateChangeCalled = true;
        std::cout << "State changed to: " << StateToString(state) << std::endl;
    });
    
    auto actor = std::make_unique<TestActor>();
    
    manager.OnActorCreate(actor.get());
    assert(preCreateCalled && "Pre-create hook not called");
    assert(postCreateCalled && "Post-create hook not called");
    
    manager.OnActorInitialize(actor.get());
    assert(preInitCalled && "Pre-initialize hook not called");
    assert(postInitCalled && "Post-initialize hook not called");
    assert(stateChangeCalled && "State change hook not called");
    assert(actor->IsInitialized() && "Actor not initialized");
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Lifecycle hooks test passed" << std::endl;
}

// Test state transitions
void TestStateTransitions() {
    std::cout << "\n=== Testing State Transitions ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    
    auto actor = std::make_unique<TestActor>();
    
    manager.OnActorCreate(actor.get());
    assert(manager.GetState(actor.get()) == LifecycleState::Uninitialized);
    std::cout << "✓ Initial state: Uninitialized" << std::endl;
    
    manager.OnActorInitialize(actor.get());
    assert(manager.GetState(actor.get()) == LifecycleState::Active);
    std::cout << "✓ After initialization: Active" << std::endl;
    
    manager.OnActorPause(actor.get());
    assert(manager.GetState(actor.get()) == LifecycleState::Paused);
    std::cout << "✓ After pause: Paused" << std::endl;
    
    manager.OnActorResume(actor.get());
    assert(manager.GetState(actor.get()) == LifecycleState::Active);
    std::cout << "✓ After resume: Active" << std::endl;
    
    manager.OnActorDestroy(actor.get());
    assert(manager.GetState(actor.get()) == LifecycleState::Destroyed);
    std::cout << "✓ After destroy: Destroyed" << std::endl;
    
    std::cout << "✓ State transitions test passed" << std::endl;
}

// Test validation
void TestValidation() {
    std::cout << "\n=== Testing Validation ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    
    auto actor = std::make_unique<TestActor>();
    
    // Test invalid state transition
    assert(!IsValidTransition(LifecycleState::Uninitialized, LifecycleState::Active));
    std::cout << "✓ Invalid transition rejected" << std::endl;
    
    // Test valid state transition
    assert(IsValidTransition(LifecycleState::Uninitialized, LifecycleState::Initializing));
    std::cout << "✓ Valid transition accepted" << std::endl;
    
    manager.OnActorCreate(actor.get());
    assert(manager.ValidateActor(actor.get()));
    std::cout << "✓ Actor validation passed" << std::endl;
    
    manager.OnActorInitialize(actor.get());
    assert(manager.ValidateState(actor.get(), LifecycleState::Active));
    std::cout << "✓ State validation passed" << std::endl;
    
    auto errors = manager.ValidateActorIntegrity(actor.get());
    assert(errors.empty() && "Actor integrity check failed");
    std::cout << "✓ Integrity validation passed" << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Validation test passed" << std::endl;
}

// Test metrics tracking
void TestMetrics() {
    std::cout << "\n=== Testing Metrics ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    
    const int actorCount = 5;
    std::vector<std::unique_ptr<TestActor>> actors;
    
    for (int i = 0; i < actorCount; ++i) {
        auto actor = std::make_unique<TestActor>("TestActor" + std::to_string(i));
        manager.OnActorCreate(actor.get());
        manager.OnActorInitialize(actor.get());
        actors.push_back(std::move(actor));
    }
    
    const auto& metrics = manager.GetMetrics();
    assert(metrics.totalCreated == actorCount);
    std::cout << "✓ Created " << metrics.totalCreated << " actors" << std::endl;
    assert(metrics.currentActive == actorCount);
    std::cout << "✓ " << metrics.currentActive << " actors active" << std::endl;
    
    // Update actors
    for (auto& actor : actors) {
        for (int i = 0; i < 10; ++i) {
            manager.OnActorUpdate(actor.get(), 0.016);
        }
    }
    
    std::cout << "✓ Avg update time: " << metrics.avgUpdateTime << "s" << std::endl;
    std::cout << "✓ Max update time: " << metrics.maxUpdateTime << "s" << std::endl;
    
    // Destroy actors
    for (auto& actor : actors) {
        manager.OnActorDestroy(actor.get());
    }
    
    assert(metrics.totalDestroyed == actorCount);
    std::cout << "✓ Destroyed " << metrics.totalDestroyed << " actors" << std::endl;
    
    std::cout << "✓ Metrics test passed" << std::endl;
}

// Test performance monitoring
void TestPerformanceMonitoring() {
    std::cout << "\n=== Testing Performance Monitoring ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    manager.EnablePerformanceTracking(true);
    
    auto actor = std::make_unique<TestActor>();
    
    manager.OnActorCreate(actor.get());
    manager.OnActorInitialize(actor.get());
    
    // Simulate some updates
    for (int i = 0; i < 100; ++i) {
        manager.OnActorUpdate(actor.get(), 0.016);
    }
    
    const auto& metrics = manager.GetMetrics();
    assert(metrics.totalUpdateTime > 0.0);
    std::cout << "✓ Performance tracking working" << std::endl;
    std::cout << "  Total update time: " << metrics.totalUpdateTime << "s" << std::endl;
    std::cout << "  Avg update time: " << metrics.avgUpdateTime << "s" << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Performance monitoring test passed" << std::endl;
}

// Test lifecycle debugger
void TestDebugger() {
    std::cout << "\n=== Testing Debugger ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    
    LifecycleDebugger::EnableDetailedLogging(true);
    
    auto actor1 = std::make_unique<TestActor>("Actor1");
    auto actor2 = std::make_unique<TestActor>("Actor2");
    
    manager.OnActorCreate(actor1.get());
    manager.OnActorInitialize(actor1.get());
    
    manager.OnActorCreate(actor2.get());
    manager.OnActorInitialize(actor2.get());
    manager.OnActorPause(actor2.get());
    
    std::cout << "\n--- Actor 1 State ---" << std::endl;
    LifecycleDebugger::PrintActorState(actor1.get());
    
    std::cout << "\n--- Actor 2 State ---" << std::endl;
    LifecycleDebugger::PrintActorState(actor2.get());
    
    std::cout << "\n--- Global Metrics ---" << std::endl;
    LifecycleDebugger::PrintGlobalMetrics();
    
    std::cout << "\n--- Active Actors ---" << std::endl;
    LifecycleDebugger::PrintActiveActors();
    
    std::cout << "\n--- State Distribution ---" << std::endl;
    LifecycleDebugger::PrintStateDistribution();
    
    manager.OnActorDestroy(actor1.get());
    manager.OnActorDestroy(actor2.get());
    
    std::cout << "\n✓ Debugger test passed" << std::endl;
}

// Test lifecycle monitor
void TestMonitor() {
    std::cout << "\n=== Testing Monitor ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    
    auto actor = std::make_unique<TestActor>();
    manager.OnActorCreate(actor.get());
    manager.OnActorInitialize(actor.get());
    
    auto report = LifecycleMonitor::GenerateHealthReport();
    std::cout << "Health Report:" << std::endl;
    std::cout << "  Total Actors: " << report.totalActors << std::endl;
    std::cout << "  Healthy Actors: " << report.healthyActors << std::endl;
    std::cout << "  Warnings: " << report.warnings.size() << std::endl;
    std::cout << "  Errors: " << report.errors.size() << std::endl;
    
    std::vector<std::string> issues;
    bool healthy = LifecycleMonitor::CheckActorHealth(actor.get(), issues);
    std::cout << "  Actor Health: " << (healthy ? "Healthy" : "Unhealthy") << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Monitor test passed" << std::endl;
}

// Test optimizer
void TestOptimizer() {
    std::cout << "\n=== Testing Optimizer ===" << std::endl;
    
    LifecycleOptimizer::EnablePooling(true);
    LifecycleOptimizer::EnableBatching(true);
    LifecycleOptimizer::EnableCaching(true);
    
    auto report = LifecycleOptimizer::GenerateOptimizationReport();
    std::cout << "Optimization Report:" << std::endl;
    std::cout << "  Pooled Actors: " << report.pooledActors << std::endl;
    std::cout << "  Batched Operations: " << report.batchedOperations << std::endl;
    std::cout << "  Recommendations: " << report.recommendations.size() << std::endl;
    
    for (const auto& rec : report.recommendations) {
        std::cout << "    - " << rec << std::endl;
    }
    
    auto recommendations = LifecycleOptimizer::GetOptimizationRecommendations();
    std::cout << "  Additional Recommendations: " << recommendations.size() << std::endl;
    
    std::cout << "✓ Optimizer test passed" << std::endl;
}

// Test integration
void TestIntegration() {
    std::cout << "\n=== Testing Integration ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    
    auto actor = std::make_unique<TestActor>();
    manager.OnActorCreate(actor.get());
    manager.OnActorInitialize(actor.get());
    
    // Test JSON export
    std::string json = LifecycleIntegration::ExportMetricsJSON();
    assert(!json.empty());
    std::cout << "✓ JSON export: " << json.length() << " bytes" << std::endl;
    
    // Test CSV export
    std::string csv = LifecycleIntegration::ExportMetricsCSV();
    assert(!csv.empty());
    std::cout << "✓ CSV export: " << csv.length() << " bytes" << std::endl;
    
    // Test external monitor registration
    bool monitorCalled = false;
    LifecycleIntegration::RegisterExternalMonitor("test", 
        [&](const LifecycleMetrics& metrics) {
            monitorCalled = true;
        });
    std::cout << "✓ External monitor registered" << std::endl;
    
    LifecycleIntegration::UnregisterExternalMonitor("test");
    std::cout << "✓ External monitor unregistered" << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Integration test passed" << std::endl;
}

// Test RAII guard
void TestLifecycleGuard() {
    std::cout << "\n=== Testing Lifecycle Guard ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    
    auto actor = std::make_unique<TestActor>();
    manager.OnActorCreate(actor.get());
    manager.OnActorInitialize(actor.get());
    
    auto initialState = manager.GetState(actor.get());
    std::cout << "Initial state: " << StateToString(initialState) << std::endl;
    
    {
        LifecycleGuard guard(actor.get(), LifecycleState::Paused);
        assert(manager.GetState(actor.get()) == LifecycleState::Paused);
        std::cout << "✓ Guard transitioned to Paused" << std::endl;
    }
    
    // Guard doesn't restore by default
    std::cout << "After guard scope: " << StateToString(manager.GetState(actor.get())) << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Lifecycle guard test passed" << std::endl;
}

// Test error handling
void TestErrorHandling() {
    std::cout << "\n=== Testing Error Handling ===" << std::endl;
    
    auto& manager = LifecycleManager::Instance();
    manager.ClearAllHooks();
    manager.ResetMetrics();
    
    bool errorHookCalled = false;
    manager.RegisterErrorHook([&](IActor* actor, LifecycleState state) {
        errorHookCalled = true;
        std::cout << "Error hook called for " << actor->GetName() << std::endl;
    });
    
    auto actor = std::make_unique<TestActor>();
    manager.OnActorCreate(actor.get());
    
    // Trigger an error
    manager.OnActorError(actor.get(), "Test error");
    assert(errorHookCalled);
    std::cout << "✓ Error hook triggered" << std::endl;
    
    const auto& metrics = manager.GetMetrics();
    assert(metrics.totalErrors > 0);
    std::cout << "✓ Error counted in metrics" << std::endl;
    
    manager.OnActorDestroy(actor.get());
    
    std::cout << "✓ Error handling test passed" << std::endl;
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  Actor Lifecycle System Test Suite  " << std::endl;
    std::cout << "======================================" << std::endl;
    
    try {
        TestLifecycleHooks();
        TestStateTransitions();
        TestValidation();
        TestMetrics();
        TestPerformanceMonitoring();
        TestDebugger();
        TestMonitor();
        TestOptimizer();
        TestIntegration();
        TestLifecycleGuard();
        TestErrorHandling();
        
        std::cout << "\n======================================" << std::endl;
        std::cout << "  ALL TESTS PASSED ✓" << std::endl;
        std::cout << "======================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed: " << e.what() << std::endl;
        return 1;
    }
}
