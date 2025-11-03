# Planetary Exploration - Quick Start Guide

## Overview

This guide helps you quickly integrate planetary landing and exploration features into your Nova Engine project.

## 5-Minute Integration

### 1. Include Headers

```cpp
#include "engine/ecs/PlanetaryComponents.h"
#include "engine/gameplay/PlanetaryLandingSystem.h"
```

### 2. Register Systems

In your `MainLoop` or system manager:

```cpp
systemScheduler.RegisterSystem(std::make_shared<PlanetaryLandingSystem>());
systemScheduler.RegisterSystem(std::make_shared<EVASystem>());
systemScheduler.RegisterSystem(std::make_shared<WeatherSystem>());
systemScheduler.RegisterSystem(std::make_shared<DayNightCycleSystem>());
systemScheduler.RegisterSystem(std::make_shared<ResourceScanningSystem>());
systemScheduler.RegisterSystem(std::make_shared<MiningSystem>());
systemScheduler.RegisterSystem(std::make_shared<EnvironmentalHazardSystem>());
systemScheduler.RegisterSystem(std::make_shared<SurfaceVehicleSystem>());
```

### 3. Create a Planet

```cpp
using namespace Nova;

// Basic habitable planet
auto planet = em.CreateEntity();

AtmosphereComponent atmo;
atmo.density = 1.225f;           // Earth-like
atmo.breathable = true;
em.AddComponent(planet, atmo);

DayNightCycleComponent dayNight;
dayNight.dayLength = 86400.0f;   // 24 hours
em.AddComponent(planet, dayNight);

WeatherComponent weather;
weather.currentWeather = WeatherComponent::WeatherType::Clear;
em.AddComponent(planet, weather);
```

### 4. Add a Landing Zone

```cpp
auto landingZone = actorFactory.CreateActor("LandingZone", {0, 0, 0});
```

### 5. Enable Landing on Ship

```cpp
auto ship = /* your ship entity */;

LandingGearComponent gear;
gear.maxLandingSpeed = 5.0f;
em.AddComponent(ship, gear);

HeatShieldComponent heatShield;
heatShield.deployed = false;     // Deploy when entering atmosphere
em.AddComponent(ship, heatShield);

GravityWellComponent gravity;
gravity.surfaceGravity = 9.81f;
em.AddComponent(ship, gravity);
```

## Common Scenarios

### Atmospheric Entry

```cpp
// Prepare for entry
auto* heatShield = em.GetComponent<HeatShieldComponent>(ship);
heatShield->deployed = true;

// Monitor heat during descent
if (heatShield->currentHeat > heatShield->maxHeat * 0.9f) {
    // Warning: Critical heat levels!
}

// Deploy landing gear at low altitude
auto* gear = em.GetComponent<LandingGearComponent>(ship);
auto* gravity = em.GetComponent<GravityWellComponent>(ship);

if (gravity->altitude < 1000.0f) {
    gear->deployed = true;
}
```

### EVA Operations

```cpp
// Equip EVA suit
auto player = em.GetEntity("Player");

EVASuitComponent suit;
suit.equipped = true;
suit.oxygenCapacity = 7200.0f;  // 2 hours
suit.jetpackFuel = 100.0f;
em.AddComponent(player, suit);

// Monitor oxygen
auto* currentSuit = em.GetComponent<EVASuitComponent>(player);
float timeRemaining = currentSuit->oxygenRemaining; // seconds

if (timeRemaining < 600.0f) {
    // Low oxygen warning
}
```

### Resource Mining

```cpp
// Add scanner
SurfaceScannerComponent scanner;
scanner.scanRange = 500.0f;
em.AddComponent(player, scanner);

// Start scan
scanner.scanning = true;

// When resources found, add mining equipment
MiningEquipmentComponent drill;
drill.type = MiningEquipmentComponent::EquipmentType::HandDrill;
drill.miningRate = 10.0f;
em.AddComponent(player, drill);

// Activate mining when near deposit
drill.active = true;
```

### Surface Vehicle

```cpp
// Spawn a rover
auto rover = actorFactory.CreateActor("SurfaceVehicle", {50, 0, 50});

// Customize
auto* vehicle = em.GetComponent<SurfaceVehicleComponent>(rover);
vehicle->type = SurfaceVehicleComponent::VehicleType::Rover;
vehicle->maxSpeed = 30.0f;
vehicle->cargoCapacity = 2000.0f;

// Enter vehicle (transfer player control)
vehicle->active = true;
```

### Weather Events

```cpp
// Trigger a storm
auto* weather = em.GetComponent<WeatherComponent>(planet);
weather->currentWeather = WeatherComponent::WeatherType::Storm;
weather->intensity = 0.8f;
weather->hazardous = true;
weather->lightningFrequency = 5.0f; // 5 strikes/minute
```

### Surface Base

```cpp
// Build an outpost
auto base = actorFactory.CreateActor("SurfaceBase", {200, 0, 200});

auto* baseComp = em.GetComponent<SurfaceBaseComponent>(base);
baseComp->type = SurfaceBaseComponent::BaseType::MiningStation;
baseComp->hasRefueling = true;
baseComp->hasMarket = true;
```

## Component Reference

### Essential Components

