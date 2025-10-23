#include "../src/ecs/EntityManagerV2.h"
#include "../src/ecs/SystemSchedulerV2.h"
#include "../src/ecs/Components.h"
#include <cassert>
#include <cmath>
#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ecs;

// Test system for physics simulation
class PhysicsSystemV2 : public SystemV2 {
public:
    void Update(EntityManagerV2& em, double dt) override {
        RecordUpdateStart();
        size_t count = 0;
        
        em.ForEach<Position, Velocity>([dt, &count](EntityHandle entity, Position& pos, Velocity& vel) {
            pos.x += vel.vx * dt;
            pos.y += vel.vy * dt;
            pos.z += vel.vz * dt;
            count++;
        });
        
        RecordUpdateEnd(count);
    }
    
    std::vector<ComponentDependency> GetDependencies() const override {
        return {
            ComponentDependency::Write<Position>(),
            ComponentDependency::Read<Velocity>()
        };
    }
    
    UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Simulation; }

    const char* GetName() const override {
        return "PhysicsSystemV2";
    }
};

// Test system for acceleration
class AccelerationSystemV2 : public SystemV2 {
public:
    void Update(EntityManagerV2& em, double dt) override {
        RecordUpdateStart();
        size_t count = 0;
        
        em.ForEach<Velocity, Acceleration>([dt, &count](EntityHandle entity, Velocity& vel, Acceleration& acc) {
            vel.vx += acc.ax * dt;
            vel.vy += acc.ay * dt;
            vel.vz += acc.az * dt;
            count++;
        });
        
        RecordUpdateEnd(count);
    }
    
    std::vector<ComponentDependency> GetDependencies() const override {
        return {
            ComponentDependency::Write<Velocity>(),
            ComponentDependency::Read<Acceleration>()
        };
    }
    
    UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Input; }
    
    const char* GetName() const override {
        return "AccelerationSystemV2";
    }
};

struct ExecutionRecorder {
    std::vector<std::string> events;
    std::mutex mutex;

    void Record(const std::string& event) {
        std::lock_guard<std::mutex> lock(mutex);
        events.push_back(event);
    }
};

class TrackingSystemA : public SystemV2 {
public:
    explicit TrackingSystemA(ExecutionRecorder& recorder) : recorder_(recorder) {}

    bool SupportsStage(UpdateStage) const override { return true; }

    void PreUpdate(EntityManagerV2&, double) override { recorder_.Record("A_Pre"); }

    void Update(EntityManagerV2&, double) override {
        recorder_.Record("A_Update");
    }

    void PostUpdate(EntityManagerV2&, double) override { recorder_.Record("A_Post"); }

    const char* GetName() const override { return "TrackingSystemA"; }

private:
    ExecutionRecorder& recorder_;
};

class TrackingSystemB : public SystemV2 {
public:
    explicit TrackingSystemB(ExecutionRecorder& recorder) : recorder_(recorder) {}

    bool SupportsStage(UpdateStage) const override { return true; }

    void PreUpdate(EntityManagerV2&, double) override { recorder_.Record("B_Pre"); }

    void Update(EntityManagerV2&, double) override {
        recorder_.Record("B_Update");
    }

    void PostUpdate(EntityManagerV2&, double) override { recorder_.Record("B_Post"); }

    const char* GetName() const override { return "TrackingSystemB"; }

    std::vector<SystemDependency> GetSystemDependencies() const override {
        return {SystemDependency::Requires<TrackingSystemA>()};
    }

private:
    ExecutionRecorder& recorder_;
};

struct DamageEvent {
    EntityHandle entity;
    int amount;
};

class DamageEmitterSystem : public SystemV2 {
public:
    explicit DamageEmitterSystem(const std::vector<EntityHandle>& targets)
        : targets_(targets) {}

    UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Input; }

    void Update(EntityManagerV2&, double) override {
        RecordUpdateStart();
        for (const auto& entity : targets_) {
            PublishEvent(DamageEvent{entity, 10});
        }
        RecordUpdateEnd(targets_.size());
    }

    const char* GetName() const override { return "DamageEmitterSystem"; }

private:
    const std::vector<EntityHandle>& targets_;
};

