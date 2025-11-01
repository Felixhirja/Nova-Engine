#pragma once
#ifndef ECS_ENTITY_MANAGER_H
#define ECS_ENTITY_MANAGER_H

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
#include <stdexcept>
#include "Component.h"
#include "ArchetypeManager.h"
#include "EntityHandle.h"
#include "Components.h"
#include "TransitionPlan.h"
#include <iostream>

// Forward declarations to avoid external dependencies
struct CelestialBodyComponent;

// ============================================================================
// ENTITY MANAGER V2 (ARCHETYPE-BASED)
// ============================================================================

namespace ecs {

// Next-generation EntityManager using archetype storage
// Provides versioned entity handles and cache-friendly component iteration
class EntityManagerV2 {
public:
    EntityManagerV2() = default;
    ~EntityManagerV2() = default;
    
    // ===== Entity Lifecycle =====
    
    EntityHandle CreateEntity() {
        EntityIndex index;
        EntityGeneration generation;
        
        // Reuse free entity slots
        if (!freeIndices_.empty()) {
            index = freeIndices_.back();
            freeIndices_.pop_back();
            
            EntityMetadata& meta = entityMetadata_[index];
            
            // Check generation overflow
            if (meta.generation == EntityHandle::MAX_GENERATION) {
                // Generation overflow - skip this slot and try next
                if (!freeIndices_.empty()) {
                    return CreateEntity();
                }
                // If no more free slots, fall through to allocate new
            } else {
                generation = ++meta.generation;
                
                EntityHandle handle(index, generation);
                meta.alive = true;
                meta.archetypeId = 0;
                
                Archetype* archetype = archetypeManager_.GetArchetype(0);
                if (!archetype) {
                    throw std::runtime_error("Empty archetype not initialized");
                }
                meta.indexInArchetype = static_cast<uint32_t>(archetype->AddEntity(handle));
                
                return handle;
            }
        }
        
        // Check entity limit (24-bit index = 16,777,215 entities max)
        if (entityMetadata_.size() >= EntityHandle::MAX_ENTITIES) {
            throw std::runtime_error("Entity limit reached (16,777,215 entities maximum)");
        }
        
        // Allocate new entity slot
        index = static_cast<EntityIndex>(entityMetadata_.size());
        generation = 0;
        entityMetadata_.emplace_back(generation, 0, 0);
        
        EntityHandle handle(index, generation);
        EntityMetadata& meta = entityMetadata_[index];
        meta.alive = true;
        meta.archetypeId = 0;
        
        Archetype* archetype = archetypeManager_.GetArchetype(0);
        if (!archetype) {
            throw std::runtime_error("Empty archetype not initialized");
        }
        meta.indexInArchetype = static_cast<uint32_t>(archetype->AddEntity(handle));
        
        return handle;
    }
    
    void DestroyEntity(EntityHandle handle) {
        if (IsIterating()) {
            QueueDeferredDestroy(handle);
            return;
        }
        DestroyEntityImmediate(handle);
    }
    
    bool IsAlive(EntityHandle handle) const {
        if (handle.IsNull()) return false;
        EntityIndex index = handle.Index();
        if (index >= entityMetadata_.size()) return false;
        
        const EntityMetadata& meta = entityMetadata_[index];
        return meta.alive && meta.generation == handle.Generation();
    }
    
    // ===== Component Management =====
    
    template<typename T, typename... Args>
    T& AddComponent(EntityHandle handle, Args&&... args) {
        if (IsIterating()) {
            return QueueDeferredAdd<T>(handle, std::forward<Args>(args)...);
        }
        return AddComponentImmediate<T>(handle, std::forward<Args>(args)...);
    }
    
    template<typename T>
    void RemoveComponent(EntityHandle handle) {
        if (IsIterating()) {
            QueueDeferredRemove<T>(handle);
            return;
        }
        RemoveComponentImmediate<T>(handle);
    }
    
    template<typename T>
    bool HasComponent(EntityHandle handle) const {
        if (!IsAlive(handle)) return false;
        
        EntityIndex index = handle.Index();
        const EntityMetadata& meta = entityMetadata_[index];
        
        const Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        return archetype && archetype->HasComponentType<T>();
    }
    
    template<typename T>
    T* GetComponent(EntityHandle handle) {
        if (!IsAlive(handle)) return nullptr;
        
        EntityIndex index = handle.Index();
        if (index >= entityMetadata_.size()) return nullptr;
        
        const EntityMetadata& meta = entityMetadata_[index];
        
        Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!archetype) return nullptr;
        
