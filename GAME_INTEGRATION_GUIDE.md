# Configuration Management System - Game Integration Guide

## 🎮 Integrating into Nova Engine Game

This guide will help you integrate the Configuration Management System into your Nova Engine game step-by-step.

---

## Step 1: Add to Build System

### Option A: Using Makefile

Add these lines to your main `Makefile`:

```makefile
# Configuration Management Objects
CONFIG_OBJS = engine/config/ConfigManager.o engine/config/ConfigEditor.o

# Build ConfigManager
engine/config/ConfigManager.o: engine/config/ConfigManager.cpp engine/config/ConfigManager.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build ConfigEditor
engine/config/ConfigEditor.o: engine/config/ConfigEditor.cpp engine/config/ConfigEditor.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Add to your main target
nova-engine: $(OBJS) $(CONFIG_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
```

### Option B: Using build_vs.bat (Visual Studio)

Add to your Visual Studio project or update `build_vs.bat`:

```batch
cl /std:c++17 /EHsc /I. ^
   engine/config/ConfigManager.cpp ^
   engine/config/ConfigEditor.cpp ^
   engine/SimpleJson.cpp ^
   [your other source files] ^
   /Fe:nova-engine.exe
```

---

## Step 2: Initialize in Game Startup

### In your main game initialization (e.g., `main.cpp` or game init):

```cpp
#include "engine/config/ConfigManager.h"
#include "engine/config/ConfigEditor.h"

int main() {
    // 1. Initialize Configuration System
    std::cout << "Initializing Configuration System..." << std::endl;
    
    auto& configMgr = NovaEngine::Config::ConfigManager::GetInstance();
    configMgr.Initialize("assets/");
    
    // 2. Enable Performance Caching
    auto& cache = configMgr.GetCache();
    cache.SetCachePolicy(NovaEngine::Config::CachePolicy::LRU, 100); // 100MB cache
    
    // 3. Preload Frequently Used Configs
    std::vector<std::string> commonConfigs = {
        "assets/actors/ships/player.json",
        "assets/actors/ships/fighter.json",
        "assets/actors/ships/cruiser.json",
        "assets/actors/world/station.json",
        "assets/actors/projectiles/laser.json"
    };
    cache.Preload(commonConfigs);
    
    std::cout << "✓ Configuration System Ready!" << std::endl;
    
    // ... rest of your game initialization
    
    return 0;
}
```

---

## Step 3: Replace Existing Config Loading

### Before (Old Way):

```cpp
// Old manual JSON loading
std::ifstream file("assets/actors/ships/player.json");
std::string content((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
auto parseResult = simplejson::Parse(content);
auto config = parseResult.value;
```

### After (New Way):

```cpp
// New ConfigManager loading (cached, validated, with analytics)
auto& configMgr = NovaEngine::Config::ConfigManager::GetInstance();
auto config = configMgr.LoadConfig("assets/actors/ships/player.json");

// Access values easily
if (config.IsObject()) {
    auto& obj = config.AsObject();
    double health = obj["health"].AsNumber(1000.0);
    double speed = obj["speed"].AsNumber(150.0);
    std::string name = obj["name"].AsString("Unknown");
}
```

---

## Step 4: Update Actor/Entity Loading

### Example: Update your Spaceship initialization

