// Comprehensive test suite for Framework Management System
// Tests all 10 roadmap features
// Compile: g++ -std=c++17 test_framework_roadmap.cpp engine/FrameworkManager.cpp -o test_framework_roadmap

#include "engine/FrameworkManager.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

// Test framework counter
int g_testsRun = 0;
int g_testsPassed = 0;
int g_testsFailed = 0;

#define TEST(name) \
    void name(); \
    struct name##_Runner { \
        name##_Runner() { \
            g_testsRun++; \
            std::cout << "Running test: " << #name << "... "; \
            try { \
                name(); \
                g_testsPassed++; \
                std::cout << "PASS ✓\n"; \
            } catch (const std::exception& e) { \
                g_testsFailed++; \
                std::cout << "FAIL ✗\n"; \
                std::cerr << "  Error: " << e.what() << "\n"; \
            } catch (...) { \
                g_testsFailed++; \
                std::cout << "FAIL ✗ (unknown exception)\n"; \
            } \
        } \
    } name##_runner; \
    void name()

// Mock frameworks for testing
class TestFrameworkA : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        state_ = FrameworkState::Running;
        return true;
    }
    void Shutdown() override { state_ = FrameworkState::Unloaded; }
    bool Validate() const override { return state_ == FrameworkState::Running; }
    std::string GetName() const override { return "TestA"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

class TestFrameworkB : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        state_ = FrameworkState::Running;
        return true;
    }
    void Shutdown() override { state_ = FrameworkState::Unloaded; }
    bool Validate() const override { return state_ == FrameworkState::Running; }
    std::string GetName() const override { return "TestB"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

class HotSwappableFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        state_ = FrameworkState::Running;
        return true;
    }
    void Shutdown() override { state_ = FrameworkState::Unloaded; }
    bool Validate() const override { return true; }
    std::string GetName() const override { return "HotSwappable"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool SupportsHotSwap() const override { return true; }
    bool PrepareForSwap() override { swapPrepared_ = true; return true; }
    bool CompleteSwap() override { swapCompleted_ = true; return true; }
    
    bool swapPrepared_ = false;
    bool swapCompleted_ = false;
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

class FailingFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        return false;  // Always fails
    }
    void Shutdown() override {}
    bool Validate() const override { return false; }
    std::string GetName() const override { return "Failing"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return FrameworkState::Failed; }
};

class FallbackFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        state_ = FrameworkState::Running;
        return true;
    }
    void Shutdown() override { state_ = FrameworkState::Unloaded; }
    bool Validate() const override { return true; }
    std::string GetName() const override { return "Fallback"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

class UnhealthyFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        state_ = FrameworkState::Running;
        return true;
    }
    void Shutdown() override { state_ = FrameworkState::Unloaded; }
    bool Validate() const override { return false; }
    std::string GetName() const override { return "Unhealthy"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    bool IsHealthy() const override { return false; }
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

// ============================================================================
// Test Suite: Dynamic Framework Loading
// ============================================================================

TEST(DynamicLoading_RegisterFramework) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("DynamicTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    auto available = fm.GetAvailableFrameworks();
    bool found = false;
    for (const auto& name : available) {
        if (name == "DynamicTest") {
            found = true;
            break;
        }
    }
    assert(found && "Framework should be registered");
}

TEST(DynamicLoading_LoadFramework) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("LoadTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "LoadTest";
    config.enabled = true;
    
    bool loaded = fm.LoadFramework("LoadTest", config);
    assert(loaded && "Framework should load successfully");
    assert(fm.IsFrameworkLoaded("LoadTest") && "Framework should be loaded");
    
    fm.UnloadFramework("LoadTest");
}

TEST(DynamicLoading_DisabledFramework) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("DisabledTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "DisabledTest";
    config.enabled = false;  // Disabled
    
    std::vector<FrameworkConfig> configs = {config};
    ValidationResult result = fm.LoadFrameworks(configs);
    
    // Should succeed but not load disabled framework
    assert(result.success && "Should succeed with disabled framework");
    assert(!fm.IsFrameworkLoaded("DisabledTest") && "Disabled framework should not load");
}

// ============================================================================
// Test Suite: Framework Dependencies
// ============================================================================

TEST(Dependencies_SimpleChain) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("DepA", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    fm.RegisterFramework("DepB", []() {
        return std::make_shared<TestFrameworkB>();
    });
    
    FrameworkConfig configA;
    configA.name = "DepA";
    
    FrameworkConfig configB;
    configB.name = "DepB";
    configB.dependencies = {"DepA"};  // B depends on A
    
    // Load A first, then B
    assert(fm.LoadFramework("DepA", configA) && "DepA should load");
    assert(fm.LoadFramework("DepB", configB) && "DepB should load with dependency");
    
    fm.UnloadFramework("DepB");
    fm.UnloadFramework("DepA");
}

TEST(Dependencies_AutoResolve) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("AutoA", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    fm.RegisterFramework("AutoB", []() {
        return std::make_shared<TestFrameworkB>();
    });
    
    std::vector<FrameworkConfig> configs;
    
    FrameworkConfig configB;
    configB.name = "AutoB";
    configB.dependencies = {"AutoA"};
    
    FrameworkConfig configA;
    configA.name = "AutoA";
    
    // Add in wrong order
    configs.push_back(configB);
    configs.push_back(configA);
    
    // Should auto-resolve to correct order
    ValidationResult result = fm.LoadFrameworks(configs);
    assert(result.success && "Dependencies should auto-resolve");
    
    fm.UnloadFramework("AutoB");
    fm.UnloadFramework("AutoA");
}

TEST(Dependencies_CircularDetection) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    std::vector<FrameworkConfig> configs;
    
    FrameworkConfig configX;
    configX.name = "CircX";
    configX.dependencies = {"CircY"};
    
    FrameworkConfig configY;
    configY.name = "CircY";
    configY.dependencies = {"CircX"};  // Circular!
    
    configs.push_back(configX);
    configs.push_back(configY);
    
    // Should detect circular dependency
    bool exceptionThrown = false;
    try {
        fm.ResolveDependencyOrder(configs);
    } catch (const std::runtime_error&) {
        exceptionThrown = true;
    }
    
    assert(exceptionThrown && "Should detect circular dependency");
}

