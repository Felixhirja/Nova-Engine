#pragma once

#include "System.h"
#include "CombatComponents.h"
#include "Components.h"
#include "EntityManager.h"
#include "QueryBuilder.h"
#include <vector>
#include <random>

// ============================================================================
// ADVANCED COMBAT SYSTEMS
// ============================================================================

/**
 * WeaponFireSystem: Handles weapon firing and projectile spawning
 */
class WeaponFireSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
private:
    void ProcessEnergyWeapon(EntityManager& em, ecs::EntityHandle entity, 
                            WeaponSystem& weapon, double dt);
    void ProcessBallisticWeapon(EntityManager& em, ecs::EntityHandle entity,
                               WeaponSystem& weapon, double dt);
    void ProcessMissileWeapon(EntityManager& em, ecs::EntityHandle entity,
                             MissileWeapon& missile, double dt);
    
    ecs::EntityHandle SpawnProjectile(EntityManager& em, ecs::EntityHandle owner,
                                     const WeaponSystem& weapon,
                                     double posX, double posY, double posZ,
                                     double dirX, double dirY, double dirZ);
    
    ecs::EntityHandle SpawnMissile(EntityManager& em, ecs::EntityHandle owner,
                                  const MissileWeapon& launcher,
                                  ecs::EntityHandle target);
};

/**
 * ProjectileSystem: Updates projectile movement and collision
 */
class ProjectileSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
private:
    void UpdateGuidedProjectile(EntityManager& em, ecs::EntityHandle entity,
                               ProjectileData& proj, Position& pos,
                               Velocity& vel, double dt);
    
    bool CheckCollision(EntityManager& em, ecs::EntityHandle projectile,
                       const Position& projPos, double& hitDist);
    
    void ApplyDamage(EntityManager& em, ecs::EntityHandle projectile,
                    ecs::EntityHandle target, const ProjectileData& data);
};

/**
 * TargetingSystem: Handles target acquisition and tracking
 */
class AdvancedTargetingSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    // Find nearest hostile target
    ecs::EntityHandle FindNearestTarget(EntityManager& em, ecs::EntityHandle shooter,
                                       double maxRange);
    
    // Calculate lead position for moving targets
    bool CalculateLead(EntityManager& em, ecs::EntityHandle shooter,
                      ecs::EntityHandle target, double projectileSpeed,
                      double& leadX, double& leadY, double& leadZ);
    
private:
    void UpdateTargetLock(EntityManager& em, ecs::EntityHandle entity,
                         TargetingSubsystem& targeting, double dt);
    
    bool IsValidTarget(EntityManager& em, ecs::EntityHandle shooter,
                      ecs::EntityHandle target);
    
    bool HasLineOfSight(EntityManager& em, ecs::EntityHandle shooter,
                       ecs::EntityHandle target);
};

/**
 * ShieldSystem: Manages directional shields
 */
class DirectionalShieldSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    // Apply damage to appropriate shield facing
    double ApplyShieldDamage(DirectionalShields& shields, ShieldFacing facing,
                            double damage, double penetration, double currentTime);
    
    // Determine which shield facing was hit based on angle
    ShieldFacing DetermineFacing(double impactX, double impactY, double impactZ,
                                double shipX, double shipY, double shipZ,
                                double shipYaw, double shipPitch, double shipRoll);
    
private:
    void RechargeShields(DirectionalShields& shields, double dt, double currentTime);
    void HandleShieldRebalancing(DirectionalShields& shields, double dt);
};

/**
 * SubsystemDamageSystem: Tracks subsystem health and failures
 */
class SubsystemDamageSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    // Apply damage to specific subsystem
    void DamageSubsystem(SubsystemHealth& health, SubsystemType type,
                        double damage);
    
private:
    void UpdateSubsystemEffects(EntityManager& em, ecs::EntityHandle entity,
                               SubsystemHealth& health, double dt);
    
    void ProcessFires(EntityManager& em, ecs::EntityHandle entity,
                     SubsystemHealth& health, double dt);
    
    void ProcessBreaches(EntityManager& em, ecs::EntityHandle entity,
                        SubsystemHealth& health, double dt);
};

