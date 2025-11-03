#pragma once

#include "../ecs/EntityManager.h"
#include "../ecs/Components.h"
#include <string>
#include <vector>
#include <functional>

/**
 * ComponentInspector - Real-time component viewing and editing
 */
class ComponentInspector {
public:
    struct PropertyInfo {
        std::string name;
        std::string value;
        std::string type;
        std::function<void(const std::string&)> setter;
    };
    
    struct ComponentInfo {
        std::string typeName;
        std::vector<PropertyInfo> properties;
    };
    
    ComponentInspector(EntityManager* em);
    
    std::vector<ComponentInfo> InspectEntity(Entity entity);
    
    bool HasComponent(Entity entity, const std::string& componentType) const;
    
    std::string GetComponentSummary(Entity entity) const;
    
private:
    EntityManager* entityManager_;
    
    void InspectPosition(Entity entity, std::vector<ComponentInfo>& result);
    void InspectVelocity(Entity entity, std::vector<ComponentInfo>& result);
    void InspectDrawComponent(Entity entity, std::vector<ComponentInfo>& result);
    void InspectPlayerController(Entity entity, std::vector<ComponentInfo>& result);
    void InspectName(Entity entity, std::vector<ComponentInfo>& result);
    void InspectRigidBody(Entity entity, std::vector<ComponentInfo>& result);
};