class DamageReceiverSystem : public SystemV2 {
public:
    explicit DamageReceiverSystem(std::vector<DamageEvent>& outEvents)
        : events_(outEvents) {}

    UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Simulation; }

    void Update(EntityManagerV2&, double) override {
        RecordUpdateStart();
        size_t processed = eventsProcessed_.exchange(0);
        RecordUpdateEnd(processed);
    }

    const char* GetName() const override { return "DamageReceiverSystem"; }

    void OnEventBusConfigured(SystemEventBus&) override {
        SubscribeEvent<DamageEvent>([this](const DamageEvent& event) {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.push_back(event);
            eventsProcessed_++;
        });
    }

private:
    std::vector<DamageEvent>& events_;
    std::mutex mutex_;
    std::atomic<size_t> eventsProcessed_{0};
};

class CycleSystemB;

class CycleSystemA : public SystemV2 {
public:
    void Update(EntityManagerV2&, double) override {}

    const char* GetName() const override { return "CycleSystemA"; }

    std::vector<SystemDependency> GetSystemDependencies() const override {
        return {SystemDependency::Requires<CycleSystemB>()};
    }
};

class CycleSystemB : public SystemV2 {
public:
    void Update(EntityManagerV2&, double) override {}

    const char* GetName() const override { return "CycleSystemB"; }

    std::vector<SystemDependency> GetSystemDependencies() const override {
        return {SystemDependency::Requires<CycleSystemA>()};
    }
};

void TestEntityVersioning() {
    std::cout << "Testing Entity Versioning..." << std::endl;
    
    EntityManagerV2 em;
    
    // Create entity
    EntityHandle entity1 = em.CreateEntity();
    assert(entity1.IsValid());
    assert(entity1.Generation() == 0);
    assert(em.IsAlive(entity1));
    
    // Destroy entity
    em.DestroyEntity(entity1);
    assert(!em.IsAlive(entity1));
    
    // Create new entity (should reuse index with new generation)
    EntityHandle entity2 = em.CreateEntity();
    assert(entity2.IsValid());
    assert(entity2.Index() == entity1.Index());  // Same index
    assert(entity2.Generation() > entity1.Generation());  // Different generation
    assert(em.IsAlive(entity2));
    assert(!em.IsAlive(entity1));  // Old handle still invalid
    
    std::cout << "  ✅ Entity versioning works correctly" << std::endl;
    std::cout << "  Entity1: Index=" << entity1.Index() << " Gen=" << (int)entity1.Generation() << std::endl;
    std::cout << "  Entity2: Index=" << entity2.Index() << " Gen=" << (int)entity2.Generation() << std::endl;
}

void TestArchetypeTransitions() {
    std::cout << "\nTesting Archetype Transitions..." << std::endl;
    
    EntityManagerV2 em;
    EntityHandle entity = em.CreateEntity();
    
    // Start with no components (archetype 0)
    assert(em.GetArchetypeCount() == 1);
    
    // Add Position component
    Position& pos = em.AddComponent<Position>(entity);
    pos.x = 1.0; pos.y = 2.0; pos.z = 3.0;
    
    assert(em.HasComponent<Position>(entity));
    assert(em.GetComponent<Position>(entity)->x == 1.0);
    std::cout << "  ✅ Added Position component" << std::endl;
    
    // Add Velocity component
    Velocity& vel = em.AddComponent<Velocity>(entity);
    vel.vx = 0.5; vel.vy = 0.25; vel.vz = 0.1;
    
    assert(em.HasComponent<Position>(entity));
    assert(em.HasComponent<Velocity>(entity));
    assert(em.GetComponent<Velocity>(entity)->vx == 0.5);
    std::cout << "  ✅ Added Velocity component" << std::endl;
    
    // Remove Position component
    em.RemoveComponent<Position>(entity);
    assert(!em.HasComponent<Position>(entity));
    assert(em.HasComponent<Velocity>(entity));
    std::cout << "  ✅ Removed Position component" << std::endl;
    
    std::cout << "  Total archetypes created: " << em.GetArchetypeCount() << std::endl;
}

