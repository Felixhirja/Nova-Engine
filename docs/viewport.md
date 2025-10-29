# Viewport3D Overview

Nova Engine's `Viewport3D` class owns the presentation surface, manages windowing backends, and orchestrates the entire frame lifecycle for 3D rendering and HUD composition. It provides a common abstraction over SDL (OpenGL and software) and GLFW contexts while exposing hooks for gameplay code to schedule draw work.

## Responsibilities

- **Initialization & Lifecycle** – `Viewport3D::Init` sets up the active backend and allocates renderer state. During each frame the main loop calls `Clear`, `Render`, `DrawHUD`, `RenderParticles`, `FinishFrame`, and `Present` in that order to produce the final image.【F:engine/MainLoop.cpp†L753-L830】
- **Backend Management** – The viewport can bind SDL or GLFW windows, report whether an OpenGL context is active (`usingGL`), and expose raw window handles for subsystems that need them.【F:engine/Viewport3D.h†L86-L137】
- **Entity Rendering** – Gameplay code can queue world geometry through `DrawEntity` overloads or override meshes on a per-entity basis with `SetEntityMesh`/`ClearEntityMesh` to customize visuals.【F:engine/Viewport3D.h†L114-L142】
- **HUD & Overlays** – `DrawHUD`, `RenderMenuOverlay`, and auxiliary helpers (axes gizmos, camera markers) layer screen-space UI and debugging tools atop the 3D scene. Brief usage hints for the debug toggles appear when `SetShowHudHints` is enabled, reminding players about F8/F9 shortcuts.【F:engine/Viewport3D.h†L152-L194】【F:engine/Viewport3D.cpp†L3969-L4126】
- **Diagnostics & Capture** – Utilities such as `CaptureToBMP`, `ToggleWorldAxes`, and `ToggleMiniAxesGizmo` assist in debugging frame output and instrumentation without leaving the running session.【F:engine/Viewport3D.h†L200-L214】

## Layouts and Views

Viewport layouts determine how many sub-views are active and how they are positioned within the window. Each layout comprises one or more `ViewportView` descriptors that specify normalized rectangles, roles (main, secondary, minimap), and whether a view should render as an overlay.

Nova Engine ships with three defaults:

1. **Single View** – A full-screen primary camera.
2. **Split Vertical** – Side-by-side primary and secondary cameras.
3. **Main + Minimap** – A full-screen primary camera with an overlaid minimap in the upper-right corner.

These presets live in `Viewport3D::CreateDefaultLayouts` and are installed during initialization, but games can provide custom layouts via `ConfigureLayouts` and `SetActiveLayout`. Runtime helpers like `CycleLayout`, `GetActiveLayoutName`, and `IsOverlayView` make it easy to present UI feedback or toggle between layouts in response to player input.【F:engine/Viewport3D.cpp†L1529-L1641】

## Frame Pacing & VSync

The viewport tracks VSync and frame rate hints so the engine can harmonize simulation pacing with display refresh. `SetFramePacingHint` stores the desired limit while `SetVSyncEnabled` delegates to the active backend (SDL in the current implementation) to change swap intervals without disrupting whichever context was previously current.【F:engine/Viewport3D.cpp†L1643-L1681】

## Extensibility Hooks

Rendering subsystems can retrieve helper components (UI batchers, particle pipelines, etc.) through the viewport, keeping graphics-specific state centralized. For example, `RenderParticles` bridges the particle system into the frame, and `GetUIBatcher` exposes the retained-mode UI layer for overlays like the ECS inspector.【F:engine/Viewport3D.h†L180-L214】【F:engine/ecs/ECSInspector.cpp†L96-L271】

> **Tip:** When adding new rendering features, start by extending `Viewport3D` so the frame lifecycle stays consistent. New effects (post-processing, debug overlays, split-screen layouts) should register through the viewport before touching gameplay systems.
