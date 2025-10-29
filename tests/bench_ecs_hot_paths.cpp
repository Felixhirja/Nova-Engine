#include "engine/ecs/EntityManagerV2.h"
#include "engine/ecs/Components.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

struct BenchResult {
    size_t entityCount;
    double addPositionMs;
    double removePositionMs;
    double destroyMs;
};

BenchResult RunBenchmark(size_t entityCount) {
    ecs::EntityManagerV2 manager;
    std::vector<ecs::EntityHandle> entities;
    entities.reserve(entityCount);

    for (size_t i = 0; i < entityCount; ++i) {
        entities.push_back(manager.CreateEntity());
    }

    auto startAdd = Clock::now();
    for (auto handle : entities) {
        auto& pos = manager.AddComponent<Position>(handle);
        pos.x = pos.y = pos.z = static_cast<double>(handle.Index());
    }
    manager.FlushDeferred();
    auto endAdd = Clock::now();

    auto startRemove = Clock::now();
    for (auto handle : entities) {
        manager.RemoveComponent<Position>(handle);
    }
    manager.FlushDeferred();
    auto endRemove = Clock::now();

    auto startDestroy = Clock::now();
    for (auto handle : entities) {
        manager.DestroyEntity(handle);
    }
    manager.FlushDeferred();
    auto endDestroy = Clock::now();

    BenchResult result{};
    result.entityCount = entityCount;
    result.addPositionMs = std::chrono::duration<double, std::milli>(endAdd - startAdd).count();
    result.removePositionMs = std::chrono::duration<double, std::milli>(endRemove - startRemove).count();
    result.destroyMs = std::chrono::duration<double, std::milli>(endDestroy - startDestroy).count();
    return result;
}

int main() {
    std::vector<size_t> archetypeSizes = {64, 256, 1024, 4096, 16384};

    std::cout << "Benchmarking ECS hot paths" << std::endl;
    std::cout << std::setw(10) << "Entities"
              << std::setw(15) << "Add Position"
              << std::setw(18) << "Remove Position"
              << std::setw(15) << "Destroy" << std::endl;

    for (size_t count : archetypeSizes) {
        BenchResult result = RunBenchmark(count);
        std::cout << std::setw(10) << result.entityCount
                  << std::setw(14) << std::fixed << std::setprecision(3) << result.addPositionMs << " ms"
                  << std::setw(17) << result.removePositionMs << " ms"
                  << std::setw(14) << result.destroyMs << " ms" << std::endl;
    }

    return 0;
}

