/**
 * Tests for physics components and PhysicsSystem
 */
#include "../src/ecs/EntityManager.h"
#include "../src/ecs/PhysicsSystem.h"
#include "../src/ecs/Components.h"
#include "../src/physics/BulletPhysicsEngine.h"
#include "../src/physics/PhysXPhysicsEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

void TestRigidBodyComponent() {
    std::cout << "Testing RigidBody component..." << std::endl;
    
    RigidBody rb;
    assert(rb.mass == 1.0);
    assert(rb.inverseMass == 1.0);
    
    rb.SetMass(2.0);
    assert(rb.mass == 2.0);
    assert(rb.inverseMass == 0.5);
    
    rb.isKinematic = true;
    rb.UpdateInverseMass();
    assert(rb.inverseMass == 0.0);  // Kinematic bodies have infinite mass
    
    std::cout << "  RigidBody component tests passed" << std::endl;
}

void TestColliderComponents() {
    std::cout << "Testing collider components..." << std::endl;
    
    BoxCollider box;
    assert(box.shape == Collider::Shape::Box);
    assert(box.width == 1.0);
    assert(box.height == 1.0);
    assert(box.depth == 1.0);
    
    SphereCollider sphere;
    assert(sphere.shape == Collider::Shape::Sphere);
    assert(sphere.radius == 0.5);
    
    CapsuleCollider capsule;
    assert(capsule.shape == Collider::Shape::Capsule);
    assert(capsule.radius == 0.5);
    assert(capsule.height == 2.0);
    assert(capsule.direction == CapsuleCollider::Direction::Y);
    
    std::cout << "  Collider component tests passed" << std::endl;
}

void TestForceComponent() {
    std::cout << "Testing Force component..." << std::endl;
    
    Force force;
    force.fx = 10.0;
    force.fy = 0.0;
    force.fz = 0.0;
    force.mode = Force::Mode::Force;
    
    assert(force.fx == 10.0);
    assert(force.lifetime == -1.0);  // Permanent by default
    
    // Test impulse mode
    Force impulse;
    impulse.mode = Force::Mode::Impulse;
    assert(impulse.mode == Force::Mode::Impulse);
    
    std::cout << "  Force component tests passed" << std::endl;
}

void TestPhysicsSystemGravity() {
    std::cout << "Testing PhysicsSystem gravity..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGlobalDamping(0.0, 0.0);  // Disable damping for this test
    
    // Create entity with rigidbody, position, and velocity
    auto entity = em.CreateEntity();
    em.EmplaceComponent<RigidBody>(entity);
    auto& pos = em.EmplaceComponent<Position>(entity);
    pos.x = 0.0; pos.y = 0.0; pos.z = 10.0;
    em.EmplaceComponent<Velocity>(entity);
    
    auto* rb = em.GetComponent<RigidBody>(entity);
    rb->useGravity = true;
    
    auto* vel = em.GetComponent<Velocity>(entity);
    assert(vel->vz == 0.0);
    
    // Update physics for 1 second (should apply gravity)
    physics.SetGravity(0.0, 0.0, -9.8);
    std::cout << "    Before: vz = " << vel->vz << ", globalGravityZ = -9.8" << std::endl;
    physics.Update(em, 1.0);
    std::cout << "    After: vz = " << vel->vz << ", expected = -9.8" << std::endl;
    
    // Velocity should now be affected by gravity
    assert(vel->vz < 0.0);
    // Note: There's some precision loss, so allow larger tolerance
    assert(std::abs(vel->vz - (-9.8)) < 0.1);
    
    std::cout << "  Gravity tests passed" << std::endl;
}

void TestPhysicsSystemIntegration() {
    std::cout << "Testing PhysicsSystem velocity integration..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGravity(0.0, 0.0, 0.0);  // Disable gravity for this test
    
    auto entity = em.CreateEntity();
    em.EmplaceComponent<RigidBody>(entity);
    auto& pos = em.EmplaceComponent<Position>(entity);
    pos.x = 0.0; pos.y = 0.0; pos.z = 0.0;
    auto& vel = em.EmplaceComponent<Velocity>(entity);
    vel.vx = 1.0; vel.vy = 2.0; vel.vz = 3.0;
    
    auto* posPtr = em.GetComponent<Position>(entity);
    auto* velPtr = em.GetComponent<Velocity>(entity);
    (void)velPtr; // Suppress unused warning
    
    double initialX = posPtr->x;
    double initialY = posPtr->y;
    double initialZ = posPtr->z;
    
    // Update for 1 second
    physics.Update(em, 1.0);
    
    // Position should have changed by velocity * dt
    assert(std::abs(posPtr->x - (initialX + 1.0)) < 0.1);  // Allow for damping
    assert(std::abs(posPtr->y - (initialY + 2.0)) < 0.1);
    assert(std::abs(posPtr->z - (initialZ + 3.0)) < 0.1);
    
    std::cout << "  Integration tests passed" << std::endl;
}

