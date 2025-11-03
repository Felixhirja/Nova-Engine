#include "../engine/LifecycleActor.h"
#include "../engine/ActorContext.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

/**
 * Comprehensive test suite for the enhanced Actor Lifecycle Management system
 */

class ComprehensiveLifecycleActor : public LifecycleActor {
public:
    ComprehensiveLifecycleActor(const std::string& name) : name_(name) {}
    
    std::string GetName() const override { return name_; }

protected:
    void OnInitialize() override {
        std::cout << "[" << name_ << "] Custom initialization..." << std::endl;
        initTime_ = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Simulate work
        initialized_ = true;
    }
    
    void OnUpdate(double dt) override {
        updateCount_++;
        totalDeltaTime_ += dt;
        
        if (updateCount_ % 50 == 0) {
            std::cout << "[" << name_ << "] Update #" << updateCount_ 
                      << " (avg dt=" << (totalDeltaTime_ / updateCount_) << ")" << std::endl;
        }
    }
    
    void OnPause() override {
        std::cout << "[" << name_ << "] Custom pause logic" << std::endl;
        pauseTime_ = std::chrono::high_resolution_clock::now();
    }
    
    void OnResume() override {
        std::cout << "[" << name_ << "] Custom resume logic" << std::endl;
        if (pauseTime_ != std::chrono::high_resolution_clock::time_point{}) {
            auto resumeTime = std::chrono::high_resolution_clock::now();
            auto pauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(resumeTime - pauseTime_);
            totalPauseTime_ += pauseDuration;
        }
    }
    
    void OnDestroy() override {
        std::cout << "[" << name_ << "] Custom cleanup logic" << std::endl;
        destroyed_ = true;
    }

public:
    // Test accessors
    bool IsCustomInitialized() const { return initialized_; }
    bool IsCustomDestroyed() const { return destroyed_; }
    size_t GetUpdateCount() const { return updateCount_; }
    double GetAverageDeltaTime() const { 
        return updateCount_ > 0 ? totalDeltaTime_ / updateCount_ : 0.0; 
    }
    auto GetTotalPauseTime() const { return totalPauseTime_; }

private:
    std::string name_;
    bool initialized_ = false;
    bool destroyed_ = false;
    size_t updateCount_ = 0;
    double totalDeltaTime_ = 0.0;
    std::chrono::high_resolution_clock::time_point initTime_;
    std::chrono::high_resolution_clock::time_point pauseTime_;
    std::chrono::milliseconds totalPauseTime_{0};
};

class PerformanceTestActor : public LifecycleActor {
public:
    PerformanceTestActor(bool slowInit = false) : slowInit_(slowInit) {}
    std::string GetName() const override { return "PerformanceTestActor"; }

protected:
    void OnInitialize() override {
        if (slowInit_) {
            std::cout << "[PerfActor] Simulating slow initialization..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(120)); // Should trigger warning
        }
    }
    
    void OnUpdate(double dt) override {
        // Simulate varying update costs
        if (updateCount_ % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // Expensive update
        }
        updateCount_++;
        (void)dt;
    }

private:
    bool slowInit_;
    size_t updateCount_ = 0;
};

/**
 * Test suite implementation
 */

