#include "../entities/ActorConfig.h"
#include "../engine/JsonSchema.h"
#include <iostream>
#include <cassert>
#include <sstream>

// Test helper macros
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "❌ ASSERTION FAILED: " << message << std::endl; \
        return false; \
    }

#define ASSERT_FALSE(condition, message) \
    if (condition) { \
        std::cerr << "❌ ASSERTION FAILED: " << message << std::endl; \
        return false; \
    }

#define ASSERT_EQ(expected, actual, message) \
    if ((expected) != (actual)) { \
        std::cerr << "❌ ASSERTION FAILED: " << message << " (expected: " << expected << ", got: " << actual << ")" << std::endl; \
        return false; \
    }

class SchemaValidationTestSuite {
private:
    int testsPassed = 0;
    int testsTotal = 0;

public:
    bool RunTest(const std::string& testName, bool (*testFunction)()) {
        testsTotal++;
        std::cout << "Running: " << testName << "..." << std::endl;
        
        if (testFunction()) {
            std::cout << "✅ PASS: " << testName << std::endl;
            testsPassed++;
            return true;
        } else {
            std::cout << "❌ FAIL: " << testName << std::endl;
            return false;
        }
    }
    
    void PrintSummary() {
        std::cout << "\n=== Test Suite Results ===" << std::endl;
        std::cout << "Passed: " << testsPassed << "/" << testsTotal << " tests" << std::endl;
        
        if (testsPassed == testsTotal) {
            std::cout << "🎉 All tests passed!" << std::endl;
        } else {
            std::cout << "⚠️  " << (testsTotal - testsPassed) << " test(s) failed." << std::endl;
        }
    }
    
    int GetExitCode() const { return testsPassed == testsTotal ? 0 : 1; }
};

// Test Cases

bool TestValidationResultBasics() {
    schema::ValidationResult result;
    
    // Test initial state
    ASSERT_TRUE(result.success, "ValidationResult should start as success");
    ASSERT_EQ(0, result.errors.size(), "ValidationResult should start with no errors");
    
    // Test adding error
    result.AddError("/test", "Test error", "test_rule", "Test suggestion");
    ASSERT_FALSE(result.success, "ValidationResult should be false after adding error");
    ASSERT_EQ(1, result.errors.size(), "ValidationResult should have 1 error");
    
    // Test error details
    const auto& error = result.errors[0];
    ASSERT_EQ("/test", error.path, "Error path should match");
    ASSERT_EQ("Test error", error.message, "Error message should match");
    ASSERT_EQ("test_rule", error.schemaRule, "Error rule should match");
    ASSERT_EQ("Test suggestion", error.suggestion, "Error suggestion should match");
    
    return true;
}

bool TestValidationErrorReporting() {
    schema::ValidationResult result;
    
    // Add multiple errors
    result.AddError("/field1", "Missing required field", "required", "Add the missing field");
    result.AddError("/field2", "Invalid type", "type", "Change to correct type");
    
    std::string report = result.GetErrorReport();
    
    // Check report contains expected elements
    ASSERT_TRUE(report.find("Validation failed with 2 error(s)") != std::string::npos, 
                "Report should contain error count");
    ASSERT_TRUE(report.find("/field1") != std::string::npos, 
                "Report should contain first error path");
    ASSERT_TRUE(report.find("Missing required field") != std::string::npos, 
                "Report should contain first error message");
    ASSERT_TRUE(report.find("💡 Suggestion: Add the missing field") != std::string::npos, 
                "Report should contain first error suggestion");
    
    return true;
}

bool TestSchemaPropertyTypeValidation() {
    using namespace schema;
    
    // Create a simple string property schema
    SchemaProperty stringProp(SchemaProperty::Type::String);
    JsonSchema schema;
    schema.AddProperty("name", stringProp);
    schema.AddRequired("name");
    
    // Test valid string
    auto validJson = simplejson::Parse(R"({"name": "Test"})");
    ASSERT_TRUE(validJson.success, "Valid JSON should parse");
    
    auto result = schema.ValidateObject(validJson.value.AsObject());
    ASSERT_TRUE(result.success, "Valid string should pass validation");
    
    // Test invalid type (number instead of string)
    auto invalidJson = simplejson::Parse(R"({"name": 123})");
    ASSERT_TRUE(invalidJson.success, "Invalid JSON should still parse");
    
    auto invalidResult = schema.ValidateObject(invalidJson.value.AsObject());
    ASSERT_FALSE(invalidResult.success, "Number should fail string validation");
    ASSERT_EQ(1, invalidResult.errors.size(), "Should have one type error");
    
    const auto& error = invalidResult.errors[0];
    ASSERT_EQ("type", error.schemaRule, "Should be a type validation error");
    
    return true;
}

