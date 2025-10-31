#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SystemSchedulerV2.h"
#include "SystemTypes.h"
#include "ai/BehaviorTree.h"
#include "DeterministicRandom.h"
#include "Components.h"
#include "../physics/PhysicsEngine.h"

class EntityManager;
namespace ecs {
class SystemSchedulerV2;
class SystemV2;
}

// Forward declarations and type definitions
struct CollisionPair {
    unsigned int entityA;
    unsigned int entityB;
    double normalX, normalY, normalZ;
    double penetration;
    double relativeVelocityX, relativeVelocityY, relativeVelocityZ;
};

class System {
public:
    // Constructor with system type
    explicit System(SystemType type);

    virtual ~System() = default;
    virtual void Update(EntityManager& entityManager, double dt) = 0;

    virtual ecs::UpdatePhase GetUpdatePhase() const { return ecs::UpdatePhase::Simulation; }

    virtual std::vector<ecs::ComponentDependency> GetComponentDependencies() const { return {}; }

    virtual std::vector<ecs::SystemDependency> GetSystemDependencies() const { return {}; }

    virtual const char* GetName() const { return typeid(*this).name(); }

    SystemType GetSystemType() const { return systemType_; }

protected:
    SystemType systemType_;
};

// Unified system class that contains all system functionality
class UnifiedSystem : public System {
public:
    explicit UnifiedSystem(SystemType type);

    void Update(EntityManager& entityManager, double dt) override;

    ecs::UpdatePhase GetUpdatePhase() const override;

    std::vector<ecs::ComponentDependency> GetComponentDependencies() const override;

    std::vector<ecs::SystemDependency> GetSystemDependencies() const override;

    const char* GetName() const override;

    // System-specific configuration methods
    void ConfigureWeaponSlot(int entityId, const std::string& weaponSlot, const WeaponSlotConfig& config);
    bool FireWeapon(EntityManager& entityManager, int entityId, const std::string& weaponSlot);
    bool CanFire(int entityId, const std::string& weaponSlot) const;
    int GetAmmoCount(int entityId, const std::string& weaponSlot) const;

    void UseExternalEngine(std::shared_ptr<physics::IPhysicsEngine> engine);
    void ResetToBuiltin();
    physics::PhysicsBackendType GetActiveBackendType() const noexcept { return activeBackend_; }
    std::shared_ptr<physics::IPhysicsEngine> GetActiveEngine() const noexcept { return externalEngine_; }
    void SetGravity(double x, double y, double z);
    void SetGlobalDamping(double linear, double angular);
    void SetMaxVelocity(double maxVel);
    void SetCollisionEnabled(bool enabled);
    bool Raycast(double originX, double originY, double originZ,
                 double dirX, double dirY, double dirZ,
                 double maxDistance, RaycastHit& hit);
    std::vector<unsigned int> OverlapSphere(double centerX, double centerY, double centerZ,
                                            double radius, unsigned int layerMask = 0xFFFFFFFF);
    std::vector<unsigned int> OverlapBox(double centerX, double centerY, double centerZ,
                                         double width, double height, double depth,
                                         unsigned int layerMask = 0xFFFFFFFF);
    void ApplyForce(unsigned int entity, double fx, double fy, double fz);
    void ApplyImpulse(unsigned int entity, double ix, double iy, double iz);
    void ApplyForceAtPoint(unsigned int entity, double fx, double fy, double fz,
                          double px, double py, double pz);
    double GetGravityX() const { return globalGravityX_; }
    double GetGravityY() const { return globalGravityY_; }
    double GetGravityZ() const { return globalGravityZ_; }