/**
 * ElectronicWarfareSystem: ECM, jamming, countermeasures
 */
class ElectronicWarfareSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    void DeployCountermeasure(EntityManager& em, ecs::EntityHandle entity,
                             ElectronicWarfare& ew, bool isFlare);
    
    void DeployDecoy(EntityManager& em, ecs::EntityHandle entity,
                    ElectronicWarfare& ew);
    
private:
    void UpdateJamming(EntityManager& em, double dt);
    void UpdateCountermeasures(EntityManager& em, double dt);
    void UpdateStealth(EntityManager& em, ecs::EntityHandle entity,
                      ElectronicWarfare& ew);
};

/**
 * SensorSystem: Detection, tracking, and sensor contacts
 */
class SensorUpdateSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
private:
    void ScanForContacts(EntityManager& em, ecs::EntityHandle entity,
                        SensorSystem& sensors, const Position& pos, double dt);
    
    bool CanDetect(const SensorSystem& sensors, const Position& sensorPos,
                  const Position& targetPos, const ElectronicWarfare* targetEW);
    
    void UpdateContactTracking(SensorSystem& sensors, double dt);
};

/**
 * CombatAISystem: AI decision making for combat
 */
class CombatAISystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
private:
    void MakeDecision(EntityManager& em, ecs::EntityHandle entity,
                     CombatAI& ai, double dt);
    
    void SelectTarget(EntityManager& em, ecs::EntityHandle entity, CombatAI& ai);
    
    void ExecuteBehavior(EntityManager& em, ecs::EntityHandle entity,
                        CombatAI& ai, double dt);
    
    void CalculateThreatLevel(EntityManager& em, ecs::EntityHandle entity,
                             ecs::EntityHandle threat, double& threatScore);
    
    void PerformEvasiveManeuvers(EntityManager& em, ecs::EntityHandle entity,
                                CombatAI& ai);
    
    void AdjustBehavior(CombatAI& ai, double healthPercent, double shieldPercent);
};

/**
 * SquadronSystem: Manages fighter squadrons and wing formations
 */
class SquadronSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    void IssueCommand(EntityManager& em, const std::string& squadronId,
                     SquadronMember::Command command,
                     ecs::EntityHandle target = ecs::EntityHandle::Null());
    
private:
    void UpdateFormation(EntityManager& em, ecs::EntityHandle entity,
                        SquadronMember& member);
    
    void ExecuteCommand(EntityManager& em, ecs::EntityHandle entity,
                       SquadronMember& member);
    
    void MaintainFormation(EntityManager& em, ecs::EntityHandle entity,
                          SquadronMember& member, const Position& leaderPos);
};

/**
 * DamageControlSystem: Handles repairs, fire suppression, breach sealing
 */
class DamageControlSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    void AssignRepairCrew(DamageControl& dc, SubsystemType subsystem, int crewCount);
    
private:
    void ProcessRepairs(EntityManager& em, ecs::EntityHandle entity,
                       DamageControl& dc, SubsystemHealth& health, double dt);
    
    void AutoAssignRepairs(DamageControl& dc, SubsystemHealth& health);
    
    void SuppressFires(EntityManager& em, ecs::EntityHandle entity,
                      DamageControl& dc, SubsystemHealth& health, double dt);
    
    void SealBreaches(EntityManager& em, ecs::EntityHandle entity,
                     DamageControl& dc, SubsystemHealth& health, double dt);
};

/**
 * BoardingSystem: Handles boarding actions and ship capture
 */
class BoardingSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    bool InitiateBoardingAction(EntityManager& em, ecs::EntityHandle attacker,
                               ecs::EntityHandle target);
    
private:
    void UpdateBoardingPhase(EntityManager& em, ecs::EntityHandle entity,
                            BoardingParty& party, double dt);
    
    void ResolveBoardingCombat(EntityManager& em, BoardingParty& attackers,
                              DamageControl& defenders, double dt);
    
    void CaptureShip(EntityManager& em, ecs::EntityHandle target,
                    ecs::EntityHandle capturer);
};

