# Actors TODOs

## Completed
- Separated infrastructure code by moving `Spaceship.cpp` into `engine/`.
- Updated include paths after refactoring actor directories.
- Implemented the high-level `Spaceship` actor in `actors/`.
- Added core NPC actor types (Trader, Pirate, Patrol) with basic AI behaviors.
- Introduced a `Station` actor for docking and station interactions.
- Created projectile actors that integrate with physics and collision hooks.

## In Progress
- Expand faction system hooks so actors can query faction state and adjust behavior.
- Build an actor spawn system driven by JSON manifests and reusable spawn bundles.

## Upcoming
- Implement a modular AI behavior system for advanced decision making and pathfinding.
- Add serialization support so actors can save and restore state.
- Deliver full collision handling between projectiles and actors, including responses.

## Backlog / Ideas
- Optimize actor rendering through render-layer batching and level-of-detail (LOD) logic.
- Explore dynamic mission/event hooks that drive actor behaviors.
- Add automated tests covering actor lifecycle, AI transitions, and serialization edge cases.
