#pragma once

#include "System.h"

class PlayerControlSystem : public System {
public:
    PlayerControlSystem() = default;
    ~PlayerControlSystem() override = default;

    void Update(EntityManager& entityManager, double dt) override;
};
