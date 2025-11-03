#include "ArchetypeManager.h"
#include "Components.h"
#include "TestComponents.h"

#include <typeindex>

namespace ecs {

namespace {

template<typename T>
bool RegisterIfMatches(Archetype* archetype, const std::type_index& typeIndex) {
    if (typeIndex == std::type_index(typeid(T))) {
        if (archetype) {
            archetype->RegisterComponentType<T>();
        }
        return true;
    }
    return false;
}

bool RegisterKnownComponentType(Archetype* archetype, const std::type_index& typeIndex) {
    if (RegisterIfMatches<Position>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Velocity>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Acceleration>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PhysicsBody>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PhysicsMaterial>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Transform2D>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Sprite>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Hitbox>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<AnimationState>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Name>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlayerController>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<MovementParameters>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<MovementBounds>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlayerPhysics>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlayerVitals>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlayerInventory>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlayerProgression>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<DockingStatus>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<LocomotionStateMachine>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<TargetLock>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<ProjectileComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<DrawComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<RigidBody>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Force>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Collider>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<CollisionInfo>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<GravitySource>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<ConstantForce>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<CharacterController>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<Joint>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<CameraComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<ViewportID>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<CelestialBodyComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<OrbitalComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<VisualCelestialComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<AtmosphereComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<SpaceStationComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<SatelliteSystemComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<StarComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<AsteroidBeltComponent>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<PlanetComponent>(archetype, typeIndex)) return true;
    
    // Test components for memory optimization tests
    if (RegisterIfMatches<SimplePosition>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<SimpleVelocity>(archetype, typeIndex)) return true;
    if (RegisterIfMatches<SimpleTestComponent>(archetype, typeIndex)) return true;
    
    return false;
}

} // namespace

// Register component array for known component types
// This uses a type registry pattern to avoid templates in .cpp
void ArchetypeManager::RegisterComponentArrayForType(Archetype* archetype, const std::type_index& typeIndex) {
    if (RegisterKnownComponentType(archetype, typeIndex)) {
        registeredComponentTypes_.insert(typeIndex);
    }
}

bool ArchetypeManager::CanProvideComponentType(const std::type_index& typeIndex) const {
    if (registeredComponentTypes_.count(typeIndex)) {
        return true;
    }
    return RegisterKnownComponentType(nullptr, typeIndex);
}

} // namespace ecs
