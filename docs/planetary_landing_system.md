# Planetary Landing & Exploration System

## Overview

The Planetary Landing & Exploration System provides comprehensive gameplay mechanics for atmospheric entry, surface exploration, resource extraction, and ground-based operations. This system integrates seamlessly with Nova Engine's ECS architecture and existing physics systems.

## Core Features

### 1. Atmospheric Entry & Heat Shields

Ships entering planetary atmospheres experience realistic physics:

- **Atmospheric Drag**: Density-based drag increases with velocity
- **Heat Generation**: Friction heating during high-speed entry
- **Heat Shield Management**: Ablative shields protect against reentry temperatures
- **Altitude-Based Effects**: Atmospheric density decreases exponentially with altitude

**Components Used:**
- `AtmosphereComponent` - Atmospheric properties (density, pressure, composition)
- `HeatShieldComponent` - Heat shield integrity and thermal management
- `GravityWellComponent` - Planetary gravity and altitude tracking

**Example Usage:**
```cpp
// Create ship with heat shield for atmospheric entry
auto ship = em.CreateEntity();
em.AddComponent<HeatShieldComponent>(ship, {
    .integrity = 100.0f,
    .maxHeat = 3000.0f,
    .deployed = true
});
```

### 2. Landing Systems

Safe planetary landing requires proper gear deployment and speed control:

- **Landing Gear**: Deployable gear with ground detection
- **Landing Speed Limits**: Safe touchdown thresholds
- **Ground Clearance**: Automatic stabilization on rough terrain
- **Landing Zone Management**: Designated spaceports and emergency sites

**Components Used:**
- `LandingGearComponent` - Gear state and deployment
- `LandingZoneComponent` - Designated landing areas

**Landing Procedure:**
1. Deploy landing gear at altitude
2. Reduce vertical velocity below safe threshold (default 5 m/s)
3. Align with landing zone beacon
4. Touchdown and gear lock

### 3. EVA Suit & Life Support

Extravehicular activity requires proper equipment:

- **Oxygen Management**: Limited oxygen supply with consumption tracking
- **Suit Integrity**: Environmental damage and leak detection
- **Jetpack Systems**: Limited thrust for mobility
- **Magnetic Boots**: Surface adherence in low gravity
- **Environmental Protection**: Radiation and temperature shielding

**Components Used:**
- `EVASuitComponent` - Complete suit systems and life support

**Oxygen System:**
- Standard capacity: 7200 seconds (2 hours)
- Consumption rate: 1.0 units/second (adjustable based on activity)
- Low oxygen warning: 600 seconds (10 minutes)
- Critical failure: Life support offline at 0 oxygen

**Example:**
```cpp
EVASuitComponent suit;
suit.equipped = true;
suit.oxygenCapacity = 7200.0f;
suit.jetpackFuel = 100.0f;
suit.radiationShielding = 0.5f;
em.AddComponent<EVASuitComponent>(entity, suit);
```

### 4. Surface Vehicles

Ground transportation for exploration and cargo hauling:

**Vehicle Types:**
- **Rover**: 4-passenger cargo transport (25 m/s, 500kg capacity)
- **Bike**: Fast single-pilot scout vehicle
- **Jetpack**: Personal aerial mobility
- **Walker**: All-terrain mechanized platform

**Vehicle Systems:**
- Fuel management and consumption
- Speed and acceleration limits
- Terrain interaction
- Passenger and cargo capacity

**Components Used:**
- `SurfaceVehicleComponent` - Vehicle configuration and state

### 5. Weather Systems

Dynamic planetary weather affects operations:

**Weather Types:**
- Clear, Cloudy, Rain, Storm, Fog, Dust, Snow, Extreme

**Weather Effects:**
- Visibility reduction
- Wind forces on vehicles and ships
- Temperature variations
- Hazardous conditions (lightning, extreme temperatures)

**Components Used:**
- `WeatherComponent` - Current weather state and intensity

**Storm Mechanics:**
- Lightning strikes: Configurable frequency
- High winds: Force application to entities
- Reduced visibility: Affects sensors and targeting
- Temperature extremes: Suit and vehicle stress

### 6. Day/Night Cycles

Realistic planetary rotation with lighting changes:

- Configurable day length (default 24 hours)
- Sun angle calculation
- Ambient light adjustment
- Twilight transitions
- Solar position for power generation

**Components Used:**
- `DayNightCycleComponent` - Time tracking and lighting

### 7. Resource Scanning & Mining

Discover and extract planetary resources:

**Scanning System:**
- Configurable scan range and resolution
- Resource deposit detection
- Hazard identification
- Structure discovery

**Mining Operations:**
- Hand drills, mining lasers, extraction rigs
- Heat management during mining
- Resource depletion tracking
- Efficiency and difficulty modifiers

**Resource Types:**
- Common: Iron, Copper, Titanium ore
- Precious: Gold, Platinum, Iridium
- Rare: Exotic elements, Quantum crystals
- Organic: Biomass, Water ice, Volatiles

