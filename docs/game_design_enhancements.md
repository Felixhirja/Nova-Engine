# Game Design Enhancement Roadmap

_Last updated: October 2025_

This roadmap tracks ongoing refinement work for the Game Design Document suite. Each section references implementation-aligned companion docs.

## Solar System Generation

- ✅ **Implementation Parity Snapshot** added to `docs/solar_system_generation.md` (see "Implementation Parity Snapshot" section).
- 🔄 Maintain documentation parity whenever `SolarSystemGenerator` or `GenerationParameters` change.
- 🔜 Next review after introducing binary stars or new station placement heuristics.

## Spaceship Components

- ✅ `docs/spaceship_taxonomy.md` now includes a component slot & hardpoint reference derived from `SpaceshipCatalog`.
- 🔄 Update matrix when `ComponentSlotCategory` or runtime catalog definitions evolve.
- 🔜 Add variant-specific modifiers once faction hull data is data-driven.

## Player Progression & Skills

- ✅ Authored `docs/player_progression_system.md` outlining XP flow, skill trees, and unlock hooks.
- 🔄 Wire runtime telemetry to validate XP curve assumptions.
- 🔜 Document moment-to-moment UX (skill tree UI, notification timing) once UI prototypes land.

## Faction System

- ✅ Created `docs/faction_system.md` covering faction identities, reputation bands, and integration points.
- 🔄 Synchronize faction IDs with runtime enums/data files.
- 🔜 Capture dynamic territory control rules after the first simulation pass.

## Resource & Crafting

- ✅ Added `docs/resource_and_crafting_system.md` defining resource taxonomy, blueprint structure, and service matrix.
- 🔄 Update when new resource categories or quality tiers are introduced.
- 🔜 Document player onboarding loop for crafting (tutorial beats, NPC guides).

## Tracking & Ownership

- Source of truth: this file plus references above.
- Review cadence: once per milestone (or when major systems land).
- Responsible team: Design leads with support from systems engineering.

