# Ship Assembly Data Schema

Last updated: 2025-10-09

## Overview

The modular ship assembly system is defined in `src/ShipAssembly.h` / `src/ShipAssembly.cpp`. It introduces two primary blueprint types that describe hull archetypes and mountable components. These blueprints populate runtime catalogs used by the assembler to validate loadouts and compute derived statistics.

## Blueprint Types

### `ShipComponentBlueprint`

Represents an installable subsystem. Key fields:

- `id` – unique identifier string used in catalogs and save data.
- `displayName` / `description` – user-facing metadata.
- `category` – `ComponentSlotCategory` enum value indicating the required slot type.
- `size` – `SlotSize` enum, compared against hull slot capacity.
- `massTons`, `powerOutputMW`, `powerDrawMW`, `thrustKN` – quantitative stats that feed into ship performance calculations.
- `heatGenerationMW`, `heatDissipationMW` – thermal profile contributions used to evaluate heat buildup versus cooling.
- `crewRequired`, `crewSupport` – staffing requirements and provided crew capacity for the component (e.g., life support modules).
- `weaponDamagePerShot`, `weaponRangeKm`, `weaponFireRatePerSecond`, `weaponAmmoCapacity`, `weaponAmmoType` – weapon performance stats (only relevant for `Weapon` category components).
- `weaponIsTurret`, `weaponTrackingSpeedDegPerSec`, `weaponProjectileSpeedKmPerSec` – weapon mechanics (turret rotation, projectile speed).

### `ShipHullBlueprint`

Defines a hull archetype that can be assembled. Key fields:

- `id`, `displayName` – unique identifier and label.
- `classType` – `SpaceshipClassType` enum linking back to taxonomy definitions.
- `baseMassTons`, `structuralIntegrity` – intrinsic hull stats.
- `slots` – array of `HullSlot` entries describing individual mounting points.
- `baseCrewRequired`, `baseCrewCapacity` – baseline staffing demands and space available baked into the hull.
- `baseHeatGenerationMW`, `baseHeatDissipationMW` – hull-level thermal characteristics that participate in the heat balance.

### `HullSlot`

- `slotId` – unique per-hull slot identifier (used for assignments and serialization).
- `category` – required `ComponentSlotCategory`.
- `size` – capacity (`SlotSize`) that components must fit within.
- `notes` – flavor text or editor hints.
- `required` – whether the slot must be populated.

## Catalogs

- `ShipComponentCatalog` and `ShipHullCatalog` provide static accessors for blueprint lookup, registration, and enumeration.
- Default blueprints are registered lazily on first use (see `RegisterDefaultComponents` / `RegisterDefaultHulls`). Defaults currently cover fighter and freighter hulls and a baseline component set.

## Assembly Flow

1. A `ShipAssemblyRequest` specifies the target hull ID and a mapping of `slotId -> componentId`.
2. `ShipAssembler::Assemble` resolves blueprints, validates category and size compatibility, and aggregates mass/power/thrust totals.
3. Diagnostics (`ShipAssemblyDiagnostics`) capture missing or invalid assignments and non-fatal warnings such as net power deficits.
4. `ShipAssemblyResult` synthesizes derived metrics, subsystem rollups, and exposes helper accessors such as `NetPowerMW()`.
5. `ShipAssemblyResult::Serialize` produces a JSON-style string suitable for saves or logging.

### `ShipAssemblyResult`

- `performance` – consolidated `ShipPerformanceMetrics` struct covering mass, thrust, power, thermal, and crew totals plus helper ratios (net power/heat, thrust-to-mass, crew utilization).
- `totalMassTons`, `totalPowerOutputMW`, `totalPowerDrawMW`, `totalThrustKN` – mirrored aggregates kept for legacy call sites.
- `availablePowerMW` – convenience value mirroring `NetPowerMW()` (output minus draw).
- `mainThrustKN` / `maneuverThrustKN` – thrust broken down by primary and maneuvering thrusters for flight model consumption.
- `totalHeatGenerationMW`, `totalHeatDissipationMW`, `NetHeatMW()` – heat balance values driven by hull and component data.
- `crewRequired`, `crewCapacity`, `CrewUtilization()` – staffing totals and ratios highlighting shortages or unused berths.
- `avionicsModuleCount`, `avionicsPowerDrawMW` – avionics inventory metrics derived from sensor/computer subsystems.
- `subsystems` – hash map keyed by `ComponentSlotCategory` containing `SubsystemSummary` rollups (per-category mass/power/thrust/thermal/crew listings).
- `ThrustToMassRatio()` – helper returning thrust-to-weight ratio in kN per ton, useful for quick performance checks.

### `SubsystemSummary`

Stores per-category aggregates that power editor diagnostics and UI slices:

- `components` – vector of `AssembledComponent` installed in the subsystem.
- `totalMassTons`, `totalPowerOutputMW`, `totalPowerDrawMW`, `totalThrustKN` – aggregated stats for just this subsystem.
- `totalHeatGenerationMW`, `totalHeatDissipationMW`, `crewRequired`, `crewSupport` – subsystem-scoped thermal and crew figures for diagnostics/UI.

### Validation Rules

- Slot compatibility requires identical `ComponentSlotCategory` and a component `SlotSize` whose ordinal does not exceed the slot's capacity. The helper `SlotSizeFits` centralizes this comparison.
- Missing assignments for required slots emit diagnostics errors and prevent successful assembly. Optional slots surface warnings when left empty.
- Unknown component or hull IDs also produce blocking errors.
- When aggregate power draw exceeds output, the assembler surfaces a warning but still returns a result so UIs can present actionable guidance.
- Additional warnings surface when heat generation exceeds dissipation or when crew requirements outstrip available capacity, enabling shipyard UX to highlight staffing shortfalls and cooling issues.
- Per-subsystem rollups power richer messaging (for example highlighting which subsystem overdraws power).

## Extending the Schema

- Register additional hulls via `ShipHullCatalog::Register`; typically expand `RegisterDefaultHulls` or load from external data.
- Introduce new component categories/sizes by updating the enums in `Spaceship.h`; the assembler automatically respects new values when registered blueprints specify them.
- For data-driven ingestion, consider loading blueprint definitions from JSON and populating the catalogs at startup.

## Related Work

- Taxonomy source: `actors/Spaceship.h` / `actors/Spaceship.cpp` (class definitions, slot specs).
- Unit test exercising the schema and assembler: `tests/test_ship_assembly.cpp`.