class LifecycleTestSuite {
public:
    static bool RunAllTests() {
        std::cout << "\n=== Comprehensive Actor Lifecycle Test Suite ===" << std::endl;
        
        // Initialize the lifecycle system
        lifecycle_utils::InitializeLifecycleSystem();
        
        bool allPassed = true;
        
        allPassed &= TestCompleteLifecycle();
        allPassed &= TestStateTransitions();
        allPassed &= TestBatchOperations();
        allPassed &= TestPerformanceMonitoring();
        allPassed &= TestHookSystem();
        allPassed &= TestValidationSystem();
        allPassed &= TestAnalyticsAndStats();
        allPassed &= TestErrorHandling();
        allPassed &= TestConcurrency();
        
        // Always run reporting tests
        TestReportingAndDebugging();
        
        // Cleanup
        lifecycle_utils::ShutdownLifecycleSystem();
        
        return allPassed;
    }

private:
    static bool TestCompleteLifecycle() {
        std::cout << "\n--- Test: Complete Lifecycle ---" << std::endl;
        
        ::EntityManager entityManager;
        auto entity = entityManager.CreateEntity();
        
        ActorContext context(entityManager, entity);
        
        auto actor = std::make_unique<ComprehensiveLifecycleActor>("CompleteTestActor");
        auto* actorPtr = actor.get();
        
        // Test full lifecycle: Creation -> Initialization -> Active -> Pause -> Resume -> Destroy
        
        // 1. Creation and context attachment
        actor->AttachContext(context);
        if (actor->GetLifecycleState() != lifecycle::ActorState::Created) {
            std::cerr << "ERROR: Actor should be in Created state after context attachment" << std::endl;
            return false;
        }
        
        // 2. Initialization
        actor->Initialize();
        if (actor->GetLifecycleState() != lifecycle::ActorState::Active) {
            std::cerr << "ERROR: Actor should be in Active state after initialization" << std::endl;
            return false;
        }
        
        if (!actorPtr->IsCustomInitialized()) {
            std::cerr << "ERROR: Custom initialization should have been called" << std::endl;
            return false;
        }
        
        // 3. Updates
        for (int i = 0; i < 25; ++i) {
            actor->Update(0.016); // 60 FPS
        }
        
        if (actorPtr->GetUpdateCount() != 25) {
            std::cerr << "ERROR: Expected 25 updates, got " << actorPtr->GetUpdateCount() << std::endl;
            return false;
        }
        
        // 4. Pause
        if (!actor->Pause()) {
            std::cerr << "ERROR: Failed to pause actor" << std::endl;
            return false;
        }
        
        if (!actor->IsPaused()) {
            std::cerr << "ERROR: Actor should be paused" << std::endl;
            return false;
        }
        
        // Test that updates don't happen while paused
        size_t updateCountBeforePause = actorPtr->GetUpdateCount();
        actor->Update(0.016);
        if (actorPtr->GetUpdateCount() != updateCountBeforePause) {
            std::cerr << "ERROR: Actor should not update while paused" << std::endl;
            return false;
        }
        
        // 5. Resume
        if (!actor->Resume()) {
            std::cerr << "ERROR: Failed to resume actor" << std::endl;
            return false;
        }
        
        if (!actor->IsActive()) {
            std::cerr << "ERROR: Actor should be active after resume" << std::endl;
            return false;
        }
        
        // Test that updates work again
        actor->Update(0.016);
        if (actorPtr->GetUpdateCount() != updateCountBeforePause + 1) {
            std::cerr << "ERROR: Actor should update after resume" << std::endl;
            return false;
        }
        
        // 6. Destruction
        if (!actor->Destroy()) {
            std::cerr << "ERROR: Failed to destroy actor" << std::endl;
            return false;
        }
        
        if (!actor->IsDestroyed()) {
            std::cerr << "ERROR: Actor should be destroyed" << std::endl;
            return false;
        }
        
        if (!actorPtr->IsCustomDestroyed()) {
            std::cerr << "ERROR: Custom destruction should have been called" << std::endl;
            return false;
        }
        
        std::cout << "✓ Complete lifecycle test passed" << std::endl;
        return true;
    }
    
    static bool TestStateTransitions() {
        std::cout << "\n--- Test: State Transitions ---" << std::endl;
        
        ::EntityManager entityManager;
        auto entity = entityManager.CreateEntity();
        
        ActorContext context;
        context.entityManager = &entityManager;
        context.entity = entity;
        context.debugName = "state_transition_test";
        
        auto actor = std::make_unique<ComprehensiveLifecycleActor>("StateTestActor");
        actor->AttachContext(context);
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Test invalid transitions
        bool invalidResult = manager.TransitionTo(actor.get(), lifecycle::ActorState::Paused);
        if (invalidResult) {
            std::cerr << "ERROR: Invalid transition Created->Paused should fail" << std::endl;
            return false;
        }
        
        // Test valid transition sequence
        std::vector<lifecycle::ActorState> validSequence = {
            lifecycle::ActorState::Initializing,
            lifecycle::ActorState::Initialized,
            lifecycle::ActorState::Active,
            lifecycle::ActorState::Pausing,
            lifecycle::ActorState::Paused,
            lifecycle::ActorState::Resuming,
            lifecycle::ActorState::Active,
            lifecycle::ActorState::Destroying,
            lifecycle::ActorState::Destroyed
        };
        
        for (auto targetState : validSequence) {
            if (!manager.TransitionTo(actor.get(), targetState)) {
                std::cerr << "ERROR: Valid transition to " 
                          << lifecycle::utils::StateToString(targetState) << " failed" << std::endl;
                return false;
            }
            
            if (actor->GetLifecycleState() != targetState) {
                std::cerr << "ERROR: State mismatch after transition" << std::endl;
                return false;
            }
        }
        
        std::cout << "✓ State transitions test passed" << std::endl;
        return true;
    }
    
