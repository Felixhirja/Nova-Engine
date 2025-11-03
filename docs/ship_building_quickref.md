# Ship Building System - Quick Reference

## Files Created

### Core System
- `engine/ShipBuilder.h` - Main ship building system (header)
- `engine/ShipBuilder.cpp` - Implementation
- `engine/ShipEditorUI.h` - ImGui visual editor (header)
- `engine/ShipEditorUI.cpp` - UI implementation

### Data Files
- `assets/config/ship_components.json` - 20 component definitions
- `assets/config/ship_hulls.json` - 6 ship hull templates
- `assets/config/ship_presets.json` - 12 preset loadouts

### Testing
- `tests/test_ship_builder.cpp` - Test program

### Documentation
- `docs/ship_building_system.md` - Complete documentation

## Quick Start

### 1. Initialize Ship Builder
```cpp
#include "engine/ShipBuilder.h"
using namespace ShipBuilding;

ShipBuilder builder;
builder.LoadHullCatalog("assets/config/ship_hulls.json");
builder.LoadComponentCatalog("assets/config/ship_components.json");
builder.LoadPresets("assets/config/ship_presets.json");
```

### 2. Create a Ship
```cpp
// From hull
auto ship = builder.CreateShip("hull_viper_mk1");

// Or load preset
auto ship = builder.LoadPreset(PresetType::Fighter);
```

### 3. Install Components
```cpp
builder.InstallComponent(*ship, "engine_slot", "engine_basic_ion");
builder.InstallComponent(*ship, "weapon_slot_1", "weapon_pulse_laser_mk1");
builder.InstallComponent(*ship, "shield_slot", "shield_basic");
```

### 4. Validate and Calculate
```cpp
// Validate
std::vector<std::string> errors, warnings;
bool valid = builder.ValidateShip(*ship, errors, warnings);

// Calculate performance
auto metrics = builder.CalculatePerformance(*ship);
```

### 5. Use Visual Editor
```cpp
#include "engine/ShipEditorUI.h"

ShipEditorUI editor(&builder);
editor.SetPlayerId("player_001");
editor.OpenEditor(ship);

// In game loop:
if (editor.IsOpen()) {
    editor.Render();
}
```

## Component Types

| Type | Description | Examples |
|------|-------------|----------|
| Engine | Main propulsion | Ion Engine, Plasma Drive |
| Weapon | Combat systems | Pulse Laser, Plasma Cannon |
| Shield | Energy shields | Basic, Reinforced, Military |
| Sensor | Detection systems | Basic Array, Advanced Suite |
| PowerPlant | Power generation | Fusion Reactor |
| CargoHold | Storage | Standard, Reinforced |
| Thruster | Maneuvering | RCS Thrusters |
| Mining | Resource extraction | Mining Laser |
| Computer | Ship AI/control | Flight Computer |

## Ship Hulls

| Hull | Class | Hardpoints | Cost | Best For |
|------|-------|------------|------|----------|
| Viper Mk1 | Fighter | 6 | $50k | Combat |
| Falcon Trader | Trader | 8 | $120k | Trading |
| Explorer Scout | Explorer | 7 | $95k | Exploration |
| Prospector Miner | Miner | 8 | $110k | Mining |
| Interceptor Mk2 | Interceptor | 6 | $65k | Speed |
| Anvil Heavy | Heavy Fighter | 8 | $145k | Heavy Combat |

## Preset Loadouts

| Preset | Hull | Focus | Components |
|--------|------|-------|------------|
| Fighter | Viper | Combat | Basic weapons + shields |
| Heavy Fighter | Anvil | Max DPS | Advanced weapons + military shield |
| Interceptor | Interceptor | Speed | Advanced engine + thrusters |
| Trader | Falcon | Cargo | 2x cargo modules |
| Freighter | Falcon | Max Cargo | 2x reinforced cargo |
| Explorer | Scout | Range | Advanced sensors + fusion |
| Scout | Scout | Speed+Sensors | Plasma drive + advanced sensors |
| Miner | Prospector | Mining | 2x mining lasers + cargo |
| Salvager | Prospector | Salvage | Max cargo capacity |
| Support | Falcon | Utility | Sensors + computer |
| Patrol | Viper | Law Enforcement | Advanced weapons + sensors |
| Bomber | Anvil | Heavy Weapons | 2x plasma cannons |

## Performance Metrics

### Propulsion
- **Max Speed** - Top velocity (m/s)
- **Acceleration** - Thrust-to-mass ratio (m/s²)
- **Maneuverability** - Turn rate (deg/s)

