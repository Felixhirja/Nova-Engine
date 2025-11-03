/**
 * Example: Framework Manager Integration with Engine Bootstrap
 * 
 * This example demonstrates how to integrate the Framework Management System
 * with the existing EngineBootstrap for a complete initialization sequence.
 */

#include "engine/FrameworkManager.h"
#include "engine/BootstrapConfiguration.h"
#include <iostream>
#include <vector>

// Example: Extended bootstrap with framework management
class EnhancedBootstrap {
public:
    struct Result {
        bool success = true;
        std::vector<std::string> warnings;
        std::vector<std::string> loadedFrameworks;
        ValidationResult frameworkValidation;
    };
    
    Result Initialize(const std::string& configPath) {
        Result result;
        
        std::cout << "=== Enhanced Bootstrap Initialization ===" << std::endl;
        
        // Step 1: Load bootstrap configuration
        std::cout << "[1/4] Loading bootstrap configuration..." << std::endl;
        std::vector<std::string> configWarnings;
        auto config = BootstrapConfiguration::LoadFromFile(configPath, &configWarnings);
        result.warnings.insert(result.warnings.end(), configWarnings.begin(), configWarnings.end());
        
        // Step 2: Register all available frameworks
        std::cout << "[2/4] Registering frameworks..." << std::endl;
        RegisterFrameworks();
        
        // Step 3: Build framework configurations from bootstrap config
        std::cout << "[3/4] Building framework configurations..." << std::endl;
        auto frameworkConfigs = BuildFrameworkConfigs(config);
        
        // Step 4: Load frameworks with dependency resolution
        std::cout << "[4/4] Loading frameworks..." << std::endl;
        auto& fmgr = FrameworkManager::GetInstance();
        result.frameworkValidation = fmgr.LoadFrameworks(frameworkConfigs);
        
        // Check results
        if (!result.frameworkValidation.success) {
            std::cerr << "Framework loading errors:" << std::endl;
            for (const auto& error : result.frameworkValidation.errors) {
                std::cerr << "  - " << error << std::endl;
            }
            result.success = false;
        }
        
        // Add warnings
        for (const auto& warning : result.frameworkValidation.warnings) {
            result.warnings.push_back(warning);
        }
        
        // Get loaded frameworks
        result.loadedFrameworks = fmgr.GetLoadedFrameworks();
        
        if (result.success) {
            std::cout << "\n✓ Bootstrap complete! Loaded " << result.loadedFrameworks.size() 
                      << " frameworks" << std::endl;
        }
        
        return result;
    }
    
    void Shutdown() {
        std::cout << "\n=== Enhanced Bootstrap Shutdown ===" << std::endl;
        
        auto& fmgr = FrameworkManager::GetInstance();
        
        // Get metrics before shutdown
        std::cout << "Framework metrics:" << std::endl;
        auto allMetrics = fmgr.GetAllMetrics();
        for (const auto& [name, metrics] : allMetrics) {
            std::cout << "  " << name << ":" << std::endl;
            std::cout << "    Init time: " << metrics.initializationTimeMs << "ms" << std::endl;
            std::cout << "    Failures: " << metrics.failureCount << std::endl;
            std::cout << "    Health: " << (metrics.isHealthy ? "Healthy" : "Unhealthy") << std::endl;
        }
        
        // Unload all frameworks
        std::cout << "\nUnloading frameworks..." << std::endl;
        fmgr.UnloadAllFrameworks();
        
        std::cout << "✓ Shutdown complete" << std::endl;
    }
    
private:
    void RegisterFrameworks() {
        auto& fmgr = FrameworkManager::GetInstance();
        
        // Register built-in frameworks
        fmgr.RegisterFramework("Graphics", []() {
            return std::make_shared<GraphicsFramework>();
        });
        
        fmgr.RegisterFramework("Audio", []() {
            return std::make_shared<AudioFramework>();
        });
        
        fmgr.RegisterFramework("Input", []() {
            return std::make_shared<InputFramework>();
        });
        
        fmgr.RegisterFramework("Physics", []() {
            return std::make_shared<PhysicsFramework>();
        });
        
        // Register fallbacks for critical frameworks
        fmgr.RegisterFallback("Graphics", []() {
            std::cout << "  Using software renderer fallback" << std::endl;
            return std::make_shared<GraphicsFramework>();  // In real code, would be SoftwareRenderer
        });
        
        std::cout << "  Registered 4 frameworks with 1 fallback" << std::endl;
    }
    
