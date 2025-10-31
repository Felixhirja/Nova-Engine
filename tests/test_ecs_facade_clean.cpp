#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/EntityManagerFacade.h"
#include "../engine/ecs/Components.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <string>

using namespace std::chrono;

// Simple performance test for facade operations
void TestFacadePerformanceImpact() {
    std::cout << "Testing Facade Performance Impact..." << std::endl;

    const size_t entityCount = 10000;

    // Test without facade
    {
        EntityManager em;
        auto start = high_resolution_clock::now();

        std::vector<Entity> entities;
        for (size_t i = 0; i < entityCount; ++i) {
            Entity entity = em.CreateEntity();
            entities.push_back(entity);
            auto pos = std::make_shared<Position>();
            pos->x = (double)i;
            pos->y = (double)i * 2.0;
            em.AddComponent<Position>(entity, pos);
            auto vel = std::make_shared<Velocity>();
            vel->vx = 1.0;
            vel->vy = 2.0;
            em.AddComponent<Velocity>(entity, vel);
        }

        auto createEnd = high_resolution_clock::now();

        // Update loop
        for (int frame = 0; frame < 100; ++frame) {
            em.ForEach<Position, Velocity>([](Entity entity, Position& pos, Velocity& vel) {
                pos.x += vel.vx * 0.016;
                pos.y += vel.vy * 0.016;
            });
        }

        auto updateEnd = high_resolution_clock::now();

        auto createTime = duration_cast<milliseconds>(createEnd - start);
        auto updateTime = duration_cast<milliseconds>(updateEnd - createEnd);

        std::cout << "  Without facade:" << std::endl;
        std::cout << "    Create time: " << createTime.count() << "ms" << std::endl;
        std::cout << "    Update time: " << updateTime.count() << "ms" << std::endl;
    }

    // Test with facade enabled
    {
        EntityManager em;
        em.EnableArchetypeFacade();
        auto start = high_resolution_clock::now();

        std::vector<Entity> entities;
        for (size_t i = 0; i < entityCount; ++i) {
            Entity entity = em.CreateEntity();
            entities.push_back(entity);
            auto pos = std::make_shared<Position>();
            pos->x = (double)i;
            pos->y = (double)i * 2.0;
            em.AddComponent<Position>(entity, pos);
            auto vel = std::make_shared<Velocity>();
            vel->vx = 1.0;
            vel->vy = 2.0;
            em.AddComponent<Velocity>(entity, vel);
        }

        auto createEnd = high_resolution_clock::now();

        // Update loop
        for (int frame = 0; frame < 100; ++frame) {
            em.ForEach<Position, Velocity>([](Entity entity, Position& pos, Velocity& vel) {
                pos.x += vel.vx * 0.016;
                pos.y += vel.vy * 0.016;
            });
        }

        auto updateEnd = high_resolution_clock::now();

        auto createTime = duration_cast<milliseconds>(createEnd - start);
        auto updateTime = duration_cast<milliseconds>(updateEnd - createEnd);

        std::cout << "  With facade:" << std::endl;
        std::cout << "    Create time: " << createTime.count() << "ms" << std::endl;
        std::cout << "    Update time: " << updateTime.count() << "ms" << std::endl;
    }

    std::cout << "  ✅ Performance impact measured" << std::endl;
}

