#pragma once

#include "System.h"

class MovementSystem : public System {
public:
    MovementSystem() = default;
    ~MovementSystem() override = default;

    void Update(EntityManager& entityManager, double dt) override;
};
