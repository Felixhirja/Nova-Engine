# Engine Axes and Matrix Conventions

## World Coordinate System
- The engine uses a right-handed world coordinate system with **+X pointing to the right**, **+Y pointing upward**, and **+Z extending forward from the origin**. Debug rendering colors these axes red, green, and blue respectively when the world-axis helper is enabled.【F:engine/Viewport3D.cpp†L2875-L2935】
- Axis helpers label both the positive and negative directions so orientation remains clear even when the scene is dense. Labels are drawn with depth testing temporarily disabled to keep them visible before restoring the previous depth state.【F:engine/Viewport3D.cpp†L2925-L2947】
- A compact HUD gizmo mirrors the same convention in screen space (X→right, Y→up, Z highlighted at the origin) and can be toggled in debug builds for quick orientation checks.【F:engine/Viewport3D.cpp†L3943-L3978】

### Debug Toggles
- Press **F8** to show or hide the 3D world-axis helper at the origin.
- Press **F9** to show or hide the HUD mini-gizmo.
  These hints are surfaced directly in the HUD when debug hints are enabled.【F:engine/Viewport3D.cpp†L3943-L3978】

## Camera Basis and Matrices
- Camera basis vectors follow the world convention: +Y is the reference up axis, the right vector is derived from `forward × up`, and the forward vector points roughly along **−Z** in world space (negative because looking “forward” down −Z keeps the system right-handed).【F:engine/Camera.cpp†L99-L158】
- The view matrix is constructed in column-major order with columns representing right, up, and **−forward**, and the translation computed by dotting these basis vectors with the camera position. This layout matches the OpenGL expectation for multiplying column vectors on the right.【F:engine/Camera.cpp†L161-L171】
- Perspective projection matrices clamp the field of view to safe limits and follow the standard OpenGL clip-space layout (column-major, `proj[11] = -1`, depth range determined by near/far planes).【F:engine/Camera.cpp†L174-L193】

## Rendering Pipeline Expectations
- Instanced draw submissions pass combined view-projection matrices to shaders via the `uViewProjection` uniform. Data comes directly from `glm::mat4` storage, so matrices must remain column-major and ready for multiplication in the shader as `uViewProjection * vec4(position, 1)`.【F:engine/graphics/InstancedMeshRenderer.cpp†L25-L108】
- Each instance buffer packs the model matrix (four `vec4` attributes) followed by color tint and a custom scalar, so authored transforms should align with this structure when populating instanced draws.【F:engine/graphics/InstancedMeshRenderer.cpp†L25-L108】

## Practical Tips
- When authoring new debug overlays or gameplay systems, align gizmos and UI cues with the established axis colors (X=red, Y=green, Z=blue) to preserve consistency with both the world-axis helper and the HUD mini-gizmo.【F:engine/Viewport3D.cpp†L2916-L2956】【F:engine/Viewport3D.cpp†L3943-L3978】
- When composing model matrices, keep in mind the camera’s forward is −Z; placing objects “ahead” of the camera typically means increasing their world-space **−Z** relative to the camera position.【F:engine/Camera.cpp†L99-L171】
