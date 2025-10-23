#pragma once
#include "Component.h"
#include <limits>
#include <string>
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
    bool thrustMode = false;
    double cameraYaw = 0.0;
    double facingYaw = 0.0;  // Player's facing direction for camera following
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

struct LocomotionStateMachine : public Component {
    enum class State {
        Idle,
        Walk,
        Sprint,
        Airborne,
        Landing
    };

    struct Weights {
        double idle = 1.0;
        double walk = 0.0;
        double sprint = 0.0;
        double airborne = 0.0;
        double landing = 0.0;
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
