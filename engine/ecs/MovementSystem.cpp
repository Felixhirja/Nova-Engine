#include "MovementSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>
#include <cmath>
#include <limits>

void MovementSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    // Basic movement integration for all entities with Position + Velocity
    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        // Update position with velocity (basic integration)
        position.x += velocity.vx * dt;
        position.y += velocity.vy * dt;
        position.z += velocity.vz * dt;

        // Apply acceleration if present
        if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
            velocity.vx += acceleration->ax * dt;
            velocity.vy += acceleration->ay * dt;
            velocity.vz += acceleration->az * dt;
        }
    });

    // Player physics logic - only for entities with PlayerPhysics
    entityManager.ForEach<Position, Velocity, PlayerPhysics>([&](Entity entity, Position& position, Velocity& velocity, PlayerPhysics& physics) {
        physics.isGrounded = false;

        if (auto* rigidBody = entityManager.GetComponent<RigidBody>(entity)) {
            rigidBody->useGravity = physics.enableGravity;
        }

        if (physics.enableGravity) {
            velocity.vz += physics.gravity * dt;
        }

        if (velocity.vz > physics.maxAscentSpeed) velocity.vz = physics.maxAscentSpeed;
        if (velocity.vz < physics.maxDescentSpeed) velocity.vz = physics.maxDescentSpeed;
    });

    // Movement bounds clamping - only for entities with MovementBounds
    entityManager.ForEach<Position, Velocity, MovementBounds>([&](Entity entity, Position& position, Velocity& velocity, MovementBounds& bounds) {
        auto getLimit = [](double bound, double offset, double extent, bool isMin) {
            if (!std::isfinite(bound)) {
                return isMin ? -std::numeric_limits<double>::infinity()
                              : std::numeric_limits<double>::infinity();
            }
            return isMin ? (bound - offset + extent) : (bound - offset - extent);
        };

        auto clampAxis = [&](double& pos, double& vel, bool clampEnabled, double minBound, double maxBound,
                             double offset, double extent, bool& hitMin) {
            hitMin = false;
            if (!clampEnabled) {
                return;
            }

            const double minAllowed = getLimit(minBound, offset, extent, true);
            const double maxAllowed = getLimit(maxBound, offset, extent, false);

            double lo = minAllowed;
            double hi = maxAllowed;
            if (lo > hi) {
                const double center = (lo + hi) * 0.5;
                lo = center;
                hi = center;
            }

            if (pos < lo) {
                pos = lo;
                vel = 0.0;
                hitMin = std::isfinite(minBound);
            } else if (pos > hi) {
                pos = hi;
                vel = 0.0;
            }
        };

        double offsetX = 0.0;
        double offsetY = 0.0;
        double offsetZ = 0.0;
        double halfWidth = 0.0;
        double halfHeight = 0.0;
        double halfDepth = 0.0;

        if (const auto* collider = entityManager.GetComponent<BoxCollider>(entity)) {
            offsetX = collider->offsetX;
            offsetY = collider->offsetY;
            offsetZ = collider->offsetZ;
            halfWidth = collider->width * 0.5;
            halfHeight = collider->height * 0.5;
            halfDepth = collider->depth * 0.5;
        }

        bool hitFloor = false;
        bool hitDummy = false;
        clampAxis(position.x, velocity.vx, bounds.clampX, bounds.minX, bounds.maxX, offsetX, halfWidth, hitDummy);
        clampAxis(position.y, velocity.vy, bounds.clampY, bounds.minY, bounds.maxY, offsetY, halfHeight, hitDummy);
        clampAxis(position.z, velocity.vz, bounds.clampZ, bounds.minZ, bounds.maxZ, offsetZ, halfDepth, hitFloor);

        // Update grounded state if this entity has PlayerPhysics
        if (auto* physics = entityManager.GetComponent<PlayerPhysics>(entity)) {
            if (hitFloor) {
                physics->isGrounded = true;
                if (velocity.vz < 0.0) {
                    velocity.vz = 0.0;
                }
            }
        }
    });
}
