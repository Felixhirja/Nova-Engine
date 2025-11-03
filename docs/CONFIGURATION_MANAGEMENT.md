# Nova Engine - Configuration Management System

## Overview

The Nova Engine Configuration Management System provides a comprehensive suite of tools for managing, validating, testing, and deploying actor configurations. This system ensures configuration quality, consistency, and reliability across the entire development lifecycle.

## Table of Contents

1. [Configuration Editor](#configuration-editor)
2. [Real-Time Validation](#real-time-validation)
3. [Configuration Templates](#configuration-templates)
4. [Version Control Integration](#version-control-integration)
5. [Deployment Pipeline](#deployment-pipeline)
6. [Automated Testing](#automated-testing)
7. [Documentation Generation](#documentation-generation)
8. [Analytics & Performance](#analytics--performance)
9. [Security Validation](#security-validation)
10. [Performance Optimization](#performance-optimization)

---

## Configuration Editor

### Visual Editor for Actor Configurations

The `ConfigEditor` class provides a visual, interactive interface for editing actor configurations with real-time validation and preview capabilities.

#### Features

- **Real-time validation** during editing
- **Undo/Redo** support with unlimited history
- **Auto-save** functionality
- **Field-level validation** with custom rules
- **Preview mode** to test changes before saving
- **Type-safe field editing**

#### Basic Usage

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

// Create editor instance
ConfigEditor editor;

// Open configuration
if (editor.OpenConfig("assets/actors/ships/player.json")) {
    // Generate layout from config
    auto layout = editor.GenerateLayout("PlayerShip");
    editor.SetCustomLayout(layout);
    
    // Edit fields
    editor.SetFieldValue("health", simplejson::JsonValue(1500.0));
    editor.SetFieldValue("speed", simplejson::JsonValue(200.0));
    
    // Validate changes
    auto validation = editor.ValidateAll();
    if (validation.valid) {
        // Save configuration
        editor.SaveConfig();
    } else {
        // Handle validation errors
        for (const auto& error : validation.errors) {
            std::cerr << "Error at " << error.path << ": " 
                     << error.message << std::endl;
        }
    }
}
```

#### Custom Field Layouts

```cpp
// Create custom layout
EditorLayout layout;
layout.configType = "Spaceship";
layout.title = "Spaceship Configuration Editor";

// Add general properties section
EditorSection general;
general.name = "General";
general.description = "Basic ship properties";

// Add health field
EditorField healthField;
healthField.id = "health";
healthField.label = "Ship Health";
healthField.type = EditorFieldType::Number;
healthField.required = true;
healthField.minValue = 100.0;
healthField.maxValue = 10000.0;
healthField.defaultValue = simplejson::JsonValue(1000.0);
healthField.tooltip = "Maximum health points for the ship";
general.fields.push_back(healthField);

// Add speed field with slider
EditorField speedField;
speedField.id = "speed";
speedField.label = "Maximum Speed";
speedField.type = EditorFieldType::Slider;
speedField.minValue = 50.0;
speedField.maxValue = 500.0;
speedField.defaultValue = simplejson::JsonValue(150.0);
general.fields.push_back(speedField);

layout.AddSection(general);

// Add weapons section
EditorSection weapons;
weapons.name = "Weapons";
weapons.description = "Weapon configuration";

EditorField weaponType;
weaponType.id = "weaponType";
weaponType.label = "Primary Weapon";
weaponType.type = EditorFieldType::Dropdown;
weaponType.allowedValues = {"laser", "plasma", "missile", "railgun"};
weaponType.defaultValue = simplejson::JsonValue("laser");
weapons.fields.push_back(weaponType);

layout.AddSection(weapons);

// Apply custom layout
editor.SetCustomLayout(layout);
```

#### Callbacks and Events

```cpp
// Validation callback
editor.SetValidationCallback([](const ValidationResult& result) {
    if (!result.valid) {
        std::cout << "Validation failed with " << result.errors.size() 
                 << " errors" << std::endl;
    }
});

// Change callback
editor.SetChangeCallback([](const std::string& fieldId, 
                           const simplejson::JsonValue& value) {
    std::cout << "Field " << fieldId << " changed to: " 
             << simplejson::Serialize(value) << std::endl;
});

// Enable auto-save every 60 seconds
editor.EnableAutoSave(60);
```

---

## Real-Time Validation

### Live Configuration Validation

The `RealTimeValidator` provides instant feedback as configurations are edited, catching errors before they cause runtime issues.

#### Usage

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

RealTimeValidator validator;

// Start validation session
validator.StartValidation("assets/actors/ships/fighter.json");

// Add validation listener
validator.AddListener([](const ValidationResult& result) {
    if (!result.valid) {
        std::cout << "Validation errors detected:" << std::endl;
        for (const auto& error : result.errors) {
            std::cout << "  - " << error.message << std::endl;
        }
    }
});

// Validate field as user types
auto result = validator.ValidateIncremental("speed", 
                                           simplejson::JsonValue(250.0));

// Set custom validation delay (default: 500ms)
validator.SetValidationDelay(300);
```

#### Custom Validation Rules

```cpp
auto& manager = ConfigManager::GetInstance();
auto& validator = manager.GetValidator();

// Register custom validator
validator.RegisterCustomValidator("positive_number", 
    [](const simplejson::JsonValue& value) {
        return value.IsNumber() && value.AsNumber() > 0.0;
    });

// Register range validator
validator.RegisterCustomValidator("speed_range",
    [](const simplejson::JsonValue& value) {
        if (!value.IsNumber()) return false;
        double speed = value.AsNumber();
        return speed >= 50.0 && speed <= 500.0;
    });
```

---

## Configuration Templates

### Template System for Common Configurations

Create reusable templates for common actor configurations to ensure consistency and reduce duplication.

#### Creating Templates

Create a template file: `assets/templates/ship_template.json`

```json
{
    "template": true,
    "name": "${SHIP_NAME}",
    "class": "${SHIP_CLASS}",
    "health": "${HEALTH:1000}",
    "speed": "${SPEED:150}",
    "shield": "${SHIELD:500}",
    "weapons": "${WEAPONS:[]}",
    "faction": "${FACTION:neutral}"
}
```

#### Using Templates

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

auto& templateMgr = ConfigTemplateManager::GetInstance();

// Register template
ConfigTemplateManager::TemplateInfo info;
info.name = "BasicShip";
info.category = "Ships";
info.description = "Basic ship configuration template";
info.tags = {"ship", "basic", "fighter"};
info.author = "Nova Engine";
info.version = "1.0";

templateMgr.RegisterTemplate("BasicShip", 
                            "assets/templates/ship_template.json",
                            info);

// Instantiate template
std::unordered_map<std::string, simplejson::JsonValue> params;
params["SHIP_NAME"] = simplejson::JsonValue("Scout Fighter");
params["SHIP_CLASS"] = simplejson::JsonValue("Scout");
params["HEALTH"] = simplejson::JsonValue(800.0);
params["SPEED"] = simplejson::JsonValue(300.0);

auto config = templateMgr.InstantiateTemplate("BasicShip", params);

// Save instantiated config
std::ofstream file("assets/actors/ships/scout_fighter.json");
file << simplejson::Serialize(config, true);
file.close();
```

#### Template Discovery

```cpp
// Search templates
auto results = templateMgr.SearchTemplates("fighter");

for (const auto& info : results) {
    std::cout << "Template: " << info.name << std::endl;
    std::cout << "  Category: " << info.category << std::endl;
    std::cout << "  Description: " << info.description << std::endl;
}

// Get by category
auto shipTemplates = templateMgr.GetTemplatesByCategory("Ships");

// Get by tag
auto basicTemplates = templateMgr.GetTemplatesByTag("basic");
```

---

## Version Control Integration

### Git Integration for Configuration Changes

The configuration system integrates with version control to track changes, manage versions, and facilitate collaboration.

#### Configuration Versioning

```cpp
#include "engine/config/ConfigManager.h"

using namespace NovaEngine::Config;

auto& versionMgr = ConfigManager::GetInstance().GetVersionManager();

// Register migration
ConfigVersionManager::Migration migration;
migration.fromVersion = ConfigVersion{1, 0, 0};
migration.toVersion = ConfigVersion{2, 0, 0};
migration.description = "Add shield field to ship configs";
migration.transform = [](const simplejson::JsonValue& oldConfig) {
    auto newConfig = oldConfig;
    if (newConfig.IsObject()) {
        auto& obj = newConfig.AsObject();
        if (obj.find("shield") == obj.end()) {
            obj["shield"] = simplejson::JsonValue(500.0);
        }
    }
    return newConfig;
};

versionMgr.RegisterMigration("Spaceship", migration);

// Check if migration needed
if (versionMgr.NeedsMigration(config, "Spaceship")) {
    auto migrated = versionMgr.Migrate(config, "Spaceship", 
                                      ConfigVersion{2, 0, 0});
    // Save migrated config
}
```

#### Version Compatibility

```json
{
    "version": "2.0.0",
    "name": "Player Ship",
    "health": 1000,
    "shield": 500,
    "speed": 150
}
```

---

## Deployment Pipeline

### Safe Configuration Deployment

Deploy configurations across different environments with validation, testing, and rollback capabilities.

#### Deployment Process

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

auto& deployment = ConfigDeployment::GetInstance();

// Configure deployment options
DeploymentOptions options;
options.target = DeploymentTarget::Production;
options.validateBeforeDeploy = true;
options.backupExisting = true;
options.runTests = true;
options.dryRun = false; // Set true for testing

// Deploy single configuration
auto result = deployment.Deploy("assets/actors/ships/player.json", options);

if (result.success) {
    std::cout << "Deployment successful!" << std::endl;
    std::cout << "Deployed files: " << result.deployedFiles.size() << std::endl;
    std::cout << "Backup created: " << result.backupFiles[0] << std::endl;
    std::cout << "Duration: " << result.deploymentDurationMs << "ms" << std::endl;
} else {
    std::cerr << "Deployment failed: " << result.message << std::endl;
    for (const auto& error : result.errors) {
        std::cerr << "  - " << error << std::endl;
    }
}

// Deploy batch
std::vector<std::string> configs = {
    "assets/actors/ships/player.json",
    "assets/actors/ships/fighter.json",
    "assets/actors/ships/cruiser.json"
};

auto batchResult = deployment.DeployBatch(configs, options);
```

#### Pre/Post Deploy Hooks

```cpp
// Set deployment hooks
deployment.SetDeploymentHook(
    // Pre-deploy hook
    [](const std::string& configPath) -> bool {
        std::cout << "Pre-deploy: Validating " << configPath << std::endl;
        // Perform custom validation
        return true; // Return false to abort deployment
    },
    // Post-deploy hook
    [](const DeploymentResult& result) {
        std::cout << "Post-deploy: " << result.message << std::endl;
        // Send notification, update metrics, etc.
    }
);
```

#### Rollback

```cpp
// List available backups
auto backups = deployment.ListBackups();

// Rollback to backup
auto rollbackResult = deployment.Rollback(backups[0]);
```

---

## Automated Testing

### Configuration Test Framework

Ensure configuration quality with automated testing.

#### Creating Test Suites

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

// Create test suite
ConfigTestSuite suite("ShipConfigTests");

// Add tests
suite.AddTest("Valid Health Range", 
              "Health must be between 100 and 10000",
              [](const simplejson::JsonValue& config) {
                  if (!config.IsObject()) return false;
                  auto& obj = config.AsObject();
                  auto it = obj.find("health");
                  if (it == obj.end() || !it->second.IsNumber()) return false;
                  double health = it->second.AsNumber();
                  return health >= 100.0 && health <= 10000.0;
              });

suite.AddTest("Speed Validation",
              "Speed must be positive",
              [](const simplejson::JsonValue& config) {
                  if (!config.IsObject()) return false;
                  auto& obj = config.AsObject();
                  auto it = obj.find("speed");
                  if (it == obj.end() || !it->second.IsNumber()) return false;
                  return it->second.AsNumber() > 0.0;
              });

suite.AddTest("Required Fields",
              "Must have name, health, and speed fields",
              [](const simplejson::JsonValue& config) {
                  if (!config.IsObject()) return false;
                  auto& obj = config.AsObject();
                  return obj.find("name") != obj.end() &&
                         obj.find("health") != obj.end() &&
                         obj.find("speed") != obj.end();
              });
```

#### Running Tests

```cpp
// Register test suite
auto& testRunner = ConfigTestRunner::GetInstance();
testRunner.RegisterSuite("Spaceship", suite);

// Run tests on single config
auto report = testRunner.RunTests("Spaceship", 
                                 "assets/actors/ships/player.json");

std::cout << "Suite: " << report.suiteName << std::endl;
std::cout << "Pass Rate: " << report.GetPassRate() << "%" << std::endl;
std::cout << "Passed: " << report.passedTests << "/" << report.totalTests << std::endl;

for (const auto& result : report.results) {
    if (!result.passed) {
        std::cout << "  FAILED: " << result.testName << std::endl;
        std::cout << "    " << result.message << std::endl;
    }
}

// Run all tests on directory
auto batchReport = testRunner.RunTestsOnDirectory("assets/actors/ships/");

// Export test report
testRunner.ExportReport(batchReport, "test_results.txt");
```

---

## Documentation Generation

### Automatic Configuration Documentation

Generate comprehensive documentation from configuration schemas and metadata.

#### Generating Documentation

```cpp
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

ConfigDocumentation::DocOptions options;
options.format = ConfigDocumentation::DocFormat::Markdown;
options.includeExamples = true;
options.includeSchema = true;
options.includeDefaults = true;
options.includeValidation = true;

// Generate documentation for specific config type
auto doc = ConfigDocumentation::GenerateDocumentation("Spaceship", options);

// Export to file
ConfigDocumentation::ExportDocumentation("Spaceship",
                                        "docs/config_spaceship.md",
                                        options);

// Generate full documentation for all configs
ConfigDocumentation::GenerateFullDocumentation("docs/config/", options);
```

#### Documentation Output Example

Generated Markdown documentation includes:

- Configuration overview
- Field descriptions
- Type information
- Default values
- Validation rules
- Examples
- Schema references

---

## Analytics & Performance

### Track Configuration Usage and Performance

Monitor how configurations are used and identify optimization opportunities.

#### Usage Analytics

```cpp
#include "engine/config/ConfigManager.h"

using namespace NovaEngine::Config;

auto& analytics = ConfigManager::GetInstance().GetAnalytics();

// Get usage statistics
auto stats = analytics.GetStats("assets/actors/ships/player.json");

std::cout << "Load Count: " << stats.loadCount << std::endl;
std::cout << "Average Load Time: " << stats.avgLoadTimeMs << "ms" << std::endl;
std::cout << "Last Used: " 
         << std::chrono::system_clock::to_time_t(stats.lastUsed) << std::endl;

// Find unused configs
auto unused = analytics.FindUnusedConfigs(30); // 30 days
std::cout << "Unused configs: " << unused.size() << std::endl;

// Get most used configs
auto mostUsed = analytics.GetMostUsed(10);
for (const auto& stat : mostUsed) {
    std::cout << stat.configPath << " - " << stat.loadCount 
             << " loads" << std::endl;
}

// Get slowest loading configs
auto slowest = analytics.GetSlowestLoading(10);
for (const auto& stat : slowest) {
    std::cout << stat.configPath << " - " << stat.avgLoadTimeMs 
             << "ms" << std::endl;
}

// Export analytics report
analytics.ExportReport("analytics_report.json");
```

---

## Security Validation

### Configuration Security Features

Validate and secure sensitive configuration data.

#### Security Validation

```cpp
#include "engine/config/ConfigManager.h"

using namespace NovaEngine::Config;

ConfigSecurity::SecurityOptions secOptions;
secOptions.validateSignatures = true;
secOptions.encryptSensitive = true;
secOptions.sanitizeInput = true;
secOptions.encryptionKey = "your-encryption-key";

// Validate security
auto secResult = ConfigSecurity::ValidateSecurity(config, secOptions);

if (!secResult.valid) {
    std::cerr << "Security validation failed!" << std::endl;
}

// Encrypt sensitive fields
std::vector<std::string> sensitiveFields = {"apiKey", "password", "token"};
auto encrypted = ConfigSecurity::EncryptSensitiveFields(config, 
                                                       sensitiveFields,
                                                       secOptions.encryptionKey);

// Decrypt when loading
auto decrypted = ConfigSecurity::DecryptSensitiveFields(encrypted,
                                                       secOptions.encryptionKey);
```

---

## Performance Optimization

### Configuration Caching and Optimization

Optimize configuration loading with intelligent caching strategies.

#### Cache Configuration

```cpp
#include "engine/config/ConfigManager.h"

using namespace NovaEngine::Config;

auto& cache = ConfigManager::GetInstance().GetCache();

// Configure cache policy
cache.SetCachePolicy(CachePolicy::LRU, 100); // 100MB max

// Preload frequently used configs
std::vector<std::string> frequentConfigs = {
    "assets/actors/ships/player.json",
    "assets/actors/ships/fighter.json",
    "assets/actors/world/station.json"
};
cache.Preload(frequentConfigs);

// Get cached config (fast)
auto* cachedConfig = cache.Get("assets/actors/ships/player.json");

// Get cache statistics
auto stats = cache.GetStats();
std::cout << "Cache Entries: " << stats.totalEntries << std::endl;
std::cout << "Memory Usage: " << stats.memoryUsageMB << " MB" << std::endl;
std::cout << "Hit Rate: " << stats.hitRate << "%" << std::endl;
std::cout << "Hits: " << stats.hits << " / Misses: " << stats.misses << std::endl;

// Clear cache if needed
cache.Clear();
```

---

## Complete Example

### Full Configuration Management Workflow

```cpp
#include "engine/config/ConfigManager.h"
#include "engine/config/ConfigEditor.h"

using namespace NovaEngine::Config;

int main() {
    // Initialize configuration system
    auto& configMgr = ConfigManager::GetInstance();
    configMgr.Initialize("assets/");
    
    // Open editor
    ConfigEditor editor;
    if (!editor.OpenConfig("assets/actors/ships/player.json")) {
        std::cerr << "Failed to open config" << std::endl;
        return 1;
    }
    
    // Generate and customize layout
    auto layout = editor.GenerateLayout("PlayerShip");
    editor.SetCustomLayout(layout);
    
    // Set up validation
    editor.SetValidationCallback([](const ValidationResult& result) {
        if (!result.valid) {
            std::cout << "Validation errors: " << result.errors.size() << std::endl;
        }
    });
    
    // Make changes
    editor.SetFieldValue("health", simplejson::JsonValue(1500.0));
    editor.SetFieldValue("shield", simplejson::JsonValue(750.0));
    
    // Validate and save
    auto validation = editor.ValidateAll();
    if (validation.valid) {
        editor.SaveConfig();
        
        // Run tests
        auto& testRunner = ConfigTestRunner::GetInstance();
        auto report = testRunner.RunTests("Spaceship",
                                         "assets/actors/ships/player.json");
        
        if (report.AllPassed()) {
            // Deploy to production
            auto& deployment = ConfigDeployment::GetInstance();
            DeploymentOptions options;
            options.target = DeploymentTarget::Production;
            options.validateBeforeDeploy = true;
            options.backupExisting = true;
            
            auto result = deployment.Deploy("assets/actors/ships/player.json",
                                          options);
            
            if (result.success) {
                std::cout << "Configuration deployed successfully!" << std::endl;
            }
        }
    }
    
    return 0;
}
```

---

## Best Practices

1. **Always validate** configurations before deployment
2. **Use templates** for common configuration patterns
3. **Enable real-time validation** during development
4. **Write automated tests** for critical configurations
5. **Document** configuration schemas and fields
6. **Monitor** configuration usage with analytics
7. **Cache** frequently used configurations
8. **Backup** before deployment
9. **Version** all configuration changes
10. **Secure** sensitive configuration data

---

## API Reference

For detailed API documentation, see:
- `engine/config/ConfigManager.h` - Main configuration system
- `engine/config/ConfigEditor.h` - Visual editor and utilities
- `docs/api/config/` - Full API documentation

---

## Troubleshooting

### Common Issues

**Configuration fails to load:**
- Check file path is correct
- Verify JSON syntax is valid
- Ensure file permissions are correct

**Validation errors:**
- Review error messages for specific issues
- Check schema requirements
- Verify field types match expected values

**Deployment fails:**
- Run validation before deployment
- Check deployment target permissions
- Review pre-deploy hook results

**Performance issues:**
- Enable caching for frequently used configs
- Use preloading for startup configs
- Check cache hit rates

---

## License

Nova Engine Configuration Management System
Copyright (c) 2024 Nova Engine Team