// ============================================================================
// Test Suite: Framework Validation
// ============================================================================

TEST(Validation_SingleFramework) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("ValidTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "ValidTest";
    
    fm.LoadFramework("ValidTest", config);
    
    ValidationResult result = fm.ValidateFramework("ValidTest");
    assert(result.success && "Framework should validate");
    
    fm.UnloadFramework("ValidTest");
}

TEST(Validation_AllFrameworks) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("Valid1", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    fm.RegisterFramework("Valid2", []() {
        return std::make_shared<TestFrameworkB>();
    });
    
    FrameworkConfig config1;
    config1.name = "Valid1";
    
    FrameworkConfig config2;
    config2.name = "Valid2";
    
    fm.LoadFramework("Valid1", config1);
    fm.LoadFramework("Valid2", config2);
    
    ValidationResult result = fm.ValidateAllFrameworks();
    assert(result.success && "All frameworks should validate");
    
    fm.UnloadFramework("Valid2");
    fm.UnloadFramework("Valid1");
}

TEST(Validation_Compatibility) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("CompatTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "CompatTest";
    config.dependencies = {"NonExistent"};
    
    ValidationResult result = fm.CheckCompatibility("CompatTest", config);
    assert(!result.success && "Should fail with unknown dependency");
}

// ============================================================================
// Test Suite: Framework Profiling
// ============================================================================

TEST(Profiling_InitializationTime) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("ProfileTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "ProfileTest";
    
    fm.LoadFramework("ProfileTest", config);
    
    FrameworkMetrics metrics = fm.GetFrameworkMetrics("ProfileTest");
    assert(metrics.initializationTimeMs >= 0 && "Should have initialization time");
    
    fm.UnloadFramework("ProfileTest");
}

TEST(Profiling_AllMetrics) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("Metric1", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    fm.RegisterFramework("Metric2", []() {
        return std::make_shared<TestFrameworkB>();
    });
    
    FrameworkConfig config1;
    config1.name = "Metric1";
    
    FrameworkConfig config2;
    config2.name = "Metric2";
    
    fm.LoadFramework("Metric1", config1);
    fm.LoadFramework("Metric2", config2);
    
    auto allMetrics = fm.GetAllMetrics();
    assert(allMetrics.size() >= 2 && "Should have metrics for loaded frameworks");
    
    fm.UnloadFramework("Metric2");
    fm.UnloadFramework("Metric1");
}

// ============================================================================
// Test Suite: Framework Hot Swapping
// ============================================================================

TEST(HotSwap_Support) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("SwapTest", []() {
        return std::make_shared<HotSwappableFramework>();
    });
    
    FrameworkConfig config;
    config.name = "SwapTest";
    
    fm.LoadFramework("SwapTest", config);
    
    assert(fm.SupportsHotSwap("SwapTest") && "Should support hot swap");
    
    fm.UnloadFramework("SwapTest");
}

TEST(HotSwap_Execute) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("SwapExec", []() {
        return std::make_shared<HotSwappableFramework>();
    });
    
    FrameworkConfig config;
    config.name = "SwapExec";
    
    fm.LoadFramework("SwapExec", config);
    
    auto newInstance = std::make_shared<HotSwappableFramework>();
    bool swapped = fm.HotSwapFramework("SwapExec", newInstance);
    
    assert(swapped && "Hot swap should succeed");
    assert(newInstance->swapCompleted_ && "Swap should be completed");
    
    fm.UnloadFramework("SwapExec");
}

// ============================================================================
// Test Suite: Framework Configuration
// ============================================================================

