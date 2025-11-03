/**
 * Configuration System Integration Test
 * 
 * Demonstrates and validates all configuration architecture features:
 * 1. Schema definition and registration
 * 2. Configuration loading and validation
 * 3. Type-safe value access
 * 4. Migration system
 * 5. Analytics tracking
 * 6. Testing framework
 */

#include "engine/ConfigSystem.h"
#include "engine/ConfigIntegration.h"
#include <iostream>
#include <cassert>

using namespace nova::config;

// ============================================================================
// Test Utilities
// ============================================================================

void AssertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "ASSERTION FAILED: " << message << "\n";
        std::exit(1);
    }
}

void TestHeader(const std::string& name) {
    std::cout << "\n========================================\n";
    std::cout << "TEST: " << name << "\n";
    std::cout << "========================================\n";
}

// ============================================================================
// Test 1: Schema Definition and Registration
// ============================================================================

void Test_SchemaDefinitionAndRegistration() {
    TestHeader("Schema Definition and Registration");
    
    // Create a custom schema
    ConfigSchema schema("TestConfig", "1.0");
    schema.AddDescription("Test configuration for validation");
    
    // Add various field types
    ConfigSchema::Field intField;
    intField.name = "count";
    intField.type = ConfigValueType::Integer;
    intField.required = true;
    intField.minValue = 1.0;
    intField.maxValue = 100.0;
    schema.AddField(intField);
    
    ConfigSchema::Field floatField;
    floatField.name = "speed";
    floatField.type = ConfigValueType::Float;
    floatField.minValue = 0.0;
    floatField.maxValue = 10.0;
    floatField.defaultValue = 5.0;
    schema.AddField(floatField);
    
    ConfigSchema::Field stringField;
    stringField.name = "name";
    stringField.type = ConfigValueType::String;
    stringField.minLength = 1;
    stringField.maxLength = 50;
    schema.AddField(stringField);
    
    // Register schema
    auto& registry = ConfigSchemaRegistry::GetInstance();
    bool registered = registry.RegisterSchema("TestConfig", schema);
    AssertTrue(registered, "Schema registration failed");
    
    // Verify registration
    AssertTrue(registry.HasSchema("TestConfig"), "Schema not found after registration");
    
    const ConfigSchema* retrieved = registry.GetSchema("TestConfig");
    AssertTrue(retrieved != nullptr, "Retrieved schema is null");
    AssertTrue(retrieved->GetName() == "TestConfig", "Schema name mismatch");
    AssertTrue(retrieved->GetFields().size() == 3, "Field count mismatch");
    
    std::cout << "✓ Schema registration: SUCCESS\n";
    std::cout << "  - Registered: TestConfig v" << retrieved->GetVersion() << "\n";
    std::cout << "  - Fields: " << retrieved->GetFields().size() << "\n";
}

// ============================================================================
// Test 2: Configuration Value Types
// ============================================================================

void Test_ConfigurationValueTypes() {
    TestHeader("Configuration Value Types");
    
    // Boolean
    ConfigValue boolVal(true);
    AssertTrue(boolVal.GetType() == ConfigValueType::Boolean, "Boolean type mismatch");
    AssertTrue(boolVal.AsBool() == true, "Boolean value mismatch");
    
    // Integer
    ConfigValue intVal(42);
    AssertTrue(intVal.GetType() == ConfigValueType::Integer, "Integer type mismatch");
    AssertTrue(intVal.AsInt() == 42, "Integer value mismatch");
    
    // Float
    ConfigValue floatVal(3.14);
    AssertTrue(floatVal.GetType() == ConfigValueType::Float, "Float type mismatch");
    AssertTrue(floatVal.AsFloat() > 3.13 && floatVal.AsFloat() < 3.15, "Float value mismatch");
    
    // String
    ConfigValue stringVal(std::string("Hello"));
    AssertTrue(stringVal.GetType() == ConfigValueType::String, "String type mismatch");
    AssertTrue(stringVal.AsString() == "Hello", "String value mismatch");
    
    // Type conversion
    AssertTrue(intVal.AsFloat() == 42.0, "Int to float conversion failed");
    AssertTrue(floatVal.AsInt() == 3, "Float to int conversion failed");
    
    std::cout << "✓ Value types: SUCCESS\n";
    std::cout << "  - Boolean: " << boolVal.AsBool() << "\n";
    std::cout << "  - Integer: " << intVal.AsInt() << "\n";
    std::cout << "  - Float: " << floatVal.AsFloat() << "\n";
    std::cout << "  - String: " << stringVal.AsString() << "\n";
}

