#include "engine/ecs/System.h"
#include <iostream>

int main() {
    std::cout << "Creating UnifiedSystem instance..." << std::endl;
    UnifiedSystem system(SystemType::Physics);
    std::cout << "UnifiedSystem created successfully!" << std::endl;
    return 0;
}