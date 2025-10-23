#include "EnergyManagementSystem.h"
#include "FeedbackEvent.h"
#include <algorithm>
#include <iostream>
#include <cmath>

EnergyManagementSystem::EnergyManagementSystem() {}

void EnergyManagementSystem::Initialize(int entityId, double totalPower, 
                                       double shieldReq, double weaponReq, double thrusterReq) {
    EnergyManagementState state;
    state.totalPowerMW = totalPower;
    state.availablePowerMW = totalPower;
    state.shieldRequirementMW = shieldReq;
    state.weaponRequirementMW = weaponReq;
    state.thrusterRequirementMW = thrusterReq;
    
    // Default balanced allocation
    state.shieldAllocation = 0.33;
    state.weaponAllocation = 0.33;
    state.thrusterAllocation = 0.34;
    
    states_[entityId] = state;
    BalancePower(states_[entityId]);
    
    std::cout << "Initialized energy management for entity " << entityId 
              << " with " << totalPower << " MW" << std::endl;
}

void EnergyManagementSystem::Update(int entityId, float deltaTime) {
    auto it = states_.find(entityId);
    if (it == states_.end()) {
        return;
    }
    
    BalancePower(it->second);
}

void EnergyManagementSystem::SetAllocation(int entityId, double shields, double weapons, double thrusters) {
    auto it = states_.find(entityId);
    if (it == states_.end()) {
        return;
    }
    
    EnergyManagementState& state = it->second;
    
    // Normalize allocations
    double total = shields + weapons + thrusters;
    if (total > 0.0) {
        state.shieldAllocation = shields / total;
        state.weaponAllocation = weapons / total;
        state.thrusterAllocation = thrusters / total;
    }
    
    BalancePower(state);
}

const EnergyManagementState* EnergyManagementSystem::GetState(int entityId) const {
    auto it = states_.find(entityId);
    return (it != states_.end()) ? &it->second : nullptr;
}

void EnergyManagementSystem::UpdateDemand(int entityId,
                                          double totalPower,
                                          double availablePower,
                                          double shieldReq,
                                          double weaponReq,
                                          double thrusterReq) {
    auto it = states_.find(entityId);
    if (it == states_.end()) {
        return;
    }

    EnergyManagementState& state = it->second;
    state.totalPowerMW = std::max(0.0, totalPower);
    state.availablePowerMW = std::clamp(availablePower, 0.0, state.totalPowerMW > 0.0 ? state.totalPowerMW : availablePower);
    state.shieldRequirementMW = std::max(0.0, shieldReq);
    state.weaponRequirementMW = std::max(0.0, weaponReq);
    state.thrusterRequirementMW = std::max(0.0, thrusterReq);

    BalancePower(state);
}

void EnergyManagementSystem::DivertPower(int entityId, PowerPriority priority, double amount) {
    auto it = states_.find(entityId);
    if (it == states_.end()) {
        return;
    }
    
    EnergyManagementState& state = it->second;
    
    // Emit power diversion event
    FeedbackEvent event(FeedbackEventType::EnergyDiverted, entityId);
    event.magnitude = amount;
    FeedbackEventManager::Get().Emit(event);
    
    // Temporarily boost allocation for priority subsystem
    switch (priority) {
        case PowerPriority::Shields:
            state.shieldAllocation = std::min(1.0, state.shieldAllocation + 0.1);
            state.weaponAllocation *= 0.9;
            state.thrusterAllocation *= 0.9;
            break;
        case PowerPriority::Weapons:
            state.weaponAllocation = std::min(1.0, state.weaponAllocation + 0.1);
            state.shieldAllocation *= 0.9;
            state.thrusterAllocation *= 0.9;
            break;
        case PowerPriority::Thrusters:
            state.thrusterAllocation = std::min(1.0, state.thrusterAllocation + 0.1);
            state.shieldAllocation *= 0.9;
            state.weaponAllocation *= 0.9;
            break;
        default:
            break;
    }
    
    // Normalize
    double total = state.shieldAllocation + state.weaponAllocation + state.thrusterAllocation;
    if (total > 0.0) {
        state.shieldAllocation /= total;
        state.weaponAllocation /= total;
        state.thrusterAllocation /= total;
    }
    
    BalancePower(state);
}

bool EnergyManagementSystem::HasPower(int entityId, PowerPriority subsystem) const {
    auto it = states_.find(entityId);
    if (it == states_.end()) {
        return false;
    }
    
    const EnergyManagementState& state = it->second;
    
    switch (subsystem) {
        case PowerPriority::Shields:
            return state.shieldPowerMW >= state.shieldRequirementMW * 0.5; // At least 50%
        case PowerPriority::Weapons:
            return state.weaponPowerMW >= state.weaponRequirementMW * 0.5;
        case PowerPriority::Thrusters:
            return state.thrusterPowerMW >= state.thrusterRequirementMW * 0.5;
        default:
            return false;
    }
}

void EnergyManagementSystem::BalancePower(EnergyManagementState& state) {
    // Distribute available power according to allocations
    state.shieldPowerMW = state.availablePowerMW * state.shieldAllocation;
    state.weaponPowerMW = state.availablePowerMW * state.weaponAllocation;
    state.thrusterPowerMW = state.availablePowerMW * state.thrusterAllocation;
    
    // Check overload
    double totalDemand = state.shieldRequirementMW + state.weaponRequirementMW + state.thrusterRequirementMW;
    if (state.overloadProtection && totalDemand > state.totalPowerMW * state.overloadThreshold) {
        std::cerr << "Warning: Power overload detected, reducing output" << std::endl;
        // Scale down all allocations proportionally
        double scaleFactor = (state.totalPowerMW * state.overloadThreshold) / totalDemand;
        state.shieldPowerMW *= scaleFactor;
        state.weaponPowerMW *= scaleFactor;
        state.thrusterPowerMW *= scaleFactor;
    }
}