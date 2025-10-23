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

HazardModifier CombineHazards(const HazardModifier& a, const HazardModifier& b) {
    HazardModifier combined;
    combined.speedMultiplier = a.speedMultiplier * b.speedMultiplier;
    combined.accelerationMultiplier = a.accelerationMultiplier * b.accelerationMultiplier;
    combined.gravityMultiplier = a.gravityMultiplier * b.gravityMultiplier;
    combined.staminaDrainRate = a.staminaDrainRate + b.staminaDrainRate;
    combined.heatGainRate = a.heatGainRate + b.heatGainRate;
    return combined;
}

SurfaceMovementProfile CombineProfiles(const SurfaceMovementProfile& baseProfile,
                                       const SurfaceMovementProfile& overrides,
                                       bool useOverride) {
    if (useOverride) {
        return overrides;
    }

    SurfaceMovementProfile combined = baseProfile;
    combined.accelerationMultiplier *= overrides.accelerationMultiplier;
    combined.decelerationMultiplier *= overrides.decelerationMultiplier;
    combined.maxSpeedMultiplier *= overrides.maxSpeedMultiplier;
    combined.jumpImpulseMultiplier *= overrides.jumpImpulseMultiplier;
    combined.gravityMultiplier *= overrides.gravityMultiplier;
    combined.frictionMultiplier *= overrides.frictionMultiplier;
    return combined;
}

void NormalizeWeights(LocomotionStateMachine::Weights& weights) {
    const double sum = weights.idle + weights.walk + weights.sprint +
                       weights.airborne + weights.landing +
                       weights.crouch + weights.slide;
    if (sum <= 1e-6) {
        weights.idle = 1.0;
        weights.walk = 0.0;
        weights.sprint = 0.0;
        weights.airborne = 0.0;
        weights.landing = 0.0;
        weights.crouch = 0.0;
        weights.slide = 0.0;
        return;
    }

    weights.idle /= sum;
    weights.walk /= sum;
    weights.sprint /= sum;
    weights.airborne /= sum;
    weights.landing /= sum;
    weights.crouch /= sum;
    weights.slide /= sum;
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
        case LocomotionStateMachine::State::Crouch:
            weights.crouch = value;
            break;
        case LocomotionStateMachine::State::Slide:
            weights.slide = value;
            break;
    }
}