        return archetype->GetComponent<T>(meta.indexInArchetype);
    }
    
    template<typename T>
    const T* GetComponent(EntityHandle handle) const {
        if (!IsAlive(handle)) return nullptr;
        
        EntityIndex index = handle.Index();
        if (index >= entityMetadata_.size()) return nullptr;
        
        const EntityMetadata& meta = entityMetadata_[index];
        
        const Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!archetype) return nullptr;
        
        return archetype->GetComponent<T>(meta.indexInArchetype);
    }
    
    // ===== Fast Iteration (Cache-Friendly) =====
    
    // ForEach with single component type
    template<typename T, typename Func>
    void ForEach(Func&& func) {
        IterationScope scope(*this);
        auto archetypes = archetypeManager_.GetArchetypesWithComponent<T>();

        for (Archetype* archetype : archetypes) {
            const auto& entities = archetype->GetEntities();
            auto* components = archetype->GetComponentVector<T>();
            
            if (!components) continue;
            
            // Iterate over contiguous arrays (cache-friendly!)
            const size_t count = entities.size();
            const EntityHandle* entityData = entities.data();
            T* componentData = components->data();
            
            // Linear traversal optimized for CPU cache prefetching
            for (size_t i = 0; i < count; ++i) {
                func(entityData[i], componentData[i]);
            }
        }
    }
    
    // ForEach with multiple component types
    template<typename T1, typename T2, typename... Ts, typename Func>
    void ForEach(Func&& func) {
        IterationScope scope(*this);
        auto archetypes = archetypeManager_.GetArchetypesWithComponents<T1, T2, Ts...>();

        for (Archetype* archetype : archetypes) {
            const auto& entities = archetype->GetEntities();
            auto* comp1 = archetype->GetComponentVector<T1>();
            auto* comp2 = archetype->GetComponentVector<T2>();
            
            if (!comp1 || !comp2) continue;
            
            // Get remaining component arrays
            std::tuple<std::vector<Ts>*...> restArrays{archetype->GetComponentVector<Ts>()...};
            
            // Check all arrays exist
            bool allExist = true;
            std::apply([&](auto*... arrays) {
                ((allExist = allExist && (arrays != nullptr)), ...);
            }, restArrays);
            
            if (!allExist) continue;
            
            // Iterate over contiguous arrays with optimized pointer access
            const size_t count = entities.size();
            const EntityHandle* entityData = entities.data();
            T1* data1 = comp1->data();
            T2* data2 = comp2->data();
            
            if constexpr (sizeof...(Ts) == 0) {
                for (size_t i = 0; i < count; ++i) {
                    func(entityData[i], data1[i], data2[i]);
                }
            } else {
                std::apply([&](auto*... arrays) {
                    for (size_t i = 0; i < count; ++i) {
                        func(entityData[i], data1[i], data2[i], (*arrays)[i]...);
                    }
                }, restArrays);
            }
        }
    }
    
    // Const versions
    template<typename T, typename Func>
    void ForEach(Func&& func) const {
        auto* self = const_cast<EntityManagerV2*>(this);
        IterationScope scope(*self);
        auto archetypes = archetypeManager_.GetArchetypesWithComponent<T>();
        
        for (const Archetype* archetype : archetypes) {
            const auto& entities = archetype->GetEntities();
            const auto* components = archetype->GetComponentVector<T>();
            
            if (!components) continue;
            
            size_t count = entities.size();
            for (size_t i = 0; i < count; ++i) {
                func(entities[i], (*components)[i]);
            }
        }
    }
    
    // ===== Batch Operations =====
    
    template<typename Func>
    void CreateEntities(size_t count, Func&& func) {
        for (size_t i = 0; i < count; ++i) {
            EntityHandle handle = CreateEntity();
            func(handle);
        }
    }
    
    template<typename... Components>
    EntityHandle CreateEntityWith(Components&&... components) {
        EntityHandle handle = CreateEntity();
        (AddComponent<Components>(handle, std::forward<Components>(components)), ...);
        return handle;
    }
    
    // ===== Statistics & Debugging =====
    
    size_t GetEntityCount() const {
        size_t count = 0;
        for (const auto& meta : entityMetadata_) {
            if (meta.alive) ++count;
        }
        return count;
    }
    
    size_t GetArchetypeCount() const {
        return archetypeManager_.GetArchetypeCount();
    }
    
    size_t GetMemoryUsage() const {
        size_t total = 0;
        total += entityMetadata_.size() * sizeof(EntityMetadata);
        total += freeIndices_.capacity() * sizeof(EntityIndex);
        total += deferredCommands_.capacity() * sizeof(std::unique_ptr<DeferredCommand>);
        return total;
    }
    
    const ArchetypeManager& GetArchetypeManager() const {
        return archetypeManager_;
    }

    bool CanProvideComponentType(const std::type_index& typeIndex) const {
        return archetypeManager_.CanProvideComponentType(typeIndex);
    }

    std::vector<std::type_index> GetComponentTypes(EntityHandle handle) const {
        std::vector<std::type_index> types;
        if (!IsAlive(handle)) {
            return types;
        }

        EntityIndex index = handle.Index();
        const EntityMetadata& meta = entityMetadata_[index];
        const Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!archetype) {
            return types;
        }

        const auto& signature = archetype->GetSignature();
        types.assign(signature.types.begin(), signature.types.end());
        return types;
    }

    void Clear() {
        FlushDeferred();
        entityMetadata_.clear();
        freeIndices_.clear();
        archetypeManager_.Clear();
    }

    void FlushDeferred() {
        if (IsIterating()) {
            return;
        }
        FlushDeferredCommands();
    }
    