    static bool TestBatchOperations() {
        std::cout << "\n--- Test: Batch Operations ---" << std::endl;
        
        const size_t numActors = 10;
        ::EntityManager entityManager;
        std::vector<std::unique_ptr<ComprehensiveLifecycleActor>> actors;
        std::vector<IActor*> actorPtrs;
        
        // Create batch of actors
        for (size_t i = 0; i < numActors; ++i) {
            auto entity = entityManager.CreateEntity();
            ActorContext context;
            context.entityManager = &entityManager;
            context.entity = entity;
            context.debugName = "batch_actor_" + std::to_string(i);
            
            auto actor = std::make_unique<ComprehensiveLifecycleActor>("BatchActor" + std::to_string(i));
            actorPtrs.push_back(actor.get());
            
            actor->AttachContext(context);
            actor->Initialize();
            
            actors.push_back(std::move(actor));
        }
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Test batch pause
        manager.BatchTransition(actorPtrs, lifecycle::ActorState::Pausing);
        manager.BatchTransition(actorPtrs, lifecycle::ActorState::Paused);
        
        for (auto* actorPtr : actorPtrs) {
            if (manager.GetState(actorPtr) != lifecycle::ActorState::Paused) {
                std::cerr << "ERROR: Batch pause failed for actor" << std::endl;
                return false;
            }
        }
        
        // Test batch resume
        manager.BatchTransition(actorPtrs, lifecycle::ActorState::Resuming);
        manager.BatchTransition(actorPtrs, lifecycle::ActorState::Active);
        
        // Test batch update
        manager.BatchUpdate(0.016);
        
        // Verify all actors were updated
        for (auto& actor : actors) {
            if (actor->GetUpdateCount() == 0) {
                std::cerr << "ERROR: Actor was not updated in batch operation" << std::endl;
                return false;
            }
        }
        
        std::cout << "✓ Batch operations test passed" << std::endl;
        return true;
    }
    
    static bool TestPerformanceMonitoring() {
        std::cout << "\n--- Test: Performance Monitoring ---" << std::endl;
        
        ::EntityManager entityManager;
        
        // Test normal actor
        {
            auto entity = entityManager.CreateEntity();
            ActorContext context;
            context.entityManager = &entityManager;
            context.entity = entity;
            context.debugName = "fast_actor";
            
            auto actor = std::make_unique<PerformanceTestActor>(false);
            actor->AttachContext(context);
            actor->Initialize();
            
            auto stats = actor->GetLifecycleStats();
            if (stats.getInitializationDuration() > 0.05) { // Should be fast
                std::cerr << "ERROR: Fast actor took too long to initialize: " 
                          << stats.getInitializationDuration() << "s" << std::endl;
                return false;
            }
        }
        
        // Test slow actor (should trigger warnings)
        {
            auto entity = entityManager.CreateEntity();
            ActorContext context;
            context.entityManager = &entityManager;
            context.entity = entity;
            context.debugName = "slow_actor";
            
            auto actor = std::make_unique<PerformanceTestActor>(true);
            actor->AttachContext(context);
            actor->Initialize(); // Should print warning
            
            auto stats = actor->GetLifecycleStats();
            if (stats.getInitializationDuration() < 0.1) {
                std::cerr << "ERROR: Slow actor should have taken longer to initialize" << std::endl;
                return false;
            }
        }
        
        std::cout << "✓ Performance monitoring test passed" << std::endl;
        return true;
    }
    
    static bool TestHookSystem() {
        std::cout << "\n--- Test: Hook System ---" << std::endl;
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Register custom hook
        bool hookCalled = false;
        manager.RegisterHook(lifecycle::LifecycleEvent::PostInitialize, "test_hook",
            [&hookCalled](lifecycle::LifecycleContext& context) {
                hookCalled = true;
                context.SetMetadata("test_key", "test_value");
            });
        
        // Create actor to trigger hook
        ::EntityManager entityManager;
        auto entity = entityManager.CreateEntity();
        ActorContext context;
        context.entityManager = &entityManager;
        context.entity = entity;
        context.debugName = "hook_test";
        
        auto actor = std::make_unique<ComprehensiveLifecycleActor>("HookTestActor");
        actor->AttachContext(context);
        actor->Initialize();
        
        if (!hookCalled) {
            std::cerr << "ERROR: Custom hook was not called" << std::endl;
            return false;
        }
        
        // Verify metadata was set
        auto* lifecycleContext = manager.GetContext(actor.get());
        if (!lifecycleContext || lifecycleContext->GetMetadata("test_key") != "test_value") {
            std::cerr << "ERROR: Hook did not set metadata correctly" << std::endl;
            return false;
        }
        
        // Cleanup hook
        manager.UnregisterHook(lifecycle::LifecycleEvent::PostInitialize, "test_hook");
        
        std::cout << "✓ Hook system test passed" << std::endl;
        return true;
    }
    
