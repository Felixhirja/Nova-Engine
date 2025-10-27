# Actor Creation Guide

This guide explains how to create new actors in the Nova Engine. Actors are game entities that represent ships, stations, projectiles, and other interactive objects in the game world.

## Overview

The Nova Engine uses an automatic actor registration system where actors register themselves when their header files are included. This makes it easy to add new actor types without modifying central registration code.

## Basic Actor Structure

All actors must inherit from the `IActor` interface and implement the required methods. Most game-specific behavior (movement, AI, rendering) should be implemented via ECS components and systems. If your branch includes higher-level bases like `Spaceship` or `NPC`, you can inherit from those for convenience; otherwise, start from `IActor` and attach the components you need.

### Minimal Actor Example

```cpp
#pragma once

#include "../engine/Actor.h"
#include "../engine/ActorRegistry.h"

// Minimal actor that satisfies the IActor interface
class MyActor : public IActor {
public:
    MyActor() = default;
    ~MyActor() override = default;

    // IActor interface
    std::string_view TypeName() const override { return "MyActor"; }
    bool IsBound() const override { return context_.HasEntity(); }
    ecs::EntityHandle GetEntity() const override { return context_.entity; }
    void AttachContext(const ActorContext& context) override { context_ = context; }
    const ActorContext& Context() const override { return context_; }

    // Optional convenience: bind an entity after creation
    void BindEntity(ecs::EntityHandle e) { context_.entity = e; }

private:
    ActorContext context_{};
};

// Automatic registration
REGISTER_ACTOR(MyActor, "MyActor")
Actors are constructed by the registry using a provided `ActorContext`. Typically you:

1) Create an entity in ECS V2, 2) Build a context with `entityManager` and `entity`, 3) Ask the registry to create your actor.

```cpp
#include "engine/ActorRegistry.h"
#include "engine/ecs/EntityManagerV2.h"

ecs::EntityManagerV2 em;
ecs::EntityHandle entity = em.CreateEntity();

ActorContext ctx;
ctx.entityManager = &em;
ctx.entity = entity;           // actor will be considered bound
ctx.debugName = "my_actor";

auto actor = ActorRegistry::Instance().Create("MyActor", ctx);
// actor->Context() now references the same ECS world and entity
```


If you create the actor before the entity exists, you can later bind it with a helper on your actor (like `BindEntity`) that sets `context_.entity`.


To render an actor, attach a `DrawComponent` to its entity and configure the mode. See `actor_rendering_guide.md` for full options.


```cpp
#include "engine/ecs/Components.h"  // Position, DrawComponent, etc.


// Attach components on creation
auto& pos = em.AddComponent<Position>(entity);
pos.x = 0.0; pos.y = 0.0; pos.z = 0.0;

auto& draw = em.AddComponent<DrawComponent>(entity);
draw.mode = DrawComponent::RenderMode::Sprite2D;  // or Billboard, Mesh3D, etc.
draw.visible = true;
draw.renderLayer = 1;
```

From inside your actor, you can also access components via the context:

```cpp
// e.g., in a custom Initialize() or method you define
if (auto* draw = Context().GetComponent<DrawComponent>()) {
    draw->mode = DrawComponent::RenderMode::Billboard;
    draw->tintR = 1.0f; draw->tintG = 1.0f; draw->tintB = 1.0f;
}
```cpp
// Add baseline movement components
em.AddComponent<Position>(entity);
em.AddComponent<Velocity>(entity);

// Later, e.g., inside a method on your actor
if (auto* pos = Context().GetComponent<Position>()) {
    pos->x += 10.0; // move along X
}
```

## Spaceship-Based Actors

For actors that represent spaceships (ships, stations, projectiles), inherit from `Spaceship` or `NPC`:

Note: Depending on your branch/state, these higher-level bases may not be present yet. If they’re unavailable, use the minimal `IActor` approach above and attach the components you need (flight model, AI, DrawComponent) directly.

### Spaceship Actor Example

