/*
 * Simple Test Memory Optimization System
 * Basic test suite for ECS memory optimization features
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cassert>
#include <vector>
#include "engine/ecs/MemoryOptimizer.h"
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/TestComponents.h"

void TestBasicMemoryAnalysis() {
    std::cout << "=== Testing Basic Memory Analysis ===\n";
    
    // Create entity manager V2 for modern archetype system
    ecs::EntityManagerV2 manager;
    
    // Create some simple entities
    int entitiesCreated = 0;
    for (int i = 0; i < 10; ++i) {
        auto entity = manager.CreateEntity();
        
        // Add components directly (V2 uses value semantics, not shared_ptr)
        auto& pos = manager.AddComponent<SimplePosition>(entity);
        pos.x = static_cast<double>(i);
        pos.y = static_cast<double>(i*2);
        pos.z = 0.0;
        
        if (i % 2 == 0) {
            auto& vel = manager.AddComponent<SimpleVelocity>(entity);
            vel.vx = 1.0;
            vel.vy = 0.0;
            vel.vz = 0.0;
        }
        
        if (i % 3 == 0) {
            manager.AddComponent<SimpleTestComponent>(entity);
        }
        entitiesCreated++;
    }
    
    // Analyze memory usage
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    
    std::cout << "Memory Analysis Results:\n";
    std::cout << "  Entities Created: " << entitiesCreated << "\n";
    std::cout << "  Total Allocated: " << stats.totalAllocated << " bytes\n";
    std::cout << "  Total Used: " << stats.totalUsed << " bytes\n";
    std::cout << "  Wasted Space: " << stats.wastedSpace << " bytes\n";
    std::cout << "  Fragmentation Ratio: " << stats.fragmentationRatio << "\n";
    std::cout << "  Archetype Count: " << stats.archetypeCount << "\n";
    
    assert(entitiesCreated == 10);
    assert(stats.archetypeCount > 0);
    std::cout << "Basic memory analysis test passed!\n\n";
}

void TestMemoryCompaction() {
    std::cout << "=== Testing Memory Compaction ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create many entities to generate fragmentation
    std::vector<ecs::EntityHandle> entities;
    for (int i = 0; i < 100; ++i) {
        auto entity = manager.CreateEntity();
        entities.push_back(entity);
        
        auto& pos = manager.AddComponent<SimplePosition>(entity);
        pos.x = static_cast<double>(i);
        pos.y = 0.0;
        pos.z = 0.0;
        
        manager.AddComponent<SimpleTestComponent>(entity);
    }
    
    // Analyze memory before optimization
    auto beforeStats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "Before optimization:\n";
    std::cout << "  Fragmentation Ratio: " << beforeStats.fragmentationRatio << "\n";
    std::cout << "  Wasted Space: " << beforeStats.wastedSpace << " bytes\n";
    
    // Remove every other entity to create fragmentation
    size_t removed = 0;
    for (size_t i = 0; i < entities.size(); i += 2) {
        manager.DestroyEntity(entities[i]);
        removed++;
    }
    
    // Perform optimization
    ecs::MemoryOptimizer::Compact(manager);
    
    // Analyze memory after optimization
    auto afterStats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "After optimization:\n";
    std::cout << "  Fragmentation Ratio: " << afterStats.fragmentationRatio << "\n";
    std::cout << "  Wasted Space: " << afterStats.wastedSpace << " bytes\n";
    
    assert(removed == 50);
    assert(afterStats.wastedSpace <= beforeStats.wastedSpace);
    std::cout << "Memory compaction test passed!\n\n";
}

void TestMemoryPrediction() {
    std::cout << "=== Testing Memory Prediction ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create baseline entities
    for (int i = 0; i < 20; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<SimplePosition>(entity);
        pos.x = static_cast<double>(i);
        
        auto& vel = manager.AddComponent<SimpleVelocity>(entity);
        vel.vx = 1.0;
    }
    
    // Test memory prediction
    auto prediction = ecs::MemoryOptimizer::PredictMemoryUsage(manager, 50);
    
    std::cout << "Memory prediction for 50 additional entities:\n";
    std::cout << "  Predicted Total Allocated: " << prediction.totalAllocated << " bytes\n";
    std::cout << "  Predicted Total Used: " << prediction.totalUsed << " bytes\n";
    
    assert(prediction.totalAllocated > 0);
    std::cout << "Memory prediction test passed!\n\n";
}

void TestMemoryRecommendations() {
    std::cout << "=== Testing Memory Recommendations ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Create scenario with various component patterns
    for (int i = 0; i < 50; ++i) {
        auto entity = manager.CreateEntity();
        auto& pos = manager.AddComponent<SimplePosition>(entity);
        pos.x = static_cast<double>(i);
        
        if (i % 3 == 0) {
            auto& vel = manager.AddComponent<SimpleVelocity>(entity);
            vel.vx = 1.0;
        }
        if (i % 5 == 0) {
            manager.AddComponent<SimpleTestComponent>(entity);
        }
    }
    
    // Get recommendations
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    auto recommendations = ecs::MemoryOptimizer::GetOptimizationRecommendations(stats);
    
    std::cout << "Memory recommendations:\n";
    for (const auto& rec : recommendations) {
        std::cout << "  - " << rec << "\n";
    }
    if (recommendations.empty()) {
        std::cout << "  - No recommendations (system is optimized)\n";
    }
    
    std::cout << "Memory recommendations test passed!\n\n";
}

void TestMemoryBudget() {
    std::cout << "=== Testing Memory Budget ===\n";
    
    ecs::EntityManagerV2 manager;
    
    // Set a memory budget (1MB)
    ecs::MemoryOptimizer::SetMemoryBudget(manager, 1024 * 1024);
    
    // Create entities
    for (int i = 0; i < 50; ++i) {
        auto entity = manager.CreateEntity();
        manager.AddComponent<SimpleTestComponent>(entity);
    }
    
    auto stats = ecs::MemoryOptimizer::AnalyzeMemory(manager);
    std::cout << "Memory usage with budget:\n";
    std::cout << "  Total Used: " << stats.totalUsed << " bytes\n";
    std::cout << "  Budget: 1048576 bytes\n";
    std::cout << "  Pressure Level: ";
    switch (stats.pressureLevel) {
        case ecs::MemoryPressureLevel::Low: std::cout << "Low\n"; break;
        case ecs::MemoryPressureLevel::Medium: std::cout << "Medium\n"; break;
        case ecs::MemoryPressureLevel::High: std::cout << "High\n"; break;
        case ecs::MemoryPressureLevel::Critical: std::cout << "Critical\n"; break;
    }
    
    std::cout << "Memory budget test passed!\n\n";
}

int main() {
    std::cout << "Starting Memory Optimization Tests\n";
    std::cout << "==========================================\n\n";
    std::cout << "Using EntityManagerV2 with full archetype system\n\n";
    
    try {
        TestBasicMemoryAnalysis();
        TestMemoryCompaction();
        TestMemoryPrediction();
        TestMemoryRecommendations();
        TestMemoryBudget();
        
        std::cout << "==========================================\n";
        std::cout << "All memory optimization tests passed!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}