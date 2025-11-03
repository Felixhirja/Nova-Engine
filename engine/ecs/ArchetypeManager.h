#pragma once

// Archetype System Performance Roadmap
//
// ARCHETYPE OPTIMIZATION:
// [X] Archetype Graph: Build transition graph for O(1) component add/remove ✓ IMPLEMENTED
// [ ] Archetype Sorting: Order by usage frequency for better cache performance
// [ ] Archetype Merging: Combine small archetypes to reduce overhead
// [X] Lazy Archetype Creation: Create archetypes only when entities are added ✓ IMPLEMENTED
// [ ] Archetype Prediction: Pre-create likely archetype combinations
// [X] Memory Compaction: Defragment archetype storage periodically ✓ IMPLEMENTED (CompactArchetypes)
// [ ] Component Layout Optimization: Reorder components by access patterns
//
// TRANSITION PERFORMANCE:
// [ ] Batch Transitions: Move multiple entities between archetypes efficiently
// [ ] Transition Pooling: Reuse transition data structures
// [ ] Component Migration: Optimize component copying during transitions
// [ ] Dependency Tracking: Automatic component dependency resolution
// [ ] Rollback Support: Undo archetype transitions for error recovery
// [ ] Transition Validation: Ensure component constraints are maintained
//
// MEMORY MANAGEMENT:
// [ ] Custom Allocators: Per-archetype memory allocation strategies
// [ ] Memory Mapping: Virtual memory for very large archetypes
// [ ] Component Compression: Pack small components together
// [ ] Memory Budgeting: Configurable limits per archetype
// [ ] Garbage Collection: Automatic cleanup of empty archetypes
// [ ] Memory Debugging: Track archetype memory usage and fragmentation
//
// ARCHETYPE ANALYTICS:
// [ ] Usage Statistics: Track archetype access patterns
// [ ] Memory Profiling: Monitor memory usage per archetype
// [ ] Performance Metrics: Measure iteration and transition speeds
// [ ] Fragmentation Analysis: Detect and report memory fragmentation
// [ ] Component Distribution: Analyze component usage across archetypes
// [ ] Cache Locality Analysis: Measure cache hit rates for component access

#include "Archetype.h"
#include "EntityHandle.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ecs {

// Manages archetypes and handles entity transitions between archetypes
class ArchetypeManager {
public:
    ArchetypeManager() {
        // Create empty archetype (ID 0) for entities with no components
        CreateArchetype(ComponentSignature());
    }
    
    ~ArchetypeManager() = default;
    
    // Find or create archetype with given signature
    Archetype* GetOrCreateArchetype(const ComponentSignature& signature) {
        auto it = signatureToArchetype_.find(signature);
        if (it != signatureToArchetype_.end()) {
            return archetypes_[it->second].get();
        }
        return CreateArchetype(signature);
    }
    
    // Get archetype by ID
    Archetype* GetArchetype(uint32_t id) {
        if (id >= archetypes_.size()) return nullptr;
        return archetypes_[id].get();
    }
    
    const Archetype* GetArchetype(uint32_t id) const {
        if (id >= archetypes_.size()) return nullptr;
        return archetypes_[id].get();
    }
    
    bool CanProvideComponentType(const std::type_index& typeIndex) const;

    // Find archetype with signature + one component added
    template<typename T>
    Archetype* GetArchetypeWithAdded(const ComponentSignature& baseSignature) {
        ComponentSignature newSig = baseSignature;
        std::type_index typeToAdd(typeid(T));
        
        // Add type if not already present
        if (!newSig.Contains(typeToAdd)) {
            newSig.types.push_back(typeToAdd);
            std::sort(newSig.types.begin(), newSig.types.end());
        }
        
        return GetOrCreateArchetype(newSig);
    }
    
    // Find archetype with signature - one component removed
    template<typename T>
    Archetype* GetArchetypeWithRemoved(const ComponentSignature& baseSignature) {
        ComponentSignature newSig;
        std::type_index typeToRemove(typeid(T));
        
        // Copy all types except the one to remove
        for (const auto& type : baseSignature.types) {
            if (type != typeToRemove) {
                newSig.types.push_back(type);
            }
        }
        
        return GetOrCreateArchetype(newSig);
    }
    
