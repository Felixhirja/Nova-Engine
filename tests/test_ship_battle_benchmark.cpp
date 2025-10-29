#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/PhysicsSystem.h"
#include "../engine/ecs/Components.h"
#include "../engine/WeaponSystem.h"
#include "../engine/ShieldSystem.h"
#include "../engine/EnergyManagementSystem.h"
#include "../engine/FeedbackEvent.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

struct BattleShip {
    Entity entityId;
    int factionId;
};

int main() {
    EntityManager entityManager;
    PhysicsSystem physics(&entityManager);
    physics.SetGravity(0.0, 0.0, 0.0);
    physics.SetGlobalDamping(0.0, 0.0);
    physics.SetCollisionEnabled(false);

    WeaponSystem weaponSystem;
    ShieldManagementSystem shieldSystem;
    EnergyManagementSystem energySystem;

    FeedbackEventManager::Get().Clear();

    const int factions = 2;
    const int shipsPerFaction = 120;
    const int totalShips = factions * shipsPerFaction;
    const double spacing = 450.0;

    std::vector<BattleShip> fleet;
    fleet.reserve(totalShips);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> offsetDist(-spacing * 0.5, spacing * 0.5);
    std::uniform_real_distribution<double> velocityDist(-6.0, 6.0);
    std::uniform_real_distribution<double> damageDist(18.0, 55.0);

    WeaponSystem::WeaponSlotConfig weaponConfig;
    weaponConfig.fireRatePerSecond = 4.0f;
    weaponConfig.ammo = -1;
    weaponConfig.damage = 28.0;
    weaponConfig.projectileSpeed = 2400.0;
    weaponConfig.projectileLifetime = 4.5;
    weaponConfig.muzzleOffsetX = 2.0;
    weaponConfig.muzzleOffsetY = 0.5;
    weaponConfig.muzzleOffsetZ = 0.0;
    weaponConfig.muzzleDirX = 1.0f;
    weaponConfig.muzzleDirY = 0.0f;
    weaponConfig.muzzleDirZ = 0.0f;

    for (int faction = 0; faction < factions; ++faction) {
        for (int i = 0; i < shipsPerFaction; ++i) {
            Entity entity = entityManager.CreateEntity();
            auto& position = entityManager.EmplaceComponent<Position>(entity);
            position.x = faction * 5200.0 + offsetDist(rng);
            position.y = i * 18.0 + offsetDist(rng) * 0.1;
            position.z = offsetDist(rng) * 0.05;

            auto& velocity = entityManager.EmplaceComponent<Velocity>(entity);
            velocity.vx = velocityDist(rng);
            velocity.vy = velocityDist(rng);
            velocity.vz = velocityDist(rng) * 0.25;

            auto& rigidBody = entityManager.EmplaceComponent<RigidBody>(entity);
            rigidBody.SetMass(32000.0);
            rigidBody.useGravity = false;
            rigidBody.linearDamping = 0.01;
            rigidBody.angularDamping = 0.01;

            auto& factionComponent = entityManager.EmplaceComponent<Faction>(entity);
            factionComponent.id = faction;

            weaponSystem.ConfigureWeaponSlot(entity, "primary", weaponConfig);
            shieldSystem.InitializeShield(entity, 320.0, 32.0, 1.8, 0.8, "shield_array_light");
            energySystem.Initialize(entity, 60.0, 18.0, 18.0, 18.0);

            fleet.push_back({entity, faction});
        }
    }

    const double dt = 1.0 / 60.0;
    const int frameCount = 300;

    size_t totalShotsFired = 0;
    double accumulatedHullOverflow = 0.0;

    auto startTime = std::chrono::steady_clock::now();
    for (int frame = 0; frame < frameCount; ++frame) {
        for (std::size_t i = 0; i < fleet.size(); ++i) {
            const BattleShip& ship = fleet[i];
            const Entity shooter = ship.entityId;

            std::size_t enemyIndex = (i + shipsPerFaction + frame) % fleet.size();
            if (fleet[enemyIndex].factionId == ship.factionId) {
                enemyIndex = (enemyIndex + shipsPerFaction) % fleet.size();
            }
            const Entity target = fleet[enemyIndex].entityId;

            if (weaponSystem.CanFire(shooter, "primary")) {
                if (weaponSystem.FireWeapon(entityManager, shooter, "primary")) {
                    ++totalShotsFired;
                }
            }

            if (((frame + static_cast<int>(i)) % 3) == 0) {
                double overflow = shieldSystem.ApplyDamage(target, damageDist(rng), &entityManager);
                accumulatedHullOverflow += overflow;
                if (overflow > 0.0) {
                    energySystem.DivertPower(target, PowerPriority::Shields, overflow * 0.1);
                }
            }

            const ShieldState* selfShield = shieldSystem.GetShieldState(shooter);
            if (selfShield && selfShield->currentCapacityMJ < selfShield->maxCapacityMJ * 0.6) {
                if (energySystem.HasPower(shooter, PowerPriority::Shields)) {
                    double repairRate = selfShield->maxCapacityMJ * 0.05;
                    shieldSystem.Recharge(shooter, repairRate * dt);
                }
            }

            energySystem.Update(shooter, static_cast<float>(dt));
        }

        weaponSystem.Update(entityManager, dt);
        shieldSystem.Update(entityManager, dt);
        physics.Update(entityManager, dt);
    }
    auto endTime = std::chrono::steady_clock::now();

    const double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    const double simulatedSeconds = frameCount * dt;
    const double framesPerSecond = (elapsedMs > 0.0) ? (simulatedSeconds / (elapsedMs / 1000.0)) : 0.0;

    double averageShield = 0.0;
    double lowestShield = 1.0;
    for (const auto& ship : fleet) {
        double pct = shieldSystem.GetShieldPercentage(ship.entityId);
        averageShield += pct;
        lowestShield = std::min(lowestShield, pct);
    }
    if (!fleet.empty()) {
        averageShield /= static_cast<double>(fleet.size());
    }

    const auto projectileEntities = entityManager.GetAllWith<Projectile>();

    std::cout << "Large-scale ship battle benchmark" << std::endl;
    std::cout << "  Ships simulated: " << fleet.size() << std::endl;
    std::cout << "  Frames simulated: " << frameCount << " (" << simulatedSeconds << " s)" << std::endl;
    std::cout << "  Wall time: " << elapsedMs << " ms" << std::endl;
    std::cout << "  Effective sim speed: " << framesPerSecond << " x real-time" << std::endl;
    std::cout << "  Shots fired: " << totalShotsFired << std::endl;
    std::cout << "  Remaining projectiles: " << projectileEntities.size() << std::endl;
    std::cout << "  Average shield: " << (averageShield * 100.0) << "%" << std::endl;
    std::cout << "  Lowest shield: " << (lowestShield * 100.0) << "%" << std::endl;
    std::cout << "  Hull overflow accumulated: " << accumulatedHullOverflow << " MJ" << std::endl;

    return 0;
}
