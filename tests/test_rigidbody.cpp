#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include <iostream>

int main() {
    std::cout << "Testing RigidBody creation..." << std::endl;

    EntityManager em;

    // Enable archetype storage
    em.EnableArchetypeFacade();

    Entity entity = em.CreateEntity();

    // Test creating Position component first
    auto position = std::make_shared<Position>();
    position->x = 0.0;
    position->y = 0.0;
    position->z = 0.0;
    em.AddComponent<Position>(entity, position);
    std::cout << "Position component added successfully" << std::endl;

    // Test creating RigidBody component
    auto rigidBody = std::make_shared<RigidBody>();
    rigidBody->isKinematic = true;
    rigidBody->useGravity = false;
    rigidBody->linearDamping = 0.0;
    rigidBody->angularDamping = 0.0;
    rigidBody->UpdateInverseMass();
    em.AddComponent<RigidBody>(entity, rigidBody);
    std::cout << "RigidBody component added successfully" << std::endl;

    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}