// Test migration edge cases with mixed component types
void TestMigrationEdgeCases() {
    std::cout << "\nTesting Migration Edge Cases..." << std::endl;

    EntityManager em;

    // Create entities with mixed supported/unsupported components
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    // Entity 1: Only supported components
    auto pos1 = std::make_shared<Position>();
    pos1->x = 1.0; pos1->y = 2.0; pos1->z = 3.0;
    em.AddComponent<Position>(e1, pos1);
    auto vel1 = std::make_shared<Velocity>();
    vel1->vx = 0.1; vel1->vy = 0.2; vel1->vz = 0.3;
    em.AddComponent<Velocity>(e1, vel1);

    // Entity 2: Mixed supported and unsupported
    auto pos2 = std::make_shared<Position>();
    pos2->x = 4.0; pos2->y = 5.0; pos2->z = 6.0;
    em.AddComponent<Position>(e2, pos2);
    auto projectile = std::make_shared<Projectile>();
    projectile->ownerEntity = 42;
    em.AddComponent<Projectile>(e2, projectile);  // Unsupported

    // Entity 3: Only unsupported components
    auto playerController = std::make_shared<PlayerController>();
    playerController->moveForward = true;
    em.AddComponent<PlayerController>(e3, playerController);  // Unsupported

    // Enable facade
    em.EnableArchetypeFacade();

    // Verify supported components migrated
    assert(em.HasComponent<Position>(e1));
    assert(em.HasComponent<Velocity>(e1));
    auto* migratedPos1 = em.GetComponent<Position>(e1);
    auto* migratedVel1 = em.GetComponent<Velocity>(e1);
    assert(migratedPos1->x == 1.0 && migratedPos1->y == 2.0 && migratedPos1->z == 3.0);
    assert(migratedVel1->vx == 0.1 && migratedVel1->vy == 0.2 && migratedVel1->vz == 0.3);

    // Verify mixed entity: supported migrated, unsupported preserved
    assert(em.HasComponent<Position>(e2));
    assert(!em.HasComponent<Velocity>(e2));  // Wasn't added
    assert(em.HasComponent<Projectile>(e2));  // Unsupported preserved
    auto* migratedPos2 = em.GetComponent<Position>(e2);
    assert(migratedPos2->x == 4.0 && migratedPos2->y == 5.0 && migratedPos2->z == 6.0);

    // Verify unsupported-only entity: no archetype migration
    assert(!em.HasComponent<Position>(e3));
    assert(em.HasComponent<PlayerController>(e3));  // Unsupported preserved

    // Test operations on migrated entities
    auto acc1 = std::make_shared<Acceleration>();
    acc1->ax = 1.0; acc1->ay = 2.0; acc1->az = 3.0;
    em.AddComponent<Acceleration>(e1, acc1);  // Should go to archetype

    assert(em.HasComponent<Acceleration>(e1));
    auto* migratedAcc1 = em.GetComponent<Acceleration>(e1);
    assert(migratedAcc1->ax == 1.0 && migratedAcc1->ay == 2.0 && migratedAcc1->az == 3.0);

    // Test ForEach works on migrated entities
    size_t count = 0;
    em.ForEach<Position, Velocity>([&count](Entity entity, Position& pos, Velocity& vel) {
        count++;
        pos.x += vel.vx;
        pos.y += vel.vy;
    });
    assert(count == 1);  // Only e1 has both Position and Velocity

    std::cout << "  ✅ Mixed component migration handled correctly" << std::endl;
}

// Test facade memory usage and cleanup
void TestFacadeMemoryManagement() {
    std::cout << "\nTesting Facade Memory Management..." << std::endl;

    EntityManager em;

    // Create entities before facade
    std::vector<Entity> entities;
    for (int i = 0; i < 100; ++i) {
        Entity entity = em.CreateEntity();
        entities.push_back(entity);
        auto pos = std::make_shared<Position>();
        pos->x = (double)i;
        em.AddComponent<Position>(entity, pos);
    }

    // Enable facade
    em.EnableArchetypeFacade();

    // Add more components (should use archetype storage)
    for (size_t i = 0; i < entities.size(); ++i) {
        auto vel = std::make_shared<Velocity>();
        vel->vx = (double)i + 1.0;  // Set vx to pos.x + 1 so assertion holds
        em.AddComponent<Velocity>(entities[i], vel);
    }

    // Verify all entities accessible
    size_t count = 0;
    em.ForEach<Position, Velocity>([&count](Entity entity, Position& pos, Velocity& vel) {
        count++;
        assert(pos.x == vel.vx - 1.0);  // x should equal vx - 1
    });
    assert(count == 100);

    // Destroy some entities
    for (size_t i = 0; i < 50; ++i) {
        em.DestroyEntity(entities[i]);
    }

    // Verify remaining entities still work
    count = 0;
    em.ForEach<Position>([&count](Entity entity, Position& pos) {
        count++;
    });
    assert(count == 50);  // Half should remain

    std::cout << "  ✅ Memory management works correctly" << std::endl;
}

