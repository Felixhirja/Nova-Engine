#pragma once

#include "ecs/System.h"
#include <unordered_map>
#include <vector>
#include <memory>

// Weapon system for firing and managing weapons
class WeaponSystem : public System {
public:
    WeaponSystem();

    void Update(EntityManager& entityManager, double dt) override;

    // Fire weapon on entity
    bool FireWeapon(int entityId, const std::string& weaponSlot);

    // Check if weapon can fire (ammo, cooldown)
    bool CanFire(int entityId, const std::string& weaponSlot) const;

private:
    // Cooldown timers (entityId -> cooldown)
    std::unordered_map<int, std::unordered_map<std::string, float>> weaponCooldowns_;

    // Ammo counts (entityId -> slot -> ammo)
    std::unordered_map<int, std::unordered_map<std::string, int>> weaponAmmo_;
};