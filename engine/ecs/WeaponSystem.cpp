#include "WeaponSystem.h"
#include "FeedbackEvent.h"
#include "ecs/Components.h"
#include <algorithm>
#include <cmath>
#include <vector>

WeaponSystem::WeaponSystem() {}

void WeaponSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    float deltaTime = static_cast<float>(dt);

    // Update cooldowns
    for (auto entityIt = weaponCooldowns_.begin(); entityIt != weaponCooldowns_.end(); ) {
        for (auto slotIt = entityIt->second.begin(); slotIt != entityIt->second.end(); ) {
            slotIt->second -= deltaTime;
            if (slotIt->second <= 0.0f) {
                slotIt = entityIt->second.erase(slotIt);
            } else {
                ++slotIt;
            }
        }

        if (entityIt->second.empty()) {
            entityIt = weaponCooldowns_.erase(entityIt);
        } else {
            ++entityIt;
        }
    }

    std::vector<Entity> expiredProjectiles;
    entityManager.ForEach<Lifetime, Projectile>([&](Entity entity, Lifetime& lifetime, Projectile&) {
        lifetime.remaining -= dt;
        if (lifetime.remaining <= 0.0) {
            expiredProjectiles.push_back(entity);
        }
    });

    for (Entity entity : expiredProjectiles) {
        entityManager.DestroyEntity(entity);
    }
}

bool WeaponSystem::FireWeapon(EntityManager& entityManager, int entityId, const std::string& weaponSlot) {
    if (!entityManager.IsAlive(entityId)) {
        return false;
    }

    const WeaponSlotConfig* config = GetConfig(entityId, weaponSlot);
    if (!config) {
        return false;
    }

    auto cooldownEntityIt = weaponCooldowns_.find(entityId);
    bool onCooldown = false;
    if (cooldownEntityIt != weaponCooldowns_.end()) {
        auto cooldownIt = cooldownEntityIt->second.find(weaponSlot);
        if (cooldownIt != cooldownEntityIt->second.end() && cooldownIt->second > 0.0f) {
            onCooldown = true;
        }
    }

    auto ammoEntityIt = weaponAmmo_.find(entityId);
    bool limitedAmmo = false;
    bool outOfAmmo = false;
    if (ammoEntityIt != weaponAmmo_.end()) {
        auto ammoIt = ammoEntityIt->second.find(weaponSlot);
        if (ammoIt != ammoEntityIt->second.end()) {
            if (ammoIt->second >= 0) {
                limitedAmmo = true;
                if (ammoIt->second <= 0) {
                    outOfAmmo = true;
                }
            }
        }
    }

    double originX = 0.0;
    double originY = 0.0;
    double originZ = 0.0;
    if (!ExtractEntityPosition(entityManager, entityId, originX, originY, originZ)) {
        return false;
    }

    if (onCooldown) {
        FeedbackEvent event(FeedbackEventType::WeaponOverheat, entityId, AlertSeverity::Warning);
        event.componentId = weaponSlot;
        event.message = weaponSlot + " cooling down";
        event.x = originX;
        event.y = originY;
        event.z = originZ;
        FeedbackEventManager::Get().Emit(event);
        return false;
    }

    if (outOfAmmo) {
        FeedbackEvent event(FeedbackEventType::AmmoEmpty, entityId, AlertSeverity::Critical);
        event.componentId = weaponSlot;
        event.message = weaponSlot + " ammunition depleted";
        event.x = originX;
        event.y = originY;
        event.z = originZ;
        FeedbackEventManager::Get().Emit(event);
        return false;
    }

    double muzzleX = originX + config->muzzleOffsetX;
    double muzzleY = originY + config->muzzleOffsetY;
    double muzzleZ = originZ + config->muzzleOffsetZ;

    Entity projectileEntity = entityManager.CreateEntity();
    auto& projectilePosition = entityManager.EmplaceComponent<Position>(projectileEntity);
    projectilePosition.x = muzzleX;
    projectilePosition.y = muzzleY;
    projectilePosition.z = muzzleZ;

    auto& projectileVelocity = entityManager.EmplaceComponent<Velocity>(projectileEntity);
    projectileVelocity.vx = config->muzzleDirX * config->projectileSpeed;
    projectileVelocity.vy = config->muzzleDirY * config->projectileSpeed;
    projectileVelocity.vz = config->muzzleDirZ * config->projectileSpeed;

    auto& body = entityManager.EmplaceComponent<RigidBody>(projectileEntity);
    body.SetMass(1.0);
    body.useGravity = false;
    body.linearDamping = 0.0;
    body.angularDamping = 0.0;

    auto& projectile = entityManager.EmplaceComponent<Projectile>(projectileEntity);
    projectile.ownerEntity = entityId;
    projectile.weaponSlot = weaponSlot;

    auto& damage = entityManager.EmplaceComponent<DamagePayload>(projectileEntity);
    damage.amount = config->damage;
    damage.sourceEntity = entityId;

    auto& lifetime = entityManager.EmplaceComponent<Lifetime>(projectileEntity);
    lifetime.remaining = config->projectileLifetime;

    FeedbackEvent fireEvent(FeedbackEventType::WeaponFired, entityId, AlertSeverity::Info);
    fireEvent.componentId = weaponSlot;
    fireEvent.message = weaponSlot + " fired";
    fireEvent.magnitude = config->damage;
    fireEvent.x = muzzleX;
    fireEvent.y = muzzleY;
    fireEvent.z = muzzleZ;
    FeedbackEventManager::Get().Emit(fireEvent);

    if (config->fireRatePerSecond > 0.0f) {
        weaponCooldowns_[entityId][weaponSlot] = 1.0f / config->fireRatePerSecond;
    }

    if (limitedAmmo) {
        ammoEntityIt->second[weaponSlot] -= 1;
    }

    return true;
}

