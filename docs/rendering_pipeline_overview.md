# Rendering Pipeline Overview

Nova Engine's rendering stack stitches together the game loop, backend window/context management, scene drawing systems, and post-processing into a layered pipeline. This document summarizes how the pieces interact so new contributors can find the right integration point quickly.

## Frame Lifecycle

Each frame is orchestrated by the `MainLoop` callbacks:

1. **Clear the framebuffer(s)** – `viewport->Clear()` resets whichever backend is active (SDL OpenGL, SDL renderer, or GLFW) and ensures the default framebuffer is bound before clearing color and depth buffers.【F:engine/MainLoop.cpp†L757-L831】【F:engine/Viewport3D.cpp†L2329-L2404】
2. **Render the 3D scene** – `viewport->Render(...)` activates the current view configuration, binds the appropriate context, and draws camera debug overlays when a GL backend is available.【F:engine/MainLoop.cpp†L787-L821】【F:engine/Viewport3D.cpp†L1703-L1727】
3. **Draw actors or manual meshes** – after the scene render the loop calls `viewport->DrawEntity(...)`, which resolves per-entity mesh overrides before delegating to `DrawMeshAt` for actual geometry submission.【F:engine/MainLoop.cpp†L790-L796】【F:engine/Viewport3D.cpp†L2620-L2642】
4. **Render HUD & diagnostics** – `viewport->DrawHUD(...)` builds retained-mode UI batches, overlays textures, and renders text metrics before flushing batched quads to the GPU.【F:engine/MainLoop.cpp†L807-L822】【F:engine/Viewport3D.cpp†L3765-L3990】
5. **Play particle effects** – any active `VisualFeedbackSystem` particles are submitted to the GPU point-sprite pipeline via `viewport->RenderParticles(...)`.【F:engine/MainLoop.cpp†L817-L820】【F:engine/Viewport3D.cpp†L4303-L4324】
6. **Finalize and present** – `viewport->FinishFrame()` resets the viewport state and `viewport->Present()` swaps buffers or presents the SDL renderer output.【F:engine/MainLoop.cpp†L829-L830】【F:engine/Viewport3D.cpp†L1684-L1701】【F:engine/Viewport3D.cpp†L2408-L2454】

This ordering keeps gameplay logic independent of the rendering backend while still allowing specialized passes between major stages.

## Backend & View Management

`Viewport3D` owns the windowing backend and context state. Switching between SDL GL, SDL software renderer, and GLFW is centralized in `SetBackend`, which also cleans up GL-only resources when the engine drops to a non-GL mode.【F:engine/Viewport3D.cpp†L1440-L1481】 During a render pass, `ActivateView` picks the active sub-viewport, makes the matching context current, and routes to either the OpenGL or SDL path, while `ApplyViewportView` computes the pixel viewport rectangle and updates GL or SDL state accordingly.【F:engine/Viewport3D.cpp†L1703-L1747】 The helper `BeginFrame()` ensures the correct context is current before clearing so multi-window setups remain stable.【F:engine/Viewport3D.cpp†L1684-L1697】

## Scene Rendering Systems

### ECS Actor Rendering

Actors that participate in the entity-component system attach a `DrawComponent`, which describes render mode, resource handles, animation, and debug options.【F:engine/ecs/Components.h†L740-L835】 The `ActorRenderer` system iterates `DrawComponent` + `Position` pairs, updates simple animation timers, and dispatches to mode-specific helpers (sprite, billboard, mesh, particles, wireframe, or custom callbacks).【F:engine/graphics/ActorRenderer.h†L15-L114】 Those helpers are currently stubs awaiting concrete shader/material integrations, but the structure keeps DrawComponent-driven rendering in one place.

### Direct Mesh Submission

Legacy and diagnostic drawing paths still run through `Viewport3D::DrawEntity`. Entities can register an override mesh and scale via `SetEntityMesh`, after which `DrawEntity` forwards to `DrawMeshAt` to bind materials, apply transforms, and fall back to the default player mesh when no override exists.【F:engine/Viewport3D.cpp†L2620-L2650】【F:engine/Viewport3D.cpp†L2468-L2558】 This lets gameplay code render without going through ECS while the new systems come online.

### Instanced Mesh Batching

For high object counts the `Nova::InstancedMeshRenderer` collects submissions keyed by mesh/material, lazily allocates per-batch instance buffers, and flushes the batches in a single draw call with per-instance matrices, color tints, and custom scalars.【F:engine/graphics/InstancedMeshRenderer.cpp†L14-L166】 `Viewport3D` constructs and initializes this renderer when a GL backend is active so scene code can submit large groups without paying per-entity draw overhead.【F:engine/Viewport3D.cpp†L1979-L2004】

### Particle Effects

Particles originate from the `VisualFeedbackSystem` and are rendered through a retained `ParticleRenderer`. The renderer allocates a VAO/VBO pair, streams live particle vertices each frame, and renders them as point sprites with additive blending and camera-aware sizing before restoring default GL state.【F:engine/graphics/ParticleRenderer.cpp†L16-L199】 `Viewport3D::RenderParticles` lazily creates the renderer on first use and skips work when no GL context is active.【F:engine/Viewport3D.cpp†L4303-L4324】

## HUD & UI Composition

Screen-space UI uses the retained-mode `UIBatcher`. The batcher owns GPU buffers for quads and triangles, accumulating colored primitives per frame before a single `Flush()` uploads them. Viewport HUD code calls `Begin()`, pushes quads/outlines/triangles for panels, reticles, gizmos, and debug hints, mixes in text via `TextRenderer`, then flushes once to minimize state churn.【F:engine/graphics/UIBatcher.cpp†L12-L199】【F:engine/Viewport3D.cpp†L3840-L3979】 Separate SDL renderer fallbacks mirror this logic with software primitives when GL is unavailable.【F:engine/Viewport3D.cpp†L3992-L4211】

## Post-Processing Pipeline

The `PostProcessPipeline` class encapsulates render-to-texture effects such as bloom and letterboxing. Initialization creates the main scene FBO (with depth), a downsampled bright-pass FBO, and ping-pong blur buffers; `BeginScene()` binds the scene FBO, and `EndScene()` chains bright extraction, blur passes, compositing back to the default framebuffer, and optional letterbox overlay.【F:engine/PostProcessPipeline.h†L20-L89】【F:engine/PostProcessPipeline.cpp†L117-L315】【F:engine/PostProcessPipeline.cpp†L399-L504】 Although integration hooks are still being threaded through the viewport, the pipeline is ready to slot in ahead of `FinishFrame()` whenever off-screen rendering is required.

## Putting It Together

At a high level: gameplay updates schedule draw work via DrawComponents, entity mesh bindings, particle systems, and HUD telemetry. The main loop hands control to `Viewport3D`, which normalizes backend state, submits scene geometry (including instanced batches), renders screen-space UI, optionally runs post-processing, and presents the final frame. This layering keeps future renderer upgrades (e.g., modern shader passes or new effects) contained within the viewport/graphics modules without destabilizing simulation code.