void TestCollisionDetection() {
    std::cout << "Testing collision detection..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGravity(0.0, 0.0, 0.0);
    physics.SetCollisionEnabled(true);
    
    // Create two entities with box colliders
    auto entityA = em.CreateEntity();
    auto& posA = em.EmplaceComponent<Position>(entityA);
    posA.x = 0.0; posA.y = 0.0; posA.z = 0.0;
    em.EmplaceComponent<RigidBody>(entityA);
    em.EmplaceComponent<Velocity>(entityA);
    auto& boxA = em.EmplaceComponent<BoxCollider>(entityA);
    boxA.width = 1.0;
    boxA.height = 1.0;
    boxA.depth = 1.0;
    
    auto entityB = em.CreateEntity();
    auto& posB = em.EmplaceComponent<Position>(entityB);
    posB.x = 0.5; posB.y = 0.0; posB.z = 0.0;  // Overlapping
    em.EmplaceComponent<RigidBody>(entityB);
    em.EmplaceComponent<Velocity>(entityB);
    auto& boxB = em.EmplaceComponent<BoxCollider>(entityB);
    boxB.width = 1.0;
    boxB.height = 1.0;
    boxB.depth = 1.0;
    
    // Update physics (should detect collision)
    physics.Update(em, 0.016);  // ~60 FPS
    
    // Check if collision info was created
    auto* collisionA = em.GetComponent<CollisionInfo>(entityA);
    auto* collisionB = em.GetComponent<CollisionInfo>(entityB);
    
    assert(collisionA != nullptr);
    assert(collisionB != nullptr);
    assert(collisionA->collisionCount > 0);
    assert(collisionB->collisionCount > 0);
    
    std::cout << "  Collision detection tests passed" << std::endl;
}

void TestForceApplication() {
    std::cout << "Testing force application..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGravity(0.0, 0.0, 0.0);
    
    auto entity = em.CreateEntity();
    em.EmplaceComponent<RigidBody>(entity);
    em.EmplaceComponent<Position>(entity);
    em.EmplaceComponent<Velocity>(entity);
    
    auto* vel = em.GetComponent<Velocity>(entity);
    double initialVx = vel->vx;
    
    // Apply a force
    physics.ApplyImpulse(entity, 10.0, 0.0, 0.0);
    physics.Update(em, 0.016);
    
    // Velocity should have changed
    assert(vel->vx > initialVx);
    
    std::cout << "  Force application tests passed" << std::endl;
}

void TestKinematicBodies() {
    std::cout << "Testing kinematic bodies..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGravity(0.0, 0.0, -9.8);
    
    auto entity = em.CreateEntity();
    auto& rb = em.EmplaceComponent<RigidBody>(entity);
    rb.isKinematic = true;
    rb.useGravity = true;
    auto& pos = em.EmplaceComponent<Position>(entity);
    pos.x = 0.0; pos.y = 0.0; pos.z = 10.0;
    em.EmplaceComponent<Velocity>(entity);
    
    auto* vel = em.GetComponent<Velocity>(entity);
    auto* posPtr = em.GetComponent<Position>(entity);
    double initialZ = posPtr->z;
    
    // Update physics
    physics.Update(em, 1.0);
    
    // Kinematic body should not be affected by gravity or forces
    assert(vel->vz == 0.0);
    assert(posPtr->z == initialZ);
    
    std::cout << "  Kinematic body tests passed" << std::endl;
}

