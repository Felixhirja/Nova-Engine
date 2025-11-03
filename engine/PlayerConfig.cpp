#include "PlayerConfig.h"
#include "ecs/Components.h"
#include "SimpleJson.h"
#include <fstream>
#include <iostream>

PlayerConfig PlayerConfig::LoadFromFile(const std::string& filePath) {
    PlayerConfig config = GetDefault();
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "[PlayerConfig] Could not open " << filePath << ", using defaults" << std::endl;
        return config;
    }
    
    try {
        // Read file content
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Parse JSON using simplejson
        auto result = simplejson::Parse(content);
        if (!result.success) {
            std::cout << "[PlayerConfig] Failed to parse " << filePath << ": " << result.errorMessage << ", using defaults" << std::endl;
            return config;
        }
        
        auto& json = result.value;
        
        if (!json.IsObject()) {
            std::cout << "[PlayerConfig] JSON root is not an object in " << filePath << ", using defaults" << std::endl;
            return config;
        }
        
        auto& jsonObj = json.AsObject();
        
        // Load spawn position
        if (auto playerIt = jsonObj.find("player"); playerIt != jsonObj.end() && playerIt->second.IsObject()) {
            auto& player = playerIt->second.AsObject();
            if (auto spawnIt = player.find("spawn"); spawnIt != player.end() && spawnIt->second.IsObject()) {
                auto& spawn = spawnIt->second.AsObject();
                if (auto posIt = spawn.find("position"); posIt != spawn.end() && posIt->second.IsObject()) {
                    auto& pos = posIt->second.AsObject();
                    if (auto xIt = pos.find("x"); xIt != pos.end() && xIt->second.IsNumber()) {
                        config.spawnPosition.x = xIt->second.AsNumber();
                    }
                    if (auto yIt = pos.find("y"); yIt != pos.end() && yIt->second.IsNumber()) {
                        config.spawnPosition.y = yIt->second.AsNumber();
                    }
                    if (auto zIt = pos.find("z"); zIt != pos.end() && zIt->second.IsNumber()) {
                        config.spawnPosition.z = zIt->second.AsNumber();
                    }
                }
            }
            
            // Load physics settings  
            if (auto physicsIt = player.find("physics"); physicsIt != player.end() && physicsIt->second.IsObject()) {
                auto& physics = physicsIt->second.AsObject();
                if (auto gravIt = physics.find("enable_gravity"); gravIt != physics.end() && gravIt->second.IsBoolean()) {
                    config.physics.enableGravity = gravIt->second.AsBoolean();
                }
                if (auto strengthIt = physics.find("gravity_strength"); strengthIt != physics.end() && strengthIt->second.IsNumber()) {
                    config.physics.gravityStrength = strengthIt->second.AsNumber();
                }
            }
            
            // Load visual settings
            if (auto visualIt = player.find("visual"); visualIt != player.end() && visualIt->second.IsObject()) {
                auto& visual = visualIt->second.AsObject();
                if (auto colorIt = visual.find("color"); colorIt != visual.end() && colorIt->second.IsObject()) {
                    auto& color = colorIt->second.AsObject();
                    if (auto rIt = color.find("r"); rIt != color.end() && rIt->second.IsNumber()) {
                        config.visual.r = static_cast<float>(rIt->second.AsNumber());
                    }
                    if (auto gIt = color.find("g"); gIt != color.end() && gIt->second.IsNumber()) {
                        config.visual.g = static_cast<float>(gIt->second.AsNumber());
                    }
                    if (auto bIt = color.find("b"); bIt != color.end() && bIt->second.IsNumber()) {
                        config.visual.b = static_cast<float>(bIt->second.AsNumber());
                    }
                }
                if (auto scaleIt = visual.find("scale"); scaleIt != visual.end() && scaleIt->second.IsNumber()) {
                    config.visual.scale = static_cast<float>(scaleIt->second.AsNumber());
                }
            }
            
            // Load camera settings
            if (auto cameraIt = player.find("camera"); cameraIt != player.end() && cameraIt->second.IsObject()) {
                auto& camera = cameraIt->second.AsObject();
                if (auto priorityIt = camera.find("priority"); priorityIt != camera.end() && priorityIt->second.IsNumber()) {
                    config.camera.priority = static_cast<int>(priorityIt->second.AsNumber());
                }
                if (auto activeIt = camera.find("is_active"); activeIt != camera.end() && activeIt->second.IsBoolean()) {
                    config.camera.isActive = activeIt->second.AsBoolean();
                }
            }
        }
        
        std::cout << "[PlayerConfig] Loaded configuration from " << filePath << std::endl;
        std::cout << "[PlayerConfig] Spawn: (" << config.spawnPosition.x << ", " << config.spawnPosition.y << ", " << config.spawnPosition.z << ")" << std::endl;
        std::cout << "[PlayerConfig] Movement speed: " << config.movement.forwardSpeed << ", Gravity: " << (config.physics.enableGravity ? "enabled" : "disabled") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[PlayerConfig] Error loading " << filePath << ": " << e.what() << ", using defaults" << std::endl;
    }
    
    return config;
}

void PlayerConfig::ApplyToPosition(std::shared_ptr<Position> pos) const {
    if (pos) {
        pos->x = spawnPosition.x;
        pos->y = spawnPosition.y;
        pos->z = spawnPosition.z;
    }
}

void PlayerConfig::ApplyToPlayerPhysics(std::shared_ptr<PlayerPhysics> physics) const {
    if (physics) {
        physics->enableGravity = this->physics.enableGravity;
        physics->gravity = this->physics.gravityStrength;
        physics->maxAscentSpeed = this->physics.maxAscentSpeed;
        physics->maxDescentSpeed = this->physics.maxDescentSpeed;
    }
}

void PlayerConfig::ApplyToDrawComponent(std::shared_ptr<DrawComponent> draw) const {
    if (draw) {
        draw->SetTint(visual.r, visual.g, visual.b);
        draw->meshScale = visual.scale;
        draw->meshHandle = visual.meshId;
    }
}

void PlayerConfig::ApplyToCameraComponent(std::shared_ptr<CameraComponent> cam) const {
    if (cam) {
        cam->priority = camera.priority;
        cam->isActive = camera.isActive;
    }
}

PlayerConfig PlayerConfig::GetDefault() {
    PlayerConfig config;
    // All defaults are already set in the struct definition
    return config;
}