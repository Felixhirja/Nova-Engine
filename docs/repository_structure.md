# Repository Structure Overview

Last updated October 2025.

This document summarizes the top-level layout of Nova-Engine after the gameplay actor refactor.

## Source Code (`src/`)

- `actors/` – Gameplay-facing façade classes that bridge ECS data with higher-level systems (e.g., `Player`, `Spaceship`). This top-level folder sits alongside `src/` to keep the engine core lean.
- `ecs/` – Core entity-component-system implementation, systems, and facades. Avoid gameplay-specific logic in this folder.
- `graphics/` – Rendering pipeline, viewport management, shader plumbing, and other visual subsystems.
- `physics/` – Physics integration, collision handling, and movement systems.
- `*.cpp` roots – Legacy entry points and engine bootstrap code. Gradually migrate actor-specific files into `actors/` as they are modernized.

## Assets (`assets/`)

- `config/` – Tuning data, movement bounds, and balance parameters consumable by the simulation.
- `ship_modules/`, `sprites/`, `ui/` – Artifacts consumed by the ship assembly, HUD, and rendering systems.

## Documentation (`docs/`)

- Design references are organized by subsystem (e.g., `spaceship_taxonomy.md`, `audio_system_design.md`).
- `repository_structure.md` (this file) explains the code layout so contributors know where to place new modules.

## Build & Tooling

- `Makefile` – Compiles sources grouped by folder (actors, ecs, physics, graphics) and defines test targets.
- `scripts/` – Utility scripts for asset packaging, diagnostics, and developer workflows.

## Contribution Guidelines

- Place new actor or player-facing facades under `actors/` with matching headers.
- When relocating files, update include paths and documentation references to keep the repo consistent.
- Document significant structural changes here to keep the team aligned on project organization.
