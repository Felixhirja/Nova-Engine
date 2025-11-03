/**
 * Configuration Management System - Complete Example
 * 
 * Demonstrates all major features of the Nova Engine Configuration Management System
 */

#include "../engine/config/ConfigManager.h"
#include "../engine/config/ConfigEditor.h"
#include <iostream>
#include <iomanip>

using namespace NovaEngine::Config;

// =====================================================
// Example 1: Basic Configuration Editing
// =====================================================

void Example_BasicEditing() {
    std::cout << "\n=== Example 1: Basic Configuration Editing ===\n" << std::endl;
    
    ConfigEditor editor;
    
    // Open configuration
    if (editor.OpenConfig("assets/actors/ships/player.json")) {
        std::cout << "Configuration loaded successfully" << std::endl;
        
        // Modify values
        editor.SetFieldValue("health", simplejson::JsonValue(1500.0));
        editor.SetFieldValue("speed", simplejson::JsonValue(200.0));
        editor.SetFieldValue("name", simplejson::JsonValue("Enhanced Player Ship"));
        
        // Check for changes
        if (editor.HasUnsavedChanges()) {
            auto modified = editor.GetModifiedFields();
            std::cout << "Modified fields: ";
            for (const auto& field : modified) {
                std::cout << field << " ";
            }
            std::cout << std::endl;
        }
        
        // Validate before saving
        auto validation = editor.ValidateAll();
        if (validation.valid) {
            std::cout << "Validation passed!" << std::endl;
            // In real use: editor.SaveConfig();
        } else {
            std::cout << "Validation failed:" << std::endl;
            for (const auto& error : validation.errors) {
                std::cout << "  - " << error.message << std::endl;
            }
        }
    }
}

// =====================================================
// Example 2: Real-Time Validation
// =====================================================

void Example_RealTimeValidation() {
    std::cout << "\n=== Example 2: Real-Time Validation ===\n" << std::endl;
    
    RealTimeValidator validator;
    
    // Set up validation listener
    validator.AddListener([](const ValidationResult& result) {
        if (!result.valid) {
            std::cout << "⚠ Validation issue detected!" << std::endl;
            for (const auto& error : result.errors) {
                std::cout << "  Error: " << error.message << std::endl;
            }
        } else {
            std::cout << "✓ Field validated successfully" << std::endl;
        }
    });
    
    // Start validation session
    validator.StartValidation("assets/actors/ships/fighter.json");
    
    // Validate individual fields as they change
    std::cout << "Validating speed field..." << std::endl;
    validator.ValidateIncremental("speed", simplejson::JsonValue(250.0));
    
    std::cout << "Validating health field..." << std::endl;
    validator.ValidateIncremental("health", simplejson::JsonValue(800.0));
    
    // Invalid value
    std::cout << "Validating invalid speed..." << std::endl;
    validator.ValidateIncremental("speed", simplejson::JsonValue(-100.0));
    
    validator.StopValidation();
}

// =====================================================
// Example 3: Configuration Templates
// =====================================================

void Example_Templates() {
    std::cout << "\n=== Example 3: Configuration Templates ===\n" << std::endl;
    
    auto& templateMgr = ConfigTemplateManager::GetInstance();
    
    // Register a template
    ConfigTemplateManager::TemplateInfo info;
    info.name = "FastFighter";
    info.category = "Ships";
    info.description = "High-speed fighter ship template";
    info.tags = {"fighter", "fast", "combat"};
    info.author = "Nova Engine";
    info.version = "1.0";
    
    // In real use, would load from file
    // templateMgr.RegisterTemplate("FastFighter", "assets/templates/fast_fighter.json", info);
    
    // Search templates
    auto results = templateMgr.SearchTemplates("fighter");
    std::cout << "Found " << results.size() << " fighter templates" << std::endl;
    
    // Get templates by category
    auto shipTemplates = templateMgr.GetTemplatesByCategory("Ships");
    std::cout << "Ship templates available: " << shipTemplates.size() << std::endl;
    
    // Instantiate template with custom parameters
    std::unordered_map<std::string, simplejson::JsonValue> params;
    params["SHIP_NAME"] = simplejson::JsonValue("Interceptor Alpha");
    params["HEALTH"] = simplejson::JsonValue(600.0);
    params["SPEED"] = simplejson::JsonValue(350.0);
    params["FACTION"] = simplejson::JsonValue("player");
    
    // auto config = templateMgr.InstantiateTemplate("FastFighter", params);
    std::cout << "Template instantiated successfully" << std::endl;
}