// Test unsupported component detection
void TestUnsupportedComponentDetection() {
    std::cout << "\nTesting Unsupported Component Detection..." << std::endl;

    EntityManager em;

    // Create an entity with an unsupported component before enabling facade
    Entity entity = em.CreateEntity();
    auto projectile = std::make_shared<Projectile>();
    projectile->ownerEntity = 123;
    em.AddComponent<Projectile>(entity, projectile);

    // Create another entity with a truly unsupported component (Faction)
    Entity entity2 = em.CreateEntity();
    auto faction = std::make_shared<Faction>();
    faction->id = 456;
    em.AddComponent<Faction>(entity2, faction);

    // Get unsupported types before facade
    auto unsupportedBefore = em.GetUnsupportedComponentTypes();
    assert(unsupportedBefore.empty());  // Should be empty initially

    // Enable facade
    em.EnableArchetypeFacade();

    // Get unsupported types after facade
    auto unsupportedAfter = em.GetUnsupportedComponentTypes();

    // Should contain Faction (unsupported) but not Projectile (now supported)
    assert(unsupportedAfter.find(std::type_index(typeid(Faction))) != unsupportedAfter.end());
    assert(unsupportedAfter.find(std::type_index(typeid(Projectile))) == unsupportedAfter.end());  // Now supported!

    // Check that supported types are not in unsupported list
    assert(unsupportedAfter.find(std::type_index(typeid(Position))) == unsupportedAfter.end());
    assert(unsupportedAfter.find(std::type_index(typeid(Velocity))) == unsupportedAfter.end());
    assert(unsupportedAfter.find(std::type_index(typeid(Acceleration))) == unsupportedAfter.end());
    assert(unsupportedAfter.find(std::type_index(typeid(PhysicsMaterial))) == unsupportedAfter.end());  // Now supported!

    // Verify we have exactly one unsupported type (Faction)
    assert(unsupportedAfter.size() == 1);

    std::cout << "  ✅ Detected " << unsupportedAfter.size() << " unsupported component types" << std::endl;
}

// Test facade with EntityManagerFacade wrapper
void TestFacadeWrapper() {
    std::cout << "\nTesting EntityManagerFacade Wrapper..." << std::endl;

    EntityManager em;
    ecs::EntityManagerFacade facade(em);

    // Create entity through facade
    Entity entity = facade.CreateEntity();

    // Add component through facade
    auto name = std::make_shared<Name>();
    name->value = "TestEntity";
    facade.AddComponent<Name>(entity, name);

    // Verify through facade
    assert(facade.HasComponent<Name>(entity));
    auto* retrieved = facade.GetComponent<Name>(entity);
    assert(retrieved != nullptr);
    assert(retrieved->value == "TestEntity");

    // Modify through facade
    retrieved->value = "Modified";

    // Verify modification
    auto* modified = facade.GetComponent<Name>(entity);
    assert(modified->value == "Modified");

    // Test ForEach through facade
    size_t count = 0;
    facade.ForEach<Name>([&count, &entity](Entity e, Name& n) {
        if (e == entity) {
            count++;
            n.value = "Iterated";
        }
    });
    assert(count == 1);

    // Verify change persisted
    auto* iterated = facade.GetComponent<Name>(entity);
    assert(iterated->value == "Iterated");

    // Remove through facade
    facade.RemoveComponent<Name>(entity);
    assert(!facade.HasComponent<Name>(entity));

    std::cout << "  ✅ EntityManagerFacade wrapper works correctly" << std::endl;
}

int main() {
    std::cout << "=== ECS Compatibility Facade Tests ===" << std::endl;
    std::cout << std::endl;

    try {
        TestFacadePerformanceImpact();
        TestMigrationEdgeCases();
        TestFacadeMemoryManagement();
        TestUnsupportedComponentDetection();
        TestFacadeWrapper();

        std::cout << "\n==================================" << std::endl;
        std::cout << "✅ ALL FACADE TESTS PASSED!" << std::endl;
        std::cout << "==================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}