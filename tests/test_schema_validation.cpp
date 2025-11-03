#include "../entities/ActorConfig.h"
#include <iostream>

int main() {
    std::cout << "=== Schema Validation Test ===" << std::endl;
    
    // Initialize schemas
    ActorConfig::InitializeSchemas();
    
    // Also load our simple schema for testing
    auto& registry = schema::SchemaRegistry::Instance();
    registry.LoadSchemaFromFile("simple_station_config", "assets/schemas/simple_station_config.schema.json");
    
    std::cout << "Schema registry initialized" << std::endl;
    
    // Test 1: Valid station config
    std::cout << "\nTest 1: Loading valid station config..." << std::endl;
    auto validResult = ActorConfig::LoadFromFileWithValidation("assets/actors/examples/trading_station_example.json", "simple_station_config");
    if (validResult.success) {
        // Get name from config if it exists
        std::string configName = "unknown";
        if (validResult.config && validResult.config->find("name") != validResult.config->end()) {
            configName = validResult.config->at("name").AsString();
        }
        std::cout << "✅ Valid config loaded successfully: " << configName << std::endl;
    } else {
        std::cout << "❌ Valid config failed" << std::endl;
        if (!validResult.validation.success) {
            std::cout << "Validation details: " << validResult.validation.GetErrorReport() << std::endl;
        }
    }
    
    // Test 2: Invalid station config (missing required fields)
    std::cout << "\nTest 2: Loading invalid station config..." << std::endl;
    auto invalidResult = ActorConfig::LoadFromFileWithValidation("assets/actors/test_invalid_station.json", "simple_station_config");
    if (invalidResult.success) {
        std::cout << "❌ Invalid config incorrectly passed validation!" << std::endl;
        std::cout << "Validation success: " << invalidResult.validation.success << std::endl;
        std::cout << "Config loaded: " << (invalidResult.config ? "yes" : "no") << std::endl;
    } else {
        std::cout << "✅ Invalid config correctly rejected" << std::endl;
        std::cout << "Error details: " << invalidResult.validation.GetErrorReport() << std::endl;
    }
    
    // Test 3: Nonexistent file
    std::cout << "\nTest 3: Loading nonexistent config..." << std::endl;
    auto missingResult = ActorConfig::LoadFromFileWithValidation("assets/actors/nonexistent.json", "simple_station_config");
    if (missingResult.success) {
        std::cout << "❌ Nonexistent file incorrectly passed!" << std::endl;
    } else {
        std::cout << "✅ Nonexistent file correctly handled" << std::endl;
        std::cout << "Error details: " << missingResult.validation.GetErrorReport() << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}