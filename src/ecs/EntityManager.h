#pragma once
#include <cassert>
#include <array>
#include <functional>
#include <memory>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "Component.h"
#include "Components.h"
#include "EntityManagerV2.h"
#include "../CelestialBody.h"

namespace entity_manager_detail {

template<typename T, typename Tuple>
struct TupleContains;

template<typename T>
struct TupleContains<T, std::tuple<>> : std::false_type {};

template<typename T, typename U, typename... Rest>
struct TupleContains<T, std::tuple<U, Rest...>> : TupleContains<T, std::tuple<Rest...>> {};

template<typename T, typename... Rest>
struct TupleContains<T, std::tuple<T, Rest...>> : std::true_type {};

template<typename T, typename Tuple>
constexpr bool IsTypeInTuple = TupleContains<T, Tuple>::value;

} // namespace entity_manager_detail

using Entity = int;

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity e);

    bool IsAlive(Entity e) const;

    void EnableArchetypeFacade();
    bool UsingArchetypeStorage() const { return usingArchetypes_; }

    ecs::EntityManagerV2& GetArchetypeManager() { return archetypeManager_; }
    const ecs::EntityManagerV2& GetArchetypeManager() const { return archetypeManager_; }

    using FacadeComponentTypes = std::tuple<
        Position,
        Velocity,
        Acceleration,
        PhysicsBody,
        Transform2D,
        Sprite,
        Hitbox,
        AnimationState,
        Name,
        PlayerController,
        MovementParameters,
        MovementBounds,
        PlayerPhysics,
        LocomotionStateMachine,
        TargetLock,
        RigidBody,
        Force,
        Collider,
        CollisionInfo,
        GravitySource,
        ConstantForce,
        CharacterController,
        Joint,
        CelestialBodyComponent,
        OrbitalComponent,
        VisualCelestialComponent,
        AtmosphereComponent,
        SpaceStationComponent,
        SatelliteSystemComponent,
        StarComponent,
        AsteroidBeltComponent,
        PlanetComponent>;

    template<typename T>
    static constexpr bool IsArchetypeFacadeCompatible() {
        return entity_manager_detail::IsTypeInTuple<std::decay_t<T>, FacadeComponentTypes>;
    }

    void MigrateToArchetypeManager(ecs::EntityManagerV2& target,
                                   std::unordered_map<Entity, ecs::EntityHandle>& legacyToModernOut,
                                   std::unordered_map<uint32_t, Entity>& modernToLegacyOut,
                                   std::unordered_set<std::type_index>& unsupportedTypesOut) const;

    const std::unordered_set<std::type_index>& GetUnsupportedComponentTypes() const {
        return unsupportedComponentTypes_;
    }

    std::vector<std::type_index> GetComponentTypes(Entity e) const;
    void EnumerateEntities(const std::function<void(Entity, const std::vector<std::type_index>&)>& callback) const;

    template<typename T>
    void AddComponent(Entity e, std::shared_ptr<T> comp) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        assert(IsAlive(e) && "Adding component to non-existent entity");
        auto& map = components[std::type_index(typeid(T))];

        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            if (!handle.IsNull()) {
                if (archetypeManager_.HasComponent<T>(handle)) {
                    T* existing = archetypeManager_.GetComponent<T>(handle);
                    if (existing) {
                        if (comp) {
                            *existing = *comp;
                        }
                        map[e] = AliasComponent(*existing);
                        return;
                    }
                }
                T& stored = comp ? archetypeManager_.AddComponent<T>(handle, *comp)
                                 : archetypeManager_.AddComponent<T>(handle);
                map[e] = AliasComponent(stored);
                return;
            }
        }

        map[e] = std::move(comp);
    }

    template<typename T, typename... Args>
    T& EmplaceComponent(Entity e, Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        AddComponent<T>(e, ptr);
        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            auto* component = archetypeManager_.GetComponent<T>(handle);
            if (component) {
                return *component;
            }
        }
        return *ptr;
    }

    template<typename T>
    void RemoveComponent(Entity e) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        auto typeIndex = std::type_index(typeid(T));
        auto it = components.find(typeIndex);
        if (it != components.end()) {
            it->second.erase(e);
        }
        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            if (!handle.IsNull()) {
                archetypeManager_.RemoveComponent<T>(handle);
            }
        }
    }

    template<typename T>
    bool HasComponent(Entity e) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return false;
        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            return !handle.IsNull() && archetypeManager_.HasComponent<T>(handle);
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return false;
        return it->second.find(e) != it->second.end();
    }

    template<typename T>
    T* GetComponent(Entity e) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return nullptr;
        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            if (handle.IsNull()) return nullptr;
            return archetypeManager_.GetComponent<T>(handle);
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto& map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<T*>(jt->second.get());
    }

    template<typename T>
    const T* GetComponent(Entity e) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return nullptr;
        if (ShouldUseArchetypeStorage<T>()) {
            ecs::EntityHandle handle = GetModernHandle(e);
            if (handle.IsNull()) return nullptr;
            return archetypeManager_.GetComponent<T>(handle);
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto& map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<const T*>(jt->second.get());
    }

    template<typename T>
    std::vector<std::pair<Entity, T*>> GetAllWith() {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        std::vector<std::pair<Entity, T*>> out;
        if (ShouldUseArchetypeStorage<T>()) {
            archetypeManager_.ForEach<T>([&](ecs::EntityHandle handle, T& component) {
                auto found = modernToLegacy_.find(handle.value);
                if (found != modernToLegacy_.end()) {
                    out.emplace_back(found->second, &component);
                }
            });
            return out;
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return out;
        for (auto& kv : it->second) {
            out.emplace_back(kv.first, static_cast<T*>(kv.second.get()));
        }
        return out;
    }

    template<typename T>
    std::vector<std::pair<Entity, const T*>> GetAllWith() const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        std::vector<std::pair<Entity, const T*>> out;
        if (ShouldUseArchetypeStorage<T>()) {
            archetypeManager_.ForEach<T>([&](ecs::EntityHandle handle, const T& component) {
                auto found = modernToLegacy_.find(handle.value);
                if (found != modernToLegacy_.end()) {
                    out.emplace_back(found->second, &component);
                }
            });
            return out;
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return out;
        for (const auto& kv : it->second) {
            out.emplace_back(kv.first, static_cast<const T*>(kv.second.get()));
        }
        return out;
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (ShouldUseArchetypeStorage<T>()) {
            if constexpr (sizeof...(Ts) == 0) {
                archetypeManager_.ForEach<T>([&](ecs::EntityHandle handle, T& first) {
                    auto found = modernToLegacy_.find(handle.value);
                    if (found != modernToLegacy_.end()) {
                        func(found->second, first);
                    }
                });
            } else {
                archetypeManager_.ForEach<T, Ts...>([&](ecs::EntityHandle handle, T& first, Ts&... rest) {
                    auto found = modernToLegacy_.find(handle.value);
                    if (found != modernToLegacy_.end()) {
                        func(found->second, first, rest...);
                    }
                });
            }
            return;
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return;
        for (auto& kv : it->second) {
            Entity ent = kv.first;
            auto* first = static_cast<T*>(kv.second.get());
            if (!first) {
                LogForEachComponentMismatch(ent,
                                            {std::type_index(typeid(T))},
                                            {std::type_index(typeid(T))});
                continue;
            }
            if constexpr (sizeof...(Ts) == 0) {
                func(ent, *first);
            } else {
                auto rest = std::tuple<Ts*...>{GetComponent<Ts>(ent)...};
                bool all = true;
                std::vector<std::type_index> missingTypes;
                const std::array<std::type_index, sizeof...(Ts)> restTypeIndices = {std::type_index(typeid(Ts))...};
                size_t idx = 0;
                auto processPtr = [&](auto* ptr) {
                    all = all && (ptr != nullptr);
                    if (!ptr) {
                        missingTypes.push_back(restTypeIndices[idx]);
                    }
                    ++idx;
                };
                std::apply([&](auto*... ptrs) {
                    (processPtr(ptrs), ...);
                }, rest);
                if (!all) {
                    std::vector<std::type_index> requestedTypes = {std::type_index(typeid(T)), std::type_index(typeid(Ts))...};
                    LogForEachComponentMismatch(ent, requestedTypes, missingTypes);
                    continue;
                }
                std::apply([&](auto*... ptrs) {
                    func(ent, *first, *ptrs...);
                }, rest);
            }
        }
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (ShouldUseArchetypeStorage<T>()) {
            auto entries = GetAllWith<T>();
            for (auto& entry : entries) {
                Entity ent = entry.first;
                const T* first = entry.second;
                if (!first) continue;
                if constexpr (sizeof...(Ts) == 0) {
                    func(ent, *first);
                } else {
                    auto rest = std::tuple<const Ts*...>{GetComponent<Ts>(ent)...};
                    bool all = true;
                    std::apply([&](auto*... ptrs) {
                        ((all = all && (ptrs != nullptr)), ...);
                    }, rest);
                    if (!all) continue;
                    std::apply([&](auto*... ptrs) {
                        func(ent, *first, *ptrs...);
                    }, rest);
                }
            }
            return;
        }
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return;
        for (const auto& kv : it->second) {
            Entity ent = kv.first;
            auto* first = static_cast<const T*>(kv.second.get());
            if (!first) {
                LogForEachComponentMismatch(ent,
                                            {std::type_index(typeid(T))},
                                            {std::type_index(typeid(T))});
                continue;
            }
            if constexpr (sizeof...(Ts) == 0) {
                func(ent, *first);
            } else {
                auto rest = std::tuple<const Ts*...>{GetComponent<Ts>(ent)...};
                bool all = true;
                std::vector<std::type_index> missingTypes;
                const std::array<std::type_index, sizeof...(Ts)> restTypeIndices = {std::type_index(typeid(Ts))...};
                size_t idx = 0;
                auto processPtr = [&](auto* ptr) {
                    all = all && (ptr != nullptr);
                    if (!ptr) {
                        missingTypes.push_back(restTypeIndices[idx]);
                    }
                    ++idx;
                };
                std::apply([&](auto*... ptrs) {
                    (processPtr(ptrs), ...);
                }, rest);
                if (!all) {
                    std::vector<std::type_index> requestedTypes = {std::type_index(typeid(T)), std::type_index(typeid(Ts))...};
                    LogForEachComponentMismatch(ent, requestedTypes, missingTypes);
                    continue;
                }
                std::apply([&](auto*... ptrs) {
                    func(ent, *first, *ptrs...);
                }, rest);
            }
        }
    }

    void Clear();

private:
    template<typename T>
    bool ShouldUseArchetypeStorage() const {
        if (!usingArchetypes_) return false;
        return unsupportedComponentTypes_.find(std::type_index(typeid(T))) == unsupportedComponentTypes_.end() &&
               archetypeManager_.CanProvideComponentType(std::type_index(typeid(T)));
    }

    ecs::EntityHandle GetModernHandle(Entity e) const {
        auto it = legacyToModern_.find(e);
        if (it != legacyToModern_.end()) {
            return it->second;
        }
        return ecs::EntityHandle::Null();
    }

    template<typename T>
    std::shared_ptr<Component> AliasComponent(T& component) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        return std::shared_ptr<Component>(static_cast<Component*>(&component), [](Component*) {});
    }

    void AliasMigratedComponents();

    static void LogForEachComponentMismatch(Entity entity,
                                            const std::vector<std::type_index>& requested,
                                            const std::vector<std::type_index>& missing);

    Entity nextEntity = 1;
    std::unordered_set<Entity> aliveEntities;
    std::vector<Entity> freeEntities;
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::shared_ptr<Component>>> components;

    bool usingArchetypes_ = false;
    ecs::EntityManagerV2 archetypeManager_;
    std::unordered_map<Entity, ecs::EntityHandle> legacyToModern_;
    std::unordered_map<uint32_t, Entity> modernToLegacy_;
    std::unordered_set<std::type_index> unsupportedComponentTypes_;
};
