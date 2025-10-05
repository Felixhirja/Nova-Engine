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
        double targetAcceleration = 0.0;
        if (controller.moveLeft && !controller.moveRight) {
            targetAcceleration = -kPlayerAcceleration;
        } else if (controller.moveRight && !controller.moveLeft) {
            targetAcceleration = kPlayerAcceleration;
        }

        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            acceleration->ax = targetAcceleration;
        }

        velocity.vx += targetAcceleration * dt;

        if (!controller.moveLeft && !controller.moveRight) {
            const double damping = kPlayerAcceleration;
            if (velocity.vx > 0.0) {
                velocity.vx = std::max(0.0, velocity.vx - damping * dt);
            } else if (velocity.vx < 0.0) {
                velocity.vx = std::min(0.0, velocity.vx + damping * dt);
            }
        }

        if (velocity.vx > kPlayerMaxSpeed) {
            velocity.vx = kPlayerMaxSpeed;
        } else if (velocity.vx < -kPlayerMaxSpeed) {
            velocity.vx = -kPlayerMaxSpeed;
        }
    });
}
