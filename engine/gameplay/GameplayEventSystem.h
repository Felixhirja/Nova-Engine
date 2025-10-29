#pragma once

#include "ecs/System.h"

class GameplayEventSystem : public System {
public:
    void Update(EntityManager& entityManager, double dt) override;
    const char* GetName() const override { return "GameplayEventSystem"; }
};

