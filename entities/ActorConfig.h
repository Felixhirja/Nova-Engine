#pragma once

#include "../engine/SimpleJson.h"
#include "../engine/JsonSchema.h"
#include <string>
#include <memory>
#include <fstream>
#include <iostream>

/**
 * ActorConfig: JSON-driven configuration system for actors with schema validation
 * Allows designers to configure actor properties via JSON files with validation
 */
class ActorConfig {
public:
    /**
     * Configuration load result with validation info
     */
    struct LoadResult {
        std::unique_ptr<simplejson::JsonObject> config;
        schema::ValidationResult validation;
        bool success = false;
        
        LoadResult() = default;
        LoadResult(std::unique_ptr<simplejson::JsonObject> cfg, schema::ValidationResult val)
            : config(std::move(cfg)), validation(std::move(val)), success(validation.success && config != nullptr) {}
    };
    
    /**
     * Load actor configuration from JSON file (legacy - no validation)
     */
    static std::unique_ptr<simplejson::JsonObject> LoadFromFile(const std::string& filename);
    
    /**
     * Load actor configuration from JSON file with schema validation
     */
    static LoadResult LoadFromFileWithValidation(const std::string& filename, const std::string& schemaId = "actor_config");
    
    /**
     * Validate a configuration object against a schema
     */
    static schema::ValidationResult ValidateConfig(const simplejson::JsonObject& config, const std::string& schemaId);
    
    /**
     * Initialize schema registry with built-in schemas
     */
    static void InitializeSchemas();

    /**
     * Get configuration value with default
     */
    template<typename T>
    static T GetValue(const simplejson::JsonObject& config, const std::string& key, T defaultValue);

    /**
     * Specialized getters for common types
     */
    static std::string GetString(const simplejson::JsonObject& config, const std::string& key, const std::string& defaultValue = "");
    static double GetNumber(const simplejson::JsonObject& config, const std::string& key, double defaultValue = 0.0);
    static bool GetBoolean(const simplejson::JsonObject& config, const std::string& key, bool defaultValue = false);
};

/**
 * StationConfig: Configuration structure for Station actors
 * Provides type-safe access to station properties loaded from JSON
 */
struct StationConfig {
    std::string name;
    double health;
    double shield;
    std::string model;
    int dockingCapacity;
    std::vector<std::string> services;
    std::string behaviorScript;
    std::string type;
    std::string faction;

    /**
     * Create StationConfig from JSON object (legacy - no validation)
     */
    static StationConfig FromJSON(const simplejson::JsonObject& json);
    
    /**
     * Create StationConfig from JSON file with validation
     */
    static StationConfig FromFile(const std::string& filename, bool validateSchema = true);
};

// Template implementations

template<typename T>
T ActorConfig::GetValue(const simplejson::JsonObject& config, const std::string& key, T defaultValue) {
    auto it = config.find(key);
    if (it != config.end()) {
        // This is a simplified implementation - in practice you'd need type-specific conversions
        // For now, just return default
        return defaultValue;
    }
    return defaultValue;
}

// Implementation

inline std::unique_ptr<simplejson::JsonObject> ActorConfig::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto result = simplejson::Parse(content);
    
    if (!result.success) {
        std::cerr << "Failed to parse JSON in file: " << filename << " - " << result.errorMessage << std::endl;
        return nullptr;
    }

    if (!result.value.IsObject()) {
        std::cerr << "JSON root is not an object in file: " << filename << std::endl;
        return nullptr;
    }

    return std::make_unique<simplejson::JsonObject>(result.value.AsObject());
}