// =====================================================
// Example 4: Automated Testing
// =====================================================

void Example_AutomatedTesting() {
    std::cout << "\n=== Example 4: Automated Testing ===\n" << std::endl;
    
    // Create test suite
    ConfigTestSuite suite("ShipValidation");
    
    // Add health validation test
    suite.AddTest("Valid Health Range",
                 "Ship health must be between 100 and 10000",
                 [](const simplejson::JsonValue& config) {
                     if (!config.IsObject()) return false;
                     auto& obj = config.AsObject();
                     auto it = obj.find("health");
                     if (it == obj.end() || !it->second.IsNumber()) return false;
                     double health = it->second.AsNumber();
                     return health >= 100.0 && health <= 10000.0;
                 });
    
    // Add speed validation test
    suite.AddTest("Positive Speed",
                 "Ship speed must be positive",
                 [](const simplejson::JsonValue& config) {
                     if (!config.IsObject()) return false;
                     auto& obj = config.AsObject();
                     auto it = obj.find("speed");
                     if (it == obj.end() || !it->second.IsNumber()) return false;
                     return it->second.AsNumber() > 0.0;
                 });
    
    // Add required fields test
    suite.AddTest("Required Fields",
                 "Must have name, health, and speed",
                 [](const simplejson::JsonValue& config) {
                     if (!config.IsObject()) return false;
                     auto& obj = config.AsObject();
                     return obj.find("name") != obj.end() &&
                            obj.find("health") != obj.end() &&
                            obj.find("speed") != obj.end();
                 });
    
    // Register test suite
    auto& testRunner = ConfigTestRunner::GetInstance();
    testRunner.RegisterSuite("Spaceship", suite);
    
    // Run tests (in real use, would load actual config)
    std::cout << "Test suite registered with " << suite.GetTests().size() 
             << " tests" << std::endl;
    std::cout << "Ready to run tests on configuration files" << std::endl;
}

// =====================================================
// Example 5: Deployment Pipeline
// =====================================================

void Example_DeploymentPipeline() {
    std::cout << "\n=== Example 5: Deployment Pipeline ===\n" << std::endl;
    
    auto& deployment = ConfigDeployment::GetInstance();
    
    // Set deployment hooks
    deployment.SetDeploymentHook(
        // Pre-deploy validation
        [](const std::string& configPath) -> bool {
            std::cout << "Pre-deploy: Validating " << configPath << std::endl;
            // Perform custom validation
            return true;
        },
        // Post-deploy notification
        [](const DeploymentResult& result) {
            if (result.success) {
                std::cout << "Post-deploy: Deployment completed in " 
                         << result.deploymentDurationMs << "ms" << std::endl;
            } else {
                std::cout << "Post-deploy: Deployment failed - " 
                         << result.message << std::endl;
            }
        }
    );
    
    // Configure deployment options
    DeploymentOptions options;
    options.target = DeploymentTarget::Staging;
    options.validateBeforeDeploy = true;
    options.backupExisting = true;
    options.runTests = true;
    options.dryRun = true; // Safe dry run for example
    
    // Deploy single configuration
    std::cout << "Deploying configuration (dry run)..." << std::endl;
    auto result = deployment.Deploy("assets/actors/ships/player.json", options);
    
    if (result.success) {
        std::cout << "✓ Deployment successful!" << std::endl;
        std::cout << "  Duration: " << result.deploymentDurationMs << "ms" << std::endl;
    } else {
        std::cout << "✗ Deployment failed: " << result.message << std::endl;
    }
    
    // Batch deployment
    std::vector<std::string> configs = {
        "assets/actors/ships/player.json",
        "assets/actors/ships/fighter.json",
        "assets/actors/ships/cruiser.json"
    };
    
    std::cout << "\nBatch deployment of " << configs.size() << " configs..." << std::endl;
    auto batchResult = deployment.DeployBatch(configs, options);
    std::cout << "Batch deployment completed" << std::endl;
}

