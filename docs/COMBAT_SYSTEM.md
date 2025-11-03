# Advanced Combat System Documentation

## Overview

The Nova Engine Advanced Combat System provides a comprehensive, modular framework for space combat simulation featuring:

- **Subsystem Targeting**: Damage individual ship systems (engines, weapons, shields, etc.)
- **Weapon Variety**: Multiple weapon types with unique characteristics
- **Shield Management**: Directional shields with recharge mechanics
- **Electronic Warfare**: Jamming, decoys, and stealth systems
- **Combat AI**: Intelligent opponents with difficulty levels
- **Damage Control**: Fire suppression, breach repair, and crew management
- **Statistics Tracking**: Comprehensive combat analytics

## Architecture

The combat system follows Nova Engine's ECS (Entity Component System) architecture:

- **Components** (`CombatComponents.h`): Data-only structures
- **Systems** (`CombatSystems.h/cpp`): Logic that processes components
- **Utilities** (`CombatUtils`): Helper functions for calculations

## Core Features

### 1. Subsystem Targeting

Target and damage individual ship systems:

```cpp
// Initialize subsystems
auto subsystems = std::make_shared<SubsystemHealth>();
subsystems->InitializeSubsystem(SubsystemType::Engines, 500.0);
subsystems->InitializeSubsystem(SubsystemType::Weapons, 300.0);
subsystems->InitializeSubsystem(SubsystemType::Shields, 400.0);
em.AddComponent<SubsystemHealth>(shipEntity, subsystems);

// Set targeting computer to target specific subsystem
auto targeting = em.GetComponent<TargetingSubsystem>(playerShip);
targeting->targetedSubsystem = SubsystemType::Engines;
```

**Subsystem Types:**
- `Engines`: Affects ship mobility
- `Weapons`: Reduces weapon effectiveness
- `Shields`: Impacts shield functionality
- `Sensors`: Reduces detection range
- `PowerPlant`: Reduces available power
- `LifeSupport`: Affects crew efficiency
- `Communications`: Disrupts coordination
- `Cockpit`: High damage can kill pilot
- `CargoHold`: Can cause cargo loss
- `Hangar`: Affects fighter deployment

**Damage States:**
- `Operational` (100-75%): Full functionality
- `Damaged` (75-40%): Reduced efficiency
- `Critical` (40-10%): Severely impaired
- `Failed` (10-0%): Non-functional
- `Destroyed` (0%): Permanent loss

### 2. Weapon Systems

#### Energy Weapons
```cpp
auto weapon = std::make_shared<WeaponSystem>();
weapon->weaponId = "laser_cannon_mk2";
weapon->type = WeaponType::Laser;
weapon->damageType = DamageType::Energy;
weapon->baseDamage = 150.0;
weapon->fireRate = 2.0;  // shots per second
weapon->projectileSpeed = 2000.0;
weapon->accuracy = 0.95;
weapon->ammo = -1;  // Infinite (energy weapon)
em.AddComponent<WeaponSystem>(shipEntity, weapon);
```

#### Ballistic Weapons
```cpp
weapon->type = WeaponType::Ballistic;
weapon->damageType = DamageType::Kinetic;
weapon->baseDamage = 200.0;
weapon->fireRate = 5.0;
weapon->ammo = 500;
weapon->maxAmmo = 500;
weapon->armorPenetration = 0.7;  // Better vs armor
```

#### Missile Launchers
```cpp
auto missile = std::make_shared<MissileWeapon>();
missile->missileType = MissileWeapon::MissileType::Heatseeking;
missile->ammo = 10;
missile->missileDamage = 500.0;
missile->missileSpeed = 300.0;
missile->lockOnTime = 3.0;
missile->isExplosive = true;
missile->blastRadius = 25.0;
em.AddComponent<MissileWeapon>(shipEntity, missile);
```

**Weapon Types:**
- `Laser`: High speed, energy-based, good vs shields
- `Ballistic`: Physical projectiles, good vs armor
- `Missile`: Guided, high damage, requires lock
- `Torpedo`: Heavy missile, anti-capital ship
- `Mine`: Area denial, proximity detonation
- `PointDefense`: Anti-missile/fighter defense
- `Beam`: Continuous damage channel
- `Railgun`: High-velocity kinetic

**Damage Types:**
- `Kinetic`: Best vs armor, reduced vs shields
- `Energy`: Best vs shields, reduced vs armor
- `Explosive`: Area damage, bypasses some armor
- `Thermal`: Causes heat buildup, fire risk
- `Electromagnetic`: EMP effects, best vs electronics
- `Chemical`: Corrosive, damage over time
- `Exotic`: Experimental, unique effects