```cpp
#pragma once

#include "Spaceship.h"
#include "../engine/ActorRegistry.h"

class MySpaceship : public Spaceship {
public:
    MySpaceship();
    ~MySpaceship() override = default;

    // IActor interface override
    std::string_view TypeName() const override { return "MySpaceship"; }

    // Optional: Override Spaceship methods
    void Update(float deltaTime) override;
    void Render() override;
};

// Automatic registration
REGISTER_ACTOR(MySpaceship, "MySpaceship")

// Inline implementations
inline MySpaceship::MySpaceship() : Spaceship() {
    // Constructor logic
}

inline void MySpaceship::Update(float deltaTime) {
    Spaceship::Update(deltaTime);
    // Additional update logic
}
inline void MySpaceship::Render() {
    Spaceship::Render();
    // Additional render logic
}
```

NPC actors (like traders, pirates, patrols) inherit from `NPC` and get automatic AI behavior:


```cpp
#pragma once

#include "NPC.h"
#include "../engine/ActorRegistry.h"

class MyNPC : public NPC {
public:
    MyNPC();
    ~MyNPC() override = default;

    // IActor interface override
    std::string_view TypeName() const override { return "MyNPC"; }

protected:
    // Override AI methods
    void UpdateAI(float deltaTime) override;
    void MakeAIDecision() override;
    void ExecuteAIBehavior(float deltaTime) override;
};

// Automatic registration
REGISTER_ACTOR(MyNPC, "MyNPC")

// Inline implementations
inline MyNPC::MyNPC() : NPC(NPCType::Trader) {
    // Set NPC characteristics
    SetAggressionLevel(0.5f);
    SetCautionLevel(0.7f);
}

inline void MyNPC::UpdateAI(float deltaTime) {
    NPC::UpdateAI(deltaTime);
    // Custom AI logic
}

inline void MyNPC::MakeAIDecision() {
    // Custom decision making
}

inline void MyNPC::ExecuteAIBehavior(float deltaTime) {
    // Custom behavior execution
}
```

## Station Actor Example

Stations are special NPCs that don't move and provide services:

```cpp
#pragma once

#include "NPC.h"
#include "../engine/ActorRegistry.h"

class MyStation : public NPC {
public:
    MyStation();
    ~MyStation() override = default;

    // IActor interface override
    std::string_view TypeName() const override { return "MyStation"; }

    // Override Initialize for custom setup
    bool Initialize(SpaceshipClassType classType, int loadoutIndex = 0);

protected:
    void UpdateAI(float deltaTime) override;
    void MakeAIDecision() override;
    void ExecuteAIBehavior(float deltaTime) override;

private:
    float serviceTimer_ = 0.0f;
};

// Automatic registration
REGISTER_ACTOR(MyStation, "MyStation")

// Inline implementations
inline MyStation::MyStation() : NPC(NPCType::Trader) {
    // Stations are peaceful and stationary
    SetAggressionLevel(0.1f);
    SetCautionLevel(0.9f);
}

inline bool MyStation::Initialize(SpaceshipClassType classType, int loadoutIndex) {
    // Call parent Initialize first
    if (!NPC::Initialize(classType, loadoutIndex)) {
        return false;
    }

    // Custom station setup
    return true;
}

inline void MyStation::UpdateAI(float deltaTime) {
    NPC::UpdateAI(deltaTime);
    serviceTimer_ += deltaTime;
}

inline void MyStation::MakeAIDecision() {
    // Station logic - stay in place, provide services
    if (GetAIState() == AIState::Idle) {
        SetAIState(AIState::Docked);
    }
}

inline void MyStation::ExecuteAIBehavior(float deltaTime) {
    switch (GetAIState()) {
        case AIState::Docked:
            // Provide services to nearby ships
            throttle_ = 0.0f; // Stations don't move
            break;
        default:
            NPC::ExecuteAIBehavior(deltaTime);
            break;
    }
}
```

## Projectile Actor Example

Projectiles are simple actors that move in a straight line:

