#include "../engine/Simulation.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
constexpr double kDt = 1.0 / 60.0;

void AdvanceSimulation(Simulation& sim, int steps) {
    for (int i = 0; i < steps; ++i) {
        sim.Update(kDt);
    }
}
} // namespace

int main() {
    Simulation legacy;
    Simulation modern;
    modern.SetUseSchedulerV2(true);

    legacy.Init();
    modern.Init();

    legacy.SetPlayerInput(false, false, false, false, false, true, 0.0);
    modern.SetPlayerInput(false, false, false, false, false, true, 0.0);

    AdvanceSimulation(legacy, 120);
    AdvanceSimulation(modern, 120);

    double legacyX = legacy.GetPlayerX();
    double modernX = modern.GetPlayerX();
    double legacyY = legacy.GetPlayerY();
    double modernY = modern.GetPlayerY();

    std::cout << "Legacy X: " << legacyX << " Modern X: " << modernX << std::endl;
    std::cout << "Legacy Y: " << legacyY << " Modern Y: " << modernY << std::endl;

    assert(std::fabs(legacyX - modernX) < 1e-4);
    assert(std::fabs(legacyY - modernY) < 1e-4);

    std::cout << "Scheduler parity regression passed" << std::endl;
    return 0;
}

