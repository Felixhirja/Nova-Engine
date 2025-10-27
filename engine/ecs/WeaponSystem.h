#pragma once

#include "ecs/System.h"
#include "ecs/EntityManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

// Weapon system for firing and managing weapons
class WeaponSystem : public System {
public:
    WeaponSystem();

    void Update(EntityManager& entityManager, double dt) override;

    struct WeaponSlotConfig {
        float fireRatePerSecond = 10.0f;
        int ammo = -1; // -1 = unlimited
        double damage = 0.0;
        double projectileSpeed = 0.0;
        double projectileLifetime = 0.0;
        double muzzleOffsetX = 0.0;
        double muzzleOffsetY = 0.0;
        double muzzleOffsetZ = 0.0;
        float muzzleDirX = 1.0f;
        float muzzleDirY = 0.0f;
        float muzzleDirZ = 0.0f;
    };

    // Fire weapon on entity
    bool FireWeapon(EntityManager& entityManager, int entityId, const std::string& weaponSlot);

    // Check if weapon can fire (ammo, cooldown)
    bool CanFire(int entityId, const std::string& weaponSlot) const;

    void ConfigureWeaponSlot(int entityId, const std::string& weaponSlot, const WeaponSlotConfig& config);

    int GetAmmoCount(int entityId, const std::string& weaponSlot) const;

private:
    // Cooldown timers (entityId -> cooldown)
    std::unordered_map<int, std::unordered_map<std::string, float>> weaponCooldowns_;

    // Ammo counts (entityId -> slot -> ammo)
    std::unordered_map<int, std::unordered_map<std::string, int>> weaponAmmo_;

    std::unordered_map<int, std::unordered_map<std::string, WeaponSlotConfig>> weaponConfigs_;

    const WeaponSlotConfig* GetConfig(int entityId, const std::string& weaponSlot) const;

    bool ExtractEntityPosition(EntityManager& entityManager, int entityId, double& x, double& y, double& z) const;
};
