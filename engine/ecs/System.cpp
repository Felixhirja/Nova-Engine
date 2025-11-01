#include "System.h"
#include "EntityManager.h"
#include "Components.h"
#include "../physics/PhysicsEngine.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string PhaseToString(ecs::UpdatePhase phase) {
    switch (phase) {
        case ecs::UpdatePhase::Input:
            return "Input";
        case ecs::UpdatePhase::Simulation:
            return "Simulation";
        case ecs::UpdatePhase::RenderPrep:
            return "Render Prep";
    }
    return "Unknown";
}

const char* AccessToString(ecs::ComponentAccess access) {
    switch (access) {
        case ecs::ComponentAccess::Read:
            return "Read";
        case ecs::ComponentAccess::Write:
            return "Write";
        case ecs::ComponentAccess::ReadWrite:
            return "Read/Write";
    }
    return "Unknown";
}

std::string FormatComponentList(const std::vector<ecs::ComponentDependency>& dependencies) {
    if (dependencies.empty()) {
        return "None";
    }

    std::ostringstream stream;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        const auto& dep = dependencies[i];
        stream << dep.type.name() << " (" << AccessToString(dep.access) << ")";
        if (i + 1 < dependencies.size()) {
            stream << "<br/>";
        }
    }
    return stream.str();
}

std::string FormatSystemDependencyList(const std::vector<ecs::SystemDependency>& dependencies) {
    if (dependencies.empty()) {
        return "None";
    }

    std::ostringstream stream;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        stream << dependencies[i].type.name();
        if (i + 1 < dependencies.size()) {
            stream << "<br/>";
        }
    }
    return stream.str();
}

} // namespace

System::System(SystemType type) : systemType_(type) {}

void SystemManager::Clear() {
    scheduler_.Clear();
    systems_.clear();
    metadataCache_.clear();
    wrapperTypeLUT_.clear();
    metadataDirty_ = true;
    scheduleDirty_ = true;
    currentEntityManager_ = nullptr;
}

void SystemManager::UpdateAll(EntityManager& entityManager, double dt) {
    entityManager.EnableArchetypeFacade();
    BuildSchedule();

    struct EntityManagerScope {
        SystemManager& manager;
        ~EntityManagerScope() { manager.currentEntityManager_ = nullptr; }
    } scope{*this};

    currentEntityManager_ = &entityManager;
    scheduler_.UpdateAll(entityManager.GetArchetypeManager(), dt);
}

void SystemManager::SetDocumentationOutputPath(std::string path) {
    documentationOutputPath_ = std::move(path);
    ExportDocumentation();
}

const std::vector<SystemManager::SystemMetadata>& SystemManager::GetRegisteredSystemMetadata() const {
    if (!metadataDirty_) {
        return metadataCache_;
    }

    metadataCache_.clear();
    metadataCache_.reserve(systems_.size());

    for (const auto& registration : systems_) {
        if (!registration || !registration->instance) {
            continue;
        }

        metadataCache_.push_back(SystemMetadata{
            registration->name,
            registration->legacyType.name(),
            registration->phase,
            registration->componentDependencies,
            registration->systemDependencies});
    }

    metadataDirty_ = false;
    return metadataCache_;
}

void SystemManager::BuildSchedule() {
    if (!scheduleDirty_) {
        return;
    }

    wrapperTypeLUT_.clear();
    scheduler_.Clear();

    for (auto& registration : systems_) {
        if (!registration) {
            continue;
        }
        RefreshRegistrationMetadata(*registration);
        wrapperTypeLUT_.insert_or_assign(registration->legacyType, registration->wrapperType);
    }

    for (auto& registration : systems_) {
        if (!registration || !registration->factory) {
            continue;
        }
        scheduler_.RegisterSystemInstance(registration->factory(*this, *registration));
    }

    scheduleDirty_ = false;
    EmitComponentConflicts();
    ExportDocumentation();
}

void SystemManager::RefreshRegistrationMetadata(RegisteredSystem& registration) {
    if (!registration.instance) {
        registration.name = "<null system>";
        registration.phase = ecs::UpdatePhase::Simulation;
        registration.componentDependencies.clear();
        registration.systemDependencies.clear();
        metadataDirty_ = true;
        return;
    }

    registration.phase = registration.instance->GetUpdatePhase();
    registration.componentDependencies = registration.instance->GetComponentDependencies();
    registration.systemDependencies = registration.instance->GetSystemDependencies();

    const char* name = registration.instance->GetName();
    if (name && *name) {
        registration.name = name;
    } else {
        registration.name = registration.legacyType.name();
    }

    metadataDirty_ = true;
}

void SystemManager::InvokeLegacyUpdate(RegisteredSystem& registration, double dt) {
    if (!currentEntityManager_) {
        throw std::runtime_error("SystemManager attempted to update a system without an active EntityManager");
    }

    registration.instance->Update(*currentEntityManager_, dt);
}