    void SetRandomManager(DeterministicRandom* randomManager);

private:
    // System-specific update methods
    void UpdateWeaponSystem(EntityManager& entityManager, double dt);
    void UpdatePhysicsSystem(EntityManager& entityManager, double dt);
    void UpdateMovementSystem(EntityManager& entityManager, double dt);
    void UpdatePlayerControlSystem(EntityManager& entityManager, double dt);
    void UpdateBehaviorTreeSystem(EntityManager& entityManager, double dt);
    void UpdateLocomotionSystem(EntityManager& entityManager, double dt);
    void UpdateShipAssemblySystem(EntityManager& entityManager, double dt);
    void UpdateSpaceshipPhysicsSystem(EntityManager& entityManager, double dt);
    void UpdateAnimationSystem(EntityManager& entityManager, double dt);
    void UpdateTargetingSystem(EntityManager& entityManager, double dt);
    void UpdateShieldSystem(EntityManager& entityManager, double dt);
    void UpdateNavigationSystem(EntityManager& entityManager, double dt);
    void UpdateGameplayEventSystem(EntityManager& entityManager, double dt);
    void UpdateMissionScriptSystem(EntityManager& entityManager, double dt);
    // Weapon system state
    std::unordered_map<int, std::unordered_map<std::string, float>> weaponCooldowns_;
    std::unordered_map<int, std::unordered_map<std::string, int>> weaponAmmo_;
    std::unordered_map<int, std::unordered_map<std::string, WeaponSlotConfig>> weaponConfigs_;

    // Physics system state
    std::shared_ptr<physics::IPhysicsEngine> externalEngine_;
    physics::PhysicsBackendType activeBackend_ = physics::PhysicsBackendType::BuiltIn;
    double globalGravityX_ = 0.0;
    double globalGravityY_ = 0.0;
    double globalGravityZ_ = -9.8;
    double globalLinearDamping_ = 0.01;
    double globalAngularDamping_ = 0.01;
    double maxVelocity_ = 100.0;
    bool collisionEnabled_ = true;
    std::vector<CollisionPair> currentCollisions_;

    // Behavior tree system state
    DeterministicRandom* randomManager_ = nullptr;

    // Helper methods
    const WeaponSlotConfig* GetWeaponConfig(int entityId, const std::string& weaponSlot) const;
    bool ExtractEntityPosition(EntityManager& entityManager, int entityId, double& x, double& y, double& z) const;

    // Physics helper methods
    void ApplyGravity(double dt);
    void ApplyForces(double dt);
    void ApplyConstantForces(double dt);
    void IntegrateVelocities(double dt);
    void DetectCollisions(double dt);
    void ResolveCollisions(double dt);
    void UpdateCharacterControllers(double dt);
    void UpdateJoints(double dt);
    void ClearFrameForces();
    void RunBuiltinSimulation(double dt);
    std::vector<CollisionPair> DetectCollisionPairs();
    std::vector<CollisionPair> DetectSweptCollisionPairs(double dt);
    bool ComputeSweptAABB(const struct BoxCollider& a, const struct Position& posA,
                          const struct Velocity* velA,
                          const struct BoxCollider& b, const struct Position& posB,
                          const struct Velocity* velB, double dt,
                          CollisionPair& result);
    bool CheckBoxBox(const struct BoxCollider& a, const struct Position& posA,
                     const struct BoxCollider& b, const struct Position& posB,
                     CollisionPair& result);
    bool CheckSphereSphere(const struct SphereCollider& a, const struct Position& posA,
                          const struct SphereCollider& b, const struct Position& posB,
                          CollisionPair& result);
    bool CheckBoxSphere(const struct BoxCollider& box, const struct Position& boxPos,
                       const struct SphereCollider& sphere, const struct Position& spherePos,
                       CollisionPair& result);
    void ResolveCollisionPair(const CollisionPair& pair, double dt);
    void SeparateColliders(unsigned int entityA, unsigned int entityB,
                          double normalX, double normalY, double normalZ,
                          double penetration);
    double DotProduct(double ax, double ay, double az, double bx, double by, double bz) const;
    double VectorLength(double x, double y, double z) const;
    void Normalize(double& x, double& y, double& z) const;
    double Clamp(double value, double min, double max) const;
};

class SystemManager {
public:
    SystemManager() = default;
    ~SystemManager() = default;

