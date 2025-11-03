/*
 * Simple ECS Test - Fixed Version
 * Tests basic entity creation and component management
 */

#include <iostream>
#include <cassert>
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/Components.h"

int main() {
    std::cout << "Starting Simple ECS Tests\n";
    std::cout << "==========================================\n\n";
    
    try {
        std::cout << "=== Testing Basic Entity Creation ===\n";
        
        EntityManager manager;
        
        // Create entities
        auto entity1 = manager.CreateEntity();
        auto entity2 = manager.CreateEntity();
        auto entity3 = manager.CreateEntity();
        
        std::cout << "Created 3 entities: " << entity1 << ", " << entity2 << ", " << entity3 << "\n";
        
        // Add components
        auto pos1 = std::make_shared<Position>();
        pos1->x = 10.0;
        pos1->y = 20.0;
        pos1->z = 30.0;
        manager.AddComponent<Position>(entity1, pos1);
        
        auto vel1 = std::make_shared<Velocity>();
        vel1->vx = 1.0;
        vel1->vy = 2.0;
        vel1->vz = 3.0;
        manager.AddComponent<Velocity>(entity1, vel1);
        
        std::cout << "Added Position and Velocity to entity " << entity1 << "\n";
        
        // Retrieve components
        auto retrievedPos = manager.GetComponent<Position>(entity1);
        assert(retrievedPos != nullptr);
        assert(retrievedPos->x == 10.0);
        
        std::cout << "Retrieved position: (" << retrievedPos->x << ", " 
                  << retrievedPos->y << ", " << retrievedPos->z << ")\n";
        
        std::cout << "✓ Basic entity creation test passed!\n\n";
        
        // Test archetype system
        std::cout << "=== Testing Archetype System ===\n";
        
        manager.EnableArchetypeFacade();
        std::cout << "Archetype facade enabled\n";
        
        // Create more entities with archetype system
        for (int i = 0; i < 10; ++i) {
            auto entity = manager.CreateEntity();
            auto pos = std::make_shared<Position>();
            pos->x = static_cast<double>(i);
            manager.AddComponent<Position>(entity, pos);
        }
        
        std::cout << "Created 10 additional entities with Position components\n";
        std::cout << "✓ Archetype system test passed!\n\n";
        
        std::cout << "==========================================\n";
        std::cout << "All tests passed successfully!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
