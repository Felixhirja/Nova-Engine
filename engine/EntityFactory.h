#pragma once

#include "EntityConfigManager.h"
#include "ecs/EntityManager.h"
#include "ActorContext.h"
#include "../entities/Player.h"
#include "../entities/NPC.h"
#include "../entities/Station.h"
#include "../entities/Spaceship.h"
#include "../entities/Projectile.h"
#include "../entities/CargoContainer.h"
#include <memory>
#include <string>

// Aliases for easier use
using Projectile = ProjectileActor;

/**
 * EntityFactory: Simplified entity creation with auto-loaded configurations
 * 
 * Features:
 * - One-line entity creation with automatic config loading
 * - Type-safe actor creation
 * - Automatic ECS integration
 * - Designer-friendly error handling
 */
class EntityFactory {
public:
    /**
     * Factory results for error handling
     */
    struct CreateResult {
        Entity entity = 0;
        bool success = false;
        std::string errorMessage;
        std::unique_ptr<IActor> actor;
    };

    /**
     * Create factory with entity manager
     */
    explicit EntityFactory(::EntityManager& entityManager) 
        : entityManager_(entityManager) {}

    /**
     * Create entities with automatic configuration loading
     */
    CreateResult CreatePlayer(double x = 0.0, double y = 0.0, double z = 0.0);
    CreateResult CreateNPC(const std::string& npcType = "default", double x = 0.0, double y = 0.0, double z = 0.0);
    CreateResult CreateStation(const std::string& stationType = "trading", double x = 0.0, double y = 0.0, double z = 0.0);
    CreateResult CreateSpaceship(const std::string& shipClass = "fighter", double x = 0.0, double y = 0.0, double z = 0.0);
    CreateResult CreateProjectile(const std::string& projectileType = "default", double x = 0.0, double y = 0.0, double z = 0.0);
    CreateResult CreateCargoContainer(const std::string& containerType = "general", double x = 0.0, double y = 0.0, double z = 0.0);

    /**
     * Generic entity creation from config type
     */
    CreateResult CreateFromConfig(const std::string& configType, double x = 0.0, double y = 0.0, double z = 0.0);

    /**
     * Create entities with custom position override
     */
    template<typename ActorType>
    CreateResult CreateCustomActor(const std::string& configType, double x, double y, double z);

    /**
     * List all available entity types that can be created
     */
    std::vector<std::string> GetAvailableTypes() const;

    /**
     * Check if a configuration exists for entity creation
     */
    bool CanCreate(const std::string& entityType) const;

    /**
     * Hot-reload configurations during development
     */
    void RefreshConfigurations();

private:
    ::EntityManager& entityManager_;

    /**
     * Helper methods for entity setup
     */
    void ApplyPosition(Entity entity, double x, double y, double z);
    bool ValidateEntity(Entity entity, const std::string& entityType);
    std::string FormatError(const std::string& operation, const std::string& entityType, const std::string& details);
};

// Template implementation
template<typename ActorType>
EntityFactory::CreateResult EntityFactory::CreateCustomActor(const std::string& configType, double x, double y, double z) {
    CreateResult result;
    
    try {
        // Create entity
        result.entity = entityManager_.CreateEntity();
        if (result.entity == 0) {
            result.errorMessage = FormatError("CreateEntity", configType, "EntityManager returned invalid entity ID");
            return result;
        }
        
        // Create actor instance
        auto actor = std::make_unique<ActorType>();
        
        // Set up actor context
        ActorContext context(entityManager_, result.entity);
        actor->AttachContext(context);
        
        // Initialize actor (this loads configuration automatically)
        actor->Initialize();
        
        // Apply position
        ApplyPosition(result.entity, x, y, z);
        
        // Validate final setup
        if (!ValidateEntity(result.entity, configType)) {
            result.errorMessage = FormatError("Validation", configType, "Entity failed post-creation validation");
            return result;
        }
        
        result.actor = std::move(actor);
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = FormatError("Exception", configType, e.what());
    }
    
    return result;
}