#include "Simulation.h"
#include <iostream>

int main() {
    std::cout << "Creating Simulation..." << std::endl;
    Simulation sim;
    std::cout << "Simulation created successfully!" << std::endl;

    std::cout << "Calling Simulation::Init()..." << std::endl;
    sim.Init();
    std::cout << "Simulation::Init() completed successfully!" << std::endl;

    return 0;
}