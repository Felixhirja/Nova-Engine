#include "TargetingSystem.h"
#include "ecs/Components.h"
#include "ecs/EntityManager.h"
#include <cmath>
#include <limits>
#include <utility>

TargetingSystem::TargetingSystem() {}

void TargetingSystem::Update(EntityManager& entityManager, double dt) {
    float deltaTime = static_cast<float>(dt);
    // Update lock progress
    for (auto it = lockProgress_.begin(); it != lockProgress_.end(); ) {
        it->second += deltaTime;
        if (it->second >= lockOnTime_) {
            // Lock complete, move to lockedTargets
            lockedTargets_[it->first] = GetTarget(it->first); // Assuming GetTarget returns the intended target
            it = lockProgress_.erase(it);
        } else {
            ++it;
        }
    }

    // Validate existing locks
    for (auto it = lockedTargets_.begin(); it != lockedTargets_.end(); ) {
        if (!IsValidTarget(entityManager, it->first, it->second)) {
            it = lockedTargets_.erase(it);
        } else {
            ++it;
        }
    }
}

bool TargetingSystem::LockOn(EntityManager& entityManager, int shooterEntity, int targetEntity) {
    if (!IsValidTarget(entityManager, shooterEntity, targetEntity)) {
        return false;
    }

    // Start lock process
    lockProgress_[shooterEntity] = 0.0f;
    // Store intended target temporarily, but since we have lockProgress, maybe use another map
    // For simplicity, assume immediate lock for now
    lockedTargets_[shooterEntity] = targetEntity;
    return true;
}

void TargetingSystem::ReleaseLock(int shooterEntity) {
    lockedTargets_.erase(shooterEntity);
    lockProgress_.erase(shooterEntity);
}

int TargetingSystem::GetTarget(int shooterEntity) const {
    auto it = lockedTargets_.find(shooterEntity);
    return (it != lockedTargets_.end()) ? it->second : -1;
}

bool TargetingSystem::AcquireTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const {
    if (shooterEntity == targetEntity) {
        return false;
    }

    if (!entityManager.IsAlive(shooterEntity) || !entityManager.IsAlive(targetEntity)) {
        return false;
    }

    Vec3 shooterPos;
    Vec3 targetPos;
    if (!ExtractPosition(entityManager, shooterEntity, shooterPos) ||
        !ExtractPosition(entityManager, targetEntity, targetPos)) {
        return false;
    }

    if (auto* shooterFaction = entityManager.GetComponent<Faction>(shooterEntity)) {
        if (auto* targetFaction = entityManager.GetComponent<Faction>(targetEntity)) {
            if (shooterFaction->id == targetFaction->id) {
                return false;
            }
        }
    }

    const double dx = static_cast<double>(targetPos.x) - static_cast<double>(shooterPos.x);
    const double dy = static_cast<double>(targetPos.y) - static_cast<double>(shooterPos.y);
    const double dz = static_cast<double>(targetPos.z) - static_cast<double>(shooterPos.z);
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double maxRange = static_cast<double>(targetingRangeKm_) * 1000.0; // Positions treated as meters
    if (distance > maxRange) {
        return false;
    }

    if (lineOfSightValidator_) {
        if (!lineOfSightValidator_(shooterPos, targetPos)) {
            return false;
        }
    }

    return true;
}

bool TargetingSystem::IsValidTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const {
    return AcquireTarget(entityManager, shooterEntity, targetEntity);
}

Vec3 TargetingSystem::CalculateLeadPosition(EntityManager& entityManager, int shooterEntity, int targetEntity, float projectileSpeed) const {
    Vec3 shooterPos{};
    Vec3 targetPos{};
    if (!ExtractPosition(entityManager, shooterEntity, shooterPos) ||
        !ExtractPosition(entityManager, targetEntity, targetPos)) {
        return Vec3{};
    }

    Vec3 targetVelocity{};
    if (auto* velocity = entityManager.GetComponent<Velocity>(targetEntity)) {
        targetVelocity.x = static_cast<float>(velocity->vx);
        targetVelocity.y = static_cast<float>(velocity->vy);
        targetVelocity.z = static_cast<float>(velocity->vz);
    }

    const double speed = static_cast<double>(projectileSpeed);
    if (speed <= 0.0) {
        return targetPos;
    }

    const double rx = static_cast<double>(targetPos.x) - static_cast<double>(shooterPos.x);
    const double ry = static_cast<double>(targetPos.y) - static_cast<double>(shooterPos.y);
    const double rz = static_cast<double>(targetPos.z) - static_cast<double>(shooterPos.z);

    const double vx = static_cast<double>(targetVelocity.x);
    const double vy = static_cast<double>(targetVelocity.y);
    const double vz = static_cast<double>(targetVelocity.z);

    const double a = (vx * vx + vy * vy + vz * vz) - (speed * speed);
    const double b = 2.0 * (rx * vx + ry * vy + rz * vz);
    const double c = rx * rx + ry * ry + rz * rz;

    constexpr double epsilon = 1e-6;
    double interceptTime = 0.0;

    if (std::abs(a) < epsilon) {
        if (std::abs(b) < epsilon) {
            // Target is stationary relative to shooter; aim directly.
            return targetPos;
        }
        interceptTime = -c / b;
    } else {
        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) {
            return targetPos;
        }

        const double sqrtDisc = std::sqrt(discriminant);
        const double denom = 2.0 * a;
        const double t1 = (-b + sqrtDisc) / denom;
        const double t2 = (-b - sqrtDisc) / denom;

        interceptTime = std::numeric_limits<double>::infinity();
        if (t1 > epsilon && t1 < interceptTime) {
            interceptTime = t1;
        }
        if (t2 > epsilon && t2 < interceptTime) {
            interceptTime = t2;
        }

        if (!std::isfinite(interceptTime)) {
            return targetPos;
        }
    }

    if (interceptTime < 0.0) {
        return targetPos;
    }

    Vec3 leadPos{};
    leadPos.x = targetPos.x + targetVelocity.x * static_cast<float>(interceptTime);
    leadPos.y = targetPos.y + targetVelocity.y * static_cast<float>(interceptTime);
    leadPos.z = targetPos.z + targetVelocity.z * static_cast<float>(interceptTime);
    return leadPos;
}

void TargetingSystem::SetLineOfSightValidator(std::function<bool(const Vec3&, const Vec3&)> validator) {
    lineOfSightValidator_ = std::move(validator);
}

bool TargetingSystem::ExtractPosition(EntityManager& entityManager, int entity, Vec3& outPosition) const {
    if (auto* position = entityManager.GetComponent<Position>(entity)) {
        outPosition.x = static_cast<float>(position->x);
        outPosition.y = static_cast<float>(position->y);
        outPosition.z = static_cast<float>(position->z);
        return true;
    }

    if (auto* transform2D = entityManager.GetComponent<Transform2D>(entity)) {
        outPosition.x = static_cast<float>(transform2D->x);
        outPosition.y = static_cast<float>(transform2D->y);
        outPosition.z = 0.0f;
        return true;
    }

    return false;
}
