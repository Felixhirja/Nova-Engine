#pragma once

#include "Component.h"
#include "EntityHandle.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// ============================================================================
// ADVANCED COMBAT SYSTEM COMPONENTS
// ============================================================================

/**
 * DamageType: Different damage type classifications
 */
enum class DamageType {
    Kinetic,        // Ballistic weapons, mass drivers
    Energy,         // Lasers, particle beams
    Explosive,      // Missiles, torpedoes
    Thermal,        // Plasma, fire
    Electromagnetic, // EMP, ion weapons
    Chemical,       // Corrosive, toxic
    Exotic          // Experimental weapons
};

/**
 * SubsystemType: Ship subsystems that can be targeted
 */
enum class SubsystemType {
    None,
    Engines,
    Weapons,
    Shields,
    Sensors,
    PowerPlant,
    LifeSupport,
    Communications,
    Cockpit,
    CargoHold,
    Hangar
};

/**
 * WeaponType: Classification of weapon types
 */
enum class WeaponType {
    Laser,
    Ballistic,
    Missile,
    Torpedo,
    Mine,
    PointDefense,
    Beam,
    Rail gun
};

/**
 * CombatAIDifficulty: AI difficulty levels
 */
enum class CombatAIDifficulty {
    Civilian,       // Non-combatant
    Easy,           // Basic tactics
    Medium,         // Competent fighter
    Hard,           // Skilled veteran
    Expert,         // Ace pilot
    Elite           // Squadron leader
};

/**
 * ShieldFacing: Directional shield segments
 */
enum class ShieldFacing {
    Forward,
    Aft,
    Port,
    Starboard,
    Dorsal,
    Ventral
};

/**
 * DamageState: Progressive damage states for subsystems
 */
enum class DamageState {
    Operational,    // 100-75%
    Damaged,        // 75-40%
    Critical,       // 40-10%
    Failed,         // 10-0%
    Destroyed       // 0%
};

/**
 * TargetingMode: Targeting computer modes
 */
enum class TargetingMode {
    Manual,
    Assisted,
    Full Auto,
    LeadComputed
};

// ============================================================================
// SUBSYSTEM TARGETING
// ============================================================================

/**
 * SubsystemHealth: Health tracking for individual subsystems
 */
struct SubsystemHealth : public Component {
    struct Subsystem {
        SubsystemType type = SubsystemType::None;
        double currentHP = 100.0;
        double maxHP = 100.0;
        DamageState state = DamageState::Operational;
        bool isOnFire = false;
        double fireIntensity = 0.0;
        bool hasBreach = false;
        double repairProgress = 0.0;
        double malfunctionChance = 0.0;
        
        double GetHealthPercent() const {
            return maxHP > 0.0 ? (currentHP / maxHP) : 0.0;
        }
        
        void UpdateDamageState() {
            double percent = GetHealthPercent();
            if (percent >= 0.75) state = DamageState::Operational;
            else if (percent >= 0.40) state = DamageState::Damaged;
            else if (percent >= 0.10) state = DamageState::Critical;
            else if (percent > 0.0) state = DamageState::Failed;
            else state = DamageState::Destroyed;
        }
    };
    
    std::unordered_map<SubsystemType, Subsystem> subsystems;
    
    void InitializeSubsystem(SubsystemType type, double hp) {
        Subsystem sub;
        sub.type = type;
        sub.currentHP = hp;
        sub.maxHP = hp;
        sub.UpdateDamageState();
        subsystems[type] = sub;
    }
    
    bool IsSubsystemOperational(SubsystemType type) const {
        auto it = subsystems.find(type);
        if (it == subsystems.end()) return false;
        return it->second.state == DamageState::Operational || 
               it->second.state == DamageState::Damaged;
    }
};

/**
 * TargetingSubsystem: Advanced targeting computer
 */
struct TargetingSubsystem : public Component {
    ecs::EntityHandle currentTarget = ecs::EntityHandle::Null();
    SubsystemType targetedSubsystem = SubsystemType::None;
    TargetingMode mode = TargetingMode::Assisted;
    
    double lockOnProgress = 0.0;
    double lockOnTime = 2.0;
    bool isLocked = false;
    
    double maxRange = 5000.0;          // meters
    double maxMissileRange = 10000.0;   // meters
    double scanResolution = 100.0;      // meters
    
    std::vector<ecs::EntityHandle> potentialTargets;
    std::vector<ecs::EntityHandle> friendlyTargets;
    
    double lastScanTime = 0.0;
    double scanInterval = 0.5;          // seconds
    
