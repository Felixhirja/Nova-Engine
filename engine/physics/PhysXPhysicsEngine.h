#pragma once

#include "PhysicsEngine.h"

namespace physics {

class PhysXPhysicsEngine : public IPhysicsEngine {
public:
    PhysXPhysicsEngine();

    PhysicsBackendType GetBackendType() const noexcept override { return PhysicsBackendType::PhysX; }
    void Initialize(const PhysicsEngineInitParams& params) override;
    void StepSimulation(PhysicsSystem& system, EntityManager& entityManager, double dt) override;
    const PhysicsEngineInitParams& GetInitParams() const noexcept override { return params_; }

    unsigned int GetLastSubStepCount() const noexcept { return lastSubStepCount_; }

    std::optional<RaycastHit> Raycast(double originX, double originY, double originZ,
                                      double dirX, double dirY, double dirZ,
                                      double maxDistance) override;

private:
    PhysicsEngineInitParams params_{};
    double accumulator_ = 0.0;
    unsigned int lastSubStepCount_ = 0;
};

} // namespace physics
