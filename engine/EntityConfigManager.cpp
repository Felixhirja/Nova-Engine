#include "EntityConfigManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>

EntityConfigManager& EntityConfigManager::GetInstance() {
    static EntityConfigManager instance;
    return instance;
}

bool EntityConfigManager::Initialize() {
    std::cout << "[EntityConfigManager] Initializing auto-loading configuration system..." << std::endl;
    
    // Clear any existing configurations
    configRegistry_.clear();
    actorConfigs_.clear();
    playerConfig_.reset();
    stationConfig_.reset();
    
    // Discover all configuration files
    DiscoverConfigurations();
    
    // Load discovered configurations
    int loadedCount = 0;
    int errorCount = 0;
    
    for (auto& [entityType, configInfo] : configRegistry_) {
        if (LoadConfiguration(configInfo.filePath, entityType)) {
            configInfo.loaded = true;
            loadedCount++;
        } else {
            configInfo.loaded = false;
            errorCount++;
        }
    }
    
    initialized_ = true;
    
    std::cout << "[EntityConfigManager] Initialization complete:" << std::endl;
    std::cout << "  - Loaded: " << loadedCount << " configurations" << std::endl;
    std::cout << "  - Errors: " << errorCount << " configurations" << std::endl;
    std::cout << "  - Available entity types: ";
    
    auto types = GetAvailableEntityTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        std::cout << types[i];
        if (i < types.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    return errorCount == 0;
}

void EntityConfigManager::DiscoverConfigurations() {
    std::cout << "[EntityConfigManager] Discovering configuration files..." << std::endl;
    
    for (const std::string& basePath : configPaths_) {
        if (!std::filesystem::exists(basePath)) {
            std::cout << "  - Path not found: " << basePath << std::endl;
            continue;
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
                if (!entry.is_regular_file()) continue;
                
                const std::string filePath = entry.path().string();
                const std::string extension = entry.path().extension().string();
                
                // Only process JSON files
                if (extension != ".json") continue;
                
                // Skip non-entity configuration files
                std::string filename = entry.path().filename().string();
                if (filename == "viewport_layouts.json" || 
                    filename.find("viewport_layouts") != std::string::npos ||
                    filename.find("hud_config.json") != std::string::npos ||
                    filename.find("bootstrap.json") != std::string::npos) {
                    continue;  // Skip viewport/UI/bootstrap configs
                }
                
                std::string entityType = ExtractEntityType(filePath);
                ConfigType configType = DetermineConfigType(filePath);
                
                // Create configuration info
                ConfigInfo info;
                info.filePath = filePath;
                info.type = configType;
                info.entityType = entityType;
                info.lastModified = entry.last_write_time();
                info.loaded = false;
                info.errorMessage = "";
                
                configRegistry_[entityType] = info;
                std::cout << "  - Found: " << entityType << " (" << filePath << ")" << std::endl;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[EntityConfigManager] Error scanning " << basePath << ": " << e.what() << std::endl;
        }
    }
}

EntityConfigManager::ConfigType EntityConfigManager::DetermineConfigType(const std::string& filePath) const {
    // Skip viewport layout configurations - these are not entity configs
    if (filePath.find("viewport_layouts.json") != std::string::npos) {
        return ConfigType::Unknown;  // Will be skipped
    }
    
    if (filePath.find("player_config.json") != std::string::npos) {
        return ConfigType::Player;
    } else if (filePath.find("assets/actors/") != std::string::npos) {
        return ConfigType::Actor;
    } else if (filePath.find("spaceship") != std::string::npos) {
        return ConfigType::Spaceship;
    }
    return ConfigType::Unknown;
}

std::string EntityConfigManager::ExtractEntityType(const std::string& filePath) const {
    std::filesystem::path path(filePath);
    std::string filename = path.stem().string();  // filename without extension
    
    // Handle special cases
    if (filename == "player_config") {
        return "player";
    }
    
    // For most actor configs, the filename is the entity type
    return filename;
}

bool EntityConfigManager::LoadConfiguration(const std::string& filePath, const std::string& entityType) {
    try {
        auto& configInfo = configRegistry_[entityType];
        
        if (configInfo.type == ConfigType::Player) {
            // Load PlayerConfig
            auto config = PlayerConfig::LoadFromFile(filePath);
            if (!config.spawnPosition.x && !config.spawnPosition.y && !config.spawnPosition.z && 
                !config.movement.forwardSpeed) {
                // Check if loading failed (all zeros might indicate default fallback)
                configInfo.errorMessage = "Failed to load player configuration - all values are zero";
                return false;
            }
            playerConfig_ = std::make_unique<PlayerConfig>(config);
            std::cout << "  - Loaded PlayerConfig from " << filePath << std::endl;
            
        } else if (configInfo.type == ConfigType::Actor) {
            // Load generic actor config using ActorConfig system
            auto config = ActorConfig::LoadFromFile(filePath);
            if (!config) {
                configInfo.errorMessage = "Failed to parse JSON or file not found";
                return false;
            }
            
            actorConfigs_[entityType] = std::move(config);
            std::cout << "  - Loaded ActorConfig for " << entityType << " from " << filePath << std::endl;
            
            // Special handling for station configs
            if (entityType == "station") {
                stationConfig_ = std::make_unique<StationConfig>(StationConfig::FromJSON(*actorConfigs_[entityType]));
            }
            
        } else {
            configInfo.errorMessage = "Unknown configuration type";
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        configRegistry_[entityType].errorMessage = std::string("Exception: ") + e.what();
        std::cerr << "[EntityConfigManager] Error loading " << entityType << " config: " << e.what() << std::endl;
        return false;
    }
}

void EntityConfigManager::ReloadAll() {
    std::cout << "[EntityConfigManager] Reloading all configurations..." << std::endl;
    Initialize();
}

void EntityConfigManager::CheckForHotReload() {
    if (!initialized_) return;
    
    bool needsReload = false;
    
    for (auto& [entityType, configInfo] : configRegistry_) {
        try {
            if (std::filesystem::exists(configInfo.filePath)) {
                auto currentModTime = std::filesystem::last_write_time(configInfo.filePath);
                if (currentModTime != configInfo.lastModified) {
                    std::cout << "[EntityConfigManager] Detected change in " << entityType << " config, reloading..." << std::endl;
                    configInfo.lastModified = currentModTime;
                    needsReload = true;
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[EntityConfigManager] Error checking file time for " << entityType << ": " << e.what() << std::endl;
        }
    }
    
    if (needsReload) {
        ReloadAll();
    }
}

std::vector<std::string> EntityConfigManager::GetAvailableEntityTypes() const {
    std::vector<std::string> types;
    for (const auto& [entityType, configInfo] : configRegistry_) {
        if (configInfo.loaded) {
            types.push_back(entityType);
        }
    }
    std::sort(types.begin(), types.end());
    return types;
}

std::vector<EntityConfigManager::ConfigInfo> EntityConfigManager::GetConfigInfo() const {
    std::vector<ConfigInfo> info;
    for (const auto& [entityType, configInfo] : configRegistry_) {
        info.push_back(configInfo);
    }
    return info;
}

PlayerConfig EntityConfigManager::GetPlayerConfig() {
    if (!playerConfig_) {
        std::cerr << "[EntityConfigManager] Player config not loaded, returning default" << std::endl;
        return PlayerConfig::LoadFromFile("assets/config/engine/player_config.json");
    }
    return *playerConfig_;
}

std::unique_ptr<simplejson::JsonObject> EntityConfigManager::GetActorConfig(const std::string& actorType) {
    auto it = actorConfigs_.find(actorType);
    if (it != actorConfigs_.end()) {
        // Return a copy of the config
        return std::make_unique<simplejson::JsonObject>(*it->second);
    }
    
    std::cerr << "[EntityConfigManager] Actor config not found for type: " << actorType << std::endl;
    return nullptr;
}

StationConfig EntityConfigManager::GetStationConfig() {
    if (!stationConfig_) {
        std::cerr << "[EntityConfigManager] Station config not loaded, attempting to load..." << std::endl;
        
        // Try to load station config directly
        auto config = ActorConfig::LoadFromFile("assets/actors/world/station.json");
        if (config) {
            stationConfig_ = std::make_unique<StationConfig>(StationConfig::FromJSON(*config));
            return *stationConfig_;
        }
        
        // Return default if loading fails
        StationConfig defaultConfig;
        defaultConfig.name = "Default Station";
        defaultConfig.health = 5000.0;
        defaultConfig.shield = 2000.0;
        defaultConfig.model = "station_large";
        defaultConfig.dockingCapacity = 4;
        defaultConfig.faction = "neutral";
        defaultConfig.type = "trading";
        return defaultConfig;
    }
    return *stationConfig_;
}

bool EntityConfigManager::HasConfig(const std::string& entityType) const {
    auto it = configRegistry_.find(entityType);
    return it != configRegistry_.end() && it->second.loaded;
}

std::string EntityConfigManager::GetConfigPath(const std::string& entityType) const {
    auto it = configRegistry_.find(entityType);
    if (it != configRegistry_.end()) {
        return it->second.filePath;
    }
    return "";
}

bool EntityConfigManager::ValidateConfig(const std::string& entityType, std::vector<std::string>& errors) const {
    errors.clear();
    
    auto it = configRegistry_.find(entityType);
    if (it == configRegistry_.end()) {
        errors.push_back("Configuration not found for entity type: " + entityType);
        return false;
    }
    
    const auto& configInfo = it->second;
    
    if (!configInfo.loaded) {
        errors.push_back("Configuration failed to load: " + configInfo.errorMessage);
        return false;
    }
    
    if (!std::filesystem::exists(configInfo.filePath)) {
        errors.push_back("Configuration file does not exist: " + configInfo.filePath);
        return false;
    }
    
    // Type-specific validation
    if (configInfo.type == ConfigType::Player && !playerConfig_) {
        errors.push_back("Player configuration object is null despite being marked as loaded");
        return false;
    }
    
    if (configInfo.type == ConfigType::Actor) {
        auto actorIt = actorConfigs_.find(entityType);
        if (actorIt == actorConfigs_.end()) {
            errors.push_back("Actor configuration object not found in cache");
            return false;
        }
    }
    
    return errors.empty();
}