bool TestRequiredPropertiesValidation() {
    using namespace schema;
    
    // Create schema with required properties
    JsonSchema schema;
    SchemaProperty stringProp(SchemaProperty::Type::String);
    schema.AddProperty("name", stringProp);
    schema.AddProperty("description", stringProp);
    schema.AddRequired("name");
    schema.AddRequired("description");
    
    // Test missing required property
    auto incompleteJson = simplejson::Parse(R"({"name": "Test"})");
    ASSERT_TRUE(incompleteJson.success, "JSON should parse");
    
    auto result = schema.ValidateObject(incompleteJson.value.AsObject());
    ASSERT_FALSE(result.success, "Missing required property should fail");
    ASSERT_TRUE(result.errors.size() > 0, "Should have at least one error");
    
    // Check for required property error
    bool foundRequiredError = false;
    for (const auto& error : result.errors) {
        if (error.schemaRule == "required" && error.path.find("description") != std::string::npos) {
            foundRequiredError = true;
            ASSERT_TRUE(error.suggestion.find("Add") != std::string::npos, 
                        "Required error should suggest adding property");
            break;
        }
    }
    ASSERT_TRUE(foundRequiredError, "Should find required property error for 'description'");
    
    return true;
}

bool TestAdditionalPropertiesValidation() {
    using namespace schema;
    
    // Create strict schema (no additional properties)
    JsonSchema schema;
    SchemaProperty stringProp(SchemaProperty::Type::String);
    schema.AddProperty("name", stringProp);
    schema.SetAdditionalProperties(false);
    
    // Test additional property
    auto jsonWithExtra = simplejson::Parse(R"({"name": "Test", "extra": "value"})");
    ASSERT_TRUE(jsonWithExtra.success, "JSON should parse");
    
    auto result = schema.ValidateObject(jsonWithExtra.value.AsObject());
    ASSERT_FALSE(result.success, "Additional property should fail validation");
    
    // Check for additional property error
    bool foundAdditionalError = false;
    for (const auto& error : result.errors) {
        if (error.schemaRule == "additionalProperties") {
            foundAdditionalError = true;
            ASSERT_TRUE(error.suggestion.find("Remove") != std::string::npos, 
                        "Additional property error should suggest removal");
            break;
        }
    }
    ASSERT_TRUE(foundAdditionalError, "Should find additional property error");
    
    return true;
}

bool TestActorConfigValidationIntegration() {
    // Initialize schemas
    ActorConfig::InitializeSchemas();
    auto& registry = schema::SchemaRegistry::Instance();
    registry.LoadSchemaFromFile("simple_station_config", "assets/schemas/simple_station_config.schema.json");
    
    // Test valid config
    auto validResult = ActorConfig::LoadFromFileWithValidation(
        "assets/actors/examples/trading_station_example.json", "simple_station_config");
    ASSERT_TRUE(validResult.success, "Valid actor config should pass validation");
    ASSERT_TRUE(validResult.config != nullptr, "Valid config should load successfully");
    
    // Test invalid config (this one has validation errors)
    auto invalidResult = ActorConfig::LoadFromFileWithValidation(
        "assets/actors/test_invalid_station.json", "simple_station_config");
    ASSERT_FALSE(invalidResult.success, "Invalid actor config should fail validation");
    ASSERT_TRUE(invalidResult.validation.errors.size() > 0, "Invalid config should have errors");
    
    return true;
}

bool TestNonexistentFileHandling() {
    auto result = ActorConfig::LoadFromFileWithValidation(
        "assets/actors/definitely_does_not_exist.json", "simple_station_config");
    
    ASSERT_FALSE(result.success, "Nonexistent file should fail");
    ASSERT_TRUE(result.validation.errors.size() > 0, "Should have file error");
    
    const auto& error = result.validation.errors[0];
    ASSERT_TRUE(error.message.find("Failed to open") != std::string::npos, 
                "Should have file open error message");
    
    return true;
}

bool TestSchemaNotFoundHandling() {
    auto result = ActorConfig::LoadFromFileWithValidation(
        "assets/actors/examples/trading_station_example.json", "nonexistent_schema");
    
    // Should still succeed but with warning (graceful degradation)
    ASSERT_TRUE(result.success, "Should succeed when schema not found (graceful degradation)");
    
    return true;
}

int main() {
    std::cout << "=== Schema Validation Unit Test Suite ===" << std::endl;
    
    SchemaValidationTestSuite suite;
    
    // Run all tests
    suite.RunTest("ValidationResult Basics", TestValidationResultBasics);
    suite.RunTest("Validation Error Reporting", TestValidationErrorReporting);
    suite.RunTest("Schema Property Type Validation", TestSchemaPropertyTypeValidation);
    suite.RunTest("Required Properties Validation", TestRequiredPropertiesValidation);
    suite.RunTest("Additional Properties Validation", TestAdditionalPropertiesValidation);
    suite.RunTest("ActorConfig Validation Integration", TestActorConfigValidationIntegration);
    suite.RunTest("Nonexistent File Handling", TestNonexistentFileHandling);
    suite.RunTest("Schema Not Found Handling", TestSchemaNotFoundHandling);
    
    suite.PrintSummary();
    return suite.GetExitCode();
}