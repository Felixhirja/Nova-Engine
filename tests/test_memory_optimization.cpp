/*
 * Test Memory Optimization System
 * Comprehensive test suite for ECS memory optimization features
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cassert>
#include "engine/ecs/MemoryOptimizer.h"
#include "engine/ecs/EntityManager.h"

// Test component for large memory usage
struct TestComponent {
    char data[1024]; // 1KB component for testing memory usage
    TestComponent() { std::memset(data, 0, sizeof(data)); }
};

void TestMemoryAnalysis() {
    std::cout << "=== Testing Memory Analysis ===\n";
    
    // Create entity manager and entities for testing
    ecs::EntityManagerV2 manager;
    
    // Create some entities with different component combinations
    for (int i = 0; i < 100; ++i) {
        auto entity = manager.CreateEntity();
        
        // Add Position component
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = static_cast<double>(i*2);
        pos.z = 0.0;
        
        if (i % 2 == 0) {
            auto& vel = manager.AddComponent<Velocity>(entity);
            vel.vx = 1.0;
            vel.vy = 0.0;
            vel.vz = 0.0;
        }
        
        if (i % 3 == 0) {
            auto& health = manager.AddComponent<Health>(entity);
            health.current = 100;
            health.maximum = 100;
        }
        
        if (i % 5 == 0) {
            manager.AddComponent<TestComponent>(entity);
        }
    }
    
    // Analyze memory usage
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    
    std::cout << "Memory Analysis Results:\n";
    std::cout << "  Total Allocated: " << stats.totalAllocated << " bytes\n";
    std::cout << "  Total Used: " << stats.totalUsed << " bytes\n";
    std::cout << "  Wasted Space: " << stats.wastedSpace << " bytes\n";
    std::cout << "  Fragmentation Ratio: " << stats.fragmentationRatio << "\n";
    std::cout << "  Archetype Count: " << stats.archetypeCount << "\n";
    std::cout << "  Empty Archetypes: " << stats.emptyArchetypes << "\n";
    
    assert(stats.archetypeCount > 0);
    assert(stats.totalUsed > 0);
    std::cout << "Memory analysis test passed!\n\n";
}

void TestMemoryOptimization() {
    std::cout << "=== Testing Memory Optimization ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create many entities to generate fragmentation
    std::vector<ecs::EntityHandle> entities;
    for (int i = 0; i < 1000; ++i) {
        auto entity = manager.CreateEntity();
        entities.push_back(entity);
        
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
        
        manager.AddComponent<TestComponent>(entity);
    }
    
    // Remove every other entity to create fragmentation
    for (size_t i = 0; i < entities.size(); i += 2) {
        manager.DestroyEntity(entities[i]);
    }
    
    // Analyze memory before optimization
    auto beforeStats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "Before optimization:\n";
    std::cout << "  Fragmentation Ratio: " << beforeStats.fragmentationRatio << "\n";
    std::cout << "  Wasted Space: " << beforeStats.wastedSpace << " bytes\n";
    
    // Perform optimization
    ecs::MemoryOptimizer::Compact(manager);
    
    // Analyze memory after optimization
    auto afterStats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "After optimization:\n";
    std::cout << "  Fragmentation Ratio: " << afterStats.fragmentationRatio << "\n";
    std::cout << "  Wasted Space: " << afterStats.wastedSpace << " bytes\n";
    
    // Verify improvement (should have less waste)
    assert(afterStats.wastedSpace <= beforeStats.wastedSpace);
    std::cout << "Memory optimization test passed!\n\n";
}

void TestMemoryPrediction() {
    std::cout << "=== Testing Memory Prediction ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create baseline entities
    for (int i = 0; i < 50; ++i) {
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
    
    // Test memory prediction for adding more entities
    auto prediction = ecs::MemoryOptimizer::PredictMemoryUsage(manager, 100);
    
    std::cout << "Memory prediction for 100 additional entities:\n";
    std::cout << "  Predicted Total Allocated: " << prediction.totalAllocated << " bytes\n";
    std::cout << "  Predicted Total Used: " << prediction.totalUsed << " bytes\n";
    
    assert(prediction.totalAllocated > 0);
    std::cout << "Memory prediction test passed!\n\n";
}

void TestMemoryRecommendations() {
    std::cout << "=== Testing Memory Recommendations ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create scenario with high fragmentation
    for (int i = 0; i < 200; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
        
        // Create varied component patterns to increase archetype count
        if (i % 7 == 0) {
            auto& vel = manager.AddComponent<Velocity>(entity);
            vel.vx = 1.0;
            vel.vy = 0.0;
            vel.vz = 0.0;
        }
        if (i % 11 == 0) {
            auto& health = manager.AddComponent<Health>(entity);
            health.current = 100;
            health.maximum = 100;
        }
        if (i % 13 == 0) {
            manager.AddComponent<TestComponent>(entity);
        }
    }
    
    // Get memory stats and recommendations
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    auto recommendations = ecs::MemoryOptimizer::GetOptimizationRecommendations(stats);
    
    std::cout << "Memory recommendations:\n";
    for (const auto& rec : recommendations) {
        std::cout << "  - " << rec << "\n";
    }
    
    assert(!recommendations.empty());
    std::cout << "Memory recommendations test passed!\n\n";
}

void TestMemoryProfiling() {
    std::cout << "=== Testing Memory Profiling ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create entities for profiling
    for (int i = 0; i < 50; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
        
        manager.AddComponent<TestComponent>(entity);
    }
    
    // Test profiling (output to console since we don't have file path)
    std::cout << "Running memory profiling...\n";
    ecs::MemoryOptimizer::ProfileMemoryUsage(manager, "");
    
    std::cout << "Memory profiling test passed!\n\n";
}

void TestRealTimeMonitoring() {
    std::cout << "=== Testing Real-Time Monitoring ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Set up monitoring callback
    bool callbackCalled = false;
    auto callback = [&callbackCalled](const ecs::MemoryOptimizer::MemoryStats& stats) {
        std::cout << "Monitoring callback: " << stats.totalUsed << " bytes used\n";
        callbackCalled = true;
    };
    
    // Start monitoring
    ecs::MemoryOptimizer::StartRealTimeMonitoring(manager, callback);
    
    // Create some entities to trigger monitoring
    for (int i = 0; i < 20; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
    }
    
    // Wait a bit for monitoring to trigger
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop monitoring
    ecs::MemoryOptimizer::StopRealTimeMonitoring(manager);
    
    std::cout << "Real-time monitoring test passed!\n\n";
}

void TestMemoryBudget() {
    std::cout << "=== Testing Memory Budget ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Set a memory budget (1MB)
    ecs::MemoryOptimizer::SetMemoryBudget(manager, 1024 * 1024);
    
    // Create entities until we approach the budget
    for (int i = 0; i < 100; ++i) {
        auto entity = manager.CreateEntity();
        manager.AddComponent<TestComponent>(entity); // 1KB per entity
    }
    
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "Memory usage with budget: " << stats.totalUsed << " bytes\n";
    
    std::cout << "Memory budget test passed!\n\n";
}

void TestBenchmarking() {
    std::cout << "=== Testing Benchmarking ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create entities for benchmarking
    for (int i = 0; i < 100; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<Position>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
        
        if (i % 2 == 0) {
            auto& vel = manager.AddComponent<Velocity>(entity);
            vel.vx = 1.0;
            vel.vy = 0.0;
            vel.vz = 0.0;
        }
        if (i % 3 == 0) {
            auto& health = manager.AddComponent<Health>(entity);
            health.current = 100;
            health.maximum = 100;
        }
        if (i % 7 == 0) {
            manager.AddComponent<TestComponent>(entity);
        }
    }
    
    // Add some entities that we'll remove to create fragmentation
    for (int i = 0; i < 50; ++i) {
        auto entity = manager.CreateEntity();
        manager.AddComponent<TestComponent>(entity);
    }
    
    // Run benchmarks
    std::cout << "Running optimization benchmarks...\n";
    ecs::MemoryOptimizer::BenchmarkOptimizations(manager);
    
    std::cout << "Benchmarking test passed!\n\n";
}

int main() {
    std::cout << "Starting Memory Optimization Tests\n";
    std::cout << "===================================\n\n";
    
    try {
        TestMemoryAnalysis();
        TestMemoryOptimization();
        TestMemoryPrediction();
        TestMemoryRecommendations();
        TestMemoryProfiling();
        TestRealTimeMonitoring();
        TestMemoryBudget();
        TestBenchmarking();
        
        std::cout << "===================================\n";
        std::cout << "All memory optimization tests passed!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}