```cpp
class Spaceship : public IActor {
private:
    std::string configPath_;
    
public:
    void Initialize() override {
        // Load ship configuration
        auto& configMgr = NovaEngine::Config::ConfigManager::GetInstance();
        auto config = configMgr.LoadConfig(configPath_);
        
        if (!config.IsObject()) {
            std::cerr << "Failed to load ship config: " << configPath_ << std::endl;
            return;
        }
        
        auto& cfg = config.AsObject();
        
        // Apply configuration
        health_ = cfg["health"].AsNumber(1000.0);
        maxSpeed_ = cfg["speed"].AsNumber(200.0);
        shipName_ = cfg["name"].AsString("Unnamed Ship");
        
        // Load weapons from config
        if (cfg.find("weapons") != cfg.end() && cfg["weapons"].IsArray()) {
            auto& weapons = cfg["weapons"].AsArray();
            for (const auto& weaponCfg : weapons) {
                LoadWeapon(weaponCfg);
            }
        }
        
        // Load systems
        if (cfg.find("systems") != cfg.end() && cfg["systems"].IsObject()) {
            auto& systems = cfg["systems"].AsObject();
            engineType_ = systems["engine"].AsString("standard");
            reactorType_ = systems["reactor"].AsString("fusion_mk1");
        }
    }
    
    void SetConfigPath(const std::string& path) {
        configPath_ = path;
    }
};
```

---

## Step 5: Add Configuration Validation

### Create validation tests for your configs:

```cpp
// In your game initialization or test suite
void SetupConfigValidation() {
    using namespace NovaEngine::Config;
    
    auto& testRunner = ConfigTestRunner::GetInstance();
    
    // Ship validation suite
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

// Validate configs on game start
void ValidateGameConfigs() {
    using namespace NovaEngine::Config;
    
    auto& testRunner = ConfigTestRunner::GetInstance();
    
    std::vector<std::string> shipsToValidate = {
        "assets/actors/ships/player.json",
        "assets/actors/ships/fighter.json",
        "assets/actors/ships/cruiser.json"
    };
    
    bool allValid = true;
    for (const auto& shipPath : shipsToValidate) {
        auto report = testRunner.RunTests("Spaceship", shipPath);
        
        if (!report.AllPassed()) {
            std::cerr << "Validation failed for: " << shipPath << std::endl;
            std::cerr << "  Passed: " << report.passedTests << "/" 
                     << report.totalTests << std::endl;
            allValid = false;
        }
    }
    
    if (allValid) {
        std::cout << "✓ All ship configurations validated!" << std::endl;
    }
}
```

---

## Step 6: Use Templates for New Entities

### Create new ships from templates:

```cpp
void CreateNewShipFromTemplate() {
    using namespace NovaEngine::Config;
    
    auto& templateMgr = ConfigTemplateManager::GetInstance();
    
    // Instantiate fighter template
    std::unordered_map<std::string, simplejson::JsonValue> params;
    params["SHIP_NAME"] = simplejson::JsonValue("Interceptor Alpha");
    params["HEALTH"] = simplejson::JsonValue(800.0);
    params["SPEED"] = simplejson::JsonValue(350.0);
    params["FACTION"] = simplejson::JsonValue("player");
    
    auto config = templateMgr.InstantiateTemplate("FighterShip", params);
    
    // Save the new configuration
    std::ofstream file("assets/actors/ships/interceptor_alpha.json");
    file << simplejson::Serialize(config, true);
    file.close();
    
    std::cout << "Created new ship: Interceptor Alpha" << std::endl;
}
```

---

## Step 7: Add Hot-Reload Support (Optional)

### Enable configuration hot-reloading during development:

```cpp
class Game {
private:
    bool devMode_ = true;
    
public:
    void Update(double deltaTime) {
        // In development mode, watch for config changes
        if (devMode_) {
            auto& watcher = NovaEngine::Config::ConfigWatcher::GetInstance();
            
            if (watcher.HasChanges()) {
                auto changedFiles = watcher.GetChangedFiles();
                
                for (const auto& file : changedFiles) {
                    std::cout << "Config changed: " << file << std::endl;
                    ReloadConfig(file);
                }
                
                watcher.ClearChanges();
            }
        }
        
        // ... rest of game update
    }
    
    void ReloadConfig(const std::string& configPath) {
        auto& configMgr = NovaEngine::Config::ConfigManager::GetInstance();
        configMgr.ReloadConfig(configPath);
        
        // Notify affected entities to reload their configs
        NotifyConfigChanged(configPath);
    }
};
```

---

## Step 8: Add Analytics Monitoring

### Track configuration usage:

