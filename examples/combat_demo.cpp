/**
 * Combat System Demo
 * 
 * This example demonstrates the Advanced Combat System features:
 * - Creating combat-ready ships
 * - Weapon firing and projectile systems
 * - Shield mechanics
 * - Subsystem damage
 * - Combat AI
 * - Squadron management
 */

#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/CombatComponents.h"
#include "../engine/ecs/CombatSystems.h"
#include "../engine/ecs/Components.h"
#include <iostream>
#include <memory>

// Helper function to create a fully-equipped combat ship
ecs::EntityHandle CreateCombatFighter(EntityManager& em, 
                                     const std::string& name,
                                     double posX, double posY, double posZ,
                                     bool isPlayer = false) {
    auto ship = em.CreateEntity();
    
    // Basic components
    em.AddComponent<Name>(ship, std::make_shared<Name>());
    em.GetComponent<Name>(ship)->value = name;
    
    em.AddComponent<Position>(ship, std::make_shared<Position>(posX, posY, posZ));
    em.AddComponent<Velocity>(ship, std::make_shared<Velocity>(0, 0, 0));
    em.AddComponent<Health>(ship, std::make_shared<Health>(500, 500));
    
    // Hull and armor
    auto hull = std::make_shared<HullDamage>();
    hull->currentArmor = 300.0;
    hull->maxArmor = 300.0;
    hull->currentHull = 500.0;
    hull->maxHull = 500.0;
    hull->armorEffectiveness = 0.7;
    em.AddComponent<HullDamage>(ship, hull);
    
    // Subsystems
    auto subsystems = std::make_shared<SubsystemHealth>();
    subsystems->InitializeSubsystem(SubsystemType::Engines, 250.0);
    subsystems->InitializeSubsystem(SubsystemType::Weapons, 200.0);
    subsystems->InitializeSubsystem(SubsystemType::Shields, 220.0);
    subsystems->InitializeSubsystem(SubsystemType::Sensors, 180.0);
    subsystems->InitializeSubsystem(SubsystemType::PowerPlant, 300.0);
    em.AddComponent<SubsystemHealth>(ship, subsystems);
    
    // Directional shields
    auto shields = std::make_shared<DirectionalShields>();
    shields->InitializeFace(ShieldFacing::Forward, 150.0);
    shields->InitializeFace(ShieldFacing::Aft, 100.0);
    shields->InitializeFace(ShieldFacing::Port, 120.0);
    shields->InitializeFace(ShieldFacing::Starboard, 120.0);
    shields->InitializeFace(ShieldFacing::Dorsal, 100.0);
    shields->InitializeFace(ShieldFacing::Ventral, 100.0);
    shields->canRebalance = true;
    shields->rebalanceRate = 15.0;
    shields->shieldsEnabled = true;
    em.AddComponent<DirectionalShields>(ship, shields);
    
    // Primary laser weapon
    auto laser = std::make_shared<WeaponSystem>();
    laser->weaponId = "laser_cannon";
    laser->type = WeaponType::Laser;
    laser->damageType = DamageType::Energy;
    laser->baseDamage = 80.0;
    laser->fireRate = 3.0;
    laser->projectileSpeed = 2000.0;
    laser->projectileLifetime = 3.0;
    laser->cooldown = 1.0 / laser->fireRate;
    laser->accuracy = 0.9;
    laser->spread = 1.0;
    laser->optimalRange = 1000.0;
    laser->maxRange = 2000.0;
    laser->energyCost = 8.0;
    laser->heatPerShot = 4.0;
    laser->ammo = -1;  // Energy weapon
    laser->armorPenetration = 0.3;
    laser->shieldPenetration = 0.5;
    em.AddComponent<WeaponSystem>(ship, laser);
    
    // Missile launcher
    auto missiles = std::make_shared<MissileWeapon>();
    missiles->weaponId = "missile_pod";
    missiles->missileType = MissileWeapon::MissileType::Heatseeking;
    missiles->ammo = 8;
    missiles->maxAmmo = 8;
    missiles->missileDamage = 400.0;
    missiles->missileSpeed = 250.0;
    missiles->missileAcceleration = 40.0;
    missiles->missileMaxSpeed = 450.0;
    missiles->missileTurnRate = 120.0;
    missiles->missileLifetime = 15.0;
    missiles->missileArmingRange = 30.0;
    missiles->missileBlastRadius = 20.0;
    missiles->lockOnTime = 2.5;
    missiles->reloadTime = 4.0;
    em.AddComponent<MissileWeapon>(ship, missiles);
    
    // Targeting system
    auto targeting = std::make_shared<TargetingSubsystem>();
    targeting->mode = TargetingMode::Assisted;
    targeting->maxRange = 5000.0;
    targeting->maxMissileRange = 8000.0;
    targeting->lockOnTime = 2.0;
    targeting->scanInterval = 0.5;
    em.AddComponent<TargetingSubsystem>(ship, targeting);
    
    // Electronic warfare
    auto ew = std::make_shared<ElectronicWarfare>();
    ew->chaffCount = 6;
    ew->chaffMax = 6;
    ew->flareCount = 6;
    ew->flareMax = 6;
    ew->decoyCount = 2;
    ew->decoyMax = 2;
    ew->countermeasureCooldown = 1.5;
    ew->jammingStrength = 0.3;
    ew->jamResistance = 0.4;
    ew->radarCrossSection = 0.8;
    em.AddComponent<ElectronicWarfare>(ship, ew);
    
    // Sensor system
    auto sensors = std::make_shared<SensorSystem>();
    sensors->activeSensors[SensorSystem::SensorType::Radar] = true;
    sensors->activeSensors[SensorSystem::SensorType::Infrared] = true;
    sensors->radarRange = 8000.0;
    sensors->irRange = 4000.0;
    sensors->scanResolution = 50.0;
    sensors->trackingAccuracy = 0.85;
    em.AddComponent<SensorSystem>(ship, sensors);
    
    // Damage control
    auto dc = std::make_shared<DamageControl>();
    dc->crewCount = 3;
    dc->maxCrew = 4;
    dc->availableCrew = 3;
    dc->repairKits = 6;
    dc->maxRepairKits = 8;
    dc->extinguishers = 4;
    dc->autoRepair = true;
    dc->repairPriority = SubsystemType::Engines;
    em.AddComponent<DamageControl>(ship, dc);
    
    // Combat statistics
    em.AddComponent<CombatStatistics>(ship, std::make_shared<CombatStatistics>());
    
    // Combat AI (if not player)
    if (!isPlayer) {
        auto ai = std::make_shared<CombatAI>();
        ai->difficulty = CombatAIDifficulty::Medium;
        ai->currentBehavior = CombatAI::Behavior::Balanced;
        ai->aggressionLevel = 0.6;
        ai->selfPreservation = 0.5;
        ai->teamwork = 0.7;
        ai->engagementRange = 1200.0;
        ai->fleeThreshold = 0.25;
        ai->useEvasiveManeuvers = true;
        ai->useCover = true;
        ai->decisionInterval = 1.0;
        em.AddComponent<CombatAI>(ship, ai);
    }
    
    // Render component
    auto draw = std::make_shared<DrawComponent>();
    draw->mode = DrawComponent::RenderMode::Mesh3D;
    draw->visible = true;
    if (isPlayer) {
        draw->SetTint(0.2f, 0.8f, 0.2f);  // Green for player
    } else {
        draw->SetTint(0.8f, 0.2f, 0.2f);  // Red for enemies
    }
    em.AddComponent<DrawComponent>(ship, draw);
    
    std::cout << "Created combat ship: " << name << std::endl;
    return ship;
}

