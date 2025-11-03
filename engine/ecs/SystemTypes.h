#pragma once

#include <typeindex>
#include <typeinfo>
#include <vector>

// Unified system types enumeration
enum class SystemType {
    Animation,
    BehaviorTree,
    DayNightCycle,
    EnvironmentalHazard,
    ECSInspector,
    EVA,
    GameplayEvent,
    Locomotion,
    Mining,
    MissionScript,
    Movement,
    Navigation,
    Physics,
    PlanetaryLanding,
    PlayerControl,
    ResourceScanning,
    Shield,
    ShipAssembly,
    ShipLogistics,
    SpaceshipPhysics,
    SurfaceVehicle,
    Targeting,
    Weapon,
    Weather,
    // Add more system types as needed
};

// Forward declaration for UnifiedSystem
class UnifiedSystem;

namespace ecs {

enum class ComponentAccess {
    Read,
    Write,
    ReadWrite,
};

struct ComponentDependency {
    std::type_index type;
    ComponentAccess access;

    ComponentDependency(std::type_index t, ComponentAccess a)
        : type(t), access(a) {}

    template<typename T>
    static ComponentDependency Read() {
        return {std::type_index(typeid(T)), ComponentAccess::Read};
    }

    template<typename T>
    static ComponentDependency Write() {
        return {std::type_index(typeid(T)), ComponentAccess::Write};
    }

    template<typename T>
    static ComponentDependency ReadWrite() {
        return {std::type_index(typeid(T)), ComponentAccess::ReadWrite};
    }
};

enum class UpdateStage {
    PreUpdate,
    Update,
    PostUpdate,
};

struct SystemDependency {
    std::type_index type;
    SystemType systemType = SystemType::Animation; // Default value

    explicit SystemDependency(std::type_index t, SystemType st = SystemType::Animation) : type(t), systemType(st) {}

    template<typename T>
    static SystemDependency Requires() {
        return SystemDependency(std::type_index(typeid(T)));
    }
};

enum class UpdatePhase {
    Input,
    Simulation,
    RenderPrep,
};

} // namespace ecs

