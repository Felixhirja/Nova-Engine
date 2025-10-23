# Controls & UX Implementation Plan

This document captures the detailed design for the outstanding Controls & UX items tracked in `docs/todo/todo_spaceship.txt`. It expands on how each feature connects to existing systems (flight assist, HUD, docking) and defines a build order with technical constraints.

## 1. Cockpit / Bridge Layouts

We will ship three layout tiers tailored to the ship taxonomy defined in `src/Spaceship.h`.

| Ship class (see `SpaceshipClassType`) | Layout concept | Primary widgets | Secondary widgets | Notes |
| --- | --- | --- | --- | --- |
| Fighter / Light craft | Narrow-forward canopy with single pilot HUD ribbon | Velocity tape, artificial horizon, shield arcs, weapon capacitor bar | Mission clock, comms stack, target bracket | Mirrors existing 7-segment HUD in `Viewport3D::DrawHUD` and keeps sightlines clear for dogfights. |
| Explorer / Freighter (medium) | Dual-seat bridge with wrap-around holo panes | Power flow schematic (from `EnergyManagementSystem`), cargo manifest highlights, nav map inset | Crew tasking reminders, jump spool timer, subsystem health grid | Uses modular panes; text rendering dependency remains. |
| Capital / Industrial | Multi-station bridge with panoramic displays | Fleet command overlay, hangar deck status, damage control routing | Flight wing assignments, logistics throughput, command priority queue | Requires multi-channel alerting from `HUDAlertSystem` plus docking telemetry feeds. |

### Layout guidelines

* All layouts use a shared widget registry so `HUDAlertSystem` can place alerts in consistent screen regions.
* Each layout exposes a "systems ribbon" that surfaces the `FlightAssistController` mode, auto-level, and dampening toggles for rapid confirmation.
* Medium and large layouts integrate docking status panes that subscribe to `DockingSystem` updates.

## 2. Flight Assist Toggles

`FlightAssistController` already exposes `EnableAutoLevel`, `EnableDampening`, and `SetMode`. We will:

1. Add a `FlightAssistToggleEvent` routed through `FeedbackEvent` so HUD cues (tone + icon flash) trigger when the pilot changes a mode.
2. Extend the simulation loop to poll the new action bindings (see §3) and call the controller methods.
3. Persist the toggle state per ship hull using the `ShipLogisticsSystem` save payload.
4. Surface the current mode, auto-level, and dampening flags in the HUD ribbon (layout-dependent) and broadcast to any telemetry listeners.

Safety guardrails:

* Auto-level forces a gradual torque application, capped by `ConfigureVelocityLimits`.
* Inertial dampening applies a velocity kill delta but respects `FuelManagementSystem` draw limits to avoid spikes.

## 3. Input Remapping & Sensitivity Tuning

We will extend `Input` with a flight profile API:

* Action bindings (`pitch_up`, `strafe_left`, `toggle_auto_level`, etc.) stored in a `FlightInputProfile` map with defaults for keyboard, HOTAS, and gamepad templates.
* Axis definitions specify positive/negative bindings and expose a sensitivity scalar; this feeds the flight model and dampening curves.
* Profile persistence via JSON under `artifacts/input_profiles/<profile>.json` with hot reload support for designers.

Runtime usage:

1. During boot, call `Input::RegisterDefaultFlightProfile()` to seed defaults.
2. Provide `Input::RemapFlightAction(actionId, keycode)` and `Input::SetFlightAxisConfig(axisId, config)` for UI screens.
3. Use `Input::IsFlightActionActive(actionId)` and `Input::SampleFlightAxis(axisId)` inside the flight loop.

A lightweight CLI diagnostics command (`scripts/input_profile_dump.py`) will print the resolved bindings to aid QA.

## 4. Tutorial Missions & Simulator Scenarios

We will stage content in the `Simulation` module:

* **Tutorial ladder** – Four missions (basic controls, combat maneuvers, docking, emergency procedures). Each mission references a JSON scenario describing spawn points, instructor VO cues, and success criteria.
* **Free-flight simulator** – Sandbox mode accessed from the main menu; reuses tutorial scripting but disables fail states.
* Mission scripting hooks into `HUDAlertSystem` for prompts and uses the telemetry feed to detect pass/fail thresholds (e.g., maintain level flight within ±5° for 10 seconds).

Implementation sequencing:

1. Add a `TutorialScenario` loader in `Simulation.cpp` that instantiates ships, waypoints, and triggers.
2. Author scenario data under `assets/tutorials/` with a manifest linking to audio/text prompts.
3. Integrate completion tracking with `ShipProgressionSystem` to unlock advanced assists.

## 5. Real-Time Telemetry HUD

Telemetry taps into `RigidBody` data (`src/ecs/Components.h`):

* Speed = magnitude of linear velocity vector.
* G-force = magnitude of acceleration / 9.80665.
* System status = aggregate from `EnergyManagementSystem`, `ShieldSystem`, and `FlightAssistController` state.

HUD work:

1. Introduce a `FlightTelemetrySample` struct (speed, gForce, angular rates, assist mode).
2. Update `Viewport3D::DrawHUD` overload to accept an optional telemetry pointer; when provided, render a three-column stack: speed tape, G-meter bar, system icons.
3. Provide color-coded thresholds (green < 3g, yellow 3-6g, red > 6g) and tie into `HUDAlertSystem` for sustained overload warnings.

## 6. Docking & Hangar UX

Docking builds on `DockingSystem` and extends Ship Logistics features.

### Components & Guidance

* Add `DockingPort` child components to station prefabs with transform anchors for guidance beacons.
* Implement procedural approach vectors that feed alignment cues (roll, pitch, yaw offsets) to the HUD docking widget.

### Transitional Animations

* Trigger animation events for airlock pressurization, clamps, and bridge extension via a `DockingSequenceController`.
* Sync audio cues with the existing `AudioFeedbackSystem` for clamps and seal confirmation.

### HUD & Safety Prompts

* The HUD docking widget displays:
  * Port identifier, distance, closure rate
  * Alignment indicators that turn green when within tolerance
  * Safety timers for pressurization cycles and clamp release windows
* Safety interlocks prevent undocking until clamps disengage and pressure equalizes; the HUD shows the blocking condition with severity coloring.

### Hangar Flow

* Once docked, transition to a hangar operations screen listing refuel, repair, and cargo transfer actions. This uses the `ShipLogisticsSystem` inventory and crew assignments to estimate durations.
* Provide quick actions (refuel max, embark crew) bound to the input profile for controller parity.

## 7. Implementation Checklist

1. Extend `Input` with flight profile remapping API and wire to simulation loop.
2. Implement telemetry sampling and HUD rendering updates once text rendering is unblocked.
3. Build docking widget and sequence controller in parallel with station asset updates.
4. Author tutorial scenarios and integrate with progression unlocks.
5. QA pass on each ship class layout for clarity and readability.

With this plan, every item in the Controls & UX section now has a concrete execution path linked to existing systems and planned assets.
