#include "../src/Simulation.h"
#include <iostream>
#include <cmath>

int main() {
    Simulation sim;
    sim.Init();

    // Test 1: with right input for 1 second, expect x ~ 2.0 (0.5 * a * t^2, a=4)
    sim.SetPlayerInput(false, true);
    const double dt = 1.0 / 60.0;
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double x = sim.GetPlayerX();
    std::cout << "Test1 player x=" << x << std::endl;
    if (std::abs(x - 2.0) > 0.05) {
        std::cerr << "Test1 FAILED: expected ~2.0" << std::endl;
        return 1;
    }

    // Test 2: hold right for long time, position should clamp at <= 5.0
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    x = sim.GetPlayerX();
    std::cout << "Test2 player x=" << x << std::endl;
    if (x > 5.0 + 1e-6) {
        std::cerr << "Test2 FAILED: expected <=5.0" << std::endl;
        return 2;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