### 3. Directional Shields

Shields are divided into six facings:

```cpp
auto shields = std::make_shared<DirectionalShields>();
shields->InitializeFace(ShieldFacing::Forward, 200.0);
shields->InitializeFace(ShieldFacing::Aft, 150.0);
shields->InitializeFace(ShieldFacing::Port, 180.0);
shields->InitializeFace(ShieldFacing::Starboard, 180.0);
shields->InitializeFace(ShieldFacing::Dorsal, 150.0);
shields->InitializeFace(ShieldFacing::Ventral, 150.0);

shields->canRebalance = true;  // Allow power transfer between facings
shields->rebalanceRate = 20.0;
em.AddComponent<DirectionalShields>(shipEntity, shields);
```

**Shield Mechanics:**
- Damage goes to the facing hit (Forward, Aft, Port, Starboard, Dorsal, Ventral)
- Recharge delay after taking damage
- Can transfer power between facings
- Overload mechanic for extreme damage
- Power allocation affects recharge rate

### 4. Electronic Warfare

#### Jamming
```cpp
auto ew = std::make_shared<ElectronicWarfare>();
ew->jammingActive = true;
ew->jammingStrength = 0.7;  // 0-1 scale
ew->jammingRange = 2000.0;  // meters
ew->jamResistance = 0.5;    // ECCM capability
em.AddComponent<ElectronicWarfare>(shipEntity, ew);
```

#### Countermeasures
```cpp
ew->chaffCount = 10;
ew->flareCount = 10;
ew->decoyCount = 3;
ew->countermeasureCooldown = 1.0;
```

#### Stealth
```cpp
ew->stealthMode = true;
ew->radarCrossSection = 0.3;    // 30% of normal
ew->thermalSignature = 0.5;     // 50% of normal
ew->emissionStrength = 0.2;     // 20% of normal
```

### 5. Combat AI

```cpp
auto ai = std::make_shared<CombatAI>();
ai->difficulty = CombatAIDifficulty::Hard;
ai->currentBehavior = CombatAI::Behavior::Aggressive;
ai->aggressionLevel = 0.8;
ai->selfPreservation = 0.4;
ai->engagementRange = 1500.0;
ai->fleeThreshold = 0.2;  // Flee at 20% HP
ai->useEvasiveManeuvers = true;
em.AddComponent<CombatAI>(shipEntity, ai);
```

**AI Difficulty Levels:**
- `Civilian`: Non-combatant, flees immediately
- `Easy`: Basic targeting, poor evasion
- `Medium`: Competent fighter, uses cover
- `Hard`: Skilled veteran, tactical decisions
- `Expert`: Ace pilot, advanced maneuvers
- `Elite`: Squadron leader, coordinates attacks

**AI Behaviors:**
- `Aggressive`: Close range, high damage focus
- `Defensive`: Maintain distance, prioritize survival
- `Balanced`: Mix of offense and defense
- `Evasive`: Focus on dodging, hit and run
- `Support`: Assist allies, provide cover
- `Flee`: Retreat to safety

### 6. Squadron System

```cpp
auto squadron = std::make_shared<SquadronMember>();
squadron->squadronId = "alpha_squadron";
squadron->role = SquadronMember::Role::Leader;
squadron->position = 0;
em.AddComponent<SquadronMember>(fighterEntity, squadron);

// Issue commands
squadronSystem->IssueCommand(em, "alpha_squadron", 
                            SquadronMember::Command::Attack, 
                            enemyCapitalShip);
```

**Squadron Commands:**
- `Attack`: Engage specified target
- `Defend`: Protect specified friendly
- `FormUp`: Return to formation
- `BreakAndAttack`: Individual engagement
- `CoverMe`: Provide fighter cover
- `Evasive`: Defensive maneuvers

### 7. Damage Control

```cpp
auto dc = std::make_shared<DamageControl>();
dc->crewCount = 5;
dc->maxCrew = 10;
dc->repairKits = 10;
dc->extinguishers = 10;
dc->autoRepair = true;
dc->repairPriority = SubsystemType::Engines;
em.AddComponent<DamageControl>(shipEntity, dc);
```

**Damage Control Features:**
- Crew-based repair system
- Fire suppression
- Hull breach sealing
- Automatic or manual repair assignment
- Priority-based repair queue
- Resource management (repair kits, extinguishers, patches)