// Create a squadron of fighters
std::vector<ecs::EntityHandle> CreateSquadron(EntityManager& em,
                                              const std::string& squadronId,
                                              int fighterCount,
                                              double baseX, double baseY, double baseZ) {
    std::vector<ecs::EntityHandle> squadron;
    
    for (int i = 0; i < fighterCount; ++i) {
        std::string name = squadronId + "_" + std::to_string(i + 1);
        double offsetX = (i % 3) * 50.0;
        double offsetZ = (i / 3) * 50.0;
        
        auto fighter = CreateCombatFighter(em, name, 
                                          baseX + offsetX, 
                                          baseY, 
                                          baseZ + offsetZ,
                                          false);
        
        // Add squadron component
        auto member = std::make_shared<SquadronMember>();
        member->squadronId = squadronId;
        member->position = i;
        member->role = (i == 0) ? SquadronMember::Role::Leader : 
                                 SquadronMember::Role::Wingman;
        
        if (i > 0) {
            member->leader = squadron[0];  // First fighter is leader
        }
        
        em.AddComponent<SquadronMember>(fighter, member);
        squadron.push_back(fighter);
    }
    
    std::cout << "Created squadron: " << squadronId 
              << " with " << fighterCount << " fighters" << std::endl;
    return squadron;
}

