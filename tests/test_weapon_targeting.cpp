#include "../engine/WeaponSystem.h"
#include "../engine/TargetingSystem.h"
#include "../engine/FeedbackEvent.h"
#include "../engine/ecs/Components.h"
#include "../engine/ecs/EntityManager.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

struct TestFeedbackListener : public IFeedbackListener {
    std::vector<FeedbackEvent> events;

    void OnFeedbackEvent(const FeedbackEvent& event) override {
        events.push_back(event);
    }
};

bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

bool TestInvalidTargets() {
    EntityManager entityManager;
    TargetingSystem targetingSystem;

    int shooter = entityManager.CreateEntity();
    auto& shooterPos = entityManager.EmplaceComponent<Position>(shooter);
    shooterPos.x = 0.0;
    shooterPos.y = 0.0;
    shooterPos.z = 0.0;
    auto& shooterFaction = entityManager.EmplaceComponent<Faction>(shooter);
    shooterFaction.id = 1;

    if (targetingSystem.AcquireTarget(entityManager, shooter, 9999)) {
        std::cerr << "AcquireTarget should fail for non-existent target" << std::endl;
        return false;
    }

    int targetNoPosition = entityManager.CreateEntity();
    auto& noPosFaction = entityManager.EmplaceComponent<Faction>(targetNoPosition);
    noPosFaction.id = 2;
    if (targetingSystem.AcquireTarget(entityManager, shooter, targetNoPosition)) {
        std::cerr << "AcquireTarget should fail when target lacks position" << std::endl;
        return false;
    }

    noPosFaction.id = 1;
    auto& sameFactionPos = entityManager.EmplaceComponent<Position>(targetNoPosition);
    sameFactionPos.x = 100.0;
    sameFactionPos.y = 0.0;
    sameFactionPos.z = 0.0;
    if (targetingSystem.AcquireTarget(entityManager, shooter, targetNoPosition)) {
        std::cerr << "AcquireTarget should fail for same faction" << std::endl;
        return false;
    }

    int outOfRangeTarget = entityManager.CreateEntity();
    auto& farFaction = entityManager.EmplaceComponent<Faction>(outOfRangeTarget);
    farFaction.id = 2;
    auto& farPos = entityManager.EmplaceComponent<Position>(outOfRangeTarget);
    farPos.x = 20000.0; // 20 km away
    farPos.y = 0.0;
    farPos.z = 0.0;
    if (targetingSystem.AcquireTarget(entityManager, shooter, outOfRangeTarget)) {
        std::cerr << "AcquireTarget should fail for out-of-range target" << std::endl;
        return false;
    }

    int validTarget = entityManager.CreateEntity();
    auto& validFaction = entityManager.EmplaceComponent<Faction>(validTarget);
    validFaction.id = 3;
    auto& validPos = entityManager.EmplaceComponent<Position>(validTarget);
    validPos.x = 3000.0; // 3 km
    validPos.y = 0.0;
    validPos.z = 0.0;

    if (!targetingSystem.AcquireTarget(entityManager, shooter, validTarget)) {
        std::cerr << "AcquireTarget should succeed for valid target" << std::endl;
        return false;
    }

    return true;
}

bool TestObstructedLineOfSight() {
    EntityManager entityManager;
    TargetingSystem targetingSystem;

    int shooter = entityManager.CreateEntity();
    auto& shooterPos = entityManager.EmplaceComponent<Position>(shooter);
    shooterPos.x = 0.0;
    shooterPos.y = 0.0;
    shooterPos.z = 0.0;
    auto& shooterFaction = entityManager.EmplaceComponent<Faction>(shooter);
    shooterFaction.id = 1;

    int target = entityManager.CreateEntity();
    auto& targetPos = entityManager.EmplaceComponent<Position>(target);
    targetPos.x = 1000.0;
    targetPos.y = 0.0;
    targetPos.z = 0.0;
    auto& targetFaction = entityManager.EmplaceComponent<Faction>(target);
    targetFaction.id = 2;

    targetingSystem.SetLineOfSightValidator([](const Vec3& shooterPosVec, const Vec3& targetPosVec) {
        // Block line of sight if target is within x > 500 and y within +-50
        if (targetPosVec.x > 500.0f && std::fabs(targetPosVec.y - shooterPosVec.y) < 50.0f) {
            return false;
        }
        return true;
    });

    if (targetingSystem.AcquireTarget(entityManager, shooter, target)) {
        std::cerr << "AcquireTarget should fail when line of sight is blocked" << std::endl;
        return false;
    }

    targetPos.y = 200.0; // Move outside of obstruction corridor
    if (!targetingSystem.AcquireTarget(entityManager, shooter, target)) {
        std::cerr << "AcquireTarget should succeed when line of sight is clear" << std::endl;
        return false;
    }

    return true;
}

