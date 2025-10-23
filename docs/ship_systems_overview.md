# Ship Systems Overview

This document summarizes the new management subsystems that support strategic and moment-to-moment spaceship gameplay. Each section highlights the responsibilities of the subsystem and the kinds of gameplay loops it enables.

## Cargo Management
- Tracks the cargo manifest with per-item mass and volume.
- Enforces freighter capacity limits before items are loaded.
- Supports offloading items to free up volume or mass budget for upcoming missions.

## Crew Management
- Maintains a roster of crew along with their primary role.
- Assigns crew members to bridge or station posts, enabling quick reassignment when mission demands change.
- Allows querying which crew are manning a given station to help drive UI overlays or alert routing.

## Ship Progression
- Stores research tree nodes, their reputation requirements, and research progress.
- Applies research contributions and auto-unlocks nodes when prerequisites and reputation thresholds are met.
- Serves as the backbone for gating new hull modules, logistics upgrades, or faction-specific perks.

## Flight Assist Controls
- Tracks whether auto-leveling and inertial dampening are enabled.
- Exposes assist modes such as manual, stability, cruise, and docking assist to configure the ship's handling profile.
- Allows velocity limit configuration so that bridge UI can show safe angular and linear speeds for a given mode.

## Fuel Management
- Stores per-tank propellant quantities, capacities, and consumption rates.
- Supports continuous fuel burn simulation and refueling operations.
- Offers a simple mission range estimate helper that can power route-planning UI widgets.

## Docking & Hangar Operations
- Registers docking ports, their current occupancy, and airlock status.
- Handles docking requests and alignment scoring to drive guidance overlays.
- Provides hooks for cargo transfer and EVA safety checks by exposing pressurization state.

## Life Support Management
- Tracks key atmospheric metrics (oxygen, CO₂, humidity, temperature) and consumables runtime.
- Consumes resources over time based on crew count and raises emergency flags when thresholds are breached.
- Supports emergency responses such as atmosphere venting and restocking consumables.

These subsystems focus on deterministic state tracking and minimal dependencies, making them easy to integrate into the existing simulation and UI layers.