private:
    // Move entity between archetypes (copy components that exist in both)
    void MoveEntityToArchetype(EntityHandle handle, Archetype* from, Archetype* to) {
        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        uint32_t oldIndex = meta.indexInArchetype;

        // Add to new archetype first (this creates space for components)
        uint32_t newIndex = static_cast<uint32_t>(to->AddEntity(handle));

        TransitionPlan plan(to, from, newIndex);
        plan.QueueEntity(oldIndex);
        plan.Execute();
        assert(to->ValidateIntegrity());

        // Remove from old archetype (this will swap-and-pop)
        EntityHandle swappedEntity = from->RemoveEntity(oldIndex);
        assert(from->ValidateIntegrity() && "Source archetype out of sync after transition");

        // Update metadata for swapped entity (if any)
        if (swappedEntity.IsValid() && swappedEntity != handle) {
            EntityIndex swappedIndex = swappedEntity.Index();
            entityMetadata_[swappedIndex].indexInArchetype = oldIndex;
        }

        // Update metadata for moved entity
        meta.archetypeId = to->GetId();
        meta.indexInArchetype = newIndex;
    }

    template<typename T, typename... Args>
    T& AddComponentImmediate(EntityHandle handle, Args&&... args) {
        if (!IsAlive(handle)) {
            throw std::runtime_error("Cannot add component to dead entity");
        }

        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!oldArchetype) {
            throw std::runtime_error("Entity's archetype is null");
        }

        if (oldArchetype->HasComponentType<T>()) {
            return *oldArchetype->GetComponent<T>(meta.indexInArchetype);
        }

        Archetype* newArchetype = archetypeManager_.GetArchetypeWithAdded<T>(
            oldArchetype->GetSignature());

        if (oldArchetype != newArchetype) {
            MoveEntityToArchetype(handle, oldArchetype, newArchetype);
        }

        T* componentPtr = newArchetype->GetComponent<T>(meta.indexInArchetype);
        if (!componentPtr) {
            throw std::runtime_error("Component not found in new archetype");
        }
        T& component = *componentPtr;
        component = T(std::forward<Args>(args)...);
        
        if (!newArchetype->ValidateIntegrity()) {
            throw std::runtime_error("Archetype integrity validation failed after adding component");
        }
        return component;
    }

    template<typename T>
    void RemoveComponentImmediate(EntityHandle handle) {
        if (!HasComponent<T>(handle)) return;

        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!oldArchetype) {
            throw std::runtime_error("Entity's archetype is null");
        }

        Archetype* newArchetype = archetypeManager_.GetArchetypeWithRemoved<T>(
            oldArchetype->GetSignature());

        MoveEntityToArchetype(handle, oldArchetype, newArchetype);
    }

    void DestroyEntityImmediate(EntityHandle handle) {
        if (!IsAlive(handle)) return;

        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (archetype) {
            EntityHandle swappedEntity = archetype->RemoveEntity(meta.indexInArchetype);
            
            if (!archetype->ValidateIntegrity()) {
                throw std::runtime_error("Archetype integrity validation failed after entity removal");
            }

            if (swappedEntity.IsValid() && swappedEntity != handle) {
                EntityIndex swappedIndex = swappedEntity.Index();
                if (swappedIndex < entityMetadata_.size()) {
                    entityMetadata_[swappedIndex].indexInArchetype = meta.indexInArchetype;
                }
            }
        }

        meta.alive = false;
        freeIndices_.push_back(index);
    }

    struct DeferredCommand {
        virtual ~DeferredCommand() = default;
        virtual void Execute(EntityManagerV2& manager) = 0;
    };

    template<typename T>
    struct DeferredAddCommand : DeferredCommand {
        EntityHandle handle;
        T component;

        template<typename... Args>
        explicit DeferredAddCommand(EntityHandle h, Args&&... args)
            : handle(h), component(std::forward<Args>(args)...) {}

        void Execute(EntityManagerV2& manager) override {
            manager.AddComponentImmediate<T>(handle, std::move(component));
        }

        T& ComponentData() { return component; }
    };

    template<typename T>
    struct DeferredRemoveCommand : DeferredCommand {
        explicit DeferredRemoveCommand(EntityHandle h) : handle(h) {}

        void Execute(EntityManagerV2& manager) override {
            manager.RemoveComponentImmediate<T>(handle);
        }

        EntityHandle handle;
    };

    struct DeferredDestroyCommand : DeferredCommand {
        explicit DeferredDestroyCommand(EntityHandle h) : handle(h) {}

        void Execute(EntityManagerV2& manager) override {
            manager.DestroyEntityImmediate(handle);
        }

        EntityHandle handle;
    };

    template<typename T, typename... Args>
    T& QueueDeferredAdd(EntityHandle handle, Args&&... args) {
        // Reserve space to prevent reallocation invalidating reference
        deferredCommands_.reserve(deferredCommands_.size() + 1);
        
        auto command = std::make_unique<DeferredAddCommand<T>>(handle, std::forward<Args>(args)...);
        T& reference = command->ComponentData();
        deferredCommands_.push_back(std::move(command));
        return reference;
    }

    template<typename T>
    void QueueDeferredRemove(EntityHandle handle) {
        deferredCommands_.push_back(std::make_unique<DeferredRemoveCommand<T>>(handle));
    }

    void QueueDeferredDestroy(EntityHandle handle) {
        deferredCommands_.push_back(std::make_unique<DeferredDestroyCommand>(handle));
    }

    bool IsIterating() const { return iterationDepth_ > 0; }

    void BeginIteration() { ++iterationDepth_; }

    void EndIteration() {
        if (iterationDepth_ == 0) return;
        --iterationDepth_;
        if (iterationDepth_ == 0) {
            FlushDeferredCommands();
        }
    }

    void FlushDeferredCommands() {
        auto commands = std::move(deferredCommands_);
        deferredCommands_.clear();
        for (auto& command : commands) {
            command->Execute(*this);
        }
    }

    class IterationScope {
    public:
        explicit IterationScope(EntityManagerV2& manager) : manager_(manager) {
            manager_.BeginIteration();
        }

        ~IterationScope() { manager_.EndIteration(); }

        IterationScope(const IterationScope&) = delete;
        IterationScope& operator=(const IterationScope&) = delete;
        IterationScope(IterationScope&&) = delete;
        IterationScope& operator=(IterationScope&&) = delete;

    private:
        EntityManagerV2& manager_;
    };
    
    std::vector<EntityMetadata> entityMetadata_;
    std::vector<EntityIndex> freeIndices_;
    ArchetypeManager archetypeManager_;
    size_t iterationDepth_ = 0;
    std::vector<std::unique_ptr<DeferredCommand>> deferredCommands_;
};

} // namespace ecs

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
    ecs::EntityHandle GetArchetypeHandle(Entity e) const;

    ecs::EntityManagerV2& GetArchetypeManager() { return archetypeManager_; }
    const ecs::EntityManagerV2& GetArchetypeManager() const { return archetypeManager_; }

    using FacadeComponentTypes = std::tuple<
        Position,
        Velocity,
        Acceleration,
        PhysicsBody,
        PhysicsMaterial,
        Transform2D,
        Sprite,
        Hitbox,
        AnimationState,
        Name,
        ViewportID,
        PlayerController,
        MovementParameters,
        MovementBounds,
        PlayerPhysics,
        PlayerVitals,
        PlayerInventory,
        PlayerProgression,
        DockingStatus,
        LocomotionStateMachine,
        TargetLock,
        ProjectileComponent,
        RigidBody,
        Force,
        Collider,
        CollisionInfo,
        GravitySource,
        ConstantForce,
        CharacterController,
        Joint,
        CameraComponent,
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

// ============================================================================
// ENTITY MANAGER FACADE (COMPATIBILITY LAYER)
// ============================================================================

namespace ecs {

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

#endif // ECS_ENTITY_MANAGER_H
