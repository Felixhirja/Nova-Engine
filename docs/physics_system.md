# Physics System Documentation

## Overview

The Nova Engine physics system provides a comprehensive ECS-based physics simulation with support for rigid bodies, collisions, forces, and constraints. It's designed to be modular, performant, and easy to integrate into gameplay code.

## Core Components

### RigidBody

Represents a physics-simulated object with mass, velocity, and response to forces.

```cpp
struct RigidBody : public Component {
    double mass = 1.0;
    double inverseMass = 1.0;  // Cached for performance
    double restitution = 0.5;  // Bounciness (0-1)
    double friction = 0.5;     // Surface friction (0-1)
    double linearDamping = 0.01;
    double angularDamping = 0.01;
    
    // Angular velocity for rotation
    double angularVelocityX/Y/Z = 0.0;
    
    // Orientation (Euler angles)
    double rotationX/Y/Z = 0.0;
    
    bool isKinematic = false;  // Not affected by physics
    bool useGravity = true;
    bool freezePositionX/Y/Z = false;
    bool freezeRotationX/Y/Z = false;
};
```

**Usage:**
```cpp
auto entity = entityManager->CreateEntity();
auto& rb = entityManager->EmplaceComponent<RigidBody>(entity);
rb.SetMass(10.0);
rb.restitution = 0.8;  // Bouncy
rb.useGravity = true;
```

### Colliders

Colliders define the shape for collision detection.

#### BoxCollider
```cpp
BoxCollider box;
box.width = 2.0;
box.height = 1.0;
box.depth = 1.0;
box.offsetX = 0.0;  // Local offset from position
```

#### SphereCollider
```cpp
SphereCollider sphere;
sphere.radius = 1.0;
```

#### CapsuleCollider
```cpp
CapsuleCollider capsule;
capsule.radius = 0.5;
capsule.height = 2.0;
capsule.direction = CapsuleCollider::Direction::Y;  // For characters
```

**Collision Layers:**
```cpp
collider.collisionLayer = 1;        // Bitmask: which layer this is on
collider.collisionMask = 0xFFFFFFFF; // Bitmask: which layers it collides with
collider.isTrigger = false;          // If true, no physical response
```

### Force

Represents a force applied to a RigidBody.

```cpp
Force force;
force.fx = 100.0;  // Force in Newtons
force.fy = 0.0;
force.fz = 0.0;
force.mode = Force::Mode::Force;  // or Impulse, Acceleration, VelocityChange
force.lifetime = -1.0;  // -1 = permanent, 0 = one-shot, >0 = duration
```

**Force Modes:**
- **Force**: Continuous force (F = ma), applied every frame
- **Impulse**: Instantaneous velocity change, applied once
- **Acceleration**: Direct acceleration (ignores mass)
- **VelocityChange**: Direct velocity change (ignores mass)

### CollisionInfo

Stores information about collisions detected this frame.

```cpp
auto* collision = entityManager->GetComponent<CollisionInfo>(entity);
if (collision && collision->collisionCount > 0) {
    for (const auto& contact : collision->contacts) {
        std::cout << "Collided with entity " << contact.otherEntity << std::endl;
        std::cout << "Penetration: " << contact.penetrationDepth << std::endl;
        std::cout << "Normal: " << contact.normalX << ", " 
                  << contact.normalY << ", " << contact.normalZ << std::endl;
    }
}
```

### GravitySource

Creates a gravitational field that affects nearby RigidBodies.

```cpp
// Planet-like gravity (inverse square law)
GravitySource gravity;
gravity.strength = 9.8;
gravity.radius = 100.0;
gravity.isUniform = false;  // Point gravity

// Earth-like uniform gravity
GravitySource uniformGravity;
uniformGravity.strength = 9.8;
uniformGravity.isUniform = true;
uniformGravity.directionX = 0.0;
uniformGravity.directionY = 0.0;
uniformGravity.directionZ = -1.0;
```

### ConstantForce

Applies a continuous force every physics update (like a thruster or wind).

```cpp
ConstantForce thruster;
thruster.forceX = 0.0;
thruster.forceY = 100.0;  // Forward thrust
thruster.forceZ = 0.0;
thruster.isLocalSpace = true;  // Force in object's local space
```

### CharacterController

Specialized physics for player/NPC characters with ground detection and movement.

```cpp
CharacterController controller;
controller.height = 2.0;
controller.radius = 0.5;
controller.moveSpeed = 5.0;
controller.jumpHeight = 1.5;
controller.stepOffset = 0.3;  // Can climb stairs
controller.slopeLimit = 45.0;  // Max walkable slope
```

### Joint