    // Lead targeting
    double leadX = 0.0;
    double leadY = 0.0;
    double leadZ = 0.0;
    bool leadValid = false;
};

// ============================================================================
// WEAPON SYSTEMS
// ============================================================================

/**
 * WeaponHardpointAdvanced: Enhanced hardpoint with more detail
 */
struct WeaponHardpointAdvanced : public Component {
    enum class Size { Small, Medium, Large, Capital, Spinal };
    enum class MountType { Fixed, Gimbal, Turret, Spinal };
    
    std::string hardpointId;
    Size size = Size::Small;
    MountType mountType = MountType::Fixed;
    
    double posX = 0.0, posY = 0.0, posZ = 0.0;  // Position on ship
    double dirX = 1.0, dirY = 0.0, dirZ = 0.0;  // Forward direction
    
    double maxYaw = 45.0;               // degrees
    double maxPitch = 45.0;             // degrees
    double currentYaw = 0.0;
    double currentPitch = 0.0;
    double rotationSpeed = 90.0;        // degrees per second
    
    bool occupied = false;
    std::string equippedWeapon;
    
    double powerDraw = 0.0;             // MW
    double heatGeneration = 0.0;        // per shot
};

/**
 * WeaponSystem: Individual weapon characteristics
 */
struct WeaponSystem : public Component {
    std::string weaponId;
    WeaponType type = WeaponType::Laser;
    DamageType damageType = DamageType::Energy;
    
    std::string hardpointId;            // Which hardpoint is this mounted on
    
    // Damage properties
    double baseDamage = 100.0;
    double armorPenetration = 0.5;      // 0-1, ability to bypass armor
    double shieldPenetration = 0.2;     // 0-1, ability to bypass shields
    
    // Firing characteristics
    double fireRate = 1.0;              // shots per second
    double projectileSpeed = 1000.0;    // m/s
    double projectileLifetime = 5.0;    // seconds
    double cooldown = 1.0;              // seconds
    double currentCooldown = 0.0;
    
    // Resource consumption
    double energyCost = 10.0;           // MW per shot
    double heatPerShot = 5.0;
    int ammo = -1;                      // -1 = infinite (energy weapon)
    int maxAmmo = 100;
    
    // Accuracy
    double accuracy = 0.95;             // 0-1, 1 = perfect
    double spread = 0.5;                // degrees of cone
    double optimalRange = 1000.0;       // meters
    double maxRange = 2000.0;           // meters
    double falloffStart = 1500.0;       // meters
    
    // Status
    bool isFiring = false;
    bool isReloading = false;
    double reloadTime = 2.0;
    double reloadProgress = 0.0;
    bool jammed = false;
    double jamChance = 0.01;            // per shot
    
    // Grouping
    int weaponGroup = 1;                // 1-6, for firing controls
    bool fireLinked = false;            // Fire in sequence vs simultaneously
};

/**
 * MissileWeapon: Specialized missile/torpedo launcher
 */
struct MissileWeapon : public Component {
    std::string weaponId;
    std::string hardpointId;
    
    // Missile types
    enum class MissileType {
        Dumbfire,
        Heatseeking,
        RadarGuided,
        BeamRiding,
        Torpedo
    };
    MissileType missileType = MissileType::Heatseeking;
    
    int ammo = 10;
    int maxAmmo = 20;
    int tubesCount = 1;
    double reloadTime = 3.0;
    double reloadProgress = 0.0;
    
    // Missile characteristics
    double missileDamage = 500.0;
    double missileSpeed = 300.0;        // m/s
    double missileAcceleration = 50.0;  // m/s²
    double missileMaxSpeed = 500.0;     // m/s
    double missileTurnRate = 180.0;     // degrees per second
    double missileLifetime = 20.0;      // seconds
    double missileArmingRange = 50.0;   // meters
    double missileBlastRadius = 25.0;   // meters
    
    // Guidance
    double lockOnTime = 3.0;
    double lockProgress = 0.0;
    bool isLocked = false;
    ecs::EntityHandle lockedTarget = ecs::EntityHandle::Null();
    
    double guidanceAccuracy = 0.9;      // 0-1
    bool canRetarget = true;
    
    // Salvo firing
    int salvoSize = 1;
    double salvoDelay = 0.2;            // seconds between missiles
    int salvoProgress = 0;
};

/**
 * ProjectileData: Data for projectile entities
 */
struct ProjectileData : public Component {
    ecs::EntityHandle owner = ecs::EntityHandle::Null();
    WeaponType weaponType = WeaponType::Ballistic;
    DamageType damageType = DamageType::Kinetic;
    
