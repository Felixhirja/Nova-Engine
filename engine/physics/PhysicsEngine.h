#pragma once

#include <string>
#include <optional>

class EntityManager;
class PhysicsSystem;

namespace physics {

enum class PhysicsBackendType {
    BuiltIn,
    Bullet,
    PhysX
};

struct PhysicsEngineInitParams {
    double fixedTimeStep = 1.0 / 60.0;
    unsigned int maxSubSteps = 4;
    bool enableContinuousCollision = false;
    std::string debugName;
};

struct RaycastHit {
    double hitX, hitY, hitZ;     // Hit point
    double normalX, normalY, normalZ; // Surface normal
    double distance;             // Distance from ray origin
    // Could add entity ID, material, etc. later
};

class IPhysicsEngine {
public:
    virtual ~IPhysicsEngine() = default;

    virtual PhysicsBackendType GetBackendType() const noexcept = 0;
    virtual void Initialize(const PhysicsEngineInitParams& params) = 0;
    virtual void StepSimulation(PhysicsSystem& system, EntityManager& entityManager, double dt) = 0;
    virtual const PhysicsEngineInitParams& GetInitParams() const noexcept = 0;

    // Raycast from origin in direction, up to maxDistance
    virtual std::optional<RaycastHit> Raycast(double originX, double originY, double originZ,
                                              double dirX, double dirY, double dirZ,
                                              double maxDistance) = 0;

protected:
    PhysicsSystem* physicsSystem_ = nullptr;
};

std::string ToString(PhysicsBackendType type);

} // namespace physics