### Combat
- **Firepower** - Total damage per second (DPS)
- **Shield Strength** - Shield hit points
- **Armor Rating** - Hull damage resistance
- **Sensor Range** - Detection distance (km)

### Power & Thermal
- **Power Generation** - Available power (MW)
- **Power Consumption** - Total draw (MW)
- **Cooling Capacity** - Heat dissipation
- **Heat Generation** - System thermal output

### Economics
- **Total Cost** - Ship + components value
- **Maintenance** - Per-cycle operating cost
- **Insurance** - 5% of total cost
- **Payout** - 90% of insured value

## API Reference

### ShipBuilder Methods

#### Creation
- `CreateShip(hullId)` - Create ship from hull
- `LoadPreset(preset)` - Load preset configuration

#### Components
- `InstallComponent(ship, hardpointId, componentId)` - Install component
- `RemoveComponent(ship, hardpointId)` - Remove component
- `GetCompatibleComponents(ship, hardpointId)` - List compatible
- `GetUpgradeOptions(componentId)` - Get upgrade path

#### Validation
- `ValidateShip(ship, errors, warnings)` - Validate configuration
- `CalculatePerformance(ship)` - Calculate metrics

#### Hangar
- `AddToHangar(ship, playerId)` - Add to player hangar
- `RemoveFromHangar(shipId, playerId)` - Remove from hangar
- `GetHangarShips(playerId)` - List player's ships
- `SetActiveShip(shipId, playerId)` - Set active ship

#### Customization
- `SetShipName(ship, name)` - Rename ship
- `SetPaintJob(ship, r, g, b, ...)` - Apply colors
- `SetDecal(ship, decalId)` - Apply decal

#### Insurance
- `CalculateInsuranceCost(ship)` - Get insurance cost
- `PurchaseInsurance(ship)` - Buy insurance
- `FileInsuranceClaim(shipId, playerId)` - Claim on loss

### ShipEditorUI Methods

- `OpenEditor(ship)` - Open editor for ship
- `CloseEditor()` - Close editor
- `IsOpen()` - Check if editor open
- `Render()` - Render UI (call each frame)
- `SetPlayerId(playerId)` - Set player context

## Keyboard Shortcuts (Editor)

| Key | Action |
|-----|--------|
| Ctrl+N | New Ship |
| Ctrl+S | Save Ship |
| Ctrl+O | Open Hangar |
| ESC | Close Editor |

## Common Patterns

### Build a Fighter
```cpp
auto ship = builder.CreateShip("hull_viper_mk1");
builder.InstallComponent(*ship, "engine_slot", "engine_advanced_ion");
builder.InstallComponent(*ship, "weapon_slot_1", "weapon_pulse_laser_mk2");
builder.InstallComponent(*ship, "weapon_slot_2", "weapon_pulse_laser_mk2");
builder.InstallComponent(*ship, "shield_slot", "shield_reinforced");
builder.SetShipName(*ship, "Crimson Ace");
builder.PurchaseInsurance(*ship);
builder.AddToHangar(ship, playerId);
```

### Check Power Balance
```cpp
auto metrics = builder.CalculatePerformance(*ship);
if (metrics.powerBalance < 0) {
    std::cout << "Power deficit: " << metrics.powerBalance << " MW\n";
    // Need to add power plant or reduce consumption
}
```

### Upgrade Components
```cpp
auto upgrades = builder.GetUpgradeOptions("weapon_pulse_laser_mk1");
if (!upgrades.empty()) {
    builder.RemoveComponent(*ship, "weapon_slot_1");
    builder.InstallComponent(*ship, "weapon_slot_1", upgrades[0]->id);
}
```

## Troubleshooting

### "Ship won't move"
- Ensure engine is installed
- Check power balance (engine needs power)

### "Weapons won't fire"
- Check power consumption
- Ensure weapons installed in weapon hardpoints

### "Ship overheating"
- Reduce active systems
- Install power plant with better cooling
- Upgrade to components with lower heat generation

### "Can't install component"
- Check hardpoint type compatibility
- Verify component size fits hardpoint
- Check for missing requirements (power plant, computer)

## Next Steps

1. Load JSON data files (implement JSON parsing)
2. Integrate editor into main game UI
3. Connect to economy system
4. Add 3D ship visualization
5. Implement save/load persistence

---

**For detailed documentation:** See `docs/ship_building_system.md`

**For examples:** Run `tests/test_ship_builder.cpp`

**For data structure:** Check JSON files in `assets/config/`