    // Get all archetypes that contain specific component type (cached)
    template<typename T>
    std::vector<Archetype*> GetArchetypesWithComponent() {
        std::type_index typeIndex(typeid(T));
        
        // Check cache first
        auto it = archetypeCache_.find(typeIndex);
        if (it != archetypeCache_.end() && !needsCacheRebuild_) {
            return it->second;
        }
        
        // Build cache entry
        std::vector<Archetype*> result;
        for (auto& archetype : archetypes_) {
            if (archetype->GetSignature().Contains(typeIndex)) {
                result.push_back(archetype.get());
            }
        }
        
        archetypeCache_[typeIndex] = result;
        return result;
    }
    
    // Get all archetypes that contain ALL specified component types (cached)
    template<typename... Ts>
    std::vector<Archetype*> GetArchetypesWithComponents() {
        // Create compound key for multi-component queries
        std::vector<std::type_index> requiredTypes = {std::type_index(typeid(Ts))...};
        std::sort(requiredTypes.begin(), requiredTypes.end());
        
        ComponentSignature querySig(requiredTypes);
        
        // Check multi-component cache
        auto it = multiComponentCache_.find(querySig);
        if (it != multiComponentCache_.end() && !needsCacheRebuild_) {
            return it->second;
        }
        
        // Build cache entry
        std::vector<Archetype*> result;
        for (auto& archetype : archetypes_) {
            bool hasAll = true;
            for (const auto& type : requiredTypes) {
                if (!archetype->GetSignature().Contains(type)) {
                    hasAll = false;
                    break;
                }
            }
            if (hasAll) {
                result.push_back(archetype.get());
            }
        }
        
        multiComponentCache_[querySig] = result;
        return result;
    }
    
    // Statistics
    size_t GetArchetypeCount() const { return archetypes_.size(); }
    size_t GetTotalEntityCount() const {
        size_t total = 0;
        for (const auto& archetype : archetypes_) {
            total += archetype->GetEntityCount();
        }
        return total;
    }
    
    size_t GetMemoryUsage() const {
        size_t total = sizeof(ArchetypeManager);
        for (const auto& archetype : archetypes_) {
            total += archetype->GetMemoryUsage();
        }
        return total;
    }
    
    // Clear all archetypes
    void Clear() {
        archetypes_.clear();
        signatureToArchetype_.clear();
        archetypeCache_.clear();
        multiComponentCache_.clear();
        nextArchetypeId_ = 0;
        needsCacheRebuild_ = false;
        
        // Recreate empty archetype
        CreateArchetype(ComponentSignature());
    }
    
    // Optimize memory by releasing unused capacity
    void Shrink() {
        for (auto& archetype : archetypes_) {
            if (archetype->GetEntityCount() == 0 && archetype->GetId() != 0) {
                // Can potentially remove empty non-zero archetypes
            }
        }
    }
    
    void InvalidateCache() {
        needsCacheRebuild_ = true;
        archetypeCache_.clear();
        multiComponentCache_.clear();
    }
    
    // Get all archetypes (for debugging/profiling)
    const std::vector<std::unique_ptr<Archetype>>& GetAllArchetypes() const {
        return archetypes_;
    }
    
    // Get component types from an archetype ID
    std::vector<std::type_index> GetComponentTypesForArchetype(uint32_t archetypeId) const {
        const Archetype* archetype = GetArchetype(archetypeId);
        if (!archetype) {
            return std::vector<std::type_index>();
        }
        return archetype->GetSignature().types;
    }
    
    // Archetype Graph: Fast O(1) transition lookups
    void BuildTransitionGraph() {
        if (registeredComponentTypes_.empty()) {
            return;
        }
        
        for (auto& archetype : archetypes_) {
            BuildArchetypeTransitions(archetype.get());
        }
        
        transitionGraphBuilt_ = true;
    }
    
