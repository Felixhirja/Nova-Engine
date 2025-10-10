#include "PlayerControlSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPlayerAcceleration = 4.0;
constexpr double kPlayerMaxSpeed = 5.0;
}

void PlayerControlSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<PlayerController, Velocity>([&](Entity entity, PlayerController& controller, Velocity& velocity) {
        double accelX = 0.0;
        double accelY = 0.0;
        if (controller.strafeLeft) accelX -= kPlayerAcceleration;
        if (controller.strafeRight) accelX += kPlayerAcceleration;
        if (controller.moveForward) accelY += kPlayerAcceleration;
        if (controller.moveBackward) accelY -= kPlayerAcceleration;

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
            if (controller.moveUp) accelZ += kPlayerAcceleration;
            if (controller.moveDown) accelZ -= kPlayerAcceleration;
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
        const double damping = kPlayerAcceleration;
        if (!controller.strafeLeft && !controller.strafeRight) {
            if (velocity.vx > 0.0) velocity.vx = std::max(0.0, velocity.vx - damping * dt);
            else if (velocity.vx < 0.0) velocity.vx = std::min(0.0, velocity.vx + damping * dt);
        }
        if (!controller.moveForward && !controller.moveBackward) {
            if (velocity.vy > 0.0) velocity.vy = std::max(0.0, velocity.vy - damping * dt);
            else if (velocity.vy < 0.0) velocity.vy = std::min(0.0, velocity.vy + damping * dt);
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
            if (!controller.moveUp && !controller.moveDown) {
                if (velocity.vz > 0.0) velocity.vz = std::max(0.0, velocity.vz - damping * dt);
                else if (velocity.vz < 0.0) velocity.vz = std::min(0.0, velocity.vz + damping * dt);
            }
            if (velocity.vz > kPlayerMaxSpeed) velocity.vz = kPlayerMaxSpeed;
            else if (velocity.vz < -kPlayerMaxSpeed) velocity.vz = -kPlayerMaxSpeed;
        }

        // Clamp horizontal speeds
        if (velocity.vx > kPlayerMaxSpeed) velocity.vx = kPlayerMaxSpeed;
        else if (velocity.vx < -kPlayerMaxSpeed) velocity.vx = -kPlayerMaxSpeed;
        if (velocity.vy > kPlayerMaxSpeed) velocity.vy = kPlayerMaxSpeed;
        else if (velocity.vy < -kPlayerMaxSpeed) velocity.vy = -kPlayerMaxSpeed;
    });
}
