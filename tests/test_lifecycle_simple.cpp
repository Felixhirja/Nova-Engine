#include "../engine/ActorLifecycleManager.h"
#include "../engine/ecs/EntityManager.h"
#include "../engine/IActor.h"
#include "../engine/ActorContext.h"
#include <iostream>
#include <memory>
#include <vector>

/**
 * Simple lifecycle test using existing actor patterns
 */

class SimpleLifecycleTestActor : public IActor {
public:
    SimpleLifecycleTestActor(const std::string& name) : name_(name) {}
    
    std::string GetName() const override { return name_; }
    
    void Initialize() override {
        std::cout << "[" << name_ << "] Initialize called" << std::endl;
        initialized_ = true;
    }
    
    void Update(double dt) override {
        updateCount_++;
        (void)dt;
    }

    bool IsInitialized() const { return initialized_; }
    size_t GetUpdateCount() const { return updateCount_; }

private:
    std::string name_;
    bool initialized_ = false;
    size_t updateCount_ = 0;
};

bool TestBasicLifecycleManagement() {
    std::cout << "\n=== Testing Basic Lifecycle Management ===" << std::endl;
    
    // Initialize lifecycle system
    auto& manager = lifecycle::ActorLifecycleManager::Instance();
    
    // Configure for testing
    lifecycle::ActorLifecycleManager::Config config;
    config.enableValidation = true;
    config.enableHooks = true;
    config.enableDebugLogging = true;
    manager.SetConfig(config);
    
    // Create EntityManager and actor
    ::EntityManager entityManager;
    auto entity = entityManager.CreateEntity();
    
    ActorContext context(entityManager, entity);
    auto actor = std::make_unique<SimpleLifecycleTestActor>("TestActor");
    SimpleLifecycleTestActor* actorPtr = actor.get();
    
    // Test registration
    manager.RegisterActor(actor.get(), &context);
    
    if (manager.GetState(actor.get()) != lifecycle::ActorState::Created) {
        std::cerr << "ERROR: Actor should be in Created state after registration" << std::endl;
        return false;
    }
    
    // Test state transitions
    if (!manager.TransitionTo(actor.get(), lifecycle::ActorState::Initializing)) {
        std::cerr << "ERROR: Failed to transition to Initializing" << std::endl;
        return false;
    }
    
    if (!manager.TransitionTo(actor.get(), lifecycle::ActorState::Initialized)) {
        std::cerr << "ERROR: Failed to transition to Initialized" << std::endl;
        return false;
    }
    
    if (!manager.TransitionTo(actor.get(), lifecycle::ActorState::Active)) {
        std::cerr << "ERROR: Failed to transition to Active" << std::endl;
        return false;
    }
    
    // Test that we can get stats
    auto stats = manager.GetStats(actor.get());
    if (stats.getLifetime() <= 0.0) {
        std::cerr << "ERROR: Lifetime should be positive" << std::endl;
        return false;
    }
    
    // Test actor count
    size_t activeCount = manager.GetActorCountInState(lifecycle::ActorState::Active);
    if (activeCount == 0) {
        std::cerr << "ERROR: Should have at least one active actor" << std::endl;
        return false;
    }
    
    // Test destruction
    if (!manager.TransitionTo(actor.get(), lifecycle::ActorState::Destroying)) {
        std::cerr << "ERROR: Failed to transition to Destroying" << std::endl;
        return false;
    }
    
    if (!manager.TransitionTo(actor.get(), lifecycle::ActorState::Destroyed)) {
        std::cerr << "ERROR: Failed to transition to Destroyed" << std::endl;
        return false;
    }
    
    // Cleanup
    manager.UnregisterActor(actor.get());
    
    std::cout << "✓ Basic lifecycle management test passed" << std::endl;
    return true;
}

bool TestHooksAndValidation() {
    std::cout << "\n=== Testing Hooks and Validation ===" << std::endl;
    
    auto& manager = lifecycle::ActorLifecycleManager::Instance();
    
    // Register a test hook
    bool hookExecuted = false;
    manager.RegisterHook(lifecycle::LifecycleEvent::PostInitialize, "test_hook",
        [&hookExecuted](lifecycle::LifecycleContext& context) {
            hookExecuted = true;
            std::cout << "[Hook] Actor '" << context.actorName << "' initialized" << std::endl;
        });
    
    // Register a validator
    manager.RegisterValidator("test_validator",
        [](const lifecycle::LifecycleContext& context, lifecycle::ActorState newState) {
            // Always allow transitions for this test
            (void)context; (void)newState;
            return true;
        });
    
    // Create and test actor
    ::EntityManager entityManager;
    auto entity = entityManager.CreateEntity();
    ActorContext context(entityManager, entity);
    auto actor = std::make_unique<SimpleLifecycleTestActor>("HookTestActor");
    
    manager.RegisterActor(actor.get(), &context);
    
    // Transition through states to trigger hooks
    manager.TransitionTo(actor.get(), lifecycle::ActorState::Initializing);
    manager.TransitionTo(actor.get(), lifecycle::ActorState::Initialized);
    
    if (!hookExecuted) {
        std::cerr << "ERROR: Hook was not executed" << std::endl;
        return false;
    }
    
    // Cleanup
    manager.UnregisterActor(actor.get());
    manager.UnregisterHook(lifecycle::LifecycleEvent::PostInitialize, "test_hook");
    manager.UnregisterValidator("test_validator");
    
    std::cout << "✓ Hooks and validation test passed" << std::endl;
    return true;
}