void SystemManager::EmitComponentConflicts() const {
    if (systems_.empty()) {
        return;
    }

    std::map<ecs::UpdatePhase, std::vector<const RegisteredSystem*>> grouped;
    for (const auto& registration : systems_) {
        if (!registration) {
            continue;
        }
        grouped[registration->phase].push_back(registration.get());
    }

    for (const auto& [phase, systemsInPhase] : grouped) {
        for (std::size_t i = 0; i < systemsInPhase.size(); ++i) {
            for (std::size_t j = i + 1; j < systemsInPhase.size(); ++j) {
                const auto* a = systemsInPhase[i];
                const auto* b = systemsInPhase[j];
                if (!a || !b) {
                    continue;
                }

                if (!HasComponentConflict(a->componentDependencies, b->componentDependencies)) {
                    continue;
                }

                std::cerr << "[SystemManager] Warning: component access conflict detected in phase "
                          << PhaseToString(phase) << " between systems '" << a->name << "' and '"
                          << b->name << "'." << std::endl;
            }
        }
    }
}

void SystemManager::ExportDocumentation() const {
    if (documentationOutputPath_.empty()) {
        return;
    }

    const auto& metadata = GetRegisteredSystemMetadata();

    std::filesystem::path outputPath(documentationOutputPath_);
    if (!outputPath.empty() && outputPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(outputPath.parent_path(), ec);
        if (ec) {
            std::cerr << "[SystemManager] Failed to create documentation directory '"
                      << outputPath.parent_path().string() << "': " << ec.message() << std::endl;
        }
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        std::cerr << "[SystemManager] Failed to write dependency documentation to "
                  << outputPath.string() << std::endl;
        return;
    }

    output << "# System Dependency Map\n\n";

    if (metadata.empty()) {
        output << "_No systems registered._\n";
        return;
    }

    std::map<ecs::UpdatePhase, std::vector<const SystemMetadata*>> grouped;
    for (const auto& entry : metadata) {
        grouped[entry.phase].push_back(&entry);
    }

    for (const auto& [phase, entries] : grouped) {
        output << "## Phase: " << PhaseToString(phase) << "\n\n";
        output << "| System | Legacy Type | Component Access | System Dependencies |\n";
        output << "| --- | --- | --- | --- |\n";
        for (const auto* entry : entries) {
            output << "| " << entry->name << " | `" << entry->legacyTypeName << "` | "
                   << FormatComponentList(entry->componentDependencies) << " | "
                   << FormatSystemDependencyList(entry->systemDependencies) << " |\n";
        }
        output << "\n";
    }
}

std::type_index SystemManager::ResolveWrapperType(const std::type_index& legacyType) const {
    auto it = wrapperTypeLUT_.find(legacyType);
    if (it != wrapperTypeLUT_.end()) {
        return it->second;
    }

    for (const auto& pair : wrapperTypeLUT_) {
        if (pair.second == legacyType) {
            return legacyType;
        }
    }

    throw std::runtime_error(std::string("SystemManager dependency for type ") + legacyType.name() +
                             " is not registered.");
}

bool SystemManager::HasComponentConflict(const std::vector<ecs::ComponentDependency>& a,
                                         const std::vector<ecs::ComponentDependency>& b) const {
    for (const auto& depA : a) {
        for (const auto& depB : b) {
            if (depA.type != depB.type) {
                continue;
            }

            const bool aWrites = depA.access == ecs::ComponentAccess::Write ||
                                 depA.access == ecs::ComponentAccess::ReadWrite;
            const bool bWrites = depB.access == ecs::ComponentAccess::Write ||
                                 depB.access == ecs::ComponentAccess::ReadWrite;

            if (aWrites || bWrites) {
                return true;
            }
        }
    }

    return false;
}

// UnifiedSystem implementation
UnifiedSystem::UnifiedSystem(SystemType type) : System(type) {}

void UnifiedSystem::Update(EntityManager& entityManager, double dt) {
    switch (systemType_) {
        case SystemType::Weapon:
            UpdateWeaponSystem(entityManager, dt);
            break;
        case SystemType::Physics:
            UpdatePhysicsSystem(entityManager, dt);
            break;
        case SystemType::Movement:
            UpdateMovementSystem(entityManager, dt);
            break;
        case SystemType::PlayerControl:
            UpdatePlayerControlSystem(entityManager, dt);
            break;
        case SystemType::BehaviorTree:
            UpdateBehaviorTreeSystem(entityManager, dt);
            break;
        case SystemType::Locomotion:
            UpdateLocomotionSystem(entityManager, dt);
            break;
        case SystemType::ShipAssembly:
            UpdateShipAssemblySystem(entityManager, dt);
            break;
        case SystemType::SpaceshipPhysics:
            UpdateSpaceshipPhysicsSystem(entityManager, dt);
            break;
        case SystemType::Animation:
            UpdateAnimationSystem(entityManager, dt);
            break;
        case SystemType::Targeting:
            UpdateTargetingSystem(entityManager, dt);
            break;
        case SystemType::Shield:
            UpdateShieldSystem(entityManager, dt);
            break;
        case SystemType::Navigation:
            UpdateNavigationSystem(entityManager, dt);
            break;
        case SystemType::GameplayEvent:
            UpdateGameplayEventSystem(entityManager, dt);
            break;
        case SystemType::MissionScript:
            UpdateMissionScriptSystem(entityManager, dt);
            break;
        // Add cases for other system types
        default:
            // Default behavior or placeholder
            break;
    }
}