bool WeaponSystem::CanFire(int entityId, const std::string& weaponSlot) const {
    const WeaponSlotConfig* config = GetConfig(entityId, weaponSlot);
    if (!config) {
        return false;
    }

    // Check cooldown
    auto entityIt = weaponCooldowns_.find(entityId);
    if (entityIt != weaponCooldowns_.end()) {
        auto cooldownIt = entityIt->second.find(weaponSlot);
        if (cooldownIt != entityIt->second.end() && cooldownIt->second > 0.0f) {
            return false;
        }
    }

    // Check ammo
    auto ammoEntityIt = weaponAmmo_.find(entityId);
    if (ammoEntityIt != weaponAmmo_.end()) {
        auto ammoIt = ammoEntityIt->second.find(weaponSlot);
        if (ammoIt != ammoEntityIt->second.end() && ammoIt->second >= 0 && ammoIt->second <= 0) {
            return false;
        }
    }

    return true;
}

void WeaponSystem::ConfigureWeaponSlot(int entityId, const std::string& weaponSlot, const WeaponSlotConfig& config) {
    weaponConfigs_[entityId][weaponSlot] = config;
    if (config.ammo >= 0) {
        weaponAmmo_[entityId][weaponSlot] = config.ammo;
    } else {
        auto ammoEntityIt = weaponAmmo_.find(entityId);
        if (ammoEntityIt != weaponAmmo_.end()) {
            ammoEntityIt->second.erase(weaponSlot);
            if (ammoEntityIt->second.empty()) {
                weaponAmmo_.erase(ammoEntityIt);
            }
        }
    }
}

int WeaponSystem::GetAmmoCount(int entityId, const std::string& weaponSlot) const {
    auto ammoEntityIt = weaponAmmo_.find(entityId);
    if (ammoEntityIt == weaponAmmo_.end()) {
        return -1;
    }
    auto ammoIt = ammoEntityIt->second.find(weaponSlot);
    if (ammoIt == ammoEntityIt->second.end()) {
        return -1;
    }
    return ammoIt->second;
}

const WeaponSystem::WeaponSlotConfig* WeaponSystem::GetConfig(int entityId, const std::string& weaponSlot) const {
    auto entityIt = weaponConfigs_.find(entityId);
    if (entityIt == weaponConfigs_.end()) {
        return nullptr;
    }
    auto slotIt = entityIt->second.find(weaponSlot);
    if (slotIt == entityIt->second.end()) {
        return nullptr;
    }
    return &slotIt->second;
}

bool WeaponSystem::ExtractEntityPosition(EntityManager& entityManager, int entityId, double& x, double& y, double& z) const {
    if (auto* position = entityManager.GetComponent<Position>(entityId)) {
        x = position->x;
        y = position->y;
        z = position->z;
        return true;
    }

    if (auto* transform2D = entityManager.GetComponent<Transform2D>(entityId)) {
        x = transform2D->x;
        y = transform2D->y;
        z = 0.0;
        return true;
    }

    return false;
}
