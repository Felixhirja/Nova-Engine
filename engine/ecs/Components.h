#pragma once

// TODO: Component System Design Roadmap
//
// COMPONENT OPTIMIZATION:
// [ ] Structure of Arrays (SoA): Convert Array of Structures to SoA for SIMD
// [ ] Component Packing: Pack small components together to reduce memory usage
// [ ] Hot/Cold Component Separation: Separate frequently/rarely accessed data
// [ ] Component Compression: Use bit fields and custom encodings for space efficiency
// [ ] Component Versioning: Handle component schema evolution gracefully
// [ ] Component Validation: Runtime checks for component invariants
// [ ] Component Templates: Generic components with type parameters
//
// COMPONENT RELATIONSHIPS:
// [ ] Component Dependencies: Automatic validation of required components
// [ ] Component Groups: Logical grouping of related components
// [ ] Component Inheritance: Base component types with specializations
// [ ] Component References: Safe references between entities
// [ ] Component Events: Notifications on component add/remove/modify
// [ ] Component Constraints: Enforce business rules at component level
//
// COMPONENT SERIALIZATION:
// [ ] Binary Serialization: Efficient component data persistence
// [ ] JSON Serialization: Human-readable component export/import
// [ ] Delta Compression: Only save changed component fields
// [ ] Version Migration: Handle component format changes during loading
// [ ] Asset References: Resolve external asset dependencies in components
// [ ] Cross-Platform Compatibility: Handle endianness and type size differences
//
// COMPONENT DEBUGGING:
// [ ] Component Inspector: Runtime viewing and editing of component values
// [ ] Component History: Track component value changes over time
// [ ] Component Statistics: Memory usage and access patterns per component type
// [ ] Component Validation: Detect invalid or corrupt component data
// [ ] Component Diffing: Compare component states between frames
//
// ADVANCED COMPONENT FEATURES:
// [ ] Dynamic Components: Runtime component type creation
// [ ] Component Pooling: Reuse component instances to reduce allocations
// [ ] Component Streaming: Load/unload components based on relevance
// [ ] Component Scripting: Script-defined components with native performance
// [ ] Component Networking: Automatic synchronization across network
// [ ] Component Animation: Built-in tweening and animation support

#include "Component.h"
#include "EntityHandle.h"

using Entity = int;

// ============================================================================
// COMPONENT SYSTEM DEFINITIONS
// ============================================================================
// This file contains all ECS component definitions for the Nova Engine.
// Components are organized into logical categories for clarity.
// ============================================================================

#include "../CameraSystem.h"
#include <cmath>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// ============================================================================
// CORE COMPONENTS
// ============================================================================

/**
 * Position: 3D position in world space
 */