    double damage = 100.0;
    double armorPenetration = 0.5;
    double shieldPenetration = 0.2;
    
    double speed = 1000.0;
    double lifetime = 5.0;
    double elapsed = 0.0;
    
    // Missile-specific
    bool isGuided = false;
    ecs::EntityHandle target = ecs::EntityHandle::Null();
    double turnRate = 0.0;
    double acceleration = 0.0;
    double armingRange = 0.0;
    bool armed = false;
    
    // Explosive
    bool isExplosive = false;
    double blastRadius = 0.0;
    
    // Beam weapon (continuous damage)
    bool isContinuous = false;
    double beamWidth = 1.0;             // meters
};

// ============================================================================
// SHIELD MANAGEMENT
// ============================================================================

/**
 * DirectionalShields: Multi-facing shield system
 */
struct DirectionalShields : public Component {
    struct ShieldFace {
        double currentShields = 100.0;
        double maxShields = 100.0;
        double rechargeRate = 10.0;     // per second
        double rechargeDelay = 3.0;     // seconds after damage
        double lastDamageTime = 0.0;
        bool overloaded = false;
        double overloadRecovery = 0.0;
        double overloadThreshold = 150.0; // Damage that causes overload
    };
    
    std::unordered_map<ShieldFacing, ShieldFace> faces;
    
    double globalRechargeMultiplier = 1.0;
    double powerAllocation = 1.0;       // 0-1, from power management
    double maxPowerDraw = 50.0;         // MW
    double currentPowerDraw = 0.0;      // MW
    
    bool shieldsEnabled = true;
    bool canRebalance = true;           // Can redistribute shield power
    double rebalanceRate = 20.0;        // shields per second transfer
    
    void InitializeFace(ShieldFacing facing, double maxShield) {
        ShieldFace face;
        face.currentShields = maxShield;
        face.maxShields = maxShield;
        faces[facing] = face;
    }
    
    double GetTotalShields() const {
        double total = 0.0;
        for (const auto& pair : faces) {
            total += pair.second.currentShields;
        }
        return total;
    }
    
    double GetTotalMaxShields() const {
        double total = 0.0;
        for (const auto& pair : faces) {
            total += pair.second.maxShields;
        }
        return total;
    }
};

// ============================================================================
// ELECTRONIC WARFARE
// ============================================================================

/**
 * ElectronicWarfare: ECM, ECCM, jamming systems
 */
struct ElectronicWarfare : public Component {
    // Jamming
    bool jammingActive = false;
    double jammingStrength = 0.5;       // 0-1
    double jammingRange = 2000.0;       // meters
    double jammingPowerCost = 10.0;     // MW
    
    // Countermeasures
    int chaff Count = 10;
    int chaffMax = 20;
    int flareCount = 10;
    int flareMax = 20;
    double countermeasureCooldown = 1.0;
    double countermeasureTimer = 0.0;
    
    // Decoys
    int decoyCount = 3;
    int decoyMax = 5;
    double decoyLifetime = 30.0;
    double decoyEffectiveness = 0.7;    // 0-1
    
    // Stealth
    double radarCrossSection = 1.0;     // Multiplier (lower = stealthier)
    double thermalSignature = 1.0;      // Multiplier
    double emissionStrength = 1.0;      // Multiplier
    bool stealthMode = false;
    double stealthPowerCost = 5.0;      // MW
    
    // Sensor disruption
    bool beingJammed = false;
    double jammedAmount = 0.0;          // 0-1, affects targeting/sensors
    double jamResistance = 0.5;         // 0-1, ECCM capability
};

/**
 * SensorSystem: Radar, IR, visual detection
 */
struct SensorSystem : public Component {
    enum class SensorType {
        Radar,
        Infrared,
        Visual,
        GravimetricNetwork
    };
    
    std::unordered_map<SensorType, bool> activeSensors;
    
    double radarRange = 10000.0;        // meters
    double irRange = 5000.0;            // meters
    double visualRange = 2000.0;        // meters
    
    double scanResolution = 100.0;      // meters
    double trackingAccuracy = 0.9;      // 0-1
    
    double powerCost = 5.0;             // MW when active
    bool passiveMode = false;           // Low power, reduced range
    
    // Tracked contacts
    struct Contact {
        ecs::EntityHandle entity = ecs::EntityHandle::Null();
        double lastSeenTime = 0.0;
        double posX = 0.0, posY = 0.0, posZ = 0.0;
        double velX = 0.0, velY = 0.0, velZ = 0.0;
        double confidence = 1.0;        // 0-1, how certain we are
        std::string classification;     // "Fighter", "Cruiser", etc.
    };
    