    static bool TestValidationSystem() {
        std::cout << "\n--- Test: Validation System ---" << std::endl;
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Register custom validator that blocks certain transitions
        manager.RegisterValidator("test_validator",
            [](const lifecycle::LifecycleContext& context, lifecycle::ActorState newState) {
                // Block transitions to paused state for actors with "no_pause" in name
                if (newState == lifecycle::ActorState::Paused && 
                    context.actorName.find("no_pause") != std::string::npos) {
                    return false;
                }
                return true;
            });
        
        // Test actor that should be blocked
        ::EntityManager entityManager;
        auto entity = entityManager.CreateEntity();
        ActorContext context;
        context.entityManager = &entityManager;
        context.entity = entity;
        context.debugName = "no_pause_test";
        
        auto actor = std::make_unique<ComprehensiveLifecycleActor>("no_pause_actor");
        actor->AttachContext(context);
        actor->Initialize();
        
        // Try to pause (should fail)
        bool pauseResult = actor->Pause();
        if (pauseResult) {
            std::cerr << "ERROR: Validator should have blocked pause transition" << std::endl;
            return false;
        }
        
        if (actor->IsPaused()) {
            std::cerr << "ERROR: Actor should not be paused after blocked transition" << std::endl;
            return false;
        }
        
        // Cleanup validator
        manager.UnregisterValidator("test_validator");
        
        // Now pause should work
        if (!actor->Pause()) {
            std::cerr << "ERROR: Pause should work after removing validator" << std::endl;
            return false;
        }
        
        std::cout << "✓ Validation system test passed" << std::endl;
        return true;
    }
    
    static bool TestAnalyticsAndStats() {
        std::cout << "\n--- Test: Analytics and Statistics ---" << std::endl;
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        size_t initialCount = manager.GetActorCount();
        
        // Create actors and generate some statistics
        ::EntityManager entityManager;
        std::vector<std::unique_ptr<ComprehensiveLifecycleActor>> actors;
        
        const size_t numActors = 5;
        for (size_t i = 0; i < numActors; ++i) {
            auto entity = entityManager.CreateEntity();
            ActorContext context;
            context.entityManager = &entityManager;
            context.entity = entity;
            context.debugName = "stats_test_" + std::to_string(i);
            
            auto actor = std::make_unique<ComprehensiveLifecycleActor>("StatsActor" + std::to_string(i));
            actor->AttachContext(context);
            actor->Initialize();
            
            // Generate some update statistics
            for (int j = 0; j < 20; ++j) {
                actor->Update(0.016);
            }
            
            actors.push_back(std::move(actor));
        }
        
        // Test actor count
        if (manager.GetActorCount() != initialCount + numActors) {
            std::cerr << "ERROR: Actor count mismatch" << std::endl;
            return false;
        }
        
        // Test state counting
        size_t activeCount = manager.GetActorCountInState(lifecycle::ActorState::Active);
        if (activeCount < numActors) {
            std::cerr << "ERROR: Should have " << numActors << " active actors, got " 
                      << activeCount << std::endl;
            return false;
        }
        
        // Test individual actor stats
        auto stats = actors[0]->GetLifecycleStats();
        if (stats.updateCallCount != 20) {
            std::cerr << "ERROR: Expected 20 updates, got " << stats.updateCallCount << std::endl;
            return false;
        }
        
        if (stats.averageUpdateTime <= 0.0) {
            std::cerr << "ERROR: Average update time should be positive" << std::endl;
            return false;
        }
        
        // Test batch statistics
        auto allStats = manager.GetAllStats();
        if (allStats.size() < numActors) {
            std::cerr << "ERROR: Should have stats for all actors" << std::endl;
            return false;
        }
        
        std::cout << "✓ Analytics and statistics test passed" << std::endl;
        return true;
    }
    
