#pragma once
#include "Component.h"
#include "EntityHandle.h"
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Position : public Component {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Velocity : public Component {
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
};

struct Sprite : public Component {
    int textureHandle = 0;
    int layer = 0;
    int frame = 0;
};

struct Acceleration : public Component {
    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;
};

struct Transform2D : public Component {
    double x = 0.0;
    double y = 0.0;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
};

struct PhysicsBody : public Component {
    double mass = 1.0;
    double drag = 0.0;
    bool affectedByGravity = true;
};

struct Hitbox : public Component {
    double width = 1.0;
    double height = 1.0;
};

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

struct Name : public Component {
    std::string value;
};

struct Faction : public Component {
    int id = 0;
};

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
    double cameraYaw = 0.0;
    double facingYaw = 0.0;  // Player's facing direction for camera following
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

struct EnvironmentSurface : public Component {
    LocomotionSurfaceType surfaceType = LocomotionSurfaceType::PlanetaryGround;
    bool overridesProfile = false;
    SurfaceMovementProfile movementProfile;
    bool isHazard = false;
    HazardModifier hazardModifier;
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

struct DockingStatus : public Component {
    bool isDocked = false;
    std::string portId;
    double alignmentScore = 0.0;
    double lastContactTime = 0.0;
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

struct TargetLock : public Component {
    unsigned int targetEntityId = 0;  // Entity ID to lock onto (0 = no target)
    bool isLocked = false;            // Whether target lock is active
    double offsetX = 0.0;             // Camera offset from target
    double offsetY = 5.0;             // Camera offset from target
    double offsetZ = 10.0;            // Camera offset from target
    double followDistance = 15.0;     // Distance to maintain from target
    double followHeight = 5.0;        // Height above target
};

struct Projectile : public Component {
    int ownerEntity = 0;
    std::string weaponSlot;
};

struct DamagePayload : public Component {
    double amount = 0.0;
    int sourceEntity = 0;
};

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

// =========================================================================
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

/**
 * Navigation Components for AI and pathfinding
 */

// Basic 3D point/vector for spatial calculations
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

struct PatrolRoute : public Component {
    std::vector<Point3D> waypoints;
    size_t currentWaypointIndex = 0;
    float arrivalThreshold = 10.0f; // Distance to consider waypoint reached
};

struct NavigationState : public Component {
    Point3D targetPosition;
    float throttle = 0.0f;
    double yaw = 0.0;
    double pitch = 0.0;
    bool hasTarget = false;
};

enum class AIState {
    Idle,
    Patrolling,
    Trading,
    Hunting,
    Fleeing,
    Docked
};

struct AIBehavior : public Component {
    AIState currentState = AIState::Idle;
    float stateTimer = 0.0f;
    float decisionTimer = 0.0f;
    ecs::EntityHandle targetEntity = ecs::EntityHandle::Null();
    float aggressionLevel = 0.5f; // 0.0 = peaceful, 1.0 = aggressive
    float cautionLevel = 0.5f;    // 0.0 = reckless, 1.0 = cautious
};

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

// =============================================================================
// SHIP SYSTEMS COMPONENTS (Data-only; no side effects)
// =============================================================================

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

// =============================================================================
// SHIP ASSEMBLY AS ECS COMPONENTS
// =============================================================================

// Desired assembly spec attached to an entity (data-only)
struct ShipAssemblySpec : public Component {
    std::string hullId;                                           // Target hull blueprint id
    std::unordered_map<std::string, std::string> slotAssignments; // slotId -> componentId
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
