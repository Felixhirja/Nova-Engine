# Engine Framework Overview

Nova Engine relies on a collection of focused frameworks that cooperate during
bootstrap and the frame scheduler loop. This guide summarizes who owns each
framework, where the core headers live, and which extension hooks to use when
adding features or integrating new subsystems.

## Runtime Framework Summary

| Framework | Ownership | Key headers | Responsibilities |
| --- | --- | --- | --- |
| Engine bootstrap & actor wiring | Core runtime | `engine/EngineBootstrap.h`, `engine/EngineBootstrap.cpp`, `engine/ActorRegistry.h` | Seeds the simulation state, exposes archetype-aware actor context, and prepares demo assets. |
| Frame scheduler & main loop | Core runtime | `engine/FrameScheduler.h`, `engine/MainLoop.cpp` | Drives input polling, simulation updates, rendering, and shutdown checks via callback hooks. |
| ECS simulation & scheduler | Gameplay systems | `engine/Simulation.h`, `engine/Simulation.cpp`, `engine/ecs/SystemSchedulerV2.h`, `engine/ecs/LegacySystemAdapter.h` | Hosts gameplay systems, manages legacy adapters, and feeds the parallel scheduler graph. |
| Resource & asset management | Asset pipeline | `engine/ResourceManager.h`, `engine/EngineBootstrap.cpp` | Loads assets, registers sprite metadata, and exposes GPU sync helpers. |
| Input & device abstraction | Platform layer | `engine/Input.h`, `engine/MainLoop.cpp` | Normalizes keyboard/mouse/gamepad polling and dispatches window callbacks. |
| Rendering & viewport | Graphics | `engine/Viewport3D.h`, `engine/Viewport3D.cpp`, `engine/MainLoop.cpp` | Manages OpenGL state, HUD overlays, and particle/HUD batching. |
| Audio playback & feedback | Audio | `engine/AudioSystem.h`, `engine/AudioFeedbackSystem.h`, `engine/AudioFeedbackSystem.cpp` | Initializes SDL_mixer, routes feedback events, and manages spatial/looping audio. |
| HUD & alert feedback | UX | `engine/HUDAlertSystem.h`, `engine/VisualFeedbackSystem.h`, `engine/MainLoop.cpp`, `engine/Viewport3D.cpp` | Translates feedback events into on-screen alerts, particles, and HUD draw calls. |

---

### Engine bootstrap & actor wiring
**Ownership:** Core runtime  
**Key headers:** `engine/EngineBootstrap.h`, `engine/EngineBootstrap.cpp`, `engine/ActorRegistry.h`

* `EngineBootstrap::Run` enables the archetype facade, populates the
  `ActorContext`, and attaches the scheduler pointer so actors can queue ECS
  work later in the frame.【F:engine/EngineBootstrap.cpp†L162-L206】
* Demo assets and sprite metadata are registered here—new bootstrap resources
  should follow the `ResourceManager::Load`/`RegisterSprite` pattern already in
  use.【F:engine/EngineBootstrap.cpp†L185-L204】
* Actor definitions are discovered through the registry; add new actor headers
  to `engine/Actors.h` to ensure they participate in the bootstrap scan.

### Frame scheduler & main loop
**Ownership:** Core runtime  
**Key headers:** `engine/FrameScheduler.h`, `engine/MainLoop.cpp`

* `FrameSchedulerConfig` exposes fixed-update and render pacing knobs, while the
  callback struct supplies hooks for input, simulation, rendering, and teardown.【F:engine/FrameScheduler.h†L7-L35】
* `MainLoop::MainLoopFunc` builds the scheduler callbacks, drives input polling,
  and wires update/render handlers, making it the primary integration point for
  frame-stage extensions.【F:engine/MainLoop.cpp†L339-L860】
* Headless controls and FPS tracking live in the `onFrameComplete` callback—new
  telemetry or diagnostics should extend this block to preserve pacing.

### ECS simulation & scheduler
**Ownership:** Gameplay systems  
**Key headers:** `engine/Simulation.h`, `engine/Simulation.cpp`,
`engine/ecs/SystemSchedulerV2.h`, `engine/ecs/LegacySystemAdapter.h`

* `Simulation` exposes the scheduler via `GetSchedulerV2()` and toggles the V2
  path with `SetUseSchedulerV2`, ensuring external callers can opt into the
  parallel execution path.【F:engine/Simulation.h†L38-L77】
* `ConfigureSchedulerV2` registers each gameplay adapter, specifying update
  phases and dependencies with `ecs::LegacySystemAdapterConfig`. New systems are
  appended here to participate in the scheduler graph.【F:engine/Simulation.cpp†L1024-L1096】
* `SystemSchedulerV2::RegisterSystem` owns lifetime and profiling data for each
  system, while `LegacySystemAdapter` bridges classic `System` implementations
  into the new scheduler without rewriting gameplay logic.【F:engine/ecs/SystemSchedulerV2.h†L296-L347】【F:engine/ecs/LegacySystemAdapter.h†L16-L56】

### Resource & asset management
**Ownership:** Asset pipeline  
**Key headers:** `engine/ResourceManager.h`, `engine/EngineBootstrap.cpp`

* `ResourceManager` assigns integer handles, tracks sprite metadata, and exposes
  backend-specific helpers (GLFW sprite buffer sync, SDL texture cache).【F:engine/ResourceManager.h†L11-L51】
