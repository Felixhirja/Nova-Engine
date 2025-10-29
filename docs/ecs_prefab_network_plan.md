# ECS Prefab, Networking, and Simulation Enhancements

This document expands the high-level roadmap for Nova Engine's next-generation entity workflow. It captures the required systems, design goals, and integration points for prefabs, deterministic simulation, and multiplayer replication.

## Prefabs and Prototypes

### Goals
- Allow designers to compose blueprints from reusable component fragments.
- Support nested prefabs where a child can override or extend ancestor components.
- Provide validation to ensure overrides respect component schemas.

### Implementation Sketch
1. **Prefab Assets**: Introduce `assets/prefabs/*.json` files that specify component bundles and optional parent references.
2. **Prefab Library**: Add an `engine/ecs/PrefabLibrary` service that resolves inheritance chains, merges component overrides, and caches instantiated archetypes.
3. **Editor Hooks**: Update the existing ECS inspector to surface prefab ancestry and highlight overridden properties.
4. **Runtime Instantiation**: Extend `EntityManager` with `SpawnFromPrefab(name, overrides)` that clones the prefab, applies runtime overrides, and returns an `EntityHandle`.

## Entity Templates & Parameterized Spawning

### Goals
- Provide ergonomic factory functions per entity archetype.
- Allow default component configuration with optional parameters exposed to scripting/UI.

### Implementation Sketch
1. Define a `TemplateDescriptor` that references a prefab and declares typed parameters.
2. Generate helper functions in `ShipAssemblySystem` and other subsystems to consume descriptor parameters and mutate components before final spawn.
3. Persist templates alongside prefabs so designers can tweak defaults without code changes.

## Multi-World Support & Partitioning

### Goals
- Run multiple ECS worlds concurrently (e.g., menu, gameplay, background simulation).
- Partition entities spatially for culling, streaming, and replication.
- Track memory usage per world for profiling and budgeting.

### Implementation Sketch
1. Introduce a `WorldId` abstraction and refactor `EntityManager` internals to scope archetype storage by world.
2. Add a `WorldAllocator` interface with arena allocators keyed by world; integrate with component pools.
3. Implement a `WorldPartition` module that maintains quadtree/Octree cells and tracks entity membership via component listeners.

## Rollback-Friendly ECS

### Goals
- Record recent component states for deterministic rollback (e.g., 10–15 frames).
- Provide hooks to re-simulate systems after rollback or correction events.

### Implementation Sketch
1. Wrap component storage with a `StateRingBuffer<T>` capturing snapshots per tick (structure-of-arrays diffed when possible).
2. Emit `RollbackEvent` through `SystemEventBus` when a rewind is requested; systems re-run via `SystemSchedulerV2::ReSimulate(range)`.
3. Ensure `DeterministicRandom` integrates with rollback by seeding per-frame streams.

## Net Replication Layer

### Goals
- Synchronize relevant entities to clients with minimal bandwidth.
- Support delta compression, prediction, and reconciliation.

### Implementation Sketch
1. Implement `GhostManager` that tracks replicated entities, interest bounds, and last acknowledged states.
2. Use component change masks from the rollback buffers to emit deltas.
3. Provide client-side prediction hooks in movement/physics systems and reconciliation callback to reapply authoritative states.

## Scripting Bindings

### Goals
- Allow scripted gameplay logic via Lua/C#/JS while safeguarding engine integrity.
- Support hot-reloading of scripts and reflection of whitelisted components.

### Implementation Sketch
1. Extend the reflection metadata generator to emit binding descriptors for approved components/systems.
2. Implement a `ScriptBridge` facade with per-language adapters (start with Lua; design for C#/JS parity).
3. Integrate hot-reload by monitoring script directories and refreshing stateful systems through lifecycle callbacks.

## GPU Streaming Path

### Goals
- Stream large buffers (meshes, skinning data) to the GPU asynchronously to avoid stalls.

### Implementation Sketch
1. Design a `GpuStreamingQueue` that stages data in persistent mapped buffers.
2. Coordinate uploads with `FrameScheduler` to amortize costs across frames.
3. Provide telemetry hooks to surface upload latency and bandwidth utilization.
