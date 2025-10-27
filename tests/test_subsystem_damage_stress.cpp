#include "../engine/ShieldSystem.h"
#include "../engine/EnergyManagementSystem.h"
#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"
#include "../engine/FeedbackEvent.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

int main() {
    EntityManager entityManager;
    ShieldSystem shieldSystem;
    EnergyManagementSystem energySystem;

    FeedbackEventManager::Get().Clear();

    const int shipCount = 32;
    std::vector<Entity> ships;
    ships.reserve(shipCount);

    for (int i = 0; i < shipCount; ++i) {
        Entity entity = entityManager.CreateEntity();
        auto& position = entityManager.EmplaceComponent<Position>(entity);
        position.x = i * 25.0;
        position.y = (i % 4) * 10.0;
        position.z = 0.0;
        ships.push_back(entity);

        double capacity = 260.0 + (i % 5) * 25.0;
        double rechargeRate = 18.0 + (i % 7) * 1.5;
        shieldSystem.InitializeShield(entity, capacity, rechargeRate, 1.5, 0.78, "shield_array_light");
        energySystem.Initialize(entity, 70.0, 20.0, 18.0, 18.0);
    }

    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> damageDist(15.0, 60.0);
    std::uniform_real_distribution<double> repairVariance(0.0, 1.0);

    const double dt = 0.25;
    const int iterations = 480;
    double accumulatedHullOverflow = 0.0;

    for (int step = 0; step < iterations; ++step) {
        for (std::size_t idx = 0; idx < ships.size(); ++idx) {
            Entity entity = ships[idx];
            if (((step + static_cast<int>(idx)) % 2) == 0) {
                double overflow = shieldSystem.ApplyDamage(entity, damageDist(rng), &entityManager);
                accumulatedHullOverflow += overflow;
                if (overflow > 0.0) {
                    energySystem.DivertPower(entity, PowerPriority::Shields, overflow * 0.1);
                }
            }

            energySystem.Update(entity, static_cast<float>(dt));
        }

        shieldSystem.Update(entityManager, dt);

        for (Entity entity : ships) {
            const ShieldState* state = shieldSystem.GetShieldState(entity);
            if (!state) {
                continue;
            }
            if (state->currentCapacityMJ < state->maxCapacityMJ * 0.45) {
                if (energySystem.HasPower(entity, PowerPriority::Shields)) {
                    double repairBoost = state->maxCapacityMJ * (0.06 + 0.04 * repairVariance(rng));
                    shieldSystem.Recharge(entity, repairBoost * dt);
                }
            }
        }
    }

    double averageShieldPct = 0.0;
    double lowestShieldPct = 1.0;
    for (Entity entity : ships) {
        const ShieldState* state = shieldSystem.GetShieldState(entity);
        if (!state) {
            std::cerr << "Missing shield state for entity " << entity << std::endl;
            return 1;
        }
        if (state->currentCapacityMJ < -1e-6) {
            std::cerr << "Shield capacity dipped below zero for entity " << entity << std::endl;
            return 2;
        }
        if (state->currentCapacityMJ > state->maxCapacityMJ + 1e-3) {
            std::cerr << "Shield capacity exceeded maximum for entity " << entity << std::endl;
            return 3;
        }
        double pct = shieldSystem.GetShieldPercentage(entity);
        averageShieldPct += pct;
        lowestShieldPct = std::min(lowestShieldPct, pct);
    }

    if (!ships.empty()) {
        averageShieldPct /= static_cast<double>(ships.size());
    }

    std::cout << "Subsystem damage/repair stress test" << std::endl;
    std::cout << "  Hull overflow accumulated: " << accumulatedHullOverflow << " MJ" << std::endl;
    std::cout << "  Average shield: " << (averageShieldPct * 100.0) << "%" << std::endl;
    std::cout << "  Minimum shield: " << (lowestShieldPct * 100.0) << "%" << std::endl;

    if (lowestShieldPct <= 0.01 || averageShieldPct <= 0.02) {
        std::cerr << "Repair loop failed to sustain shield levels" << std::endl;
        return 4;
    }

    return 0;
}
