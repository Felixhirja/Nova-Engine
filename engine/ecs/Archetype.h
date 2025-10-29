#pragma once
#include "EntityHandle.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "ComponentTraits.h"

namespace ecs {

// Type-erased component array storage
class ComponentArray {
public:
    virtual ~ComponentArray() = default;
    virtual void* Get(size_t index) = 0;
    virtual const void* Get(size_t index) const = 0;
    virtual void RemoveAndSwap(size_t index) = 0;
    virtual void Reserve(size_t capacity) = 0;
    virtual size_t Size() const = 0;
    virtual size_t Capacity() const = 0;
    virtual void Clear() = 0;
    virtual std::unique_ptr<ComponentArray> Clone() const = 0;

    // Copy component from another array (for archetype transitions)
    virtual void CopyFrom(const ComponentArray* src, size_t srcIndex) = 0;
    virtual void CopyBlockFrom(const ComponentArray* src, size_t srcIndex, size_t count) = 0;
    virtual void CopyToIndex(const ComponentArray* src, size_t srcIndex, size_t dstIndex) = 0;
    virtual void CopyBlockToIndex(const ComponentArray* src, size_t srcIndex, size_t count, size_t dstIndex) = 0;
    
    // Resize array to specific size with default-constructed elements
    virtual void Resize(size_t newSize) = 0;

    virtual bool IsTriviallyCopyable() const = 0;
    virtual size_t ElementSize() const = 0;
};

// Typed component storage - contiguous array (SoA layout)
template<typename T>
class TypedComponentArray : public ComponentArray {
public:
    TypedComponentArray() { components_.reserve(64); }
    ~TypedComponentArray() override = default;
    
    void* Get(size_t index) override {
        assert(index < components_.size() && "Component index out of bounds");
        return &components_[index];
    }
    
    const void* Get(size_t index) const override {
        assert(index < components_.size() && "Component index out of bounds");
        return &components_[index];
    }
    
    T& GetTyped(size_t index) {
        assert(index < components_.size() && "Component index out of bounds");
        return components_[index];
    }
    
    const T& GetTyped(size_t index) const {
        assert(index < components_.size() && "Component index out of bounds");
        return components_[index];
    }
    
    // Add component using move semantics (no copy)
    template<typename... Args>
    T& Emplace(Args&&... args) {
        return components_.emplace_back(std::forward<Args>(args)...);
    }
    
    // Remove component at index and swap with last (O(1) removal)
    void RemoveAndSwap(size_t index) override {
        assert(index < components_.size() && "Component index out of bounds");
        if (index < components_.size() - 1) {
            components_[index] = std::move(components_.back());
        }
        components_.pop_back();
    }
    
    void Reserve(size_t capacity) override {
        components_.reserve(capacity);
    }
    
    size_t Size() const override {
        return components_.size();
    }
    
    size_t Capacity() const override {
        return components_.capacity();
    }
    
    void Clear() override {
        components_.clear();
    }

    std::unique_ptr<ComponentArray> Clone() const override {
        auto clone = std::make_unique<TypedComponentArray<T>>();
        clone->components_ = components_;
        return clone;
    }

    // Copy component from another array (for archetype transitions)
    void CopyFrom(const ComponentArray* src, size_t srcIndex) override {
        const TypedComponentArray<T>* typedSrc = static_cast<const TypedComponentArray<T>*>(src);
        assert(typedSrc != nullptr && "Source array type mismatch");
        assert(srcIndex < typedSrc->Size() && "Source index out of bounds");
        AppendRange(&typedSrc->GetTyped(srcIndex), 1);
    }

    void CopyBlockFrom(const ComponentArray* src, size_t srcIndex, size_t count) override {
        const TypedComponentArray<T>* typedSrc = static_cast<const TypedComponentArray<T>*>(src);
        assert(typedSrc != nullptr && "Source array type mismatch");
        assert(srcIndex + count <= typedSrc->Size() && "Source range out of bounds");
        AppendRange(&typedSrc->components_[srcIndex], count);
    }

    void CopyToIndex(const ComponentArray* src, size_t srcIndex, size_t dstIndex) override {
        const TypedComponentArray<T>* typedSrc = static_cast<const TypedComponentArray<T>*>(src);
        assert(typedSrc != nullptr && "Source array type mismatch");
        assert(srcIndex < typedSrc->Size() && "Source index out of bounds");
        assert(dstIndex < components_.size() && "Destination index out of bounds");
        components_[dstIndex] = typedSrc->GetTyped(srcIndex);
    }

    void CopyBlockToIndex(const ComponentArray* src, size_t srcIndex, size_t count, size_t dstIndex) override {
        const TypedComponentArray<T>* typedSrc = static_cast<const TypedComponentArray<T>*>(src);
        assert(typedSrc != nullptr && "Source array type mismatch");
        assert(srcIndex + count <= typedSrc->Size() && "Source range out of bounds");
        assert(dstIndex + count <= components_.size() && "Destination range out of bounds");
        
        for (size_t i = 0; i < count; ++i) {
            components_[dstIndex + i] = typedSrc->components_[srcIndex + i];
        }
    }

