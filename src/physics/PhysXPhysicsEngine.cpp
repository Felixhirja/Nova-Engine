#include "PhysXPhysicsEngine.h"

#include <algorithm>

#include "../ecs/PhysicsSystem.h"

namespace physics {

PhysXPhysicsEngine::PhysXPhysicsEngine() {
    params_.debugName = "PhysXCompatibility";
}

void PhysXPhysicsEngine::Initialize(const PhysicsEngineInitParams& params) {
    params_ = params;
    if (params_.fixedTimeStep <= 0.0) {
        params_.fixedTimeStep = 1.0 / 60.0;
    }
    if (params_.maxSubSteps == 0) {
        params_.maxSubSteps = 4;
    }
    if (params_.debugName.empty()) {
        params_.debugName = "PhysXCompatibility";
    }
    accumulator_ = 0.0;
    lastSubStepCount_ = 0;
}

void PhysXPhysicsEngine::StepSimulation(PhysicsSystem& system, EntityManager& entityManager, double dt) {
    accumulator_ += dt;
    const double step = params_.fixedTimeStep > 0.0 ? params_.fixedTimeStep : dt;
    const unsigned int maxSteps = std::max(1u, params_.maxSubSteps);

    unsigned int performed = 0;
    if (accumulator_ >= step) {
        unsigned int stepsToRun = static_cast<unsigned int>(accumulator_ / step);
        stepsToRun = std::min(stepsToRun, maxSteps);
        for (unsigned int i = 0; i < stepsToRun; ++i) {
            system.StepWithBuiltin(entityManager, step);
        }
        performed += stepsToRun;
        accumulator_ -= step * stepsToRun;
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
