#pragma once
#include "ArchetypeManager.h"
#include "EntityHandle.h"
#include "TransitionPlan.h"
#include <cassert>
#include <typeindex>
#include <vector>

#include <memory>
#include <utility>

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
            
            // Increment generation to invalidate old handles
            generation = ++entityMetadata_[index].generation;
        } else {
            // Allocate new entity slot
            index = static_cast<EntityIndex>(entityMetadata_.size());
            generation = 0;
            entityMetadata_.emplace_back(generation, 0, 0);  // Empty archetype (ID 0)
        }
        
        EntityHandle handle(index, generation);
        EntityMetadata& meta = entityMetadata_[index];
        meta.alive = true;
        meta.archetypeId = 0;  // Start in empty archetype
        
        // Add to empty archetype
        Archetype* archetype = archetypeManager_.GetArchetype(0);
        assert(archetype != nullptr && "Empty archetype must exist");
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
        const EntityMetadata& meta = entityMetadata_[index];
        
        Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (!archetype) return nullptr;
        
        return archetype->GetComponent<T>(meta.indexInArchetype);
    }
    
    template<typename T>
    const T* GetComponent(EntityHandle handle) const {
        if (!IsAlive(handle)) return nullptr;
        
        EntityIndex index = handle.Index();
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
            size_t count = entities.size();
            for (size_t i = 0; i < count; ++i) {
                func(entities[i], (*components)[i]);
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
            
            // Iterate over contiguous arrays
            size_t count = entities.size();
            for (size_t i = 0; i < count; ++i) {
                if constexpr (sizeof...(Ts) == 0) {
                    func(entities[i], (*comp1)[i], (*comp2)[i]);
                } else {
                    std::apply([&](auto*... arrays) {
                        func(entities[i], (*comp1)[i], (*comp2)[i], (*arrays)[i]...);
                    }, restArrays);
                }
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

        TransitionPlan plan(to, from);
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
        assert(IsAlive(handle) && "Cannot add component to dead entity");

        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        assert(oldArchetype != nullptr && "Entity's archetype is null");

        if (oldArchetype->HasComponentType<T>()) {
            return *oldArchetype->GetComponent<T>(meta.indexInArchetype);
        }

        Archetype* newArchetype = archetypeManager_.GetArchetypeWithAdded<T>(
            oldArchetype->GetSignature());

        if (oldArchetype != newArchetype) {
            MoveEntityToArchetype(handle, oldArchetype, newArchetype);
        }

        T& component = newArchetype->EmplaceComponent<T>(meta.indexInArchetype,
                                                         std::forward<Args>(args)...);
        assert(newArchetype->ValidateIntegrity());
        return component;
    }

    template<typename T>
    void RemoveComponentImmediate(EntityHandle handle) {
        if (!HasComponent<T>(handle)) return;

        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];

        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        assert(oldArchetype != nullptr && "Entity's archetype is null");

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
            assert(archetype->ValidateIntegrity());

            if (swappedEntity.IsValid() && swappedEntity != handle) {
                EntityIndex swappedIndex = swappedEntity.Index();
                entityMetadata_[swappedIndex].indexInArchetype = meta.indexInArchetype;
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
        assert(iterationDepth_ > 0);
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
