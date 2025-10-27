# Actor Rendering System

This document explains how to make actors visible in the Nova Engine using the DrawComponent system.

## Overview

Actors are made visible by attaching a `DrawComponent` to their entity. The `DrawComponent` supports multiple rendering modes and can be customized for different visual appearances.

## Basic Setup

### 1. Add DrawComponent to Actor Initialization

In your actor's `Initialize()` method, add a `DrawComponent`:

```cpp
// In your actor's Initialize method
context_.entityManager->AddComponent<DrawComponent>(context_.entity);
if (auto* draw = context_.GetComponent<DrawComponent>()) {
    draw->mode = DrawComponent::RenderMode::Mesh3D;  // Choose render mode
    draw->visible = true;
    draw->renderLayer = 1;  // Rendering order
    // ... other properties
}
```

### 2. Configure Rendering Properties

#### Common Properties (all modes):
- `visible`: Whether the actor is rendered
- `renderLayer`: Drawing order (higher = drawn later)
- `opacity`: Transparency (0.0 = invisible, 1.0 = opaque)
- `tintR/G/B`: Color tinting

#### Mesh3D Mode (for 3D models):
```cpp
draw->mode = DrawComponent::RenderMode::Mesh3D;
draw->meshHandle = spaceshipMeshId;  // Your 3D model
draw->meshScale = 1.0f;             // Uniform scale
draw->materialHandle = materialId;   // Shader/material
```

#### Billboard Mode (for projectiles/effects):
```cpp
draw->mode = DrawComponent::RenderMode::Billboard;
draw->textureHandle = projectileTextureId;
draw->spriteScale = 0.5f;  // Size
```

#### Sprite2D Mode (for 2D elements):
```cpp
draw->mode = DrawComponent::RenderMode::Sprite2D;
draw->textureHandle = spriteTextureId;
draw->spriteFrame = currentFrame;
```

## Render Modes

### Mesh3D
- Full 3D model rendering
- Supports materials, lighting, shadows
- Best for spaceships, stations, complex objects

### Billboard
- 2D sprite that always faces camera
- Efficient for particles, projectiles
- Automatic orientation

### Sprite2D
- Traditional 2D sprite rendering
- Fixed screen orientation
- Good for UI elements, simple graphics

### Particles
- Particle system rendering
- For effects like engine trails, explosions

### Wireframe
- Debug wireframe rendering
- Shows collision shapes, bounding boxes

### Custom
- Use custom rendering callback
- For specialized rendering needs

## Animation

The DrawComponent supports sprite animation:

```cpp
// Start an animation
draw->StartAnimation(0, 15, true);  // Frames 0-15, looping

// In your update loop
draw->UpdateAnimation(deltaTime);
```

## ActorRenderer Integration

To render all actors, use the `ActorRenderer` system:

```cpp
#include "graphics/ActorRenderer.h"

ActorRenderer renderer;
renderer.Initialize();

// In your render loop
renderer.Render(entityManager, camera);
```

## Examples

### Spaceship Actor
```cpp
if (auto* draw = context_.GetComponent<DrawComponent>()) {
    draw->mode = DrawComponent::RenderMode::Mesh3D;
    draw->meshHandle = spaceshipMeshId;
    draw->meshScale = 1.0f;
    draw->renderLayer = 1;
    draw->castShadows = true;
}
```

### Station Actor
```cpp
if (auto* draw = context_.GetComponent<DrawComponent>()) {
    draw->mode = DrawComponent::RenderMode::Mesh3D;
    draw->meshHandle = stationMeshId;
    draw->meshScaleX = 3.0f;  // Wide
    draw->meshScaleY = 2.0f;  // Tall
    draw->meshScaleZ = 3.0f;  // Deep
    draw->tintR = 0.8f;       // Blue-gray tint
    draw->tintG = 0.8f;
    draw->tintB = 0.9f;
}
```

### Projectile Actor
```cpp
if (auto* draw = context_.GetComponent<DrawComponent>()) {
    draw->mode = DrawComponent::RenderMode::Billboard;
    draw->textureHandle = projectileTextureId;
    draw->spriteScale = 0.5f;
    draw->renderLayer = 2;  // Above ships
    draw->tintR = 1.0f;     // Bright
    draw->tintG = 1.0f;
    draw->tintB = 0.5f;     // Blue tint
}
```

## Performance Tips

1. Use appropriate render modes (billboards for simple objects)
2. Set `visible = false` for off-screen actors
3. Use LOD (Level of Detail) for distant objects
4. Group similar objects for instanced rendering
5. Use lower render layers for background objects

## Debug Rendering

Enable debug visualization:

```cpp
draw->showBoundingBox = true;
draw->showCollisionShape = true;
draw->debugColorR = 1.0f;  // Red debug color
```