#include <iostream>
#include <vector>
#include <memory>
#include "engine/Simulation.cpp"
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/Components.h"

int main() {
    std::cout << "Testing BuildEnvironmentFromBounds function..." << std::endl;

    // Create default movement bounds
    MovementBounds bounds;
    bounds.minX = -10.0;
    bounds.maxX = 10.0;
    bounds.minY = -10.0;
    bounds.maxY = 10.0;
    bounds.minZ = -5.0;
    bounds.maxZ = 5.0;
    bounds.clampX = true;
    bounds.clampY = true;
    bounds.clampZ = true;

    try {
        auto definitions = BuildEnvironmentFromBounds(bounds);
        std::cout << "BuildEnvironmentFromBounds succeeded, created " << definitions.size() << " definitions" << std::endl;

        for (size_t i = 0; i < definitions.size(); ++i) {
            const auto& def = definitions[i];
            std::cout << "Definition " << i << ": center=(" << def.centerX << "," << def.centerY << "," << def.centerZ
                      << ") size=(" << def.sizeX << "," << def.sizeY << "," << def.sizeZ << ")" << std::endl;
        }

        std::cout << "BuildEnvironmentFromBounds test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in BuildEnvironmentFromBounds: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}