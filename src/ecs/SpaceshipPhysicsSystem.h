#pragma once

#include "System.h"
#include "EntityManager.h"

class SpaceshipPhysicsSystem : public System {
public:
    SpaceshipPhysicsSystem() = default;
    void Update(EntityManager& entityManager, double dt) override;
};