inline ActorConfig::LoadResult ActorConfig::LoadFromFileWithValidation(const std::string& filename, const std::string& schemaId) {
    LoadResult result;
    
    // Load JSON file
    std::ifstream file(filename);
    if (!file.is_open()) {
        result.validation.AddError("", "Failed to open config file: " + filename);
        return result;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto parseResult = simplejson::Parse(content);
    
    if (!parseResult.success) {
        result.validation.AddError("", "Failed to parse JSON: " + parseResult.errorMessage);
        return result;
    }

    if (!parseResult.value.IsObject()) {
        result.validation.AddError("", "JSON root is not an object");
        return result;
    }

    result.config = std::make_unique<simplejson::JsonObject>(parseResult.value.AsObject());
    
    // Validate against schema if available
    auto* schema = schema::SchemaRegistry::Instance().GetSchema(schemaId);
    if (schema) {
        result.validation = schema->ValidateObject(*result.config);
        
        if (!result.validation.success) {
            std::cerr << "[ActorConfig] Validation failed for " << filename << ":\n" 
                      << result.validation.GetErrorReport() << std::endl;
        }
    } else {
        std::cout << "[ActorConfig] Warning: Schema '" << schemaId << "' not found, skipping validation for " << filename << std::endl;
        // Still consider it successful if we can't find the schema
        result.validation.success = true;
    }
    
    result.success = result.validation.success && result.config != nullptr;
    return result;
}

inline schema::ValidationResult ActorConfig::ValidateConfig(const simplejson::JsonObject& config, const std::string& schemaId) {
    auto* schema = schema::SchemaRegistry::Instance().GetSchema(schemaId);
    if (!schema) {
        schema::ValidationResult result;
        result.AddError("", "Schema '" + schemaId + "' not found in registry");
        return result;
    }
    
    return schema->ValidateObject(config);
}

inline void ActorConfig::InitializeSchemas() {
    static bool initialized = false;
    if (initialized) return;
    
    auto& registry = schema::SchemaRegistry::Instance();
    
    // Load built-in schemas
    registry.LoadSchemaFromFile("actor_config", "assets/schemas/actor_config.schema.json");
    registry.LoadSchemaFromFile("ship_config", "assets/schemas/ship_config.schema.json");
    registry.LoadSchemaFromFile("station_config", "assets/schemas/station_config.schema.json");
    
    std::cout << "[ActorConfig] Schema registry initialized with schemas: ";
    auto schemaIds = registry.GetSchemaIds();
    for (size_t i = 0; i < schemaIds.size(); ++i) {
        std::cout << schemaIds[i];
        if (i < schemaIds.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    initialized = true;
}

// Specialized implementations

inline std::string ActorConfig::GetString(const simplejson::JsonObject& config, const std::string& key, const std::string& defaultValue) {
    auto it = config.find(key);
    if (it != config.end() && it->second.IsString()) {
        return it->second.AsString(defaultValue);
    }
    return defaultValue;
}

inline double ActorConfig::GetNumber(const simplejson::JsonObject& config, const std::string& key, double defaultValue) {
    auto it = config.find(key);
    if (it != config.end() && it->second.IsNumber()) {
        return it->second.AsNumber(defaultValue);
    }
    return defaultValue;
}

inline bool ActorConfig::GetBoolean(const simplejson::JsonObject& config, const std::string& key, bool defaultValue) {
    auto it = config.find(key);
    if (it != config.end() && it->second.IsBoolean()) {
        return it->second.AsBoolean(defaultValue);
    }
    return defaultValue;
}

// StationConfig implementation

inline StationConfig StationConfig::FromJSON(const simplejson::JsonObject& json) {
    StationConfig config;

    config.name = ActorConfig::GetString(json, "name", "Unnamed Station");
    config.health = ActorConfig::GetNumber(json, "health", 5000.0);
    config.shield = ActorConfig::GetNumber(json, "shield", 2000.0);
    config.model = ActorConfig::GetString(json, "model", "station_large");
    config.dockingCapacity = static_cast<int>(ActorConfig::GetNumber(json, "dockingCapacity", 4.0));
    config.behaviorScript = ActorConfig::GetString(json, "behaviorScript", "");
    config.type = ActorConfig::GetString(json, "type", "trading");
    config.faction = ActorConfig::GetString(json, "faction", "neutral");

    // Load services array
    auto servicesIt = json.find("services");
    if (servicesIt != json.end() && servicesIt->second.IsArray()) {
        auto& servicesArray = servicesIt->second.AsArray();
        config.services.reserve(servicesArray.size());
        for (const auto& service : servicesArray) {
            if (service.IsString()) {
                config.services.push_back(service.AsString());
            }
        }
    }

    return config;
}

inline StationConfig StationConfig::FromFile(const std::string& filename, bool validateSchema) {
    StationConfig config;
    
    if (validateSchema) {
        auto loadResult = ActorConfig::LoadFromFileWithValidation(filename, "station_config");
        if (loadResult.success && loadResult.config) {
            config = FromJSON(*loadResult.config);
        } else {
            std::cerr << "[StationConfig] Failed to load and validate " << filename << std::endl;
            if (!loadResult.validation.success) {
                std::cerr << loadResult.validation.GetErrorReport() << std::endl;
            }
        }
    } else {
        auto configObj = ActorConfig::LoadFromFile(filename);
        if (configObj) {
            config = FromJSON(*configObj);
        }
    }
    
    return config;
}