```cpp
#pragma once

#include "Projectile.h"
#include "../engine/ActorRegistry.h"

class MyProjectile : public ProjectileActor {
public:
    MyProjectile();
    ~MyProjectile() override = default;

    // IActor interface override
    std::string_view TypeName() const override { return "MyProjectile"; }

    // Optional: Override for custom behavior
    void Update(float deltaTime) override;
};

// Automatic registration
REGISTER_ACTOR(MyProjectile, "MyProjectile")

// Inline implementations
inline MyProjectile::MyProjectile() : ProjectileActor() {
    // Set projectile properties
    SetDamage(50.0f);
    SetLifetime(5.0f);
}

inline void MyProjectile::Update(float deltaTime) {
    ProjectileActor::Update(deltaTime);
    // Custom projectile logic
}
```

## Registration Requirements

### Required Includes

```cpp
#include "../engine/ActorRegistry.h"  // For REGISTER_ACTOR macro
```

Include the appropriate base class header:

- `"../engine/Actor.h"` for basic actors
- `"Spaceship.h"` for spaceship actors
- `"NPC.h"` for NPC actors
- `"Projectile.h"` for projectile actors

### REGISTER_ACTOR Macro

Place this macro **after** the class definition but **before** inline implementations:

```cpp
class MyActor : public BaseClass {
    // Class definition
};

// Automatic registration - MUST be here
REGISTER_ACTOR(MyActor, "MyActor")

// Inline implementations
inline MyActor::MyActor() {
    // Constructor
}
```

**Important:** The registration name should match the class name and be unique across all actors.

## File Location

Place new actor files in the `actors/` directory with the naming convention:

- `MyActor.h` for the header file
- `MyActor.cpp` for implementation file (if needed)

## Build System Integration

The build system automatically detects new actor headers and includes them in the build. No additional configuration is needed.

## Testing New Actors

After creating a new actor:

1. Include the header in your test file or main game code
2. Use the ActorRegistry to create instances:

```cpp
#include "actors/MyActor.h"

// Create an actor
auto actor = ActorRegistry::Instance().Create("MyActor", context);
```

1. Verify the actor behaves as expected in your game logic

## Best Practices

- **Keep it simple**: Start with basic functionality and add features incrementally
- **Use appropriate base classes**: Choose the right base class for your actor type
- **Override methods thoughtfully**: Only override methods when you need custom behavior
- **Test thoroughly**: Verify your actor works in different game scenarios
- **Follow naming conventions**: Use descriptive names that match the class name
- **Document your actors**: Add comments explaining special behavior or requirements

## Common Patterns

### Custom Initialization

Override `Initialize()` for spaceship-based actors to set up custom properties:

```cpp
bool Initialize(SpaceshipClassType classType, int loadoutIndex = 0) override {
    if (!Spaceship::Initialize(classType, loadoutIndex)) {
        return false;
    }
    // Custom setup
    return true;
}
```

### Custom AI Behavior

For NPCs, override the AI methods:

```cpp
void MakeAIDecision() override {
    // Custom decision logic
}

void ExecuteAIBehavior(float deltaTime) override {
    // Custom behavior execution
}
```

### Event Handling

Actors can respond to game events by checking conditions in `Update()`:

```cpp
void Update(float deltaTime) override {
    BaseClass::Update(deltaTime);

    // Check for events and respond
    if (someCondition) {
        // Handle event
    }
}
```

## Troubleshooting

### Compilation Errors

- **"REGISTER_ACTOR not found"**: Make sure to include `ActorRegistry.h`
- **"No matching function"**: Check that your constructor signature matches the base class
- **"Multiple definition"**: Ensure REGISTER_ACTOR appears only once per class

### Runtime Issues

- **Actor not found**: Verify the registration name matches exactly
- **Wrong behavior**: Check that you're calling the right base class methods
- **Memory leaks**: Ensure proper cleanup in destructors

## Advanced Topics

- **Component Integration**: Actors can use ECS components for additional functionality
- **Rendering**: See `actor_rendering_guide.md` for visual representation
- **Physics**: Actors automatically integrate with the physics system
- **Serialization**: Actors can be saved/loaded using the serialization system
