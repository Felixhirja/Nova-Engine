#pragma once

#include "ecs/System.h"
#include <unordered_map>
#include <string>

// Shield state for an entity
struct ShieldState {
    double currentCapacityMJ = 0.0;      // Current shield energy
    double maxCapacityMJ = 0.0;          // Maximum shield energy
    double rechargeRateMJPerSec = 0.0;   // Recharge rate per second
    double rechargeDelaySeconds = 0.0;   // Delay before recharge starts
    double damageAbsorption = 1.0;       // Fraction of damage absorbed
    double timeSinceLastHit = 0.0;       // Time since last damage
    bool isActive = true;                // Shield online status
    std::string shieldComponentId;       // Reference to blueprint
};

// Shield system for damage absorption and recharge
class ShieldManagementSystem : public System {
public:
    ShieldManagementSystem();

    void Update(EntityManager& entityManager, double dt) override;

    // Initialize shield for entity
    void InitializeShield(int entityId, double capacity, double rechargeRate, 
                         double rechargeDelay, double absorption, const std::string& componentId);

    // Apply damage to shield, returns actual damage dealt to hull (overflow)
    double ApplyDamage(int entityId, double damage, EntityManager* entityManager = nullptr);

    // Get shield percentage (0.0-1.0)
    double GetShieldPercentage(int entityId) const;

    // Get shield state
    const ShieldState* GetShieldState(int entityId) const;

    // Toggle shield on/off
    void SetShieldActive(int entityId, bool active);

    // Recharge shield manually (e.g., power diversion)
    void Recharge(int entityId, double amount);

private:
    std::unordered_map<int, ShieldState> shieldStates_;
    EntityManager* lastKnownEntityManager_ = nullptr;
};