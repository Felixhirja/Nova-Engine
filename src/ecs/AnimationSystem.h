#pragma once

#include "System.h"

class AnimationSystem : public System {
public:
    AnimationSystem() = default;
    ~AnimationSystem() override = default;

    void Update(EntityManager& entityManager, double dt) override;
};
