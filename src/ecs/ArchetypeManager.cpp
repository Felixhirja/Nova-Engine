#include "ArchetypeManager.h"
#include "Components.h"
#include "../CelestialBody.h"

namespace ecs {

// Register component array for known component types
// This uses a type registry pattern to avoid templates in .cpp
void ArchetypeManager::RegisterComponentArrayForType(Archetype* archetype, const std::type_index& typeIndex) {
    // Basic ECS components
    if (typeIndex == std::type_index(typeid(Position))) {
        archetype->RegisterComponentType<Position>();
    } else if (typeIndex == std::type_index(typeid(Velocity))) {
        archetype->RegisterComponentType<Velocity>();
    } else if (typeIndex == std::type_index(typeid(Acceleration))) {
        archetype->RegisterComponentType<Acceleration>();
    } else if (typeIndex == std::type_index(typeid(PhysicsBody))) {
        archetype->RegisterComponentType<PhysicsBody>();
    } else if (typeIndex == std::type_index(typeid(Transform2D))) {
        archetype->RegisterComponentType<Transform2D>();
    } else if (typeIndex == std::type_index(typeid(Sprite))) {
        archetype->RegisterComponentType<Sprite>();
    } else if (typeIndex == std::type_index(typeid(Hitbox))) {
        archetype->RegisterComponentType<Hitbox>();
    } else if (typeIndex == std::type_index(typeid(AnimationState))) {
        archetype->RegisterComponentType<AnimationState>();
    } else if (typeIndex == std::type_index(typeid(Name))) {
        archetype->RegisterComponentType<Name>();
    } else if (typeIndex == std::type_index(typeid(PlayerController))) {
        archetype->RegisterComponentType<PlayerController>();
    } else if (typeIndex == std::type_index(typeid(MovementBounds))) {
        archetype->RegisterComponentType<MovementBounds>();
    } else if (typeIndex == std::type_index(typeid(PlayerPhysics))) {
        archetype->RegisterComponentType<PlayerPhysics>();
    } else if (typeIndex == std::type_index(typeid(TargetLock))) {
        archetype->RegisterComponentType<TargetLock>();
    }
    // Celestial body components
    else if (typeIndex == std::type_index(typeid(CelestialBodyComponent))) {
        archetype->RegisterComponentType<CelestialBodyComponent>();
    } else if (typeIndex == std::type_index(typeid(OrbitalComponent))) {
        archetype->RegisterComponentType<OrbitalComponent>();
    } else if (typeIndex == std::type_index(typeid(VisualCelestialComponent))) {
        archetype->RegisterComponentType<VisualCelestialComponent>();
    } else if (typeIndex == std::type_index(typeid(AtmosphereComponent))) {
        archetype->RegisterComponentType<AtmosphereComponent>();
    } else if (typeIndex == std::type_index(typeid(SpaceStationComponent))) {
        archetype->RegisterComponentType<SpaceStationComponent>();
    } else if (typeIndex == std::type_index(typeid(SatelliteSystemComponent))) {
        archetype->RegisterComponentType<SatelliteSystemComponent>();
    } else if (typeIndex == std::type_index(typeid(StarComponent))) {
        archetype->RegisterComponentType<StarComponent>();
    } else if (typeIndex == std::type_index(typeid(AsteroidBeltComponent))) {
        archetype->RegisterComponentType<AsteroidBeltComponent>();
    } else if (typeIndex == std::type_index(typeid(PlanetComponent))) {
        archetype->RegisterComponentType<PlanetComponent>();
    }
    // Add more component types here as needed
}

} // namespace ecs
