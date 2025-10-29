# Camera System Overview

Nova Engine's third-person camera is composed of a lightweight `Camera` primitive, a high-level follow module, and optional helpers for configuration, presets, and runtime control. This document captures how the pieces fit together and what each module is responsible for.

## Core `Camera` primitive

The `Camera` class stores position, orientation, and field-of-view state while providing helper methods that make rendering and gameplay code simpler.

* Position/orientation/zoom are available through `SetPosition`, `SetOrientation`, and `SetZoom`, plus read-only accessors used by the rest of the engine. The constructor clamps FOV to the `[30°, 90°]` guardrails so callers never see invalid zoom values. 【F:engine/Camera.h†L21-L48】【F:engine/Camera.cpp†L14-L55】
* `SetTargetZoom` and `UpdateZoom` implement a simple exponential lerp toward a desired FOV, allowing smooth zoom transitions driven by input or scripted events. 【F:engine/Camera.h†L39-L44】【F:engine/Camera.cpp†L38-L53】
* `ApplyToOpenGL` writes the camera transform into the fixed-function model-view matrix whenever GLFW/SDL builds are active, while headless builds provide a no-op stub. 【F:engine/Camera.h†L33-L36】【F:engine/Camera.cpp†L56-L76】
* Modern renderers can fetch column-major view/projection matrices via `GetViewMatrix` and `GetProjectionMatrix`, removing the dependency on legacy OpenGL state. 【F:engine/Camera.h†L38-L45】【F:engine/Camera.cpp†L128-L164】
* `BuildBasis` returns an orthonormal forward/right/up triple that downstream systems reuse for navigation, input, and debugging gizmos. It includes safeguards for degenerate cases near the poles. 【F:engine/Camera.h†L45-L48】【F:engine/Camera.cpp†L78-L126】

## Target-lock follow module

`UpdateTargetLockCamera` is the central routine that drives cinematic third-person framing. It works against three data structures:

* `CameraFollowConfig` bundles all tunable parameters (orbit, smoothing, camera safety, free-cam tuning, and optional behaviors). A built-in `Validate` pass clamps hot-loaded values to safe ranges at runtime. 【F:engine/CameraFollow.h†L25-L137】
* `CameraFollowState` tracks per-frame data such as smoothing history, orbit offsets, and teleport timers so the camera can pick up where it left off. 【F:engine/CameraFollow.h†L139-L168】
* `CameraFollowInput` provides the caller's per-frame inputs (player position and mouse-look offsets) and communicates whether target lock should be active. 【F:engine/CameraFollow.h†L170-L179】

During an update the function:

1. Clamps `deltaTime`, blends toward the requested target-lock weight, and maintains distinct orbit offsets for locked vs. free states to avoid sudden yaw jumps. 【F:engine/CameraFollow.cpp†L17-L86】
2. Computes the desired shoulder, orbit position, and smoothing targets, then enforces minimum player distance and a configurable ground clamp. Optional teleport recovery and obstacle avoidance hooks further sanitize the target location. 【F:engine/CameraFollow.cpp†L88-L203】
3. Derives a yaw/pitch target aimed at the player, including special handling when the camera is nearly vertical, and blends toward it with configurable responsiveness and optional pitch clamping. 【F:engine/CameraFollow.cpp†L205-L256】
4. Records bookkeeping (teleport timers and last desired location) so the next frame can apply the same smoothing rules. 【F:engine/CameraFollow.cpp†L258-L275】

## Free-look controller

`CameraFollowController` wraps the follow routine with additional input handling for the designer-facing "free camera" tools.

* The controller owns both config and state blobs and exposes helpers to reset or inject new tuning values (for example, after loading an external profile). 【F:engine/CameraFollowController.h†L21-L33】
* `Update` first delegates to `UpdateTargetLockCamera`, then adds extra rotation and movement only when the camera is fully in free mode. It also swallows one frame of input when transitioning into target-lock so the view does not stutter. 【F:engine/CameraFollowController.cpp†L18-L54】
* `ApplyFreeCameraMovement` constructs a camera-aligned basis, computes desired velocities from WASD-style inputs, smooths them exponentially, and integrates position—complete with sprint/slow modifiers and deadzone snapping. 【F:engine/CameraFollowController.cpp†L56-L122】
* `ApplyFreeLookRotation` interprets mouse deltas as yaw/pitch adjustments with configurable inversion and clamps pitch to ±89° to avoid gimbal lock. 【F:engine/CameraFollowController.cpp†L124-L170】

## Config profiles

Camera tuning lives in INI-style profiles that can be swapped without code changes.

* `CameraConfigLoader::LoadCameraFollowConfigProfile` locates and parses a profile file, supporting both absolute paths and a small set of relative fallbacks. Profiles can be named; if the requested one does not exist the loader falls back to a `default` profile or the first entry. 【F:engine/CameraConfigLoader.cpp†L129-L200】
* The parser accepts doubles, integers, and booleans and maps each key to its corresponding `CameraFollowConfig` field. The same `Validate` logic runs after parsing to catch bad data. 【F:engine/CameraConfigLoader.cpp†L25-L123】

### Example profile snippet

```ini
[default]
orbitDistance = 12.0
orbitHeight = 3.0
transitionSpeed = 3.0
posResponsiveness = 10.0
rotResponsiveness = 12.0
softGroundClamp = true
obstacleMargin = 0.5
```

Save the file next to the executable (or provide an absolute path) and call `LoadCameraFollowConfig(path, config)` before wiring the config into the controller.

## Presets

`CameraPresets` offers a trio of baked transforms for rapid prototyping and debug views. Each preset sets position, orientation, and zoom in one call, and the helper immediately synchronizes the zoom target so the lerp system does not fight the new setting. 【F:engine/CameraPresets.cpp†L1-L22】

## Integration checklist

1. Construct a `Camera` instance and `CameraFollowController` at startup.
2. Load or author a `CameraFollowConfig`, optionally using `CameraConfigLoader`.
3. Feed `CameraFollowInput` each frame (player position + input deltas) and, when in free mode, populate a `CameraMovementInput` struct for the controller.
4. Invoke `CameraFollowController::Update` each frame; the underlying camera now holds the smoothed transform and can be supplied to rendering systems such as `Viewport3D`.

This document should give engineers and designers enough context to tune the follow behavior, hook it into gameplay code, and extend it with future features like new target-lock modes or additional camera presets.