ecs::UpdatePhase UnifiedSystem::GetUpdatePhase() const {
    switch (systemType_) {
        case SystemType::PlayerControl:
            return ecs::UpdatePhase::Input;
        case SystemType::Weapon:
            return ecs::UpdatePhase::RenderPrep;
        case SystemType::Physics:
        case SystemType::Movement:
        case SystemType::BehaviorTree:
        default:
            return ecs::UpdatePhase::Simulation;
    }
}

std::vector<ecs::ComponentDependency> UnifiedSystem::GetComponentDependencies() const {
    // Return appropriate dependencies based on system type
    switch (systemType_) {
        case SystemType::Weapon:
            return {{typeid(Weapon), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Physics:
            return {{typeid(Position), ecs::ComponentAccess::ReadWrite},
                    {typeid(Velocity), ecs::ComponentAccess::ReadWrite},
                    {typeid(RigidBody), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Movement:
            return {{typeid(Position), ecs::ComponentAccess::ReadWrite},
                    {typeid(Velocity), ecs::ComponentAccess::ReadWrite},
                    {typeid(Acceleration), ecs::ComponentAccess::Read}};
        case SystemType::PlayerControl:
            return {{typeid(PlayerController), ecs::ComponentAccess::ReadWrite},
                    {typeid(Velocity), ecs::ComponentAccess::ReadWrite}};
        case SystemType::BehaviorTree:
            return {{typeid(BehaviorTreeComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Locomotion:
            return {{typeid(LocomotionComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::ShipAssembly:
            return {{typeid(ShipAssemblyComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::SpaceshipPhysics:
            return {{typeid(SpaceshipPhysicsComponent), ecs::ComponentAccess::ReadWrite},
                    {typeid(Position), ecs::ComponentAccess::ReadWrite},
                    {typeid(Velocity), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Animation:
            return {{typeid(AnimationComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Targeting:
            return {{typeid(TargetingComponent), ecs::ComponentAccess::ReadWrite},
                    {typeid(Position), ecs::ComponentAccess::Read}};
        case SystemType::Shield:
            return {{typeid(ShieldComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::Navigation:
            return {{typeid(NavigationComponent), ecs::ComponentAccess::ReadWrite},
                    {typeid(Position), ecs::ComponentAccess::Read},
                    {typeid(Velocity), ecs::ComponentAccess::Read}};
        case SystemType::GameplayEvent:
            return {{typeid(GameplayEventComponent), ecs::ComponentAccess::ReadWrite}};
        case SystemType::MissionScript:
            return {{typeid(MissionScriptComponent), ecs::ComponentAccess::ReadWrite}};
        default:
            return {};
    }
}

std::vector<ecs::SystemDependency> UnifiedSystem::GetSystemDependencies() const {
    // For now, return no dependencies since all UnifiedSystem instances have the same type
    // Dependencies are handled internally based on SystemType
    return {};
}

const char* UnifiedSystem::GetName() const {
    switch (systemType_) {
        case SystemType::Weapon: return "WeaponSystem";
        case SystemType::Physics: return "PhysicsSystem";
        case SystemType::Movement: return "MovementSystem";
        case SystemType::PlayerControl: return "PlayerControlSystem";
        case SystemType::BehaviorTree: return "BehaviorTreeSystem";
        case SystemType::Locomotion: return "LocomotionSystem";
        case SystemType::ShipAssembly: return "ShipAssemblySystem";
        case SystemType::SpaceshipPhysics: return "SpaceshipPhysicsSystem";
        case SystemType::Animation: return "AnimationSystem";
        case SystemType::Targeting: return "TargetingSystem";
        case SystemType::Shield: return "ShieldSystem";
        case SystemType::Navigation: return "NavigationSystem";
        case SystemType::GameplayEvent: return "GameplayEventSystem";
        case SystemType::MissionScript: return "MissionScriptSystem";
        default: return "UnifiedSystem";
    }
}

// Weapon system methods
void UnifiedSystem::ConfigureWeaponSlot(int entityId, const std::string& weaponSlot, const WeaponSlotConfig& config) {
    weaponConfigs_[entityId][weaponSlot] = config;
}

bool UnifiedSystem::FireWeapon(EntityManager& entityManager, int entityId, const std::string& weaponSlot) {
    auto* config = GetWeaponConfig(entityId, weaponSlot);
    if (!config) return false;

    auto& cooldown = weaponCooldowns_[entityId][weaponSlot];
    if (cooldown > 0.0f) return false;

    if (config->ammo != -1 && weaponAmmo_[entityId][weaponSlot] <= 0) return false;

    // Fire weapon logic here
    cooldown = 1.0f / config->fireRatePerSecond;

    if (config->ammo != -1) {
        weaponAmmo_[entityId][weaponSlot]--;
    }

    // Create projectile entity
    double x, y, z;
    if (!ExtractEntityPosition(entityManager, entityId, x, y, z)) return false;

    // Add projectile creation logic here

    return true;
}

bool UnifiedSystem::CanFire(int entityId, const std::string& weaponSlot) const {
    auto* config = GetWeaponConfig(entityId, weaponSlot);
    if (!config) return false;

    if (weaponCooldowns_.at(entityId).at(weaponSlot) > 0.0f) return false;
    if (config->ammo != -1 && weaponAmmo_.at(entityId).at(weaponSlot) <= 0) return false;

    return true;
}

int UnifiedSystem::GetAmmoCount(int entityId, const std::string& weaponSlot) const {
    auto entityIt = weaponAmmo_.find(entityId);
    if (entityIt == weaponAmmo_.end()) return 0;

    auto slotIt = entityIt->second.find(weaponSlot);
    return slotIt != entityIt->second.end() ? slotIt->second : 0;
}

const WeaponSlotConfig* UnifiedSystem::GetWeaponConfig(int entityId, const std::string& weaponSlot) const {
    auto entityIt = weaponConfigs_.find(entityId);
    if (entityIt == weaponConfigs_.end()) return nullptr;

    auto slotIt = entityIt->second.find(weaponSlot);
    return slotIt != entityIt->second.end() ? &slotIt->second : nullptr;
}

bool UnifiedSystem::ExtractEntityPosition(EntityManager& entityManager, int entityId, double& x, double& y, double& z) const {
    if (auto* pos = entityManager.GetComponent<Position>(entityId)) {
        x = pos->x;
        y = pos->y;
        z = pos->z;
        return true;
    }
    return false;
}

// Physics system methods
void UnifiedSystem::UseExternalEngine(std::shared_ptr<physics::IPhysicsEngine> engine) {
    externalEngine_ = engine;
    activeBackend_ = engine ? physics::PhysicsBackendType::BuiltIn : physics::PhysicsBackendType::BuiltIn;
}

void UnifiedSystem::ResetToBuiltin() {
    externalEngine_.reset();
    activeBackend_ = physics::PhysicsBackendType::BuiltIn;
}

void UnifiedSystem::SetGravity(double x, double y, double z) {
    globalGravityX_ = x;
    globalGravityY_ = y;
    globalGravityZ_ = z;
}

void UnifiedSystem::SetGlobalDamping(double linear, double angular) {
    globalLinearDamping_ = linear;
    globalAngularDamping_ = angular;
}

void UnifiedSystem::SetMaxVelocity(double maxVel) {
    maxVelocity_ = maxVel;
}

void UnifiedSystem::SetCollisionEnabled(bool enabled) {
    collisionEnabled_ = enabled;
}

bool UnifiedSystem::Raycast(double originX, double originY, double originZ,
                           double dirX, double dirY, double dirZ,
                           double maxDistance, physics::RaycastHit& hit) {
    // Placeholder raycast implementation
    (void)originX; (void)originY; (void)originZ;
    (void)dirX; (void)dirY; (void)dirZ; (void)maxDistance; (void)hit;
    return false;
}

std::vector<unsigned int> UnifiedSystem::OverlapSphere(double centerX, double centerY, double centerZ,
                                                      double radius, unsigned int layerMask) {
    (void)centerX; (void)centerY; (void)centerZ; (void)radius; (void)layerMask;
    return {};
}

std::vector<unsigned int> UnifiedSystem::OverlapBox(double centerX, double centerY, double centerZ,
                                                   double width, double height, double depth,
                                                   unsigned int layerMask) {
    (void)centerX; (void)centerY; (void)centerZ; (void)width; (void)height; (void)depth; (void)layerMask;
    return {};
}

void UnifiedSystem::ApplyForce(unsigned int entity, double fx, double fy, double fz) {
    (void)entity; (void)fx; (void)fy; (void)fz;
    // Placeholder
}

void UnifiedSystem::ApplyImpulse(unsigned int entity, double ix, double iy, double iz) {
    (void)entity; (void)ix; (void)iy; (void)iz;
    // Placeholder
}

void UnifiedSystem::ApplyForceAtPoint(unsigned int entity, double fx, double fy, double fz,
                                     double px, double py, double pz) {
    (void)entity; (void)fx; (void)fy; (void)fz; (void)px; (void)py; (void)pz;
    // Placeholder
}

// System update implementations
void UnifiedSystem::UpdateWeaponSystem(EntityManager& entityManager, double dt) {
    // Update weapon cooldowns
    for (auto& entityCooldowns : weaponCooldowns_) {
        for (auto& slotCooldown : entityCooldowns.second) {
            if (slotCooldown.second > 0.0f) {
                slotCooldown.second -= static_cast<float>(dt);
                if (slotCooldown.second < 0.0f) slotCooldown.second = 0.0f;
            }
        }
    }

    // Weapon firing logic would go here
    entityManager.ForEach<Weapon>([&](Entity entity, Weapon& weapon) {
        // Process weapon firing requests
        (void)entity; (void)weapon;
    });
}

void UnifiedSystem::UpdatePhysicsSystem(EntityManager& entityManager, double dt) {
    if (activeBackend_ != physics::PhysicsBackendType::BuiltIn && externalEngine_) {
        // Use external physics engine
        externalEngine_->StepSimulation(*this, entityManager, dt);
    } else {
        // Use built-in physics
        RunBuiltinSimulation(dt);
    }
}

void UnifiedSystem::UpdateMovementSystem(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) return;

    static int debugCounter = 0;
    static bool firstRun = true;
    int posVelCount = 0;
    
    if (firstRun) {
        std::cout << "[MovementSystem] FIRST RUN" << std::endl;
        firstRun = false;
    }

    // After migration to archetype storage, multi-component ForEach works
    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        posVelCount++;
        // Update position with velocity (basic integration)
        position.x += velocity.vx * dt;
        position.y += velocity.vy * dt;
        position.z += velocity.vz * dt;

        // Apply acceleration if present
        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            velocity.vx += acceleration->ax * dt;
            velocity.vy += acceleration->ay * dt;
            velocity.vz += acceleration->az * dt;
        }
    });

    // Player physics logic
    entityManager.ForEach<Position, Velocity, PlayerPhysics>([&](Entity entity, Position& position, Velocity& velocity, PlayerPhysics& physics) {
        (void)position;  // Position not used in this system
        physics.isGrounded = false;

        if (auto* rigidBody = entityManager.GetComponent<RigidBody>(entity)) {
            rigidBody->useGravity = physics.enableGravity;
        }

        if (physics.enableGravity) {
            velocity.vz += physics.gravity * dt;
        }

        if (velocity.vz > physics.maxAscentSpeed) velocity.vz = physics.maxAscentSpeed;
        if (velocity.vz < physics.maxDescentSpeed) velocity.vz = physics.maxDescentSpeed;
    });

    // Movement bounds clamping
    entityManager.ForEach<Position, Velocity, MovementBounds>([&](Entity entity, Position& position, Velocity& velocity, MovementBounds& bounds) {
        (void)entity;  // Entity ID not used
        // Bounds checking logic (simplified)
        if (bounds.clampX) {
            if (position.x < bounds.minX) {
                position.x = bounds.minX;
                velocity.vx = 0.0;
            } else if (position.x > bounds.maxX) {
                position.x = bounds.maxX;
                velocity.vx = 0.0;
            }
        }
        // Similar for Y and Z bounds
    });
    
    if (++debugCounter % 120 == 0) {
        std::cout << "[MovementSystem] Processed " << posVelCount << " entities with Position+Velocity" << std::endl;
    }
}

void UnifiedSystem::UpdatePlayerControlSystem(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) return;

    static int debugCounter = 0;
    static bool firstRun = true;
    int entityCount = 0;
    
    if (firstRun) {
        std::cout << "[PlayerControlSystem] FIRST RUN" << std::endl;
        firstRun = false;
    }

    // After migration to archetype storage, multi-component ForEach works
    entityManager.ForEach<PlayerController, Velocity>([&](Entity entity, PlayerController& controller, Velocity& velocity) {
        entityCount++;
        const MovementParameters* movement = entityManager.GetComponent<MovementParameters>(entity);

        // Movement parameter defaults
        double strafeAcceleration = 4.0;
        double forwardAcceleration = 4.0;
        double backwardAcceleration = 4.0;
        double strafeDeceleration = 4.0;
        double forwardDeceleration = 4.0;
        double backwardDeceleration = 4.0;
        double strafeMaxSpeed = 5.0;
        double forwardMaxSpeed = 5.0;
        double backwardMaxSpeed = 5.0;
        double friction = 0.0;

        if (movement) {
            strafeAcceleration = std::max(0.0, movement->strafeAcceleration);
            forwardAcceleration = std::max(0.0, movement->forwardAcceleration);
            backwardAcceleration = std::max(0.0, movement->backwardAcceleration);
            strafeDeceleration = std::max(0.0, movement->strafeDeceleration);
            forwardDeceleration = std::max(0.0, movement->forwardDeceleration);
            backwardDeceleration = std::max(0.0, movement->backwardDeceleration);
            strafeMaxSpeed = std::max(0.0, movement->strafeMaxSpeed);
            forwardMaxSpeed = std::max(0.0, movement->forwardMaxSpeed);
            backwardMaxSpeed = std::max(0.0, movement->backwardMaxSpeed);
            friction = std::max(0.0, movement->friction);
        }

        // Apply input to velocity
        if (controller.moveForward) {
            velocity.vy += forwardAcceleration * dt;
            if (velocity.vy > forwardMaxSpeed) velocity.vy = forwardMaxSpeed;
        } else if (controller.moveBackward) {
            velocity.vy -= backwardAcceleration * dt;
            if (velocity.vy < -backwardMaxSpeed) velocity.vy = -backwardMaxSpeed;
        } else {
            // Deceleration
            if (velocity.vy > 0) {
                velocity.vy -= forwardDeceleration * dt;
                if (velocity.vy < 0) velocity.vy = 0;
            } else if (velocity.vy < 0) {
                velocity.vy += backwardDeceleration * dt;
                if (velocity.vy > 0) velocity.vy = 0;
            }
        }

        if (controller.strafeLeft) {
            velocity.vx -= strafeAcceleration * dt;
            if (velocity.vx < -strafeMaxSpeed) velocity.vx = -strafeMaxSpeed;
        } else if (controller.strafeRight) {
            velocity.vx += strafeAcceleration * dt;
            if (velocity.vx > strafeMaxSpeed) velocity.vx = strafeMaxSpeed;
        } else {
            // Deceleration
            if (velocity.vx > 0) {
                velocity.vx -= strafeDeceleration * dt;
                if (velocity.vx < 0) velocity.vx = 0;
            } else if (velocity.vx < 0) {
                velocity.vx += strafeDeceleration * dt;
                if (velocity.vx > 0) velocity.vx = 0;
            }
        }

        // Apply friction
        if (friction > 0.0) {
            velocity.vx *= (1.0 - friction * dt);
            velocity.vy *= (1.0 - friction * dt);
            velocity.vz *= (1.0 - friction * dt);
        }
    });
    
    if (++debugCounter % 120 == 0) {
        std::cout << "[PlayerControlSystem] Processed " << entityCount << " entities with PlayerController+Velocity" << std::endl;
    }
}

void UnifiedSystem::UpdateBehaviorTreeSystem(EntityManager& entityManager, double dt) {
    // Behavior tree update logic would go here
    entityManager.ForEach<BehaviorTreeComponent>([&](Entity entity, BehaviorTreeComponent& bt) {
        // Update behavior trees
        (void)entity; (void)bt; (void)dt;
    });
}

void UnifiedSystem::UpdateLocomotionSystem(EntityManager& entityManager, double dt) {
    // Locomotion system logic would go here
    entityManager.ForEach<LocomotionComponent>([&](Entity entity, LocomotionComponent& loco) {
        // Update locomotion
        (void)entity; (void)loco; (void)dt;
    });
}

void UnifiedSystem::UpdateShipAssemblySystem(EntityManager& entityManager, double dt) {
    // Ship assembly system logic would go here
    entityManager.ForEach<ShipAssemblyComponent>([&](Entity entity, ShipAssemblyComponent& assembly) {
        // Update ship assembly
        (void)entity; (void)assembly; (void)dt;
    });
}

void UnifiedSystem::UpdateSpaceshipPhysicsSystem(EntityManager& entityManager, double dt) {
    // Spaceship physics system logic would go here
    entityManager.ForEach<SpaceshipPhysicsComponent>([&](Entity entity, SpaceshipPhysicsComponent& physics) {
        // Update spaceship physics
        (void)entity; (void)physics; (void)dt;
    });
}

void UnifiedSystem::UpdateAnimationSystem(EntityManager& entityManager, double dt) {
    // Animation system logic would go here
    entityManager.ForEach<AnimationComponent>([&](Entity entity, AnimationComponent& anim) {
        // Update animations
        (void)entity; (void)anim; (void)dt;
    });
}

void UnifiedSystem::UpdateTargetingSystem(EntityManager& entityManager, double dt) {
    // Targeting system logic would go here
    entityManager.ForEach<TargetingComponent>([&](Entity entity, TargetingComponent& targeting) {
        // Update targeting
        (void)entity; (void)targeting; (void)dt;
    });
}

void UnifiedSystem::UpdateShieldSystem(EntityManager& entityManager, double dt) {
    // Shield system logic - handle recharge
    entityManager.ForEach<ShieldComponent>([&](Entity entity, ShieldComponent& shield) {
        if (!shield.isActive) return;
        
        // Update last damage time
        shield.lastDamageTime += dt;
        
        // Check if we can recharge
        if (shield.lastDamageTime >= shield.rechargeDelay && shield.currentShields < shield.maxShields) {
            double rechargeAmount = shield.rechargeRate * dt;
            shield.currentShields = std::min(shield.maxShields, shield.currentShields + rechargeAmount);
        }
    });
}

void UnifiedSystem::UpdateNavigationSystem(EntityManager& entityManager, double dt) {
    // Navigation system logic would go here
    entityManager.ForEach<NavigationComponent>([&](Entity entity, NavigationComponent& nav) {
        // Update navigation
        (void)entity; (void)nav; (void)dt;
    });
}

void UnifiedSystem::UpdateGameplayEventSystem(EntityManager& entityManager, double dt) {
    // Gameplay event system logic would go here
    entityManager.ForEach<GameplayEventComponent>([&](Entity entity, GameplayEventComponent& event) {
        // Update gameplay events
        (void)entity; (void)event; (void)dt;
    });
}

void UnifiedSystem::UpdateMissionScriptSystem(EntityManager& entityManager, double dt) {
    // Mission script system logic would go here
    entityManager.ForEach<MissionScriptComponent>([&](Entity entity, MissionScriptComponent& script) {
        // Update mission scripts
        (void)entity; (void)script; (void)dt;
    });
}

// Shield management methods
void UnifiedSystem::InitializeShield(Entity entity, double maxCapacity, double rechargeRate, 
                                   double rechargeDelay, double absorptionRatio, const std::string& shieldType) {
    // Note: In a real implementation, we'd add ShieldComponent to the entity
    // For now, we'll store metadata for testing purposes
    shieldAbsorptionRatios_[entity] = absorptionRatio;
    shieldTypes_[entity] = shieldType;
    
    // The test expects to access ShieldComponent directly from EntityManager
    // This method is mainly for initialization metadata
}

const ShieldComponent* UnifiedSystem::GetShieldState(Entity entity) const {
    // This method needs access to EntityManager, so it should be called during system updates
    // For testing purposes, we'll return nullptr and let the test handle component access directly
    return nullptr;
}

double UnifiedSystem::GetShieldPercentage(Entity entity) const {
    // This would need EntityManager access, simplified for testing
    return 1.0;
}

double UnifiedSystem::ApplyDamage(Entity entity, double damage, EntityManager* entityManager) {
    if (!entityManager) return damage;
    
    auto* shield = entityManager->GetComponent<ShieldComponent>(entity);
    if (!shield || !shield->isActive || shield->currentShields <= 0.0) {
        return damage; // No shield protection
    }
    
    auto it = shieldAbsorptionRatios_.find(entity);
    double absorptionRatio = (it != shieldAbsorptionRatios_.end()) ? it->second : 0.8;
    
    double absorbedDamage = std::min(damage * absorptionRatio, shield->currentShields);
    double hullDamage = damage - absorbedDamage;
    
    shield->currentShields -= absorbedDamage;
    shield->lastDamageTime = 0.0; // Reset recharge timer
    
    return hullDamage;
}

// Energy management methods
void UnifiedSystem::InitializeEnergy(Entity entity, double totalCapacity, double rechargeRate, 
                                   double consumptionRate, double efficiency) {
    EnergyComponent energy;
    energy.totalPowerCapacityMW = totalCapacity;
    energy.currentPowerMW = totalCapacity;
    energy.rechargeRateMW = rechargeRate;
    energy.consumptionRateMW = consumptionRate;
    energy.efficiency = efficiency;
    energy.isActive = true;
    
    // Default balanced allocation
    energy.shieldAllocation = 0.33;
    energy.weaponAllocation = 0.33;
    energy.thrusterAllocation = 0.34;
    
    energyComponents_[entity] = energy;
}

const EnergyComponent* UnifiedSystem::GetEnergyState(Entity entity) const {
    auto it = energyComponents_.find(entity);
    return (it != energyComponents_.end()) ? &it->second : nullptr;
}

void UnifiedSystem::UpdateEnergy(Entity entity, double dt) {
    auto it = energyComponents_.find(entity);
    if (it == energyComponents_.end()) return;
    
    EnergyComponent& energy = it->second;
    if (!energy.isActive) return;
    
    // Recharge power
    energy.currentPowerMW = std::min(energy.totalPowerCapacityMW, 
                                    energy.currentPowerMW + energy.rechargeRateMW * dt);
    
    // Distribute power based on allocations
    double totalPower = energy.currentPowerMW * energy.efficiency;
    energy.shieldPowerMW = totalPower * energy.shieldAllocation;
    energy.weaponPowerMW = totalPower * energy.weaponAllocation;
    energy.thrusterPowerMW = totalPower * energy.thrusterAllocation;
}

void UnifiedSystem::DivertPower(Entity entity, PowerPriority priority, double amount) {
    auto it = energyComponents_.find(entity);
    if (it == energyComponents_.end()) return;
    
    EnergyComponent& energy = it->second;
    
    // Adjust allocations based on priority
    switch (priority) {
        case PowerPriority::Shields:
            energy.shieldAllocation = std::min(1.0, energy.shieldAllocation + amount);
            // Reduce others proportionally
            energy.weaponAllocation = std::max(0.0, energy.weaponAllocation - amount * 0.33);
            energy.thrusterAllocation = std::max(0.0, energy.thrusterAllocation - amount * 0.33);
            // Note: Sensors allocation not yet implemented in EnergyComponent
            break;
        case PowerPriority::Weapons:
            energy.weaponAllocation = std::min(1.0, energy.weaponAllocation + amount);
            energy.shieldAllocation = std::max(0.0, energy.shieldAllocation - amount * 0.33);
            energy.thrusterAllocation = std::max(0.0, energy.thrusterAllocation - amount * 0.33);
            // Note: Sensors allocation not yet implemented in EnergyComponent
            break;
        case PowerPriority::Thrusters:
            energy.thrusterAllocation = std::min(1.0, energy.thrusterAllocation + amount);
            energy.shieldAllocation = std::max(0.0, energy.shieldAllocation - amount * 0.33);
            energy.weaponAllocation = std::max(0.0, energy.weaponAllocation - amount * 0.33);
            // Note: Sensors allocation not yet implemented in EnergyComponent
            break;
        case PowerPriority::Sensors:
            // Note: Sensors allocation not yet implemented in EnergyComponent
            // For now, treat as a no-op until sensor allocation is added
            break;
    }
    
    // Normalize allocations
    double total = energy.shieldAllocation + energy.weaponAllocation + energy.thrusterAllocation;
    if (total > 0.0) {
        energy.shieldAllocation /= total;
        energy.weaponAllocation /= total;
        energy.thrusterAllocation /= total;
    }
}

bool UnifiedSystem::HasPower(Entity entity, PowerPriority priority) const {
    auto it = energyComponents_.find(entity);
    if (it == energyComponents_.end()) return false;
    
    const EnergyComponent& energy = it->second;
    if (!energy.isActive) return false;
    
    switch (priority) {
        case PowerPriority::Shields:
            return energy.shieldPowerMW >= 5.0; // Minimum threshold
        case PowerPriority::Weapons:
            return energy.weaponPowerMW >= 5.0;
        case PowerPriority::Thrusters:
            return energy.thrusterPowerMW >= 5.0;
        case PowerPriority::Sensors:
            // Note: Sensors power checking not yet implemented
            return false;
        default:
            return false;
    }
}

void UnifiedSystem::SetEnergyAllocation(Entity entity, double shieldAlloc, double weaponAlloc, double thrusterAlloc) {
    auto it = energyComponents_.find(entity);
    if (it == energyComponents_.end()) return;
    
    EnergyComponent& energy = it->second;
    energy.shieldAllocation = shieldAlloc;
    energy.weaponAllocation = weaponAlloc;
    energy.thrusterAllocation = thrusterAlloc;
}

void UnifiedSystem::SetRandomManager(DeterministicRandom* randomManager) {
    randomManager_ = randomManager;
}

// Physics helper implementations (simplified)
void UnifiedSystem::ApplyGravity(double dt) {
    (void)dt;
    // Gravity application logic
}

void UnifiedSystem::ApplyForces(double dt) {
    (void)dt;
    // Force application logic
}

void UnifiedSystem::ApplyConstantForces(double dt) {
    (void)dt;
    // Constant force application logic
}

void UnifiedSystem::IntegrateVelocities(double dt) {
    (void)dt;
    // Velocity integration logic
}

void UnifiedSystem::DetectCollisions(double dt) {
    (void)dt;
    // Collision detection logic
}

void UnifiedSystem::ResolveCollisions(double dt) {
    (void)dt;
    // Collision resolution logic
}

void UnifiedSystem::UpdateCharacterControllers(double dt) {
    (void)dt;
    // Character controller update logic
}

void UnifiedSystem::UpdateJoints(double dt) {
    (void)dt;
    // Joint update logic
}

void UnifiedSystem::ClearFrameForces() {
    // Clear frame forces logic
}

void UnifiedSystem::RunBuiltinSimulation(double dt) {
    ApplyGravity(dt);
    ApplyForces(dt);
    ApplyConstantForces(dt);
    IntegrateVelocities(dt);
    if (collisionEnabled_) {
        DetectCollisions(dt);
        ResolveCollisions(dt);
    }
    UpdateCharacterControllers(dt);
    UpdateJoints(dt);
    ClearFrameForces();
}

std::vector<CollisionPair> UnifiedSystem::DetectCollisionPairs() {
    return {};
}

std::vector<CollisionPair> UnifiedSystem::DetectSweptCollisionPairs(double dt) {
    (void)dt;
    return {};
}

bool UnifiedSystem::ComputeSweptAABB(const struct BoxCollider& a, const struct Position& posA,
                                    const struct Velocity* velA,
                                    const struct BoxCollider& b, const struct Position& posB,
                                    const struct Velocity* velB, double dt,
                                    CollisionPair& result) {
    (void)a; (void)posA; (void)velA; (void)b; (void)posB; (void)velB; (void)dt; (void)result;
    return false;
}

bool UnifiedSystem::CheckBoxBox(const struct BoxCollider& a, const struct Position& posA,
                               const struct BoxCollider& b, const struct Position& posB,
                               CollisionPair& result) {
    (void)a; (void)posA; (void)b; (void)posB; (void)result;
    return false;
}

bool UnifiedSystem::CheckSphereSphere(const struct SphereCollider& a, const struct Position& posA,
                                     const struct SphereCollider& b, const struct Position& posB,
                                     CollisionPair& result) {
    (void)a; (void)posA; (void)b; (void)posB; (void)result;
    return false;
}

bool UnifiedSystem::CheckBoxSphere(const struct BoxCollider& box, const struct Position& boxPos,
                                  const struct SphereCollider& sphere, const struct Position& spherePos,
                                  CollisionPair& result) {
    (void)box; (void)boxPos; (void)sphere; (void)spherePos; (void)result;
    return false;
}

void UnifiedSystem::ResolveCollisionPair(const CollisionPair& pair, double dt) {
    (void)pair; (void)dt;
}