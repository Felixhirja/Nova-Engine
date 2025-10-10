# Spaceship Class Taxonomy

This document defines the baseline taxonomy for player-flyable and AI-controlled spacecraft within Star Engine. It captures the signature traits, baseline statistics, progression tiers, and faction-specific variants for each major class.

## Shared Terminology

- **Mass Class** – Structural weight tier used by physics, docking, and logistics systems.
- **Crew Complement** – Recommended number of active stations (pilot, gunner, engineer, etc.).
- **Power Budget** – Baseline power plant output in megawatts used for balance and subsystem sizing.
- **Hardpoint Layout** – Count of primary/secondary weapon mounts, utility pylons, and module sockets.
- **Progression Tier** – The typical character or faction milestone where the hull unlocks in the campaign.

## Summary Matrix

| Class       | Mass Class | Crew Complement | Power Budget (MW) | Hardpoints (Primary / Utility / Module) | Signature Roles |
|-------------|------------|-----------------|-------------------|------------------------------------------|-----------------|
| Fighter     | Light      | 1-2             | 8-12              | 2 / 1 / 1                                | Interception, dogfighting, patrol |
| Freighter   | Medium     | 2-4             | 18-26             | 1 / 2 / 3                                | Cargo hauling, convoy support |
| Explorer    | Medium     | 3-5             | 16-22             | 1 / 3 / 3                                | Survey, reconnaissance, science |
| Industrial  | Heavy      | 4-6             | 24-34             | 2 / 2 / 4                                | Mining, repair, construction |
| Capital     | Super Heavy| 8-18            | 60-120            | 6 / 4 / 6                                | Fleet command, heavy support |

---

## Fighter Class

- **Signature Traits**
  - Emphasizes high thrust-to-mass ratio for rapid acceleration and strafing.
  - Minimal profile to reduce target cross-section and improve stealth coatings.
  - Limited endurance; relies on carriers or forward bases for refuel/rearm cycles.
- **Baseline Stats**
  - Hull mass: 25-35 tons
  - Crew: single-seat with optional drone wingman slot
  - Power output: 10 MW baseline
  - Hardpoints: 2 primary (fixed/pivot guns), 1 utility (countermeasures), 1 module (avionics upgrade)
- **Progression Tiers**
  - Tier 1: Starter light fighter unlocked during tutorial arc.
  - Tier 2: Specialist interceptors with enhanced maneuver thrusters.
  - Tier 3: Elite strike fighters with modular wing pylons and stealth packages.
- **Faction Variants**
  - **Terran Navy "Raptor"** – Balanced stats, supports missile racks.
  - **Outer Rim Syndicate "Viper"** – Sacrifices armor for boosted engines and smuggling compartment.
  - **Zenith Collective "Aurora"** – Integrates energy re-routing module for sustained beam weapons.

## Freighter Class

- **Signature Traits**
  - Optimized cargo volume and modular bays for containers or specialized payloads.
  - Reinforced frames for micro-jump stability and convoy docking.
  - Moderate armament; depends on escort or drone defenses.
- **Baseline Stats**
  - Hull mass: 90-120 tons
  - Crew: 3 core stations (pilot, logistics, engineer)
  - Power output: 22 MW baseline
  - Hardpoints: 1 primary (defensive turret), 2 utility (countermeasures, tractor beam), 3 modules (cargo bay extensions, shield capacitor, drone bay)
- **Progression Tiers**
  - Tier 1: Light haulers for intra-system trade missions.
  - Tier 2: Medium freighters with detachable cargo pods and security locks.
  - Tier 3: Heavy freighters integrating jump-capable cargo frames and automated loaders.
- **Faction Variants**
  - **Terran Commerce Guild "Atlas"** – Focus on security seals and customs compliance modules.
  - **Frontier Miners Union "Prospector"** – Swappable mining rigs and ore refining bay.
  - **Free Traders League "Nomad"** – Expanded crew quarters and smuggling compartments.

## Explorer Class

- **Signature Traits**
  - Extended sensor suites, survey drones, and science laboratories.
  - Efficient drives with hybrid atmospheric capability.
  - Modular bays for data cores, sample containment, and research pods.
