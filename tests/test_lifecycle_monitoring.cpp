#include "../engine/LifecycleMonitoring.h"
#include "../engine/LifecycleActor.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <thread>
#include <chrono>

// Simple test actor for monitoring
class MonitoringTestActor : public LifecycleActor {
public:
    MonitoringTestActor(const std::string& name, bool slowInit = false) 
        : name_(name), slowInit_(slowInit) {}
    
    std::string GetName() const override { return name_; }

protected:
    void OnInitialize() override {
        if (slowInit_) {
            // Simulate slow initialization
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
        }
        std::cout << "[TestActor] " << name_ << " initialized" << std::endl;
    }
    
    void OnUpdate(double dt) override {
        (void)dt;
        // Simple update logic
    }

private:
    std::string name_;
    bool slowInit_;
};

int main() {
    std::cout << "=== Lifecycle Monitoring Test ===" << std::endl;
    
    // Initialize ECS and lifecycle systems
    EntityManager entityManager;
    lifecycle_utils::InitializeLifecycleSystem();
    
    std::cout << "\n--- Testing Real-time Monitoring ---" << std::endl;
    
    // Create various actors to trigger monitoring events
    std::vector<std::unique_ptr<MonitoringTestActor>> actors;
    
    // Create some normal actors
    for (int i = 0; i < 5; ++i) {
        auto actor = std::make_unique<MonitoringTestActor>("NormalActor_" + std::to_string(i));
        Entity entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        actor->AttachContext(context);
        actor->Initialize();
        actors.push_back(std::move(actor));
    }
    
    // Create a slow-initializing actor to trigger monitoring alert
    {
        auto slowActor = std::make_unique<MonitoringTestActor>("SlowActor", true);
        Entity entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        slowActor->AttachContext(context);
        slowActor->Initialize();  // This should trigger a slow init alert
        actors.push_back(std::move(slowActor));
    }
    
    // Wait a moment for monitoring to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test manual alert generation
    lifecycle::LifecycleMonitor::Instance().AddAlert(
        lifecycle::AlertLevel::Warning, 
        "Test manual alert", 
        "TestType", 
        "TestActor", 
        42.0
    );
    
    // Generate many actors quickly to trigger creation rate alert
    std::cout << "\n--- Testing High Creation Rate Detection ---" << std::endl;
    for (int i = 0; i < 60; ++i) {
        auto actor = std::make_unique<MonitoringTestActor>("BurstActor_" + std::to_string(i));
        Entity entity = entityManager.CreateEntity();
        ActorContext context(entityManager, entity);
        actor->AttachContext(context);
        actor->Initialize();
        actors.push_back(std::move(actor));
    }
    
    // Wait for monitoring to detect the burst
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Print real-time monitoring dashboard
    std::cout << "\n--- Real-time Monitoring Dashboard ---" << std::endl;
    lifecycle::LifecycleMonitor::Instance().PrintDashboard();
    
    // Test console commands
    std::cout << "\n--- Testing Console Commands ---" << std::endl;
    lifecycle::LifecycleConsoleCommands::ExecuteCommand("lifecycle.status");
    
    std::cout << "\n--- Testing Analytics Export ---" << std::endl;
    std::cout << "Analytics JSON:" << std::endl;
    std::cout << lifecycle::LifecycleAnalytics::Instance().ExportJson() << std::endl;
    
    std::cout << "\n--- Testing Monitoring Export ---" << std::endl;
    std::cout << "Monitoring JSON (first 500 chars):" << std::endl;
    auto monitoringJson = lifecycle::LifecycleMonitor::Instance().ExportMonitoringData();
    std::cout << monitoringJson.substr(0, 500) << "..." << std::endl;
    
    // Test quick health check
    std::cout << "\n--- Quick Health Check ---" << std::endl;
    lifecycle::monitoring_utils::PrintQuickHealthCheck();
    
    // Test periodic monitoring (simulate some time passing)
    std::cout << "\n--- Simulating Runtime (2 seconds) ---" << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        if (elapsed >= 2.0) break;
        
        // Update some actors
        for (auto& actor : actors) {
            if (actor && actor->IsActive()) {
                actor->Update(0.016);  // ~60 FPS
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Slower updates for testing
    }
    
    std::cout << "Runtime simulation complete." << std::endl;
    
    // Final monitoring report
    std::cout << "\n--- Final Monitoring Report ---" << std::endl;
    lifecycle::LifecycleMonitor::Instance().PrintDashboard();
    
    // Cleanup - destroy some actors to test destruction monitoring
    std::cout << "\n--- Testing Actor Destruction Monitoring ---" << std::endl;
    for (size_t i = 0; i < actors.size() / 2; ++i) {
        if (actors[i]) {
            actors[i]->Destroy();
        }
    }
    actors.clear();  // Clear remaining actors
    
    // Final analytics report
    std::cout << "\n--- Final Analytics Report ---" << std::endl;
    lifecycle::LifecycleAnalytics::Instance().PrintReport();
    
    // Shutdown lifecycle system
    lifecycle_utils::ShutdownLifecycleSystem();
    
    std::cout << "\n=== Monitoring Test Complete ===" << std::endl;
    return 0;
}