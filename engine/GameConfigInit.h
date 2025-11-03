#pragma once

/**
 * Game Configuration Initialization
 * 
 * Drop-in helper to initialize the Configuration Management System
 * in your Nova Engine game.
 * 
 * Usage in your main.cpp:
 *   #include "engine/GameConfigInit.h"
 *   
 *   int main() {
 *       GameConfigInit::Initialize();
 *       // ... your game code
 *       GameConfigInit::Shutdown();
 *   }
 */

#include "config/ConfigManager.h"
#include "config/ConfigEditor.h"
#include <iostream>
#include <vector>
#include <string>

namespace NovaEngine {

class GameConfigInit {
public:
    /**
     * Initialize the Configuration Management System
     * Call this at the start of your game
     */
    static bool Initialize(const std::string& assetsPath = "assets/") {
        std::cout << "Initializing Configuration System..." << std::endl;
        
        try {
            // 1. Initialize ConfigManager
            auto& configMgr = Config::ConfigManager::GetInstance();
            configMgr.Initialize(assetsPath);
            
            // 2. Enable Performance Caching
            auto& cache = configMgr.GetCache();
            cache.SetCachePolicy(Config::CachePolicy::LRU, 100); // 100MB cache
            
            // 3. Preload Common Configurations
            std::vector<std::string> commonConfigs = {
                assetsPath + "actors/ships/player.json",
                assetsPath + "actors/ships/fighter.json",
                assetsPath + "actors/ships/cruiser.json",
                assetsPath + "actors/world/station.json"
            };
            
            // Only preload existing files
            std::vector<std::string> existingConfigs;
            for (const auto& config : commonConfigs) {
                if (std::filesystem::exists(config)) {
                    existingConfigs.push_back(config);
                }
            }
            
            if (!existingConfigs.empty()) {
                cache.Preload(existingConfigs);
                std::cout << "  Preloaded " << existingConfigs.size() 
                         << " configurations" << std::endl;
            }
            
            // 4. Setup Validation (optional but recommended)
            SetupValidation();
            
            std::cout << "✓ Configuration System Ready!" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "✗ Failed to initialize config system: " 
                     << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * Shutdown and show analytics
     * Call this when your game exits
     */
    static void Shutdown() {
        std::cout << "\nConfiguration System Shutdown" << std::endl;
        
        try {
            auto& analytics = Config::ConfigManager::GetInstance().GetAnalytics();
            
            // Show top 5 most used configs
            auto mostUsed = analytics.GetMostUsed(5);
            if (!mostUsed.empty()) {
                std::cout << "\nMost Used Configurations:" << std::endl;
                for (size_t i = 0; i < mostUsed.size(); ++i) {
                    const auto& stat = mostUsed[i];
                    std::cout << "  " << (i + 1) << ". " << stat.configPath 
                             << " (" << stat.loadCount << " loads)" << std::endl;
                }
            }
            
            // Cache statistics
            auto& cache = Config::ConfigManager::GetInstance().GetCache();
            auto cacheStats = cache.GetStats();
            
            std::cout << "\nCache Performance:" << std::endl;
            std::cout << "  Hit Rate: " << cacheStats.hitRate << "%" << std::endl;
            std::cout << "  Total Entries: " << cacheStats.totalEntries << std::endl;
            std::cout << "  Memory Used: " << cacheStats.memoryUsageMB << " MB" << std::endl;
            
        } catch (...) {
            // Ignore errors during shutdown
        }
        
        std::cout << "✓ Configuration System Shutdown Complete" << std::endl;
    }
    
    /**
     * Validate a specific configuration file
     */
    static bool ValidateConfig(const std::string& configPath, 
                               const std::string& configType = "Spaceship") {
        auto& testRunner = Config::ConfigTestRunner::GetInstance();
        auto report = testRunner.RunTests(configType, configPath);
        
        if (report.AllPassed()) {
            std::cout << "✓ " << configPath << " validated successfully" << std::endl;
            return true;
        } else {
            std::cerr << "✗ " << configPath << " validation failed:" << std::endl;
            std::cerr << "  Passed: " << report.passedTests << "/" 
                     << report.totalTests << " tests" << std::endl;
            return false;
        }
    }
    
    /**
     * Reload a configuration (useful for hot-reload during development)
     */
    static void ReloadConfig(const std::string& configPath) {
        auto& configMgr = Config::ConfigManager::GetInstance();
        configMgr.ReloadConfig(configPath);
        std::cout << "Reloaded: " << configPath << std::endl;
    }
    
    /**
     * Get a configuration (cached, validated)
     */
    static simplejson::JsonValue LoadConfig(const std::string& configPath) {
        auto& configMgr = Config::ConfigManager::GetInstance();
        return configMgr.LoadConfig(configPath);
    }
    
private:
    /**
     * Setup default validation rules
     */
    static void SetupValidation() {
        using namespace Config;
        
        auto& testRunner = ConfigTestRunner::GetInstance();
        
        // Spaceship validation
        ConfigTestSuite shipTests("ShipValidation");
        
        shipTests.AddTest("Health Range", 
            "Ship health must be between 100 and 10000",
            [](const simplejson::JsonValue& config) {
                if (!config.IsObject()) return false;
                auto& obj = config.AsObject();
                auto it = obj.find("health");
                if (it == obj.end() || !it->second.IsNumber()) return false;
                double health = it->second.AsNumber();
                return health >= 100.0 && health <= 10000.0;
            });
        
        shipTests.AddTest("Speed Positive",
            "Ship speed must be positive",
            [](const simplejson::JsonValue& config) {
                if (!config.IsObject()) return false;
                auto& obj = config.AsObject();
                auto it = obj.find("speed");
                if (it == obj.end() || !it->second.IsNumber()) return false;
                return it->second.AsNumber() > 0.0;
            });
        
        shipTests.AddTest("Required Fields",
            "Must have name, health, and speed",
            [](const simplejson::JsonValue& config) {
                if (!config.IsObject()) return false;
                auto& obj = config.AsObject();
                return obj.find("name") != obj.end() &&
                       obj.find("health") != obj.end() &&
                       obj.find("speed") != obj.end();
            });
        
        testRunner.RegisterSuite("Spaceship", shipTests);
    }
};

} // namespace NovaEngine
