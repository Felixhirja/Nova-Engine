#pragma once

#include <string>

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

class IPhysicsEngine {
public:
    virtual ~IPhysicsEngine() = default;

    virtual PhysicsBackendType GetBackendType() const noexcept = 0;
    virtual void Initialize(const PhysicsEngineInitParams& params) = 0;
    virtual void StepSimulation(PhysicsSystem& system, EntityManager& entityManager, double dt) = 0;
    virtual const PhysicsEngineInitParams& GetInitParams() const noexcept = 0;
};

std::string ToString(PhysicsBackendType type);

} // namespace physics
