#pragma once

#include "EntityFactory.h"
#include "ecs/EntityManager.h"
#include <string>
#include <vector>
#include <memory>

/**
 * EntitySpawnManager: Automatic entity spawning from JSON configurations
 * 
 * Features:
 * - Data-driven entity spawning (no engine code changes)
 * - JSON-configured spawn lists
 * - Conditional spawning support
 * - Automatic startup spawning
 */
class EntitySpawnManager {
public:
    /**
     * Spawn configuration for individual entities
     */
    struct SpawnConfig {
        std::string id;
        std::string type;
        
        struct Position {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
        } position;
        
        struct Rotation {
            double yaw = 0.0;
            double pitch = 0.0;
            double roll = 0.0;
        } rotation;
        
        std::string spawnCondition = "always";
        bool enabled = true;
    };
    
    /**
     * World spawn configuration
     */
    struct WorldConfig {
        std::string worldName;
        std::string version;
        bool autoSpawnEnabled = true;
        bool spawnOnStartup = true;
        double spawnRadius = 1000.0;
        std::vector<SpawnConfig> entities;
    };

    /**
     * Create spawn manager with entity factory
     */
    explicit EntitySpawnManager(EntityFactory& factory) : factory_(factory) {}

    /**
     * Load spawn configuration from JSON file
     */
    bool LoadSpawnConfig(const std::string& configPath);
    
    /**
     * Spawn all entities marked for startup spawning
     */
    void SpawnStartupEntities();
    
    /**
     * Spawn specific entity by ID
     */
    bool SpawnEntity(const std::string& entityId);
    
    /**
     * Spawn all entities matching condition
     */
    void SpawnConditionalEntities(const std::string& condition);
    
    /**
     * Get loaded world configuration
     */
    const WorldConfig& GetWorldConfig() const { return worldConfig_; }
    
    /**
     * Check if spawn config is loaded
     */
    bool IsConfigLoaded() const { return configLoaded_; }

private:
    EntityFactory& factory_;
    WorldConfig worldConfig_;
    bool configLoaded_ = false;
    
    /**
     * Parse JSON spawn configuration
     */
    bool ParseSpawnConfig(const std::string& jsonContent);
};