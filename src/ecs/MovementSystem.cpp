#include "MovementSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>

void MovementSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        auto* physics = entityManager.GetComponent<PlayerPhysics>(entity);
        if (physics) {
            physics->isGrounded = false;
            if (physics->enableGravity) {
                velocity.vz += physics->gravity * dt;
                if (velocity.vz < physics->maxDescentSpeed) {
                    velocity.vz = physics->maxDescentSpeed;
                }
            }
        }

        // Update velocity with acceleration
        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            velocity.vx += acceleration->ax * dt;
            velocity.vy += acceleration->ay * dt;
            velocity.vz += acceleration->az * dt;
        }

        if (physics) {
            if (velocity.vz > physics->maxAscentSpeed) velocity.vz = physics->maxAscentSpeed;
            if (velocity.vz < physics->maxDescentSpeed) velocity.vz = physics->maxDescentSpeed;
        }

        // Update position with velocity
        position.x += velocity.vx * dt;
        position.y += velocity.vy * dt;
        position.z += velocity.vz * dt;

        if (auto* bounds = entityManager.GetComponent<MovementBounds>(entity)) {
            if (bounds->clampX) {
                if (position.x > bounds->maxX) {
                    position.x = bounds->maxX;
                    if (velocity.vx > 0.0) {
                        velocity.vx = 0.0;
                    }
                } else if (position.x < bounds->minX) {
                    position.x = bounds->minX;
                    if (velocity.vx < 0.0) {
                        velocity.vx = 0.0;
                    }
                }
            }

            if (bounds->clampY) {
                if (position.y > bounds->maxY) {
                    position.y = bounds->maxY;
                    if (velocity.vy > 0.0) {
                        velocity.vy = 0.0;
                    }
                } else if (position.y < bounds->minY) {
                    position.y = bounds->minY;
                    if (velocity.vy < 0.0) {
                        velocity.vy = 0.0;
                    }
                }
            }

            if (bounds->clampZ) {
                if (position.z > bounds->maxZ) {
                    position.z = bounds->maxZ;
                    if (velocity.vz > 0.0) {
                        velocity.vz = 0.0;
                    }
                } else if (position.z < bounds->minZ) {
                    position.z = bounds->minZ;
                    if (velocity.vz < 0.0) {
                        velocity.vz = 0.0;
                    }
                    if (physics) {
                        physics->isGrounded = true;
                    }
                }
            }
        }
    });
}