* Bootstrap code demonstrates the canonical pattern: load assets, register
  sprite metadata, and stash handles for later actor use.【F:engine/EngineBootstrap.cpp†L185-L204】
* When extending the asset pipeline, prefer registering metadata immediately
  after load so GPU buffers and ECS components remain in sync.

### Input & device abstraction
**Ownership:** Platform layer  
**Key headers:** `engine/Input.h`, `engine/MainLoop.cpp`

* `Input::Init`, `PollKey`, and `IsKeyHeld` provide a minimal platform-neutral
  API, while platform-specific window handles enable scroll and focus plumbing.【F:engine/Input.h†L26-L68】
* The main loop refreshes key state at frame start, feeds the menu system, and
  handles quit toggles—attach new global hotkeys in this section to keep input
  behavior centralized.【F:engine/MainLoop.cpp†L367-L520】
* SDL- and GLFW-specific callbacks live alongside the generic path; extend them
  cautiously to preserve headless operation.

### Rendering & viewport
**Ownership:** Graphics  
**Key headers:** `engine/Viewport3D.h`, `engine/Viewport3D.cpp`, `engine/MainLoop.cpp`

* Rendering callbacks clear the frame, draw world geometry, and invoke HUD +
  particle rendering using the viewport helpers.【F:engine/MainLoop.cpp†L752-L820】
* `Viewport3D::DrawHUD` configures OpenGL state, draws textured HUD overlays, and
  routes UI batch calls—extend this when adding new HUD panels or overlays.【F:engine/Viewport3D.cpp†L3765-L3849】
* Mouse-wheel zoom, capture shortcuts, and ECS inspector rendering are wired
  through the main loop render stage to keep viewport responsibilities isolated.【F:engine/MainLoop.cpp†L787-L829】

### Audio playback & feedback
**Ownership:** Audio  
**Key headers:** `engine/AudioSystem.h`, `engine/AudioFeedbackSystem.h`, `engine/AudioFeedbackSystem.cpp`

* `AudioSystem` exposes static initialization, sound/music playback, and
  fine-grained volume controls built on SDL_mixer.【F:engine/AudioSystem.h†L24-L119】
* `AudioFeedbackSystem` registers reusable clips, maps `FeedbackEvent`s to audio
  cues, and maintains looping alarm state—new events should be handled in
  `OnFeedbackEvent`.【F:engine/AudioFeedbackSystem.h†L1-L66】【F:engine/AudioFeedbackSystem.cpp†L202-L260】
* Listener orientation/position setters keep spatial audio aligned with camera
  updates; call them from frame code when integrating new camera rigs.

### HUD & alert feedback
**Ownership:** UX  
**Key headers:** `engine/HUDAlertSystem.h`, `engine/VisualFeedbackSystem.h`, `engine/MainLoop.cpp`, `engine/Viewport3D.cpp`

* `HUDAlertSystem` builds prioritized alert queues from feedback events and
  exposes `GetActiveAlerts()` for rendering overlays.【F:engine/HUDAlertSystem.h†L8-L76】
* `VisualFeedbackSystem` responds to events with particles and screen shake; the
  main loop updates it during the fixed step and hands particle buffers to the
  viewport for rendering.【F:engine/VisualFeedbackSystem.h†L1-L45】【F:engine/MainLoop.cpp†L740-L819】
* HUD drawing pulls telemetry, particles, and alerts together via
  `Viewport3D::DrawHUD`, making it the aggregation point for UI extensions.【F:engine/Viewport3D.cpp†L3765-L3849】

---

## Quick Start: Registering a Framework Subsystem

Follow these steps when introducing a new gameplay framework that must execute
alongside existing systems and participate in bootstrap wiring.

1. **Expose the scheduler to bootstrap.** `MainLoop::Init` already passes the
   active scheduler pointer into `EngineBootstrap::Run`, ensuring actor context
   and demo content have access to ECS orchestration.【F:engine/MainLoop.cpp†L294-L299】
2. **Register the subsystem with the V2 scheduler.** Mirror the pattern in
   `Simulation::ConfigureSchedulerV2` by adapting legacy systems into
   `ecs::SystemSchedulerV2` with explicit phase and dependency metadata.【F:engine/Simulation.cpp†L1024-L1096】
3. **Hook into gameplay initialization.** Toggle `Simulation::SetUseSchedulerV2`
   (or ensure it remains enabled) so the new subsystem runs during
   `callbacks.onFixedUpdate`.

```cpp
// Example: register a ThermalSystem that depends on movement results
#include "ecs/LegacySystemAdapter.h"
#include "ThermalSystem.h"

void Simulation::ConfigureSchedulerV2(EntityManager& entityManager) {
    // ...existing adapters...
    using ThermalAdapter = ecs::LegacySystemAdapter<ThermalSystem>;
    ecs::LegacySystemAdapterConfig thermalConfig;
    thermalConfig.phase = ecs::UpdatePhase::Simulation;
    thermalConfig.systemDependencies.push_back(
        ecs::SystemDependency::Requires<MovementAdapter>());
    schedulerV2_.RegisterSystem<ThermalAdapter>(entityManager, thermalConfig);
    // ...
}
```

With the adapter registered, the fixed-update callback automatically drives the
new subsystem each frame, and bootstrap-created actors can access the scheduler
through their `ActorContext` when queuing work or dispatching feedback events.【F:engine/MainLoop.cpp†L740-L819】【F:engine/EngineBootstrap.cpp†L167-L204】
