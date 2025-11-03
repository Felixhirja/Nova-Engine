# Mining System Integration Guide

Quick guide to integrate the mining system into your Nova Engine game.

## Step 1: Include Headers

In your game initialization code:

```cpp
#include "engine/ecs/MiningComponents.h"
#include "engine/ecs/MiningSystems.h"

// Optional: Include pre-built actors
#include "entities/MiningVessel.h"
#include "entities/Asteroid.h"
#include "entities/MiningDrone.h"
#include "entities/MiningStation.h"
```

## Step 2: Create Mining System

Add to your game state or main loop class:

```cpp
class YourGame {
private:
    EntityManager entityManager;
    Nova::MiningSystem miningSystem;  // Add this
    
public:
    // ... your code
};
```

## Step 3: Update in Game Loop

Add to your main update loop:

```cpp
void YourGame::Update(double deltaTime) {
    // ... existing updates
    
    // Add mining system updates
    miningSystem.UpdateProspectors(&entityManager, deltaTime);
    miningSystem.UpdateLaserDrills(&entityManager, deltaTime);
    miningSystem.UpdateExtractors(&entityManager, deltaTime);
    miningSystem.UpdateRefineries(&entityManager, deltaTime);
    miningSystem.UpdateMiningDrones(&entityManager, deltaTime);
    miningSystem.UpdateMiningHazards(&entityManager, deltaTime);
    miningSystem.UpdateMiningClaims(&entityManager, deltaTime);
    miningSystem.UpdateToolDurability(&entityManager, deltaTime);
    
    // ... rest of your updates
}
```

## Step 4: Create Mining Content

### Option A: Using Pre-built Actors (Recommended)

```cpp
void YourGame::SetupMiningArea() {
    // Create mining station
    auto station = actorFactory->CreateActor<MiningStation>("Prosperity Hub");
    
    // Position station
    auto* pos = entityManager.GetComponent<Position>(station);
    pos->x = 10000;
    pos->y = 0;
    pos->z = 0;
    
    // Create asteroid field
    for (int i = 0; i < 30; i++) {
        float angle = (i / 30.0f) * 2.0f * M_PI;
        float radius = 5000.0f;
        
        auto asteroid = actorFactory->CreateActor<Asteroid>(
            Nova::ResourceType::IronOre,
            5000.0f + (rand() % 10000),
            (rand() % 100) < 15  // 15% chance of hazard
        );
        
        auto* asteroidPos = entityManager.GetComponent<Position>(asteroid);
        asteroidPos->x = radius * cos(angle);
        asteroidPos->z = radius * sin(angle);
    }
    
    // Create player mining vessel
    playerShip = actorFactory->CreateActor<MiningVessel>(
        Nova::MiningVesselComponent::VesselClass::IndustrialMiner
    );
}
```

### Option B: Manual Component Setup

```cpp
Entity YourGame::CreateCustomMiningShip() {
    Entity ship = entityManager.CreateEntity();
    
    // Basic components
    entityManager.AddComponent<Position>(ship, {0, 0, 0});
    entityManager.AddComponent<Velocity>(ship, {0, 0, 0});
    
    // Mining equipment
    Nova::LaserDrillComponent drill;
    drill.power = 100.0f;
    drill.miningRate = 15.0f;
    entityManager.AddComponent<Nova::LaserDrillComponent>(ship, drill);
    
    Nova::ResourceCargoComponent cargo;
    cargo.capacity = 2000.0f;
    entityManager.AddComponent<Nova::ResourceCargoComponent>(ship, cargo);
    
    Nova::ProspectorComponent prospector;
    prospector.scanRange = 600.0f;
    entityManager.AddComponent<Nova::ProspectorComponent>(ship, prospector);
    
    // Stats tracking
    entityManager.AddComponent<Nova::MiningStatsComponent>(ship, {});
    
    return ship;
}
```

## Step 5: Player Interactions

### Activate Prospector
```cpp
void YourGame::OnPlayerPressScanButton() {
    auto* prospector = entityManager.GetComponent<Nova::ProspectorComponent>(playerShip);
    if (prospector && !prospector->scanning) {
        prospector->scanning = true;
        ShowMessage("Scanning for resources...");
    }
}
```

### Start Mining
```cpp
void YourGame::OnPlayerTargetAsteroid(Entity asteroidID) {
    auto* drill = entityManager.GetComponent<Nova::LaserDrillComponent>(playerShip);
    if (drill && !drill->active) {
        drill->targetEntityID = asteroidID;
        drill->active = true;
        ShowMessage("Mining started");
    }
}
```

