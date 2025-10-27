#pragma once

#include "EntityManager.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * Compatibility facade that lets legacy systems continue to use the classic
 * EntityManager API while operating directly on the archetype-backed
 * EntityManagerV2 implementation. The facade forwards all supported calls to
 * the legacy manager after enabling the archetype facade. Unsupported
 * operations (primarily component types that have not been registered with the
 * archetype manager) trigger compile-time assertions so missing coverage is
 * caught during migration.
 *
 * Unsupported pathways:
 *  - Direct access to the legacy component map is not available.
 *  - Component types that are not enumerated in EntityManager::FacadeComponentTypes
 *    remain on the legacy storage path and cannot be accessed through this
 *    adapter.
 */
namespace ecs {

class EntityManagerFacade {
public:
    explicit EntityManagerFacade(EntityManager& legacyManager)
        : legacyManager_(legacyManager) {
        legacyManager_.EnableArchetypeFacade();
    }

    Entity CreateEntity() { return legacyManager_.CreateEntity(); }
    void DestroyEntity(Entity entity) { legacyManager_.DestroyEntity(entity); }

    bool IsAlive(Entity entity) const { return legacyManager_.IsAlive(entity); }

    template<typename T>
    void AddComponent(Entity entity, std::shared_ptr<T> component) {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        legacyManager_.AddComponent<T>(entity, std::move(component));
    }

    template<typename T, typename... Args>
    T& EmplaceComponent(Entity entity, Args&&... args) {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.EmplaceComponent<T>(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        legacyManager_.RemoveComponent<T>(entity);
    }

    template<typename T>
    bool HasComponent(Entity entity) const {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.HasComponent<T>(entity);
    }

    template<typename T>
    T* GetComponent(Entity entity) {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.GetComponent<T>(entity);
    }

    template<typename T>
    const T* GetComponent(Entity entity) const {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.GetComponent<T>(entity);
    }

    template<typename T>
    std::vector<std::pair<Entity, T*>> GetAllWith() {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.GetAllWith<T>();
    }

    template<typename T>
    std::vector<std::pair<Entity, const T*>> GetAllWith() const {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        return legacyManager_.GetAllWith<T>();
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        static_assert((EntityManager::IsArchetypeFacadeCompatible<Ts>() && ...),
                      "Component type is not supported by the archetype facade");
        legacyManager_.ForEach<T, Ts...>(std::forward<Func>(func));
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) const {
        static_assert(EntityManager::IsArchetypeFacadeCompatible<T>(),
                      "Component type is not supported by the archetype facade");
        static_assert((EntityManager::IsArchetypeFacadeCompatible<Ts>() && ...),
                      "Component type is not supported by the archetype facade");
        legacyManager_.ForEach<T, Ts...>(std::forward<Func>(func));
    }

    void Clear() { legacyManager_.Clear(); }

    ecs::EntityManagerV2& GetArchetypeManager() { return legacyManager_.GetArchetypeManager(); }
    const ecs::EntityManagerV2& GetArchetypeManager() const { return legacyManager_.GetArchetypeManager(); }

private:
    EntityManager& legacyManager_;
};

} // namespace ecs

