#include "SpaceshipJsonLoader.h"
#include <iostream>

int main() {
    std::cout << "Testing spaceship JSON loading..." << std::endl;

    // Test JSON loading
    bool loadSuccess = SpaceshipJsonLoader::LoadSpaceshipsFromDirectory("assets/ships");
    if (!loadSuccess) {
        std::cout << "ERROR: Failed to load spaceship definitions!" << std::endl;
        return 1;
    }

    const auto& spaceshipCatalog = SpaceshipJsonLoader::GetLoadedSpaceships();
    std::cout << "Loaded " << spaceshipCatalog.size() << " spaceship definitions" << std::endl;

    if (spaceshipCatalog.empty()) {
        std::cout << "ERROR: No spaceship definitions loaded!" << std::endl;
        return 1;
    }

    // Print some basic info about loaded spaceships
    for (const auto& ship : spaceshipCatalog) {
        std::cout << "Loaded spaceship: " << ship.className
                  << " (class: " << static_cast<int>(ship.classType)
                  << ", mass: " << ship.baseline.minMassTons << " tons)" << std::endl;
    }

    std::cout << "Spaceship JSON loading test completed successfully!" << std::endl;
    return 0;
}