// ============================================================================
// Test 3: Validation Engine
// ============================================================================

void Test_ValidationEngine() {
    TestHeader("Validation Engine");
    
    // Create test schema
    ConfigSchema schema("ValidationTest", "1.0");
    
    ConfigSchema::Field requiredField;
    requiredField.name = "required_value";
    requiredField.type = ConfigValueType::Integer;
    requiredField.required = true;
    schema.AddField(requiredField);
    
    ConfigSchema::Field rangeField;
    rangeField.name = "speed";
    rangeField.type = ConfigValueType::Float;
    rangeField.minValue = 0.0;
    rangeField.maxValue = 10.0;
    schema.AddField(rangeField);
    
    ConfigValidator validator(schema);
    
    // Test 1: Valid configuration
    simplejson::JsonObject validConfig;
    validConfig["required_value"] = simplejson::JsonValue(5.0);
    validConfig["speed"] = simplejson::JsonValue(5.0);
    
    ValidationResult result1 = validator.Validate(validConfig);
    AssertTrue(result1.isValid, "Valid config marked as invalid");
    std::cout << "✓ Valid config validation: PASSED\n";
    
    // Test 2: Missing required field
    simplejson::JsonObject missingRequired;
    missingRequired["speed"] = simplejson::JsonValue(5.0);
    
    ValidationResult result2 = validator.Validate(missingRequired);
    AssertTrue(!result2.isValid, "Missing required field not detected");
    AssertTrue(!result2.errors.empty(), "No error message for missing field");
    std::cout << "✓ Missing required field: DETECTED\n";
    std::cout << "  - Error: " << result2.errors[0] << "\n";
    
    // Test 3: Out of range value
    simplejson::JsonObject outOfRange;
    outOfRange["required_value"] = simplejson::JsonValue(5.0);
    outOfRange["speed"] = simplejson::JsonValue(15.0);  // Exceeds max
    
    ValidationResult result3 = validator.Validate(outOfRange);
    AssertTrue(!result3.isValid, "Out of range value not detected");
    std::cout << "✓ Range validation: DETECTED\n";
    std::cout << "  - Error: " << result3.errors[0] << "\n";
}

// ============================================================================
// Test 4: Configuration Loading and Access
// ============================================================================

void Test_ConfigurationLoadingAndAccess() {
    TestHeader("Configuration Loading and Access");
    
    // Create test configuration
    ConfigSchema schema("AccessTest", "1.0");
    schema.AddField("maxSpeed", ConfigValueType::Float, false);
    schema.AddField("playerName", ConfigValueType::String, false);
    schema.AddField("debugMode", ConfigValueType::Boolean, false);
    
    ConfigSchemaRegistry::GetInstance().RegisterSchema("AccessTest", schema);
    
    Configuration config("AccessTest", &schema);
    
    // Create test JSON
    simplejson::JsonObject json;
    json["maxSpeed"] = simplejson::JsonValue(10.5);
    json["playerName"] = simplejson::JsonValue(std::string("TestPlayer"));
    json["debugMode"] = simplejson::JsonValue(true);
    
    config.LoadFromJson(json);
    
    // Test type-safe access
    double speed = config.Get<double>("maxSpeed", 0.0);
    AssertTrue(speed > 10.4 && speed < 10.6, "Float access failed");
    
    std::string name = config.Get<std::string>("playerName", "");
    AssertTrue(name == "TestPlayer", "String access failed");
    
    bool debug = config.Get<bool>("debugMode", false);
    AssertTrue(debug == true, "Boolean access failed");
    
    // Test default values
    int missing = config.Get<int>("nonexistent", 999);
    AssertTrue(missing == 999, "Default value not returned");
    
    std::cout << "✓ Configuration access: SUCCESS\n";
    std::cout << "  - maxSpeed: " << speed << "\n";
    std::cout << "  - playerName: " << name << "\n";
    std::cout << "  - debugMode: " << debug << "\n";
    std::cout << "  - default value: " << missing << "\n";
}