**Components Used:**
- `SurfaceScannerComponent` - Scanning equipment
- `ResourceDepositComponent` - Resource locations
- `MiningEquipmentComponent` - Mining tools

**Mining Workflow:**
1. Deploy surface scanner
2. Scan for resource signatures
3. Navigate to deposit location
4. Activate mining equipment
5. Monitor heat buildup
6. Collect extracted materials

### 8. Cave Systems

Underground exploration opportunities:

- Multiple entrance points
- Depth and complexity tracking
- Ambient lighting (minimal in caves)
- Oxygen levels (typically none)
- Resource concentrations
- Potential hostile encounters

**Components Used:**
- `CaveSystemComponent` - Cave network data

### 9. Flora & Fauna

Biological encounters on habitable worlds:

**Entity Types:**
- Flora (plants, trees)
- Fauna (animals, insects)
- Fungal organisms
- Bacterial colonies
- Unknown xenobiology

**Interaction:**
- Hostile vs. passive behavior
- Harvestable biomass
- Scientific scanning
- Danger assessment

**Components Used:**
- `BiologicalEntityComponent` - Organism data

### 10. Surface Base Building

Establish permanent outposts:

**Base Types:**
- Outpost: Basic habitat
- Mining Station: Resource processing
- Research Lab: Scientific operations
- Habitat: Population center
- Spaceport: Ship services
- Military: Defense installation

**Base Systems:**
- Life support and oxygen generation
- Power management
- Population tracking
- Integrity monitoring
- Service availability (refueling, repair, medical, market)

**Components Used:**
- `SurfaceBaseComponent` - Base configuration and state

**Construction:**
```cpp
Nova::SurfaceBaseComponent base;
base.type = Nova::SurfaceBaseComponent::BaseType::MiningStation;
base.name = "Mining Complex 7";
base.population = 100;
base.underConstruction = true;
base.constructionProgress = 0.5f; // 50% complete
```

### 11. Environmental Hazards

Dangerous planetary conditions:

**Hazard Types:**
- Radiation zones
- Extreme heat/cold
- Toxic atmosphere
- Acid rain
- Seismic activity
- Volcanic eruptions
- Lava flows

**Hazard Mechanics:**
- Radius-based damage
- Intensity scaling
- Duration tracking (permanent or temporary)
- Warning systems

**Components Used:**
- `EnvironmentalHazardComponent` - Hazard configuration

## System Integration

### ECS Systems

All planetary systems extend the base `System` class:

1. **PlanetaryLandingSystem** - Atmospheric entry and landing
2. **EVASystem** - EVA suit and life support
3. **SurfaceVehicleSystem** - Ground vehicle physics
4. **WeatherSystem** - Dynamic weather patterns
5. **DayNightCycleSystem** - Planetary rotation and lighting
6. **ResourceScanningSystem** - Resource detection
7. **MiningSystem** - Resource extraction
8. **EnvironmentalHazardSystem** - Hazard damage and tracking

### Actor Entities

Pre-configured actor templates for quick spawning:

- `SurfaceVehicle` - Ground transport
- `LandingZone` - Designated landing sites
- `ResourceDeposit` - Mineable resources
- `SurfaceBase` - Planetary installations

### Physics Integration

Systems integrate with existing Nova Engine physics:

- `VelocityComponent` - Movement and forces
- `PositionComponent` - Spatial tracking
- `HealthComponent` - Damage from hazards
- `RotationComponent` - Orientation

## Usage Examples

### Setting Up a Planetary Environment

```cpp
// Create planet with atmosphere
auto planet = em.CreateEntity();

AtmosphereComponent atmo;
atmo.density = 1.225f;        // Earth-like
atmo.pressure = 101.325f;      // Sea level
atmo.temperature = 288.15f;    // 15°C
atmo.breathable = true;
em.AddComponent<AtmosphereComponent>(planet, atmo);

// Add day/night cycle
DayNightCycleComponent cycle;
cycle.dayLength = 86400.0f;    // 24 hours
cycle.currentTime = 43200.0f;  // Start at noon
em.AddComponent<DayNightCycleComponent>(planet, cycle);

// Add weather system
WeatherComponent weather;
weather.currentWeather = WeatherComponent::WeatherType::Clear;
em.AddComponent<WeatherComponent>(planet, weather);
```

### Spawning a Landing Zone with Resources

```cpp
// Create spaceport
auto spaceport = actorFactory.CreateActor("LandingZone", {100, 0, 200});

// Add nearby resource deposits
auto ironDeposit = actorFactory.CreateActor("ResourceDeposit", {150, 0, 220});
auto titaniumDeposit = actorFactory.CreateActor("ResourceDeposit", {180, 0, 250});

// Configure deposit types
if (auto* deposit = em.GetComponent<ResourceDepositComponent>(ironDeposit)) {
    deposit->type = ResourceDepositComponent::ResourceType::IronOre;
    deposit->quantity = 10000.0f;
}
```

### Equipping a Player for EVA

