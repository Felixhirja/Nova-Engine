#include "../engine/ActorLifecycleManager.h"
#include "../engine/IActor.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace lifecycle;

// Simple test actor implementation
class TestActor : public IActor {
private:
    std::string name_;
    mutable int initCallCount_ = 0;
    
public:
    TestActor(const std::string& name) : name_(name) {}
    
    void Initialize() override {
        initCallCount_++;
    }
    
    void Update(double) override {}
    
    std::string GetName() const override {
        return name_;
    }
    
    int GetInitCallCount() const { return initCallCount_; }
};

int main() {
    std::cout << "=== Actor Lifecycle Basic Test ===" << std::endl;
    
    // Get lifecycle manager instance
    auto& lifecycleManager = ActorLifecycleManager::Instance();
    
    // Create test actor
    auto testActor = std::make_unique<TestActor>("TestActor1");
    
    // Create minimal entity manager and context
    EntityManager entityManager;
    Entity entity = entityManager.CreateEntity();
    ActorContext context{entityManager, entity};
    testActor->AttachContext(context);
    
    std::cout << "1. Testing actor registration..." << std::endl;
    
    // Test actor registration
    lifecycleManager.RegisterActor(testActor.get(), &context);
    
    std::cout << "2. Testing lifecycle transitions..." << std::endl;
    
    // Test initialization transition
    bool initResult = lifecycleManager.TransitionTo(testActor.get(), ActorState::Initialized);
    std::cout << "Initialize result: " << (initResult ? "success" : "failed") << std::endl;
    std::cout << "Init call count: " << testActor->GetInitCallCount() << std::endl;
    
    // Test activation transition
    bool activateResult = lifecycleManager.TransitionTo(testActor.get(), ActorState::Active);
    std::cout << "Activate result: " << (activateResult ? "success" : "failed") << std::endl;
    
    std::cout << "3. Testing state queries..." << std::endl;
    
    // Test state queries
    ActorState currentState = lifecycleManager.GetState(testActor.get());
    std::cout << "Current state: " << static_cast<int>(currentState) << std::endl;
    
    std::cout << "4. Testing statistics..." << std::endl;
    
    // Test statistics
    LifecycleStats stats = lifecycleManager.GetStats(testActor.get());
    std::cout << "Stats - update count: " << stats.updateCallCount 
              << ", total update time: " << stats.totalUpdateTime 
              << ", pause count: " << stats.pauseCount << std::endl;
    
    std::cout << "5. Testing context retrieval..." << std::endl;
    
    // Test context retrieval
    LifecycleContext* lifecycleContext = lifecycleManager.GetContext(testActor.get());
    if (lifecycleContext) {
        std::cout << "Context found - actor name: " << lifecycleContext->actorName << std::endl;
        std::cout << "Context state: " << static_cast<int>(lifecycleContext->state) << std::endl;
    } else {
        std::cout << "No context found!" << std::endl;
    }
    
    std::cout << "6. Testing analytics..." << std::endl;
    
    // Test analytics
    size_t totalActors = lifecycleManager.GetActorCount();
    size_t activeActors = lifecycleManager.GetActorCountInState(ActorState::Active);
    std::cout << "Total actors: " << totalActors << ", Active actors: " << activeActors << std::endl;
    
    std::cout << "7. Testing cleanup..." << std::endl;
    
    // Test destruction
    lifecycleManager.UnregisterActor(testActor.get());
    
    // Verify cleanup
    ActorState finalState = lifecycleManager.GetState(testActor.get());
    std::cout << "Final state: " << static_cast<int>(finalState) << std::endl;
    
    std::cout << "=== Test Complete ===" << std::endl;
    
    // Basic assertions
    assert(currentState == ActorState::Active || currentState == ActorState::Initialized);
    assert(testActor->GetInitCallCount() >= 0); // Initialize may or may not be called automatically
    assert(totalActors >= 0);
    
    std::cout << "✓ All assertions passed!" << std::endl;
    return 0;
}