bool TestBatchOperations() {
    std::cout << "\n=== Testing Batch Operations ===" << std::endl;
    
    auto& manager = lifecycle::ActorLifecycleManager::Instance();
    
    // Create multiple actors
    ::EntityManager entityManager;
    std::vector<std::unique_ptr<SimpleLifecycleTestActor>> actors;
    std::vector<IActor*> actorPtrs;
    
    for (int i = 0; i < 3; ++i) {
        auto entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        
        auto actor = std::make_unique<SimpleLifecycleTestActor>("BatchActor" + std::to_string(i));
        actorPtrs.push_back(actor.get());
        
        manager.RegisterActor(actor.get(), &context);
        
        actors.push_back(std::move(actor));
    }
    
    // Test batch transition
    manager.BatchTransition(actorPtrs, lifecycle::ActorState::Initializing);
    manager.BatchTransition(actorPtrs, lifecycle::ActorState::Initialized);
    manager.BatchTransition(actorPtrs, lifecycle::ActorState::Active);
    
    // Verify all actors are in Active state
    for (auto* actorPtr : actorPtrs) {
        if (manager.GetState(actorPtr) != lifecycle::ActorState::Active) {
            std::cerr << "ERROR: Actor should be in Active state after batch transition" << std::endl;
            return false;
        }
    }
    
    // Test batch update
    manager.BatchUpdate(0.016);
    
    // Cleanup
    for (auto& actor : actors) {
        manager.UnregisterActor(actor.get());
    }
    
    std::cout << "✓ Batch operations test passed" << std::endl;
    return true;
}

bool TestAnalytics() {
    std::cout << "\n=== Testing Analytics ===" << std::endl;
    
    auto& manager = lifecycle::ActorLifecycleManager::Instance();
    
    // Create an actor and generate some statistics
    ::EntityManager entityManager;
    auto entity = entityManager.CreateEntity();
    ActorContext context(entityManager, entity);
    auto actor = std::make_unique<SimpleLifecycleTestActor>("AnalyticsActor");
    
    manager.RegisterActor(actor.get(), &context);
    
    // Transition through states
    manager.TransitionTo(actor.get(), lifecycle::ActorState::Initializing);
    manager.TransitionTo(actor.get(), lifecycle::ActorState::Initialized);
    manager.TransitionTo(actor.get(), lifecycle::ActorState::Active);
    
    // Generate update statistics
    for (int i = 0; i < 10; ++i) {
        manager.BatchUpdate(0.016);
    }
    
    // Check statistics
    auto stats = manager.GetStats(actor.get());
    if (stats.getLifetime() <= 0.0) {
        std::cerr << "ERROR: Lifetime should be positive" << std::endl;
        return false;
    }
    
    if (stats.getInitializationDuration() < 0.0) {
        std::cerr << "ERROR: Initialization duration should be non-negative" << std::endl;
        return false;
    }
    
    // Test overall analytics
    auto allStats = manager.GetAllStats();
    if (allStats.empty()) {
        std::cerr << "ERROR: Should have statistics for registered actors" << std::endl;
        return false;
    }
    
    size_t totalActors = manager.GetActorCount();
    if (totalActors == 0) {
        std::cerr << "ERROR: Should have registered actors" << std::endl;
        return false;
    }
    
    // Cleanup
    manager.UnregisterActor(actor.get());
    
    std::cout << "✓ Analytics test passed" << std::endl;
    return true;
}

void TestReporting() {
    std::cout << "\n=== Testing Reporting ===" << std::endl;
    
    auto& manager = lifecycle::ActorLifecycleManager::Instance();
    
    // Print debug information
    manager.PrintDebugInfo();
    
    // Get and print state report
    std::string report = manager.GetStateReport();
    std::cout << "\nState Report:\n" << report << std::endl;
    
    std::cout << "✓ Reporting test completed" << std::endl;
}

int main() {
    std::cout << "=== Actor Lifecycle Management Test ===" << std::endl;
    
    bool allPassed = true;
    
    try {
        allPassed &= TestBasicLifecycleManagement();
        allPassed &= TestHooksAndValidation();
        allPassed &= TestBatchOperations();
        allPassed &= TestAnalytics();
        
        // Always run reporting test
        TestReporting();
        
        // Cleanup system
        lifecycle::ActorLifecycleManager::Instance().DestroyAllActors();
        lifecycle::ActorLifecycleManager::Instance().GarbageCollect();
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 2;
    }
    
    if (allPassed) {
        std::cout << "\n🎉 All lifecycle tests passed!" << std::endl;
        std::cout << "Actor Lifecycle Management system is working correctly." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some lifecycle tests failed!" << std::endl;
        return 1;
    }
}