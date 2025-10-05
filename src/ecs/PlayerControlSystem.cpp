#include "PlayerControlSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>

namespace {
constexpr double kPlayerAcceleration = 4.0;
constexpr double kPlayerMaxSpeed = 5.0;
}

void PlayerControlSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<PlayerController, Velocity>([&](Entity entity, PlayerController& controller, Velocity& velocity) {
        // Compute desired acceleration in local space
        double localAx = 0.0, localAy = 0.0, localAz = 0.0;
        if (controller.moveForward) localAx += kPlayerAcceleration;
        if (controller.moveBackward) localAx -= kPlayerAcceleration;
        if (controller.moveUp) localAy += kPlayerAcceleration;
        if (controller.moveDown) localAy -= kPlayerAcceleration;
        if (controller.strafeLeft) localAz -= kPlayerAcceleration;
        if (controller.strafeRight) localAz += kPlayerAcceleration;

        // Rotate by camera yaw to get world acceleration
        double cosYaw = cos(controller.cameraYaw);
        double sinYaw = sin(controller.cameraYaw);
        double worldAx = localAx * cosYaw - localAz * sinYaw;
        double worldAz = localAx * sinYaw + localAz * cosYaw;
        double worldAy = localAy; // up/down not rotated

        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            acceleration->ax = worldAx;
            acceleration->ay = worldAy;
            acceleration->az = worldAz;
        }

        // Update velocities
        velocity.vx += worldAx * dt;
        velocity.vy += worldAy * dt;
        velocity.vz += worldAz * dt; // Assuming we add az handling

        // Damping when no input
        const double damping = kPlayerAcceleration;
        if (!controller.moveForward && !controller.moveBackward) {
            if (velocity.vx > 0.0) velocity.vx = std::max(0.0, velocity.vx - damping * dt);
            else if (velocity.vx < 0.0) velocity.vx = std::min(0.0, velocity.vx + damping * dt);
        }
        if (!controller.moveUp && !controller.moveDown) {
            if (velocity.vy > 0.0) velocity.vy = std::max(0.0, velocity.vy - damping * dt);
            else if (velocity.vy < 0.0) velocity.vy = std::min(0.0, velocity.vy + damping * dt);
        }
        if (!controller.strafeLeft && !controller.strafeRight) {
            if (velocity.vz > 0.0) velocity.vz = std::max(0.0, velocity.vz - damping * dt);
            else if (velocity.vz < 0.0) velocity.vz = std::min(0.0, velocity.vz + damping * dt);
        }

        // Clamp speeds
        if (velocity.vx > kPlayerMaxSpeed) velocity.vx = kPlayerMaxSpeed;
        else if (velocity.vx < -kPlayerMaxSpeed) velocity.vx = -kPlayerMaxSpeed;
        if (velocity.vy > kPlayerMaxSpeed) velocity.vy = kPlayerMaxSpeed;
        else if (velocity.vy < -kPlayerMaxSpeed) velocity.vy = -kPlayerMaxSpeed;
        if (velocity.vz > kPlayerMaxSpeed) velocity.vz = kPlayerMaxSpeed;
        else if (velocity.vz < -kPlayerMaxSpeed) velocity.vz = -kPlayerMaxSpeed;
    });
}
