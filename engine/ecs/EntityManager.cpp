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

ecs::EntityHandle EntityManager::GetArchetypeHandle(Entity e) const {
    if (!usingArchetypes_) {
        return ecs::EntityHandle::Null();
    }
    return GetModernHandle(e);
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

    std::unordered_map<Entity, ecs::EntityHandle> newLegacyToModern;
    std::unordered_map<uint32_t, Entity> newModernToLegacy;
    std::unordered_set<std::type_index> unsupported;

    MigrateToArchetypeManager(archetypeManager_, newLegacyToModern, newModernToLegacy, unsupported);

    legacyToModern_ = std::move(newLegacyToModern);
    modernToLegacy_ = std::move(newModernToLegacy);
    unsupportedComponentTypes_ = std::move(unsupported);

    AliasMigratedComponents();

    usingArchetypes_ = true;
}

void EntityManager::MigrateToArchetypeManager(ecs::EntityManagerV2& target,
                                              std::unordered_map<Entity, ecs::EntityHandle>& legacyToModernOut,
                                              std::unordered_map<uint32_t, Entity>& modernToLegacyOut,
                                              std::unordered_set<std::type_index>& unsupportedTypesOut) const {
    target.Clear();
    legacyToModernOut.clear();
    modernToLegacyOut.clear();
    unsupportedTypesOut.clear();

    for (Entity entity : aliveEntities) {
        ecs::EntityHandle handle = target.CreateEntity();
        legacyToModernOut.emplace(entity, handle);
        modernToLegacyOut.emplace(handle.value, entity);
    }

    for (const auto& [typeIndex, entityMap] : components) {
        if (entityMap.empty()) {
            continue;
        }

        if (!target.CanProvideComponentType(typeIndex)) {
            unsupportedTypesOut.insert(typeIndex);
            continue;
        }

        bool migrated = false;
        auto migrateIfMatches = [&](auto typeTag) {
            using ComponentType = std::decay_t<decltype(typeTag)>;
            if (typeIndex != std::type_index(typeid(ComponentType))) {
                return;
            }

            migrated = true;
            for (const auto& [entity, componentPtr] : entityMap) {
                if (!componentPtr) {
                    continue;
                }
                auto typedPtr = std::dynamic_pointer_cast<ComponentType>(componentPtr);
                if (!typedPtr) {
                    continue;
                }

                auto handleIt = legacyToModernOut.find(entity);
                if (handleIt == legacyToModernOut.end()) {
                    continue;
                }

                ComponentType* existing = target.GetComponent<ComponentType>(handleIt->second);
                if (existing) {
                    *existing = *typedPtr;
                } else {
                    target.AddComponent<ComponentType>(handleIt->second, *typedPtr);
                }
            }
        };

        std::apply([&](auto... typeTag) { (migrateIfMatches(typeTag), ...); }, FacadeComponentTypes{});

        if (!migrated) {
            unsupportedTypesOut.insert(typeIndex);
        }
    }
}

void EntityManager::AliasMigratedComponents() {
    for (auto& [typeIndex, entityMap] : components) {
        if (unsupportedComponentTypes_.count(typeIndex)) {
            continue;
        }

        auto aliasIfMatches = [&](auto typeTag) {
            using ComponentType = std::decay_t<decltype(typeTag)>;
            if (typeIndex != std::type_index(typeid(ComponentType))) {
                return;
            }

            for (auto& [entity, componentPtr] : entityMap) {
                if (!componentPtr) {
                    continue;
                }

                auto handleIt = legacyToModern_.find(entity);
                if (handleIt == legacyToModern_.end()) {
                    continue;
                }

                ComponentType* stored = archetypeManager_.GetComponent<ComponentType>(handleIt->second);
                if (!stored) {
                    continue;
                }

                componentPtr = AliasComponent(*stored);
            }
        };

        std::apply([&](auto... typeTag) { (aliasIfMatches(typeTag), ...); }, FacadeComponentTypes{});
    }
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
