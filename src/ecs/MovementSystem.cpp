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

    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        auto* rigidBody = entityManager.GetComponent<RigidBody>(entity);

        auto* physics = entityManager.GetComponent<PlayerPhysics>(entity);
        if (physics) {
            physics->isGrounded = false;
            if (rigidBody) {
                rigidBody->useGravity = physics->enableGravity;
            }
            if (physics->enableGravity) {
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

        // Update position with velocity (integrate even when using rigid bodies for now)
        position.x += velocity.vx * dt;
        position.y += velocity.vy * dt;
        position.z += velocity.vz * dt;

        if (auto* bounds = entityManager.GetComponent<MovementBounds>(entity)) {
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
            clampAxis(position.x, velocity.vx, bounds->clampX, bounds->minX, bounds->maxX, offsetX, halfWidth, hitDummy);
            clampAxis(position.y, velocity.vy, bounds->clampY, bounds->minY, bounds->maxY, offsetY, halfHeight, hitDummy);
            clampAxis(position.z, velocity.vz, bounds->clampZ, bounds->minZ, bounds->maxZ, offsetZ, halfDepth, hitFloor);

            if (physics && hitFloor) {
                physics->isGrounded = true;
                if (velocity.vz < 0.0) {
                    velocity.vz = 0.0;
                }
            }
        }
    });
}