bool TestWeaponCooldownEdgeCases() {
    EntityManager entityManager;
    WeaponSystem weaponSystem;

    int shooter = entityManager.CreateEntity();
    auto& shooterPos = entityManager.EmplaceComponent<Position>(shooter);
    shooterPos.x = 0.0;
    shooterPos.y = 0.0;
    shooterPos.z = 0.0;

    WeaponSystem::WeaponSlotConfig config;
    config.fireRatePerSecond = 2.0f; // 0.5s cooldown
    config.ammo = 2;
    config.damage = 25.0;
    config.projectileSpeed = 300.0;
    config.projectileLifetime = 1.5;
    config.muzzleDirX = 1.0f;
    config.muzzleDirY = 0.0f;
    config.muzzleDirZ = 0.0f;
    weaponSystem.ConfigureWeaponSlot(shooter, "primary", config);

    FeedbackEventManager::Get().Clear();
    auto listener = std::make_shared<TestFeedbackListener>();
    FeedbackEventManager::Get().Subscribe(listener);

    if (!weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
        std::cerr << "First shot should fire" << std::endl;
        return false;
    }

    auto projectiles = entityManager.GetAllWith<Projectile>();
    if (projectiles.size() != 1) {
        std::cerr << "Expected one projectile after first shot" << std::endl;
        return false;
    }

    auto* lifetime = entityManager.GetComponent<Lifetime>(projectiles.back().first);
    if (!lifetime || !approxEqual(lifetime->remaining, config.projectileLifetime)) {
        std::cerr << "Projectile lifetime not initialized correctly" << std::endl;
        return false;
    }

    if (weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
        std::cerr << "Weapon should not fire while on cooldown" << std::endl;
        return false;
    }

    if (listener->events.empty() || listener->events.back().type != FeedbackEventType::WeaponOverheat) {
        std::cerr << "Expected WeaponOverheat event when firing during cooldown" << std::endl;
        return false;
    }

    weaponSystem.Update(entityManager, 0.25);
    if (weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
        std::cerr << "Weapon should still be cooling down" << std::endl;
        return false;
    }

    weaponSystem.Update(entityManager, 0.25);
    if (!weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
        std::cerr << "Weapon should fire after cooldown" << std::endl;
        return false;
    }

    projectiles = entityManager.GetAllWith<Projectile>();
    if (projectiles.size() != 2) {
        std::cerr << "Expected two projectiles after second shot" << std::endl;
        return false;
    }

    weaponSystem.Update(entityManager, 0.5);
    if (weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
        std::cerr << "Weapon should not fire with empty ammo" << std::endl;
        return false;
    }

    if (listener->events.back().type != FeedbackEventType::AmmoEmpty) {
        std::cerr << "Expected AmmoEmpty event when out of ammo" << std::endl;
        return false;
    }

    weaponSystem.Update(entityManager, 2.0);
    projectiles = entityManager.GetAllWith<Projectile>();
    if (!projectiles.empty()) {
        std::cerr << "Projectiles should expire after lifetime" << std::endl;
        return false;
    }

    return true;
}

int main() {
    if (!TestInvalidTargets()) {
        return 1;
    }

    if (!TestObstructedLineOfSight()) {
        return 2;
    }

    if (!TestWeaponCooldownEdgeCases()) {
        return 3;
    }

    std::cout << "Weapon and targeting system tests passed." << std::endl;
    return 0;
}
