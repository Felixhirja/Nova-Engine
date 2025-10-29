# Actors Module

This directory contains gameplay-level actors such as the player spaceship, NPCs, and world entities like stations and projectiles. Infrastructure code that underpins rendering or ECS integration lives in `engine/`.

## Recent Highlights
- ✅ Header-only `Spaceship` actor with ECS integration
- ✅ Base NPC actor with Trader, Pirate, and Patrol subclasses that expose AI hooks
- ✅ Station actor for docking and hub interactions
- ✅ Projectile actors wired into physics and collision stubs

## TODOs
Active planning for actor work now lives in [`todos.md`](todos.md). Key focus areas include faction integration, JSON-driven spawners, advanced AI behaviors, serialization, and collision polish.

## Rendering Tips
- Batch render calls by render layer to minimize state changes.
- Use level-of-detail (LOD) techniques for distant actors.
