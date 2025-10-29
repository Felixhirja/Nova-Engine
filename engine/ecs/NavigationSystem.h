#pragma once

#include "System.h"
#include "navigation/NavigationGridBuilder.h"

class NavigationSystem : public System {
public:
    void Update(EntityManager& entityManager, double dt) override;
    const char* GetName() const override { return "NavigationSystem"; }

private:
    navigation::NavigationGridBuilder builder_;
};

