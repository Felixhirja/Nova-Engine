#include "LocomotionSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>
#include <cmath>

namespace {

double HorizontalSpeed(const Velocity& velocity) {
    return std::sqrt(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
}

double Approach(double current, double target, double rate, double dt) {
    if (rate <= 0.0 || dt <= 0.0) {
        return target;
    }

    const double delta = target - current;
    const double maxStep = rate * dt;
    if (std::fabs(delta) <= maxStep) {
        return target;
    }
    return current + (delta > 0.0 ? maxStep : -maxStep);
}

void NormalizeWeights(LocomotionStateMachine::Weights& weights) {
    const double sum = weights.idle + weights.walk + weights.sprint +
                       weights.airborne + weights.landing;
    if (sum <= 1e-6) {
        weights.idle = 1.0;
        weights.walk = 0.0;
        weights.sprint = 0.0;
        weights.airborne = 0.0;
        weights.landing = 0.0;
        return;
    }

    weights.idle /= sum;
    weights.walk /= sum;
    weights.sprint /= sum;
    weights.airborne /= sum;
    weights.landing /= sum;
}

void SetWeight(LocomotionStateMachine::Weights& weights,
               LocomotionStateMachine::State state,
               double value) {
    switch (state) {
        case LocomotionStateMachine::State::Idle:
            weights.idle = value;
            break;
        case LocomotionStateMachine::State::Walk:
            weights.walk = value;
            break;
        case LocomotionStateMachine::State::Sprint:
            weights.sprint = value;
            break;
        case LocomotionStateMachine::State::Airborne:
            weights.airborne = value;
            break;
        case LocomotionStateMachine::State::Landing:
            weights.landing = value;
            break;
    }
}

LocomotionStateMachine::State DetermineGroundState(const LocomotionStateMachine& locomotion,
                                                   double horizontalSpeed,
                                                   bool hasMovementInput) {
    if (horizontalSpeed >= locomotion.sprintSpeedThreshold) {
        return LocomotionStateMachine::State::Sprint;
    }

    if (!hasMovementInput && horizontalSpeed < locomotion.walkSpeedThreshold) {
        return LocomotionStateMachine::State::Idle;
    }

    if (horizontalSpeed >= locomotion.walkSpeedThreshold) {
        return LocomotionStateMachine::State::Walk;
    }

    if (hasMovementInput) {
        return LocomotionStateMachine::State::Walk;
    }

    if (horizontalSpeed <= locomotion.idleSpeedThreshold) {
        return LocomotionStateMachine::State::Idle;
    }

    return LocomotionStateMachine::State::Walk;
}

void ApplyBlendTargets(LocomotionStateMachine& locomotion,
                       const LocomotionStateMachine::Weights& targets,
                       double dt) {
    LocomotionStateMachine::Weights current = locomotion.blendWeights;
    current.idle = Approach(current.idle, targets.idle, locomotion.blendSmoothing, dt);
    current.walk = Approach(current.walk, targets.walk, locomotion.blendSmoothing, dt);
    current.sprint = Approach(current.sprint, targets.sprint, locomotion.blendSmoothing, dt);
    current.airborne = Approach(current.airborne, targets.airborne, locomotion.blendSmoothing, dt);
    current.landing = Approach(current.landing, targets.landing, locomotion.blendSmoothing, dt);
    NormalizeWeights(current);
    locomotion.blendWeights = current;
}

}  // namespace

void LocomotionSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<LocomotionStateMachine, Velocity>(
        [&](Entity entity, LocomotionStateMachine& locomotion, Velocity& velocity) {
            const auto* movement = entityManager.GetComponent<MovementParameters>(entity);
            const auto* controller = entityManager.GetComponent<PlayerController>(entity);
            auto* physics = entityManager.GetComponent<PlayerPhysics>(entity);

            if (movement) {
                const double forwardMax = std::max(0.0, movement->forwardMaxSpeed);
                const double backwardMax = std::max(0.0, movement->backwardMaxSpeed);
                const double strafeMax = std::max(0.0, movement->strafeMaxSpeed);
                const double baseSpeed = std::max(forwardMax, std::max(backwardMax, strafeMax));
                if (baseSpeed > 0.0) {
                    locomotion.idleSpeedThreshold = std::max(0.1, baseSpeed * 0.1);
                    locomotion.walkSpeedThreshold = std::max(locomotion.idleSpeedThreshold + 0.1, baseSpeed * 0.4);
                    locomotion.sprintSpeedThreshold = std::max(locomotion.walkSpeedThreshold + 0.1, baseSpeed * 0.85);
                }
            }

            const double horizontalSpeed = HorizontalSpeed(velocity);
            bool grounded = true;
            if (physics) {
                grounded = physics->isGrounded;
            } else if (std::fabs(velocity.vz) > locomotion.airborneVerticalSpeedThreshold) {
                grounded = false;
            }

            const bool hasMovementInput = controller &&
                (controller->moveForward || controller->moveBackward ||
                 controller->strafeLeft || controller->strafeRight);

            const bool wasGrounded = locomotion.wasGrounded;
            const bool justLanded = (!wasGrounded && grounded);
            locomotion.wasGrounded = grounded;

            if (!grounded) {
                locomotion.landingTimer = 0.0;
            } else if (justLanded) {
                locomotion.landingTimer = locomotion.landingDuration;
            }

            const bool landingActive = grounded && locomotion.landingTimer > 0.0 &&
                (justLanded || locomotion.currentState == LocomotionStateMachine::State::Landing);

            const auto groundState = DetermineGroundState(locomotion, horizontalSpeed, hasMovementInput);

            LocomotionStateMachine::State targetState = locomotion.currentState;
            if (!grounded) {
                targetState = LocomotionStateMachine::State::Airborne;
            } else if (landingActive) {
                targetState = LocomotionStateMachine::State::Landing;
            } else {
                targetState = groundState;
            }

            if (targetState != locomotion.currentState) {
                locomotion.previousState = locomotion.currentState;
                locomotion.currentState = targetState;
                locomotion.timeInState = 0.0;
            } else {
                locomotion.timeInState += dt;
            }

            LocomotionStateMachine::Weights targetWeights;
            targetWeights.idle = 0.0;
            targetWeights.walk = 0.0;
            targetWeights.sprint = 0.0;
            targetWeights.airborne = 0.0;
            targetWeights.landing = 0.0;

            if (targetState == LocomotionStateMachine::State::Landing) {
                SetWeight(targetWeights, LocomotionStateMachine::State::Landing, 1.0);
                double landingBlend = 1.0;
                if (locomotion.landingDuration > 1e-6) {
                    landingBlend = std::clamp(1.0 - (locomotion.landingTimer / locomotion.landingDuration), 0.0, 1.0);
                }
                if (groundState != LocomotionStateMachine::State::Landing) {
                    SetWeight(targetWeights, groundState, landingBlend);
                }
            } else {
                SetWeight(targetWeights, targetState, 1.0);
            }

            ApplyBlendTargets(locomotion, targetWeights, dt);

            if (grounded && locomotion.landingTimer > 0.0) {
                locomotion.landingTimer = std::max(0.0, locomotion.landingTimer - dt);
            }
        });
}
