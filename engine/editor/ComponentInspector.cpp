#include "ComponentInspector.h"
#include <sstream>
#include <iomanip>

ComponentInspector::ComponentInspector(EntityManager* em)
    : entityManager_(em) {
}

std::vector<ComponentInspector::ComponentInfo> ComponentInspector::InspectEntity(Entity entity) {
    std::vector<ComponentInfo> result;
    
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return result;
    }
    
    InspectPosition(entity, result);
    InspectVelocity(entity, result);
    InspectDrawComponent(entity, result);
    InspectPlayerController(entity, result);
    InspectName(entity, result);
    InspectRigidBody(entity, result);
    
    return result;
}

bool ComponentInspector::HasComponent(Entity entity, const std::string& componentType) const {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) return false;
    
    if (componentType == "Position") return entityManager_->GetComponent<Position>(entity) != nullptr;
    if (componentType == "Velocity") return entityManager_->GetComponent<Velocity>(entity) != nullptr;
    if (componentType == "DrawComponent") return entityManager_->GetComponent<DrawComponent>(entity) != nullptr;
    if (componentType == "PlayerController") return entityManager_->GetComponent<PlayerController>(entity) != nullptr;
    if (componentType == "Name") return entityManager_->GetComponent<Name>(entity) != nullptr;
    if (componentType == "RigidBody") return entityManager_->GetComponent<RigidBody>(entity) != nullptr;
    
    return false;
}

std::string ComponentInspector::GetComponentSummary(Entity entity) const {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return "Invalid entity";
    }
    
    std::stringstream ss;
    int count = 0;
    
    if (entityManager_->GetComponent<Position>(entity)) { ss << "Pos "; count++; }
    if (entityManager_->GetComponent<Velocity>(entity)) { ss << "Vel "; count++; }
    if (entityManager_->GetComponent<DrawComponent>(entity)) { ss << "Draw "; count++; }
    if (entityManager_->GetComponent<PlayerController>(entity)) { ss << "Player "; count++; }
    if (entityManager_->GetComponent<Name>(entity)) { ss << "Name "; count++; }
    if (entityManager_->GetComponent<RigidBody>(entity)) { ss << "Physics "; count++; }
    
    if (count == 0) return "No components";
    
    return ss.str() + "(" + std::to_string(count) + ")";
}

void ComponentInspector::InspectPosition(Entity entity, std::vector<ComponentInfo>& result) {
    auto* pos = entityManager_->GetComponent<Position>(entity);
    if (!pos) return;
    
    ComponentInfo info;
    info.typeName = "Position";
    
    info.properties.push_back({
        "x", std::to_string(pos->x), "double",
        [this, entity](const std::string& val) {
            if (auto* p = entityManager_->GetComponent<Position>(entity)) {
                p->x = std::stod(val);
            }
        }
    });
    
    info.properties.push_back({
        "y", std::to_string(pos->y), "double",
        [this, entity](const std::string& val) {
            if (auto* p = entityManager_->GetComponent<Position>(entity)) {
                p->y = std::stod(val);
            }
        }
    });
    
    info.properties.push_back({
        "z", std::to_string(pos->z), "double",
        [this, entity](const std::string& val) {
            if (auto* p = entityManager_->GetComponent<Position>(entity)) {
                p->z = std::stod(val);
            }
        }
    });
    
    result.push_back(info);
}

void ComponentInspector::InspectVelocity(Entity entity, std::vector<ComponentInfo>& result) {
    auto* vel = entityManager_->GetComponent<Velocity>(entity);
    if (!vel) return;
    
    ComponentInfo info;
    info.typeName = "Velocity";
    
    info.properties.push_back({
        "vx", std::to_string(vel->vx), "double",
        [this, entity](const std::string& val) {
            if (auto* v = entityManager_->GetComponent<Velocity>(entity)) {
                v->vx = std::stod(val);
            }
        }
    });
    
    info.properties.push_back({
        "vy", std::to_string(vel->vy), "double",
        [this, entity](const std::string& val) {
            if (auto* v = entityManager_->GetComponent<Velocity>(entity)) {
                v->vy = std::stod(val);
            }
        }
    });
    
    info.properties.push_back({
        "vz", std::to_string(vel->vz), "double",
        [this, entity](const std::string& val) {
            if (auto* v = entityManager_->GetComponent<Velocity>(entity)) {
                v->vz = std::stod(val);
            }
        }
    });
    
    result.push_back(info);
}

