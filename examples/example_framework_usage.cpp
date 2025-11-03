// Example demonstrating comprehensive framework management features
// Compile: g++ -std=c++17 example_framework_usage.cpp engine/FrameworkManager.cpp -o example_framework_usage

#include "engine/FrameworkManager.h"
#include <iostream>
#include <chrono>
#include <thread>

// Custom framework example: Network framework with hot-swap support
class NetworkFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        std::cout << "[NetworkFramework] Initializing...\n";
        state_ = FrameworkState::Initializing;
        
        // Read configuration
        auto it = config.settings.find("port");
        if (it != config.settings.end()) {
            port_ = std::stoi(it->second);
            std::cout << "[NetworkFramework] Using port: " << port_ << "\n";
        }
        
        // Simulate network initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        state_ = FrameworkState::Running;
        metrics_.isHealthy = true;
        metrics_.memoryUsageBytes = 1024 * 1024;  // 1MB
        
        std::cout << "[NetworkFramework] Initialized successfully\n";
        return true;
    }
    
    void Shutdown() override {
        std::cout << "[NetworkFramework] Shutting down...\n";
        state_ = FrameworkState::Unloading;
        
        // Cleanup
        connectionCount_ = 0;
        
        state_ = FrameworkState::Unloaded;
    }
    
    bool Validate() const override {
        return state_ == FrameworkState::Running && port_ > 0;
    }
    
    std::string GetName() const override { return "Network"; }
    std::string GetVersion() const override { return "2.1.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override {
        return metrics_.isHealthy && 
               state_ == FrameworkState::Running && 
               connectionCount_ < maxConnections_;
    }
    
    FrameworkMetrics GetMetrics() const override {
        return metrics_;
    }
    
    bool SupportsHotSwap() const override { return true; }
    
    bool PrepareForSwap() override {
        std::cout << "[NetworkFramework] Preparing for hot swap...\n";
        std::cout << "[NetworkFramework] Saving " << connectionCount_ << " active connections\n";
        savedConnections_ = connectionCount_;
        return true;
    }
    
    bool CompleteSwap() override {
        std::cout << "[NetworkFramework] Completing hot swap...\n";
        std::cout << "[NetworkFramework] Restoring " << savedConnections_ << " connections\n";
        connectionCount_ = savedConnections_;
        return true;
    }
    
    // Custom methods
    void SimulateConnections(int count) {
        connectionCount_ += count;
        std::cout << "[NetworkFramework] Active connections: " << connectionCount_ << "\n";
    }

private:
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
    int port_ = 8080;
    int connectionCount_ = 0;
    int maxConnections_ = 100;
    int savedConnections_ = 0;
};

// Fallback implementation: Offline mode
class OfflineNetworkFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        std::cout << "[OfflineNetworkFramework] Initializing offline mode...\n";
        state_ = FrameworkState::Running;
        return true;
    }
    
    void Shutdown() override {
        state_ = FrameworkState::Unloaded;
    }
    
    bool Validate() const override { return true; }
    std::string GetName() const override { return "Network (Offline)"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
};

// Example: Failing framework to demonstrate fallback
class FailingFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override {
        std::cout << "[FailingFramework] Initialization failed!\n";
        return false;
    }
    
    void Shutdown() override {}
    bool Validate() const override { return false; }
    std::string GetName() const override { return "Failing"; }
    std::string GetVersion() const override { return "0.1.0"; }
    FrameworkState GetState() const override { return FrameworkState::Failed; }
};

void DemonstrateDynamicLoading() {
    std::cout << "\n=== 1. Dynamic Framework Loading ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Register custom framework
    fm.RegisterFramework("Network", []() {
        return std::make_shared<NetworkFramework>();
    });
    
    // Load with configuration
    FrameworkConfig networkConfig;
    networkConfig.name = "Network";
    networkConfig.version = "2.1.0";
    networkConfig.description = "Network communication framework";
    networkConfig.required = false;
    networkConfig.priority = 50;
    networkConfig.settings["port"] = "9000";
    networkConfig.settings["protocol"] = "TCP";
    
    bool loaded = fm.LoadFramework("Network", networkConfig);
    std::cout << "Network framework loaded: " << (loaded ? "YES" : "NO") << "\n";
}

void DemonstrateDependencies() {
    std::cout << "\n=== 2. Framework Dependencies ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Register frameworks with dependencies
    fm.RegisterFramework("Graphics", []() {
        return std::make_shared<GraphicsFramework>();
    });
    
    fm.RegisterFramework("Physics", []() {
        return std::make_shared<PhysicsFramework>();
    });
    
    // Configure with dependencies
    std::vector<FrameworkConfig> configs;
    
    FrameworkConfig graphicsConfig;
    graphicsConfig.name = "Graphics";
    graphicsConfig.required = true;
    graphicsConfig.priority = 100;
    
    FrameworkConfig physicsConfig;
    physicsConfig.name = "Physics";
    physicsConfig.required = true;
    physicsConfig.priority = 50;
    physicsConfig.dependencies = {"Graphics"};  // Physics depends on Graphics
    
    configs.push_back(physicsConfig);  // Added in wrong order
    configs.push_back(graphicsConfig);
    
    // Batch load - automatically resolves correct order
    ValidationResult result = fm.LoadFrameworks(configs);
    
    if (result.success) {
        std::cout << "All frameworks loaded in correct dependency order\n";
        auto loaded = fm.GetLoadedFrameworks();
        std::cout << "Load order: ";
        for (const auto& name : loaded) {
            std::cout << name << " ";
        }
        std::cout << "\n";
    }
}

