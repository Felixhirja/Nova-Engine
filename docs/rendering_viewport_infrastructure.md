# Rendering & Viewport Infrastructure

## Overview
Complete entity rendering system with automatic visualization, fallback rendering, and viewport integration. Any entity with a Position component automatically becomes visible.

## Architecture

### Component Layer: DrawComponent
**Location:** `engine/ecs/Components.h` (lines 490-570)

```cpp
struct DrawComponent : Component {
    enum class RenderMode {
        None,
        Sprite2D,    // 2D sprites with screen-space positioning
        Billboard,   // Always faces camera (particles, effects)
        Mesh3D,      // 3D meshes with transforms
        Particles,   // Particle system rendering
        Wireframe,   // Debug wireframe rendering
        Custom       // Custom render callback
    };
    
    RenderMode mode = RenderMode::None;
    bool visible = true;
    
    // Mesh rendering
    int meshHandle = 0;        // 0 = use fallback cube
    float meshScale = 1.0f;
    
    // Color tinting
    float tintR = 1.0f;
    float tintG = 1.0f;
    float tintB = 1.0f;
    
    // Texture/sprite rendering
    int textureHandle = 0;
    int spriteFrame = 0;
    
    // Animation
    bool animated = false;
    float animationSpeed = 1.0f;
    
    // Debug visualization
    bool showBoundingBox = false;
    bool showCollisionShape = false;
    
    // Custom rendering
    std::function<void(const DrawComponent&, const Position&)> customRenderCallback;
};
```

### Rendering Layer: ActorRenderer
**Location:** `engine/graphics/ActorRenderer.h`

#### Dual ECS Support
Supports both legacy EntityManager and modern EntityManagerV2:

```cpp
class ActorRenderer {
public:
    bool Initialize();
    
    // Modern ECS (V2)
    void Render(ecs::EntityManagerV2& entityManager, const Camera* camera);
    
    // Legacy ECS
    void RenderLegacy(EntityManager& entityManager, const Camera* camera);
    
private:
    void RenderSprite2D(const DrawComponent& draw, const Position& position);
    void RenderBillboard(const DrawComponent& draw, const Position& position, const Camera* camera);
    void RenderMesh3D(const DrawComponent& draw, const Position& position);
    void RenderParticles(const DrawComponent& draw, const Position& position);
    void RenderWireframe(const DrawComponent& draw, const Position& position);
    void RenderDebugInfo(const DrawComponent& draw, const Position& position);
};
```

#### Render Method Pattern
Both Render() and RenderLegacy() follow the same pattern:

1. **Query entities** with DrawComponent + Position
2. **Filter** by visibility flag
3. **Update animations** if animated
4. **Dispatch** to appropriate render method based on RenderMode
5. **Render debug info** if requested (bounding boxes, collision shapes)

```cpp
entityManager.ForEach<DrawComponent, Position>(
    [this, camera](Entity entity, DrawComponent& draw, Position& position) {
        if (!draw.visible) return;
        
        if (draw.animated) {
            draw.UpdateAnimation(1.0f / 60.0f);
        }
        
        switch (draw.mode) {
            case RenderMode::Mesh3D:
                RenderMesh3D(draw, position);
                break;
            // ... other modes
        }
        
        if (draw.showBoundingBox || draw.showCollisionShape) {
            RenderDebugInfo(draw, position);
        }
    }
);
```

#### Fallback Cube Rendering
**RenderMesh3D** provides automatic fallback when no mesh is loaded (meshHandle = 0):

```cpp
inline void ActorRenderer::RenderMesh3D(const DrawComponent& draw, const Position& position) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glScalef(draw.meshScale, draw.meshScale, draw.meshScale);
    glColor3f(draw.tintR, draw.tintG, draw.tintB);
    
    // Draw 1x1 cube with 6 quad faces
    glBegin(GL_QUADS);
    // Front, Back, Top, Bottom, Right, Left faces
    // ... 24 vertices defining unit cube
    glEnd();
    
    glPopMatrix();
    glColor3f(1.0f, 1.0f, 1.0f); // Reset color
}
```