    void Resize(size_t newSize) override {
        components_.resize(newSize);
    }

    // Direct access to underlying vector for fast iteration
    std::vector<T>& GetVector() { return components_; }
    const std::vector<T>& GetVector() const { return components_; }

    bool IsTriviallyCopyable() const override {
        return IsTriviallyRelocatable<T>;
    }

    size_t ElementSize() const override { return sizeof(T); }

private:
    void AppendRange(const T* data, size_t count) {
        if (count == 0) {
            return;
        }
        size_t oldSize = components_.size();
        components_.resize(oldSize + count);
        T* dst = components_.data() + oldSize;
        ComponentTraits<T>::CopyRange(dst, data, count);
    }

    std::vector<T> components_;  // Contiguous storage for cache locality
};

// Component type signature (sorted type indices)
struct ComponentSignature {
    std::vector<std::type_index> types;
    
    ComponentSignature() = default;
    
    explicit ComponentSignature(std::vector<std::type_index> typeList)
        : types(std::move(typeList)) {
        std::sort(types.begin(), types.end());
    }
    
    template<typename... Ts>
    static ComponentSignature Create() {
        return ComponentSignature({std::type_index(typeid(Ts))...});
    }
    
    bool operator==(const ComponentSignature& other) const {
        return types == other.types;
    }
    
    bool operator<(const ComponentSignature& other) const {
        return types < other.types;
    }
    
    size_t Hash() const {
        size_t hash = 0;
        for (const auto& type : types) {
            hash ^= type.hash_code() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
    
    bool Contains(std::type_index type) const {
        return std::binary_search(types.begin(), types.end(), type);
    }
    
    size_t Size() const { return types.size(); }
};

} // namespace ecs

// Hash support for ComponentSignature
namespace std {
template<>
struct hash<ecs::ComponentSignature> {
    size_t operator()(const ecs::ComponentSignature& sig) const {
        return sig.Hash();
    }
};
} // namespace std

namespace ecs {

// Archetype: stores entities with identical component signatures
// All entities in an archetype have the same set of component types
// Components stored in separate contiguous arrays (Structure of Arrays)
class Archetype {
public:
    explicit Archetype(uint32_t id, ComponentSignature signature)
        : id_(id), signature_(std::move(signature)) {
        entities_.reserve(64);
    }
    
    ~Archetype() = default;
    
    // Accessors
    uint32_t GetId() const { return id_; }
    const ComponentSignature& GetSignature() const { return signature_; }
    size_t GetEntityCount() const { return entities_.size(); }
    
    // Entity management
    EntityHandle GetEntity(size_t index) const {
        assert(index < entities_.size() && "Entity index out of bounds");
        return entities_[index];
    }
    
    // Add entity and return its index in this archetype
    size_t AddEntity(EntityHandle entity) {
        size_t index = entities_.size();
        entities_.push_back(entity);
        
        // Ensure all component arrays are sized to match entity count
        for (auto& [typeIndex, array] : componentArrays_) {
            array->Resize(entities_.size());
        }
        
        return index;
    }
    
    // Remove entity at index (swap with last)
    // Returns the entity that was swapped (for updating metadata)
    EntityHandle RemoveEntity(size_t index) {
        assert(index < entities_.size() && "Entity index out of bounds");
        
        EntityHandle swapped = EntityHandle::Null();

        // Swap with last entity (if not already last)
        if (index < entities_.size() - 1) {
            swapped = entities_.back();
            entities_[index] = swapped;
            
            // Swap component data for all component types
            for (auto& [typeIndex, array] : componentArrays_) {
                array->RemoveAndSwap(index);
            }
        } else {
            // Just remove from all arrays
            for (auto& [typeIndex, array] : componentArrays_) {
                array->RemoveAndSwap(index);
            }
        }
        
        entities_.pop_back();
        return swapped;
    }
    
    // Component array management
    template<typename T>
    void RegisterComponentType() {
        std::type_index typeIndex(typeid(T));
        if (componentArrays_.find(typeIndex) == componentArrays_.end()) {
            componentArrays_[typeIndex] = std::make_unique<TypedComponentArray<T>>();
        }
    }
    
    template<typename T>
    bool HasComponentType() const {
        return signature_.Contains(std::type_index(typeid(T)));
    }
    
    template<typename T>
    TypedComponentArray<T>* GetComponentArray() {
        std::type_index typeIndex(typeid(T));
        auto it = componentArrays_.find(typeIndex);
        if (it == componentArrays_.end()) return nullptr;
        return static_cast<TypedComponentArray<T>*>(it->second.get());
    }
    
    template<typename T>
    const TypedComponentArray<T>* GetComponentArray() const {
        std::type_index typeIndex(typeid(T));
        auto it = componentArrays_.find(typeIndex);
        if (it == componentArrays_.end()) return nullptr;
        return static_cast<const TypedComponentArray<T>*>(it->second.get());
    }
    
