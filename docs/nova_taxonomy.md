# Nova Engine Gameplay Taxonomy

This document summarizes the major taxonomic groups used when describing gameplay-facing content within Nova Engine. It is intended to help designers, programmers, and QA align on shared terminology when discussing feature scope, asset requirements, or balancing passes. Each section lists the defining traits for the category, the typical data sources, and relevant subsystems that consume the information.

## 1. Playable & AI Entities

- **Core Definition**: Any runtime object that can navigate the world, receive input (player or AI), and interact through combat, logistics, or scripted events.
- **Primary Classes**: `Spaceship`, `Drone`, `CapitalShip`, `StationModule`.
- **Data Sources**: `src/Spaceship.cpp`, faction tuning tables, AI behavior trees, and the physics configuration files in `assets/config/`.
- **Key Subsystems**: Flight model, targeting, damage routing, docking, and mission scripting.

### 1.1 Sub-Taxonomy: Spacecraft

- **Light Craft**: Fighters, couriers, and interceptors optimized for high acceleration and low crew count. Share audio banks with the small-hull soundset and reuse the cockpit HUD widgets.
- **Medium Craft**: Explorers, freighters, gunships. Bridge the gap between agile fighters and heavy industrial hulls; integrate auxiliary systems such as survey scanners and logistics bays.
- **Heavy Craft**: Industrial rigs, carriers, and dreadnoughts with multi-crew layouts. Drive the capital ship combat loops, hangar gameplay, and large-scale AI battles.
- **Non-Combat Craft**: Shuttles, rescue boats, and habitat transports. Participate in narrative missions and civilian traffic simulations.

## 2. Environments & Locales

- **Core Definition**: Spatial zones the player can traverse, including orbital spaces, stations, megastructures, and planetary surfaces.
- **Primary Classes**: `SceneGraph`, `SectorDefinition`, `InteriorChunk`, and `MissionZone`.
- **Data Sources**: Procedural generation rules in `assets/worldgen/`, authored layouts in `assets/scenes/`, and shader configuration in `shaders/`.
- **Key Subsystems**: Streaming/level-of-detail, lighting, audio reverb profiles, mission triggers, and weather simulation (for atmospheric locales).

### 2.1 Sub-Taxonomy: Environment Archetypes

- **Deep Space Sectors**: Sparse geometry, heavy reliance on skyboxes, navigation hazards such as debris fields.
- **Orbital Installations**: Dense object fields, docking collars, rotating habitat modules; interacts closely with the docking minigame code.
- **Planetside Zones**: Terrain heightfields, atmospheric scattering shaders, ground vehicle spawns.
- **Cinematic Set Pieces**: Mission-specific scenes with bespoke scripting, often tied to campaign beats.

## 3. Technology & Equipment

- **Core Definition**: Upgradable hardware that augments spacecraft, stations, or player abilities.
- **Primary Classes**: `ModuleDefinition`, `WeaponBlueprint`, `UtilityRig`, `PowerCore`.
- **Data Sources**: JSON/Lua descriptors in `assets/equipment/`, shader/material files, particle systems in `assets/vfx/`.
- **Key Subsystems**: Inventory, crafting, damage model, UI equipment browser, progression/unlocks.

### 3.1 Sub-Taxonomy: Equipment Categories

- **Offensive Systems**: Ballistic turrets, energy lances, missile racks. Balanced via DPS curves in the combat balancing spreadsheets.
- **Defensive Systems**: Shields, armor plates, countermeasure launchers. Interact with the mitigation curves and heat dissipation models.
- **Utility Systems**: Tractor beams, scan suites, fabrication modules. Connects to missions, economy loops, and exploration gating.
- **Support Modules**: Crew amenities, medical bays, command uplinks. Feed into morale, crew management, and narrative events.

## 4. Narrative & Progression Elements

- **Core Definition**: Structures that pace the player experience, unlock content, and deliver story beats.
- **Primary Classes**: `MissionScript`, `DialogueBundle`, `FactionStanding`, `ProgressionNode`.
- **Data Sources**: Dialogue JSON in `assets/narrative/`, mission graphs, progression tables, and localization files.
- **Key Subsystems**: Mission orchestration, branching dialogue, reputation tracking, and reward distribution.

### 4.1 Sub-Taxonomy: Progression Nodes

- **Campaign Milestones**: Gate high-tier ships and modules; tracked in `ProgressionNode` definitions with narrative hooks.
- **Faction Reputation**: Unlocks faction-specific hulls, cosmetics, and missions; integrates with `FactionStanding` computations.
- **Skill Trees**: Pilot/crew abilities affecting cooldowns, crafting efficiency, or tactical bonuses.
- **Economy Unlocks**: Station services, trade routes, and crafting recipes tied to market simulation parameters.

## 5. World Simulation Systems

- **Core Definition**: Background systems that evolve the universe, simulate supply/demand, and drive emergent events.
- **Primary Classes**: `EconomyController`, `LogisticsNetwork`, `ConflictDirector`, `DynamicEvent`.
- **Data Sources**: Simulation constants in `assets/sim/`, AI director scripts, and telemetry gathered from playtests.
- **Key Subsystems**: Trade lane generation, faction warfare, distress call injection, dynamic mission generator.

### 5.1 Sub-Taxonomy: Simulation Pillars

- **Economy Layer**: Commodity flow, price updates, resource node depletion.
- **Conflict Layer**: Faction fleets, territory control, war weariness metrics.
- **Exploration Layer**: Discovery of anomalies, relics, and procedural points of interest.
- **Logistics Layer**: Supply chain management for stations, convoy scheduling, and cargo mission availability.

---

By cataloging Nova Engine content within these taxonomic groups, teams can quickly assess scope overlap, identify missing coverage, and maintain consistent terminology across design documents, implementation notes, and toolchain metadata. Future updates should append additional subsections rather than rewriting existing categories to preserve cross-references.
