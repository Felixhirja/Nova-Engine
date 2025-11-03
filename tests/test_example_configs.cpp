#include "../entities/ActorConfig.h"
#include <iostream>
#include <vector>
#include <string>

struct ExampleConfig {
    std::string filename;
    std::string expectedName;
    std::string description;
};

int main() {
    std::cout << "=== Example Actor Configs Validation Test ===" << std::endl;
    
    // Initialize schemas
    ActorConfig::InitializeSchemas();
    
    // Also load our simple schema for testing
    auto& registry = schema::SchemaRegistry::Instance();
    registry.LoadSchemaFromFile("simple_station_config", "assets/schemas/simple_station_config.schema.json");
    
    std::cout << "Schema registry initialized" << std::endl;
    
    // Define all example configs to test
    std::vector<ExampleConfig> examples = {
        {"assets/actors/examples/trading_station_example.json", "Aurora Trading Station", "Commercial trading hub"},
        {"assets/actors/examples/military_station_example.json", "Fortress Omega", "Military defensive outpost"},
        {"assets/actors/examples/research_station_example.json", "Research Outpost Kepler", "Scientific research facility"},
        {"assets/actors/examples/mining_station_example.json", "Mining Platform Delta-7", "Asteroid mining operation"}
    };
    
    int passed = 0;
    int total = examples.size();
    
    std::cout << "\nValidating " << total << " example configurations...\n" << std::endl;
    
    for (const auto& example : examples) {
        std::cout << "Testing: " << example.filename << std::endl;
        std::cout << "Description: " << example.description << std::endl;
        
        auto result = ActorConfig::LoadFromFileWithValidation(example.filename, "simple_station_config");
        
        if (result.success) {
            // Get name from config
            std::string configName = "unknown";
            if (result.config && result.config->find("name") != result.config->end()) {
                configName = result.config->at("name").AsString();
            }
            
            if (configName == example.expectedName) {
                std::cout << "✅ PASS: Config loaded successfully as '" << configName << "'" << std::endl;
                passed++;
            } else {
                std::cout << "❌ FAIL: Expected name '" << example.expectedName << "' but got '" << configName << "'" << std::endl;
            }
        } else {
            std::cout << "❌ FAIL: Validation failed" << std::endl;
            std::cout << "Error details: " << result.validation.GetErrorReport() << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Summary
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << " examples" << std::endl;
    
    if (passed == total) {
        std::cout << "🎉 All example configurations are valid!" << std::endl;
        return 0;
    } else {
        std::cout << "⚠️  Some example configurations failed validation." << std::endl;
        return 1;
    }
}