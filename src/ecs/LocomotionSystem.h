#pragma once

#include "System.h"

class LocomotionSystem : public System {
public:
    LocomotionSystem() = default;
    ~LocomotionSystem() override = default;

    void Update(EntityManager& entityManager, double dt) override;
};