// ============================================================================
// Test 5: Migration System
// ============================================================================

void Test_MigrationSystem() {
    TestHeader("Migration System");
    
    ConfigMigration migration("1.0", "2.0");
    
    migration.AddStep("Add new field", [](simplejson::JsonObject& config) {
        config["newField"] = simplejson::JsonValue(42.0);
        return true;
    });
    
    migration.AddStep("Rename field", [](simplejson::JsonObject& config) {
        if (config.find("oldName") != config.end()) {
            config["newName"] = config["oldName"];
            config.erase("oldName");
        }
        return true;
    });
    
    // Test migration
    simplejson::JsonObject config;
    config["oldName"] = simplejson::JsonValue(std::string("value"));
    
    bool success = migration.Migrate(config);
    AssertTrue(success, "Migration failed");
    AssertTrue(config.find("newField") != config.end(), "New field not added");
    AssertTrue(config.find("newName") != config.end(), "Field not renamed");
    AssertTrue(config.find("oldName") == config.end(), "Old field not removed");
    
    std::cout << "✓ Migration: SUCCESS\n";
    std::cout << "  - Version: " << migration.GetFromVersion() 
              << " -> " << migration.GetToVersion() << "\n";
    std::cout << "  - Steps executed: 2\n";
}

// ============================================================================
// Test 6: Analytics System
// ============================================================================

void Test_AnalyticsSystem() {
    TestHeader("Analytics System");
    
    auto& analytics = ConfigAnalytics::GetInstance();
    analytics.Reset();
    
    const std::string configName = "TestAnalytics";
    
    // Record some operations
    analytics.RecordLoad(configName, std::chrono::milliseconds(5));
    analytics.RecordLoad(configName, std::chrono::milliseconds(3));
    analytics.RecordAccess(configName, "field1");
    analytics.RecordAccess(configName, "field1");
    analytics.RecordAccess(configName, "field2");
    analytics.RecordValidationFailure(configName);
    
    // Check stats
    auto stats = analytics.GetStats(configName);
    AssertTrue(stats.loadCount == 2, "Load count incorrect");
    AssertTrue(stats.accessCount == 3, "Access count incorrect");
    AssertTrue(stats.validationFailures == 1, "Validation failure count incorrect");
    AssertTrue(stats.averageLoadTime.count() == 4, "Average load time incorrect");
    AssertTrue(stats.fieldAccessCounts["field1"] == 2, "Field access count incorrect");
    
    std::cout << "✓ Analytics: SUCCESS\n";
    std::cout << "  - Loads: " << stats.loadCount << "\n";
    std::cout << "  - Accesses: " << stats.accessCount << "\n";
    std::cout << "  - Avg load time: " << stats.averageLoadTime.count() << "ms\n";
    std::cout << "  - Validation failures: " << stats.validationFailures << "\n";
}

// ============================================================================
// Test 7: Built-in Schema Integration
// ============================================================================

void Test_BuiltInSchemaIntegration() {
    TestHeader("Built-in Schema Integration");
    
    // Register built-in schemas
    ConfigIntegration::RegisterBuiltInSchemas();
    
    auto& registry = ConfigSchemaRegistry::GetInstance();
    
    // Verify built-in schemas are registered
    AssertTrue(registry.HasSchema("PlayerConfig"), "PlayerConfig not registered");
    AssertTrue(registry.HasSchema("BootstrapConfiguration"), "Bootstrap not registered");
    AssertTrue(registry.HasSchema("ActorConfig"), "ActorConfig not registered");
    
    // Get schema info
    auto schemaInfo = registry.GetSchemaInfo();
    std::cout << "✓ Built-in schemas registered: " << schemaInfo.size() << "\n";
    
    for (const auto& info : schemaInfo) {
        std::cout << "  - " << info.typeName 
                  << " (v" << info.version << ", " 
                  << info.fieldCount << " fields)\n";
    }
}

