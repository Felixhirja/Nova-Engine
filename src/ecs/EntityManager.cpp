#include "EntityManager.h"
#include "Components.h"
#include "TypeNameUtils.h"
#include "../CelestialBody.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <unordered_set>

EntityManager::EntityManager() {}
EntityManager::~EntityManager() {}

Entity EntityManager::CreateEntity() {
    Entity e = 0;
    if (!freeEntities.empty()) {
        e = freeEntities.back();
        freeEntities.pop_back();
    } else {
        e = nextEntity++;
    }
    aliveEntities.insert(e);

    if (usingArchetypes_) {
        ecs::EntityHandle handle = archetypeManager_.CreateEntity();
        legacyToModern_[e] = handle;
        modernToLegacy_[handle.value] = e;
    }
    return e;
}

void EntityManager::DestroyEntity(Entity e) {
    if (aliveEntities.erase(e) == 0) {
        return;
    }
    for (auto &kv : components) {
        kv.second.erase(e);
    }

    if (usingArchetypes_) {
        auto it = legacyToModern_.find(e);
        if (it != legacyToModern_.end()) {
            ecs::EntityHandle handle = it->second;
            archetypeManager_.DestroyEntity(handle);
            modernToLegacy_.erase(handle.value);
            legacyToModern_.erase(it);
        }
    }
    freeEntities.push_back(e);
}

bool EntityManager::IsAlive(Entity e) const {
    if (usingArchetypes_) {
        ecs::EntityHandle handle = GetModernHandle(e);
        if (handle.IsNull()) {
            return false;
        }
        return archetypeManager_.IsAlive(handle);
    }
    return aliveEntities.find(e) != aliveEntities.end();
}

void EntityManager::Clear() {
    components.clear();
    aliveEntities.clear();
    freeEntities.clear();
    nextEntity = 1;
    if (usingArchetypes_) {
        archetypeManager_.Clear();
        legacyToModern_.clear();
        modernToLegacy_.clear();
    }
}

