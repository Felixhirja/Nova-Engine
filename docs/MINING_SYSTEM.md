# Mining & Resource Extraction System

A comprehensive mining and resource extraction system for Nova Engine featuring asteroid mining, laser drills, extractors, refineries, resource prospecting, mining vessels, automated drones, environmental hazards, and a complete resource economy.

## Features

### ✅ Asteroid Mining
- **Resource Deposits**: Multiple resource types from common ores to exotic materials
- **Deposit Properties**: Density, hardness, mining difficulty, instability
- **Dynamic Asteroids**: Rotating, drifting, potentially unstable asteroids
- **Depletion**: Visual feedback as resources are extracted
- **Discovery System**: Asteroids must be discovered and surveyed

### ✅ Mining Tools & Equipment

#### Laser Drills
- High-powered mining tool with thermal management
- Power consumption and efficiency mechanics
- Heat generation and cooling systems
- Overheat protection
- Range-based targeting
- Upgradeable (5 tech levels)
- Auto-targeting and cooling boost upgrades

#### Mechanical Extractors
- Alternative mining method (slower but efficient)
- Tool durability and wear mechanics
- Multiple types: BasicDrill, ImpactHammer, ChemicalDissolver, NaniteSwarm
- Close-range operation
- Different power consumption profiles

### ✅ Resource Prospecting
- Survey scanners with configurable range and resolution
- Deposit detection and value estimation
- Composition analysis
- Accuracy improvements through upgrades
- Energy-based scanning system
- Rare element detection capability

### ✅ Refining System
- Multiple refinery types: BasicSmelter, ChemicalProcessor, AdvancedRefinery, MolecularFabricator
- Input/output storage management
- Processing rates and efficiency
- Recipe system for material conversion
- Progress tracking
- Power consumption

### ✅ Mining Vessels
Four vessel classes with specialized capabilities:

1. **Solo Miner**: Single-pilot operation, 1 laser drill, basic cargo
2. **Industrial Miner**: Multi-crew, 2 laser drills, 1 extractor, larger cargo
3. **Mining Barge**: Large vessel, 4 laser drills, 2 extractors, 1 refinery, 4 cargo holds
4. **Mothership**: Massive operation base, 6 laser drills, 4 extractors, 2 refineries, 8 cargo holds

Features:
- Crew capacity management
- Equipment slots for tools
- Fuel consumption tracking
- Maintenance costs
- Mining certification/licensing
- Visual feedback based on cargo status

### ✅ Automated Mining Drones
- Autonomous mining operations
- Multiple modes: Idle, Prospecting, Mining, Returning, Recharging
- Power/autonomy management (default 1 hour operation)
- Cargo capacity and auto-return when full
- Target resource selection
- Risk tolerance and hazard avoidance
- AI-driven deposit search
- Automatic mothership docking

### ✅ Resource Types

#### Common Ores
- Iron Ore
- Copper Ore
- Nickel Ore
- Silicate Rock
- Carbon Compounds

#### Rare Elements
- Titanium Ore
- Platinum Ore
- Gold Ore
- Rare Earth Elements
- Uranium

#### Exotic Materials
- Anti-Matter
- Exotic Crystals
- Alien Artifacts
- Quantum Matter
- Dark Matter Residue

#### Volatiles
- Water Ice
- Methane
- Ammonia
- Helium-3
- Hydrogen

#### Refined Materials
- Steel
- Electronics
- Advanced Alloys
- Fusion Fuel
- Nanomaterials

### ✅ Mining Claims System
- Claim registration and ownership
- Radius-based territorial claims
- Time-limited claims (renewable)
- Contested claim mechanics
- Claim value estimation
- Authority registration

### ✅ Environmental Hazards
Seven hazard types affecting mining operations:
1. **Radiation**: Continuous damage to ships and equipment
2. **Unstable Asteroids**: Risk of fragmentation/explosion
3. **Volcanic Activity**: Heat damage
4. **Gas Vents**: Chemical damage
5. **Micro-Meteoroids**: Physical damage
6. **Electrical Storms**: System disruption
7. **Gravity Anomalies**: Navigation hazards