Connects two rigid bodies with constraints.

```cpp
Joint spring;
spring.type = Joint::Type::Spring;
spring.connectedEntity = otherEntity;
spring.springStrength = 100.0;
spring.springDamping = 10.0;
spring.breakable = true;
spring.breakForce = 500.0;
```

## PhysicsSystem

The PhysicsSystem manages all physics simulation.

### Initialization

```cpp
PhysicsSystem physics(&entityManager);
physics.SetGravity(0.0, 0.0, -9.8);  // Earth-like gravity
physics.SetCollisionEnabled(true);
physics.SetMaxVelocity(100.0);
```

### Update Loop

```cpp
// In your main game loop
double dt = 1.0 / 60.0;  // Fixed timestep
physics.Update(dt);
```

### Force Application

```cpp
// Apply continuous force
physics.ApplyForce(entity, 10.0, 0.0, 0.0);

// Apply instant impulse
physics.ApplyImpulse(entity, 50.0, 0.0, 0.0);

// Apply force at a point (generates torque)
physics.ApplyForceAtPoint(entity, 
    10.0, 0.0, 0.0,  // Force vector
    0.0, 1.0, 0.0);  // Point offset from center
```

### Collision Queries

```cpp
// Raycast
PhysicsSystem::RaycastHit hit;
if (physics.Raycast(0.0, 0.0, 0.0,  // Origin
                    1.0, 0.0, 0.0,  // Direction
                    100.0,           // Max distance
                    hit)) {
    std::cout << "Hit entity " << hit.entity << " at distance " << hit.distance << std::endl;
}

// Overlap sphere (find all entities in radius)
auto entities = physics.OverlapSphere(playerX, playerY, playerZ, 5.0);
for (auto entity : entities) {
    std::cout << "Found entity " << entity << " nearby" << std::endl;
}

// Overlap box
auto entities = physics.OverlapBox(x, y, z, width, height, depth, layerMask);
```

## Common Patterns

### Create a Dynamic Physics Object

```cpp
auto ball = entityManager.CreateEntity();

// Position & velocity
entityManager.EmplaceComponent<Position>(ball, Position{0.0, 0.0, 10.0});
entityManager.EmplaceComponent<Velocity>(ball);

// Physics
auto& rb = entityManager.EmplaceComponent<RigidBody>(ball);
rb.SetMass(1.0);
rb.restitution = 0.7;  // Bouncy ball

// Collision
auto& sphere = entityManager.EmplaceComponent<SphereCollider>(ball);
sphere.radius = 0.5;
```

### Create a Static Platform

```cpp
auto platform = entityManager.CreateEntity();
entityManager.EmplaceComponent<Position>(platform, Position{0.0, 0.0, 0.0});

auto& rb = entityManager.EmplaceComponent<RigidBody>(platform);
rb.isKinematic = true;  // Won't move

auto& box = entityManager.EmplaceComponent<BoxCollider>(platform);
box.width = 10.0;
box.height = 1.0;
box.depth = 10.0;
```

### Apply Jump Force

```cpp
void Jump(Entity player, PhysicsSystem& physics) {
    auto* rb = entityManager->GetComponent<RigidBody>(player);
    auto* vel = entityManager->GetComponent<Velocity>(player);
    
    if (rb && vel) {
        // Apply upward impulse
        physics.ApplyImpulse(player, 0.0, 0.0, 10.0);
    }
}
```

### Trigger Volume (No Physical Response)

```cpp
auto trigger = entityManager.CreateEntity();
entityManager.EmplaceComponent<Position>(trigger, Position{0.0, 0.0, 0.0});

auto& box = entityManager.EmplaceComponent<BoxCollider>(trigger);
box.width = 5.0;
box.height = 5.0;
box.depth = 5.0;
box.isTrigger = true;  // No collision response

// In game logic
auto* collision = entityManager.GetComponent<CollisionInfo>(player);
if (collision) {
    for (const auto& contact : collision->contacts) {
        if (contact.otherEntity == trigger) {
            std::cout << "Player entered trigger!" << std::endl;
        }
    }
}
```

### Explosion Force

```cpp
void Explode(double x, double y, double z, double radius, double force, 
             EntityManager& em, PhysicsSystem& physics) {
    auto nearbyEntities = physics.OverlapSphere(x, y, z, radius);
    
    for (auto entity : nearbyEntities) {
        auto* pos = em.GetComponent<Position>(entity);
        auto* rb = em.GetComponent<RigidBody>(entity);
        
        if (pos && rb && !rb->isKinematic) {
            // Calculate direction and distance
            double dx = pos->x - x;
            double dy = pos->y - y;
            double dz = pos->z - z;
            double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist > 0.001) {
                // Falloff with distance
                double strength = force * (1.0 - dist / radius);
                double nx = dx / dist;
                double ny = dy / dist;
                double nz = dz / dist;
                
                physics.ApplyImpulse(entity, 
                    nx * strength,
                    ny * strength,
                    nz * strength);
            }
        }
    }
}
```

