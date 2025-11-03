#include "engine/ecs/System.h"
#include "engine/ecs/EntityManager.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Creating EntityManager..." << std::endl;
    EntityManager em;
    std::cout << "EntityManager created successfully!" << std::endl;

    std::cout << "Creating player entity..." << std::endl;
    Entity playerEntity = em.CreateEntity();
    std::cout << "Player entity created: " << playerEntity << std::endl;

    std::cout << "Adding Position component..." << std::endl;
    auto pos = std::make_shared<Position>();
    pos->x = 0.0;
    pos->y = 0.0;
    em.AddComponent<Position>(playerEntity, pos);
    std::cout << "Position component added!" << std::endl;

    std::cout << "Adding Velocity component..." << std::endl;
    auto vel = std::make_shared<Velocity>();
    vel->vx = 0.0;
    vel->vy = 0.0;
    em.AddComponent<Velocity>(playerEntity, vel);
    std::cout << "Velocity component added!" << std::endl;

    std::cout << "Adding Acceleration component..." << std::endl;
    auto acc = std::make_shared<Acceleration>();
    acc->ax = 0.0;
    acc->ay = 0.0;
    acc->az = 0.0;
    em.AddComponent<Acceleration>(playerEntity, acc);
    std::cout << "Acceleration component added!" << std::endl;

    std::cout << "Adding PlayerController component..." << std::endl;
    auto controller = std::make_shared<PlayerController>();
    controller->moveLeft = false;
    controller->moveRight = false;
    controller->moveForward = false;
    controller->moveBackward = false;
    controller->moveUp = false;
    controller->moveDown = false;
    controller->strafeLeft = false;
    controller->strafeRight = false;
    controller->sprint = false;
    controller->crouch = false;
    controller->slide = false;
    controller->boost = false;
    controller->cameraYaw = 0.0; // Assuming Camera::kDefaultYawRadians is 0.0
    em.AddComponent<PlayerController>(playerEntity, controller);
    std::cout << "PlayerController component added!" << std::endl;

    std::cout << "Adding LocomotionStateMachine component..." << std::endl;
    auto locomotion = std::make_shared<LocomotionStateMachine>();
    locomotion->wasGrounded = true;
    const double forwardMax = 5.0; // Default values
    const double backwardMax = 5.0;
    const double strafeMax = 5.0;
    const double baseSpeed = std::max(forwardMax, std::max(backwardMax, strafeMax));
    if (baseSpeed > 0.0) {
        locomotion->idleSpeedThreshold = std::max(0.1, baseSpeed * 0.1);
        locomotion->walkSpeedThreshold = std::max(locomotion->idleSpeedThreshold + 0.1, baseSpeed * 0.4);
        locomotion->sprintSpeedThreshold = std::max(locomotion->walkSpeedThreshold + 0.1, baseSpeed * 0.85);
        locomotion->slideSpeedThreshold = std::max(locomotion->walkSpeedThreshold, baseSpeed * 0.65);
    }
    locomotion->stamina = locomotion->maxStamina;
    locomotion->heat = 0.0;
    locomotion->activeSurfaceType = locomotion->defaultSurfaceType;
    if (locomotion->surfaceProfiles.count(locomotion->defaultSurfaceType)) {
        locomotion->activeSurfaceProfile = locomotion->surfaceProfiles.at(locomotion->defaultSurfaceType);
    }
    locomotion->activeHazardModifier = locomotion->hazardBaseline;
    locomotion->currentCameraOffset = locomotion->defaultCameraOffset;
    locomotion->baseJumpImpulse = 6.0; // Default value
    em.AddComponent<LocomotionStateMachine>(playerEntity, locomotion);
    std::cout << "LocomotionStateMachine component added!" << std::endl;

    std::cout << "All components added successfully!" << std::endl;
    return 0;
}