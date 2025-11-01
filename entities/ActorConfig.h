#pragma once

#include "../engine/SimpleJson.h"
#include <string>
#include <memory>
#include <fstream>
#include <iostream>

/**
 * ActorConfig: JSON-driven configuration system for actors
 * Allows designers to configure actor properties via JSON files
 */
class ActorConfig {
public:
    /**
     * Load actor configuration from JSON file
     */
    static std::unique_ptr<simplejson::JsonObject> LoadFromFile(const std::string& filename);

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
     * Create StationConfig from JSON object
     */
    static StationConfig FromJSON(const simplejson::JsonObject& json);
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