## Performance Considerations

### Collision Detection Complexity

The system uses broad-phase and narrow-phase collision detection:
- **Broad Phase**: O(n²) for n colliders (can be optimized with spatial partitioning)
- **Narrow Phase**: Depends on shape complexity

**Optimization Tips:**
- Use simpler shapes when possible (sphere > capsule > box)
- Disable collision for static decorative objects
- Use collision layers to filter unnecessary checks
- Consider spatial partitioning for large scenes

### Fixed Timestep

Always use a fixed timestep for physics to ensure deterministic simulation:

```cpp
const double FIXED_DT = 1.0 / 60.0;  // 60 Hz
double accumulator = 0.0;

void GameLoop(double frameTime) {
    accumulator += frameTime;
    
    while (accumulator >= FIXED_DT) {
        physics.Update(FIXED_DT);
        accumulator -= FIXED_DT;
    }
    
    // Render with interpolation
    double alpha = accumulator / FIXED_DT;
    Render(alpha);
}
```

### Memory Usage

Typical component sizes:
- RigidBody: ~200 bytes
- BoxCollider: ~100 bytes
- CollisionInfo: Dynamic (grows with contacts)

For 1000 physics objects: ~300 KB

## Integration with Existing Systems

### With MovementSystem

```cpp
// Physics-based player movement
void UpdatePlayerMovement(Entity player, PhysicsSystem& physics, double dt) {
    auto* controller = em.GetComponent<PlayerController>(player);
    auto* rb = em.GetComponent<RigidBody>(player);
    
    if (controller && rb) {
        double forceX = 0.0;
        double forceY = 0.0;
        
        if (controller->moveForward) forceY += 100.0;
        if (controller->moveBackward) forceY -= 100.0;
        if (controller->strafeLeft) forceX -= 100.0;
        if (controller->strafeRight) forceX += 100.0;
        
        physics.ApplyForce(player, forceX, forceY, 0.0);
    }
}
```

### With Visual Effects

```cpp
// Trigger particle effects on collision
void HandleCollisionEffects(EntityManager& em, VisualFeedbackSystem& vfx) {
    em.ForEach<CollisionInfo, Position>([&](Entity e, CollisionInfo& info, Position& pos) {
        if (info.collisionCount > 0) {
            for (const auto& contact : info.contacts) {
                // Spawn spark particles at contact point
                vfx.SpawnSparks(contact.contactPointX,
                               contact.contactPointY,
                               contact.contactPointZ,
                               contact.impulse * 0.1);
            }
        }
    });
}
```

## Future Enhancements

Planned features for the physics system:

- [ ] Spatial partitioning (octree/grid) for broad-phase optimization
- [ ] Continuous collision detection (CCD) for fast-moving objects
- [ ] Compound colliders (multiple shapes per entity)
- [ ] Mesh colliders for complex geometry
- [ ] Cloth/soft body simulation
- [ ] Physics materials with advanced friction models
- [ ] Constraint solver for complex joint configurations
- [ ] Sleep/wake system for inactive objects
- [ ] Physics debugging visualization
- [ ] Multi-threading for collision detection

## Troubleshooting

### Objects Pass Through Each Other
- Check that both entities have colliders enabled
- Verify collision layers are compatible
- Reduce timestep or enable CCD for fast objects
- Increase collider size slightly

### Jittery Movement
- Use fixed timestep
- Increase damping values
- Check for conflicting forces
- Verify mass values are reasonable (0.1 - 1000)

### Poor Performance
- Reduce number of active colliders
- Use collision layers to filter checks
- Simplify collider shapes
- Disable collision for distant objects
- Consider spatial partitioning

### Objects Float or Sink
- Check gravity settings
- Verify mass values
- Check if `useGravity` is enabled
- Review buoyancy/drag settings

## References

- [Game Physics Engine Development](https://www.amazon.com/Game-Physics-Engine-Development-Commercial-Grade/dp/0123819768) - Ian Millington
- [Real-Time Collision Detection](https://www.amazon.com/Real-Time-Collision-Detection-Interactive-Technology/dp/1558607323) - Christer Ericson
- [Box2D Manual](https://box2d.org/documentation/) - Erin Catto
- [Unity Physics Documentation](https://docs.unity3d.com/Manual/PhysicsSection.html)
