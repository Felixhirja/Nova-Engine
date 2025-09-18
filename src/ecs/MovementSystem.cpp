#include "MovementSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>

void MovementSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& position, Velocity& velocity) {
        position.x += velocity.vx * dt;
        position.y += velocity.vy * dt;

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
        }
    });
}