### Check Cargo
```cpp
void YourGame::DisplayCargoStatus() {
    auto* cargo = entityManager.GetComponent<Nova::ResourceCargoComponent>(playerShip);
    if (cargo) {
        float fillPercent = (cargo->currentMass / cargo->capacity) * 100.0f;
        ShowUI(std::format("Cargo: {:.0f}%", fillPercent));
        
        // List resources
        for (const auto& [type, amount] : cargo->resources) {
            ShowUI(std::format("  {}: {:.1f} kg", 
                GetResourceName(type), amount));
        }
    }
}
```

### Sell at Station
```cpp
void YourGame::SellResources(Entity stationID) {
    auto* cargo = entityManager.GetComponent<Nova::ResourceCargoComponent>(playerShip);
    auto* market = entityManager.GetComponent<Nova::ResourceMarketComponent>(stationID);
    
    if (cargo && market) {
        float totalEarnings = 0.0f;
        
        for (auto& [type, amount] : cargo->resources) {
            if (amount > 0.0f) {
                float price = market->buyPrices[type];
                float value = amount * price;
                totalEarnings += value;
                
                // Transfer to station
                market->supply[type] += amount;
                amount = 0.0f;
            }
        }
        
        cargo->currentMass = 0.0f;
        cargo->resources.clear();
        
        playerCredits += totalEarnings;
        ShowMessage(std::format("Sold cargo for {:.0f} credits", totalEarnings));
    }
}
```

## Step 6: UI Integration

### Mining HUD
```cpp
void YourGame::RenderMiningHUD() {
    auto* drill = entityManager.GetComponent<Nova::LaserDrillComponent>(playerShip);
    auto* cargo = entityManager.GetComponent<Nova::ResourceCargoComponent>(playerShip);
    auto* durability = entityManager.GetComponent<Nova::ToolDurabilityComponent>(playerShip);
    
    if (drill) {
        DrawBar("Drill Power", drill->power / drill->maxPower);
        DrawBar("Drill Heat", drill->currentHeat / drill->maxHeat);
        DrawStatus("Status", drill->active ? "MINING" : "Idle");
        
        if (drill->overheated) {
            DrawWarning("OVERHEATED - Cooling down");
        }
    }
    
    if (cargo) {
        DrawBar("Cargo", cargo->currentMass / cargo->capacity);
    }
    
    if (durability && durability->condition < 30.0f) {
        DrawWarning("Tool condition low - repair needed");
    }
}
```

### Scanner Display
```cpp
void YourGame::ShowScanResults() {
    auto* prospector = entityManager.GetComponent<Nova::ProspectorComponent>(playerShip);
    
    if (prospector && prospector->scanProgress >= 1.0f) {
        ShowMessage(std::format("Scan complete: {} deposits found", 
            prospector->detectedDeposits.size()));
        
        // Mark deposits on map
        for (Entity depositID : prospector->detectedDeposits) {
            float value = prospector->depositValues[depositID];
            AddMapMarker(depositID, "Resource Deposit", value);
        }
    }
}
```

## Step 7: Automated Mining (Optional)

Deploy drones for passive income:

```cpp
void YourGame::DeployMiningDrones() {
    // Create drone fleet
    for (int i = 0; i < 5; i++) {
        auto drone = actorFactory->CreateActor<MiningDrone>(
            playerShip,  // Mothership ID
            Nova::ResourceType::IronOre  // Target resource
        );
        
        // Activate drone
        auto* droneComp = entityManager.GetComponent<Nova::MiningDroneComponent>(drone);
        droneComp->mode = Nova::MiningDroneComponent::DroneMode::Prospecting;
    }
    
    ShowMessage("Deployed 5 mining drones");
}
```

## Performance Notes

- Mining system updates are lightweight (component iteration)
- Hazard checks can be optimized with spatial partitioning
- Consider updating drones every N frames for large fleets
- Market price updates can be done less frequently (every few seconds)

## Testing

Compile and run the example:
```bash
g++ -std=c++17 examples/mining_example.cpp -I./engine -o mining_test
./mining_test
```

## Troubleshooting

**Problem:** Drill not mining
- **Solution:** Check drill->active, drill->power > 0, !drill->overheated, targetEntityID valid

**Problem:** Drones staying idle
- **Solution:** Set mode to Prospecting, ensure mothershipID is valid, check power > 0

**Problem:** Cargo not filling
- **Solution:** Check cargo capacity not full, target has resources, within drill range

## Next Steps

1. Add visual effects for mining lasers
2. Implement sound effects for drilling/scanning
3. Create UI for refinery management
4. Add mining missions/contracts
5. Implement mining territory conflicts
6. Add upgrade system for equipment

## Support Files

- Full docs: `docs/MINING_SYSTEM.md`
- Quick ref: `MINING_QUICK_REFERENCE.md`
- Example: `examples/mining_example.cpp`
- Components: `engine/ecs/MiningComponents.h`
- Systems: `engine/ecs/MiningSystems.h`

## Complete Example

See `examples/mining_example.cpp` for a fully working simulation that demonstrates all features.
