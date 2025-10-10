#include "WeaponSystem.h"
#include "ecs/EntityManager.h"
#include "FeedbackEvent.h"
#include <iostream>

WeaponSystem::WeaponSystem() {}

void WeaponSystem::Update(EntityManager& entityManager, double dt) {
    (void)entityManager; // Unused for now
    float deltaTime = static_cast<float>(dt);
    
    // Update cooldowns
    for (auto& entityEntry : weaponCooldowns_) {
        for (auto it = entityEntry.second.begin(); it != entityEntry.second.end(); ) {
            it->second -= deltaTime;
            if (it->second <= 0.0f) {
                it = entityEntry.second.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool WeaponSystem::FireWeapon(int entityId, const std::string& weaponSlot) {
    if (!CanFire(entityId, weaponSlot)) {
        // Check if it's because of ammo
        auto ammoEntityIt = weaponAmmo_.find(entityId);
        if (ammoEntityIt != weaponAmmo_.end()) {
            auto ammoIt = ammoEntityIt->second.find(weaponSlot);
            if (ammoIt != ammoEntityIt->second.end() && ammoIt->second <= 0) {
                FeedbackEvent event(FeedbackEventType::AmmoEmpty, entityId);
                event.componentId = weaponSlot;
                FeedbackEventManager::Get().Emit(event);
            }
        }
        return false;
    }

    // TODO: Create projectile entity
    std::cout << "Firing weapon on entity " << entityId << " slot " << weaponSlot << std::endl;

    // Emit weapon fired event
    FeedbackEvent event(FeedbackEventType::WeaponFired, entityId);
    event.componentId = weaponSlot;
    // TODO: Get weapon position for 3D audio
    FeedbackEventManager::Get().Emit(event);

    // Set cooldown
    weaponCooldowns_[entityId][weaponSlot] = 1.0f / 10.0f; // Assume 10 fps fire rate

    // Decrement ammo
    if (weaponAmmo_[entityId].find(weaponSlot) != weaponAmmo_[entityId].end()) {
        weaponAmmo_[entityId][weaponSlot]--;
    }

    return true;
}

bool WeaponSystem::CanFire(int entityId, const std::string& weaponSlot) const {
    // Check cooldown
    auto entityIt = weaponCooldowns_.find(entityId);
    if (entityIt != weaponCooldowns_.end()) {
        if (entityIt->second.find(weaponSlot) != entityIt->second.end()) {
            return false;
        }
    }

    // Check ammo
    auto ammoEntityIt = weaponAmmo_.find(entityId);
    if (ammoEntityIt != weaponAmmo_.end()) {
        auto ammoIt = ammoEntityIt->second.find(weaponSlot);
        if (ammoIt != ammoEntityIt->second.end() && ammoIt->second <= 0) {
            return false;
        }
    }

    return true;
}