Features:
- Intensity scaling (0-1)
- Damage rates
- Area of effect (radius)
- Intermittent hazards (cyclic on/off)
- Warning range for detection
- Dynamic behavior over time

### ✅ Resource Economy
- **Market System**: Buy/sell prices for all resource types
- **Dynamic Pricing**: Supply/demand mechanics
- **Market Volatility**: Price fluctuations
- **Reputation Bonuses**: Better prices for established miners
- **Black Market**: Optional illegal trading
- **Station Trading**: Mining stations as trading hubs

### ✅ Tool Maintenance
- Durability tracking (0-100%)
- Degradation rate based on use
- Repair cost calculation
- Performance penalties at low durability
- Maintenance intervals and schedules
- Tool breakdown mechanics

### ✅ Mining Statistics
Comprehensive tracking system:
- Total mined mass (lifetime and session)
- Deposits discovered and exhausted
- Resources mined by type
- Total earnings and expenses
- Active mining time tracking
- Personal records (largest haul, highest value resource, most mined resource)

## Component Architecture

### Core Components

```cpp
// Enhanced resource deposits with detailed properties
Nova::EnhancedResourceDepositComponent

// Laser drilling system
Nova::LaserDrillComponent

// Mechanical extraction
Nova::ExtractorComponent

// Survey and prospecting
Nova::ProspectorComponent

// Refining operations
Nova::RefineryComponent

// Cargo storage
Nova::ResourceCargoComponent

// Vessel designation
Nova::MiningVesselComponent

// Autonomous drones
Nova::MiningDroneComponent

// Mining hazards
Nova::MiningHazardComponent

// Market trading
Nova::ResourceMarketComponent

// Statistics tracking
Nova::MiningStatsComponent

// Tool maintenance
Nova::ToolDurabilityComponent

// Territorial claims
Nova::MiningClaimComponent
```

## System Architecture

The `Nova::MiningSystem` class manages all mining operations:

```cpp
Nova::MiningSystem miningSystem;

// Update all mining operations
miningSystem.UpdateLaserDrills(entityManager, deltaTime);
miningSystem.UpdateExtractors(entityManager, deltaTime);
miningSystem.UpdateProspectors(entityManager, deltaTime);
miningSystem.UpdateRefineries(entityManager, deltaTime);
miningSystem.UpdateMiningDrones(entityManager, deltaTime);
miningSystem.UpdateMiningHazards(entityManager, deltaTime);
miningSystem.UpdateMiningClaims(entityManager, deltaTime);
miningSystem.UpdateToolDurability(entityManager, deltaTime);
```

## Entity Actors

### Pre-built Mining Entities

1. **MiningVessel**: Fully-equipped mining ship
   ```cpp
   MiningVessel(VesselClass::IndustrialMiner);
   ```

2. **Asteroid**: Minable resource deposit
   ```cpp
   Asteroid(ResourceType::TitaniumOre, 10000.0f, hasHazard: true);
   ```

3. **MiningDrone**: Automated mining unit
   ```cpp
   MiningDrone(mothershipID, ResourceType::IronOre);
   ```

4. **MiningStation**: Resource trading and refining hub
   ```cpp
   MiningStation("Prosperity Station");
   ```

## Usage Examples

### Creating a Mining Operation

```cpp
// Create mining station
auto stationEntity = entityFactory->CreateActor<MiningStation>("Mining Hub Alpha");

// Create mining vessel
auto vesselEntity = entityFactory->CreateActor<MiningVessel>(
    Nova::MiningVesselComponent::VesselClass::MiningBarge
);

// Spawn asteroids in belt
for (int i = 0; i < 50; i++) {
    float angle = (i / 50.0f) * 2.0f * M_PI;
    float radius = 5000.0f + (rand() % 2000);
    
    auto asteroid = entityFactory->CreateActor<Asteroid>(
        Nova::ResourceType::IronOre,
        5000.0f + (rand() % 10000),
        (rand() % 100) < 20  // 20% chance of hazard
    );
    
    // Position in belt
    auto* pos = em->GetComponent<Position>(asteroid);
    pos->x = radius * cos(angle);
    pos->z = radius * sin(angle);
}

// Deploy mining drones
for (int i = 0; i < 5; i++) {
    auto drone = entityFactory->CreateActor<MiningDrone>(
        vesselEntity,
        Nova::ResourceType::TitaniumOre
    );
}
```

