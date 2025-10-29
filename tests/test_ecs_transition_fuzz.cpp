#include "engine/ecs/EntityManagerV2.h"
#include "engine/ecs/Components.h"

#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using ecs::EntityHandle;
using ecs::EntityManagerV2;

namespace {

bool ValidateIntegrity(const EntityManagerV2& manager) {
    const auto& archetypes = manager.GetArchetypeManager().GetAllArchetypes();
    size_t total = 0;
    for (const auto& archetype : archetypes) {
        if (!archetype->ValidateIntegrity()) {
            return false;
        }
        total += archetype->GetEntityCount();
    }

    return total == manager.GetEntityCount();
}

} // namespace

int main(int argc, char** argv) {
    uint32_t seed = 0;
    if (argc > 1) {
        seed = static_cast<uint32_t>(std::stoul(argv[1]));
    } else {
        seed = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    std::mt19937 rng(seed);
    std::cout << "ECS transition fuzz test seed: " << seed << std::endl;

    EntityManagerV2 manager;

    constexpr size_t kEntityCount = 100000;
    std::vector<EntityHandle> entities;
    entities.reserve(kEntityCount);
    std::vector<bool> alive(kEntityCount, false);
    std::vector<bool> hasPosition(kEntityCount, false);
    std::vector<bool> hasVelocity(kEntityCount, false);

    for (size_t i = 0; i < kEntityCount; ++i) {
        EntityHandle handle = manager.CreateEntity();
        entities.push_back(handle);
        alive[i] = true;
    }

    std::uniform_int_distribution<size_t> entityDist(0, kEntityCount - 1);
    std::uniform_int_distribution<int> actionDist(0, 5);
    std::uniform_int_distribution<int> componentDist(0, 1);

    constexpr size_t kIterations = 250000;

    for (size_t iteration = 0; iteration < kIterations; ++iteration) {
        size_t index = entityDist(rng);
        EntityHandle handle = entities[index];

        int action = actionDist(rng);
        int componentChoice = componentDist(rng);

        if (action == 0 || action == 1) {
            // Add or remove Position
            if (componentChoice == 0) {
                if (!hasPosition[index] && alive[index]) {
                    auto& pos = manager.AddComponent<Position>(handle);
                    pos.x = static_cast<double>(iteration % 1024);
                    pos.y = pos.x * 0.5;
                    pos.z = pos.x * 0.25;
                    hasPosition[index] = true;
                } else if (hasPosition[index]) {
                    manager.RemoveComponent<Position>(handle);
                    hasPosition[index] = false;
                }
            } else {
                if (!hasVelocity[index] && alive[index]) {
                    auto& vel = manager.AddComponent<Velocity>(handle);
                    vel.vx = 1.0;
                    vel.vy = 0.5;
                    vel.vz = 0.25;
                    hasVelocity[index] = true;
                } else if (hasVelocity[index]) {
                    manager.RemoveComponent<Velocity>(handle);
                    hasVelocity[index] = false;
                }
            }
        } else if (action == 2) {
            // Destroy and recreate entity
            if (alive[index]) {
                manager.DestroyEntity(handle);
                alive[index] = false;
                hasPosition[index] = false;
                hasVelocity[index] = false;
            } else {
                entities[index] = manager.CreateEntity();
                alive[index] = true;
            }
        } else {
            // Exercise deferred command buffer while iterating
            manager.ForEach<Position, Velocity>([&](EntityHandle iterHandle, Position& pos, Velocity& vel) {
                pos.x += vel.vx * 0.016;
                pos.y += vel.vy * 0.016;
                pos.z += vel.vz * 0.016;

                if (rng() % 13 == 0) {
                    manager.RemoveComponent<Velocity>(iterHandle);
                    size_t idx = iterHandle.Index();
                    if (idx < hasVelocity.size()) {
                        hasVelocity[idx] = false;
                    }
                }
                if (rng() % 17 == 0) {
                    manager.AddComponent<Position>(iterHandle);
                    size_t idx = iterHandle.Index();
                    if (idx < hasPosition.size()) {
                        hasPosition[idx] = true;
                    }
                }
            });
            manager.FlushDeferred();
        }

        if ((iteration % 1024) == 0) {
            manager.FlushDeferred();
            if (!ValidateIntegrity(manager)) {
                std::cerr << "Integrity failure after iteration " << iteration << std::endl;
                return 1;
            }
        }
    }

    manager.FlushDeferred();

    if (!ValidateIntegrity(manager)) {
        std::cerr << "Integrity failure at end of fuzz test" << std::endl;
        return 1;
    }

    std::cout << "Fuzz test completed successfully" << std::endl;
    return 0;
}

