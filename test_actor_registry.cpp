#include "ActorRegistry.h"
#include "Actors.h"  // Include all actor headers for registration
#include <iostream>

int main() {
    auto& registry = ActorRegistry::Instance();
    auto names = registry.RegisteredActorNames();

    std::cout << "Registered Actors:" << std::endl;
    for (const auto& name : names) {
        std::cout << "  - " << name << std::endl;
    }

    std::cout << "\nTotal registered actors: " << names.size() << std::endl;

    // Test creating the TestNPC
    if (registry.IsRegistered("TestNPC")) {
        std::cout << "✓ TestNPC is successfully registered!" << std::endl;
    } else {
        std::cout << "✗ TestNPC registration failed!" << std::endl;
    }

    return 0;
}