    std::vector<FrameworkConfig> BuildFrameworkConfigs(const BootstrapConfiguration& bootstrap) {
        std::vector<FrameworkConfig> configs;
        
        // Graphics framework
        if (bootstrap.loadRendering) {
            FrameworkConfig config;
            config.name = "Graphics";
            config.description = "OpenGL rendering system";
            config.required = true;
            config.enabled = true;
            config.priority = 100;  // Highest priority
            config.settings["vsync"] = "true";
            config.settings["msaa"] = "4";
            configs.push_back(config);
        }
        
        // Audio framework
        if (bootstrap.loadAudio) {
            FrameworkConfig config;
            config.name = "Audio";
            config.description = "Audio playback system";
            config.required = false;
            config.enabled = true;
            config.priority = 70;
            config.settings["channels"] = "32";
            configs.push_back(config);
        }
        
        // Input framework
        if (bootstrap.loadInput) {
            FrameworkConfig config;
            config.name = "Input";
            config.description = "Input device management";
            config.required = true;
            config.enabled = true;
            config.priority = 90;
            configs.push_back(config);
        }
        
        // Physics framework (depends on graphics for debug rendering)
        FrameworkConfig physicsConfig;
        physicsConfig.name = "Physics";
        physicsConfig.description = "Physics simulation";
        physicsConfig.required = false;
        physicsConfig.enabled = true;
        physicsConfig.priority = 50;
        if (bootstrap.loadRendering) {
            physicsConfig.dependencies = {"Graphics"};  // Optional dependency
        }
        physicsConfig.settings["gravity"] = "9.81";
        physicsConfig.settings["timestep"] = "0.016";
        configs.push_back(physicsConfig);
        
        // Add optional frameworks from bootstrap config
        for (const auto& optionalFramework : bootstrap.optionalFrameworks) {
            FrameworkConfig config;
            config.name = optionalFramework;
            config.description = "Optional framework: " + optionalFramework;
            config.required = false;
            config.enabled = true;
            config.priority = 10;
            configs.push_back(config);
        }
        
        std::cout << "  Built " << configs.size() << " framework configurations" << std::endl;
        return configs;
    }
};