struct Position : public Component {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    
    Position() = default;
    Position(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

/**
 * Velocity: Linear velocity vector
 */
struct Velocity : public Component {
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    
    Velocity() = default;
    Velocity(double vx_, double vy_, double vz_) : vx(vx_), vy(vy_), vz(vz_) {}
};

/**
 * Sprite: 2D sprite rendering component
 */
struct Sprite : public Component {
    int textureHandle = 0;
    int layer = 0;
    int frame = 0;
    
    Sprite() = default;
    Sprite(int tex, int lyr) : textureHandle(tex), layer(lyr) {}
    Sprite(int tex, int lyr, int frm) : textureHandle(tex), layer(lyr), frame(frm) {}
};

/**
 * Acceleration: Linear acceleration vector
 */
struct Acceleration : public Component {
    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;
};

/**
 * Name: Human-readable identifier for entities
 */
struct Name : public Component {
    std::string value;
};

/**
 * ViewportID: Reference to which viewport should render this entity
 */
struct ViewportID : public Component {
    int viewportId = 0;  // 0 = main viewport, can support multiple viewports
    
    ViewportID() = default;
    explicit ViewportID(int vp) : viewportId(vp) {}
};

/**
 * CameraComponent: Marks an entity as camera target for following
 */
struct CameraComponent : public Component {
    bool isActive = true;        // Whether this camera target is currently active
    int priority = 0;            // Higher priority cameras take precedence (e.g., player = 100)
    
    CameraComponent() = default;
    explicit CameraComponent(int prio) : priority(prio) {}
};

/**
 * Faction: Faction affiliation for multiplayer/gameplay
 */
struct Faction : public Component {
    int id = 0;
    
    Faction() = default;
    Faction(int factionId) : id(factionId) {}
};

/**
 * Lifetime: Component for entities that should be destroyed after time
 */
struct Lifetime : public Component {
    double remaining = 0.0;
};

// ============================================================================
// PHYSICS COMPONENTS
// ============================================================================

/**
 * RigidBody: Represents a physics-simulated body with mass, velocity, and forces
 */
struct RigidBody : public Component {
    // Kinematic properties
    double mass = 1.0;                    // Mass in kg
    double inverseMass = 1.0;             // Cached 1/mass for performance
    double restitution = 0.5;             // Bounciness (0 = no bounce, 1 = perfect bounce)
    double friction = 0.5;                // Surface friction (0 = ice, 1 = rubber)
    double linearDamping = 0.01;          // Air resistance for velocity
    double angularDamping = 0.01;         // Air resistance for rotation

    // Angular velocity (for rotation)
    double angularVelocityX = 0.0;
    double angularVelocityY = 0.0;
    double angularVelocityZ = 0.0;

    // Orientation (Euler angles in radians)
    double rotationX = 0.0;
    double rotationY = 0.0;
    double rotationZ = 0.0;

    // Physics state flags
    bool isKinematic = false;             // If true, not affected by forces/collisions
    bool useGravity = true;               // Whether affected by gravity
    bool freezePositionX = false;         // Lock X-axis movement
    bool freezePositionY = false;         // Lock Y-axis movement
    bool freezePositionZ = false;         // Lock Z-axis movement
    bool freezeRotationX = false;         // Lock X-axis rotation
    bool freezeRotationY = false;         // Lock Y-axis rotation
    bool freezeRotationZ = false;         // Lock Z-axis rotation

    // Center of mass offset from position
    double centerOfMassX = 0.0;
    double centerOfMassY = 0.0;
    double centerOfMassZ = 0.0;

    RigidBody() {
        UpdateInverseMass();
    }

    void SetMass(double m) {
        mass = m;
        UpdateInverseMass();
    }

    void UpdateInverseMass() {
        inverseMass = (mass > 0.0 && !isKinematic) ? (1.0 / mass) : 0.0;
    }
};

/**
 * Force: Represents a force applied to a RigidBody
 */
struct Force : public Component {
    double fx = 0.0;  // Force X component (Newtons)
    double fy = 0.0;  // Force Y component
    double fz = 0.0;  // Force Z component

    // Force application point (relative to object center)
    double pointX = 0.0;
    double pointY = 0.0;
    double pointZ = 0.0;

    // Force mode
    enum class Mode {
        Force,          // Continuous force (F = ma)
        Impulse,        // Instantaneous force (applied once)
        Acceleration,   // Direct acceleration (ignores mass)
        VelocityChange  // Direct velocity change (applied once, ignores mass)
    };
    Mode mode = Mode::Force;

    // Lifetime (-1 = permanent, 0 = applied and cleared, >0 = duration in seconds)
    double lifetime = -1.0;

    // Whether this is a local or world-space force
    bool isLocalSpace = false;
};

struct ForceAccumulator : public Component {
    double accumulatedForceX = 0.0;
    double accumulatedForceY = 0.0;
    double accumulatedForceZ = 0.0;
    double accumulatedImpulseX = 0.0;
    double accumulatedImpulseY = 0.0;
    double accumulatedImpulseZ = 0.0;

    void Clear() {
        accumulatedForceX = 0.0;
        accumulatedForceY = 0.0;
        accumulatedForceZ = 0.0;
        accumulatedImpulseX = 0.0;
        accumulatedImpulseY = 0.0;
        accumulatedImpulseZ = 0.0;
    }
};

/**
 * ColliderType: Base class for different collider shapes
 */
struct Collider : public Component {
    enum class Shape {
        Box,
        Sphere,
        Capsule,
        Cylinder,
        Mesh  // For future complex collision meshes
    };

    Shape shape = Shape::Box;

    // Offset from entity position
    double offsetX = 0.0;
    double offsetY = 0.0;
    double offsetZ = 0.0;

    // Collision layers (bitmask for filtering)
    unsigned int collisionLayer = 1;      // Which layer this collider is on
    unsigned int collisionMask = 0xFFFFFFFF;  // Which layers it can collide with

    // Flags
    bool isTrigger = false;               // If true, generates events but no collision response
    bool isEnabled = true;                // Can be disabled without removing component

    // Material properties (can override RigidBody values)
    double materialRestitution = -1.0;    // -1 = use RigidBody value
    double materialFriction = -1.0;       // -1 = use RigidBody value
};

/**
 * BoxCollider: Axis-aligned or oriented box collider
 */
struct BoxCollider : public Collider {
    double width = 1.0;   // X extent
    double height = 1.0;  // Y extent
    double depth = 1.0;   // Z extent

    BoxCollider() {
        shape = Shape::Box;
    }
};

/**
 * SphereCollider: Spherical collider
 */
struct SphereCollider : public Collider {
    double radius = 0.5;

    SphereCollider() {
        shape = Shape::Sphere;
    }
};

/**
 * CapsuleCollider: Capsule (cylinder with hemispherical ends)
 */
struct CapsuleCollider : public Collider {
    double radius = 0.5;
    double height = 2.0;  // Total height including hemispheres

    enum class Direction {
        X,  // Capsule aligned along X-axis
        Y,  // Capsule aligned along Y-axis (default for characters)
        Z   // Capsule aligned along Z-axis
    };
    Direction direction = Direction::Y;

    CapsuleCollider() {
        shape = Shape::Capsule;
    }
};

/**
 * CollisionInfo: Stores information about a collision event
 */
struct CollisionInfo : public Component {
    struct Contact {
        unsigned int otherEntity = 0;  // Entity we collided with
        double normalX = 0.0;          // Collision normal (from other to this)
        double normalY = 0.0;
        double normalZ = 1.0;
        double penetrationDepth = 0.0; // How far objects overlap
        double contactPointX = 0.0;    // World-space contact point
        double contactPointY = 0.0;
        double contactPointZ = 0.0;
        double impulse = 0.0;          // Magnitude of collision impulse
        double timestamp = 0.0;        // When collision occurred
        double timeOfImpact = 0.0;     // Normalized time of impact within frame (0..1)
    };

    std::vector<Contact> contacts;     // All active contacts this frame
    int collisionCount = 0;            // Number of collisions this frame

    void Clear() {
        contacts.clear();
        collisionCount = 0;
    }
};

/**
 * GravitySource: Creates a gravitational field
 */
struct GravitySource : public Component {
    double strength = 9.8;             // Gravitational acceleration at 1 unit distance
    double radius = 100.0;             // Maximum influence radius (-1 = infinite)
    bool isUniform = false;            // If true, constant gravity (like Earth's surface)

    // Direction for uniform gravity (normalized)
    double directionX = 0.0;
    double directionY = 0.0;
    double directionZ = -1.0;
};

/**
 * ConstantForce: Applies a continuous force every physics update
 */
struct ConstantForce : public Component {
    double forceX = 0.0;
    double forceY = 0.0;
    double forceZ = 0.0;

    double torqueX = 0.0;  // Rotational force
    double torqueY = 0.0;
    double torqueZ = 0.0;

    bool isLocalSpace = false;  // If true, force is in object's local space
};

/**
 * SpaceshipFlightModel: Detailed flight dynamics model for spacecraft.
 * Stores thrust capabilities, atmospheric coefficients, and orientation state
 * so that advanced physics systems can simulate realistic behaviour.
 */
struct SpaceshipFlightModel : public Component {
    // Mass and thrust configuration
    double massKg = 25000.0;             // Vehicle mass in kilograms
    double maxMainThrustN = 400000.0;    // Forward thrust capability (Newtons)
    double maxReverseThrustN = 250000.0; // Reverse thrust capability (Newtons)
    double maxLateralThrustN = 120000.0; // Lateral/side thrust (Newtons)
    double maxVerticalThrustN = 150000.0;// Vertical thrust (Newtons)
    double maxLinearSpeed = 0.0;         // Optional linear speed cap (m/s, 0 = unlimited)
    double linearDamping = 0.25;         // Linear damping constant (N per m/s)

    // Control inputs (-1..1 range expected)
    double throttle = 0.0;               // Forward/backward throttle input
    double strafeInput = 0.0;            // Lateral thruster input
    double verticalInput = 0.0;          // Vertical thruster input
    double pitchInput = 0.0;             // Pitch control input
    double yawInput = 0.0;               // Yaw control input
    double rollInput = 0.0;              // Roll control input

    // Orientation state (Euler angles in radians)
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;

    // Angular velocity state (radians per second)
    double angularVelocityX = 0.0;
    double angularVelocityY = 0.0;
    double angularVelocityZ = 0.0;

    // Rotational characteristics
    double maxPitchTorque = 350000.0;    // Maximum pitch torque (N*m)
    double maxYawTorque = 350000.0;      // Maximum yaw torque (N*m)
    double maxRollTorque = 250000.0;     // Maximum roll torque (N*m)
    double inertiaTensorX = 120000.0;    // Moment of inertia around X axis
    double inertiaTensorY = 160000.0;    // Moment of inertia around Y axis
    double inertiaTensorZ = 100000.0;    // Moment of inertia around Z axis
    double angularDamping = 0.3;         // Base angular damping coefficient

    // Atmospheric flight configuration
    bool atmosphericFlightEnabled = true;
    double seaLevelAtmosphericDensity = 1.225; // kg/m^3 at sea level
    double atmosphereScaleHeight = 8000.0;     // Density falloff height (meters)
    double atmosphereBaseAltitude = 0.0;       // Altitude where atmosphere begins (meters)
    double dragCoefficient = 0.25;             // Aerodynamic drag coefficient
    double liftCoefficient = 0.7;              // Lift coefficient for wings/control surfaces
    double referenceArea = 20.0;               // Effective reference area (m^2)
    double atmosphericAngularDrag = 6000.0;    // Additional angular damping from atmosphere
    double gravity = -9.81;                    // Local gravitational acceleration (m/s^2)

    // Telemetry values updated by the physics system
    double currentAtmosphericDensity = 0.0;
    double lastAppliedForceX = 0.0;
    double lastAppliedForceY = 0.0;
    double lastAppliedForceZ = 0.0;
    double lastAppliedTorqueX = 0.0;
    double lastAppliedTorqueY = 0.0;
    double lastAppliedTorqueZ = 0.0;
    double lastLinearAccelerationX = 0.0;
    double lastLinearAccelerationY = 0.0;
    double lastLinearAccelerationZ = 0.0;
    double lastAngularAccelerationX = 0.0;
    double lastAngularAccelerationY = 0.0;
    double lastAngularAccelerationZ = 0.0;
};

/**
 * CharacterController: Specialized physics for player/NPC characters
 */
struct CharacterController : public Component {
    double height = 2.0;
    double radius = 0.5;
    double stepOffset = 0.3;           // Maximum step height
    double slopeLimit = 45.0;          // Maximum walkable slope (degrees)
    double skinWidth = 0.08;           // Collision detection margin

    // Movement
    double moveSpeed = 5.0;
    double sprintMultiplier = 1.5;
    double crouchMultiplier = 0.5;
    double jumpHeight = 1.5;
    double gravity = 20.0;

    // State
    bool isGrounded = false;
    bool isCrouching = false;
    double verticalVelocity = 0.0;

    // Ground detection
    double groundCheckDistance = 0.1;
    unsigned int groundLayer = 1;
};

/**
 * Joint: Connects two rigid bodies with constraints
 */
struct Joint : public Component {
    enum class Type {
        Fixed,       // No relative movement
        Hinge,       // Rotation around one axis
        Spring,      // Spring-damper connection
        Distance     // Fixed distance constraint
    };

    Type type = Type::Fixed;
    unsigned int connectedEntity = 0;  // Other entity in joint (0 = world)

    // Connection points (relative to each entity)
    double anchorX = 0.0;
    double anchorY = 0.0;
    double anchorZ = 0.0;

    double connectedAnchorX = 0.0;
    double connectedAnchorY = 0.0;
    double connectedAnchorZ = 0.0;

    // Type-specific parameters
    double springStrength = 100.0;     // For spring joints
    double springDamping = 10.0;       // For spring joints
    double maxDistance = 1.0;          // For distance joints
    double minDistance = 0.0;          // For distance joints

    bool breakable = false;
    double breakForce = 1000.0;        // Force required to break joint
};

// ============================================================================
// RENDERING COMPONENTS
// ============================================================================

/**
 * DrawComponent: Controls how an actor is rendered visually
 * Supports different rendering modes (sprites, meshes, particles, etc.)
 */
struct DrawComponent : public Component {
    enum class RenderMode {
        None,           // Not rendered
        Sprite2D,       // 2D sprite rendering
        Billboard,      // Billboard sprite (always faces camera)
        Mesh3D,         // 3D mesh rendering
        Particles,      // Particle system
        Wireframe,      // Debug wireframe
        Custom          // Custom rendering callback
    };

    RenderMode mode = RenderMode::None;

    // Common properties
    bool visible = true;
    int renderLayer = 0;              // Rendering order (higher = drawn later)
    float opacity = 1.0f;             // 0.0 = transparent, 1.0 = opaque
    bool castShadows = true;
    bool receiveShadows = true;

    // Sprite/Billboard mode properties
    int textureHandle = 0;            // Texture ID for sprite rendering
    int spriteFrame = 0;              // Current animation frame
    float spriteScale = 1.0f;         // Size multiplier
    bool flipHorizontal = false;
    bool flipVertical = false;

    // 3D Mesh mode properties
    int meshHandle = 0;               // Mesh resource ID
    int materialHandle = 0;           // Material/shader ID
    float meshScale = 1.0f;           // Uniform scale
    float meshScaleX = 1.0f;          // Non-uniform scale X
    float meshScaleY = 1.0f;          // Non-uniform scale Y
    float meshScaleZ = 1.0f;          // Non-uniform scale Z

    // Particle system properties
    int particleSystemHandle = 0;     // Particle system ID
    bool particlesActive = true;

    // Animation properties
    bool animated = false;
    int currentAnimationFrame = 0;
    float animationSpeed = 1.0f;      // Frames per second
    float animationTimer = 0.0f;
    int animationStartFrame = 0;
    int animationEndFrame = 0;
    bool animationLooping = true;
    bool animationPlaying = true;

    // Color tinting
    float tintR = 1.0f;               // Red tint (0.0-1.0)
    float tintG = 1.0f;               // Green tint (0.0-1.0)
    float tintB = 1.0f;               // Blue tint (0.0-1.0)

    // LOD (Level of Detail) properties
    bool useLOD = false;
    float lodDistance1 = 50.0f;       // Distance for first LOD switch
    float lodDistance2 = 100.0f;      // Distance for second LOD switch
    int lodMesh1 = 0;                 // Alternative mesh for LOD1
    int lodMesh2 = 0;                 // Alternative mesh for LOD2

    // Debug properties
    bool showBoundingBox = false;
    bool showCollisionShape = false;
    float debugColorR = 1.0f;
    float debugColorG = 1.0f;
    float debugColorB = 1.0f;

    // Custom rendering callback (for advanced use cases)
    std::function<void(const DrawComponent&, const Position&)> customRenderCallback;

    // Helper methods
    void SetTint(float r, float g, float b) {
        tintR = r;
        tintG = g;
        tintB = b;
    }

    void SetScale(float scale) {
        meshScale = scale;
        meshScaleX = scale;
        meshScaleY = scale;
        meshScaleZ = scale;
    }

    void SetScale(float x, float y, float z) {
        meshScaleX = x;
        meshScaleY = y;
        meshScaleZ = z;
        meshScale = 1.0f; // Reset uniform scale when using non-uniform
    }

    void StartAnimation(int startFrame, int endFrame, bool loop = true) {
        animationStartFrame = startFrame;
        animationEndFrame = endFrame;
        currentAnimationFrame = startFrame;
        animationTimer = 0.0f;
        animationLooping = loop;
        animationPlaying = true;
        animated = true;
    }

    void StopAnimation() {
        animationPlaying = false;
    }

    void UpdateAnimation(float deltaTime) {
        if (!animated || !animationPlaying) return;

        animationTimer += deltaTime * animationSpeed;

        if (animationTimer >= 1.0f) {
            animationTimer = 0.0f;
            currentAnimationFrame++;

            if (currentAnimationFrame > animationEndFrame) {
                if (animationLooping) {
                    currentAnimationFrame = animationStartFrame;
                } else {
                    currentAnimationFrame = animationEndFrame;
                    animationPlaying = false;
                }
            }
        }
    }
};

// ============================================================================
// GAMEPLAY COMPONENTS
// ============================================================================

/**
 * Transform2D: 2D transformation for sprites and UI elements
 */
struct Transform2D : public Component {
    double x = 0.0;
    double y = 0.0;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
};

/**
 * PhysicsBody: Legacy physics body (being phased out)
 */
struct PhysicsBody : public Component {
    double mass = 1.0;
    double drag = 0.0;
    bool affectedByGravity = true;
};

/**
 * PhysicsMaterial: Material properties for physics interactions
 */
struct PhysicsMaterial : public Component {
    double staticFriction = 0.6;
    double dynamicFriction = 0.4;
    double restitution = 0.1;
    double density = 1.0;
};

/**
 * Hitbox: Simple collision box for 2D physics
 */
struct Hitbox : public Component {
    double width = 1.0;
    double height = 1.0;
};

/**
 * AnimationState: Animation controller for sprites
 */
struct AnimationState : public Component {
    int currentFrame = 0;
    double frameTimer = 0.0;
    double frameDuration = 0.1;
    bool looping = true;
    int startFrame = 0;
    int endFrame = 0;
    bool playing = true;
    bool pingPong = false;
    int playbackDirection = 1;
};

/**
 * PlayerController: Input state for player character
 */
struct PlayerController : public Component {
    bool moveLeft = false;
    bool moveRight = false;
    bool moveForward = false;
    bool moveBackward = false;
    bool moveUp = false;
    bool moveDown = false;
    bool strafeLeft = false;
    bool strafeRight = false;
    bool jumpRequested = false;
    bool sprint = false;
    bool crouch = false;
    bool slide = false;
    bool boost = false;
    bool thrustMode = false;
    double cameraYaw = 0.0;  // Will be initialized from camera defaults
    double facingYaw = 0.0;  // Player's facing direction for camera following
};

/**
 * TargetLock: Camera targeting system
 */
struct TargetLock : public Component {
    unsigned int targetEntityId = 0;  // Entity ID to lock onto (0 = no target)
    bool isLocked = false;            // Whether target lock is active
    double offsetX = 0.0;             // Camera offset from target
    double offsetY = 5.0;             // Camera offset from target
    double offsetZ = 10.0;            // Camera offset from target
    double followDistance = 15.0;     // Distance to maintain from target
    double followHeight = 5.0;        // Height above target
};

/**
 * ProjectileComponent: Basic projectile component
 */
struct ProjectileComponent : public Component {
    int ownerEntity = 0;
    std::string weaponSlot;
};

/**
 * DamagePayload: Damage dealing component
 */
struct DamagePayload : public Component {
    double amount = 0.0;
    int sourceEntity = 0;
};

/**
 * DockingStatus: Docking state for ships
 */
struct DockingStatus : public Component {
    bool isDocked = false;
    std::string portId;
    double alignmentScore = 0.0;
    double lastContactTime = 0.0;
};

// ============================================================================
// AI AND NAVIGATION COMPONENTS
// ============================================================================

enum class AIState {
    Idle,
    Patrolling,
    Trading,
    Hunting,
    Fleeing,
    Docked
};

/**
 * Basic 3D point/vector for spatial calculations
 */
struct Point3D {
    double x, y, z;
    Point3D(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}

    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }

    double distanceTo(const Point3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

/**
 * PatrolRoute: AI patrol waypoints
 */
struct PatrolRoute : public Component {
    std::vector<Point3D> waypoints;
    size_t currentWaypointIndex = 0;
    float arrivalThreshold = 10.0f; // Distance to consider waypoint reached
};

/**
 * NavigationState: AI navigation target
 */
struct NavigationState : public Component {
    Point3D targetPosition;
    float throttle = 0.0f;
    double yaw = 0.0;
    double pitch = 0.0;
    bool hasTarget = false;
};

/**
 * NavigationGrid: 3D navigation grid for pathfinding
 */
struct NavigationGrid : public Component {
    int width = 0;
    int height = 0;
    int layers = 1;
    double cellSize = 1.0;
    Point3D origin;
    std::vector<uint8_t> walkableMask;

    bool IsWalkable(int x, int y, int layer = 0) const {
        if (x < 0 || y < 0 || layer < 0) return false;
        if (x >= width || y >= height || layer >= layers) return false;
        size_t index = static_cast<size_t>(layer) * static_cast<size_t>(width * height)
                     + static_cast<size_t>(y * width + x);
        if (index >= walkableMask.size()) return false;
        return walkableMask[index] != 0;
    }
};

/**
 * AIBehavior: AI state and behavior configuration
 */
struct AIBehavior : public Component {
    AIState currentState = AIState::Idle;
    float stateTimer = 0.0f;
    float decisionTimer = 0.0f;
    ecs::EntityHandle targetEntity = ecs::EntityHandle::Null();
    float aggressionLevel = 0.5f; // 0.0 = peaceful, 1.0 = aggressive
    float cautionLevel = 0.5f;    // 0.0 = reckless, 1.0 = cautious
};

/**
 * BehaviorTreeHandle: Reference to behavior tree
 */
struct BehaviorTreeHandle : public Component {
    std::string treeId;
    bool autoActivate = true;
};

/**
 * MissionObjective: Mission objective state
 */
struct MissionObjective : public Component {
    enum class State {
        Inactive,
        Active,
        Completed,
        Failed
    };

    struct Trigger {
        std::string id;
        std::string description;
        double threshold = 0.0;
    };

    std::string id;
    std::string description;
    State state = State::Inactive;
    std::vector<Trigger> successConditions;
    std::vector<Trigger> failureConditions;
};

/**
 * MissionState: Overall mission state
 */
struct MissionState : public Component {
    std::string missionId;
    std::deque<std::string> objectiveOrder;
    std::unordered_map<std::string, MissionObjective::State> objectiveStates;
    bool failed = false;
    bool completed = false;
};

/**
 * StatusEffect: Temporary status effects
 */
struct StatusEffect : public Component {
    std::string id;
    double magnitude = 0.0;
    double duration = 0.0;
    double elapsed = 0.0;
    bool stacks = false;
};

/**
 * ScriptedTrigger: Event triggers
 */
struct ScriptedTrigger : public Component {
    std::string id;
    std::string description;
    bool oneShot = true;
    bool active = true;
    std::function<bool(const Position&)> condition;
};

/**
 * DamageEvent: Damage event data
 */
struct DamageEvent {
    double amount = 0.0;
    unsigned int sourceEntity = 0;
    std::string damageType;
};

/**
 * StatusEffectEvent: Status effect event data
 */
struct StatusEffectEvent {
    std::string effectId;
    double magnitude = 0.0;
    double duration = 0.0;
};

/**
 * GameplayEvent: Generic gameplay event
 */
struct GameplayEvent : public Component {
    enum class Type {
        Damage,
        StatusEffectApplied,
        TriggerActivated
    } type = Type::Damage;

    std::variant<std::monostate, DamageEvent, StatusEffectEvent, std::string> payload;
    double timestamp = 0.0;
};

/**
 * GameplayEventBuffer: Event queue for gameplay systems
 */
struct GameplayEventBuffer : public Component {
    std::deque<GameplayEvent> events;
    double lastDispatchTime = 0.0;

    void Push(const GameplayEvent& event) {
        events.push_back(event);
    }

    std::vector<GameplayEvent> ConsumeAll() {
        std::vector<GameplayEvent> result(events.begin(), events.end());
        events.clear();
        return result;
    }
};

/**
 * DeterministicRandomSeed: Seeded random number generation
 */
struct DeterministicRandomSeed : public Component {
    uint64_t baseSeed = 0u;
    uint64_t entitySeed = 0u;
    std::mt19937_64 generator{0u};

    void Reseed(uint64_t newSeed) {
        entitySeed = newSeed;
        generator.seed(newSeed);
    }
};

/**
 * ReplayBookmark: Replay system bookmarks
 */
struct ReplayBookmark : public Component {
    std::string label;
    size_t frameIndex = 0u;
};

struct MovementParameters : public Component {
    double strafeAcceleration = 4.0;
    double forwardAcceleration = 4.0;
    double backwardAcceleration = 4.0;
    double strafeDeceleration = 4.0;
    double forwardDeceleration = 4.0;
    double backwardDeceleration = 4.0;
    double strafeMaxSpeed = 5.0;
    double forwardMaxSpeed = 5.0;
    double backwardMaxSpeed = 5.0;
    double friction = 0.0;
};

struct MovementBounds : public Component {
    double minX = -std::numeric_limits<double>::infinity();
    double maxX = std::numeric_limits<double>::infinity();
    double minY = -std::numeric_limits<double>::infinity();
    double maxY = std::numeric_limits<double>::infinity();
    double minZ = -std::numeric_limits<double>::infinity();
    double maxZ = std::numeric_limits<double>::infinity();
    bool clampX = false;
    bool clampY = false;
    bool clampZ = false;
};

struct PlayerPhysics : public Component {
    bool enableGravity = true;
    bool thrustMode = false;
    bool isGrounded = true;
    double gravity = -9.8;
    double jumpImpulse = 6.0;
    double maxAscentSpeed = 10.0;
    double maxDescentSpeed = -20.0;
    double thrustAcceleration = 8.0;
    double thrustDamping = 6.0;
};

struct PlayerVitals : public Component {
    double health = 100.0;
    double maxHealth = 100.0;
    double shields = 0.0;
    double maxShields = 0.0;
    double energy = 0.0;
    double maxEnergy = 0.0;
    double temperature = 36.0;
    double fatigue = 0.0;
};

struct PlayerInventory : public Component {
    struct ItemSlot {
        std::string id;
        std::string displayName;
        double massTons = 0.0;
        double volumeM3 = 0.0;
        int quantity = 0;
        bool equipped = false;
        bool questItem = false;
    };

    double carriedMassTons = 0.0;
    double carriedVolumeM3 = 0.0;
    double maxMassTons = 120.0;
    double maxVolumeM3 = 4.0;
    std::vector<ItemSlot> items;
};

struct PlayerProgression : public Component {
    double experience = 0.0;
    double lifetimeExperience = 0.0;
    int level = 1;
    int skillPoints = 0;
    int blueprintCredits = 0;
    std::unordered_map<std::string, int> reputationByFaction;
    std::unordered_set<std::string> unlockedSkillNodes;
};

enum class LocomotionSurfaceType {
    Unknown,
    PlanetaryGround,
    Spacewalk,
    ZeroGInterior
};

struct SurfaceMovementProfile {
    double accelerationMultiplier = 1.0;
    double decelerationMultiplier = 1.0;
    double maxSpeedMultiplier = 1.0;
    double jumpImpulseMultiplier = 1.0;
    double gravityMultiplier = 1.0;
    double frictionMultiplier = 1.0;
};

struct HazardModifier {
    double speedMultiplier = 1.0;
    double accelerationMultiplier = 1.0;
    double gravityMultiplier = 1.0;
    double staminaDrainRate = 0.0;
    double heatGainRate = 0.0;
};

struct LocomotionStateMachine : public Component {
    enum class State {
        Idle,
        Walk,
        Sprint,
        Airborne,
        Landing,
        Crouch,
        Slide
    };

    struct Weights {
        double idle = 1.0;
        double walk = 0.0;
        double sprint = 0.0;
        double airborne = 0.0;
        double landing = 0.0;
        double crouch = 0.0;
        double slide = 0.0;
    };

    State currentState = State::Idle;
    State previousState = State::Idle;
    Weights blendWeights;
    double timeInState = 0.0;
    double landingTimer = 0.0;
    double landingDuration = 0.25;
    double blendSmoothing = 8.0;
    double idleSpeedThreshold = 0.2;
    double walkSpeedThreshold = 1.5;
    double sprintSpeedThreshold = 4.5;
    double airborneVerticalSpeedThreshold = 0.2;
    bool wasGrounded = true;
    double crouchCameraOffset = -0.4;
    double slideCameraOffset = -0.6;
    double defaultCameraOffset = 0.0;
    double cameraSmoothing = 12.0;
    double currentCameraOffset = 0.0;
    double stamina = 100.0;
    double maxStamina = 100.0;
    double staminaRegenRate = 25.0;
    double sprintStaminaCost = 35.0;
    double sprintAccelerationMultiplier = 1.2;
    double sprintSpeedMultiplier = 1.35;
    double crouchSpeedMultiplier = 0.4;
    double crouchAccelerationMultiplier = 0.35;
    double slideSpeedMultiplier = 1.15;
    double slideDecelerationMultiplier = 0.45;
    double slideDuration = 0.75;
    double slideCooldown = 0.65;
    double slideSpeedThreshold = 3.0;
    double slideTimer = 0.0;
    double slideCooldownTimer = 0.0;
    double airborneAccelerationMultiplier = 0.5;
    double boostDuration = 0.35;
    double boostTimer = 0.0;
    double boostSpeedMultiplier = 1.4;
    double boostAccelerationMultiplier = 1.35;
    double heat = 0.0;
    double maxHeat = 100.0;
    double heatDissipationRate = 35.0;
    double boostHeatCostPerSecond = 45.0;
    LocomotionSurfaceType activeSurfaceType = LocomotionSurfaceType::PlanetaryGround;
    LocomotionSurfaceType defaultSurfaceType = LocomotionSurfaceType::PlanetaryGround;
    SurfaceMovementProfile activeSurfaceProfile;
    HazardModifier activeHazardModifier;
    std::unordered_map<LocomotionSurfaceType, SurfaceMovementProfile> surfaceProfiles = {
        {LocomotionSurfaceType::PlanetaryGround, SurfaceMovementProfile{}},
        {LocomotionSurfaceType::Spacewalk, SurfaceMovementProfile{0.55, 0.4, 0.75, 0.35, 0.2, 0.1}},
        {LocomotionSurfaceType::ZeroGInterior, SurfaceMovementProfile{0.7, 0.65, 0.85, 0.15, 0.15, 0.25}},
    };
    HazardModifier hazardBaseline;
    double runtimeAccelerationMultiplier = 1.0;
    double runtimeDecelerationMultiplier = 1.0;
    double runtimeMaxSpeedMultiplier = 1.0;
    double runtimeGravityMultiplier = 1.0;
    double runtimeFrictionMultiplier = 1.0;
    double runtimeJumpImpulseMultiplier = 1.0;
    bool boostActive = false;
    double baseGravity = -9.8;
    bool baseGravityInitialized = false;
    double baseJumpImpulse = 6.0;
    bool baseJumpInitialized = false;
};

struct EnvironmentSurface : public Component {
    LocomotionSurfaceType surfaceType = LocomotionSurfaceType::PlanetaryGround;
    bool overridesProfile = false;
    SurfaceMovementProfile movementProfile;
    bool isHazard = false;
    HazardModifier hazardModifier;
};

// ============================================================================
// SHIP SYSTEMS COMPONENTS (Data-only; no side effects)
// ============================================================================

// Primary power generation (e.g., reactor). Provides electrical output to the grid
struct Reactor : public Component {
    double maxOutputMW = 10.0;     // Maximum continuous output
    double currentOutputMW = 0.0;  // Current output setpoint
    double rampRateMWPerSec = 5.0; // How fast output can change
    double efficiency = 0.35;      // Thermal/electrical efficiency
    double heatPerMW = 2.0;        // Heat generated per MW output
    bool online = true;            // Whether the reactor is producing power
    std::string fuelType;          // Optional descriptor
    double fuelMassKg = 0.0;       // Optional remaining fuel
};

// Electrical distribution summary for a craft
struct PowerGrid : public Component {
    double capacityMW = 10.0;      // Grid capacity (breaker/limit)
    double availableMW = 0.0;      // Headroom remaining
    double loadMW = 0.0;           // Current total load
    double reserveMW = 0.0;        // Allocated reserve
    bool overload = false;         // True if load exceeds capacity
    bool brownout = false;         // True if load exceeds generation
};

// Energy storage (batteries/capacitors)
struct EnergyStorage : public Component {
    double capacityMJ = 100.0;     // Total stored energy capacity
    double currentMJ = 0.0;        // Current stored energy
    double maxChargeMW = 5.0;      // Max charging rate
    double maxDischargeMW = 8.0;   // Max discharge rate
    double chargeEfficiency = 0.95;// Round-trip efficiency factors
};

// Defensive shield system
struct ShieldSystem : public Component {
    double maxShield = 100.0;
    double currentShield = 100.0;
    double rechargeRate = 10.0;    // Units per second
    double rechargeDelay = 3.0;    // Delay after taking damage
    double lastDamageTime = 0.0;   // For managing delay
    double damageAbsorption = 1.0; // Multiplier for incoming damage
    bool online = true;
};

// Hull and structural integrity
struct HullIntegrity : public Component {
    double maxHP = 1000.0;
    double currentHP = 1000.0;
    double damageResistance = 0.0;   // Flat reduction
    double damageMultiplier = 1.0;   // Scaling factor for incoming damage
};

// Heat accumulation and dissipation
struct ThermalSystem : public Component {
    double heat = 0.0;
    double maxHeat = 100.0;
    double dissipationPerSec = 20.0; // Passive cooling
    double overheatThreshold = 90.0; // Begins throttling at this level
    bool overheated = false;
};

// Hardpoint: mounting position for weapons/modules
struct WeaponHardpoint : public Component {
    enum class Size { Small, Medium, Large, Capital };
    enum class MountType { Fixed, Gimbal, Turret };

    std::string id;                 // Identifier within the ship
    Size size = Size::Small;
    MountType mount = MountType::Fixed;
    double arcYawDeg = 0.0;         // Allowed yaw arc (for gimbal/turret)
    double arcPitchDeg = 0.0;       // Allowed pitch arc
    bool occupied = false;
};

// Weapon instance/controller mounted in a hardpoint
struct WeaponMount : public Component {
    std::string hardpointId;        // Which hardpoint it occupies
    std::string weaponClass;        // e.g., "laser.cannon.m1"
    double cooldown = 0.5;          // Seconds between shots
    double cooldownTimer = 0.0;
    double heatPerShot = 5.0;       // Heat added per shot
    int ammo = -1;                  // -1 = infinite/energy weapon
    bool triggerHeld = false;
};

// Sensor/scan capability
struct SensorSuite : public Component {
    double rangeKm = 50.0;          // Maximum detection range
    double resolution = 1.0;        // Lower = finer
    double scanPowerMW = 0.2;       // Power draw when active
    bool activeScan = false;        // If true, active scanning enabled
};

// Navigation computer/autopilot summary
struct NavigationComputer : public Component {
    double targetX = 0.0;
    double targetY = 0.0;
    double targetZ = 0.0;
    bool hasTarget = false;
    bool autopilotEnabled = false;
};

// Cargo bay inventory (lightweight summary)
struct CargoHold : public Component {
    struct Item { std::string id; double massTons = 0.0; double volumeM3 = 0.0; int quantity = 0; };
    double capacityMassTons = 100.0;
    double capacityVolumeM3 = 50.0;
    double usedMassTons = 0.0;
    double usedVolumeM3 = 0.0;
    std::vector<Item> items;
};

// Docking interface
struct DockingPort : public Component {
    std::string portId;             // Unique identifier
    bool occupied = false;
    double alignmentScore = 0.0;    // How well aligned is the current approach
    std::string compatibleSize;     // e.g., "small", "medium", "large"
};

// Communications/transponder
struct Communications : public Component {
    std::string callsign;
    bool transponderOn = true;
    double broadcastRangeKm = 100.0;
};

// ============================================================================
// SHIP ASSEMBLY AS ECS COMPONENTS
// ============================================================================

// Desired assembly spec attached to an entity (data-only)
struct ShipAssemblySpec : public Component {
    std::string hullId;                                           // Target hull blueprint id
    std::unordered_map<std::string, std::string> slotAssignments; // slotId -> componentId
    
    ShipAssemblySpec() = default;
    ShipAssemblySpec(std::string hull, std::unordered_map<std::string, std::string> slots)
        : hullId(std::move(hull)), slotAssignments(std::move(slots)) {}
};

// Summary metrics from an assembly evaluation
struct ShipAssemblyMetrics : public Component {
    double massTons = 0.0;
    double totalThrustKN = 0.0;
    double mainThrustKN = 0.0;
    double maneuverThrustKN = 0.0;
    double powerOutputMW = 0.0;
    double powerDrawMW = 0.0;
    double netPowerMW = 0.0;
    double heatGenerationMW = 0.0;
    double heatDissipationMW = 0.0;
    double netHeatMW = 0.0;
    int crewRequired = 0;
    int crewCapacity = 0;
    int avionicsModuleCount = 0;
    double avionicsPowerDrawMW = 0.0;
};

// ============================================================================
// SPACESHIP MARKER/DESCRIPTOR
// =========================================================================

/**
 * SpaceshipTag: Minimal descriptor to mark an entity as a spaceship.
 * This component carries lightweight identity metadata without implying
 * or attaching any physics/rendering/AI components. Systems may opt-in
 * to attach additional components based on this descriptor.
 */
struct SpaceshipTag : public Component {
    // Opaque class identifier (e.g., catalog key). Optional.
    std::string classId;

    // Human-friendly display name. Optional.
    std::string displayName;

    // Which default loadout index was intended (-1 = unspecified)
    int loadoutIndex = -1;

    // True if controlled by player; false for AI or neutral.
    bool playerControlled = false;
};

/**
 * BehaviorTreeComponent: AI behavior tree state and execution
 */
struct BehaviorTreeComponent : public Component {
    std::string treeId;
    std::string currentNodeId;
    double executionTimer = 0.0;
    bool isActive = true;
    std::unordered_map<std::string, double> blackboard;
};

/**
 * WeaponSlotConfig: Configuration for weapon slots
 */
struct WeaponSlotConfig {
    float fireRatePerSecond = 1.0f;
    int ammo = -1; // -1 = infinite
    double damage = 10.0;
    double projectileSpeed = 100.0;
    double projectileLifetime = 2.0;
    float muzzleDirX = 1.0f;
    float muzzleDirY = 0.0f;
    float muzzleDirZ = 0.0f;
};

/**
 * Weapon: Basic weapon component for firing projectiles
 */
struct Weapon : public Component {
    WeaponSlotConfig config;
    double lastFireTime = 0.0;
    bool isFiring = false;
    Entity targetEntity = 0;
};

/**
 * LocomotionComponent: Movement and locomotion state
 */
struct LocomotionComponent : public Component {
    double speed = 0.0;
    double maxSpeed = 10.0;
    double acceleration = 5.0;
    double deceleration = 5.0;
    bool isGrounded = true;
    double jumpForce = 10.0;
    double stamina = 100.0;
    double maxStamina = 100.0;
};

/**
 * ShipAssemblyComponent: Spaceship assembly and configuration
 */
struct ShipAssemblyComponent : public Component {
    std::string shipType;
    std::vector<std::string> installedModules;
    double assemblyProgress = 1.0; // 0-1, 1 = complete
    bool isAssembled = true;
};

/**
 * SpaceshipPhysicsComponent: Advanced physics for spaceships
 */
struct SpaceshipPhysicsComponent : public Component {
    double thrustPower = 100.0;
    double rotationTorque = 50.0;
    double mass = 1000.0;
    double dragCoefficient = 0.1;
    bool hasGravityDrive = false;
    double fuelLevel = 100.0;
    double maxFuel = 100.0;
};

/**
 * AnimationComponent: Animation state and playback
 */
struct AnimationComponent : public Component {
    std::string currentAnimation;
    double animationTime = 0.0;
    double animationSpeed = 1.0;
    bool isLooping = true;
    bool isPlaying = true;
    std::unordered_map<std::string, double> animationLengths;
};

/**
 * TargetingComponent: Target acquisition and tracking
 */
struct TargetingComponent : public Component {
    Entity currentTarget = 0;
    double lockOnProgress = 0.0;
    double maxLockOnRange = 1000.0;
    bool isLocked = false;
    double lastScanTime = 0.0;
    std::vector<Entity> potentialTargets;
};

/**
 * ShieldComponent: Energy shield system
 */
/**
 * Health: Health/hit points component
 */
struct Health : public Component {
    double current = 100.0;
    double maximum = 100.0;
    
    Health() = default;
    Health(double cur, double max) : current(cur), maximum(max) {}
};

struct ShieldComponent : public Component {
    double currentShields = 100.0;
    double maxShields = 100.0;
    double rechargeRate = 10.0; // per second
    double rechargeDelay = 2.0; // seconds after taking damage
    double lastDamageTime = 0.0;
    bool isActive = true;

    // Parameterized constructor for EmplaceComponent
    ShieldComponent(double current = 100.0, double max = 100.0, double recharge = 10.0, 
                   double delay = 2.0, double lastDamage = 0.0, bool active = true)
        : currentShields(current), maxShields(max), rechargeRate(recharge), 
          rechargeDelay(delay), lastDamageTime(lastDamage), isActive(active) {}
};

// Alias for backward compatibility
using Shield = ShieldComponent;

/**
 * NavigationComponent: Pathfinding and navigation
 */
struct NavigationComponent : public Component {
    std::vector<std::pair<double, double>> path;
    size_t currentPathIndex = 0;
    Entity destinationEntity = 0;
    double arrivalRadius = 10.0;
    bool isNavigating = false;
    double lastPathUpdate = 0.0;
};

/**
 * GameplayEventComponent: Event system integration
 */
struct GameplayEventComponent : public Component {
    std::queue<std::string> pendingEvents;
    std::unordered_map<std::string, double> eventTimers;
    bool isProcessingEvents = true;
    double lastEventTime = 0.0;
};

/**
 * MissionScriptComponent: Mission scripting system
 */
struct MissionScriptComponent : public Component {
    std::string currentMissionId;
    std::unordered_map<std::string, std::string> missionState;
    std::vector<std::string> activeObjectives;
    double missionTimer = 0.0;
    bool isMissionActive = false;
};

/**
 * EnergyComponent: Power management and distribution system for ships
 */
struct EnergyComponent : public Component {
    double totalPowerCapacityMW = 30.0;  // Total power generation capacity in MW
    double currentPowerMW = 30.0;        // Current available power in MW
    double shieldAllocation = 0.33;      // Fraction of power allocated to shields (0.0-1.0)  
    double weaponAllocation = 0.33;      // Fraction of power allocated to weapons (0.0-1.0)
    double thrusterAllocation = 0.34;    // Fraction of power allocated to thrusters (0.0-1.0)
    double shieldPowerMW = 9.9;          // Actual power delivered to shields in MW
    double weaponPowerMW = 9.9;          // Actual power delivered to weapons in MW
    double thrusterPowerMW = 10.2;       // Actual power delivered to thrusters in MW
    double rechargeRateMW = 8.0;         // Base recharge rate in MW per second
    double consumptionRateMW = 10.0;     // Base consumption rate in MW per second
    double efficiency = 0.8;             // Power system efficiency (0.0-1.0)
    bool isActive = true;                // Whether the power system is active
};

// ============================================================================
// CELESTIAL BODY COMPONENTS
// ============================================================================

/**
 * Vector3: 3D vector utility for celestial mechanics
 */
struct Vector3 {
    double x, y, z;
    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    double Length() const {
        return std::sqrt(x*x + y*y + z*z);
    }
    
    Vector3 Normalized() const {
        double len = Length();
        if (len > 0.0) return Vector3(x/len, y/len, z/len);
        return Vector3(0, 0, 0);
    }
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator*(double scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    double Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    double Distance(const Vector3& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

/**
 * CelestialBodyComponent: Core properties of any celestial body
 */
struct CelestialBodyComponent : public Component {
    enum class BodyType {
        Star,
        RockyPlanet,      // Mercury, Venus, Earth, Mars type
        GasGiant,         // Jupiter, Saturn type
        IceGiant,         // Uranus, Neptune type
        Moon,
        Asteroid,
        SpaceStation,
        AsteroidBelt
    };
    
    BodyType type = BodyType::RockyPlanet;
    std::string name = "Unnamed";
    
    // Physical properties
    double mass = 5.972e24;           // kg (Earth default)
    double radius = 6371.0;           // km (Earth default)
    double rotationPeriod = 24.0;     // hours
    double axialTilt = 0.0;           // degrees
    double temperature = 288.0;       // Kelvin (Earth default: ~15°C)
    
    // Composition and features
    bool hasAtmosphere = false;
    double atmosphereDensity = 0.0;   // kg/m³
    bool hasRings = false;
    bool hasMagneticField = false;
    bool isHabitable = false;
    
    // Gameplay properties
    bool isLandable = false;
    bool isDockable = false;          // For stations
    int faction = 0;                  // Faction ownership (0 = neutral)
};

/**
 * OrbitalComponent: Orbital mechanics using Keplerian elements
 */
struct OrbitalComponent : public Component {
    unsigned int parentEntity = 0;    // Entity ID of parent body (0 = orbits star/barycenter)
    
    // Classical orbital elements
    double semiMajorAxis = 1.0;       // AU for planets, km for moons
    double eccentricity = 0.0;        // 0 = circular, 0-1 = ellipse
    double inclination = 0.0;         // degrees from reference plane
    double longitudeOfAscendingNode = 0.0;  // Ω (degrees)
    double argumentOfPeriapsis = 0.0; // ω (degrees)
    double meanAnomalyAtEpoch = 0.0;  // M₀ (degrees)
    
    // Derived properties
    double orbitalPeriod = 365.25;    // days
    double currentMeanAnomaly = 0.0;  // Current M (degrees)
    
    // Cached position (updated by orbital system)
    Vector3 cachedPosition = Vector3(0, 0, 0);
    Vector3 cachedVelocity = Vector3(0, 0, 0);
    double lastUpdateTime = 0.0;
};

/**
 * VisualCelestialComponent: Visual representation for celestial bodies
 */
struct VisualCelestialComponent : public Component {
    int textureHandle = -1;
    int normalMapHandle = -1;
    int cloudTextureHandle = -1;
    
    // Color (used if no texture or for tinting)
    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;
    
    // Material properties
    float emissive = 0.0f;            // For stars (0-1)
    float specular = 0.0f;            // For water/ice reflection
    float roughness = 0.5f;           // Surface roughness
    float metallic = 0.0f;            // Metallic property
    
    // Clouds (for applicable planets)
    float cloudCoverage = 0.0f;       // 0-1
    float cloudSpeed = 0.0f;          // Rotation speed relative to surface
    
    // Rings (for gas giants)
    int ringTextureHandle = -1;
    float ringInnerRadius = 0.0f;     // km
    float ringOuterRadius = 0.0f;     // km
    float ringOpacity = 1.0f;
    
    // LOD settings
    int currentLOD = 0;               // 0 = highest detail
    float lodDistance0 = 100.0f;      // Distance thresholds for LOD
    float lodDistance1 = 500.0f;
    float lodDistance2 = 2000.0f;

    // Shader configuration (populated by SolarSystem when available)
    std::string surfaceVertexShader;      // Vertex shader for body surface
    std::string surfaceFragmentShader;    // Fragment shader for body surface
    std::string orbitVertexShader;        // Vertex shader for orbit visualization
    std::string orbitFragmentShader;      // Fragment shader for orbit visualization
};

/**
 * AtmosphereComponent: Atmospheric properties for planets
 */
struct AtmosphereComponent : public Component {
    float density = 1.225f;           // kg/m³ at surface (Earth = 1.225)
    float scaleHeight = 8.5f;         // km (thickness)
    float pressure = 101.325f;        // kPa at surface
    
    // Visual properties
    float colorR = 0.5f;
    float colorG = 0.7f;
    float colorB = 1.0f;
    float colorA = 0.3f;              // Atmosphere glow intensity
    
    // Composition (simplified)
    float oxygenRatio = 0.21f;        // For habitability
    float nitrogenRatio = 0.78f;
    float carbonDioxideRatio = 0.0004f;
    
    // Weather effects
    bool hasWeather = false;
    float cloudSpeed = 10.0f;         // m/s
    float weatherIntensity = 0.5f;    // For visual effects
};

/**
 * SpaceStationComponent: Properties specific to space stations
 */
struct SpaceStationComponent : public Component {
    enum class StationType {
        Trading,      // Commodity markets
        Military,     // Defense and security
        Research,     // Scientific facilities
        Mining,       // Ore processing
        Residential,  // Habitation
        Shipyard      // Ship construction and repair
    };
    
    StationType stationType = StationType::Trading;
    
    // Facilities
    int dockingPorts = 4;
    bool hasShipyard = false;
    bool hasRepairFacility = false;
    bool hasRefuelStation = true;
    bool hasMarket = false;
    
    // Population and resources
    int population = 1000;
    int maxPopulation = 5000;
    
    // Services (bit flags could be used for more services)
    std::vector<int> availableServices; // Service IDs
    
    // Economy
    int wealthLevel = 1;              // 1-5, affects prices and available goods
};

/**
 * SatelliteSystemComponent: Component for tracking a body's moons/satellites
 */
struct SatelliteSystemComponent : public Component {
    std::vector<unsigned int> satelliteEntities; // Entity IDs of moons/stations
    int moonCount = 0;
    int stationCount = 0;
};

/**
 * StarComponent: Star-specific properties
 */
struct StarComponent : public Component {
    enum class SpectralType {
        O,  // Blue, very hot, massive
        B,  // Blue-white, hot
        A,  // White, hot
        F,  // Yellow-white, medium
        G,  // Yellow, Sun-like
        K,  // Orange, cool
        M   // Red, cool, small
    };
    
    SpectralType spectralType = SpectralType::G;
    int spectralSubclass = 2;         // 0-9 (e.g., G2 for Sun)
    
    double luminosity = 1.0;          // Relative to Sun
    double surfaceTemperature = 5778.0; // Kelvin
    
    // Habitable zone boundaries
    double habitableZoneInner = 0.95; // AU
    double habitableZoneOuter = 1.37; // AU
    
    // Visual effects
    float coronaSize = 1.5f;          // Multiplier for corona render
    bool hasFlares = true;            // Solar flares
    float flareIntensity = 0.5f;
};

/**
 * AsteroidBeltComponent: Asteroid belt region (not individual asteroids)
 */
struct AsteroidBeltComponent : public Component {
    double innerRadius = 2.2;         // AU
    double outerRadius = 3.2;         // AU
    double thickness = 0.5;           // AU (vertical extent)
    
    enum class DensityLevel {
        Sparse,
        Moderate,
        Dense,
        VeryDense
    };
    DensityLevel density = DensityLevel::Moderate;
    
    enum class CompositionType {
        Metallic,   // Iron, nickel
        Rocky,      // Silicates
        Icy,        // Water ice, frozen volatiles
        Mixed       // Combination
    };
    CompositionType composition = CompositionType::Rocky;
    
    // Approximate count of significant asteroids
    int asteroidCount = 1000;
    
    // Resource richness (for mining gameplay)
    float resourceRichness = 0.5f;    // 0-1
};

/**
 * PlanetComponent: Planet-specific additional data
 */
struct PlanetComponent : public Component {
    // Geological activity
    bool isTectonicallyActive = false;
    bool hasVolcanism = false;
    
    // Surface features
    bool hasOceans = false;
    float oceanCoverage = 0.0f;       // 0-1 (Earth = 0.71)
    bool hasIceCaps = false;
    float iceCoverage = 0.0f;
    
    // Biosphere
    bool hasLife = false;
    bool hasIntelligentLife = false;
    float biodiversityIndex = 0.0f;   // 0-1
    
    // Resources
    float mineralWealth = 0.5f;       // 0-1, mining value
    float organicResources = 0.0f;    // 0-1, biological resources
    
    // Surface conditions
    float gravity = 9.81f;            // m/s² (Earth = 9.81)
    float radiationLevel = 0.0f;      // Sieverts/hour (hazard level)
};

/**
 * OrbitalPosition: Result of orbital position calculation
 */
struct OrbitalPosition {
    Vector3 position;
    Vector3 velocity;
    double trueAnomaly = 0.0;         // Current angle in orbit
    double distance = 0.0;            // Distance from parent
    bool isValid = false;
};

/**
 * GenerationParameters: Parameters for procedural celestial body generation
 */
struct GenerationParameters {
    unsigned int seed = 0;
    
    // System-wide parameters
    int minPlanets = 3;
    int maxPlanets = 10;
    float gasGiantProbability = 0.4f;
    float asteroidBeltProbability = 0.7f;
    float moonProbability = 0.6f;     // For rocky planets
    
    // Station generation
    int minStations = 2;
    int maxStations = 8;
    float stationNearHabitableProbability = 0.8f;
    
    // Visual variety
    bool generateRings = true;
    bool generateAtmospheres = true;
    bool generateMoons = true;
};