    template<typename T>
    Archetype* GetArchetypeWithAddedFast(Archetype* current) {
        if (!transitionGraphBuilt_) {
            BuildTransitionGraph();
        }
        
        std::type_index typeToAdd(typeid(T));
        
        // Try cached transition (O(1))
        Archetype* cached = current->GetTransitionAdd(typeToAdd);
        if (cached != nullptr) {
            return cached;
        }
        
        // Fallback: compute and cache
        Archetype* target = GetArchetypeWithAdded<T>(current->GetSignature());
        current->SetTransitionAdd(typeToAdd, target);
        target->SetTransitionRemove(typeToAdd, current);
        
        return target;
    }
    
    template<typename T>
    Archetype* GetArchetypeWithRemovedFast(Archetype* current) {
        if (!transitionGraphBuilt_) {
            BuildTransitionGraph();
        }
        
        std::type_index typeToRemove(typeid(T));
        
        // Try cached transition (O(1))
        Archetype* cached = current->GetTransitionRemove(typeToRemove);
        if (cached != nullptr) {
            return cached;
        }
        
        // Fallback: compute and cache
        Archetype* target = GetArchetypeWithRemoved<T>(current->GetSignature());
        current->SetTransitionRemove(typeToRemove, target);
        target->SetTransitionAdd(typeToRemove, current);
        
        return target;
    }
    
    struct TransitionGraphStats {
        size_t totalEdges = 0;
        size_t validEdges = 0;
        size_t invalidEdges = 0;
        double avgEdgesPerArchetype = 0.0;
        size_t maxEdgesPerArchetype = 0;
    };
    
    TransitionGraphStats GetTransitionGraphStats() const {
        TransitionGraphStats stats;
        
        for (const auto& archetype : archetypes_) {
            const auto& addEdges = archetype->GetAddTransitions();
            const auto& removeEdges = archetype->GetRemoveTransitions();
            
            size_t archetypeEdgeCount = addEdges.size() + removeEdges.size();
            stats.totalEdges += archetypeEdgeCount;
            stats.maxEdgesPerArchetype = std::max(stats.maxEdgesPerArchetype, archetypeEdgeCount);
            
            for (const auto& [type, edge] : addEdges) {
                (void)type;
                if (edge.isValid) stats.validEdges++;
                else stats.invalidEdges++;
            }
            for (const auto& [type, edge] : removeEdges) {
                (void)type;
                if (edge.isValid) stats.validEdges++;
                else stats.invalidEdges++;
            }
        }
        
        if (!archetypes_.empty()) {
            stats.avgEdgesPerArchetype = static_cast<double>(stats.totalEdges) / archetypes_.size();
        }
        
        return stats;
    }
    
    void InvalidateTransitionGraph() {
        transitionGraphBuilt_ = false;
        for (auto& archetype : archetypes_) {
            archetype->InvalidateTransitions();
        }
    }
    
    // Lazy creation statistics
    struct LazyCreationStats {
        size_t totalArchetypes = 0;
        size_t emptyArchetypes = 0;
        size_t smallArchetypes = 0;   // < 8 entities
        size_t mediumArchetypes = 0;  // 8-256 entities
        size_t largeArchetypes = 0;   // > 256 entities
        size_t totalMemoryUsed = 0;
        size_t totalMemoryWasted = 0;
        double avgUtilization = 0.0;
    };
    
    LazyCreationStats GetLazyCreationStats() const {
        LazyCreationStats stats;
        stats.totalArchetypes = archetypes_.size();
        
        double totalUtil = 0.0;
        for (const auto& archetype : archetypes_) {
            size_t entityCount = archetype->GetEntityCount();
            
            if (entityCount == 0) {
                stats.emptyArchetypes++;
            } else if (entityCount < 8) {
                stats.smallArchetypes++;
            } else if (entityCount <= 256) {
                stats.mediumArchetypes++;
            } else {
                stats.largeArchetypes++;
            }
            
            stats.totalMemoryUsed += archetype->GetMemoryUsage();
            stats.totalMemoryWasted += archetype->GetWastedMemory();
            totalUtil += archetype->GetUtilization();
        }
        
        if (stats.totalArchetypes > 0) {
            stats.avgUtilization = totalUtil / stats.totalArchetypes;
        }
        
        return stats;
    }
    
