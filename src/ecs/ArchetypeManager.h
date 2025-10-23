#pragma once
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
    
    // Get all archetypes that contain specific component type
    template<typename T>
    std::vector<Archetype*> GetArchetypesWithComponent() {
        std::vector<Archetype*> result;
        std::type_index typeIndex(typeid(T));
        
        for (auto& archetype : archetypes_) {
            if (archetype->GetSignature().Contains(typeIndex)) {
                result.push_back(archetype.get());
            }
        }
        
        return result;
    }
    
    // Get all archetypes that contain ALL specified component types
    template<typename... Ts>
    std::vector<Archetype*> GetArchetypesWithComponents() {
        std::vector<Archetype*> result;
        std::vector<std::type_index> requiredTypes = {std::type_index(typeid(Ts))...};
        
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
    
    // Clear all archetypes
    void Clear() {
        archetypes_.clear();
        signatureToArchetype_.clear();
        nextArchetypeId_ = 0;
        
        // Recreate empty archetype
        CreateArchetype(ComponentSignature());
    }
    
    // Get all archetypes (for debugging/profiling)
    const std::vector<std::unique_ptr<Archetype>>& GetAllArchetypes() const {
        return archetypes_;
    }
    
private:
    Archetype* CreateArchetype(const ComponentSignature& signature) {
        uint32_t id = nextArchetypeId_++;
        auto archetype = std::make_unique<Archetype>(id, signature);
        Archetype* ptr = archetype.get();
        
        // Register component arrays for all types in signature
        for (const auto& typeIndex : signature.types) {
            RegisterComponentArrayForType(ptr, typeIndex);
        }
        
        archetypes_.push_back(std::move(archetype));
        signatureToArchetype_[signature] = id;
        
        return ptr;
    }
    
    // Type-erased component array registration
    void RegisterComponentArrayForType(Archetype* archetype, const std::type_index& typeIndex);

    std::vector<std::unique_ptr<Archetype>> archetypes_;
    std::unordered_map<ComponentSignature, uint32_t> signatureToArchetype_;
    uint32_t nextArchetypeId_ = 0;
    std::unordered_set<std::type_index> registeredComponentTypes_;
};

} // namespace ecs
