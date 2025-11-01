#pragma once

#include "ecs/System.h"
#include <functional>
#include <unordered_map>
#include <vector>

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

// Targeting system for lock-on mechanics and range checks
class TargetingSystem : public UnifiedSystem {
public:
    TargetingSystem();

    void Update(EntityManager& entityManager, double dt) override;

    // Attempt to lock onto a target entity
    bool LockOn(EntityManager& entityManager, int shooterEntity, int targetEntity);

    // Release lock
    void ReleaseLock(int shooterEntity);

    // Get current target for shooter
    int GetTarget(int shooterEntity) const;

    bool AcquireTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const;

    // Check if target is in range and valid
    bool IsValidTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const;

    // Calculate lead position for projectile
    Vec3 CalculateLeadPosition(EntityManager& entityManager, int shooterEntity, int targetEntity, float projectileSpeed) const;

    void SetLineOfSightValidator(std::function<bool(const Vec3&, const Vec3&)> validator);

private:
    // Map shooter entity to target entity
    std::unordered_map<int, int> lockedTargets_;

    // Targeting range in km (convert to world units)
    float targetingRangeKm_ = 10.0f;

    // Lock-on time in seconds
    float lockOnTime_ = 2.0f;

    // Current lock progress
    std::unordered_map<int, float> lockProgress_;

    std::function<bool(const Vec3&, const Vec3&)> lineOfSightValidator_;

    bool ExtractPosition(EntityManager& entityManager, int entity, Vec3& outPosition) const;
};
