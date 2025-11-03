# Actors Module - Auto-Loading Entity System

This directory contains gameplay-level actors such as the player spaceship, NPCs, and world entities like stations and projectiles. Infrastructure code that underpins rendering or ECS integration lives in `engine/`.

## 🚀 What "Auto-Loading" Means Here

**Automatic Build Registration**: The build system scans `entities/*.h` and automatically includes new entities - no manual registration macros needed!

**JSON Configuration Auto-Loading**: Entities automatically load their configuration from JSON files in `assets/actors/`

**ECS Component Auto-Setup**: Entities automatically configure their rendering, physics, and other components

**Factory Auto-Integration**: The EntityFactory can create entities with one line of code

## 📋 Key Features Demonstrated

**Scalability**: Easily add hundreds of entity types without modifying core engine code or factories.

**Designer Accessibility**: Non-programmers can tweak entity behaviors via JSON files, enabling rapid iteration.

**Reduced Boilerplate**: No need for explicit registration or setup code—headers and configs handle it automatically.

**Error Resilience**: Built-in validation during loading catches config issues early, with fallback defaults.

**Seamless Modding Support**: Community creators can drop in new headers and JSONs without recompiling the engine core.

**Performance Optimization**: Lazy loading of configs ensures only active entities consume resources.

## 🗂️ The Auto-Loading Architecture

### Header-Only Pattern (e.g., CargoContainer.h)

- Inherits from `IActor`
- Uses `#include "../engine/EntityCommon.h"` for proper include order
- Implements auto-loading in `Initialize()` method

### JSON Configuration (e.g., cargo_container.json)

- Designer-friendly properties in `assets/actors/`
- Rich metadata for gameplay systems
- Multiple variants supported

### Build System Auto-Registration

- Makefile automatically scans `entities/*.h`
- Generates `engine/Entities.h` with all includes
- No manual registration required!

### EntityFactory Integration

- Type-safe creation methods (e.g., `CreateCargoContainer()`)
- Generic creation via `CreateFromConfig("cargo_container")`
- Automatic error handling and validation

## Implemented Actors

### Core Actors

- **Player** (`Player.h`): Header-only player spaceship actor with JSON configuration, input handling and camera control
- **Spaceship** (`Spaceship.h`): Header-only generic spaceship actor for any ship type with JSON configuration
- **Projectile** (`Projectile.h`): Header-only physics-based projectile actor (bullets, missiles, lasers) with JSON configuration
- **CargoContainer** (`CargoContainer.h`): Header-only cargo storage entity with auto-loading JSON configuration (demonstrates full auto-loading pattern)

## 🎮 Auto-Loading Gameplay Features

**Inventory Management**: Cargo containers can store and manage player items with capacity limits defined in JSON.

**Physics Interactions**: Automatic setup for collision detection, gravity, and destructibility based on config metadata.

**Visual Variants**: Supports multiple skins or models (e.g., damaged, locked) loaded from JSON for dynamic rendering.

**Event Triggers**: Integrates with ECS to handle events like opening, exploding, or spawning contents on destruction.

## 🔧 Implementation Example

Here's how the auto-loading pattern works in practice with CargoContainer:

### CargoContainer.h (Header-Only Entity)

```cpp
#include "../engine/EntityCommon.h"

class CargoContainer : public IActor {
public:
    CargoContainer() { LoadConfiguration(); }
    
    void Initialize() override {
        SetupComponents();  // Auto-applies JSON config to ECS
    }
    
private:
    void LoadConfiguration() {
        config_.LoadFromFile("assets/actors/cargo_container.json");
    }
    
    void SetupComponents() {
        // Auto-setup DrawComponent, PhysicsBody, Position from JSON
    }
    
    ActorConfig config_;
};
```

### assets/actors/cargo_container.json (Designer Config)

```json
{
    "name": "Standard Cargo Container",
    "capacity": 2500.0,
    "cargoType": "general",
    "health": 500.0,
    "mass": 1000.0,
    "visual": {
        "scale": 2.0,
        "color": [0.6, 0.4, 0.2]
    }
}
```

### EntityFactory Integration (Auto-Generated)

```cpp
// One-line creation with full auto-loading
auto container = EntityFactory::CreateCargoContainer(entityManager, x, y, z);

// Generic creation via string
auto container = EntityFactory::CreateFromConfig("cargo_container", entityManager, x, y, z);
```

This auto-loading pattern can be used to create any new entity in Nova Engine - just create the header, add the JSON config, and the build system handles the rest!

### NPC Actors

- **NPC** (`NPC.h/.cpp`): Base NPC class with AI hooks and JSON configuration
  - **TraderNPC**: Trades goods between stations
  - **PirateNPC**: Hostile ships that attack traders
  - **PatrolNPC**: Military ships that patrol areas

### World Entities

- **Station** (`Station.h`): Header-only space station actor with docking and services, JSON configuration

## Architecture

Actors follow a consistent pattern:

- Inherit from `IActor` base class
- Use `ActorContext` for ECS integration
- Load configuration from JSON files in `assets/actors/`
- Automatically register with `ActorRegistry` via `REGISTER_ACTOR` macro
- Header-only where possible for designer-friendly extension

## JSON Configuration System

All actors now support designer-friendly JSON configuration:

```json
{
    "name": "Player Ship",
    "speed": 100.0,
    "health": 1000.0,
    "shield": 500.0,
    "model": "player_ship",
    "faction": "player"
}
```

Configuration files are loaded automatically in `Initialize()` methods and provide:

- **Type-safe access**: `ActorConfig::GetString()`, `GetNumber()`, `GetBoolean()`
- **Default values**: Graceful fallback if config file missing or incomplete
- **Designer iteration**: No code changes needed to tweak actor properties

### Station-Specific Features

Stations now support advanced designer configuration:

```json
{
    "name": "Alpha Outpost",
    "health": 8000.0,
    "shield": 3000.0,
    "model": "station_model_high",
    "dockingCapacity": 6,
    "services": ["trading", "repair", "missions"],
    "behaviorScript": "station_behaviors.lua",
    "type": "military",
    "faction": "federation"
}
```

- **Dynamic Services**: Services array defines what the station offers (trading, repair, missions)
- **Scriptable Behaviors**: `behaviorScript` field for Lua scripting (future integration)
- **Type Configuration**: Station type (trading/military/mining/research) configurable via JSON
- **Serialization Support**: `Serialize()` and `Deserialize()` methods for save/load functionality

## Recent Highlights

- ✅ Complete actor system with IActor interface and ActorRegistry
- ✅ Header-only Player, Spaceship, Station, and Projectile actors
- ✅ Split NPC actors (header + implementation) for complex AI
- ✅ JSON-driven configuration system for all actors
- ✅ Automatic actor registration via build system
- ✅ Successful compilation and testing
- ✅ **Station Refactoring**: Designer-friendly JSON config, serialization support, scriptable behaviors

## TODOs

Active planning for actor work now lives in [`todos.md`](todos.md). Key focus areas include faction integration, JSON-driven spawners, advanced AI behaviors, serialization, and collision polish.

## Rendering Tips

- Batch render calls by render layer to minimize state changes.
- Use level-of-detail (LOD) techniques for distant actors.