    template<typename T, typename... Args>
    T& RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of<System, T>::value, "System must derive from System base class");
        auto registration = std::make_unique<RegisteredSystem>();
        registration->instance = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *static_cast<T*>(registration->instance.get());
        registration->legacyType = std::type_index(typeid(T));
        registration->wrapperType = std::type_index(typeid(LegacySystemWrapper<T>));
        registration->factory = [](SystemManager& manager, RegisteredSystem& registrationRef) {
            return std::make_unique<LegacySystemWrapper<T>>(manager, registrationRef);
        };
        RefreshRegistrationMetadata(*registration);
        systems_.emplace_back(std::move(registration));
        scheduleDirty_ = true;
        metadataDirty_ = true;
        return ref;
    }

    // Specialized method for UnifiedSystem
    UnifiedSystem& RegisterSystem(SystemType systemType) {
        auto registration = std::make_unique<RegisteredSystem>();
        registration->instance = std::make_unique<UnifiedSystem>(systemType);
        UnifiedSystem& ref = *static_cast<UnifiedSystem*>(registration->instance.get());
        registration->legacyType = std::type_index(typeid(UnifiedSystem));
        registration->wrapperType = std::type_index(typeid(LegacySystemWrapper<UnifiedSystem>));
        registration->factory = [](SystemManager& manager, RegisteredSystem& registrationRef) {
            return std::make_unique<LegacySystemWrapper<UnifiedSystem>>(manager, registrationRef);
        };
        RefreshRegistrationMetadata(*registration);
        systems_.emplace_back(std::move(registration));
        scheduleDirty_ = true;
        metadataDirty_ = true;
        return ref;
    }

    void Clear();
    void UpdateAll(EntityManager& entityManager, double dt);

    void SetDocumentationOutputPath(std::string path);

    struct SystemMetadata {
        std::string name;
        std::string legacyTypeName;
        ecs::UpdatePhase phase;
        std::vector<ecs::ComponentDependency> componentDependencies;
        std::vector<ecs::SystemDependency> systemDependencies;
    };

    const std::vector<SystemMetadata>& GetRegisteredSystemMetadata() const;

private:
    struct RegisteredSystem {
        std::unique_ptr<System> instance;
        std::type_index legacyType{typeid(void)};
        std::type_index wrapperType{typeid(void)};
        std::string name;
        ecs::UpdatePhase phase{ecs::UpdatePhase::Simulation};
        std::vector<ecs::ComponentDependency> componentDependencies;
        std::vector<ecs::SystemDependency> systemDependencies;
        std::function<std::unique_ptr<ecs::SystemV2>(SystemManager&, RegisteredSystem&)> factory;
    };

    template<typename LegacySystemType>
    class LegacySystemWrapper;

    void BuildSchedule();
    void RefreshRegistrationMetadata(RegisteredSystem& registration);
    void InvokeLegacyUpdate(RegisteredSystem& registration, double dt);
    void EmitComponentConflicts() const;
    void ExportDocumentation() const;
    std::type_index ResolveWrapperType(const std::type_index& legacyType) const;
    std::type_index ResolveUnifiedSystemType(SystemType systemType) const;
    bool HasComponentConflict(const std::vector<ecs::ComponentDependency>& a,
                              const std::vector<ecs::ComponentDependency>& b) const;

    std::vector<std::unique_ptr<RegisteredSystem>> systems_;
    mutable std::vector<SystemMetadata> metadataCache_;
    mutable bool metadataDirty_ = true;
    mutable std::unordered_map<std::type_index, std::type_index> wrapperTypeLUT_;
    ecs::SystemSchedulerV2 scheduler_{};
    EntityManager* currentEntityManager_ = nullptr;
    bool scheduleDirty_ = true;
    std::string documentationOutputPath_;
};

template<typename LegacySystemType>
class SystemManager::LegacySystemWrapper : public ecs::SystemV2 {
public:
    LegacySystemWrapper(SystemManager& owner, RegisteredSystem& registration)
        : owner_(owner), registration_(registration) {}

    void Update(ecs::EntityManagerV2& entityManager, double dt) override {
        (void)entityManager;
        owner_.InvokeLegacyUpdate(registration_, dt);
    }

    std::vector<ecs::ComponentDependency> GetDependencies() const override {
        return registration_.componentDependencies;
    }

    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        std::vector<ecs::SystemDependency> resolved;
        resolved.reserve(registration_.systemDependencies.size());
        for (const auto& dependency : registration_.systemDependencies) {
            resolved.emplace_back(owner_.ResolveWrapperType(dependency.type));
        }
        return resolved;
    }

    ecs::UpdatePhase GetUpdatePhase() const override {
        return registration_.phase;
    }

    const char* GetName() const override { return registration_.name.c_str(); }

    bool SupportsDuplicateRegistration() const override { return true; }

private:
    SystemManager& owner_;
    RegisteredSystem& registration_;
};

// Include all system headers for convenience
// Note: Individual system classes have been merged into UnifiedSystem
// These headers are kept for reference but are no longer used