- **Baseline Stats**
  - Hull mass: 80-95 tons
  - Crew: 4-5 specialists (pilot, navigator, scientist, engineer, optional comms)
  - Power output: 19 MW baseline
  - Hardpoints: 1 primary (defensive turret), 3 utility (sensor array, drone control, repair beam), 3 modules (labs, data core, stealth probe bay)
- **Progression Tiers**
  - Tier 1: Survey corvettes for planetary mapping contracts.
  - Tier 2: Deep-space scouts with long-range jump matrix and cloaked probes.
  - Tier 3: Expedition cruisers with onboard fabrication and advanced anomaly shielding.
- **Faction Variants**
  - **Academy of Sciences "Odyssey"** – Enhanced lab capacity and science crew buffs.
  - **Free Horizon Cartographers "Pathfinder"** – Jump range bonus, terrain scanners.
  - **Shadow Consortium "Phantom"** – Sensor-masking systems and covert data vaults.

## Industrial Class

- **Signature Traits**
  - Heavy-duty frames supporting industrial equipment (mining lasers, repair gantries).
  - Robust power distribution for simultaneous tool operation.
  - Expanded utility slots for drones, cranes, or fabrication rigs.
- **Baseline Stats**
  - Hull mass: 140-180 tons
  - Crew: 5 technicians (pilot, chief engineer, mining supervisor, drone operator, logistics)
  - Power output: 30 MW baseline
  - Hardpoints: 2 primary (defensive cannons), 2 utility (tractor beams, repair projectors), 4 modules (mining rigs, fabrication arrays, salvage bay, shield inducers)
- **Progression Tiers**
  - Tier 1: Workhorse utility ships for salvage/repair missions.
  - Tier 2: Deep-core miners with armored drill heads and ore refineries.
  - Tier 3: Construction platforms capable of deploying outposts and orbital structures.
- **Faction Variants**
  - **Union of Labor "Forge"** – Resilient hulls with redundancy for hazardous environments.
  - **Corporate Combine "Constructor"** – Advanced fabrication modules and supply chain bonuses.
  - **Scavenger Clans "Scrap Queen"** – Expanded salvage bays and modular crane arms.

## Capital Class

- **Signature Traits**
  - Command-and-control platforms with layered shielding and hanger capacity.
  - Multiple subsystem redundancies and distributed crew stations.
  - Acts as mobile base for strike craft, logistics, and fleet coordination.
- **Baseline Stats**
  - Hull mass: 600-950 tons
  - Crew: 12-18 (bridge crew, tactical officers, engineering division, flight deck)
  - Power output: 85 MW baseline (scales up to 120 MW for flagship variants)
  - Hardpoints: 6 primary (turrets/beam arrays), 4 utility (point-defense grids, sensor masts), 6 modules (hangar bays, shield amplifiers, command modules, drone racks, medical bay, fabrication plant)
- **Progression Tiers**
  - Tier 2: Escort carriers accessible through faction reputation milestones.
  - Tier 3: Battleships and command dreadnoughts unlocked via endgame campaigns.
  - Tier 4: Legendary flagships tied to narrative arcs and faction loyalty quests.
- **Faction Variants**
  - **Terran Navy "Resolute"** – Balanced defenses with fighter bay bonuses.
  - **Zenith Collective "Echelon"** – Superior energy projectors and psionic shielding nodes.
  - **Outer Rim Syndicate "Leviathan"** – Heavy armor plating and boarding pod launchers.

---

## Implementation Notes

- Baseline stats should feed directly into data tables consumed by the simulation and balance tools.
- Faction variants inherit from the base class archetype using additive modifiers (JSON or Lua descriptors TBD).
- Progression tiers map to milestone IDs in the mission and economy systems; unlocking a hull automatically exposes associated component blueprints.
- Coordination required with art, audio, and UI teams to ensure consistent visual language and telemetry exposure.
- The runtime taxonomy is currently implemented in-code via `SpaceshipCatalog` (`src/Spaceship.cpp`) exposing the structures defined in this document.