void DemonstrateValidation() {
    std::cout << "\n=== 3. Framework Validation ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Validate all frameworks
    ValidationResult result = fm.ValidateAllFrameworks();
    
    std::cout << "Validation result: " << (result.success ? "PASS" : "FAIL") << "\n";
    
    if (!result.errors.empty()) {
        std::cout << "Errors:\n";
        for (const auto& error : result.errors) {
            std::cout << "  - " << error << "\n";
        }
    }
    
    if (!result.warnings.empty()) {
        std::cout << "Warnings:\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  - " << warning << "\n";
        }
    }
}

void DemonstrateProfiling() {
    std::cout << "\n=== 4. Framework Profiling ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    auto allMetrics = fm.GetAllMetrics();
    
    std::cout << "Framework Performance Metrics:\n";
    std::cout << "----------------------------------------\n";
    
    for (const auto& [name, metrics] : allMetrics) {
        std::cout << name << ":\n";
        std::cout << "  Init Time: " << metrics.initializationTimeMs << "ms\n";
        std::cout << "  Shutdown Time: " << metrics.shutdownTimeMs << "ms\n";
        std::cout << "  Memory: " << (metrics.memoryUsageBytes / 1024) << " KB\n";
        std::cout << "  Failures: " << metrics.failureCount << "\n";
        std::cout << "  Health: " << (metrics.isHealthy ? "Healthy" : "UNHEALTHY") << "\n";
        if (!metrics.lastError.empty()) {
            std::cout << "  Last Error: " << metrics.lastError << "\n";
        }
        std::cout << "\n";
    }
}

void DemonstrateHotSwapping() {
    std::cout << "\n=== 5. Framework Hot Swapping ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Check if Physics supports hot swap
    if (fm.SupportsHotSwap("Physics")) {
        std::cout << "Physics framework supports hot swapping\n";
        
        // Create new instance with upgraded implementation
        auto newPhysics = std::make_shared<PhysicsFramework>();
        
        if (fm.HotSwapFramework("Physics", newPhysics)) {
            std::cout << "Successfully hot-swapped Physics framework!\n";
        }
    } else {
        std::cout << "Physics framework does not support hot swapping\n";
    }
}

void DemonstrateConfiguration() {
    std::cout << "\n=== 6. Framework Configuration ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Runtime configuration changes
    fm.SetFrameworkSetting("Network", "port", "7777");
    fm.SetFrameworkSetting("Network", "timeout", "30");
    
    std::cout << "Network port: " << fm.GetFrameworkSetting("Network", "port") << "\n";
    std::cout << "Network timeout: " << fm.GetFrameworkSetting("Network", "timeout") << "\n";
}

void DemonstrateMonitoring() {
    std::cout << "\n=== 7. Framework Monitoring ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    auto frameworks = fm.GetLoadedFrameworks();
    
    std::cout << "Framework Health Status:\n";
    std::cout << "----------------------------------------\n";
    
    for (const auto& name : frameworks) {
        bool healthy = fm.IsFrameworkHealthy(name);
        std::cout << name << ": " << (healthy ? "✓ Healthy" : "✗ UNHEALTHY") << "\n";
    }
}

void DemonstrateFallbacks() {
    std::cout << "\n=== 8. Framework Fallbacks ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Register failing framework
    fm.RegisterFramework("Failing", []() {
        return std::make_shared<FailingFramework>();
    });
    
    // Register fallback
    fm.RegisterFallback("Failing", []() {
        return std::make_shared<OfflineNetworkFramework>();
    });
    
    // Try to load - will fail and use fallback
    FrameworkConfig config;
    config.name = "Failing";
    config.required = false;
    
    bool loaded = fm.LoadFramework("Failing", config);
    std::cout << "Framework loaded (with fallback): " << (loaded ? "YES" : "NO") << "\n";
}

void DemonstrateDocumentation() {
    std::cout << "\n=== 9. Framework Documentation ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Generate documentation for specific framework
    std::string networkDocs = fm.GenerateFrameworkDoc("Network");
    std::cout << "Network Framework Documentation:\n";
    std::cout << networkDocs << "\n";
    
    // Generate full documentation (uncomment to see all)
    // std::string fullDocs = fm.GenerateDocumentation();
    // std::cout << fullDocs << "\n";
}

void DemonstrateTesting() {
    std::cout << "\n=== 10. Framework Testing ===\n";
    
    FrameworkManager& fm = FrameworkManager::GetInstance();
    
    // Run tests on all frameworks
    auto results = fm.RunAllTests();
    
    std::cout << "Framework Test Results:\n";
    std::cout << "----------------------------------------\n";
    
    for (const auto& [name, result] : results) {
        std::cout << name << ": " << (result.success ? "PASS ✓" : "FAIL ✗") << "\n";
        
        if (!result.errors.empty()) {
            for (const auto& error : result.errors) {
                std::cout << "  Error: " << error << "\n";
            }
        }
        
        if (!result.warnings.empty()) {
            for (const auto& warning : result.warnings) {
                std::cout << "  Warning: " << warning << "\n";
            }
        }
    }
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "Framework Management System - Comprehensive Demo\n";
    std::cout << "===========================================\n";
    
    try {
        // Demonstrate all 10 framework management features
        DemonstrateDynamicLoading();
        DemonstrateDependencies();
        DemonstrateValidation();
        DemonstrateProfiling();
        DemonstrateHotSwapping();
        DemonstrateConfiguration();
        DemonstrateMonitoring();
        DemonstrateFallbacks();
        DemonstrateDocumentation();
        DemonstrateTesting();
        
        std::cout << "\n===========================================\n";
        std::cout << "All framework features demonstrated successfully!\n";
        std::cout << "===========================================\n";
        
        // Cleanup
        std::cout << "\nCleaning up...\n";
        FrameworkManager::GetInstance().UnloadAllFrameworks();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