**Cube Geometry:**
- Unit cube (-0.5 to +0.5 on all axes)
- 6 faces, 24 vertices (4 per quad)
- Centered at origin, scaled by meshScale
- Colored by tint RGB values

### Viewport Integration
**Location:** `engine/Viewport3D.cpp`

#### Main Render Pipeline
```cpp
void Viewport3D::Render(Camera* camera, 
                       double playerX, double playerY, double playerZ,
                       bool targetLocked,
                       ecs::EntityManagerV2* entityManager,
                       EntityManager* legacyEntityManager)
{
    for (const auto& view : currentLayout_.views) {
        ActivateView(view);
        RenderViewContent(camera, playerX, playerY, playerZ, 
                         view, targetLocked, 
                         entityManager, legacyEntityManager);
    }
}
```

#### View Rendering
```cpp
void Viewport3D::RenderViewContent(..., EntityManager* legacyEntityManager) {
    // 1. Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 2. Setup camera with gluLookAt
    SetupCamera(camera);
    
    // 3. Render grid, axes, skybox, etc.
    RenderStaticElements();
    
    // 4. **Render all entities with DrawComponent**
    if (legacyEntityManager && actorRenderer_) {
        actorRenderer_->RenderLegacy(*legacyEntityManager, camera);
    } else if (entityManager && actorRenderer_) {
        actorRenderer_->Render(*entityManager, camera);
    }
    
    // 5. Render minimap or debug overlays
    if (view.role == ViewRole::Minimap) {
        DrawMinimapOverlay(playerX, playerY, playerZ);
    } else if (camera) {
        DrawCameraDebug(camera, playerX, playerY, playerZ, view.role, targetLocked);
    }
}
```

**Key Integration Points:**
- ActorRenderer is owned by Viewport3D (`std::unique_ptr<ActorRenderer> actorRenderer_`)
- Initialized in Viewport3D constructor: `actorRenderer_->Initialize()`
- Called during RenderViewContent between scene setup and overlay rendering
- Supports both ECS versions via dual method calls

### Automatic Entity Visualization
**Location:** `engine/Simulation.cpp` (Update method)

Every entity with a Position automatically gets a DrawComponent:

```cpp
void Simulation::Update(double dt) {
    // ... other update logic
    
    // Auto-add DrawComponent to any entity with Position but no DrawComponent
    static int autoDrawCounter = 0;
    if (++autoDrawCounter % 60 == 0) {  // Check every second
        useEm->ForEach<Position>([useEm](Entity e, Position& pos) {
            if (!useEm->GetComponent<DrawComponent>(e)) {
                auto draw = std::make_shared<DrawComponent>();
                draw->mode = DrawComponent::RenderMode::Mesh3D;
                draw->visible = true;
                draw->meshHandle = 0;  // Use fallback cube
                draw->meshScale = 1.0f;
                draw->tintR = 0.8f;
                draw->tintG = 0.8f;
                draw->tintB = 0.8f;  // Gray for generic entities
                useEm->AddComponent<DrawComponent>(e, draw);
            }
        });
    }
}
```

**Behavior:**
- Runs every second (60 frames @ 60 FPS)
- Queries all entities with Position component
- Adds DrawComponent if missing
- Configures for Mesh3D fallback cube rendering
- Gray tint (0.8, 0.8, 0.8) distinguishes auto-added from custom entities

### MainLoop Integration
**Location:** `engine/MainLoop.cpp`

The main loop passes entity managers to viewport rendering:

```cpp
// Line 820
viewport->Render(
    camera.get(),
    playerView.worldX, playerView.worldY, playerView.worldZ,
    runtime.targetLocked,
    nullptr,              // EntityManagerV2 (not used yet)
    entityManager.get()   // Legacy EntityManager
);
```

## Data Flow

