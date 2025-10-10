#pragma once

#include "ecs/System.h"
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

// Targeting system for lock-on mechanics and range checks
class TargetingSystem : public System {
public:
    TargetingSystem();

    void Update(EntityManager& entityManager, double dt) override;

    // Attempt to lock onto a target entity
    bool LockOn(EntityManager& entityManager, int shooterEntity, int targetEntity);

    // Release lock
    void ReleaseLock(int shooterEntity);

    // Get current target for shooter
    int GetTarget(int shooterEntity) const;

    // Check if target is in range and valid
    bool IsValidTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const;

    // Calculate lead position for projectile
    glm::vec3 CalculateLeadPosition(EntityManager& entityManager, int shooterEntity, int targetEntity, float projectileSpeed) const;

private:
    // Map shooter entity to target entity
    std::unordered_map<int, int> lockedTargets_;

    // Targeting range in km (convert to world units)
    float targetingRangeKm_ = 10.0f;

    // Lock-on time in seconds
    float lockOnTime_ = 2.0f;

    // Current lock progress
    std::unordered_map<int, float> lockProgress_;
};