# Actors Module

This directory contains gameplay-level actors such as the player spaceship, NPCs, and world entities like stations and projectiles. Infrastructure code that underpins rendering or ECS integration lives in `engine/`.

## Implemented Actors

### Core Actors

- **Player** (`Player.h`): Header-only player spaceship actor with JSON configuration, input handling and camera control
- **Spaceship** (`Spaceship.h`): Header-only generic spaceship actor for any ship type with JSON configuration
- **Projectile** (`Projectile.h`): Header-only physics-based projectile actor (bullets, missiles, lasers) with JSON configuration

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
