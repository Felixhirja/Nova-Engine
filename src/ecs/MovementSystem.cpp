#include "MovementSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>

void MovementSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        auto* rigidBody = entityManager.GetComponent<RigidBody>(entity);
        bool usesPhysicsIntegration = (rigidBody != nullptr);

        auto* physics = entityManager.GetComponent<PlayerPhysics>(entity);
        if (physics) {
            physics->isGrounded = false;
            if (rigidBody) {
                rigidBody->useGravity = physics->enableGravity;
            }
            if (!usesPhysicsIntegration && physics->enableGravity) {
                velocity.vz += physics->gravity * dt;
            }
            if (velocity.vz > physics->maxAscentSpeed) velocity.vz = physics->maxAscentSpeed;
            if (velocity.vz < physics->maxDescentSpeed) velocity.vz = physics->maxDescentSpeed;
        }

        // Update velocity with acceleration
        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            velocity.vx += acceleration->ax * dt;
            velocity.vy += acceleration->ay * dt;
            velocity.vz += acceleration->az * dt;
        }

        // Update position with velocity
        if (!usesPhysicsIntegration) {
            position.x += velocity.vx * dt;
            position.y += velocity.vy * dt;
            position.z += velocity.vz * dt;
        }
    });
}
