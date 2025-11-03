#include "engine/FrameworkManager.h"
#include <iostream>
#include <cassert>

// Custom test framework for demonstration
class TestFramework : public IFramework {
public:
    explicit TestFramework(const std::string& name) : name_(name) {}
    
    bool Initialize(const FrameworkConfig& config) override {
        std::cout << "[" << name_ << "] Initializing..." << std::endl;
        state_ = FrameworkState::Initializing;
        
        // Simulate initialization work
        for (const auto& [key, value] : config.settings) {
            std::cout << "[" << name_ << "] Setting " << key << " = " << value << std::endl;
        }
        
        state_ = FrameworkState::Running;
        metrics_.isHealthy = true;
        return true;
    }
    
    void Shutdown() override {
        std::cout << "[" << name_ << "] Shutting down..." << std::endl;
        state_ = FrameworkState::Unloading;
        state_ = FrameworkState::Unloaded;
    }
    
    bool Validate() const override {
        return state_ == FrameworkState::Running;
    }
    
    std::string GetName() const override { return name_; }
    std::string GetVersion() const override { return "1.0.0-test"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override {
        return metrics_.isHealthy && state_ == FrameworkState::Running;
    }
    
    FrameworkMetrics GetMetrics() const override { return metrics_; }
    
private:
    std::string name_;
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
};

void TestBasicRegistrationAndLoading() {
    std::cout << "\n=== Test: Basic Registration and Loading ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Register test framework
    fmgr.RegisterFramework("Test1", []() {
        return std::make_shared<TestFramework>("Test1");
    });
    
    // Configure and load
    FrameworkConfig config;
    config.name = "Test1";
    config.description = "Test framework 1";
    config.required = true;
    config.enabled = true;
    config.settings["option1"] = "value1";
    
    bool loaded = fmgr.LoadFramework("Test1", config);
    assert(loaded && "Framework should load successfully");
    assert(fmgr.IsFrameworkLoaded("Test1") && "Framework should be loaded");
    
    // Validate
    auto validation = fmgr.ValidateFramework("Test1");
    assert(validation.success && "Framework should validate successfully");
    
    // Check health
    assert(fmgr.IsFrameworkHealthy("Test1") && "Framework should be healthy");
    
    // Get metrics
    auto metrics = fmgr.GetFrameworkMetrics("Test1");
    std::cout << "Initialization time: " << metrics.initializationTimeMs << "ms" << std::endl;
    
    // Unload
    bool unloaded = fmgr.UnloadFramework("Test1");
    assert(unloaded && "Framework should unload successfully");
    assert(!fmgr.IsFrameworkLoaded("Test1") && "Framework should not be loaded");
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestDependencyResolution() {
    std::cout << "\n=== Test: Dependency Resolution ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Register frameworks
    fmgr.RegisterFramework("Base", []() {
        return std::make_shared<TestFramework>("Base");
    });
    fmgr.RegisterFramework("Dependent", []() {
        return std::make_shared<TestFramework>("Dependent");
    });
    
    // Configure with dependency
    FrameworkConfig baseConfig;
    baseConfig.name = "Base";
    baseConfig.priority = 100;
    
    FrameworkConfig depConfig;
    depConfig.name = "Dependent";
    depConfig.dependencies = {"Base"};
    depConfig.priority = 50;
    
    // Load in wrong order - should be corrected automatically
    std::vector<FrameworkConfig> configs = {depConfig, baseConfig};
    auto result = fmgr.LoadFrameworks(configs);
    
    assert(result.success && "Frameworks should load with dependency resolution");
    assert(fmgr.IsFrameworkLoaded("Base") && "Base framework should be loaded");
    assert(fmgr.IsFrameworkLoaded("Dependent") && "Dependent framework should be loaded");
    
    // Try to unload base (should fail due to dependent)
    bool unloadBase = fmgr.UnloadFramework("Base");
    assert(!unloadBase && "Should not unload framework with dependents");
    
    // Unload dependent first
    fmgr.UnloadFramework("Dependent");
    fmgr.UnloadFramework("Base");
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestHotSwapping() {
    std::cout << "\n=== Test: Hot Swapping ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Load physics framework (supports hot swap)
    fmgr.RegisterFramework("Physics", []() {
        return std::make_shared<PhysicsFramework>();
    });
    
    FrameworkConfig config;
    config.name = "Physics";
    fmgr.LoadFramework("Physics", config);
    
    assert(fmgr.SupportsHotSwap("Physics") && "Physics should support hot swap");
    
    // Create new instance
    auto newInstance = std::make_shared<PhysicsFramework>();
    
    // Hot swap
    bool swapped = fmgr.HotSwapFramework("Physics", newInstance);
    assert(swapped && "Hot swap should succeed");
    assert(fmgr.IsFrameworkLoaded("Physics") && "Framework should still be loaded");
    
    fmgr.UnloadFramework("Physics");
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestFallbackMechanism() {
    std::cout << "\n=== Test: Fallback Mechanism ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Register framework with fallback
    fmgr.RegisterFramework("Graphics", []() {
        return std::make_shared<GraphicsFramework>();
    });
    
    FrameworkConfig config;
    config.name = "Graphics";
    fmgr.LoadFramework("Graphics", config);
    
    // Register fallback
    fmgr.RegisterFallback("Graphics", []() {
        return std::make_shared<TestFramework>("Graphics-Fallback");
    });
    
    assert(fmgr.HasFallback("Graphics") && "Should have fallback");
    
    fmgr.UnloadFramework("Graphics");
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestFrameworkTesting() {
    std::cout << "\n=== Test: Framework Testing ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Load multiple frameworks
    fmgr.RegisterFramework("Test1", []() {
        return std::make_shared<TestFramework>("Test1");
    });
    fmgr.RegisterFramework("Test2", []() {
        return std::make_shared<TestFramework>("Test2");
    });
    
    FrameworkConfig config1;
    config1.name = "Test1";
    fmgr.LoadFramework("Test1", config1);
    
    FrameworkConfig config2;
    config2.name = "Test2";
    fmgr.LoadFramework("Test2", config2);
    
    // Run tests on specific framework
    auto testResult = fmgr.RunFrameworkTests("Test1");
    assert(testResult.success && "Framework tests should pass");
    
    // Run tests on all frameworks
    auto allResults = fmgr.RunAllTests();
    assert(allResults.size() == 2 && "Should test both frameworks");
    
    for (const auto& [name, result] : allResults) {
        std::cout << name << ": " << (result.success ? "PASS" : "FAIL") << std::endl;
        assert(result.success && "All tests should pass");
    }
    
    fmgr.UnloadAllFrameworks();
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestDocumentationGeneration() {
    std::cout << "\n=== Test: Documentation Generation ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Load a framework
    fmgr.RegisterFramework("Audio", []() {
        return std::make_shared<AudioFramework>();
    });
    
    FrameworkConfig config;
    config.name = "Audio";
    config.description = "Audio playback system";
    config.version = "1.0.0";
    fmgr.LoadFramework("Audio", config);
    
    // Generate documentation
    std::string doc = fmgr.GenerateFrameworkDoc("Audio");
    assert(!doc.empty() && "Documentation should be generated");
    std::cout << "Generated documentation:\n" << doc << std::endl;
    
    // Generate full documentation
    std::string fullDoc = fmgr.GenerateDocumentation();
    assert(!fullDoc.empty() && "Full documentation should be generated");
    
    fmgr.UnloadFramework("Audio");
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestMetricsAndMonitoring() {
    std::cout << "\n=== Test: Metrics and Monitoring ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Load frameworks
    fmgr.RegisterFramework("Input", []() {
        return std::make_shared<InputFramework>();
    });
    fmgr.RegisterFramework("Audio", []() {
        return std::make_shared<AudioFramework>();
    });
    
    FrameworkConfig inputConfig;
    inputConfig.name = "Input";
    fmgr.LoadFramework("Input", inputConfig);
    
    FrameworkConfig audioConfig;
    audioConfig.name = "Audio";
    fmgr.LoadFramework("Audio", audioConfig);
    
    // Get all metrics
    auto allMetrics = fmgr.GetAllMetrics();
    assert(allMetrics.size() == 2 && "Should have metrics for both frameworks");
    
    for (const auto& [name, metrics] : allMetrics) {
        std::cout << name << ":" << std::endl;
        std::cout << "  Init time: " << metrics.initializationTimeMs << "ms" << std::endl;
        std::cout << "  Healthy: " << (metrics.isHealthy ? "Yes" : "No") << std::endl;
        std::cout << "  Failures: " << metrics.failureCount << std::endl;
        
        assert(metrics.isHealthy && "Framework should be healthy");
        assert(metrics.initializationTimeMs >= 0 && "Init time should be non-negative");
    }
    
    fmgr.UnloadAllFrameworks();
    
    std::cout << "✓ Test passed" << std::endl;
}

void TestCircularDependencies() {
    std::cout << "\n=== Test: Circular Dependency Detection ===" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Register frameworks
    fmgr.RegisterFramework("A", []() { return std::make_shared<TestFramework>("A"); });
    fmgr.RegisterFramework("B", []() { return std::make_shared<TestFramework>("B"); });
    fmgr.RegisterFramework("C", []() { return std::make_shared<TestFramework>("C"); });
    
    // Create circular dependency: A -> B -> C -> A
    FrameworkConfig configA;
    configA.name = "A";
    configA.dependencies = {"C"};
    
    FrameworkConfig configB;
    configB.name = "B";
    configB.dependencies = {"A"};
    
    FrameworkConfig configC;
    configC.name = "C";
    configC.dependencies = {"B"};
    
    // Try to load - should fail due to circular dependency
    std::vector<FrameworkConfig> configs = {configA, configB, configC};
    auto result = fmgr.LoadFrameworks(configs);
    
    assert(!result.success && "Should detect circular dependency");
    std::cout << "Correctly detected circular dependency" << std::endl;
    
    std::cout << "✓ Test passed" << std::endl;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Framework Management System - Comprehensive Tests    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        TestBasicRegistrationAndLoading();
        TestDependencyResolution();
        TestHotSwapping();
        TestFallbackMechanism();
        TestFrameworkTesting();
        TestDocumentationGeneration();
        TestMetricsAndMonitoring();
        TestCircularDependencies();
        
        std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              ALL TESTS PASSED ✓✓✓                     ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