### Starting a Mining Operation

```cpp
// Activate prospector
auto* prospector = em->GetComponent<Nova::ProspectorComponent>(vesselEntity);
prospector->scanning = true;

// Wait for scan results, then start mining
auto* drill = em->GetComponent<Nova::LaserDrillComponent>(vesselEntity);
drill->targetEntityID = nearestAsteroidID;
drill->active = true;

// Deploy drones
auto drones = em->GetEntitiesWithComponent<Nova::MiningDroneComponent>();
for (auto droneEntity : drones) {
    auto* drone = em->GetComponent<Nova::MiningDroneComponent>(droneEntity);
    drone->mode = Nova::MiningDroneComponent::DroneMode::Prospecting;
}
```

### Refining Resources

```cpp
auto* refinery = em->GetComponent<Nova::RefineryComponent>(stationEntity);
refinery->inputResource = Nova::ResourceType::IronOre;
refinery->outputResource = Nova::ResourceType::Steel;
refinery->inputAmount = 1000.0f;
refinery->active = true;
```

### Trading at Station

```cpp
auto* market = em->GetComponent<Nova::ResourceMarketComponent>(stationEntity);

// Sell mined ore
float ironPrice = market->buyPrices[Nova::ResourceType::IronOre];
float totalValue = minerCargo->resources[Nova::ResourceType::IronOre] * ironPrice;

// Buy refined materials
float steelPrice = market->sellPrices[Nova::ResourceType::Steel];
```

## Gameplay Integration

### Player Mining Loop
1. Scan asteroid belt with prospector
2. Analyze deposits for valuable resources
3. Claim valuable territory
4. Deploy mining equipment
5. Extract resources avoiding hazards
6. Manage tool durability and power
7. Refine raw materials (optional)
8. Transport to station
9. Trade for credits
10. Upgrade equipment and vessels

### Automated Mining
1. Deploy mothership to asteroid belt
2. Configure drone parameters
3. Launch autonomous mining drones
4. Drones prospect, mine, and return automatically
5. Monitor operations and hazards
6. Collect refined resources
7. Maintain drone fleet

### Economic Gameplay
- Market price fluctuations based on supply/demand
- Claim speculation and territory control
- Refining for value-added products
- Risk vs. reward in hazardous zones
- Competition between mining operations
- Contract fulfillment for stations

## Performance Considerations

The mining system is designed for efficiency:
- Component-based architecture for cache-friendly iteration
- Spatial partitioning for hazard checks (can be optimized further)
- Minimal per-frame allocations
- Batch processing of drones and refineries
- Lazy evaluation of market prices

## Future Enhancements

Potential additions:
- [ ] Mining guilds and reputation
- [ ] Piracy and claim defense
- [ ] Advanced ore processing chains
- [ ] Asteroid towing and repositioning
- [ ] Deep space mining expeditions
- [ ] Alien artifact discovery
- [ ] Environmental storytelling through deposits
- [ ] Multiplayer claim conflicts
- [ ] Mining contracts and missions
- [ ] Dynamic asteroid belt generation

## Configuration

Resource values, mining rates, and other parameters can be tuned in `MiningComponents.h` and `MiningSystems.h` for game balance.

## See Also

- `engine/ecs/MiningComponents.h` - All mining component definitions
- `engine/ecs/MiningSystems.h` - Mining system logic
- `entities/MiningVessel.h` - Mining ship actor
- `entities/Asteroid.h` - Asteroid actor
- `entities/MiningDrone.h` - Drone actor
- `entities/MiningStation.h` - Station actor
- `engine/ecs/PlanetaryComponents.h` - Related planetary/surface components
