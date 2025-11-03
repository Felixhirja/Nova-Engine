#include "../engine/LifecycleAnalytics.h"
#include "../engine/LifecycleActor.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <thread>
#include <chrono>

// Simple test actor for analytics (no continuous monitoring)
class SimpleAnalyticsTestActor : public LifecycleActor {
public:
    SimpleAnalyticsTestActor(const std::string& name) : name_(name) {}
    
    std::string GetName() const override { return name_; }

protected:
    void OnInitialize() override {
        std::cout << "[TestActor] " << name_ << " initialized" << std::endl;
    }
    
    void OnUpdate(double dt) override {
        (void)dt;
        // Simple update logic
    }

private:
    std::string name_;
};

int main() {
    std::cout << "=== Lifecycle Analytics Simple Test ===" << std::endl;
    
    // Initialize ECS and lifecycle systems (without continuous monitoring)
    EntityManager entityManager;
    
    // Initialize analytics only (no background monitoring thread)
    lifecycle::LifecycleAnalytics::Instance().Initialize();
    
    std::cout << "\n--- Testing Analytics Collection ---" << std::endl;
    
    // Create various actors to test analytics
    std::vector<std::unique_ptr<SimpleAnalyticsTestActor>> actors;
    
    // Create some normal actors
    for (int i = 0; i < 5; ++i) {
        auto actor = std::make_unique<SimpleAnalyticsTestActor>("TestActor_" + std::to_string(i));
        Entity entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        actor->AttachContext(context);
        actor->Initialize();
        actors.push_back(std::move(actor));
    }
    
    // Create actors of different types
    for (int i = 0; i < 3; ++i) {
        auto actor = std::make_unique<SimpleAnalyticsTestActor>("SpecialActor_" + std::to_string(i));
        Entity entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        actor->AttachContext(context);
        actor->Initialize();
        actors.push_back(std::move(actor));
    }
    
    // Wait for analytics to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test analytics reporting
    std::cout << "\n--- Analytics Report ---" << std::endl;
    lifecycle::LifecycleAnalytics::Instance().PrintReport();
    
    std::cout << "\n--- Analytics JSON Export ---" << std::endl;
    std::cout << lifecycle::LifecycleAnalytics::Instance().ExportJson() << std::endl;
    
    // Test actor destruction and analytics
    std::cout << "\n--- Testing Actor Destruction Analytics ---" << std::endl;
    for (size_t i = 0; i < actors.size() / 2; ++i) {
        if (actors[i]) {
            actors[i]->Destroy();
        }
    }
    
    // Wait for destruction analytics to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Final analytics report
    std::cout << "\n--- Final Analytics Report ---" << std::endl;
    lifecycle::LifecycleAnalytics::Instance().PrintReport();
    
    // Clear remaining actors
    actors.clear();
    
    // Shutdown analytics
    lifecycle::LifecycleAnalytics::Instance().Shutdown();
    
    std::cout << "\n=== Analytics Test Complete ===" << std::endl;
    return 0;
}