```cpp
auto player = em.GetEntity("Player");

EVASuitComponent suit;
suit.equipped = true;
suit.helmetSealed = true;
suit.lifeSupportActive = true;
suit.oxygenCapacity = 14400.0f;  // 4 hours for extended ops
suit.jetpackFuel = 100.0f;
em.AddComponent<EVASuitComponent>(player, suit);

// Add mining equipment
MiningEquipmentComponent drill;
drill.type = MiningEquipmentComponent::EquipmentType::HandDrill;
drill.miningRate = 15.0f;
drill.range = 3.0f;
em.AddComponent<MiningEquipmentComponent>(player, drill);
```

### Creating an Environmental Hazard

```cpp
// Radiation zone near ancient ruins
auto hazard = em.CreateEntity();
em.AddComponent<PositionComponent>(hazard, {500, 0, 500});

EnvironmentalHazardComponent radiation;
radiation.type = EnvironmentalHazardComponent::HazardType::Radiation;
radiation.intensity = 0.8f;
radiation.damageRate = 5.0f;   // 5 damage/second
radiation.radius = 50.0f;       // 50 meter danger zone
em.AddComponent<EnvironmentalHazardComponent>(hazard, radiation);
```

## Performance Considerations

### Optimization Strategies

1. **Spatial Partitioning**: Use quadtree/octree for large planetary surfaces
2. **LOD Systems**: Reduce detail for distant surface features
3. **Query Optimization**: Leverage ECS QueryBuilder for efficient updates
4. **Component Pooling**: Reuse components for transient entities
5. **Update Frequency**: Vary system update rates based on priority

### Recommended Update Rates

- PlanetaryLandingSystem: 60 Hz (critical for landing)
- EVASystem: 30 Hz (life support monitoring)
- WeatherSystem: 1 Hz (gradual changes)
- DayNightCycleSystem: 1 Hz (slow transitions)
- ResourceScanningSystem: 10 Hz (active scanning)
- MiningSystem: 30 Hz (resource extraction)

## Future Enhancements

### Planned Features

- **Procedural Terrain Generation**: Heightmaps and surface detail
- **Biome Systems**: Diverse planetary regions
- **Weather Simulation**: Advanced meteorology
- **Tectonic Activity**: Earthquakes and volcanic events
- **Underground Networks**: Complex cave systems
- **Base Expansion**: Modular construction
- **NPC Populations**: Dynamic inhabitants
- **Mission Integration**: Ground-based objectives
- **Multiplayer Sync**: Shared planetary instances

### Integration Points

- **Solar System Generator**: Planetary parameters from generation
- **Faction System**: Base ownership and control
- **Economy System**: Resource trading and pricing
- **Mission System**: Ground missions and objectives
- **Multiplayer**: Synchronized planetary state

## API Reference

### Key Functions

**PlanetaryLandingSystem:**
- `CalculateAtmosphericDrag(atmo, velocity, altitude)` - Drag force calculation
- `CalculateHeatLoad(velocity, density)` - Reentry heating
- `IsLandingSafe(gear, velocity, normal)` - Landing safety check

**EVASystem:**
- `UpdateOxygenConsumption(em, deltaTime)` - O2 tracking
- `UpdateSuitIntegrity(em, deltaTime)` - Environmental damage
- `UpdateJetpack(em, deltaTime)` - Jetpack fuel management

**MiningSystem:**
- `ProcessMining(em, deltaTime)` - Resource extraction
- `DepositResources(em, entity, amount)` - Inventory management

## Configuration

### JSON Configuration Example

```json
{
  "planetary_landing": {
    "atmosphere": {
      "max_safe_entry_speed": 7800.0,
      "heat_shield_required": true,
      "drag_coefficient": 0.5
    },
    "landing": {
      "max_safe_speed": 5.0,
      "gear_deploy_time": 2.0,
      "auto_deploy_altitude": 500.0
    },
    "eva": {
      "default_oxygen_capacity": 7200.0,
      "oxygen_warning_threshold": 600.0,
      "jetpack_thrust": 500.0
    },
    "weather": {
      "enable_dynamic_weather": true,
      "transition_time_min": 600.0,
      "storm_damage_enabled": true
    }
  }
}
```

## Troubleshooting

### Common Issues

**Problem**: Ships taking heat damage during entry
- **Solution**: Deploy heat shield before entering atmosphere
- **Check**: HeatShieldComponent.deployed = true

**Problem**: Oxygen depleting too quickly
- **Solution**: Adjust oxygenConsumptionRate in EVASuitComponent
- **Default**: 1.0 units/second

**Problem**: Mining equipment overheating
- **Solution**: Activate in bursts, allow cooling periods
- **Monitor**: MiningEquipmentComponent.heatGeneration

**Problem**: Weather not changing
- **Solution**: Verify WeatherSystem is registered in MainLoop
- **Check**: System update order

## Credits

Developed for Nova Engine's planetary exploration milestone. Integrates with existing ECS V2, physics systems, and actor framework.

## See Also

- [ECS System Documentation](ecs/README_IMPROVEMENTS.md)
- [Physics System](physics/README.md)
- [Solar System Generation](solar_system_generation.md)
- [Actor Factory System](../engine/ActorFactorySystem.h)
