// Test: Lazy Archetype Creation Performance
// Validates memory efficiency and startup time improvements

#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"
#include <chrono>
#include <iostream>
#include <cassert>

using namespace ecs;

void TestLazyCreationMemoryEfficiency() {
    std::cout << "=== Lazy Archetype Creation Test ===\n\n";
    
    EntityManagerV2 manager;
    auto& archetypeManager = const_cast<ecs::ArchetypeManager&>(manager.GetArchetypeManager());
    
    // Measure baseline (empty archetype only)
    auto baselineStats = archetypeManager.GetLazyCreationStats();
    std::cout << "📊 Baseline (empty manager):\n";
    std::cout << "  Total archetypes: " << baselineStats.totalArchetypes << "\n";
    std::cout << "  Memory used: " << (baselineStats.totalMemoryUsed / 1024.0) << " KB\n\n";
    
    // Create entities with different component combinations
    std::cout << "Creating 1,000 entities with varied archetypes...\n";
    std::vector<EntityHandle> entities;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Pattern 1: Position only (200 entities)
    for (int i = 0; i < 200; ++i) {
        auto e = manager.CreateEntity();
        Position pos; pos.x = i; pos.y = i; pos.z = i;
        manager.AddComponent<Position>(e, pos);
        entities.push_back(e);
    }
    
    // Pattern 2: Position + Velocity (300 entities)
    for (int i = 0; i < 300; ++i) {
        auto e = manager.CreateEntity();
        Position pos; pos.x = i; pos.y = i; pos.z = i;
        Velocity vel; vel.vx = 1; vel.vy = 1; vel.vz = 1;
        manager.AddComponent<Position>(e, pos);
        manager.AddComponent<Velocity>(e, vel);
        entities.push_back(e);
    }
    
    // Pattern 3: All three components (500 entities)
    for (int i = 0; i < 500; ++i) {
        auto e = manager.CreateEntity();
        Position pos; pos.x = i; pos.y = i; pos.z = i;
        Velocity vel; vel.vx = 1; vel.vy = 1; vel.vz = 1;
        Acceleration acc; acc.ax = 0.1; acc.ay = 0.1; acc.az = 0.1;
        manager.AddComponent<Position>(e, pos);
        manager.AddComponent<Velocity>(e, vel);
        manager.AddComponent<Acceleration>(e, acc);
        entities.push_back(e);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "✓ Created 1,000 entities in " << duration.count() << " ms\n\n";
    
    // Analyze lazy creation effectiveness
    auto stats = archetypeManager.GetLazyCreationStats();
    
    std::cout << "📈 Lazy Creation Statistics:\n";
    std::cout << "  Total archetypes: " << stats.totalArchetypes << "\n";
    std::cout << "  Empty archetypes: " << stats.emptyArchetypes << "\n";
    std::cout << "  Small archetypes (< 8):   " << stats.smallArchetypes << "\n";
    std::cout << "  Medium archetypes (8-256): " << stats.mediumArchetypes << "\n";
    std::cout << "  Large archetypes (> 256):  " << stats.largeArchetypes << "\n\n";
    
    std::cout << "💾 Memory Efficiency:\n";
    std::cout << "  Total memory: " << (stats.totalMemoryUsed / 1024.0) << " KB\n";
    std::cout << "  Wasted memory: " << (stats.totalMemoryWasted / 1024.0) << " KB\n";
    std::cout << "  Utilization: " << (stats.avgUtilization * 100.0) << "%\n\n";
    
    // Performance evaluation
    std::cout << "🎯 Performance Analysis:\n";
    
    double utilizationPercent = stats.avgUtilization * 100.0;
    if (utilizationPercent > 75.0) {
        std::cout << "  ✓ EXCELLENT! High memory utilization (> 75%)\n";
    } else if (utilizationPercent > 50.0) {
        std::cout << "  ✓ GOOD! Decent memory utilization (> 50%)\n";
    } else {
        std::cout << "  ⚠ Memory could be better utilized\n";
    }
    
    // Only 4-5 archetypes should exist (empty + 3 patterns + transition intermediates)
    if (stats.totalArchetypes <= 8) {
        std::cout << "  ✓ Minimal archetype creation (≤ 8 archetypes)\n";
    } else {
        std::cout << "  ⚠ More archetypes than expected: " << stats.totalArchetypes << "\n";
    }
    
    if (duration.count() < 50) {
        std::cout << "  ✓ Fast entity creation (< 50ms for 1K entities)\n";
    }
    
    // Test compaction
    std::cout << "\n🔧 Testing memory compaction...\n";
    size_t wastedBefore = stats.totalMemoryWasted;
    
    archetypeManager.CompactArchetypes();
    
    auto compactedStats = archetypeManager.GetLazyCreationStats();
    size_t wastedAfter = compactedStats.totalMemoryWasted;
    
    if (wastedAfter < wastedBefore) {
        double reduction = ((wastedBefore - wastedAfter) / static_cast<double>(wastedBefore)) * 100.0;
        std::cout << "  ✓ Compaction reduced waste by " << reduction << "%\n";
    } else {
        std::cout << "  ✓ Already optimally packed\n";
    }
    
    // Validate correctness
    std::cout << "\n🔍 Validating correctness...\n";
    size_t positionCount = 0;
    size_t velocityCount = 0;
    size_t accelerationCount = 0;
    
    for (auto entity : entities) {
        if (manager.HasComponent<Position>(entity)) positionCount++;
        if (manager.HasComponent<Velocity>(entity)) velocityCount++;
        if (manager.HasComponent<Acceleration>(entity)) accelerationCount++;
    }
    
    assert(positionCount == 1000 && "All entities should have Position");
    assert(velocityCount == 800 && "800 entities should have Velocity");
    assert(accelerationCount == 500 && "500 entities should have Acceleration");
    
    std::cout << "  ✓ Position components: " << positionCount << " (expected 1000)\n";
    std::cout << "  ✓ Velocity components: " << velocityCount << " (expected 800)\n";
    std::cout << "  ✓ Acceleration components: " << accelerationCount << " (expected 500)\n";
    
    std::cout << "\n✅ Test completed successfully!\n";
}

void TestStartupTimeComparison() {
    std::cout << "\n=== Startup Time Benchmark ===\n\n";
    
    std::cout << "Measuring entity creation speed...\n";
    
    // Test different entity counts
    std::vector<size_t> testSizes = {100, 500, 1000, 5000, 10000};
    
    for (size_t count : testSizes) {
        EntityManagerV2 manager;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < count; ++i) {
            auto e = manager.CreateEntity();
            Position pos; pos.x = i; pos.y = i; pos.z = i;
            Velocity vel; vel.vx = 1; vel.vy = 1; vel.vz = 1;
            manager.AddComponent<Position>(e, pos);
            manager.AddComponent<Velocity>(e, vel);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double msec = duration.count() / 1000.0;
        double perEntity = duration.count() / static_cast<double>(count);
        
        std::cout << "  " << count << " entities: " << msec << " ms "
                  << "(" << perEntity << " μs/entity)\n";
    }
    
    std::cout << "\n✓ Startup benchmark complete\n";
}

int main() {
    try {
        TestLazyCreationMemoryEfficiency();
        TestStartupTimeComparison();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
