#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/PhysicsSystem.h"
#include "../engine/ecs/Components.h"

#include <cmath>
#include <iostream>

namespace {

bool approx(double a, double b, double eps = 1e-4) {
    return std::fabs(a - b) <= eps;
}

bool TestImpulseApplication() {
    EntityManager entityManager;
    PhysicsSystem physics(&entityManager);
    physics.SetGravity(0.0, 0.0, 0.0);
    physics.SetCollisionEnabled(false);

    Entity entity = entityManager.CreateEntity();
    auto& rigidBody = entityManager.EmplaceComponent<RigidBody>(entity);
    rigidBody.SetMass(2.0);
    rigidBody.useGravity = false;
    auto& velocity = entityManager.EmplaceComponent<Velocity>(entity);
    velocity.vx = 0.0;

    physics.ApplyImpulse(entity, 10.0, 0.0, 0.0);
    physics.Update(entityManager, 0.016);

    if (!approx(velocity.vx, 5.0, 1e-3)) {
        std::cerr << "Impulse did not apply expected velocity change: " << velocity.vx << std::endl;
        return false;
    }
    if (entityManager.HasComponent<Force>(entity)) {
        std::cerr << "Impulse force component was not cleared after update" << std::endl;
        return false;
    }
    return true;
}

bool TestConstantForceAcceleration() {
    EntityManager entityManager;
    PhysicsSystem physics(&entityManager);
    physics.SetGravity(0.0, 0.0, 0.0);
    physics.SetCollisionEnabled(false);

    Entity entity = entityManager.CreateEntity();
    auto& rigidBody = entityManager.EmplaceComponent<RigidBody>(entity);
    rigidBody.SetMass(5.0);
    rigidBody.useGravity = false;
    auto& velocity = entityManager.EmplaceComponent<Velocity>(entity);
    auto& constantForce = entityManager.EmplaceComponent<ConstantForce>(entity);
    constantForce.forceX = 50.0;

    physics.Update(entityManager, 0.5);

    double expected = (constantForce.forceX * rigidBody.inverseMass) * 0.5;
    if (!approx(velocity.vx, expected, 1e-3)) {
        std::cerr << "Constant force produced unexpected velocity: " << velocity.vx
                  << " expected " << expected << std::endl;
        return false;
    }
    return true;
}

bool TestPointGravitySource() {
    EntityManager entityManager;
    PhysicsSystem physics(&entityManager);
    physics.SetGravity(0.0, 0.0, 0.0);
    physics.SetCollisionEnabled(false);

    Entity sourceEntity = entityManager.CreateEntity();
    auto& sourcePos = entityManager.EmplaceComponent<Position>(sourceEntity);
    sourcePos.x = 0.0;
    sourcePos.y = 0.0;
    sourcePos.z = 0.0;
    auto& gravitySource = entityManager.EmplaceComponent<GravitySource>(sourceEntity);
    gravitySource.strength = 9.8;
    gravitySource.radius = 0.0; // Infinite range
    gravitySource.isUniform = false;

    Entity entity = entityManager.CreateEntity();
    auto& position = entityManager.EmplaceComponent<Position>(entity);
    position.x = 0.0;
    position.y = 0.0;
    position.z = 10.0;
    auto& rigidBody = entityManager.EmplaceComponent<RigidBody>(entity);
    rigidBody.SetMass(1.0);
    rigidBody.useGravity = true;
    auto& velocity = entityManager.EmplaceComponent<Velocity>(entity);

    physics.Update(entityManager, 1.0);

    if (!(velocity.vz < 0.0)) {
        std::cerr << "Gravity source failed to accelerate entity toward origin" << std::endl;
        return false;
    }
    double expectedVz = -0.098; // strength / distance^2 at 10 units
    if (!approx(velocity.vz, expectedVz, 5e-3)) {
        std::cerr << "Gravity source produced unexpected velocity: " << velocity.vz
                  << " expected near " << expectedVz << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main() {
    if (!TestImpulseApplication()) {
        return 1;
    }
    if (!TestConstantForceAcceleration()) {
        return 2;
    }
    if (!TestPointGravitySource()) {
        return 3;
    }

    std::cout << "Physics behavior tests passed." << std::endl;
    return 0;
}