LocomotionStateMachine::State DetermineGroundState(const LocomotionStateMachine& locomotion,
                                                   double horizontalSpeed,
                                                   bool hasMovementInput,
                                                   bool sprintRequested) {
    if (sprintRequested) {
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
    current.crouch = Approach(current.crouch, targets.crouch, locomotion.blendSmoothing, dt);
    current.slide = Approach(current.slide, targets.slide, locomotion.blendSmoothing, dt);
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
            auto* collision = entityManager.GetComponent<CollisionInfo>(entity);

            if (movement) {
                const double forwardMax = std::max(0.0, movement->forwardMaxSpeed);
                const double backwardMax = std::max(0.0, movement->backwardMaxSpeed);
                const double strafeMax = std::max(0.0, movement->strafeMaxSpeed);
                const double baseSpeed = std::max(forwardMax, std::max(backwardMax, strafeMax));
                if (baseSpeed > 0.0) {
                    locomotion.idleSpeedThreshold = std::max(0.1, baseSpeed * 0.1);
                    locomotion.walkSpeedThreshold = std::max(locomotion.idleSpeedThreshold + 0.1, baseSpeed * 0.4);
                    locomotion.sprintSpeedThreshold = std::max(locomotion.walkSpeedThreshold + 0.1, baseSpeed * 0.85);
                    locomotion.slideSpeedThreshold = std::max(locomotion.walkSpeedThreshold, baseSpeed * 0.65);
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
                 controller->strafeLeft || controller->strafeRight ||
                 controller->moveLeft || controller->moveRight);

            LocomotionSurfaceType surfaceType = locomotion.defaultSurfaceType;
            SurfaceMovementProfile surfaceProfile = locomotion.surfaceProfiles.count(surfaceType)
                                                        ? locomotion.surfaceProfiles.at(surfaceType)
                                                        : SurfaceMovementProfile{};
            HazardModifier hazardModifier = locomotion.hazardBaseline;

            if (collision) {
                for (const auto& contact : collision->contacts) {
                    const auto* surface = entityManager.GetComponent<EnvironmentSurface>(contact.otherEntity);
                    if (!surface) {
                        continue;
                    }

                    SurfaceMovementProfile profileOverride = surface->movementProfile;
                    surfaceType = surface->surfaceType;
                    if (surfaceType == LocomotionSurfaceType::Unknown) {
                        surfaceType = locomotion.defaultSurfaceType;
                    }

                    SurfaceMovementProfile baseProfile = locomotion.surfaceProfiles.count(surfaceType)
                                                             ? locomotion.surfaceProfiles.at(surfaceType)
                                                             : SurfaceMovementProfile{};
                    surfaceProfile = CombineProfiles(baseProfile, profileOverride, surface->overridesProfile);
                    if (surface->isHazard) {
                        hazardModifier = CombineHazards(hazardModifier, surface->hazardModifier);
                    }

                    if (contact.normalZ > 0.2) {
                        grounded = true;
                    }
                }
            }

            locomotion.activeSurfaceType = surfaceType;
            locomotion.activeSurfaceProfile = surfaceProfile;
            locomotion.activeHazardModifier = hazardModifier;

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

            const bool wantsSlide = controller && controller->slide;
            const bool wantsCrouch = controller && controller->crouch;
            bool wantsSprint = controller && controller->sprint && locomotion.stamina > 0.0;
            const bool wantsBoost = controller && controller->boost;

            if (locomotion.slideTimer > 0.0) {
                locomotion.slideTimer = std::max(0.0, locomotion.slideTimer - dt);
            }
            if (locomotion.slideCooldownTimer > 0.0) {
                locomotion.slideCooldownTimer = std::max(0.0, locomotion.slideCooldownTimer - dt);
            }

            bool slidingActive = locomotion.slideTimer > 0.0;
            const bool slideReady = locomotion.slideCooldownTimer <= 0.0;
            if (grounded && wantsSlide && slideReady && horizontalSpeed >= locomotion.slideSpeedThreshold) {
                slidingActive = true;
                locomotion.slideTimer = locomotion.slideDuration;
                locomotion.slideCooldownTimer = locomotion.slideDuration + locomotion.slideCooldown;
            }

            if (slidingActive && !grounded) {
                slidingActive = false;
                locomotion.slideTimer = 0.0;
            }

            if (slidingActive) {
                wantsSprint = false;
            }

            if (wantsSprint && (!grounded || locomotion.stamina <= 0.0)) {
                wantsSprint = false;
            }

            if (wantsSprint) {
                locomotion.stamina = std::max(0.0, locomotion.stamina - locomotion.sprintStaminaCost * dt);
            } else {
                locomotion.stamina = std::min(locomotion.maxStamina,
                                              locomotion.stamina + locomotion.staminaRegenRate * dt);
            }

            if (hazardModifier.staminaDrainRate > 0.0) {
                locomotion.stamina = std::max(0.0, locomotion.stamina - hazardModifier.staminaDrainRate * dt);
            }

            bool boostAllowed = locomotion.heat < locomotion.maxHeat - 1e-6;
            if (wantsBoost && boostAllowed) {
                locomotion.boostTimer = locomotion.boostDuration;
                locomotion.heat = std::min(locomotion.maxHeat,
                                           locomotion.heat + locomotion.boostHeatCostPerSecond * dt);
            } else if (locomotion.boostTimer > 0.0) {
                locomotion.boostTimer = std::max(0.0, locomotion.boostTimer - dt);
            }

            if (hazardModifier.heatGainRate > 0.0) {
                locomotion.heat = std::min(locomotion.maxHeat,
                                           locomotion.heat + hazardModifier.heatGainRate * dt);
            }

            if (locomotion.boostTimer <= 0.0) {
                locomotion.heat = std::max(0.0, locomotion.heat - locomotion.heatDissipationRate * dt);
            }

            const auto groundState = DetermineGroundState(locomotion, horizontalSpeed, hasMovementInput, wantsSprint);

            LocomotionStateMachine::State targetState = locomotion.currentState;
            if (!grounded) {
                targetState = LocomotionStateMachine::State::Airborne;
            } else if (landingActive) {
                targetState = LocomotionStateMachine::State::Landing;
            } else if (slidingActive) {
                targetState = LocomotionStateMachine::State::Slide;
            } else if (wantsCrouch) {
                targetState = LocomotionStateMachine::State::Crouch;
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
            targetWeights.crouch = 0.0;
            targetWeights.slide = 0.0;

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

            double accelerationScale = surfaceProfile.accelerationMultiplier * hazardModifier.accelerationMultiplier;
            double decelerationScale = surfaceProfile.decelerationMultiplier * hazardModifier.accelerationMultiplier;
            double maxSpeedScale = surfaceProfile.maxSpeedMultiplier * hazardModifier.speedMultiplier;
            double gravityScale = surfaceProfile.gravityMultiplier * hazardModifier.gravityMultiplier;
            double frictionScale = surfaceProfile.frictionMultiplier;
            double jumpImpulseScale = surfaceProfile.jumpImpulseMultiplier * hazardModifier.gravityMultiplier;

            switch (locomotion.currentState) {
                case LocomotionStateMachine::State::Sprint:
                    accelerationScale *= locomotion.sprintAccelerationMultiplier;
                    decelerationScale *= locomotion.sprintAccelerationMultiplier;
                    maxSpeedScale *= locomotion.sprintSpeedMultiplier;
                    jumpImpulseScale *= 1.05;
                    break;
                case LocomotionStateMachine::State::Crouch:
                    accelerationScale *= locomotion.crouchAccelerationMultiplier;
                    decelerationScale *= locomotion.crouchAccelerationMultiplier;
                    maxSpeedScale *= locomotion.crouchSpeedMultiplier;
                    frictionScale *= 1.2;
                    jumpImpulseScale *= 0.8;
                    break;
                case LocomotionStateMachine::State::Slide:
                    maxSpeedScale *= locomotion.slideSpeedMultiplier;
                    decelerationScale *= locomotion.slideDecelerationMultiplier;
                    frictionScale *= 0.5;
                    jumpImpulseScale *= 1.05;
                    break;
                case LocomotionStateMachine::State::Airborne:
                    accelerationScale *= locomotion.airborneAccelerationMultiplier;
                    decelerationScale *= locomotion.airborneAccelerationMultiplier;
                    frictionScale *= 0.9;
                    break;
                default:
                    break;
            }

            if (locomotion.boostTimer > 0.0) {
                accelerationScale *= locomotion.boostAccelerationMultiplier;
                maxSpeedScale *= locomotion.boostSpeedMultiplier;
                jumpImpulseScale *= locomotion.boostAccelerationMultiplier;
            }

            locomotion.runtimeAccelerationMultiplier = accelerationScale;
            locomotion.runtimeDecelerationMultiplier = decelerationScale;
            locomotion.runtimeMaxSpeedMultiplier = maxSpeedScale;
            locomotion.runtimeGravityMultiplier = gravityScale;
            locomotion.runtimeFrictionMultiplier = frictionScale;
            locomotion.runtimeJumpImpulseMultiplier = jumpImpulseScale;
            locomotion.boostActive = (locomotion.boostTimer > 0.0);

            double targetCameraOffset = locomotion.defaultCameraOffset;
            if (locomotion.currentState == LocomotionStateMachine::State::Crouch) {
                targetCameraOffset = locomotion.crouchCameraOffset;
            } else if (locomotion.currentState == LocomotionStateMachine::State::Slide) {
                targetCameraOffset = locomotion.slideCameraOffset;
            }

            locomotion.currentCameraOffset = Approach(locomotion.currentCameraOffset,
                                                     targetCameraOffset,
                                                     locomotion.cameraSmoothing,
                                                     dt);

            if (physics) {
                if (!locomotion.baseGravityInitialized) {
                    locomotion.baseGravity = physics->gravity;
                    locomotion.baseGravityInitialized = true;
                }
                if (!locomotion.baseJumpInitialized) {
                    locomotion.baseJumpImpulse = physics->jumpImpulse;
                    locomotion.baseJumpInitialized = true;
                }
                physics->gravity = locomotion.baseGravity * locomotion.runtimeGravityMultiplier;
                physics->jumpImpulse = locomotion.baseJumpImpulse * locomotion.runtimeJumpImpulseMultiplier;
            }
        });
}