| Component | Purpose | Key Fields |
|-----------|---------|------------|
| `AtmosphereComponent` | Planetary atmosphere | density, breathable, toxicity |
| `HeatShieldComponent` | Reentry protection | integrity, currentHeat, deployed |
| `LandingGearComponent` | Landing system | deployed, onGround, maxLandingSpeed |
| `EVASuitComponent` | Life support | oxygenRemaining, suitIntegrity, jetpackFuel |
| `WeatherComponent` | Weather state | currentWeather, intensity, hazardous |
| `SurfaceVehicleComponent` | Ground transport | type, fuel, maxSpeed, cargoCapacity |
| `ResourceDepositComponent` | Mineable resources | type, quantity, discovered |
| `MiningEquipmentComponent` | Mining tools | active, miningRate, heatGeneration |

## System Responsibilities

| System | Update Frequency | Purpose |
|--------|------------------|---------|
| `PlanetaryLandingSystem` | 60 Hz | Atmospheric physics, landing |
| `EVASystem` | 30 Hz | Life support, suit integrity |
| `WeatherSystem` | 1 Hz | Weather patterns, effects |
| `DayNightCycleSystem` | 1 Hz | Planetary rotation, lighting |
| `ResourceScanningSystem` | 10 Hz | Resource detection |
| `MiningSystem` | 30 Hz | Resource extraction |
| `EnvironmentalHazardSystem` | 30 Hz | Hazard damage |
| `SurfaceVehicleSystem` | 60 Hz | Vehicle physics |

## Configuration Tips

### Performance Tuning

```cpp
// Adjust system priorities in SystemScheduler
scheduler.SetPriority("PlanetaryLandingSystem", 100);  // High priority
scheduler.SetPriority("WeatherSystem", 10);            // Low priority

// Reduce scan range for better performance
scanner.scanRange = 100.0f;  // Instead of 500.0f

// Limit active hazards
constexpr int MAX_ACTIVE_HAZARDS = 50;
```

### Gameplay Balance

```cpp
// Easier difficulty
suit.oxygenCapacity = 14400.0f;      // 4 hours instead of 2
gear.maxLandingSpeed = 10.0f;        // More forgiving
drill.miningRate = 20.0f;            // Faster mining

// Harder difficulty
suit.oxygenCapacity = 3600.0f;       // 1 hour
gear.maxLandingSpeed = 3.0f;         // Precise landing required
atmo.radiationLevel = 0.5f;          // Constant suit damage
```

## Debugging

### Common Issues

**Ship not landing:**
```cpp
// Check components
auto* gear = em.GetComponent<LandingGearComponent>(ship);
assert(gear && gear->deployed);

auto* pos = em.GetComponent<PositionComponent>(ship);
assert(pos->position.y <= gear->groundClearance);
```

**Oxygen not depleting:**
```cpp
auto* suit = em.GetComponent<EVASuitComponent>(player);
assert(suit->equipped && suit->helmetSealed);
```

**Mining not working:**
```cpp
auto* equipment = em.GetComponent<MiningEquipmentComponent>(player);
assert(equipment->active && equipment->power > 0.0f);

// Check proximity to deposit
auto* deposit = em.GetComponent<ResourceDepositComponent>(target);
float distance = glm::distance(playerPos, depositPos);
assert(distance <= equipment->range);
```

## Best Practices

1. **Always deploy heat shields** before atmospheric entry above 1000 m/s
2. **Monitor oxygen levels** during EVA operations
3. **Allow mining equipment to cool** to prevent damage
4. **Check weather conditions** before extended surface operations
5. **Establish bases near landing zones** for safe operations
6. **Scan areas** before committing to landing sites
7. **Keep vehicle fuel reserves** for return trips

## Next Steps

- Read full documentation: `docs/planetary_landing_system.md`
- Check actor templates: `entities/SurfaceVehicle.h`, `entities/SurfaceBase.h`
- Explore example missions in `examples/planetary_mission.cpp` (coming soon)
- Integrate with Solar System Generator for procedural planets

## Example: Complete Landing Sequence

```cpp
void ExecuteLandingSequence(EntityManager& em, Entity ship) {
    auto* velocity = em.GetComponent<VelocityComponent>(ship);
    auto* gravity = em.GetComponent<GravityWellComponent>(ship);
    auto* heatShield = em.GetComponent<HeatShieldComponent>(ship);
    auto* gear = em.GetComponent<LandingGearComponent>(ship);
    
    float altitude = gravity->altitude;
    float speed = glm::length(velocity->velocity);
    
    // Phase 1: High altitude entry (>50km)
    if (altitude > 50000.0f && speed > 1000.0f) {
        heatShield->deployed = true;
        // Angle of attack: nose up 30 degrees for drag
    }
    
    // Phase 2: Mid altitude (10-50km)
    if (altitude < 50000.0f && altitude > 10000.0f) {
        // Monitor heat
        if (heatShield->currentHeat > heatShield->maxHeat * 0.8f) {
            // Increase angle of attack to slow down
        }
    }
    
    // Phase 3: Final approach (<10km)
    if (altitude < 10000.0f) {
        gear->deployed = true;
        
        // Target vertical velocity
        float targetVerticalSpeed = 5.0f;
        if (std::abs(velocity->velocity.y) > targetVerticalSpeed) {
            // Apply thrust to slow descent
        }
    }
    
    // Phase 4: Touchdown (<100m)
    if (altitude < 100.0f && gear->deployed) {
        // Fine control, maintain stable descent
        if (gear->onGround) {
            velocity->velocity = glm::vec3(0);
            gear->locked = true;
            // Landing complete!
        }
    }
}
```

## Support

For questions or issues:
- Check documentation in `docs/planetary_landing_system.md`
- Review component definitions in `engine/ecs/PlanetaryComponents.h`
- Examine system implementations in `engine/gameplay/PlanetaryLandingSystem.cpp`

Happy exploring! 🚀🌍