// ============================================================================
// Test 8: Configuration Testing Framework
// ============================================================================

void Test_ConfigurationTestingFramework() {
    TestHeader("Configuration Testing Framework");
    
    // Create test schema
    ConfigSchema schema("TestFrameworkConfig", "1.0");
    schema.AddField("value", ConfigValueType::Integer, true);
    ConfigSchemaRegistry::GetInstance().RegisterSchema("TestFrameworkConfig", schema);
    
    // Create test suite
    ConfigTestSuite suite("TestFrameworkConfig");
    
    // Add test cases
    suite.AddTestCase({
        "valid_config",
        R"({"value": 42})",
        true,
        {}
    });
    
    suite.AddTestCase({
        "missing_required",
        R"({"other": 123})",
        false,
        {"Required field 'value' is missing"}
    });
    
    // Run tests
    bool allPassed = suite.RunTests(false);
    AssertTrue(allPassed, "Some tests failed");
    
    std::cout << suite.GetReport();
    std::cout << "✓ Test framework: SUCCESS\n";
}

// ============================================================================
// Test 9: Documentation Generation
// ============================================================================

void Test_DocumentationGeneration() {
    TestHeader("Documentation Generation");
    
    // Generate documentation for all registered schemas
    std::string docs = ConfigIntegration::GenerateDocumentation();
    AssertTrue(!docs.empty(), "Documentation generation failed");
    AssertTrue(docs.find("PlayerConfig") != std::string::npos, 
               "PlayerConfig not in documentation");
    
    std::cout << "✓ Documentation generation: SUCCESS\n";
    std::cout << "  - Generated " << docs.length() << " characters\n";
    
    // Export JSON Schema
    std::string jsonSchema = ConfigIntegration::ExportSchemaAsJsonSchema("PlayerConfig");
    AssertTrue(!jsonSchema.empty(), "JSON Schema export failed");
    AssertTrue(jsonSchema.find("\"type\"") != std::string::npos, 
               "Invalid JSON Schema format");
    
    std::cout << "✓ JSON Schema export: SUCCESS\n";
}

// ============================================================================
// Test 10: Unified Config System
// ============================================================================

void Test_UnifiedConfigSystem() {
    TestHeader("Unified Config System");
    
    auto& system = ConfigSystem::GetInstance();
    
    // Initialize
    bool initialized = system.Initialize();
    AssertTrue(initialized, "System initialization failed");
    
    // Register a test schema
    ConfigSchema schema("SystemTest", "1.0");
    schema.AddField("testValue", ConfigValueType::Integer, true);
    system.RegisterSchema("SystemTest", schema);
    
    // Verify schema registration
    const ConfigSchema* retrieved = system.GetSchema("SystemTest");
    AssertTrue(retrieved != nullptr, "Schema not registered in system");
    
    // Test hot reload control
    system.EnableHotReload(true);
    system.CheckHotReload();  // Should not crash
    system.EnableHotReload(false);
    
    std::cout << "✓ Unified system: SUCCESS\n";
    std::cout << "  - Initialization: OK\n";
    std::cout << "  - Schema registration: OK\n";
    std::cout << "  - Hot reload: OK\n";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "=========================================\n";
    std::cout << "Configuration System Integration Tests\n";
    std::cout << "=========================================\n";
    
    try {
        Test_SchemaDefinitionAndRegistration();
        Test_ConfigurationValueTypes();
        Test_ValidationEngine();
        Test_ConfigurationLoadingAndAccess();
        Test_MigrationSystem();
        Test_AnalyticsSystem();
        Test_BuiltInSchemaIntegration();
        Test_ConfigurationTestingFramework();
        Test_DocumentationGeneration();
        Test_UnifiedConfigSystem();
        
        std::cout << "\n=========================================\n";
        std::cout << "ALL TESTS PASSED ✓\n";
        std::cout << "=========================================\n";
        std::cout << "\nConfiguration Architecture is ready for production use.\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED WITH EXCEPTION:\n";
        std::cerr << e.what() << "\n";
        return 1;
    }
}