void TestCacheFriendlyIteration() {
    std::cout << "\nTesting Cache-Friendly Iteration..." << std::endl;
    
    EntityManagerV2 em;
    const size_t entityCount = 10000;
    
    // Create many entities with Position and Velocity
    auto startCreate = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < entityCount; ++i) {
        EntityHandle entity = em.CreateEntity();
        auto& pos = em.AddComponent<Position>(entity);
        auto& vel = em.AddComponent<Velocity>(entity);
        
        pos.x = (double)i;
        pos.y = (double)i * 2.0;
        pos.z = (double)i * 3.0;
        
        vel.vx = 1.0;
        vel.vy = 2.0;
        vel.vz = 3.0;
    }
    
    auto endCreate = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> createTime = endCreate - startCreate;
    
    std::cout << "  Created " << entityCount << " entities in " 
              << createTime.count() << "ms" << std::endl;
    
    // Iterate and update positions
    auto startIterate = std::chrono::high_resolution_clock::now();
    
    size_t count = 0;
    em.ForEach<Position, Velocity>([&count](EntityHandle entity, Position& pos, Velocity& vel) {
        pos.x += vel.vx * 0.016;
        pos.y += vel.vy * 0.016;
        pos.z += vel.vz * 0.016;
        count++;
    });
    
    auto endIterate = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> iterateTime = endIterate - startIterate;
    
    assert(count == entityCount);
    std::cout << "  ✅ Iterated " << count << " entities in " 
              << iterateTime.count() << "ms" << std::endl;
    std::cout << "  Performance: " << (entityCount / (iterateTime.count() / 1000.0)) 
              << " entities/sec" << std::endl;
}

void TestParallelSystemExecution() {
    std::cout << "\nTesting Parallel System Execution..." << std::endl;

    EntityManagerV2 em;
    SystemSchedulerV2 scheduler;
    
    // Register systems
    scheduler.RegisterSystem<AccelerationSystemV2>();
    scheduler.RegisterSystem<PhysicsSystemV2>();
    
    std::cout << "  Using " << scheduler.GetThreadCount() << " threads" << std::endl;
    
    // Create test entities
    const size_t entityCount = 5000;
    for (size_t i = 0; i < entityCount; ++i) {
        EntityHandle entity = em.CreateEntity();
        auto& pos = em.AddComponent<Position>(entity);
        auto& vel = em.AddComponent<Velocity>(entity);
        auto& acc = em.AddComponent<Acceleration>(entity);
        
        pos.x = pos.y = pos.z = 0.0;
        vel.vx = vel.vy = vel.vz = 0.0;
        acc.ax = 1.0; acc.ay = 2.0; acc.az = 3.0;
    }
    
    // Run systems for several frames
    double dt = 0.016;  // 60 FPS
    for (int frame = 0; frame < 60; ++frame) {
        scheduler.UpdateAll(em, dt);
    }
    
    // Check results
    bool correct = true;
    em.ForEach<Position>([&correct, dt](EntityHandle entity, const Position& pos) {
        // After 60 frames with constant acceleration:
        // v = a * t, x = 0.5 * a * t^2
        double t = 60.0 * dt;
        double expectedVel = 1.0 * t;  // vx = ax * t
        double expectedPos = 0.5 * 1.0 * t * t;  // x = 0.5 * ax * t^2
        
        // Allow for floating point error
        if (std::abs(pos.x - expectedPos) > 0.1) {
            correct = false;
        }
    });
    
    assert(correct);
    std::cout << "  ✅ Systems executed correctly" << std::endl;
    
    // Print profiling data
    auto profiles = scheduler.GetSystemProfiles();
    std::cout << "\n  System Profiling:" << std::endl;
    for (const auto& profile : profiles) {
        std::cout << "    " << profile.name << ": "
                  << profile.lastUpdateTime << "ms ("
                  << profile.lastEntitiesProcessed << " entities, total "
                  << profile.totalEntitiesProcessed << ", cache misses "
                  << profile.lastCacheMisses << ")" << std::endl;
    }
    std::cout << "  Total update time (last frame): " << scheduler.GetTotalUpdateTime() << "ms" << std::endl;
}

