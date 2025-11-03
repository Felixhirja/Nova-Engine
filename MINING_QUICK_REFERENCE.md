# Mining System - Quick Reference Card

## Quick Start (3 Lines)
```cpp
Nova::MiningSystem miningSystem;
auto vessel = entityFactory->CreateActor<MiningVessel>(VesselClass::SoloMiner);
auto asteroid = entityFactory->CreateActor<Asteroid>(ResourceType::TitaniumOre, 10000.0f);
```

## Core Update Loop
```cpp
miningSystem.UpdateProspectors(&em, deltaTime);
miningSystem.UpdateLaserDrills(&em, deltaTime);
miningSystem.UpdateExtractors(&em, deltaTime);
miningSystem.UpdateRefineries(&em, deltaTime);
miningSystem.UpdateMiningDrones(&em, deltaTime);
miningSystem.UpdateMiningHazards(&em, deltaTime);
miningSystem.UpdateMiningClaims(&em, deltaTime);
miningSystem.UpdateToolDurability(&em, deltaTime);
```

## Vessel Classes
| Class | Crew | Lasers | Extractors | Refineries | Cargo Holds |
|-------|------|--------|------------|------------|-------------|
| SoloMiner | 1 | 1 | 0 | 0 | 1 (1,000 kg) |
| IndustrialMiner | 3 | 2 | 1 | 0 | 2 (2,000 kg) |
| MiningBarge | 8 | 4 | 2 | 1 | 4 (4,000 kg) |
| Mothership | 20 | 6 | 4 | 2 | 8 (8,000 kg) |

## Resource Value (Credits/kg)
| Resource | Value | Resource | Value |
|----------|-------|----------|-------|
| Iron Ore | 10 | Titanium Ore | 50 |
| Copper Ore | 15 | Platinum Ore | 200 |
| Nickel Ore | 18 | Gold Ore | 250 |
| Water Ice | 5 | Rare Earth | 300 |
| Helium-3 | 400 | Exotic Crystals | 1,000 |

## Component Quick Access
```cpp
// Check drill status
auto* drill = em->GetComponent<Nova::LaserDrillComponent>(entity);
if (drill->active && !drill->overheated) { /* mining */ }

// Check cargo
auto* cargo = em->GetComponent<Nova::ResourceCargoComponent>(entity);
float fillPercent = cargo->currentMass / cargo->capacity;

// Check tool condition
auto* tool = em->GetComponent<Nova::ToolDurabilityComponent>(entity);
if (tool->condition < 20.0f) { /* needs repair */ }

// Get mining stats
auto* stats = em->GetComponent<Nova::MiningStatsComponent>(entity);
float profit = stats->totalEarnings - stats->totalExpenses;
```

## Drone States
```cpp
enum class DroneMode {
    Idle,          // Waiting for orders
    Prospecting,   // Searching for deposits
    Mining,        // Actively mining
    Returning,     // Heading back to mothership
    Recharging     // Docked and recharging
};
```

## Start Mining Checklist
1. ✅ Create mining vessel
2. ✅ Scan for asteroids (`prospector->scanning = true`)
3. ✅ Target deposit (`drill->targetEntityID = asteroidID`)
4. ✅ Activate drill (`drill->active = true`)
5. ✅ Monitor heat and power
6. ✅ Transport to station when cargo full
7. ✅ Sell resources at market

## Hazard Types
- **Radiation**: Continuous damage
- **Unstable Asteroid**: Explosion risk
- **Volcanic Activity**: Heat damage
- **Gas Vents**: Chemical damage
- **Micro-Meteoroids**: Physical damage
- **Electrical Storms**: System disruption
- **Gravity Anomalies**: Navigation issues

## Common Patterns

### Auto-Mining Setup
```cpp
// Deploy drones
for (int i = 0; i < 3; i++) {
    auto drone = entityFactory->CreateActor<MiningDrone>(vesselID, ResourceType::IronOre);
    auto* droneComp = em->GetComponent<Nova::MiningDroneComponent>(drone);
    droneComp->mode = Nova::MiningDroneComponent::DroneMode::Prospecting;
}
```

### Refining Resources
```cpp
auto* refinery = em->GetComponent<Nova::RefineryComponent>(stationID);
refinery->inputResource = Nova::ResourceType::IronOre;
refinery->outputResource = Nova::ResourceType::Steel;
refinery->inputAmount = 1000.0f;
refinery->active = true;
```

### Market Trading
```cpp
auto* market = em->GetComponent<Nova::ResourceMarketComponent>(stationID);
float ironPrice = market->buyPrices[Nova::ResourceType::IronOre];
float earnings = myCargo->resources[Nova::ResourceType::IronOre] * ironPrice;
```

## Performance Tips
- Update mining systems once per frame
- Use spatial partitioning for hazard checks (future optimization)
- Batch drone updates
- Cache frequently accessed components
- Lazy-evaluate market prices

## Files Reference
| File | Purpose |
|------|---------|
| `engine/ecs/MiningComponents.h` | All component definitions |
| `engine/ecs/MiningSystems.h` | System update logic |
| `entities/MiningVessel.h` | Mining ship actor |
| `entities/Asteroid.h` | Asteroid actor |
| `entities/MiningDrone.h` | Autonomous drone actor |
| `entities/MiningStation.h` | Trading hub actor |
| `docs/MINING_SYSTEM.md` | Full documentation |
| `examples/mining_example.cpp` | Working example code |

## Troubleshooting

**Drill not mining?**
- Check `drill->active == true`
- Check `drill->power > 0`
- Check `!drill->overheated`
- Check `drill->targetEntityID` is valid
- Check deposit has quantity remaining

**Drone stuck in Idle?**
- Set `drone->mode = Prospecting`
- Check `drone->mothershipID` is valid
- Check `drone->remainingPower > 0`
- Check deposits exist in search radius

**Cargo not filling?**
- Check `cargo->currentMass < cargo->capacity`
- Check drill/extractor is active
- Check target deposit has resources
- Check distance to deposit (within range)

## Default Values
- Laser drill range: 50m
- Prospector range: 500m
- Drone autonomy: 1 hour (3600s)
- Drone cargo: 200kg
- Refinery efficiency: 75-90%
- Tool degradation: 0.01% per second of use