    std::vector<Contact> contacts;
    double contactUpdateInterval = 0.5; // seconds
    double lastContactUpdate = 0.0;
};

// ============================================================================
// COMBAT STATISTICS & ANALYTICS
// ============================================================================

/**
 * CombatStatistics: Track combat performance
 */
struct CombatStatistics : public Component {
    int kills = 0;
    int assists = 0;
    int deaths = 0;
    
    double totalDamageDealt = 0.0;
    double totalDamageReceived = 0.0;
    int shotsFired = 0;
    int shotsHit = 0;
    int missilesFired = 0;
    int missilesHit = 0;
    
    double timeInCombat = 0.0;
    double totalFlightTime = 0.0;
    
    int subsystemsDestroyed = 0;
    int alliesLost = 0;
    int enemiesDefeated = 0;
    
    double longestKillRange = 0.0;
    double averageKillRange = 0.0;
    
    std::unordered_map<std::string, int> killsByShipType;
    std::unordered_map<WeaponType, int> killsByWeapon;
    
    double GetAccuracy() const {
        return shotsFired > 0 ? (double)shotsHit / shotsFired : 0.0;
    }
    
    double GetKillDeathRatio() const {
        return deaths > 0 ? (double)kills / deaths : kills;
    }
};

/**
 * CombatReputation: Fame, infamy, faction standing
 */
struct CombatReputation : public Component {
    double fame = 0.0;                  // Positive reputation
    double infamy = 0.0;                // Negative reputation/notoriety
    
    std::unordered_map<int, double> factionStanding; // factionId -> standing (-1 to 1)
    
    int combatRank = 0;                 // 0 = Harmless, 10 = Elite
    double rankProgress = 0.0;
    
    std::vector<std::string> achievements;
    std::vector<std::string> titles;
};

// ============================================================================
// DAMAGE CONTROL & REPAIR
// ============================================================================

/**
 * DamageControl: Active damage control and repair
 */
struct DamageControl : public Component {
    struct RepairTask {
        SubsystemType subsystem = SubsystemType::None;
        double progress = 0.0;          // 0-1
        double repairRate = 0.1;        // per second
        int assignedCrew = 0;
    };
    
    std::vector<RepairTask> activeRepairs;
    
    int crewCount = 5;
    int maxCrew = 10;
    int availableCrew = 5;
    
    int repairKits = 10;
    int maxRepairKits = 20;
    
    bool autoRepair = true;
    SubsystemType repairPriority = SubsystemType::Engines;
    
    // Fire suppression
    int extinguishers = 10;
    int maxExtinguishers = 15;
    double fireSuppressionRate = 0.2;   // per second
    
    // Hull breach repair
    int hullPatches = 5;
    int maxHullPatches = 10;
    double breachRepairTime = 10.0;     // seconds
};

/**
 * HullDamage: Physical damage to hull and armor
 */
struct HullDamage : public Component {
    double currentArmor = 500.0;
    double maxArmor = 500.0;
    double armorEffectiveness = 1.0;    // Multiplier for damage reduction
    
    double currentHull = 1000.0;
    double maxHull = 1000.0;
    
    struct DamageLocation {
        double x = 0.0, y = 0.0, z = 0.0;
        double size = 1.0;
        DamageType type = DamageType::Kinetic;
        double timestamp = 0.0;
    };
    
    std::vector<DamageLocation> damageMarks;
    
    int breachCount = 0;
    bool catastrophicDamage = false;
    double structuralIntegrity = 1.0;  // 0-1
};

// ============================================================================
// COMBAT AI
// ============================================================================

/**
 * CombatAI: AI behavior for combat
 */
struct CombatAI : public Component {
    CombatAIDifficulty difficulty = CombatAIDifficulty::Medium;
    
    enum class Behavior {
        Aggressive,
        Defensive,
        Balanced,
        Evasive,
        Support,
        Flee
    };
    Behavior currentBehavior = Behavior::Balanced;
    
    ecs::EntityHandle primaryTarget = ecs::EntityHandle::Null();
    std::vector<ecs::EntityHandle> threatList;
    
    double aggressionLevel = 0.5;       // 0-1
    double selfPreservation = 0.5;      // 0-1
    double teamwork = 0.5;              // 0-1
    
    // Tactical decisions
    double engagementRange = 1500.0;    // Preferred combat range
    double fleeThreshold = 0.2;         // HP % to flee at
    bool useEvasiveManeuvers = true;
    bool useCover = true;
    double coverDistance = 500.0;       // Distance to keep from cover
    
