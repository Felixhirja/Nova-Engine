#pragma once
#include <cstdint>
#include <functional>

namespace ecs {

// Entity ID is 32-bit: 24 bits for index, 8 bits for generation
// This allows 16 million entities with 256 recycling generations per ID
using EntityIndex = uint32_t;
using EntityGeneration = uint8_t;

// Packed entity handle with versioning
struct EntityHandle {
    static constexpr EntityIndex INDEX_MASK = 0x00FFFFFF;  // 24 bits
    static constexpr uint32_t GENERATION_SHIFT = 24;
    static constexpr EntityIndex NULL_INDEX = INDEX_MASK;  // Reserved for null entity
    
    uint32_t value = 0;  // Packed: [8-bit generation][24-bit index]
    
    // Constructors
    constexpr EntityHandle() noexcept : value(NULL_INDEX) {}
    constexpr EntityHandle(EntityIndex index, EntityGeneration generation) noexcept
        : value((static_cast<uint32_t>(generation) << GENERATION_SHIFT) | (index & INDEX_MASK)) {}
    constexpr explicit EntityHandle(uint32_t packed) noexcept : value(packed) {}
    
    // Accessors
    constexpr EntityIndex Index() const noexcept { 
        return value & INDEX_MASK; 
    }
    
    constexpr EntityGeneration Generation() const noexcept { 
        return static_cast<EntityGeneration>(value >> GENERATION_SHIFT); 
    }
    
    constexpr bool IsNull() const noexcept { 
        return Index() == NULL_INDEX; 
    }
    
    constexpr bool IsValid() const noexcept { 
        return !IsNull(); 
    }
    
    // Comparisons
    constexpr bool operator==(const EntityHandle& other) const noexcept {
        return value == other.value;
    }
    
    constexpr bool operator!=(const EntityHandle& other) const noexcept {
        return value != other.value;
    }
    
    constexpr bool operator<(const EntityHandle& other) const noexcept {
        return value < other.value;
    }
    
    // Null entity constant
    static constexpr EntityHandle Null() noexcept {
        return EntityHandle(NULL_INDEX, 0);
    }
};

// Hash support for EntityHandle (for use in unordered containers)
struct EntityHandleHash {
    std::size_t operator()(const EntityHandle& handle) const noexcept {
        return std::hash<uint32_t>{}(handle.value);
    }
};

// Entity metadata stored by EntityManager
struct EntityMetadata {
    EntityGeneration generation = 0;  // Current generation (for versioning)
    uint32_t archetypeId = 0;         // Which archetype this entity belongs to
    uint32_t indexInArchetype = 0;    // Index within archetype's storage
    bool alive = false;               // Is this entity currently active
    
    EntityMetadata() = default;
    EntityMetadata(EntityGeneration gen, uint32_t archetype, uint32_t index)
        : generation(gen), archetypeId(archetype), indexInArchetype(index), alive(true) {}
};

} // namespace ecs

// Backward compatibility: alias for old code
using Entity = ecs::EntityHandle;