// Demonstrate combat scenario
void DemoCombatScenario(EntityManager& em) {
    std::cout << "\n=== Combat System Demo ===" << std::endl;
    std::cout << "Setting up combat scenario..." << std::endl;
    
    // Create player ship
    auto playerShip = CreateCombatFighter(em, "Player", 0.0, 0.0, 0.0, true);
    std::cout << "\nPlayer ship created at origin" << std::endl;
    
    // Create enemy squadron
    auto enemySquadron = CreateSquadron(em, "Alpha_Squadron", 4, 2000.0, 0.0, 500.0);
    std::cout << "\nEnemy squadron spawned at range" << std::endl;
    
    // Set up targeting - player targets squadron leader
    auto playerTargeting = em.GetComponent<TargetingSubsystem>(playerShip);
    if (playerTargeting && !enemySquadron.empty()) {
        playerTargeting->currentTarget = enemySquadron[0];
        playerTargeting->targetedSubsystem = SubsystemType::Engines;
        std::cout << "Player targeting squadron leader's engines" << std::endl;
    }
    
    // Enemy AI targets player
    for (auto enemy : enemySquadron) {
        auto ai = em.GetComponent<CombatAI>(enemy);
        if (ai) {
            ai->primaryTarget = playerShip;
        }
    }
    std::cout << "Enemy squadron locked onto player" << std::endl;
    
    // Initialize combat systems
    WeaponFireSystem weaponSystem;
    ProjectileSystem projectileSystem;
    AdvancedTargetingSystem targetingSystem;
    DirectionalShieldSystem shieldSystem;
    SubsystemDamageSystem subsystemSystem;
    CombatAISystem aiSystem;
    SquadronSystem squadronSystem;
    DamageControlSystem damageControlSystem;
    CombatStatisticsSystem statsSystem;
    
    std::cout << "\nCombat systems initialized" << std::endl;
    std::cout << "Running combat simulation..." << std::endl;
    
    // Simulate combat for a few seconds
    double dt = 0.016;  // ~60 FPS
    int frames = 300;   // ~5 seconds
    
    for (int frame = 0; frame < frames; ++frame) {
        // Update combat systems
        targetingSystem.Update(em, dt);
        aiSystem.Update(em, dt);
        squadronSystem.Update(em, dt);
        weaponSystem.Update(em, dt);
        projectileSystem.Update(em, dt);
        shieldSystem.Update(em, dt);
        subsystemSystem.Update(em, dt);
        damageControlSystem.Update(em, dt);
        statsSystem.Update(em, dt);
        
        // Print status every second
        if (frame % 60 == 0) {
            int second = frame / 60;
            std::cout << "\n--- Second " << second << " ---" << std::endl;
            
            // Player status
            auto playerHealth = em.GetComponent<Health>(playerShip);
            auto playerShields = em.GetComponent<DirectionalShields>(playerShip);
            if (playerHealth && playerShields) {
                double shieldTotal = playerShields->GetTotalShields();
                std::cout << "Player: HP=" << playerHealth->current 
                         << "/" << playerHealth->maximum
                         << " Shields=" << shieldTotal << std::endl;
            }
            
            // Enemy squadron status
            int alive = 0;
            for (auto enemy : enemySquadron) {
                auto health = em.GetComponent<Health>(enemy);
                if (health && health->current > 0.0) {
                    alive++;
                }
            }
            std::cout << "Enemy Squadron: " << alive << "/" 
                     << enemySquadron.size() << " active" << std::endl;
            
            // Player stats
            auto stats = em.GetComponent<CombatStatistics>(playerShip);
            if (stats) {
                std::cout << "Combat Stats: Kills=" << stats->kills
                         << " Accuracy=" << (stats->GetAccuracy() * 100.0) << "%"
                         << " Damage Dealt=" << stats->totalDamageDealt << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Combat Demo Complete ===" << std::endl;
    
    // Final statistics
    auto finalStats = em.GetComponent<CombatStatistics>(playerShip);
    if (finalStats) {
        std::cout << "\nFinal Player Statistics:" << std::endl;
        std::cout << "  Kills: " << finalStats->kills << std::endl;
        std::cout << "  Assists: " << finalStats->assists << std::endl;
        std::cout << "  Shots Fired: " << finalStats->shotsFired << std::endl;
        std::cout << "  Shots Hit: " << finalStats->shotsHit << std::endl;
        std::cout << "  Accuracy: " << (finalStats->GetAccuracy() * 100.0) << "%" << std::endl;
        std::cout << "  Total Damage Dealt: " << finalStats->totalDamageDealt << std::endl;
        std::cout << "  Total Damage Received: " << finalStats->totalDamageReceived << std::endl;
        std::cout << "  Time in Combat: " << finalStats->timeInCombat << "s" << std::endl;
    }
}

// Demonstrate subsystem targeting
void DemoSubsystemTargeting(EntityManager& em) {
    std::cout << "\n=== Subsystem Targeting Demo ===" << std::endl;
    
    auto ship = CreateCombatFighter(em, "TestShip", 0, 0, 0, false);
    
    auto subsystems = em.GetComponent<SubsystemHealth>(ship);
    if (!subsystems) return;
    
    std::cout << "\nInitial subsystem status:" << std::endl;
    for (const auto& pair : subsystems->subsystems) {
        std::cout << "  " << static_cast<int>(pair.first) << ": " 
                 << pair.second.currentHP << "/" << pair.second.maxHP
                 << " (" << (pair.second.GetHealthPercent() * 100.0) << "%)" << std::endl;
    }
    
    // Damage engines
    SubsystemDamageSystem damageSystem;
    damageSystem.DamageSubsystem(*subsystems, SubsystemType::Engines, 150.0);
    
    std::cout << "\nAfter damaging engines by 150:" << std::endl;
    auto& engines = subsystems->subsystems[SubsystemType::Engines];
    std::cout << "  Engines: " << engines.currentHP << "/" << engines.maxHP
             << " State: " << static_cast<int>(engines.state) << std::endl;
    
    // Damage weapons critically
    damageSystem.DamageSubsystem(*subsystems, SubsystemType::Weapons, 180.0);
    
    std::cout << "\nAfter critically damaging weapons by 180:" << std::endl;
    auto& weapons = subsystems->subsystems[SubsystemType::Weapons];
    std::cout << "  Weapons: " << weapons.currentHP << "/" << weapons.maxHP
             << " State: " << static_cast<int>(weapons.state) << std::endl;
    
    std::cout << "\nSubsystem targeting demo complete!" << std::endl;
}

// Main demo function
int main() {
    std::cout << "Nova Engine - Advanced Combat System Demo" << std::endl;
    std::cout << "==========================================\n" << std::endl;
    
    // Create entity manager
    EntityManager em;
    
    // Run demos
    DemoSubsystemTargeting(em);
    DemoCombatScenario(em);
    
    std::cout << "\nAll demos completed successfully!" << std::endl;
    return 0;
}