TEST(Configuration_Settings) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("ConfigTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "ConfigTest";
    config.settings["key1"] = "value1";
    config.settings["key2"] = "value2";
    
    fm.LoadFramework("ConfigTest", config);
    
    std::string value = fm.GetFrameworkSetting("ConfigTest", "key1");
    assert(value == "value1" && "Should retrieve setting");
    
    fm.UnloadFramework("ConfigTest");
}

TEST(Configuration_RuntimeModification) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("RuntimeConfig", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "RuntimeConfig";
    
    fm.LoadFramework("RuntimeConfig", config);
    
    fm.SetFrameworkSetting("RuntimeConfig", "newKey", "newValue");
    std::string value = fm.GetFrameworkSetting("RuntimeConfig", "newKey");
    
    assert(value == "newValue" && "Should modify settings at runtime");
    
    fm.UnloadFramework("RuntimeConfig");
}

// ============================================================================
// Test Suite: Framework Monitoring
// ============================================================================

TEST(Monitoring_Health) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("HealthyTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "HealthyTest";
    
    fm.LoadFramework("HealthyTest", config);
    
    bool healthy = fm.IsFrameworkHealthy("HealthyTest");
    assert(healthy && "Framework should be healthy");
    
    fm.UnloadFramework("HealthyTest");
}

TEST(Monitoring_UnhealthyDetection) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("UnhealthyTest", []() {
        return std::make_shared<UnhealthyFramework>();
    });
    
    FrameworkConfig config;
    config.name = "UnhealthyTest";
    
    fm.LoadFramework("UnhealthyTest", config);
    
    bool healthy = fm.IsFrameworkHealthy("UnhealthyTest");
    assert(!healthy && "Framework should be unhealthy");
    
    fm.UnloadFramework("UnhealthyTest");
}

// ============================================================================
// Test Suite: Framework Fallbacks
// ============================================================================

TEST(Fallback_Registration) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("FallbackTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "FallbackTest";
    
    fm.LoadFramework("FallbackTest", config);
    
    fm.RegisterFallback("FallbackTest", []() {
        return std::make_shared<FallbackFramework>();
    });
    
    assert(fm.HasFallback("FallbackTest") && "Should have fallback registered");
    
    fm.UnloadFramework("FallbackTest");
}

// ============================================================================
// Test Suite: Framework Documentation
// ============================================================================

TEST(Documentation_Generate) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("DocTest", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "DocTest";
    config.description = "Test framework for documentation";
    
    fm.LoadFramework("DocTest", config);
    
    std::string doc = fm.GenerateFrameworkDoc("DocTest");
    assert(!doc.empty() && "Should generate documentation");
    assert(doc.find("Test framework for documentation") != std::string::npos && 
           "Documentation should contain description");
    
    fm.UnloadFramework("DocTest");
}

TEST(Documentation_GenerateAll) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    std::string allDocs = fm.GenerateDocumentation();
    assert(!allDocs.empty() && "Should generate full documentation");
    assert(allDocs.find("Framework Manager Documentation") != std::string::npos &&
           "Documentation should have title");
}

// ============================================================================
// Test Suite: Framework Testing
// ============================================================================

TEST(Testing_SingleFramework) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("TestTarget", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    FrameworkConfig config;
    config.name = "TestTarget";
    
    fm.LoadFramework("TestTarget", config);
    
    ValidationResult result = fm.RunFrameworkTests("TestTarget");
    assert(result.success && "Tests should pass");
    
    fm.UnloadFramework("TestTarget");
}

TEST(Testing_AllFrameworks) {
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    fm.RegisterFramework("Test1", []() {
        return std::make_shared<TestFrameworkA>();
    });
    
    fm.RegisterFramework("Test2", []() {
        return std::make_shared<TestFrameworkB>();
    });
    
    FrameworkConfig config1;
    config1.name = "Test1";
    
    FrameworkConfig config2;
    config2.name = "Test2";
    
    fm.LoadFramework("Test1", config1);
    fm.LoadFramework("Test2", config2);
    
    auto results = fm.RunAllTests();
    assert(results.size() >= 2 && "Should have test results");
    
    fm.UnloadFramework("Test2");
    fm.UnloadFramework("Test1");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "===========================================\n";
    std::cout << "Framework Management System - Test Suite\n";
    std::cout << "Testing all 10 roadmap features\n";
    std::cout << "===========================================\n\n";
    
    // Tests run automatically via static initialization
    
    std::cout << "\n===========================================\n";
    std::cout << "Test Results:\n";
    std::cout << "  Total:  " << g_testsRun << "\n";
    std::cout << "  Passed: " << g_testsPassed << " ✓\n";
    std::cout << "  Failed: " << g_testsFailed << " ✗\n";
    std::cout << "===========================================\n";
    
    // Cleanup
    FrameworkManager::GetInstance().UnloadAllFrameworks();
    
    return g_testsFailed == 0 ? 0 : 1;
}