    // Decision timing
    double decisionInterval = 1.0;      // seconds
    double lastDecisionTime = 0.0;
    
    // Formation flying
    bool inFormation = false;
    ecs::EntityHandle formationLeader = ecs::EntityHandle::Null();
    int formationPosition = 0;
    double formationSpacing = 100.0;    // meters
    
    // Target prioritization
    std::function<double(ecs::EntityHandle)> threatEvaluator;
};

/**
 * SquadronMember: Part of fighter squadron or fleet
 */
struct SquadronMember : public Component {
    std::string squadronId;
    int position = 0;                   // Position in formation
    ecs::EntityHandle wingman = ecs::EntityHandle::Null();
    ecs::EntityHandle leader = ecs::EntityHandle::Null();
    
    enum class Role {
        Leader,
        Wingman,
        Support,
        Scout
    };
    Role role = Role::Wingman;
    
    // Squadron commands
    enum class Command {
        None,
        Attack,
        Defend,
        FormUp,
        BreakAndAttack,
        CoverMe,
        Evasive
    };
    Command currentCommand = Command::None;
    ecs::EntityHandle commandTarget = ecs::EntityHandle::Null();
};

// ============================================================================
// BOARDING & CAPTURE
// ============================================================================

/**
 * BoardingParty: Boarding action component
 */
struct BoardingParty : public Component {
    ecs::EntityHandle targetShip = ecs::EntityHandle::Null();
    
    int marines = 10;
    int marinesMax = 20;
    double marineSkill = 0.5;           // 0-1
    
    enum class BoardingPhase {
        Approaching,
        Breaching,
        Fighting,
        Securing,
        Complete,
        Failed
    };
    BoardingPhase phase = BoardingPhase::Approaching;
    
    double progress = 0.0;              // 0-1
    double phaseTimer = 0.0;
    
    SubsystemType targetSubsystem = SubsystemType::None;
    bool sabotage = false;              // Sabotage vs capture
};

// ============================================================================
// WRECK SALVAGE
// ============================================================================

/**
 * WreckData: Salvageable wreck information
 */
struct WreckData : public Component {
    std::string originalShipClass;
    double timeOfDeath = 0.0;
    
    struct SalvageItem {
        std::string componentId;
        std::string componentType;
        double condition = 1.0;         // 0-1
        double value = 100.0;
        bool recovered = false;
    };
    
    std::vector<SalvageItem> salvageableComponents;
    
    double totalValue = 0.0;
    double recoveredValue = 0.0;
    
    bool isSalvaging = false;
    ecs::EntityHandle salvager = ecs::EntityHandle::Null();
    double salvageProgress = 0.0;       // 0-1
    double salvageTime = 30.0;          // seconds
    
    double wreckLifetime = 300.0;       // seconds before despawn
    double wreckAge = 0.0;
};

/**
 * SalvageSystem: Ship equipped with salvage tools
 */
struct SalvageSystem : public Component {
    double salvageSpeed = 1.0;          // Multiplier
    double cargoCapacity = 1000.0;      // tons
    double currentCargo = 0.0;          // tons
    
    bool salvageBeamActive = false;
    ecs::EntityHandle currentWreck = ecs::EntityHandle::Null();
    
    double salvageRange = 100.0;        // meters
    double salvagePowerCost = 5.0;      // MW
};

// ============================================================================
// COUNTERMEASURES & SPECIAL SYSTEMS
// ============================================================================

/**
 * MineLayer: Mine deployment system
 */
struct MineLayer : public Component {
    int mineCount = 5;
    int mineMax = 10;
    
    double mineDamage = 800.0;
    double mineBlastRadius = 50.0;      // meters
    double mineArmingTime = 2.0;        // seconds
    double mineLifetime = 600.0;        // seconds
    double mineDetectionRange = 100.0;  // meters
    
    double deploymentCooldown = 5.0;    // seconds
    double lastDeploymentTime = 0.0;
    
    enum class TriggerMode {
        Proximity,
        Remote,
        Timed
    };
    TriggerMode triggerMode = TriggerMode::Proximity;
};

/**
 * TractorBeam: Tractor beam system for recovery/capture
 */
struct TractorBeam : public Component {
    bool active = false;
    ecs::EntityHandle target = ecs::EntityHandle::Null();
    
    double maxForce = 10000.0;          // Newtons
    double maxRange = 500.0;            // meters
    double powerCost = 20.0;            // MW
    
    double pullSpeed = 10.0;            // m/s
    bool canPushRepel = true;
    
    bool isTowing = false;
    double towingDistance = 50.0;       // meters to maintain
};

#endif // COMBAT_COMPONENTS_H