```cpp
void ShowConfigAnalytics() {
    using namespace NovaEngine::Config;
    
    auto& analytics = ConfigManager::GetInstance().GetAnalytics();
    
    // Get most used configurations
    auto mostUsed = analytics.GetMostUsed(10);
    
    std::cout << "\n=== Configuration Analytics ===" << std::endl;
    std::cout << "Most Used Configurations:" << std::endl;
    
    for (size_t i = 0; i < mostUsed.size(); ++i) {
        const auto& stat = mostUsed[i];
        std::cout << (i + 1) << ". " << stat.configPath << std::endl;
        std::cout << "   Loads: " << stat.loadCount << std::endl;
        std::cout << "   Avg Time: " << stat.avgLoadTimeMs << "ms" << std::endl;
    }
    
    // Find optimization opportunities
    auto unused = analytics.FindUnusedConfigs(30);
    if (!unused.empty()) {
        std::cout << "\nUnused configs (30+ days): " << unused.size() << std::endl;
    }
    
    auto slowest = analytics.GetSlowestLoading(5);
    if (!slowest.empty()) {
        std::cout << "\nSlowest loading configs:" << std::endl;
        for (const auto& stat : slowest) {
            std::cout << "  " << stat.configPath 
                     << " (" << stat.avgLoadTimeMs << "ms)" << std::endl;
        }
    }
}
```

---

## Complete Integration Example

Here's a complete example showing integration in your main game loop:

```cpp
#include "engine/config/ConfigManager.h"
#include "engine/config/ConfigEditor.h"
#include <iostream>

class NovaGame {
private:
    bool running_ = true;
    
public:
    bool Initialize() {
        std::cout << "=== Nova Engine Initialization ===" << std::endl;
        
        // 1. Initialize Config System
        std::cout << "1. Initializing Configuration System..." << std::endl;
        InitializeConfigSystem();
        
        // 2. Setup Validation
        std::cout << "2. Setting up validation..." << std::endl;
        SetupConfigValidation();
        
        // 3. Validate All Configs
        std::cout << "3. Validating configurations..." << std::endl;
        if (!ValidateGameConfigs()) {
            std::cerr << "Configuration validation failed!" << std::endl;
            return false;
        }
        
        // 4. Load Game Entities
        std::cout << "4. Loading game entities..." << std::endl;
        LoadGameEntities();
        
        std::cout << "✓ Initialization Complete!" << std::endl;
        return true;
    }
    
    void Run() {
        while (running_) {
            Update(0.016); // ~60 FPS
            Render();
        }
    }
    
    void Shutdown() {
        std::cout << "\n=== Nova Engine Shutdown ===" << std::endl;
        
        // Show analytics before shutdown
        ShowConfigAnalytics();
        
        std::cout << "✓ Shutdown Complete" << std::endl;
    }
    
private:
    void InitializeConfigSystem() {
        using namespace NovaEngine::Config;
        
        auto& configMgr = ConfigManager::GetInstance();
        configMgr.Initialize("assets/");
        
        // Enable caching
        auto& cache = configMgr.GetCache();
        cache.SetCachePolicy(CachePolicy::LRU, 100);
        
        // Preload common configs
        cache.Preload({
            "assets/actors/ships/player.json",
            "assets/actors/ships/fighter.json",
            "assets/actors/world/station.json"
        });
    }
    
    void SetupConfigValidation() {
        using namespace NovaEngine::Config;
        
        auto& testRunner = ConfigTestRunner::GetInstance();
        ConfigTestSuite shipTests("ShipValidation");
        
        shipTests.AddTest("Health Range", "Health 100-10000",
            [](const simplejson::JsonValue& cfg) {
                if (!cfg.IsObject()) return false;
                auto& obj = cfg.AsObject();
                auto it = obj.find("health");
                if (it == obj.end() || !it->second.IsNumber()) return false;
                double h = it->second.AsNumber();
                return h >= 100.0 && h <= 10000.0;
            });
        
        testRunner.RegisterSuite("Spaceship", shipTests);
    }
    
    bool ValidateGameConfigs() {
        using namespace NovaEngine::Config;
        
        auto& testRunner = ConfigTestRunner::GetInstance();
        auto report = testRunner.RunTests("Spaceship", 
            "assets/actors/ships/player.json");
        
        return report.AllPassed();
    }
    
    void LoadGameEntities() {
        using namespace NovaEngine::Config;
        
        auto& configMgr = ConfigManager::GetInstance();
        
        // Load player ship
        auto playerConfig = configMgr.LoadConfig(
            "assets/actors/ships/player.json");
        CreatePlayerShip(playerConfig);
        
        // Load stations
        auto stationConfig = configMgr.LoadConfig(
            "assets/actors/world/station.json");
        CreateStation(stationConfig);
    }
    
    void CreatePlayerShip(const simplejson::JsonValue& config) {
        // Your ship creation code
        std::cout << "  ✓ Player ship loaded" << std::endl;
    }
    
    void CreateStation(const simplejson::JsonValue& config) {
        // Your station creation code
        std::cout << "  ✓ Station loaded" << std::endl;
    }
    
    void Update(double deltaTime) {
        // Game update logic
    }
    
    void Render() {
        // Game rendering logic
    }
    
    void ShowConfigAnalytics() {
        using namespace NovaEngine::Config;
        
        auto& analytics = ConfigManager::GetInstance().GetAnalytics();
        auto mostUsed = analytics.GetMostUsed(5);
        
        std::cout << "Configuration Usage:" << std::endl;
        for (const auto& stat : mostUsed) {
            std::cout << "  " << stat.configPath 
                     << " (" << stat.loadCount << " loads)" << std::endl;
        }
    }
};

int main() {
    NovaGame game;
    
    if (!game.Initialize()) {
        return 1;
    }
    
    game.Run();
    game.Shutdown();
    
    return 0;
}
```