// Example usage
int main() {
    std::cout << "╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Framework Manager + Bootstrap Integration Demo   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    EnhancedBootstrap bootstrap;
    
    // Initialize (would load from assets/bootstrap.json in real code)
    auto result = bootstrap.Initialize("assets/bootstrap.json");
    
    if (result.success) {
        std::cout << "\nLoaded frameworks:" << std::endl;
        for (const auto& framework : result.loadedFrameworks) {
            std::cout << "  ✓ " << framework << std::endl;
        }
    } else {
        std::cerr << "\nBootstrap failed!" << std::endl;
        return 1;
    }
    
    if (!result.warnings.empty()) {
        std::cout << "\nWarnings:" << std::endl;
        for (const auto& warning : result.warnings) {
            std::cout << "  ! " << warning << std::endl;
        }
    }
    
    // Simulate runtime
    std::cout << "\n--- Simulating Runtime ---" << std::endl;
    
    auto& fmgr = FrameworkManager::GetInstance();
    
    // Monitor framework health
    std::cout << "\nHealth check:" << std::endl;
    for (const auto& name : result.loadedFrameworks) {
        bool healthy = fmgr.IsFrameworkHealthy(name);
        std::cout << "  " << name << ": " << (healthy ? "✓ Healthy" : "✗ Unhealthy") << std::endl;
    }
    
    // Example: Hot swap physics framework during runtime
    if (fmgr.IsFrameworkLoaded("Physics") && fmgr.SupportsHotSwap("Physics")) {
        std::cout << "\nDemonstrating hot swap..." << std::endl;
        auto newPhysics = std::make_shared<PhysicsFramework>();
        if (fmgr.HotSwapFramework("Physics", newPhysics)) {
            std::cout << "  ✓ Physics framework hot swapped successfully" << std::endl;
        }
    }
    
    // Example: Runtime configuration change
    std::cout << "\nChanging runtime configuration..." << std::endl;
    fmgr.SetFrameworkSetting("Physics", "gravity", "3.71");  // Mars gravity
    std::cout << "  Set Physics gravity to Mars gravity (3.71 m/s²)" << std::endl;
    
    // Example: Validate all frameworks
    std::cout << "\nValidating all frameworks..." << std::endl;
    auto validation = fmgr.ValidateAllFrameworks();
    if (validation.success) {
        std::cout << "  ✓ All frameworks validated successfully" << std::endl;
    } else {
        std::cout << "  ✗ Validation failed" << std::endl;
        for (const auto& error : validation.errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    
    // Example: Run automated tests
    std::cout << "\nRunning framework tests..." << std::endl;
    auto testResults = fmgr.RunAllTests();
    for (const auto& [name, testResult] : testResults) {
        std::cout << "  " << name << ": " << (testResult.success ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    // Example: Generate documentation
    std::cout << "\nGenerating documentation..." << std::endl;
    std::string docs = fmgr.GenerateDocumentation();
    std::cout << "  Generated " << docs.length() << " bytes of documentation" << std::endl;
    
    // Shutdown
    bootstrap.Shutdown();
    
    std::cout << "\n╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║            Demo Complete ✓                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;
    
    return 0;
}

/**
 * Expected Output:
 * 
 * ╔════════════════════════════════════════════════════╗
 * ║  Framework Manager + Bootstrap Integration Demo   ║
 * ╚════════════════════════════════════════════════════╝
 * 
 * === Enhanced Bootstrap Initialization ===
 * [1/4] Loading bootstrap configuration...
 * [2/4] Registering frameworks...
 *   Registered 4 frameworks with 1 fallback
 * [3/4] Building framework configurations...
 *   Built 4 framework configurations
 * [4/4] Loading frameworks...
 * [GraphicsFramework] Initializing...
 * [GraphicsFramework] Initialized successfully
 * [FrameworkManager] Loaded framework: Graphics (0.5ms)
 * [InputFramework] Initializing...
 * [InputFramework] Initialized successfully
 * [FrameworkManager] Loaded framework: Input (0.4ms)
 * [AudioFramework] Initializing...
 * [AudioFramework] Initialized successfully
 * [FrameworkManager] Loaded framework: Audio (0.4ms)
 * [PhysicsFramework] Initializing...
 * [PhysicsFramework] Initialized successfully
 * [FrameworkManager] Loaded framework: Physics (0.3ms)
 * 
 * ✓ Bootstrap complete! Loaded 4 frameworks
 * 
 * Loaded frameworks:
 *   ✓ Graphics
 *   ✓ Input
 *   ✓ Audio
 *   ✓ Physics
 * 
 * --- Simulating Runtime ---
 * 
 * Health check:
 *   Graphics: ✓ Healthy
 *   Input: ✓ Healthy
 *   Audio: ✓ Healthy
 *   Physics: ✓ Healthy
 * 
 * Demonstrating hot swap...
 * [PhysicsFramework] Preparing for hot swap...
 * [PhysicsFramework] Initializing...
 * [PhysicsFramework] Initialized successfully
 * [PhysicsFramework] Completing hot swap...
 * [FrameworkManager] Hot swapped framework: Physics
 *   ✓ Physics framework hot swapped successfully
 * 
 * Changing runtime configuration...
 *   Set Physics gravity to Mars gravity (3.71 m/s²)
 * 
 * Validating all frameworks...
 *   ✓ All frameworks validated successfully
 * 
 * Running framework tests...
 *   Graphics: ✓ PASS
 *   Input: ✓ PASS
 *   Audio: ✓ PASS
 *   Physics: ✓ PASS
 * 
 * Generating documentation...
 *   Generated 1234 bytes of documentation
 * 
 * === Enhanced Bootstrap Shutdown ===
 * Framework metrics:
 *   Graphics:
 *     Init time: 0.5ms
 *     Failures: 0
 *     Health: Healthy
 *   Input:
 *     Init time: 0.4ms
 *     Failures: 0
 *     Health: Healthy
 *   Audio:
 *     Init time: 0.4ms
 *     Failures: 0
 *     Health: Healthy
 *   Physics:
 *     Init time: 0.3ms
 *     Failures: 0
 *     Health: Healthy
 * 
 * Unloading frameworks...
 * [FrameworkManager] Unloading all frameworks...
 * [PhysicsFramework] Shutting down...
 * [FrameworkManager] Unloaded framework: Physics (0.2ms)
 * [AudioFramework] Shutting down...
 * [FrameworkManager] Unloaded framework: Audio (0.2ms)
 * [InputFramework] Shutting down...
 * [FrameworkManager] Unloaded framework: Input (0.2ms)
 * [GraphicsFramework] Shutting down...
 * [FrameworkManager] Unloaded framework: Graphics (0.2ms)
 * ✓ Shutdown complete
 * 
 * ╔════════════════════════════════════════════════════╗
 * ║            Demo Complete ✓                         ║
 * ╚════════════════════════════════════════════════════╝
 */