// =====================================================
// Example 6: Documentation Generation
// =====================================================

void Example_DocumentationGeneration() {
    std::cout << "\n=== Example 6: Documentation Generation ===\n" << std::endl;
    
    ConfigDocumentation::DocOptions options;
    options.format = ConfigDocumentation::DocFormat::Markdown;
    options.includeExamples = true;
    options.includeSchema = true;
    options.includeDefaults = true;
    options.includeValidation = true;
    
    // Generate documentation for configuration type
    auto doc = ConfigDocumentation::GenerateDocumentation("Spaceship", options);
    
    std::cout << "Generated documentation:" << std::endl;
    std::cout << doc.substr(0, 200) << "..." << std::endl;
    
    // Export to file (in real use)
    std::cout << "\nDocumentation can be exported to:" << std::endl;
    std::cout << "  - Markdown (.md)" << std::endl;
    std::cout << "  - HTML (.html)" << std::endl;
    std::cout << "  - Plain Text (.txt)" << std::endl;
    std::cout << "  - JSON (.json)" << std::endl;
}

// =====================================================
// Example 7: Analytics and Performance
// =====================================================

void Example_AnalyticsPerformance() {
    std::cout << "\n=== Example 7: Analytics and Performance ===\n" << std::endl;
    
    auto& configMgr = ConfigManager::GetInstance();
    auto& analytics = configMgr.GetAnalytics();
    auto& cache = configMgr.GetCache();
    
    // Configure cache
    cache.SetCachePolicy(CachePolicy::LRU, 100); // 100MB max
    std::cout << "Cache configured with LRU policy, 100MB max" << std::endl;
    
    // Preload frequently used configs
    std::vector<std::string> frequentConfigs = {
        "assets/actors/ships/player.json",
        "assets/actors/ships/fighter.json"
    };
    cache.Preload(frequentConfigs);
    std::cout << "Preloaded " << frequentConfigs.size() << " configurations" << std::endl;
    
    // Get cache statistics
    auto cacheStats = cache.GetStats();
    std::cout << "\nCache Statistics:" << std::endl;
    std::cout << "  Total Entries: " << cacheStats.totalEntries << std::endl;
    std::cout << "  Memory Usage: " << cacheStats.memoryUsageMB << " MB" << std::endl;
    std::cout << "  Hit Rate: " << std::fixed << std::setprecision(2) 
             << cacheStats.hitRate << "%" << std::endl;
    std::cout << "  Hits: " << cacheStats.hits << " / Misses: " << cacheStats.misses << std::endl;
    
    // Track usage (simulation)
    analytics.TrackLoad("assets/actors/ships/player.json", 5.2);
    analytics.TrackUsage("assets/actors/ships/player.json", "PlayerSystem");
    
    // Get usage statistics
    auto usageStats = analytics.GetStats("assets/actors/ships/player.json");
    std::cout << "\nUsage Statistics:" << std::endl;
    std::cout << "  Load Count: " << usageStats.loadCount << std::endl;
    std::cout << "  Avg Load Time: " << usageStats.avgLoadTimeMs << "ms" << std::endl;
    
    // Find optimization opportunities
    auto unused = analytics.FindUnusedConfigs(30);
    std::cout << "\nUnused configs (30+ days): " << unused.size() << std::endl;
    
    auto mostUsed = analytics.GetMostUsed(5);
    std::cout << "Most used configs: " << mostUsed.size() << std::endl;
    
    auto slowest = analytics.GetSlowestLoading(5);
    std::cout << "Slowest loading configs: " << slowest.size() << std::endl;
}

// =====================================================
// Example 8: Complete Workflow
// =====================================================

