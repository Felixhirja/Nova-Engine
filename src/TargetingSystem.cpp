#include "TargetingSystem.h"
#include "ecs/EntityManager.h"
#include "Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

bool TargetingSystem::IsValidTarget(EntityManager& entityManager, int shooterEntity, int targetEntity) const {
    (void)entityManager; // TODO: Implement proper entity validation
    (void)shooterEntity;
    (void)targetEntity;
    
    // TODO: Check if entities exist and have transforms
    // TODO: Check range validation
    // TODO: Check line of sight, faction, etc.
    
    // For now, return true - this will be implemented when Transform component is integrated
    return true;
}

glm::vec3 TargetingSystem::CalculateLeadPosition(EntityManager& entityManager, int shooterEntity, int targetEntity, float projectileSpeed) const {
    (void)entityManager;
    (void)shooterEntity;
    (void)targetEntity;
    (void)projectileSpeed;
    
    // TODO: Implement lead calculation when Transform/physics components are integrated
    return glm::vec3(0.0f);
}