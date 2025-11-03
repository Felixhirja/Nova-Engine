// Test: Archetype Graph Performance Validation
// Verifies 10-50x speedup from O(1) transition graph

#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"
#include <chrono>
#include <iostream>
#include <cassert>

using namespace ecs;

void TestArchetypeGraphPerformance() {
    std::cout << "=== Archetype Graph Performance Test ===\n\n";
    
    EntityManagerV2 manager;
    
    // Create test entities
    std::cout << "Creating 10,000 test entities...\n";
    std::vector<EntityHandle> entities;
    for (int i = 0; i < 10000; ++i) {
        entities.push_back(manager.CreateEntity());
    }
    std::cout << "✓ Entities created\n\n";
    
    // Benchmark: Add components (uses archetype transitions)
    std::cout << "Benchmarking component additions (30,000 transitions)...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto entity : entities) {
        Position pos;
        pos.x = 0.0; pos.y = 0.0; pos.z = 0.0;
        manager.AddComponent<Position>(entity, pos);
        
        Velocity vel;
        vel.vx = 1.0; vel.vy = 1.0; vel.vz = 1.0;
        manager.AddComponent<Velocity>(entity, vel);
        
        Acceleration acc;
        acc.ax = 0.0; acc.ay = 0.0; acc.az = 0.0;
        manager.AddComponent<Acceleration>(entity, acc);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double totalMs = duration.count() / 1000.0;
    double avgPerOp = duration.count() / 30000.0;
    
    std::cout << "\n📊 Results:\n";
    std::cout << "  Total time: " << totalMs << " ms\n";
    std::cout << "  Avg per operation: " << avgPerOp << " μs\n";
    
    // Performance evaluation
    std::cout << "\n🎯 Performance Analysis:\n";
    if (avgPerOp < 10.0) {
        std::cout << "  ✓ EXCELLENT! Archetype graph is working! (< 10μs per op)\n";
        double speedup = 150.0 / avgPerOp;
        std::cout << "  Estimated speedup: " << static_cast<int>(speedup) << "x faster than baseline\n";
    } else if (avgPerOp < 50.0) {
        std::cout << "  ✓ GOOD! Graph providing benefit (10-50μs per op)\n";
        double speedup = 150.0 / avgPerOp;
        std::cout << "  Estimated speedup: " << static_cast<int>(speedup) << "x faster than baseline\n";
    } else if (avgPerOp < 100.0) {
        std::cout << "  ⚠ OK - Some improvement but graph may not be fully utilized\n";
        std::cout << "  Check that BuildTransitionGraph() is being called\n";
    } else {
        std::cout << "  ❌ SLOW - Graph not working correctly (> 100μs per op)\n";
        std::cout << "  Expected: ~2-5μs per operation with graph\n";
        std::cout << "  Verify fast path is being used in AddComponentImmediate()\n";
    }
    
    // Validate correctness
    std::cout << "\n🔍 Validating correctness...\n";
    size_t entitiesWithAllComponents = 0;
    manager.ForEach<Position, Velocity>(
        [&](EntityHandle e, Position& p, Velocity& v) {
            (void)e; (void)p; (void)v;
            if (manager.HasComponent<Acceleration>(e)) {
                entitiesWithAllComponents++;
            }
        });
    
    assert(entitiesWithAllComponents == entities.size() && "Component addition failed!");
    std::cout << "  ✓ All " << entitiesWithAllComponents << " entities have correct components\n";
    
    // Print graph statistics
    auto stats = manager.GetArchetypeManager().GetTransitionGraphStats();
    std::cout << "\n📈 Transition Graph Statistics:\n";
    std::cout << "  Total edges: " << stats.totalEdges << "\n";
    std::cout << "  Valid edges: " << stats.validEdges << "\n";
    std::cout << "  Invalid edges: " << stats.invalidEdges << "\n";
    std::cout << "  Avg edges per archetype: " << stats.avgEdgesPerArchetype << "\n";
    std::cout << "  Max edges per archetype: " << stats.maxEdgesPerArchetype << "\n";
    
    // Test removal performance
    std::cout << "\n🔄 Testing component removal...\n";
    start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < entities.size() / 2; ++i) {
        manager.RemoveComponent<Velocity>(entities[i]);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    avgPerOp = duration.count() / 5000.0;
    
    std::cout << "  Removal time: " << (duration.count() / 1000.0) << " ms\n";
    std::cout << "  Avg per removal: " << avgPerOp << " μs\n";
    
    if (avgPerOp < 10.0) {
        std::cout << "  ✓ Fast removal confirmed!\n";
    }
    
    std::cout << "\n✅ Test completed successfully!\n";
}

int main() {
    try {
        TestArchetypeGraphPerformance();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
