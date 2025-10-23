#pragma once
#include "ArchetypeManager.h"
#include "EntityHandle.h"
#include <cassert>
#include <typeindex>
#include <vector>

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
        if (!IsAlive(handle)) return;
        
        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];
        
        // Remove from archetype
        Archetype* archetype = archetypeManager_.GetArchetype(meta.archetypeId);
        if (archetype) {
            EntityHandle swappedEntity = archetype->RemoveEntity(meta.indexInArchetype);
            
            // Update metadata for swapped entity (if any)
            if (swappedEntity.IsValid() && swappedEntity != handle) {
                EntityIndex swappedIndex = swappedEntity.Index();
                entityMetadata_[swappedIndex].indexInArchetype = meta.indexInArchetype;
            }
        }
        
        // Mark as dead and add to free list
        meta.alive = false;
        freeIndices_.push_back(index);
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
        assert(IsAlive(handle) && "Cannot add component to dead entity");
        
        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];
        
        // Get current archetype
        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        assert(oldArchetype != nullptr && "Entity's archetype is null");
        
        // Check if component already exists
        if (oldArchetype->HasComponentType<T>()) {
            return *oldArchetype->GetComponent<T>(meta.indexInArchetype);
        }
        
        // Find/create new archetype with component added
        Archetype* newArchetype = archetypeManager_.GetArchetypeWithAdded<T>(
            oldArchetype->GetSignature()
        );
        
        // Move entity to new archetype if different
        if (oldArchetype != newArchetype) {
            MoveEntityToArchetype(handle, oldArchetype, newArchetype);
        }
        
        // Add component to new archetype
        T& component = newArchetype->EmplaceComponent<T>(meta.indexInArchetype, 
                                                         std::forward<Args>(args)...);
        return component;
    }
    
    template<typename T>
    void RemoveComponent(EntityHandle handle) {
        if (!HasComponent<T>(handle)) return;
        
        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];
        
        // Get current archetype
        Archetype* oldArchetype = archetypeManager_.GetArchetype(meta.archetypeId);
        assert(oldArchetype != nullptr && "Entity's archetype is null");
        
        // Find/create new archetype with component removed
        Archetype* newArchetype = archetypeManager_.GetArchetypeWithRemoved<T>(
            oldArchetype->GetSignature()
        );
        
        // Move entity to new archetype
        MoveEntityToArchetype(handle, oldArchetype, newArchetype);
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

    void Clear() {
        entityMetadata_.clear();
        freeIndices_.clear();
        archetypeManager_.Clear();
    }
    
private:
    // Move entity between archetypes (copy components that exist in both)
    void MoveEntityToArchetype(EntityHandle handle, Archetype* from, Archetype* to) {
        EntityIndex index = handle.Index();
        EntityMetadata& meta = entityMetadata_[index];
        
        uint32_t oldIndex = meta.indexInArchetype;
        
        // Add to new archetype first (this creates space for components)
        uint32_t newIndex = static_cast<uint32_t>(to->AddEntity(handle));
        
        // Copy components that exist in both archetypes
        const auto& fromSig = from->GetSignature();
        const auto& toSig = to->GetSignature();
        
        for (const auto& typeIndex : fromSig.types) {
            if (toSig.Contains(typeIndex)) {
                // Copy this component from old archetype to new archetype
                to->CopyComponentFrom(from, oldIndex, typeIndex);
            }
        }
        
        // Remove from old archetype (this will swap-and-pop)
        EntityHandle swappedEntity = from->RemoveEntity(oldIndex);
        
        // Update metadata for swapped entity (if any)
        if (swappedEntity.IsValid() && swappedEntity != handle) {
            EntityIndex swappedIndex = swappedEntity.Index();
            entityMetadata_[swappedIndex].indexInArchetype = oldIndex;
        }
        
        // Update metadata for moved entity
        meta.archetypeId = to->GetId();
        meta.indexInArchetype = newIndex;
    }
    
    std::vector<EntityMetadata> entityMetadata_;
    std::vector<EntityIndex> freeIndices_;
    ArchetypeManager archetypeManager_;
};

} // namespace ecs
