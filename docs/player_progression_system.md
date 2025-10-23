# Player Progression & Skill Framework

_Last updated: October 2025_

## Design Goals

1. **Guide the campaign arc** with clear milestones that surface new ships, equipment, and mission types.
2. **Reinforce playstyles** through distinct skill tracks (combat, piloting, industry, diplomacy) that provide meaningful perks without invalidating baseline gameplay.
3. **Integrate with existing data surfaces** so runtime systems can query a single source of truth. Progression hooks must reference:
   - `ProgressionTier` definitions embedded in `SpaceshipCatalog` (`src/Spaceship.cpp`).
   - Faction reputation stored via the ECS `Faction` component and economy metadata on stations (`SpaceStationComponent`).
   - Resource classifications emitted by solar-system generation (`PlanetComponent`, `AsteroidBeltComponent`).

## Progression Currency & Level Curve

| Element | Description | Implementation Notes |
|---------|-------------|----------------------|
| **Experience (XP)** | Earned from missions, combat, exploration scans, crafting deliveries. | Track per-player profile; XP gates global level and awards skill points. |
| **Skill Points** | Granted every level (1 point) and at narrative milestones (bonus points). | Spent on skill nodes; refund tokens available via rare market item. |
| **Reputation** | Per-faction standing ranging from -100 to +100. | Backed by `Faction` component relationships; influences mission pools and docking permissions. |
| **Blueprint Credits** | Special currency used to unlock advanced crafting recipes and ship modules. | Awarded from high-tier contracts, exploration finds, and faction shops. |

### Level Bands

| Band | Levels | Unlock Focus |
|------|--------|--------------|
| Initiate | 1-5 | Tutorial arc, unlock starter fighter (`ProgressionTier` 1). |
| Specialist | 6-15 | Introduce branch choice missions, unlock Tier 2 ship variants. |
| Veteran | 16-25 | Open capital reputation tracks, mid-tier crafting recipes. |
| Legend | 26-40 | Access Tier 3-4 hulls, experimental technology, faction flagships. |

Level milestones trigger curated missions (see `MissionScript` taxonomy) and station services (repair discounts, blueprint vendors) by toggling service flags on the owning `SpaceStationComponent`.

## Skill Trees

Each skill tree is a directed graph of `ProgressionNode` records (JSON/Lua data TBD). Nodes require:

- `id` (string) – unique identifier used by save data and UI.
- `category` – one of `Pilot`, `Combat`, `Industrial`, `Diplomacy`, `Science`.
- `tier` – numeric depth; deeper nodes demand higher player level and prerequisite completions.
- `cost` – skill points required.
- `effects` – declarative modifiers consumed by gameplay systems (e.g., `{"type":"weapon_heat", "value":-0.1}`).

### Pilot Tree

Focuses on flight handling and traversal QoL.

| Tier | Node | Effect |
|------|------|--------|
| 1 | Vector Mastery | +10% strafe acceleration (`MovementParameters::strafeAcceleration`). |
| 1 | Gravity Sense | Reduces gravity penalty when in planetary atmospheres by 15%. |
| 2 | Slipspace Familiarity | +1 micro-jump charge capacity; reduces cooldown by 10%. |
| 3 | Precision Docking | Auto-align assistance unlock; grants docking fee discounts with neutral factions. |

### Combat Tree

Improves weapon handling and durability.

| Tier | Node | Effect |
|------|------|--------|
| 1 | Weapons Certification | +5% weapon capacitor efficiency; reduces `Weapon` subsystem power draw. |
| 2 | Shield Harmonics | +10% shield regen rate; ties into `ShieldSystem.cpp`. |
| 2 | Tactical Awareness | Expands threat indicators in HUD; unlocks multi-target tagging in `TargetingSystem.cpp`. |
| 3 | Ace Maneuvers | Temporarily boosts maneuver thruster output when shields fall below 30%. |

### Industrial Tree

Ties into resource gathering, logistics, and crafting.

| Tier | Node | Effect |
|------|------|--------|
| 1 | Prospecting License | Reveals resource veins on HUD using data from `AsteroidBeltComponent::resourceRichness`. |
| 2 | Fabrication Specialist | -15% crafting time at stations with `hasShipyard`. |
| 2 | Cargo Optimizer | +10% cargo capacity when `Cargo` subsystem utilization > 50%. |
| 3 | Autonomous Fleet | Grants ability to dispatch NPC miners using spare industrial ships. |

### Diplomacy Tree

Rewards negotiation, faction play, and station services.

| Tier | Node | Effect |
|------|------|--------|
| 1 | Trade Liaison | +5 baseline reputation when first docking with a faction station. |
| 2 | Conflict Mediator | Unlocks special missions to reduce faction wars; increases rewards for escort contracts. |
| 3 | Alliance Architect | Grants passive reputation drift toward aligned factions when running their missions. |

### Science Tree

Connects exploration and anomaly research.

| Tier | Node | Effect |
|------|------|--------|
| 1 | Anomaly Scanner | Adds rare node detection to exploration UI (leverages `StarComponent::hasFlares`). |
| 2 | Exobiology | Doubles biomass yields from planets with `PlanetComponent::hasLife`. |
| 3 | Temporal Analytics | Unlocks experimental upgrades requiring Exotics resource type; reduces cooldown on research station projects by 20%.

## Unlock Framework

1. **Ship Unlocks** – `ProgressionTier` entries in `SpaceshipCatalog` already specify the tier name and description. Tie each tier to:
   - Minimum player level.
   - Minimum faction reputation (e.g., Helios Dominion Tier 2 hull requires +40 Dominion rep).
   - Required blueprint credit spend.

2. **Module Unlocks** – Ship component blueprints reference prerequisite skill nodes (`Industrial:Fabrication Specialist`) or faction standings. The assembler UI should fetch unlock status before presenting options.

3. **Mission Chains** – Narrative designers author mission sequences per level band. Completion toggles new station services via `availableServices` on `SpaceStationComponent`.

4. **Economy Integration** – Resource crafting recipes require specific resources plus skill nodes. Example: Advanced shield matrix = Metallic Ore × 20 + Exotics × 5 + `Science:Temporal Analytics` unlocked.

## Data & Tooling Plan

- **Data Format** – Store skill trees and unlock tables in `assets/progression/` as JSON. Loader populates an in-memory `ProgressionCatalog` at startup.
- **Editor Support** – Extend existing tooling to visualize node graphs and validate prerequisite loops.
- **Telemetry** – Capture skill point spend and unlock timings to tune XP curves; feed results into analytics dashboards.

## Next Steps

1. Implement `ProgressionCatalog` runtime API mirroring `SpaceshipCatalog` for skill nodes and unlock tables.
2. Wire XP/reputation accrual into mission, combat, and crafting systems.
3. Build UI widgets for skill tree browsing and unlock notifications.
4. Integrate with save/load so progression state persists across sessions.