    // Add component for entity at index
    template<typename T, typename... Args>
    T& EmplaceComponent([[maybe_unused]] size_t entityIndex, Args&&... args) {
        auto* array = GetComponentArray<T>();
        assert(array != nullptr && "Component type not registered in archetype");
        // Component array should grow to match entity index
        return array->Emplace(std::forward<Args>(args)...);
    }
    
    // Get component for entity at index
    template<typename T>
    T* GetComponent(size_t entityIndex) {
        auto* array = GetComponentArray<T>();
        if (!array || entityIndex >= array->Size()) return nullptr;
        return &array->GetTyped(entityIndex);
    }
    
    template<typename T>
    const T* GetComponent(size_t entityIndex) const {
        auto* array = GetComponentArray<T>();
        if (!array || entityIndex >= array->Size()) return nullptr;
        return &array->GetTyped(entityIndex);
    }
    
    // Reserve capacity for entities and components
    void Reserve(size_t capacity) {
        entities_.reserve(capacity);
        for (auto& [typeIndex, array] : componentArrays_) {
            array->Reserve(capacity);
        }
    }
    
    // Clear all entities and components
    void Clear() {
        entities_.clear();
        for (auto& [typeIndex, array] : componentArrays_) {
            array->Clear();
        }
    }
    
    // Iteration support - get all component arrays for fast ForEach
    template<typename T>
    std::vector<T>* GetComponentVector() {
        auto* array = GetComponentArray<T>();
        return array ? &array->GetVector() : nullptr;
    }
    
    template<typename T>
    const std::vector<T>* GetComponentVector() const {
        auto* array = GetComponentArray<T>();
        return array ? &array->GetVector() : nullptr;
    }
    
    const std::vector<EntityHandle>& GetEntities() const {
        return entities_;
    }

    bool ValidateIntegrity() const {
        const size_t entityCount = entities_.size();
        for (const auto& [typeIndex, array] : componentArrays_) {
            (void)typeIndex;
            if (array->Size() != entityCount) {
                return false;
            }
        }
        return true;
    }

    // Copy component from another archetype (for archetype transitions)
    // Returns true if the component type exists in both archetypes and was copied
    bool CopyComponentFrom(const Archetype* srcArchetype, size_t srcIndex,
                          std::type_index typeIndex) {
        // Check if both archetypes have this component type
        auto srcIt = srcArchetype->componentArrays_.find(typeIndex);
        auto dstIt = componentArrays_.find(typeIndex);

        if (srcIt == srcArchetype->componentArrays_.end() ||
            dstIt == componentArrays_.end()) {
            return false;
        }

        // Copy the component
        dstIt->second->CopyFrom(srcIt->second.get(), srcIndex);
        return true;
    }

    void CopyComponentBlockFrom(const Archetype* srcArchetype, size_t srcIndex,
                                size_t count, std::type_index typeIndex, bool useBlockCopy) {
        auto srcIt = srcArchetype->componentArrays_.find(typeIndex);
        auto dstIt = componentArrays_.find(typeIndex);
        if (srcIt == srcArchetype->componentArrays_.end() ||
            dstIt == componentArrays_.end()) {
            return;
        }

        if (useBlockCopy) {
            dstIt->second->CopyBlockFrom(srcIt->second.get(), srcIndex, count);
        } else {
            for (size_t i = 0; i < count; ++i) {
                dstIt->second->CopyFrom(srcIt->second.get(), srcIndex + i);
            }
        }
    }

    void CopyComponentToIndex(const Archetype* srcArchetype, size_t srcIndex,
                              std::type_index typeIndex, size_t dstIndex) {
        auto srcIt = srcArchetype->componentArrays_.find(typeIndex);
        auto dstIt = componentArrays_.find(typeIndex);
        if (srcIt == srcArchetype->componentArrays_.end() ||
            dstIt == componentArrays_.end()) {
            return;
        }

        dstIt->second->CopyToIndex(srcIt->second.get(), srcIndex, dstIndex);
    }

    void CopyComponentBlockToIndex(const Archetype* srcArchetype, size_t srcIndex,
                                   size_t count, std::type_index typeIndex, size_t dstIndex) {
        auto srcIt = srcArchetype->componentArrays_.find(typeIndex);
        auto dstIt = componentArrays_.find(typeIndex);
        if (srcIt == srcArchetype->componentArrays_.end() ||
            dstIt == componentArrays_.end()) {
            return;
        }

        dstIt->second->CopyBlockToIndex(srcIt->second.get(), srcIndex, count, dstIndex);
    }

    const ComponentArray* GetComponentArrayRaw(std::type_index typeIndex) const {
        auto it = componentArrays_.find(typeIndex);
        return it != componentArrays_.end() ? it->second.get() : nullptr;
    }

    ComponentArray* GetComponentArrayRaw(std::type_index typeIndex) {
        auto it = componentArrays_.find(typeIndex);
        return it != componentArrays_.end() ? it->second.get() : nullptr;
    }

private:
    uint32_t id_;
    ComponentSignature signature_;
    std::vector<EntityHandle> entities_;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentArray>> componentArrays_;
    friend class TransitionPlan;
};

} // namespace ecs