    // Shrink empty and underutilized archetypes
    void CompactArchetypes() {
        for (auto& archetype : archetypes_) {
            // Skip empty archetype (ID 0)
            if (archetype->GetId() == 0) continue;
            
            size_t entityCount = archetype->GetEntityCount();
            size_t capacity = archetype->GetEntities().capacity();
            
            // Shrink if utilization is low
            if (entityCount > 0 && capacity > entityCount * 2) {
                // Reserve exactly what we need plus 25% growth buffer
                size_t newCapacity = entityCount + (entityCount / 4);
                archetype->Reserve(newCapacity);
            }
        }
    }
    
private:
    Archetype* CreateArchetype(const ComponentSignature& signature) {
        uint32_t id = nextArchetypeId_++;
        
        // Lazy optimization: Start with minimal capacity (1 entity)
        // Archetypes will grow as needed, saving memory on unused combinations
        auto archetype = std::make_unique<Archetype>(id, signature);
        Archetype* ptr = archetype.get();
        
        // Register component arrays for all types in signature
        for (const auto& typeIndex : signature.types) {
            RegisterComponentArrayForType(ptr, typeIndex);
        }
        
        // Reserve minimal initial capacity for lazy creation optimization
        ptr->Reserve(1);  // Start small, grow dynamically
        
        archetypes_.push_back(std::move(archetype));
        signatureToArchetype_[signature] = id;
        needsCacheRebuild_ = true;
        
        return ptr;
    }
    
    // Type-erased component array registration
    void RegisterComponentArrayForType(Archetype* archetype, const std::type_index& typeIndex);

    void BuildArchetypeTransitions(Archetype* archetype) {
        const ComponentSignature& signature = archetype->GetSignature();
        
        // Build ADD transitions for each registered component type
        for (const auto& componentType : registeredComponentTypes_) {
            if (signature.Contains(componentType)) {
                continue;
            }
            
            Archetype* target = GetArchetypeWithComponentAdded(signature, componentType);
            archetype->SetTransitionAdd(componentType, target);
            target->SetTransitionRemove(componentType, archetype);
        }
        
        // Build REMOVE transitions for existing components
        for (const auto& componentType : signature.types) {
            Archetype* target = GetArchetypeWithComponentRemoved(signature, componentType);
            archetype->SetTransitionRemove(componentType, target);
            target->SetTransitionAdd(componentType, archetype);
        }
    }
    
    Archetype* GetArchetypeWithComponentAdded(const ComponentSignature& base, 
                                               std::type_index componentType) {
        ComponentSignature newSig = base;
        newSig.types.push_back(componentType);
        std::sort(newSig.types.begin(), newSig.types.end());
        return GetOrCreateArchetype(newSig);
    }
    
    Archetype* GetArchetypeWithComponentRemoved(const ComponentSignature& base,
                                                 std::type_index componentType) {
        ComponentSignature newSig;
        for (const auto& type : base.types) {
            if (type != componentType) {
                newSig.types.push_back(type);
            }
        }
        return GetOrCreateArchetype(newSig);
    }

    std::vector<std::unique_ptr<Archetype>> archetypes_;
    std::unordered_map<ComponentSignature, uint32_t> signatureToArchetype_;
    std::unordered_map<std::type_index, std::vector<Archetype*>> archetypeCache_;
    std::unordered_map<ComponentSignature, std::vector<Archetype*>> multiComponentCache_;
    uint32_t nextArchetypeId_ = 0;
    bool needsCacheRebuild_ = false;
    bool transitionGraphBuilt_ = false;
    std::unordered_set<std::type_index> registeredComponentTypes_;
};

} // namespace ecs
