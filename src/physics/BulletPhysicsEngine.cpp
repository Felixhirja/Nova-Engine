#include "BulletPhysicsEngine.h"

#include <algorithm>

#include "../ecs/PhysicsSystem.h"

namespace physics {

BulletPhysicsEngine::BulletPhysicsEngine() {
    params_.debugName = "BulletCompatibility";
}

void BulletPhysicsEngine::Initialize(const PhysicsEngineInitParams& params) {
    params_ = params;
    if (params_.fixedTimeStep <= 0.0) {
        params_.fixedTimeStep = 1.0 / 60.0;
    }
    if (params_.maxSubSteps == 0) {
        params_.maxSubSteps = 60;
    }
    if (params_.debugName.empty()) {
        params_.debugName = "BulletCompatibility";
    }
    accumulator_ = 0.0;
    lastSubStepCount_ = 0;
}

void BulletPhysicsEngine::StepSimulation(PhysicsSystem& system, EntityManager& entityManager, double dt) {
    accumulator_ += dt;
    const double step = params_.fixedTimeStep > 0.0 ? params_.fixedTimeStep : dt;
    const unsigned int maxSteps = std::max(1u, params_.maxSubSteps);

    unsigned int performed = 0;
    while (accumulator_ + 1e-9 >= step && performed < maxSteps) {
        system.StepWithBuiltin(entityManager, step);
        accumulator_ -= step;
        ++performed;
    }

    if (performed == 0) {
        system.StepWithBuiltin(entityManager, dt);
        accumulator_ = 0.0;
        performed = 1;
    } else if (accumulator_ > 1e-6) {
        system.StepWithBuiltin(entityManager, accumulator_);
        accumulator_ = 0.0;
        ++performed;
    }

    lastSubStepCount_ = performed;
}

} // namespace physics