void TestSystemMessaging() {
    std::cout << "\nTesting System Messaging..." << std::endl;

    EntityManagerV2 em;
    SystemSchedulerV2 scheduler;

    std::vector<EntityHandle> targets;
    for (int i = 0; i < 5; ++i) {
        targets.push_back(em.CreateEntity());
    }

    std::vector<DamageEvent> received;
    scheduler.RegisterSystem<DamageEmitterSystem>(targets);
    scheduler.RegisterSystem<DamageReceiverSystem>(received);

    scheduler.UpdateAll(em, 0.016);

    assert(received.size() == targets.size());
    int totalDamage = 0;
    for (const auto& evt : received) {
        totalDamage += evt.amount;
    }
    assert(totalDamage == static_cast<int>(targets.size() * 10));

    auto profiles = scheduler.GetSystemProfiles();
    bool receiverProfileFound = false;
    for (const auto& profile : profiles) {
        if (std::string(profile.name) == "DamageReceiverSystem") {
            receiverProfileFound = true;
            assert(profile.totalEntitiesProcessed >= received.size());
        }
    }
    assert(receiverProfileFound);

    std::cout << "  ✅ Events delivered between systems (" << received.size() << " events)" << std::endl;
}

void TestMultiPhaseOrdering() {
    std::cout << "\nTesting Multi-Stage Ordering..." << std::endl;

    EntityManagerV2 em;
    SystemSchedulerV2 scheduler;
    ExecutionRecorder recorder;

    scheduler.RegisterSystem<TrackingSystemA>(recorder);
    scheduler.RegisterSystem<TrackingSystemB>(recorder);

    scheduler.UpdateAll(em, 0.016);

    std::vector<std::string> expected = {
        "A_Pre", "B_Pre", "A_Update", "B_Update", "A_Post", "B_Post"
    };

    assert(recorder.events == expected);
    std::cout << "  ✅ Multi-stage ordering respected" << std::endl;
}

void TestDependencyCycleDetection() {
    std::cout << "\nTesting Dependency Cycle Detection..." << std::endl;

    EntityManagerV2 em;
    SystemSchedulerV2 scheduler;

    scheduler.RegisterSystem<CycleSystemA>();
    scheduler.RegisterSystem<CycleSystemB>();

    bool caught = false;
    try {
        scheduler.UpdateAll(em, 0.016);
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✅ Caught cycle: " << e.what() << std::endl;
    }

    assert(caught);
}

void TestStressTest() {
    std::cout << "\nStress Test: 50,000 Entities..." << std::endl;
    
    EntityManagerV2 em;
    const size_t entityCount = 50000;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create entities
    for (size_t i = 0; i < entityCount; ++i) {
        EntityHandle entity = em.CreateEntity();
        em.AddComponent<Position>(entity);
        em.AddComponent<Velocity>(entity);
        
        if (i % 2 == 0) {
            em.AddComponent<Acceleration>(entity);
        }
        if (i % 3 == 0) {
            em.AddComponent<Name>(entity);
        }
    }
    
    auto endCreate = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> createTime = endCreate - startTime;
    
    std::cout << "  Created " << entityCount << " entities in " 
              << createTime.count() << "ms" << std::endl;
    std::cout << "  Archetypes: " << em.GetArchetypeCount() << std::endl;
    
    // Iterate multiple times
    size_t iterations = 100;
    auto startIterate = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        em.ForEach<Position, Velocity>([](EntityHandle entity, Position& pos, Velocity& vel) {
            pos.x += vel.vx;
            pos.y += vel.vy;
            pos.z += vel.vz;
        });
    }
    
    auto endIterate = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> iterateTime = endIterate - startIterate;
    
    double avgIterationTime = iterateTime.count() / iterations;
    double fps = 1000.0 / avgIterationTime;
    
    std::cout << "  ✅ " << iterations << " iterations in " 
              << iterateTime.count() << "ms" << std::endl;
    std::cout << "  Average: " << avgIterationTime << "ms per iteration" << std::endl;
    std::cout << "  Equivalent FPS: " << fps << " (if only this system)" << std::endl;
}

int main() {
    std::cout << "=== ECS V2 Archetype System Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        TestEntityVersioning();
        TestArchetypeTransitions();
        TestCacheFriendlyIteration();
        TestParallelSystemExecution();
        TestSystemMessaging();
        TestStressTest();
        TestMultiPhaseOrdering();
        TestDependencyCycleDetection();
        
        std::cout << "\n==================================" << std::endl;
        std::cout << "✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "==================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