### 8. Boarding Actions

```cpp
auto boarding = std::make_shared<BoardingParty>();
boarding->targetShip = enemyShipEntity;
boarding->marines = 10;
boarding->marineSkill = 0.7;
boarding->targetSubsystem = SubsystemType::Cockpit;
em.AddComponent<BoardingParty>(attackerShip, boarding);
```

**Boarding Phases:**
1. `Approaching`: Moving to target
2. `Breaching`: Cutting through hull
3. `Fighting`: Combat with defenders
4. `Securing`: Taking control of ship
5. `Complete`: Ship captured
6. `Failed`: Boarding repelled

### 9. Wreck Salvage

```cpp
// Automatically created when ship is destroyed
auto salvage = std::make_shared<SalvageSystem>();
salvage->salvageSpeed = 1.5;
salvage->cargoCapacity = 1000.0;
salvage->salvageRange = 100.0;
em.AddComponent<SalvageSystem>(salvageShip, salvage);
```

**Salvageable Items:**
- Intact weapons
- Functional components
- Cargo contents
- Rare materials
- Ship hull sections

### 10. Combat Statistics

```cpp
auto stats = std::make_shared<CombatStatistics>();
// Automatically tracked:
// - Kills, assists, deaths
// - Damage dealt/received
// - Shot accuracy
// - Kill ranges
// - Weapon effectiveness
em.AddComponent<CombatStatistics>(playerShip, stats);

// Access statistics
double accuracy = stats->GetAccuracy();
double kdr = stats->GetKillDeathRatio();
```

## System Integration

### Main Loop Integration

```cpp
// In your main game loop or system scheduler
void GameLoop::Update(double dt) {
    // Combat systems
    weaponFireSystem.Update(entityManager, dt);
    projectileSystem.Update(entityManager, dt);
    targetingSystem.Update(entityManager, dt);
    shieldSystem.Update(entityManager, dt);
    subsystemDamageSystem.Update(entityManager, dt);
    electronicWarfareSystem.Update(entityManager, dt);
    sensorSystem.Update(entityManager, dt);
    combatAISystem.Update(entityManager, dt);
    squadronSystem.Update(entityManager, dt);
    damageControlSystem.Update(entityManager, dt);
    boardingSystem.Update(entityManager, dt);
    salvageSystem.Update(entityManager, dt);
    statsSystem.Update(entityManager, dt);
}
```

### Complete Ship Setup Example

```cpp
ecs::EntityHandle CreateCombatShip(EntityManager& em) {
    auto ship = em.CreateEntity();
    
    // Core components
    em.AddComponent<Position>(ship, std::make_shared<Position>(0, 0, 0));
    em.AddComponent<Velocity>(ship, std::make_shared<Velocity>(0, 0, 0));
    em.AddComponent<Health>(ship, std::make_shared<Health>(1000, 1000));
    
    // Hull and armor
    auto hull = std::make_shared<HullDamage>();
    hull->currentArmor = 500.0;
    hull->maxArmor = 500.0;
    hull->currentHull = 1000.0;
    hull->maxHull = 1000.0;
    em.AddComponent<HullDamage>(ship, hull);
    
    // Subsystems
    auto subsystems = std::make_shared<SubsystemHealth>();
    subsystems->InitializeSubsystem(SubsystemType::Engines, 400.0);
    subsystems->InitializeSubsystem(SubsystemType::Weapons, 300.0);
    subsystems->InitializeSubsystem(SubsystemType::Shields, 350.0);
    subsystems->InitializeSubsystem(SubsystemType::PowerPlant, 500.0);
    em.AddComponent<SubsystemHealth>(ship, subsystems);
    
    // Shields
    auto shields = std::make_shared<DirectionalShields>();
    for (auto facing : {ShieldFacing::Forward, ShieldFacing::Aft, 
                       ShieldFacing::Port, ShieldFacing::Starboard,
                       ShieldFacing::Dorsal, ShieldFacing::Ventral}) {
        shields->InitializeFace(facing, 200.0);
    }
    shields->canRebalance = true;
    em.AddComponent<DirectionalShields>(ship, shields);
    
    // Weapons
    auto weapon = std::make_shared<WeaponSystem>();
    weapon->weaponId = "main_gun";
    weapon->type = WeaponType::Laser;
    weapon->baseDamage = 150.0;
    weapon->fireRate = 2.0;
    weapon->accuracy = 0.9;
    em.AddComponent<WeaponSystem>(ship, weapon);
    
    // Targeting
    auto targeting = std::make_shared<TargetingSubsystem>();
    targeting->mode = TargetingMode::Assisted;
    targeting->maxRange = 5000.0;
    em.AddComponent<TargetingSubsystem>(ship, targeting);
    
    // Electronic warfare
    auto ew = std::make_shared<ElectronicWarfare>();
    ew->chaffCount = 10;
    ew->flareCount = 10;
    ew->jamResistance = 0.5;
    em.AddComponent<ElectronicWarfare>(ship, ew);
    
    // Damage control
    auto dc = std::make_shared<DamageControl>();
    dc->crewCount = 5;
    dc->autoRepair = true;
    em.AddComponent<DamageControl>(ship, dc);
    
    // Combat AI (if NPC)
    auto ai = std::make_shared<CombatAI>();
    ai->difficulty = CombatAIDifficulty::Medium;
    ai->currentBehavior = CombatAI::Behavior::Balanced;
    em.AddComponent<CombatAI>(ship, ai);
    
    // Statistics
    em.AddComponent<CombatStatistics>(ship, std::make_shared<CombatStatistics>());
    
    return ship;
}
```