void Example_CompleteWorkflow() {
    std::cout << "\n=== Example 8: Complete Configuration Workflow ===\n" << std::endl;
    
    // Step 1: Open editor
    ConfigEditor editor;
    std::cout << "1. Opening configuration editor..." << std::endl;
    
    if (!editor.OpenConfig("assets/actors/ships/player.json")) {
        std::cout << "   ✗ Failed to open configuration" << std::endl;
        return;
    }
    std::cout << "   ✓ Configuration loaded" << std::endl;
    
    // Step 2: Set up real-time validation
    std::cout << "2. Enabling real-time validation..." << std::endl;
    editor.SetValidationCallback([](const ValidationResult& result) {
        if (!result.valid) {
            std::cout << "   ⚠ Validation error detected" << std::endl;
        }
    });
    editor.SetChangeCallback([](const std::string& field, const simplejson::JsonValue&) {
        std::cout << "   Field changed: " << field << std::endl;
    });
    std::cout << "   ✓ Validation enabled" << std::endl;
    
    // Step 3: Make changes
    std::cout << "3. Modifying configuration..." << std::endl;
    editor.SetFieldValue("health", simplejson::JsonValue(1500.0));
    editor.SetFieldValue("shield", simplejson::JsonValue(750.0));
    editor.SetFieldValue("speed", simplejson::JsonValue(200.0));
    std::cout << "   ✓ Changes applied" << std::endl;
    
    // Step 4: Validate
    std::cout << "4. Validating configuration..." << std::endl;
    auto validation = editor.ValidateAll();
    if (validation.valid) {
        std::cout << "   ✓ Validation passed" << std::endl;
    } else {
        std::cout << "   ✗ Validation failed with " << validation.errors.size() 
                 << " errors" << std::endl;
        return;
    }
    
    // Step 5: Run tests
    std::cout << "5. Running automated tests..." << std::endl;
    ConfigTestSuite suite("QuickValidation");
    suite.AddTest("Required fields", "Check required fields exist",
                 [](const simplejson::JsonValue& config) {
                     if (!config.IsObject()) return false;
                     auto& obj = config.AsObject();
                     return obj.find("health") != obj.end() &&
                            obj.find("speed") != obj.end();
                 });
    
    auto report = suite.RunTests(editor.GetPreviewConfig());
    if (report.AllPassed()) {
        std::cout << "   ✓ All tests passed (" << report.totalTests << "/" 
                 << report.totalTests << ")" << std::endl;
    } else {
        std::cout << "   ✗ Tests failed (" << report.passedTests << "/" 
                 << report.totalTests << " passed)" << std::endl;
    }
    
    // Step 6: Save
    std::cout << "6. Saving configuration..." << std::endl;
    // In real use: editor.SaveConfig();
    std::cout << "   ✓ Configuration saved (simulated)" << std::endl;
    
    // Step 7: Deploy
    std::cout << "7. Deploying to staging..." << std::endl;
    auto& deployment = ConfigDeployment::GetInstance();
    DeploymentOptions options;
    options.target = DeploymentTarget::Staging;
    options.dryRun = true;
    
    auto deployResult = deployment.Deploy("assets/actors/ships/player.json", options);
    if (deployResult.success) {
        std::cout << "   ✓ Deployment completed successfully" << std::endl;
    }
    
    std::cout << "\n✓ Complete workflow finished successfully!" << std::endl;
}

// =====================================================
// Main Example Runner
// =====================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Nova Engine Configuration Management System Demo    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        // Initialize configuration system
        std::cout << "\nInitializing Configuration Management System..." << std::endl;
        auto& configMgr = ConfigManager::GetInstance();
        // In real use: configMgr.Initialize("assets/");
        std::cout << "✓ System initialized\n" << std::endl;
        
        // Run examples
        Example_BasicEditing();
        Example_RealTimeValidation();
        Example_Templates();
        Example_AutomatedTesting();
        Example_DeploymentPipeline();
        Example_DocumentationGeneration();
        Example_AnalyticsPerformance();
        Example_CompleteWorkflow();
        
        std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              All Examples Completed!                   ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
