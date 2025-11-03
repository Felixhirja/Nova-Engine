#pragma once

#include "SimpleJson.h"
#include "PlayerConfig.h"
#include "../entities/ActorConfig.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>

/**
 * EntityConfigManager: Auto-loading configuration system for all entities
 * 
 * Features:
 * - Auto-discovery of config files in assets/actors/ and assets/config/
 * - Cached loading with hot-reload support
 * - Type-safe configuration access
 * - Designer-friendly error reporting
 * - Centralized entity creation with configs
 */
class EntityConfigManager {
public:
    /**
     * Configuration types supported by the system
     */
    enum class ConfigType {
        Player,     // assets/config/player_config.json
        Actor,      // assets/actors/*.json (NPC, Station, Projectile, etc.)
        Spaceship,  // assets/actors/spaceship.json variants
        Unknown
    };

    /**
     * Configuration metadata for tracking loaded configs
     */
    struct ConfigInfo {
        std::string filePath;
        ConfigType type;
        std::string entityType;  // "player", "npc", "station", etc.
        std::filesystem::file_time_type lastModified;
        bool loaded;
        std::string errorMessage;
    };

    /**
     * Singleton access for global configuration management
     */
    static EntityConfigManager& GetInstance();

    /**
     * Initialize and auto-discover all configuration files
     * Should be called during engine bootstrap
     */
    bool Initialize();

    /**
     * Reload all configurations (useful for development)
     */
    void ReloadAll();

    /**
     * Check for modified files and reload them
     */
    void CheckForHotReload();

    /**
     * Get available entity types that can be created
     */
    std::vector<std::string> GetAvailableEntityTypes() const;

    /**
     * Get configuration info for debugging/tools
     */
    std::vector<ConfigInfo> GetConfigInfo() const;

    /**
     * Type-safe configuration access
     */
    PlayerConfig GetPlayerConfig();
    std::unique_ptr<simplejson::JsonObject> GetActorConfig(const std::string& actorType);
    StationConfig GetStationConfig();

    /**
     * Entity creation with automatic config application
     */
    template<typename ActorType>
    std::unique_ptr<ActorType> CreateConfiguredActor(const std::string& configName = "");

    /**
     * Check if a configuration exists for an entity type
     */
    bool HasConfig(const std::string& entityType) const;

    /**
     * Get the file path for a configuration
     */
    std::string GetConfigPath(const std::string& entityType) const;

    /**
     * Designer-friendly config validation
     */
    bool ValidateConfig(const std::string& entityType, std::vector<std::string>& errors) const;

private:
    EntityConfigManager() = default;
    ~EntityConfigManager() = default;

    // Prevent copying
    EntityConfigManager(const EntityConfigManager&) = delete;
    EntityConfigManager& operator=(const EntityConfigManager&) = delete;

    /**
     * Internal methods for auto-discovery and loading
     */
    void DiscoverConfigurations();
    bool LoadConfiguration(const std::string& filePath, const std::string& entityType);
    ConfigType DetermineConfigType(const std::string& filePath) const;
    std::string ExtractEntityType(const std::string& filePath) const;

    /**
     * Configuration storage
     */
    std::unordered_map<std::string, ConfigInfo> configRegistry_;
    std::unordered_map<std::string, std::unique_ptr<simplejson::JsonObject>> actorConfigs_;
    std::unique_ptr<PlayerConfig> playerConfig_;
    std::unique_ptr<StationConfig> stationConfig_;

    bool initialized_ = false;
    
    // Paths for configuration discovery
    const std::vector<std::string> configPaths_ = {
        "assets/config/",
        "assets/actors/"
    };
};

// Template implementation for actor creation
template<typename ActorType>
std::unique_ptr<ActorType> EntityConfigManager::CreateConfiguredActor(const std::string& configName) {
    auto actor = std::make_unique<ActorType>();
    
    // The actor's Initialize() method will automatically load its config
    // This template provides a convenient factory method
    
    return actor;
}