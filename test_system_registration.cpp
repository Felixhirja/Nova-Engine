#include "engine/ecs/System.h"
#include <iostream>

int main() {
    std::cout << "Creating SystemManager..." << std::endl;
    SystemManager systemManager;
    std::cout << "SystemManager created successfully!" << std::endl;

    std::cout << "Registering PlayerControl system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::PlayerControl);
    std::cout << "PlayerControl registered!" << std::endl;

    std::cout << "Registering Movement system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Movement);
    std::cout << "Movement registered!" << std::endl;

    std::cout << "Registering Locomotion system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Locomotion);
    std::cout << "Locomotion registered!" << std::endl;

    std::cout << "Registering ShipAssembly system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::ShipAssembly);
    std::cout << "ShipAssembly registered!" << std::endl;

    std::cout << "Registering SpaceshipPhysics system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::SpaceshipPhysics);
    std::cout << "SpaceshipPhysics registered!" << std::endl;

    std::cout << "Registering Animation system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Animation);
    std::cout << "Animation registered!" << std::endl;

    std::cout << "Registering Targeting system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Targeting);
    std::cout << "Targeting registered!" << std::endl;

    std::cout << "Registering Weapon system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Weapon);
    std::cout << "Weapon registered!" << std::endl;

    std::cout << "Registering Shield system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Shield);
    std::cout << "Shield registered!" << std::endl;

    std::cout << "Registering BehaviorTree system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::BehaviorTree);
    std::cout << "BehaviorTree registered!" << std::endl;

    std::cout << "Registering Navigation system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::Navigation);
    std::cout << "Navigation registered!" << std::endl;

    std::cout << "Registering GameplayEvent system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::GameplayEvent);
    std::cout << "GameplayEvent registered!" << std::endl;

    std::cout << "Registering MissionScript system..." << std::endl;
    systemManager.RegisterUnifiedSystem(SystemType::MissionScript);
    std::cout << "MissionScript registered!" << std::endl;

    std::cout << "All systems registered successfully!" << std::endl;
    return 0;
}