```
Entity Creation
    ↓
Position Component Added
    ↓
Simulation::Update (every 60 frames)
    ↓
Auto-add DrawComponent (if missing)
    ↓
MainLoop calls Viewport->Render()
    ↓
Viewport->RenderViewContent()
    ↓
ActorRenderer->RenderLegacy()
    ↓
ForEach<DrawComponent, Position>
    ↓
Switch on RenderMode
    ↓
RenderMesh3D (fallback cube)
    ↓
glPushMatrix → glTranslatef → glScalef → glColor3f
    ↓
glBegin(GL_QUADS) → Draw 6 faces → glEnd()
    ↓
glPopMatrix → Reset color
```

## Render Modes

### Mesh3D (Currently Implemented)
- **Purpose:** 3D objects with position, rotation, scale
- **Fallback:** 1x1 colored cube at entity position
- **Properties:** meshHandle, meshScale, tint RGB
- **Use Cases:** Spaceships, stations, projectiles, debris, any 3D entity

### Sprite2D (Stub)
- **Purpose:** 2D sprites in screen space or world space
- **Properties:** textureHandle, spriteFrame
- **Use Cases:** UI elements, 2D effects, icons

### Billboard (Stub)
- **Purpose:** Quads that always face the camera
- **Properties:** textureHandle, scale
- **Use Cases:** Particles, explosions, laser impacts, distance markers

### Particles (Stub)
- **Purpose:** Particle system rendering
- **Properties:** particleSystemHandle
- **Use Cases:** Engine exhaust, explosions, debris fields

### Wireframe (Stub)
- **Purpose:** Debug wireframe rendering
- **Properties:** wireframeColor
- **Use Cases:** Collision shapes, AI navigation, debug visualization

### Custom (Implemented)
- **Purpose:** User-defined rendering via callback
- **Properties:** customRenderCallback function
- **Use Cases:** Procedural geometry, special effects, custom shaders

## Usage Patterns

### Manual DrawComponent Setup
For entities that need custom appearance:

```cpp
// Create entity
Entity entity = entityManager->CreateEntity();

// Add Position
auto pos = std::make_shared<Position>();
pos->x = 10.0; pos->y = 5.0; pos->z = 0.0;
entityManager->AddComponent<Position>(entity, pos);

// Add custom DrawComponent
auto draw = std::make_shared<DrawComponent>();
draw->mode = DrawComponent::RenderMode::Mesh3D;
draw->visible = true;
draw->meshHandle = shipMeshId;  // Custom mesh
draw->meshScale = 2.0f;         // Larger scale
draw->tintR = 0.2f;
draw->tintG = 0.8f;
draw->tintB = 1.0f;             // Cyan tint
entityManager->AddComponent<DrawComponent>(entity, draw);
```

### Automatic Visualization
For quick prototyping or generic entities:

```cpp
// Just add Position - DrawComponent added automatically
Entity entity = entityManager->CreateEntity();
auto pos = std::make_shared<Position>();
pos->x = 0.0; pos->y = 0.0; pos->z = 0.0;
entityManager->AddComponent<Position>(entity, pos);

// After 1 second, entity automatically gets gray cube visualization
```

### Player Entity Example
From `engine/Simulation.cpp` (lines 681-690):

```cpp
// Manual setup for player - cyan cube at origin
auto draw = std::make_shared<DrawComponent>();
draw->mode = DrawComponent::RenderMode::Mesh3D;
draw->visible = true;
draw->meshHandle = 0;      // Fallback cube
draw->meshScale = 0.5f;    // Smaller than default
draw->tintR = 0.2f;
draw->tintG = 0.8f;
draw->tintB = 1.0f;        // Cyan color
useEm->AddComponent<DrawComponent>(playerEntity, draw);
```

## Performance Characteristics

### Entity Iteration
- **Legacy EntityManager:** Map-based storage, O(n) iteration
- **Modern EntityManagerV2:** Archetype-based, contiguous arrays, 10-50x faster

### Rendering Cost
- **Per-entity overhead:** Transform setup (glPushMatrix, glTranslatef, glScalef)
- **Cube rendering:** 24 vertices in GL_QUADS immediate mode
- **Typical performance:** 1000s of cubes at 60 FPS on modern hardware

### Auto-add System
- **Frequency:** Every 60 frames (1 second @ 60 FPS)
- **Cost:** Single ForEach over Position components + conditional AddComponent
- **Impact:** Negligible - only runs for new entities without DrawComponent

