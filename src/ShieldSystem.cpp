#include "ShieldSystem.h"
#include "ecs/EntityManager.h"
#include "FeedbackEvent.h"
#include <algorithm>
#include <iostream>

ShieldSystem::ShieldSystem() {}

void ShieldSystem::Update(EntityManager& entityManager, double dt) {
    (void)entityManager; // Unused for now
    float deltaTime = static_cast<float>(dt);
    for (auto& entry : shieldStates_) {
        int entityId = entry.first;
        ShieldState& shield = entry.second;
        
        if (!shield.isActive) {
            continue;
        }

        // Track previous capacity for recharge detection
        double prevCapacity = shield.currentCapacityMJ;

        // Update time since last hit
        shield.timeSinceLastHit += deltaTime;

        // Recharge if delay period has passed
        if (shield.timeSinceLastHit >= shield.rechargeDelaySeconds) {
            double rechargeAmount = shield.rechargeRateMJPerSec * deltaTime;
            shield.currentCapacityMJ = std::min(
                shield.currentCapacityMJ + rechargeAmount,
                shield.maxCapacityMJ
            );
            
            // Emit recharge event if shield is recharging
            if (prevCapacity > 0.0 && prevCapacity < shield.maxCapacityMJ && 
                shield.currentCapacityMJ > prevCapacity) {
                FeedbackEvent event(FeedbackEventType::ShieldRecharging, entityId);
                event.magnitude = shield.currentCapacityMJ / shield.maxCapacityMJ * 100.0;
                FeedbackEventManager::Get().Emit(event);
            }
            
            // Emit fully charged event
            if (prevCapacity < shield.maxCapacityMJ && 
                shield.currentCapacityMJ >= shield.maxCapacityMJ) {
                FeedbackEvent event(FeedbackEventType::ShieldFullyCharged, entityId);
                FeedbackEventManager::Get().Emit(event);
            }
        }
        
        // Check for low shield warning
        double shieldPercent = shield.currentCapacityMJ / shield.maxCapacityMJ;
        if (shieldPercent < 0.25 && shieldPercent > 0.0) {
            FeedbackEvent event(FeedbackEventType::WarningLowShields, entityId, AlertSeverity::Warning);
            event.magnitude = shieldPercent * 100.0;
            FeedbackEventManager::Get().Emit(event);
        }
    }
}

void ShieldSystem::InitializeShield(int entityId, double capacity, double rechargeRate,
                                   double rechargeDelay, double absorption, const std::string& componentId) {
    ShieldState state;
    state.maxCapacityMJ = capacity;
    state.currentCapacityMJ = capacity; // Start fully charged
    state.rechargeRateMJPerSec = rechargeRate;
    state.rechargeDelaySeconds = rechargeDelay;
    state.damageAbsorption = std::clamp(absorption, 0.0, 1.0);
    state.timeSinceLastHit = rechargeDelay; // Start ready to recharge
    state.isActive = true;
    state.shieldComponentId = componentId;
    
    shieldStates_[entityId] = state;
    std::cout << "Initialized shield for entity " << entityId 
              << " with capacity " << capacity << " MJ" << std::endl;
}

double ShieldSystem::ApplyDamage(int entityId, double damage) {
    auto it = shieldStates_.find(entityId);
    if (it == shieldStates_.end() || !it->second.isActive) {
        // No shield or inactive, full damage to hull
        FeedbackEvent event(FeedbackEventType::HullDamage, entityId, AlertSeverity::Warning);
        event.magnitude = damage;
        // TODO: Get entity position for 3D audio
        FeedbackEventManager::Get().Emit(event);
        return damage;
    }

    ShieldState& shield = it->second;
    
    // Calculate absorbed damage
    double absorbedDamage = damage * shield.damageAbsorption;
    double hullDamage = damage - absorbedDamage;

    // Emit shield hit event
    FeedbackEvent hitEvent(FeedbackEventType::ShieldHit, entityId);
    hitEvent.magnitude = absorbedDamage;
    // TODO: Get entity position for 3D audio
    FeedbackEventManager::Get().Emit(hitEvent);

    // Apply damage to shield
    if (shield.currentCapacityMJ >= absorbedDamage) {
        shield.currentCapacityMJ -= absorbedDamage;
    } else {
        // Shield depleted, overflow to hull
        double overflow = absorbedDamage - shield.currentCapacityMJ;
        shield.currentCapacityMJ = 0.0;
        hullDamage += overflow;
        
        // Emit shield depleted event
        FeedbackEvent depletedEvent(FeedbackEventType::ShieldDepleted, entityId, AlertSeverity::Critical);
        FeedbackEventManager::Get().Emit(depletedEvent);
    }

    // Reset recharge timer
    shield.timeSinceLastHit = 0.0;

    return hullDamage;
}

double ShieldSystem::GetShieldPercentage(int entityId) const {
    auto it = shieldStates_.find(entityId);
    if (it == shieldStates_.end() || it->second.maxCapacityMJ <= 0.0) {
        return 0.0;
    }
    return it->second.currentCapacityMJ / it->second.maxCapacityMJ;
}

const ShieldState* ShieldSystem::GetShieldState(int entityId) const {
    auto it = shieldStates_.find(entityId);
    return (it != shieldStates_.end()) ? &it->second : nullptr;
}

void ShieldSystem::SetShieldActive(int entityId, bool active) {
    auto it = shieldStates_.find(entityId);
    if (it != shieldStates_.end()) {
        it->second.isActive = active;
    }
}

void ShieldSystem::Recharge(int entityId, double amount) {
    auto it = shieldStates_.find(entityId);
    if (it != shieldStates_.end() && it->second.isActive) {
        it->second.currentCapacityMJ = std::min(
            it->second.currentCapacityMJ + amount,
            it->second.maxCapacityMJ
        );
    }
}