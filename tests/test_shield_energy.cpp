#include "ecs/System.h"
#include "ecs/EntityManager.h"
#include <iostream>
#include <cmath>

bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

int main() {
    // Create an EntityManager for systems that need it
    EntityManager entityManager;
    
    // Create UnifiedSystem instances for shield and energy management
    UnifiedSystem shieldSystem(SystemType::Shield);
    UnifiedSystem energySystem(SystemType::Shield); // Using Shield type for energy management too
    
    // Create entity first
    Entity entity = entityManager.CreateEntity();
    int entityId = entity;
    
    // Initialize shield component on entity
    entityManager.EmplaceComponent<ShieldComponent>(entityId, 
        150.0,  // currentShields
        150.0,  // maxShields
        5.0,    // rechargeRate
        3.0,    // rechargeDelay
        0.0,    // lastDamageTime
        true    // isActive
    );
    
    // Initialize shield system
    shieldSystem.InitializeShield(entityId, 150.0, 5.0, 3.0, 0.8, "shield_array_light");
    
    const ShieldComponent* state = entityManager.GetComponent<ShieldComponent>(entityId);
    if (!state) {
        std::cerr << "Shield state not found after initialization" << std::endl;
        return 1;
    }
    
    if (!approxEqual(state->currentShields, 150.0) || !approxEqual(state->maxShields, 150.0)) {
        std::cerr << "Shield initialized with incorrect capacity" << std::endl;
        return 2;
    }
    
    if (!approxEqual(shieldSystem.GetShieldPercentage(entityId), 1.0)) {
        std::cerr << "Shield should be at 100%" << std::endl;
        return 3;
    }
    
    // Test damage absorption
    double hullDamage = shieldSystem.ApplyDamage(entityId, 50.0, &entityManager);
    // 50 damage * 0.8 absorption = 40 to shield, 10 to hull
    if (!approxEqual(hullDamage, 10.0)) {
        std::cerr << "Hull damage incorrect: expected 10.0, got " << hullDamage << std::endl;
        return 4;
    }
    
    state = entityManager.GetComponent<ShieldComponent>(entityId);
    if (!approxEqual(state->currentShields, 110.0)) {
        std::cerr << "Shield capacity incorrect after damage: " << state->currentShields << std::endl;
        return 5;
    }
    
    // Test shield depletion and overflow
    hullDamage = shieldSystem.ApplyDamage(entityId, 200.0, &entityManager);
    // 200 * 0.8 = 160 absorbed, but only 110 available
    // 50 overflow + 40 unabsorbed = 90 to hull
    if (!approxEqual(hullDamage, 90.0)) {
        std::cerr << "Hull damage on shield depletion incorrect: expected 90.0, got " << hullDamage << std::endl;
        return 6;
    }
    
    state = entityManager.GetComponent<ShieldComponent>(entityId);
    if (!approxEqual(state->currentShields, 0.0)) {
        std::cerr << "Shield should be depleted" << std::endl;
        return 7;
    }
    
    // Test recharge delay
    shieldSystem.Update(entityManager, 2.0); // 2 seconds, still within delay
    state = entityManager.GetComponent<ShieldComponent>(entityId);
    if (state->currentShields > 0.1) {
        std::cerr << "Shield should not recharge during delay period" << std::endl;
        return 8;
    }
    
    shieldSystem.Update(entityManager, 2.0); // 2 more seconds, total 4, past 3-second delay
    state = entityManager.GetComponent<ShieldComponent>(entityId);
    // Should recharge: 2 second * 5 MJ/s = 10 MJ (recharges for full deltaTime once delay passed)
    if (!approxEqual(state->currentShields, 10.0, 0.1)) {
        std::cerr << "Shield recharge incorrect: expected ~10.0, got " << state->currentShields << std::endl;
        return 9;
    }
    
    // Test energy management
    int shipId = 10;
    
    energySystem.InitializeEnergy(shipId, 30.0, 8.0, 10.0, 1.0);
    
    const EnergyComponent* energyState = energySystem.GetEnergyState(shipId);
    if (!energyState) {
        std::cerr << "Energy state not found after initialization" << std::endl;
        return 10;
    }
    
    // Check default balanced allocation
    if (!approxEqual(energyState->shieldAllocation, 0.33, 0.01) ||
        !approxEqual(energyState->weaponAllocation, 0.33, 0.01) ||
        !approxEqual(energyState->thrusterAllocation, 0.34, 0.01)) {
        std::cerr << "Default allocation incorrect" << std::endl;
        return 11;
    }
    
    // Test power distribution
    energySystem.UpdateEnergy(shipId, 1.0f);
    energyState = energySystem.GetEnergyState(shipId);
    
    if (!approxEqual(energyState->shieldPowerMW, 9.9, 0.5) ||
        !approxEqual(energyState->weaponPowerMW, 9.9, 0.5) ||
        !approxEqual(energyState->thrusterPowerMW, 10.2, 0.5)) {
        std::cerr << "Power distribution incorrect" << std::endl;
        return 12;
    }
    
    // Test power diversion
    energySystem.DivertPower(shipId, PowerPriority::Shields, 0.1);
    energyState = energySystem.GetEnergyState(shipId);

    if (energyState->shieldAllocation <= 0.33) {
        std::cerr << "Shield allocation should increase after diversion" << std::endl;
        return 13;
    }    // Test HasPower
    if (!energySystem.HasPower(shipId, PowerPriority::Thrusters)) {
        std::cerr << "Thrusters should have sufficient power" << std::endl;
        return 14;
    }
    
    // Test custom allocation
    energySystem.SetEnergyAllocation(shipId, 0.5, 0.3, 0.2);
    energyState = energySystem.GetEnergyState(shipId);
    
    if (!approxEqual(energyState->shieldAllocation, 0.5, 0.01) ||
        !approxEqual(energyState->weaponAllocation, 0.3, 0.01) ||
        !approxEqual(energyState->thrusterAllocation, 0.2, 0.01)) {
        std::cerr << "Custom allocation not applied correctly" << std::endl;
        return 15;
    }
    
    std::cout << "Shield and energy management tests passed." << std::endl;
    return 0;
}