#include "EntitySpawnManager.h"
#include "SimpleJson.h"
#include <fstream>
#include <iostream>

bool EntitySpawnManager::LoadSpawnConfig(const std::string& configPath) {
    std::cout << "[EntitySpawnManager] Loading spawn configuration from: " << configPath << std::endl;
    
    // Read JSON file
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "[EntitySpawnManager] ERROR: Could not open spawn config: " << configPath << std::endl;
        return false;
    }
    
    std::string jsonContent((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();
    
    if (!ParseSpawnConfig(jsonContent)) {
        std::cerr << "[EntitySpawnManager] ERROR: Failed to parse spawn config" << std::endl;
        return false;
    }
    
    configLoaded_ = true;
    std::cout << "[EntitySpawnManager] Successfully loaded spawn config: " << worldConfig_.worldName 
              << " with " << worldConfig_.entities.size() << " entities" << std::endl;
    
    return true;
}

void EntitySpawnManager::SpawnStartupEntities() {
    if (!configLoaded_) {
        std::cerr << "[EntitySpawnManager] ERROR: No spawn config loaded" << std::endl;
        return;
    }
    
    if (!worldConfig_.spawnOnStartup) {
        std::cout << "[EntitySpawnManager] Startup spawning disabled in config" << std::endl;
        return;
    }
    
    std::cout << "[EntitySpawnManager] Spawning startup entities..." << std::endl;
    
    int spawnedCount = 0;
    for (const auto& entityConfig : worldConfig_.entities) {
        if (entityConfig.enabled && entityConfig.spawnCondition == "always") {
            if (SpawnEntity(entityConfig.id)) {
                spawnedCount++;
            }
        }
    }
    
    std::cout << "[EntitySpawnManager] Spawned " << spawnedCount << " startup entities" << std::endl;
}

bool EntitySpawnManager::SpawnEntity(const std::string& entityId) {
    // Find entity config by ID
    const SpawnConfig* config = nullptr;
    for (const auto& entityConfig : worldConfig_.entities) {
        if (entityConfig.id == entityId) {
            config = &entityConfig;
            break;
        }
    }
    
    if (!config) {
        std::cerr << "[EntitySpawnManager] ERROR: Entity ID not found: " << entityId << std::endl;
        return false;
    }
    
    if (!config->enabled) {
        std::cout << "[EntitySpawnManager] Entity " << entityId << " is disabled, skipping" << std::endl;
        return false;
    }
    
    std::cout << "[EntitySpawnManager] Spawning entity: " << entityId << " (type: " << config->type << ")" << std::endl;
    
    // Use EntityFactory to spawn the entity
    auto result = factory_.CreateSpaceship(config->type, 
                                         config->position.x, 
                                         config->position.y, 
                                         config->position.z);
    
    if (result.success) {
        std::cout << "[EntitySpawnManager] Successfully spawned " << entityId 
                  << " at (" << config->position.x << ", " << config->position.y << ", " << config->position.z 
                  << ") - Entity ID: " << result.entity << std::endl;
        return true;
    } else {
        std::cerr << "[EntitySpawnManager] ERROR: Failed to spawn " << entityId << ": " << result.errorMessage << std::endl;
        return false;
    }
}

void EntitySpawnManager::SpawnConditionalEntities(const std::string& condition) {
    std::cout << "[EntitySpawnManager] Spawning entities with condition: " << condition << std::endl;
    
    int spawnedCount = 0;
    for (const auto& entityConfig : worldConfig_.entities) {
        if (entityConfig.enabled && entityConfig.spawnCondition == condition) {
            if (SpawnEntity(entityConfig.id)) {
                spawnedCount++;
            }
        }
    }
    
    std::cout << "[EntitySpawnManager] Spawned " << spawnedCount << " entities with condition: " << condition << std::endl;
}

bool EntitySpawnManager::ParseSpawnConfig(const std::string& jsonContent) {
    auto parseResult = simplejson::Parse(jsonContent);
    if (!parseResult.success) {
        std::cerr << "[EntitySpawnManager] JSON parse error: " << parseResult.errorMessage << std::endl;
        return false;
    }
    
    const auto& root = parseResult.value;
    if (!root.IsObject()) {
        std::cerr << "[EntitySpawnManager] Root is not a JSON object" << std::endl;
        return false;
    }
    
    const auto& rootObj = root.AsObject();
    
    // Parse world config
    auto worldNameIt = rootObj.find("world_name");
    worldConfig_.worldName = (worldNameIt != rootObj.end() && worldNameIt->second.IsString()) 
                           ? worldNameIt->second.AsString() : "Unknown World";
    
    auto versionIt = rootObj.find("version");
    worldConfig_.version = (versionIt != rootObj.end() && versionIt->second.IsString()) 
                         ? versionIt->second.AsString() : "1.0";
    
    // Parse spawn config section
    auto spawnConfigIt = rootObj.find("spawn_config");
    if (spawnConfigIt != rootObj.end() && spawnConfigIt->second.IsObject()) {
        const auto& spawnConfigObj = spawnConfigIt->second.AsObject();
        
        auto autoSpawnIt = spawnConfigObj.find("auto_spawn_enabled");
        worldConfig_.autoSpawnEnabled = (autoSpawnIt != spawnConfigObj.end() && autoSpawnIt->second.IsBoolean()) 
                                      ? autoSpawnIt->second.AsBoolean() : true;
        
        auto spawnOnStartupIt = spawnConfigObj.find("spawn_on_startup");
        worldConfig_.spawnOnStartup = (spawnOnStartupIt != spawnConfigObj.end() && spawnOnStartupIt->second.IsBoolean()) 
                                    ? spawnOnStartupIt->second.AsBoolean() : true;
        
        auto spawnRadiusIt = spawnConfigObj.find("spawn_radius");
        worldConfig_.spawnRadius = (spawnRadiusIt != spawnConfigObj.end() && spawnRadiusIt->second.IsNumber()) 
                                 ? spawnRadiusIt->second.AsNumber() : 1000.0;
    }
    
    // Parse entities array
    auto entitiesIt = rootObj.find("entities");
    if (entitiesIt != rootObj.end() && entitiesIt->second.IsArray()) {
        const auto& entitiesArray = entitiesIt->second.AsArray();
        
        for (const auto& entityValue : entitiesArray) {
            if (!entityValue.IsObject()) continue;
            
            const auto& entityObj = entityValue.AsObject();
            SpawnConfig config;
            
            auto idIt = entityObj.find("id");
            config.id = (idIt != entityObj.end() && idIt->second.IsString()) 
                      ? idIt->second.AsString() : "unnamed";
            
            auto typeIt = entityObj.find("type");
            config.type = (typeIt != entityObj.end() && typeIt->second.IsString()) 
                        ? typeIt->second.AsString() : "spaceship";
            
            auto conditionIt = entityObj.find("spawn_condition");
            config.spawnCondition = (conditionIt != entityObj.end() && conditionIt->second.IsString()) 
                                  ? conditionIt->second.AsString() : "always";
            
            auto enabledIt = entityObj.find("enabled");
            config.enabled = (enabledIt != entityObj.end() && enabledIt->second.IsBoolean()) 
                           ? enabledIt->second.AsBoolean() : true;
            
            // Parse position
            auto positionIt = entityObj.find("position");
            if (positionIt != entityObj.end() && positionIt->second.IsObject()) {
                const auto& posObj = positionIt->second.AsObject();
                
                auto xIt = posObj.find("x");
                config.position.x = (xIt != posObj.end() && xIt->second.IsNumber()) 
                                  ? xIt->second.AsNumber() : 0.0;
                
                auto yIt = posObj.find("y");
                config.position.y = (yIt != posObj.end() && yIt->second.IsNumber()) 
                                  ? yIt->second.AsNumber() : 0.0;
                
                auto zIt = posObj.find("z");
                config.position.z = (zIt != posObj.end() && zIt->second.IsNumber()) 
                                  ? zIt->second.AsNumber() : 0.0;
            }
            
            // Parse rotation (optional)
            auto rotationIt = entityObj.find("rotation");
            if (rotationIt != entityObj.end() && rotationIt->second.IsObject()) {
                const auto& rotObj = rotationIt->second.AsObject();
                
                auto yawIt = rotObj.find("yaw");
                config.rotation.yaw = (yawIt != rotObj.end() && yawIt->second.IsNumber()) 
                                    ? yawIt->second.AsNumber() : 0.0;
                
                auto pitchIt = rotObj.find("pitch");
                config.rotation.pitch = (pitchIt != rotObj.end() && pitchIt->second.IsNumber()) 
                                      ? pitchIt->second.AsNumber() : 0.0;
                
                auto rollIt = rotObj.find("roll");
                config.rotation.roll = (rollIt != rotObj.end() && rollIt->second.IsNumber()) 
                                     ? rollIt->second.AsNumber() : 0.0;
            }
            
            worldConfig_.entities.push_back(config);
        }
    }
    
    return true;
}