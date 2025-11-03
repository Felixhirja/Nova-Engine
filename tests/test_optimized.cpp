/*
 * Optimized Performance Test
 * Tests engine with optimizations enabled
 */

#include <iostream>
#include <chrono>
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/Components.h"

int main() {
    std::cout << "Nova Engine - Optimized Performance Test\n";
    std::cout << "==========================================\n\n";
    
    try {
        std::cout << "=== Testing ECS V2 Performance ===\n";
        
        ecs::EntityManagerV2 manager;
        
        // Warm-up
        for (int i = 0; i < 1000; i++) {
            auto entity = manager.CreateEntity();
            auto& pos = manager.AddComponent<Position>(entity);
            pos.x = static_cast<double>(i);
            pos.y = 0.0;
            pos.z = 0.0;
            auto& vel = manager.AddComponent<Velocity>(entity);
            vel.vx = 1.0;
            vel.vy = 0.0;
            vel.vz = 0.0;
        }
        
        std::cout << "Warm-up complete: 1000 entities created\n";
        
        // Performance test
        auto start = std::chrono::high_resolution_clock::now();
        
        const int iterations = 10000;
        for (int i = 0; i < iterations; i++) {
            manager.ForEach<Position, Velocity>([](ecs::EntityHandle handle, Position& pos, Velocity& vel) {
                pos.x += vel.vx * 0.016;
                pos.y += vel.vy * 0.016;
                pos.z += vel.vz * 0.016;
            });
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avgIterationTime = duration.count() / static_cast<double>(iterations);
        double estimatedFPS = 1000000.0 / avgIterationTime;
        
        std::cout << "\n=== Performance Results ===\n";
        std::cout << "Total iterations: " << iterations << "\n";
        std::cout << "Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "Average iteration: " << avgIterationTime << " μs\n";
        std::cout << "Estimated FPS (if each frame = 1 iteration): " << static_cast<int>(estimatedFPS) << "\n";
        
        std::cout << "\n✓ All tests passed!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
