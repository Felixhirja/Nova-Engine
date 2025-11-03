# Combat System Integration Guide

## Quick Integration Steps

### 1. Include Headers

Add to your main game files:

```cpp
#include "engine/ecs/CombatComponents.h"
#include "engine/ecs/CombatSystems.h"
```

### 2. Initialize Systems

In your game initialization:

```cpp
class GameEngine {
private:
    // Combat systems
    WeaponFireSystem weaponSystem;
    ProjectileSystem projectileSystem;
    AdvancedTargetingSystem targetingSystem;
    DirectionalShieldSystem shieldSystem;
    SubsystemDamageSystem subsystemSystem;
    ElectronicWarfareSystem ewSystem;
    SensorUpdateSystem sensorSystem;
    CombatAISystem combatAI;
    SquadronSystem squadronSystem;
    DamageControlSystem damageControlSystem;
    BoardingSystem boardingSystem;
    SalvageManagementSystem salvageSystem;
    CombatStatisticsSystem statsSystem;
    MineFieldSystem mineSystem;
    TractorBeamSystem tractorSystem;
};
```

### 3. Update in Game Loop

Add to your main update loop:

```cpp
void GameEngine::Update(double dt) {
    // High-frequency systems (every frame)
    targetingSystem.Update(entityManager, dt);
    weaponSystem.Update(entityManager, dt);
    projectileSystem.Update(entityManager, dt);
    shieldSystem.Update(entityManager, dt);
    
    // Medium-frequency systems (~10 Hz)
    static double sensorAccum = 0.0;
    sensorAccum += dt;
    if (sensorAccum >= 0.1) {
        sensorSystem.Update(entityManager, sensorAccum);
        ewSystem.Update(entityManager, sensorAccum);
        sensorAccum = 0.0;
    }
    
    // Low-frequency systems (~2 Hz)
    static double aiAccum = 0.0;
    aiAccum += dt;
    if (aiAccum >= 0.5) {
        combatAI.Update(entityManager, aiAccum);
        squadronSystem.Update(entityManager, aiAccum);
        aiAccum = 0.0;
    }
    
    // Very low-frequency systems (~1 Hz)
    static double slowAccum = 0.0;
    slowAccum += dt;
    if (slowAccum >= 1.0) {
        subsystemSystem.Update(entityManager, slowAccum);
        damageControlSystem.Update(entityManager, slowAccum);
        boardingSystem.Update(entityManager, slowAccum);
        salvageSystem.Update(entityManager, slowAccum);
        statsSystem.Update(entityManager, slowAccum);
        mineSystem.Update(entityManager, slowAccum);
        tractorSystem.Update(entityManager, slowAccum);
        slowAccum = 0.0;
    }
}
```

### 4. Modify Makefile

Add combat system to your build:

```makefile
# In Makefile, add to SOURCES
COMBAT_SOURCES = engine/ecs/CombatSystems.cpp

# Add to overall sources
SOURCES += $(COMBAT_SOURCES)
```

### 5. Create Combat Ships

Use the helper function pattern:

```cpp
ecs::EntityHandle SpawnCombatShip(EntityManager& em, const std::string& shipClass) {
    auto ship = em.CreateEntity();
    
    // Add all combat components
    em.AddComponent<SubsystemHealth>(ship, std::make_shared<SubsystemHealth>());
    em.AddComponent<DirectionalShields>(ship, std::make_shared<DirectionalShields>());
    em.AddComponent<WeaponSystem>(ship, std::make_shared<WeaponSystem>());
    em.AddComponent<TargetingSubsystem>(ship, std::make_shared<TargetingSubsystem>());
    em.AddComponent<ElectronicWarfare>(ship, std::make_shared<ElectronicWarfare>());
    em.AddComponent<DamageControl>(ship, std::make_shared<DamageControl>());
    em.AddComponent<CombatStatistics>(ship, std::make_shared<CombatStatistics>());
    
    // Configure based on ship class
    // ... (see examples/combat_demo.cpp for full example)
    
    return ship;
}
```

## Minimal Example

Simplest possible combat setup:

```cpp
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/CombatComponents.h"
#include "engine/ecs/CombatSystems.h"

int main() {
    EntityManager em;
    
    // Create two ships
    auto ship1 = em.CreateEntity();
    auto ship2 = em.CreateEntity();
    
    // Add basic components
    em.AddComponent<Position>(ship1, std::make_shared<Position>(0, 0, 0));
    em.AddComponent<Position>(ship2, std::make_shared<Position>(100, 0, 0));
    
    // Add weapon to ship1
    auto weapon = std::make_shared<WeaponSystem>();
    weapon->type = WeaponType::Laser;
    weapon->baseDamage = 100.0;
    weapon->fireRate = 1.0;
    weapon->isFiring = true;
    em.AddComponent<WeaponSystem>(ship1, weapon);
    
    // Add health to ship2
    em.AddComponent<Health>(ship2, std::make_shared<Health>(500, 500));
    
    // Create systems
    WeaponFireSystem weaponSys;
    ProjectileSystem projSys;
    
    // Game loop
    for (int i = 0; i < 100; i++) {
        double dt = 0.016;  // 60 FPS
        weaponSys.Update(em, dt);
        projSys.Update(em, dt);
    }
    
    return 0;
}
```

## Integration with Existing Systems

### Player Ship

```cpp
// In your Player actor or component setup
void Player::Initialize() {
    // ... existing player setup ...
    
    // Add combat capabilities
    auto weapon = std::make_shared<WeaponSystem>();
    weapon->weaponId = "player_gun";
    weapon->type = WeaponType::Laser;
    weapon->baseDamage = 150.0;
    context_.GetEntityManager()->AddComponent<WeaponSystem>(
        context_.GetEntity(), weapon);
    
    auto targeting = std::make_shared<TargetingSubsystem>();
    targeting->mode = TargetingMode::Assisted;
    context_.GetEntityManager()->AddComponent<TargetingSubsystem>(
        context_.GetEntity(), targeting);
}

// In player input handling
void Player::HandleInput(Input& input) {
    // ... existing input ...
    
    // Fire weapon
    if (input.IsKeyPressed(SDLK_SPACE)) {
        auto weapon = context_.GetEntityManager()->GetComponent<WeaponSystem>(
            context_.GetEntity());
        if (weapon) {
            weapon->isFiring = true;
        }
    }
    
    // Cycle targets
    if (input.IsKeyPressed(SDLK_t)) {
        auto targeting = context_.GetEntityManager()->GetComponent<TargetingSubsystem>(
            context_.GetEntity());
        if (targeting) {
            // Find next target logic
        }
    }
}
```

### Enemy Ships

```cpp
// In your NPC/Enemy actor
void EnemyShip::Initialize() {
    // ... existing setup ...
    
    // Add AI
    auto ai = std::make_shared<CombatAI>();
    ai->difficulty = CombatAIDifficulty::Medium;
    ai->currentBehavior = CombatAI::Behavior::Aggressive;
    context_.GetEntityManager()->AddComponent<CombatAI>(
        context_.GetEntity(), ai);
    
    // Add weapon
    auto weapon = std::make_shared<WeaponSystem>();
    weapon->type = WeaponType::Ballistic;
    weapon->baseDamage = 100.0;
    context_.GetEntityManager()->AddComponent<WeaponSystem>(
        context_.GetEntity(), weapon);
}
```

### UI Integration

```cpp
// Display combat HUD
void CombatHUD::Render() {
    auto player = GetPlayerEntity();
    
    // Shield status
    auto shields = em.GetComponent<DirectionalShields>(player);
    if (shields) {
        DrawShieldBar("Forward", shields->faces[ShieldFacing::Forward]);
        DrawShieldBar("Aft", shields->faces[ShieldFacing::Aft]);
        // ... other facings
    }
    
    // Weapon status
    auto weapon = em.GetComponent<WeaponSystem>(player);
    if (weapon) {
        DrawWeaponInfo(weapon->weaponId, weapon->ammo, weapon->currentCooldown);
    }
    
    // Target info
    auto targeting = em.GetComponent<TargetingSubsystem>(player);
    if (targeting && targeting->currentTarget.IsValid()) {
        auto targetHealth = em.GetComponent<Health>(targeting->currentTarget);
        if (targetHealth) {
            DrawTargetReticle(targeting->leadX, targeting->leadY, targeting->leadZ);
            DrawTargetHealthBar(targetHealth->current, targetHealth->maximum);
        }
    }
    
    // Combat stats
    auto stats = em.GetComponent<CombatStatistics>(player);
    if (stats) {
        DrawCombatStats(stats->kills, stats->GetAccuracy());
    }
}
```

## Configuration Files

### Weapon Configuration (JSON)

Create `assets/weapons/laser_mk1.json`:

```json
{
  "weaponId": "laser_mk1",
  "displayName": "Laser Cannon Mk1",
  "type": "laser",
  "damageType": "energy",
  "baseDamage": 120.0,
  "fireRate": 2.0,
  "projectileSpeed": 2000.0,
  "projectileLifetime": 3.0,
  "accuracy": 0.90,
  "spread": 1.0,
  "optimalRange": 1000.0,
  "maxRange": 2000.0,
  "energyCost": 10.0,
  "heatPerShot": 5.0,
  "cooldown": 0.5,
  "armorPenetration": 0.3,
  "shieldPenetration": 0.6,
  "effects": {
    "muzzleFlash": "laser_flash",
    "impactEffect": "laser_impact",
    "trailEffect": "laser_trail"
  }
}
```

### Ship Combat Configuration

Create `assets/ships/fighter_mk1_combat.json`:

```json
{
  "shipClass": "fighter_mk1",
  "combat": {
    "hull": {
      "maxHP": 600.0,
      "maxArmor": 350.0,
      "armorEffectiveness": 0.75
    },
    "shields": {
      "type": "directional",
      "facings": {
        "forward": 200.0,
        "aft": 120.0,
        "port": 150.0,
        "starboard": 150.0,
        "dorsal": 120.0,
        "ventral": 120.0
      },
      "rechargeRate": 12.0,
      "rechargeDelay": 2.5,
      "canRebalance": true,
      "rebalanceRate": 18.0
    },
    "subsystems": {
      "engines": 300.0,
      "weapons": 250.0,
      "shields": 280.0,
      "sensors": 200.0,
      "powerPlant": 400.0
    },
    "hardpoints": [
      {
        "id": "nose_gun",
        "size": "small",
        "type": "fixed",
        "defaultWeapon": "laser_mk1"
      },
      {
        "id": "wing_left",
        "size": "small",
        "type": "gimbal",
        "defaultWeapon": "ballistic_cannon"
      }
    ],
    "electronicWarfare": {
      "chaffCount": 8,
      "flareCount": 8,
      "jammingStrength": 0.4,
      "jamResistance": 0.5,
      "radarCrossSection": 0.8
    },
    "damageControl": {
      "crewCount": 3,
      "repairKits": 6,
      "extinguishers": 5
    }
  }
}
```

## Testing Your Integration

### Basic Test

```cpp
void TestCombatSystem() {
    EntityManager em;
    
    // Create test ship
    auto ship = em.CreateEntity();
    em.AddComponent<Position>(ship, std::make_shared<Position>(0, 0, 0));
    
    auto weapon = std::make_shared<WeaponSystem>();
    weapon->type = WeaponType::Laser;
    weapon->baseDamage = 100.0;
    weapon->isFiring = true;
    em.AddComponent<WeaponSystem>(ship, weapon);
    
    // Run systems
    WeaponFireSystem ws;
    ws.Update(em, 0.016);
    
    // Verify projectile was created
    int projectileCount = 0;
    em.ForEach<ProjectileData>([&](auto e, auto& p) { projectileCount++; });
    
    assert(projectileCount > 0 && "Projectile should be created");
    std::cout << "✓ Combat system test passed!" << std::endl;
}
```

## Performance Tips

1. **Batch Updates**: Update similar systems together
2. **Variable Tick Rates**: Update AI less frequently than physics
3. **Spatial Queries**: Use octrees for large numbers of entities
4. **Component Caching**: Cache frequently accessed components
5. **Early Exits**: Check simple conditions before expensive operations

## Common Pitfalls

1. **Missing Position Component**: All combat entities need Position
2. **Null Target Checks**: Always verify target validity before use
3. **Cooldown Reset**: Remember to reset weapon cooldowns after firing
4. **Shield Facing**: Ensure shield facings are initialized
5. **Projectile Cleanup**: Expired projectiles must be destroyed

## Next Steps

1. ✅ Integrate combat systems into main loop
2. ✅ Add player weapon controls
3. ✅ Create enemy AI ships
4. ✅ Implement combat HUD
5. ✅ Add weapon and ship configuration files
6. ✅ Test with multiple simultaneous combats
7. ✅ Tune balance parameters
8. ✅ Add visual/audio effects

## Support

- Full documentation: `docs/COMBAT_SYSTEM.md`
- Component reference: `engine/ecs/CombatComponents.h`
- System reference: `engine/ecs/CombatSystems.h`
- Complete example: `examples/combat_demo.cpp`
- Quick reference: `engine/ecs/COMBAT_README.md`

---

**Ready to fight!** 🚀💥