void EntityManager::EnableArchetypeFacade() {
    if (usingArchetypes_) {
        return;
    }

    archetypeManager_.Clear();
    legacyToModern_.clear();
    modernToLegacy_.clear();
    unsupportedComponentTypes_.clear();

    for (Entity entity : aliveEntities) {
        ecs::EntityHandle handle = archetypeManager_.CreateEntity();
        legacyToModern_[entity] = handle;
        modernToLegacy_[handle.value] = entity;
    }

    auto migrateIfMatches = [&](const std::type_index& typeIndex,
                                auto typeTag,
                                auto& entityMap,
                                bool& migrated) {
        using ComponentType = decltype(typeTag);
        if (typeIndex != std::type_index(typeid(ComponentType))) {
            return;
        }

        for (auto& [entity, componentPtr] : entityMap) {
            if (!componentPtr) continue;
            auto typedPtr = std::dynamic_pointer_cast<ComponentType>(componentPtr);
            if (!typedPtr) continue;

            ecs::EntityHandle handle = GetModernHandle(entity);
            if (handle.IsNull()) continue;

            ComponentType& stored = archetypeManager_.AddComponent<ComponentType>(handle, *typedPtr);
            componentPtr = AliasComponent(stored);
        }

        migrated = true;
    };

    for (auto& [typeIndex, entityMap] : components) {
        if (entityMap.empty()) {
            continue;
        }
        if (!archetypeManager_.CanProvideComponentType(typeIndex)) {
            unsupportedComponentTypes_.insert(typeIndex);
            continue;
        }

        bool migrated = false;
        migrateIfMatches(typeIndex, Position{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Velocity{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Acceleration{}, entityMap, migrated);
        migrateIfMatches(typeIndex, PhysicsBody{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Transform2D{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Sprite{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Hitbox{}, entityMap, migrated);
        migrateIfMatches(typeIndex, AnimationState{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Name{}, entityMap, migrated);
        migrateIfMatches(typeIndex, PlayerController{}, entityMap, migrated);
        migrateIfMatches(typeIndex, MovementParameters{}, entityMap, migrated);
        migrateIfMatches(typeIndex, MovementBounds{}, entityMap, migrated);
        migrateIfMatches(typeIndex, PlayerPhysics{}, entityMap, migrated);
        migrateIfMatches(typeIndex, LocomotionStateMachine{}, entityMap, migrated);
        migrateIfMatches(typeIndex, TargetLock{}, entityMap, migrated);
        migrateIfMatches(typeIndex, RigidBody{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Force{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Collider{}, entityMap, migrated);
        migrateIfMatches(typeIndex, CollisionInfo{}, entityMap, migrated);
        migrateIfMatches(typeIndex, GravitySource{}, entityMap, migrated);
        migrateIfMatches(typeIndex, ConstantForce{}, entityMap, migrated);
        migrateIfMatches(typeIndex, CharacterController{}, entityMap, migrated);
        migrateIfMatches(typeIndex, Joint{}, entityMap, migrated);
        migrateIfMatches(typeIndex, CelestialBodyComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, OrbitalComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, VisualCelestialComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, AtmosphereComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, SpaceStationComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, SatelliteSystemComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, StarComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, AsteroidBeltComponent{}, entityMap, migrated);
        migrateIfMatches(typeIndex, PlanetComponent{}, entityMap, migrated);

        if (!migrated) {
            unsupportedComponentTypes_.insert(typeIndex);
        }
    }

    usingArchetypes_ = true;
}

std::vector<std::type_index> EntityManager::GetComponentTypes(Entity e) const {
    std::vector<std::type_index> result;
    if (!IsAlive(e)) {
        return result;
    }

    std::unordered_set<std::type_index> seen;
    seen.reserve(16);

    if (usingArchetypes_) {
        ecs::EntityHandle handle = GetModernHandle(e);
        if (!handle.IsNull()) {
            auto archetypeTypes = archetypeManager_.GetComponentTypes(handle);
            for (const auto& typeIndex : archetypeTypes) {
                if (seen.insert(typeIndex).second) {
                    result.push_back(typeIndex);
                }
            }
        }
    }

    for (const auto& [typeIndex, entityMap] : components) {
        auto it = entityMap.find(e);
        if (it != entityMap.end()) {
            if (seen.insert(typeIndex).second) {
                result.push_back(typeIndex);
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const std::type_index& a, const std::type_index& b) {
        return a.hash_code() < b.hash_code();
    });

    return result;
}

void EntityManager::EnumerateEntities(const std::function<void(Entity, const std::vector<std::type_index>&)>& callback) const {
    if (!callback) {
        return;
    }

    for (Entity entity : aliveEntities) {
        callback(entity, GetComponentTypes(entity));
    }
}

namespace {
std::string JoinTypeNames(const std::vector<std::type_index>& types) {
    if (types.empty()) {
        return "(none)";
    }

    std::ostringstream oss;
    bool first = true;
    for (const auto& type : types) {
        if (!first) {
            oss << ", ";
        }
        oss << ecs::debug::GetReadableTypeName(type);
        first = false;
    }
    return oss.str();
}
} // namespace

void EntityManager::LogForEachComponentMismatch(Entity entity,
                                                const std::vector<std::type_index>& requested,
                                                const std::vector<std::type_index>& missing) {
    if (missing.empty()) {
        return;
    }

    static std::unordered_set<std::string> loggedMessages;
    std::ostringstream keyStream;
    keyStream << entity;
    for (const auto& type : missing) {
        keyStream << '|' << type.name();
    }
    keyStream << "->";
    for (const auto& type : requested) {
        keyStream << type.name() << ';';
    }

    std::string key = keyStream.str();
    if (!loggedMessages.insert(key).second) {
        return;
    }

    std::ostringstream message;
    message << "[ECS] ForEach mismatch on entity " << entity
            << ": missing {" << JoinTypeNames(missing)
            << "} while requesting {" << JoinTypeNames(requested) << "}";
    std::cerr << message.str() << std::endl;

#ifndef NDEBUG
    assert(!"Entity missing required components for ForEach");
#endif
}