---

## Testing Your Integration

### 1. Build your game with config system:

```bash
# Windows
build_vs.bat

# Or using Make
make
```

### 2. Run with validation:

```bash
./nova-engine --validate-configs
```

### 3. Check analytics:

```bash
./nova-engine --show-analytics
```

---

## Troubleshooting

### Issue: Configs not loading

**Solution:**
```cpp
// Enable debug output
auto& configMgr = ConfigManager::GetInstance();
configMgr.SetDebugMode(true);
```

### Issue: Cache not working

**Solution:**
```cpp
// Check cache stats
auto& cache = configMgr.GetCache();
auto stats = cache.GetStats();
std::cout << "Hit rate: " << stats.hitRate << "%" << std::endl;

// Increase cache size if needed
cache.SetCachePolicy(CachePolicy::LRU, 200); // Increase to 200MB
```

### Issue: Validation failing

**Solution:**
```cpp
// Get detailed validation errors
auto validation = configMgr.ValidateConfig("config.json");
if (!validation.valid) {
    for (const auto& error : validation.errors) {
        std::cerr << error.path << ": " << error.message << std::endl;
    }
}
```

---

## Best Practices

1. **Always validate configs on startup** in development mode
2. **Use caching** for frequently loaded configs
3. **Monitor analytics** to optimize config usage
4. **Use templates** for similar entities
5. **Test configs** before deploying to production
6. **Enable hot-reload** during development
7. **Track config versions** for backwards compatibility

---

## Next Steps

1. ✅ Add config system to your build
2. ✅ Initialize in game startup
3. ✅ Replace old config loading
4. ✅ Add validation tests
5. ✅ Use templates for new entities
6. ✅ Monitor with analytics

**Your Nova Engine is now powered by the Configuration Management System!** 🚀

---

For more details, see:
- `docs/CONFIGURATION_MANAGEMENT.md` - Complete documentation
- `docs/CONFIG_QUICK_REFERENCE.md` - Quick reference
- `examples/config_management_example.cpp` - Working examples