/**
 * SalvageSystem: Handles wreck salvaging and component recovery
 */
class SalvageManagementSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    void CreateWreck(EntityManager& em, ecs::EntityHandle destroyedShip);
    
private:
    void UpdateSalvageOperations(EntityManager& em, double dt);
    void UpdateWreckAging(EntityManager& em, double dt);
    void GenerateSalvageComponents(WreckData& wreck, const std::string& shipClass);
};

/**
 * CombatStatisticsSystem: Tracks combat stats and achievements
 */
class CombatStatisticsSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    void RecordKill(EntityManager& em, ecs::EntityHandle killer,
                   ecs::EntityHandle victim, WeaponType weapon, double range);
    
    void RecordAssist(EntityManager& em, ecs::EntityHandle assistant,
                     ecs::EntityHandle victim);
    
    void RecordDamage(EntityManager& em, ecs::EntityHandle attacker,
                     ecs::EntityHandle victim, double damage);
    
private:
    void UpdateCombatTime(EntityManager& em, double dt);
    void CheckAchievements(EntityManager& em, ecs::EntityHandle entity);
};

/**
 * MineFieldSystem: Manages deployed mines
 */
class MineFieldSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    ecs::EntityHandle DeployMine(EntityManager& em, ecs::EntityHandle deployer,
                                 const MineLayer& layer);
    
private:
    void UpdateMineProximity(EntityManager& em, double dt);
    void DetonateMine(EntityManager& em, ecs::EntityHandle mine);
};

/**
 * TractorBeamSystem: Tractor beam operations
 */
class TractorBeamSystem : public UnifiedSystem {
public:
    void Update(EntityManager& em, double dt) override;
    
    bool ActivateTractorBeam(EntityManager& em, ecs::EntityHandle source,
                            ecs::EntityHandle target);
    
    void DeactivateTractorBeam(EntityManager& em, ecs::EntityHandle source);
    
private:
    void ApplyTractorForce(EntityManager& em, ecs::EntityHandle source,
                          ecs::EntityHandle target, const TractorBeam& beam);
};

// ============================================================================
// COMBAT UTILITIES
// ============================================================================

namespace CombatUtils {
    // Calculate damage after armor/shield penetration
    double CalculateEffectiveDamage(double baseDamage, double penetration,
                                   double armor, double armorEffectiveness);
    
    // Determine if a shot hits based on accuracy, range, and other factors
    bool RollHitChance(double accuracy, double range, double optimalRange,
                      double maxRange, double targetSpeed, double jamming);
    
    // Calculate angle between attacker and target
    double CalculateAngle(double fromX, double fromY, double fromZ,
                         double toX, double toY, double toZ,
                         double dirX, double dirY, double dirZ);
    
    // Check if angle is within gimbal/turret traverse limits
    bool IsWithinTraverse(double angle, double maxYaw, double maxPitch);
    
    // Generate random spread for weapon fire
    void ApplyWeaponSpread(double& dirX, double& dirY, double& dirZ,
                          double spreadDegrees, std::mt19937& rng);
    
    // Calculate interception point for moving target
    bool CalculateInterceptPoint(double projSpeed,
                                double shooterX, double shooterY, double shooterZ,
                                double targetX, double targetY, double targetZ,
                                double targetVX, double targetVY, double targetVZ,
                                double& interceptX, double& interceptY, double& interceptZ,
                                double& timeToIntercept);
    
    // Damage type effectiveness vs shields/armor
    double GetDamageTypeMultiplier(DamageType damage, bool vsShield);
    
    // Convert world position to ship-relative direction
    ShieldFacing WorldToShieldFacing(double relX, double relY, double relZ);
    
    // Calculate explosion damage with falloff
    double CalculateExplosionDamage(double baseDamage, double distance,
                                   double blastRadius);
}

#endif // COMBAT_SYSTEMS_H