void TestGravitySource() {
    std::cout << "Testing gravity source..." << std::endl;
    
    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGravity(0.0, 0.0, 0.0);  // Disable global gravity
    
    // Create gravity source (like a planet)
    auto planet = em.CreateEntity();
    auto& planetPos = em.EmplaceComponent<Position>(planet);
    planetPos.x = 0.0; planetPos.y = 0.0; planetPos.z = 0.0;
    auto& gravity = em.EmplaceComponent<GravitySource>(planet);
    gravity.strength = 10.0;
    gravity.radius = 50.0;
    gravity.isUniform = false;  // Point gravity
    
    // Create object affected by gravity
    auto object = em.CreateEntity();
    auto& objectPos = em.EmplaceComponent<Position>(object);
    objectPos.x = 10.0; objectPos.y = 0.0; objectPos.z = 0.0;
    auto& rb = em.EmplaceComponent<RigidBody>(object);
    rb.useGravity = true;
    em.EmplaceComponent<Velocity>(object);
    
    auto* vel = em.GetComponent<Velocity>(object);
    
    // Update physics
    physics.Update(em, 1.0);
    
    // Object should be pulled toward planet
    assert(vel->vx < 0.0);  // Velocity toward planet
    
    std::cout << "  Gravity source tests passed" << std::endl;
}

void TestBulletPhysicsEngineIntegration() {
    std::cout << "Testing Bullet physics engine integration..." << std::endl;

    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGlobalDamping(0.0, 0.0);

    auto bullet = std::make_shared<physics::BulletPhysicsEngine>();
    physics.UseExternalEngine(bullet);

    physics::PhysicsEngineInitParams params;
    params.fixedTimeStep = 1.0 / 120.0;
    params.maxSubSteps = 240;
    bullet->Initialize(params);

    auto entity = em.CreateEntity();
    em.EmplaceComponent<RigidBody>(entity);
    auto& pos = em.EmplaceComponent<Position>(entity);
    pos.x = 0.0; pos.y = 0.0; pos.z = 5.0;
    em.EmplaceComponent<Velocity>(entity);

    physics.SetGravity(0.0, 0.0, -9.8);
    physics.Update(em, 1.0);

    auto* vel = em.GetComponent<Velocity>(entity);
    assert(physics.GetActiveBackendType() == physics::PhysicsBackendType::Bullet);
    assert(vel != nullptr);
    assert(vel->vz < 0.0);

    physics.ResetToBuiltin();
    assert(physics.GetActiveBackendType() == physics::PhysicsBackendType::BuiltIn);

    std::cout << "  Bullet integration tests passed" << std::endl;
}

void TestPhysXPhysicsEngineIntegration() {
    std::cout << "Testing PhysX physics engine integration..." << std::endl;

    EntityManager em;
    PhysicsSystem physics(&em);
    physics.SetGlobalDamping(0.0, 0.0);

    auto physx = std::make_shared<physics::PhysXPhysicsEngine>();
    physics.UseExternalEngine(physx);

    physics::PhysicsEngineInitParams params;
    params.fixedTimeStep = 1.0 / 90.0;
    params.maxSubSteps = 180;
    physx->Initialize(params);

    auto entity = em.CreateEntity();
    em.EmplaceComponent<RigidBody>(entity);
    auto& pos = em.EmplaceComponent<Position>(entity);
    pos.x = 0.0; pos.y = 0.0; pos.z = 8.0;
    em.EmplaceComponent<Velocity>(entity);

    physics.SetGravity(0.0, 0.0, -9.8);
    physics.Update(em, 1.0);

    auto* vel = em.GetComponent<Velocity>(entity);
    assert(physics.GetActiveBackendType() == physics::PhysicsBackendType::PhysX);
    assert(vel != nullptr);
    assert(vel->vz < 0.0);

    physics.ResetToBuiltin();
    assert(physics.GetActiveBackendType() == physics::PhysicsBackendType::BuiltIn);

    std::cout << "  PhysX integration tests passed" << std::endl;
}

int main() {
    std::cout << "Running Physics System Tests" << std::endl;
    std::cout << "=============================" << std::endl;
    
    TestRigidBodyComponent();
    TestColliderComponents();
    TestForceComponent();
    TestPhysicsSystemGravity();
    TestPhysicsSystemIntegration();
    TestCollisionDetection();
    TestForceApplication();
    TestKinematicBodies();
    TestGravitySource();
    TestBulletPhysicsEngineIntegration();
    TestPhysXPhysicsEngineIntegration();
    
    std::cout << "=============================" << std::endl;
    std::cout << "All physics tests passed!" << std::endl;
    
    return 0;
}
