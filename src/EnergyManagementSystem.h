#pragma once

#include <string>
#include <unordered_map>

// Power distribution priorities
enum class PowerPriority {
    Shields,
    Weapons,
    Thrusters,
    Sensors
};

// Energy management state for a ship
struct EnergyManagementState {
    double totalPowerMW = 0.0;
    double availablePowerMW = 0.0;
    
    // Power allocation per subsystem (as percentage 0.0-1.0)
    double shieldAllocation = 0.33;
    double weaponAllocation = 0.33;
    double thrusterAllocation = 0.34;
    
    // Actual power delivered
    double shieldPowerMW = 0.0;
    double weaponPowerMW = 0.0;
    double thrusterPowerMW = 0.0;
    
    // Power requirements
    double shieldRequirementMW = 0.0;
    double weaponRequirementMW = 0.0;
    double thrusterRequirementMW = 0.0;
    
    // Overload protection
    bool overloadProtection = true;
    double overloadThreshold = 1.1; // 110% capacity before emergency shutdown
};

// Energy management system for power distribution
class EnergyManagementSystem {
public:
    EnergyManagementSystem();
    
    // Initialize energy management for entity
    void Initialize(int entityId, double totalPower, double shieldReq, double weaponReq, double thrusterReq);
    
    // Update power distribution
    void Update(int entityId, float deltaTime);
    
    // Set allocation percentages (must sum to ~1.0)
    void SetAllocation(int entityId, double shields, double weapons, double thrusters);
    
    // Get current state
    const EnergyManagementState* GetState(int entityId) const;
    
    // Emergency power diversion
    void DivertPower(int entityId, PowerPriority priority, double amount);
    
    // Check if subsystem has enough power
    bool HasPower(int entityId, PowerPriority subsystem) const;
    
private:
    std::unordered_map<int, EnergyManagementState> states_;
    
    void BalancePower(EnergyManagementState& state);
};