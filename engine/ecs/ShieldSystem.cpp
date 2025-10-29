#include "ShieldSystem.h"
#include <algorithm>

ShieldManagementSystem::ShieldManagementSystem() = default;

void ShieldManagementSystem::Update(EntityManager& entityManager, double dt) {
    lastKnownEntityManager_ = &entityManager;

    for (auto& [entityId, shield] : shieldStates_) {
        if (!shield.isActive) continue;

        // Update time since last hit
        shield.timeSinceLastHit += dt;

        // Recharge if enough time has passed
        if (shield.timeSinceLastHit >= shield.rechargeDelaySeconds &&
            shield.currentCapacityMJ < shield.maxCapacityMJ) {
            double rechargeAmount = shield.rechargeRateMJPerSec * dt;
            shield.currentCapacityMJ = std::min(shield.currentCapacityMJ + rechargeAmount,
                                               shield.maxCapacityMJ);
        }
    }
}

void ShieldManagementSystem::InitializeShield(int entityId, double capacity, double rechargeRate,
                                             double rechargeDelay, double absorption, const std::string& componentId) {
    ShieldState state;
    state.maxCapacityMJ = capacity;
    state.currentCapacityMJ = capacity;  // Start fully charged
    state.rechargeRateMJPerSec = rechargeRate;
    state.rechargeDelaySeconds = rechargeDelay;
    state.damageAbsorption = absorption;
    state.shieldComponentId = componentId;
    state.timeSinceLastHit = rechargeDelay;  // Allow immediate recharge
    state.isActive = true;

    shieldStates_[entityId] = state;
}

double ShieldManagementSystem::ApplyDamage(int entityId, double damage, EntityManager* entityManager) {
    auto it = shieldStates_.find(entityId);
    if (it == shieldStates_.end() || !it->second.isActive) {
        return damage;  // No shield or inactive, all damage goes through
    }

    ShieldState& shield = it->second;
    double absorbedDamage = damage * shield.damageAbsorption;
    double overflowDamage = damage - absorbedDamage;

    // Apply damage to shield
    shield.currentCapacityMJ = std::max(0.0, shield.currentCapacityMJ - absorbedDamage);
    shield.timeSinceLastHit = 0.0;  // Reset recharge timer

    // If shield is depleted, remaining damage goes to hull
    if (shield.currentCapacityMJ <= 0.0) {
        overflowDamage += (absorbedDamage - shield.currentCapacityMJ) / shield.damageAbsorption;
        shield.currentCapacityMJ = 0.0;
    }

    return overflowDamage;
}

double ShieldManagementSystem::GetShieldPercentage(int entityId) const {
    auto it = shieldStates_.find(entityId);
    if (it == shieldStates_.end()) return 0.0;

    const ShieldState& shield = it->second;
    if (shield.maxCapacityMJ <= 0.0) return 0.0;

    return shield.currentCapacityMJ / shield.maxCapacityMJ;
}

const ShieldState* ShieldManagementSystem::GetShieldState(int entityId) const {
    auto it = shieldStates_.find(entityId);
    return (it != shieldStates_.end()) ? &it->second : nullptr;
}

void ShieldManagementSystem::SetShieldActive(int entityId, bool active) {
    auto it = shieldStates_.find(entityId);
    if (it != shieldStates_.end()) {
        it->second.isActive = active;
    }
}

void ShieldManagementSystem::Recharge(int entityId, double amount) {
    auto it = shieldStates_.find(entityId);
    if (it != shieldStates_.end()) {
        ShieldState& shield = it->second;
        shield.currentCapacityMJ = std::min(shield.currentCapacityMJ + amount, shield.maxCapacityMJ);
    }
}