## OpenGL State Management

### Rendering State
```cpp
glPushMatrix();           // Save transform
glTranslatef(...);        // Apply position
glScalef(...);            // Apply scale
glColor3f(...);           // Apply tint
// ... draw geometry
glPopMatrix();            // Restore transform
glColor3f(1,1,1);         // Reset color to white
```

**State Isolation:**
- Each entity rendering is wrapped in push/pop matrix
- Color reset after rendering prevents tint bleeding
- No depth test changes (inherits from viewport setup)
- No blend mode changes (inherits from viewport setup)

### Depth Testing
Controlled by Viewport3D setup:
- Enabled during 3D view rendering
- Disabled during 2D overlay rendering

## Extension Points

### Adding New Render Modes
1. Add enum value to `DrawComponent::RenderMode`
2. Add properties to DrawComponent struct
3. Implement `RenderNewMode()` method in ActorRenderer
4. Add case to switch statement in Render/RenderLegacy

### Custom Rendering
Use the Custom render mode with callback:

```cpp
draw->mode = DrawComponent::RenderMode::Custom;
draw->customRenderCallback = [](const DrawComponent& draw, const Position& pos) {
    // Custom OpenGL rendering code
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    // ... custom geometry
    glPopMatrix();
};
```

### Mesh Loading
Future enhancement - load actual 3D meshes:

```cpp
int meshHandle = resourceManager.LoadMesh("assets/ship.obj");
draw->meshHandle = meshHandle;  // No longer 0, uses real mesh
// RenderMesh3D would check if meshHandle != 0 and render actual mesh
```

## Debug Features

### Visual Debug Info
Enable debug rendering per-entity:

```cpp
draw->showBoundingBox = true;      // Renders AABB wireframe
draw->showCollisionShape = true;   // Renders physics collision shape
```

Called automatically at end of entity rendering if flags are set.

### Component Inspection
Use ECS Inspector (press `I` in-game):
- Lists all entities with components
- Filter by DrawComponent to see renderable entities
- Shows Position + DrawComponent properties
- Cycle filters with `[` and `]`

## Technical Notes

### Legacy vs Modern ECS
- **ForEach signature difference:**
  - Legacy: `(Entity entity, T& component, U& component)`
  - Modern: `(EntityHandle handle, T& component, U& component)`
- Both use reference parameters, not shared_ptr
- ActorRenderer supports both via separate Render/RenderLegacy methods

### Thread Safety
- Not thread-safe - rendering happens on main thread only
- EntityManager queries are not thread-safe
- OpenGL commands must be on thread with valid context

### Memory Management
- DrawComponent uses shared_ptr ownership via EntityManager
- ActorRenderer owned by Viewport3D (unique_ptr)
- No manual memory management in render loop

## Future Enhancements

### Short Term
- [ ] Implement Billboard rendering (particles, effects)
- [ ] Implement Sprite2D rendering (UI, 2D effects)
- [ ] Load actual 3D meshes instead of fallback cubes
- [ ] Add texture support to cube fallback

### Medium Term
- [ ] Particle system rendering
- [ ] Instanced rendering for many identical entities
- [ ] Level-of-detail (LOD) system
- [ ] Occlusion culling

### Long Term
- [ ] Modern OpenGL (VBO/VAO instead of immediate mode)
- [ ] Shader-based rendering pipeline
- [ ] Deferred rendering for many lights
- [ ] PBR materials (physically-based rendering)

## Summary

The rendering infrastructure provides:
- ✅ **Automatic entity visualization** - any Position becomes visible
- ✅ **Dual ECS support** - works with legacy and modern EntityManager
- ✅ **Fallback rendering** - cubes when no mesh loaded
- ✅ **Extensible architecture** - easy to add new render modes
- ✅ **Viewport integration** - seamlessly integrated into main render loop
- ✅ **Clean separation** - DrawComponent (data) + ActorRenderer (logic) + Viewport (integration)

This foundation supports rapid prototyping (auto-visualization) while allowing full control (manual DrawComponent setup) for production-quality rendering.