## Performance Considerations

### Optimization Tips

1. **Component Pooling**: Reuse projectile entities
2. **Spatial Partitioning**: Use octrees for collision detection
3. **Update Frequency**: Update AI less frequently than physics
4. **LOD System**: Reduce simulation detail for distant entities
5. **Query Caching**: Cache commonly used entity queries

### Recommended Update Rates

- Weapons/Projectiles: Every frame
- Shields: Every frame
- Sensors: 10 Hz (0.1s intervals)
- Combat AI: 2-5 Hz (0.2-0.5s intervals)
- Damage Control: 1 Hz (1s intervals)
- Statistics: 1 Hz (1s intervals)

## Configuration Files

### Weapon Configuration (JSON)

```json
{
  "weaponId": "plasma_cannon_mk3",
  "type": "energy",
  "damageType": "thermal",
  "baseDamage": 180.0,
  "fireRate": 1.5,
  "projectileSpeed": 1500.0,
  "accuracy": 0.92,
  "spread": 0.8,
  "optimalRange": 1200.0,
  "maxRange": 2500.0,
  "energyCost": 15.0,
  "heatPerShot": 8.0,
  "cooldown": 0.66,
  "armorPenetration": 0.4,
  "shieldPenetration": 0.6
}
```

### Ship Combat Configuration

```json
{
  "shipClass": "interceptor",
  "hull": {
    "maxHP": 800.0,
    "maxArmor": 400.0,
    "armorEffectiveness": 0.8
  },
  "shields": {
    "facingStrength": {
      "forward": 250.0,
      "aft": 150.0,
      "sides": 200.0
    },
    "rechargeRate": 15.0,
    "rechargeDelay": 2.5,
    "canRebalance": true
  },
  "subsystems": {
    "engines": 350.0,
    "weapons": 280.0,
    "shields": 320.0,
    "powerPlant": 450.0
  },
  "weapons": [
    {
      "hardpointId": "nose_gun",
      "weaponId": "laser_cannon_mk2"
    }
  ],
  "electronicWarfare": {
    "chaffCount": 8,
    "flareCount": 8,
    "jammingStrength": 0.4,
    "stealthCapable": false
  }
}
```

## Future Enhancements

- [ ] Weapon overheating mechanics
- [ ] Armor types with damage resistance
- [ ] Shield frequency matching
- [ ] Advanced formation AI
- [ ] Capital ship combat balance
- [ ] Fighter wing tactics
- [ ] Damage model refinement
- [ ] Combat replay system
- [ ] Kill cam feature
- [ ] Advanced hit detection
- [ ] Procedural damage visuals
- [ ] Dynamic music system
- [ ] Voice communications

## Troubleshooting

### Common Issues

**Projectiles not spawning:**
- Verify WeaponFireSystem is in update loop
- Check weapon cooldown and ammo
- Ensure ship has Position component

**Shields not recharging:**
- Check recharge delay hasn't elapsed
- Verify shield power allocation > 0
- Ensure shields aren't overloaded

**AI not responding:**
- Verify CombatAISystem is active
- Check AI has valid target
- Ensure decision interval has passed

**Damage not applying:**
- Confirm ProjectileSystem is running
- Verify collision detection range
- Check damage calculation in logs

## API Reference

See `CombatComponents.h` and `CombatSystems.h` for complete API documentation.

## License

Part of Nova Engine - See main LICENSE file.
