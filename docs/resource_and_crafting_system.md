# Resource & Crafting System Design

_Last updated: October 2025_

## System Overview

The resource/crafting loop binds exploration, combat salvage, and industrial gameplay. It consumes data emitted by procedural generation (`docs/solar_system_generation.md`) and feeds into ship assembly, economy, and progression systems.

Key runtime touchpoints:

- `ResourceManager` (`src/ResourceManager.cpp`) – central inventory catalog & asset loader.
- `ShipAssembly` pipeline – validates crafted components against hull slots.
- `SpaceStationComponent` – indicates available services (refinery, shipyard, fabrication) and production queues.
- `PlanetComponent` / `AsteroidBeltComponent` – provide resource richness values for gathering nodes.

## Resource Taxonomy

| Category | Source | Usage | Notes |
|----------|--------|-------|-------|
| Metallic Ore | Asteroid belts with `composition = Metallic`, industrial station contracts. | Hull plating, weapon frames, structural upgrades. | Quality modifier derived from `resourceRichness`. |
| Rare Alloys | Salvaged from capital wrecks, faction rewards. | High-end weapons, capital ship components. | Requires refinery tier ≥ 3. |
| Volatiles | Gas giants (scoop operations), icy moons with high `organicResources`. | Fuel production, missile propellants, coolant. | Dangerous to harvest; needs specialized modules. |
| Exotics | Flares, anomalies (`StarComponent::hasFlares`), research missions. | Experimental tech, science unlocks. | Often time-limited events. |
| Biomass | Habitable planets (`PlanetComponent::hasLife`) and agricultural stations. | Medical supplies, bio-upgrades, food trade. | Spoils without refrigeration modules. |
| Manufactured Components | Produced in fabrication facilities from base resources. | Ready-to-install modules for ship assembly. | Includes electronics, shield matrices, drive coils. |

## Gathering & Processing Flow

1. **Extraction Missions**
   - Player equips ship with appropriate industrial modules (see `ComponentSlotCategory::Industrial`).
   - Landing or mining minigame uses resource density curves derived from procedural metadata.
   - Output is raw resource stacks with quality ratings.

2. **Refinement**
   - Stations with `availableServices` containing `Refinery` accept raw materials.
   - Processing time and yield determined by station `wealthLevel`, player skill nodes (`Industrial:Fabrication Specialist`), and installed refinery upgrades.
   - Produces refined commodities plus by-products (e.g., slag for trade).

3. **Fabrication**
   - Crafting console UI consumes refined resources + blueprint.
   - Blueprint requirements reference ship component IDs from `ShipComponentCatalog`.
   - Resulting component enters cargo or station storage; flagged as `crafted` for analytics.

4. **Installation**
   - Ship assembly UI validates slot compatibility (`ShipAssembly.cpp`) and updates ship performance metrics.
   - Crafted items inherit quality tier, affecting stats (e.g., +5% power output for superior reactors).

## Blueprint Structure

Store blueprints in `assets/blueprints/*.json` with fields:

```json
{
  "id": "component.powerplant.mark2",
  "displayName": "Mark II Fusion Core",
  "category": "PowerPlant",
  "size": "Medium",
  "inputs": [
    { "resource": "metallic_ore", "quantity": 15 },
    { "resource": "rare_alloy", "quantity": 4 },
    { "resource": "exotics", "quantity": 1 }
  ],
  "craftTimeSeconds": 180,
  "requiresSkill": "Industrial:Fabrication Specialist",
  "requiresReputation": { "faction": "Auroran Combine", "standing": 30 },
  "outputComponentId": "powerplant_fusion_mk2"
}
```

## Station Services Matrix

| Service | Component Flag | Description |
|---------|----------------|-------------|
| Refinery | `availableServices` contains `Refinery` | Converts raw ore/volatiles/biomass into refined goods. |
| Fabricator | `hasShipyard = true` | Builds components and hull sections; tied to blueprint catalog. |
| Market | `hasMarket = true` | Enables trading of resources, buying missing ingredients. |
| Research Lab | `stationType = Research` | Unlocks experimental blueprints in exchange for Exotics + reputation. |
| Shipyard | `hasShipyard = true` + `stationType = Shipyard` | Required to assemble new hulls and install large components. |

## Crafting Quality Tiers

Quality influences component stats and resale value. Tier outcome depends on:

- Input quality (average of resource quality ratings).
- Station tier (wealth level, installed upgrades).
- Player skill modifiers (`Industrial:Autonomous Fleet`, `Science:Temporal Analytics`).

| Tier | Name | Stat Bonus |
|------|------|------------|
| 0 | Crude | -10% to primary stat, used when crafting fails but still produces output. |
| 1 | Standard | Baseline stats from blueprint definition. |
| 2 | Superior | +5% to key stat (power output, shield regen, etc.). |
| 3 | Masterwork | +10% stat bonus, cosmetic variant unlocked. |

## Economy Integration

- Resources feed the dynamic pricing model; scarcity is tied to faction conflicts and player activity.
- Crafting orders can be scheduled as asynchronous jobs (station queue). Completion events trigger notifications and update mission boards.
- Salvage missions populate with manufactured components instead of raw resources, bypassing early-stage crafting steps.

## Analytics & Telemetry

- Track resource acquisition rates, crafting success/quality tiers, and blueprint usage.
- Flag bottlenecks where players lack specific resources; adjust spawn weights or mission rewards accordingly.
- Monitor faction-specific crafting (e.g., Syndicate black-market modules) to tune reputation gating.

## Implementation Checklist

- [ ] Extend `ResourceManager` to register resource categories, stack sizes, and quality metadata.
- [ ] Implement refinement & fabrication job queues linked to station services.
- [ ] Author blueprint JSON schema and loader; integrate with ship assembly validation.
- [ ] Build UI for crafting queue management and blueprint browsing.
- [ ] Add analytics hooks for resource acquisition and crafting outcomes.