    static bool TestErrorHandling() {
        std::cout << "\n--- Test: Error Handling ---" << std::endl;
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Test null actor handling
        bool result = manager.TransitionTo(nullptr, lifecycle::ActorState::Active);
        if (result) {
            std::cerr << "ERROR: Transition with null actor should fail" << std::endl;
            return false;
        }
        
        // Test unregistered actor
        ::EntityManager entityManager;
        auto entity = entityManager.CreateEntity();
        ActorContext context;
        context.entityManager = &entityManager;
        context.entity = entity;
        
        ComprehensiveLifecycleActor unregisteredActor("UnregisteredActor");
        // Don't attach context, so it's not registered
        
        result = manager.TransitionTo(&unregisteredActor, lifecycle::ActorState::Active);
        if (result) {
            std::cerr << "ERROR: Transition with unregistered actor should fail" << std::endl;
            return false;
        }
        
        // Test getting stats for non-existent actor
        auto stats = manager.GetStats(&unregisteredActor);
        if (stats.updateCallCount != 0 || stats.getLifetime() != 0.0) {
            std::cerr << "ERROR: Stats for non-existent actor should be default" << std::endl;
            return false;
        }
        
        std::cout << "✓ Error handling test passed" << std::endl;
        return true;
    }
    
    static bool TestConcurrency() {
        std::cout << "\n--- Test: Basic Concurrency Safety ---" << std::endl;
        
        // This is a basic test since full concurrency testing requires more complex setup
        // We'll test that the lifecycle manager doesn't crash with concurrent access
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        ::EntityManager entityManager;
        std::vector<std::unique_ptr<ComprehensiveLifecycleActor>> actors;
        
        // Create some actors
        for (int i = 0; i < 3; ++i) {
            auto entity = entityManager.CreateEntity();
            ActorContext context;
            context.entityManager = &entityManager;
            context.entity = entity;
            context.debugName = "concurrent_test_" + std::to_string(i);
            
            auto actor = std::make_unique<ComprehensiveLifecycleActor>("ConcurrentActor" + std::to_string(i));
            actor->AttachContext(context);
            actor->Initialize();
            
            actors.push_back(std::move(actor));
        }
        
        // Test concurrent access to different methods
        std::vector<std::thread> threads;
        
        // Thread 1: Query operations
        threads.emplace_back([&manager]() {
            for (int i = 0; i < 10; ++i) {
                auto stats = manager.GetAllStats();
                auto count = manager.GetActorCount();
                (void)stats; (void)count; // Suppress unused warnings
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Thread 2: Batch operations
        threads.emplace_back([&manager]() {
            for (int i = 0; i < 5; ++i) {
                manager.BatchUpdate(0.016);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
        
        // Wait for threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        // If we get here without crashing, the basic concurrency test passed
        std::cout << "✓ Basic concurrency safety test passed" << std::endl;
        return true;
    }
    
    static void TestReportingAndDebugging() {
        std::cout << "\n--- Test: Reporting and Debugging ---" << std::endl;
        
        auto& manager = lifecycle::ActorLifecycleManager::Instance();
        
        // Print debug information
        manager.PrintDebugInfo();
        
        // Print performance report
        lifecycle_utils::PrintPerformanceReport();
        
        // Print lifecycle report
        lifecycle_utils::PrintLifecycleReport();
        
        // Test individual actor reporting
        auto stats = manager.GetAllStats();
        if (!stats.empty()) {
            std::cout << "\nSample actor performance stats:" << std::endl;
            std::cout << "  Lifetime: " << stats[0].getLifetime() << "s" << std::endl;
            std::cout << "  Init time: " << stats[0].getInitializationDuration() << "s" << std::endl;
            std::cout << "  Updates: " << stats[0].updateCallCount << std::endl;
            std::cout << "  Avg update time: " << stats[0].averageUpdateTime << "s" << std::endl;
        }
        
        std::cout << "✓ Reporting and debugging test completed" << std::endl;
    }
};

int main() {
    try {
        bool success = LifecycleTestSuite::RunAllTests();
        
        if (success) {
            std::cout << "\n🎉 All comprehensive lifecycle tests passed!" << std::endl;
            std::cout << "Actor Lifecycle Management system is working correctly." << std::endl;
            return 0;
        } else {
            std::cout << "\n❌ Some comprehensive lifecycle tests failed!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Test suite crashed with exception: " << e.what() << std::endl;
        return 2;
    }
}