void ComponentInspector::InspectDrawComponent(Entity entity, std::vector<ComponentInfo>& result) {
    auto* draw = entityManager_->GetComponent<DrawComponent>(entity);
    if (!draw) return;
    
    ComponentInfo info;
    info.typeName = "DrawComponent";
    
    std::string renderMode;
    switch (draw->mode) {
        case DrawComponent::RenderMode::None: renderMode = "None"; break;
        case DrawComponent::RenderMode::Mesh3D: renderMode = "Mesh3D"; break;
        case DrawComponent::RenderMode::Billboard: renderMode = "Billboard"; break;
        case DrawComponent::RenderMode::Sprite2D: renderMode = "Sprite2D"; break;
        case DrawComponent::RenderMode::Particles: renderMode = "Particles"; break;
        case DrawComponent::RenderMode::Wireframe: renderMode = "Wireframe"; break;
        case DrawComponent::RenderMode::Custom: renderMode = "Custom"; break;
    }
    
    PropertyInfo modeProp;
    modeProp.name = "mode";
    modeProp.value = renderMode;
    modeProp.type = "enum";
    info.properties.push_back(modeProp);
    
    PropertyInfo texProp;
    texProp.name = "textureHandle";
    texProp.value = std::to_string(draw->textureHandle);
    texProp.type = "int";
    info.properties.push_back(texProp);
    
    PropertyInfo visProp;
    visProp.name = "visible";
    visProp.value = draw->visible ? "true" : "false";
    visProp.type = "bool";
    info.properties.push_back(visProp);
    
    result.push_back(info);
}

void ComponentInspector::InspectPlayerController(Entity entity, std::vector<ComponentInfo>& result) {
    auto* player = entityManager_->GetComponent<PlayerController>(entity);
    if (!player) return;
    
    ComponentInfo info;
    info.typeName = "PlayerController";
    
    PropertyInfo fwdProp;
    fwdProp.name = "moveForward";
    fwdProp.value = player->moveForward ? "true" : "false";
    fwdProp.type = "bool";
    info.properties.push_back(fwdProp);
    
    PropertyInfo backProp;
    backProp.name = "moveBackward";
    backProp.value = player->moveBackward ? "true" : "false";
    backProp.type = "bool";
    info.properties.push_back(backProp);
    
    PropertyInfo boostProp;
    boostProp.name = "boost";
    boostProp.value = player->boost ? "true" : "false";
    boostProp.type = "bool";
    info.properties.push_back(boostProp);
    
    result.push_back(info);
}

void ComponentInspector::InspectName(Entity entity, std::vector<ComponentInfo>& result) {
    auto* name = entityManager_->GetComponent<Name>(entity);
    if (!name) return;
    
    ComponentInfo info;
    info.typeName = "Name";
    
    info.properties.push_back({
        "value", name->value, "string",
        [this, entity](const std::string& val) {
            if (auto* n = entityManager_->GetComponent<Name>(entity)) {
                n->value = val;
            }
        }
    });
    
    result.push_back(info);
}

void ComponentInspector::InspectRigidBody(Entity entity, std::vector<ComponentInfo>& result) {
    auto* rb = entityManager_->GetComponent<RigidBody>(entity);
    if (!rb) return;
    
    ComponentInfo info;
    info.typeName = "RigidBody";
    
    info.properties.push_back({
        "mass", std::to_string(rb->mass), "double",
        [this, entity](const std::string& val) {
            if (auto* r = entityManager_->GetComponent<RigidBody>(entity)) {
                r->SetMass(std::stod(val));
            }
        }
    });
    
    info.properties.push_back({
        "useGravity", rb->useGravity ? "true" : "false", "bool", nullptr
    });
    
    info.properties.push_back({
        "isKinematic", rb->isKinematic ? "true" : "false", "bool", nullptr
    });
    
    result.push_back(info);
}
