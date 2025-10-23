#include "PlayerControlSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kDefaultAcceleration = 4.0;
constexpr double kDefaultMaxSpeed = 5.0;
constexpr double kDefaultFriction = 0.0;
}

void PlayerControlSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<PlayerController, Velocity>([&](Entity entity, PlayerController& controller, Velocity& velocity) {
        const MovementParameters* movement = entityManager.GetComponent<MovementParameters>(entity);
        auto sanitize = [](double value, double fallback) {
            if (!std::isfinite(value)) {
                return fallback;
            }
            return value;
        };

        double strafeAcceleration = kDefaultAcceleration;
        double forwardAcceleration = kDefaultAcceleration;
        double backwardAcceleration = kDefaultAcceleration;
        double strafeDeceleration = kDefaultAcceleration;
        double forwardDeceleration = kDefaultAcceleration;
        double backwardDeceleration = kDefaultAcceleration;
        double strafeMaxSpeed = kDefaultMaxSpeed;
        double forwardMaxSpeed = kDefaultMaxSpeed;
        double backwardMaxSpeed = kDefaultMaxSpeed;
        double friction = kDefaultFriction;

        if (movement) {
            strafeAcceleration = std::max(0.0, sanitize(movement->strafeAcceleration, kDefaultAcceleration));
            forwardAcceleration = std::max(0.0, sanitize(movement->forwardAcceleration, kDefaultAcceleration));
            backwardAcceleration = std::max(0.0, sanitize(movement->backwardAcceleration, kDefaultAcceleration));
            strafeDeceleration = std::max(0.0, sanitize(movement->strafeDeceleration, kDefaultAcceleration));
            forwardDeceleration = std::max(0.0, sanitize(movement->forwardDeceleration, kDefaultAcceleration));
            backwardDeceleration = std::max(0.0, sanitize(movement->backwardDeceleration, kDefaultAcceleration));
            strafeMaxSpeed = std::max(0.0, sanitize(movement->strafeMaxSpeed, kDefaultMaxSpeed));
            forwardMaxSpeed = std::max(0.0, sanitize(movement->forwardMaxSpeed, kDefaultMaxSpeed));
            backwardMaxSpeed = std::max(0.0, sanitize(movement->backwardMaxSpeed, kDefaultMaxSpeed));
            friction = std::max(0.0, sanitize(movement->friction, kDefaultFriction));
        }

        if (const auto* locomotion = entityManager.GetComponent<LocomotionStateMachine>(entity)) {
            auto applyScale = [](double& value, double scale) {
                if (scale > 0.0 && std::fabs(scale - 1.0) > 1e-6) {
                    value *= scale;
                }
            };
            applyScale(strafeAcceleration, locomotion->runtimeAccelerationMultiplier);
            applyScale(forwardAcceleration, locomotion->runtimeAccelerationMultiplier);
            applyScale(backwardAcceleration, locomotion->runtimeAccelerationMultiplier);
            applyScale(strafeDeceleration, locomotion->runtimeDecelerationMultiplier);
            applyScale(forwardDeceleration, locomotion->runtimeDecelerationMultiplier);
            applyScale(backwardDeceleration, locomotion->runtimeDecelerationMultiplier);
            applyScale(strafeMaxSpeed, locomotion->runtimeMaxSpeedMultiplier);
            applyScale(forwardMaxSpeed, locomotion->runtimeMaxSpeedMultiplier);
            applyScale(backwardMaxSpeed, locomotion->runtimeMaxSpeedMultiplier);
            applyScale(friction, locomotion->runtimeFrictionMultiplier);
        }

        double accelX = 0.0;
        double accelY = 0.0;
        if (controller.strafeLeft) accelX -= strafeAcceleration;
        if (controller.strafeRight) accelX += strafeAcceleration;
        if (controller.moveForward) accelY += forwardAcceleration;
        if (controller.moveBackward) accelY -= backwardAcceleration;

        auto* physics = entityManager.GetComponent<PlayerPhysics>(entity);
        double accelZ = 0.0;
        if (physics) {
            physics->thrustMode = controller.thrustMode;
            if (physics->thrustMode) {
                if (controller.moveUp) accelZ += physics->thrustAcceleration;
                if (controller.moveDown) accelZ -= physics->thrustAcceleration;
            } else {
                if (controller.jumpRequested && physics->isGrounded) {
                    velocity.vz = physics->jumpImpulse;
                    physics->isGrounded = false;
                }
            }
        } else {
            double verticalAcceleration = strafeAcceleration;
            if (controller.moveUp) accelZ += verticalAcceleration;
            if (controller.moveDown) accelZ -= verticalAcceleration;
        }

        controller.jumpRequested = false;

        // Update player facing direction based on movement input
        if (accelY != 0.0 || accelX != 0.0) {
            controller.facingYaw = atan2(accelX, accelY);
        }

        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            acceleration->ax = accelX;
            acceleration->ay = accelY;
            acceleration->az = accelZ;
        }

        // Update velocities directly for responsive feel
        velocity.vx += accelX * dt;
        velocity.vy += accelY * dt;
        if (physics && physics->thrustMode) {
            velocity.vz += accelZ * dt;
        } else if (!physics) {
            velocity.vz += accelZ * dt;
        }

        // Damping when no horizontal input
        auto applyDirectionalDamping = [&](double& vel, double positiveRate, double negativeRate) {
            if (vel > 0.0) {
                vel = std::max(0.0, vel - positiveRate * dt);
            } else if (vel < 0.0) {
                vel = std::min(0.0, vel + negativeRate * dt);
            }
        };
        if (!controller.strafeLeft && !controller.strafeRight) {
            applyDirectionalDamping(velocity.vx, strafeDeceleration, strafeDeceleration);
        }
        if (!controller.moveForward && !controller.moveBackward) {
            applyDirectionalDamping(velocity.vy, forwardDeceleration, backwardDeceleration);
        }

        if (friction > 0.0) {
            auto applyFriction = [&](double& vel) {
                if (vel > 0.0) {
                    vel = std::max(0.0, vel - friction * dt);
                } else if (vel < 0.0) {
                    vel = std::min(0.0, vel + friction * dt);
                }
            };
            applyFriction(velocity.vx);
            applyFriction(velocity.vy);
        }

        if (physics) {
            if (physics->thrustMode) {
                if (!controller.moveUp && !controller.moveDown) {
                    double thrustDamping = physics->thrustDamping;
                    if (velocity.vz > 0.0) velocity.vz = std::max(0.0, velocity.vz - thrustDamping * dt);
                    else if (velocity.vz < 0.0) velocity.vz = std::min(0.0, velocity.vz + thrustDamping * dt);
                }
                if (velocity.vz > physics->maxAscentSpeed) velocity.vz = physics->maxAscentSpeed;
                if (velocity.vz < physics->maxDescentSpeed) velocity.vz = physics->maxDescentSpeed;
            } else {
                if (velocity.vz > physics->maxAscentSpeed) velocity.vz = physics->maxAscentSpeed;
                if (velocity.vz < physics->maxDescentSpeed) velocity.vz = physics->maxDescentSpeed;
            }
        } else {
            double verticalDeceleration = strafeDeceleration;
            double verticalMaxSpeed = strafeMaxSpeed;
            if (!controller.moveUp && !controller.moveDown) {
                if (velocity.vz > 0.0) velocity.vz = std::max(0.0, velocity.vz - verticalDeceleration * dt);
                else if (velocity.vz < 0.0) velocity.vz = std::min(0.0, velocity.vz + verticalDeceleration * dt);
            }
            if (velocity.vz > verticalMaxSpeed) velocity.vz = verticalMaxSpeed;
            else if (velocity.vz < -verticalMaxSpeed) velocity.vz = -verticalMaxSpeed;
        }

        // Clamp horizontal speeds
        if (velocity.vx > strafeMaxSpeed) velocity.vx = strafeMaxSpeed;
        else if (velocity.vx < -strafeMaxSpeed) velocity.vx = -strafeMaxSpeed;
        if (velocity.vy > forwardMaxSpeed) velocity.vy = forwardMaxSpeed;
        else if (velocity.vy < -backwardMaxSpeed) velocity.vy = -backwardMaxSpeed;
    });
}
