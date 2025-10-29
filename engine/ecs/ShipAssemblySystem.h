#pragma once

#include "EntityManager.h"
#include "System.h"

class ShipAssemblySystem : public System {
public:
    void Update(EntityManager& entityManager, double dt) override {
        (void)entityManager;
        (void)dt;
        // Placeholder system – full implementation tracked separately.
    }
};

