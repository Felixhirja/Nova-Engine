

#include "Simulation.h"
#include "ecs/MovementSystem.h"
#include "ecs/PlayerControlSystem.h"

#include <algorithm>
#include <iostream>
#include <memory>

Simulation::Simulation() : inputLeft(false), inputRight(false) {
    activeEm = &em;
}

Simulation::~Simulation() {}

void Simulation::Init(EntityManager* externalEm) {
    position = 0.0;
    std::cout << "Simulation initialized. position=" << position << std::endl;

    activeEm = externalEm ? externalEm : &em;
    EntityManager* useEm = activeEm;

    if (!externalEm) {
        useEm->Clear();
    }

    systemManager.Clear();
    systemManager.RegisterSystem<PlayerControlSystem>();
    systemManager.RegisterSystem<MovementSystem>();

    // Create player entity in ECS
    playerEntity = useEm->CreateEntity();
    auto pos = std::make_shared<Position>();
    pos->x = 0.0;
    pos->y = 0.0;
    useEm->AddComponent<Position>(playerEntity, pos);

    auto vel = std::make_shared<Velocity>();
    vel->vx = 0.0;
    vel->vy = 0.0;
    useEm->AddComponent<Velocity>(playerEntity, vel);

    auto acc = std::make_shared<Acceleration>();
    acc->ax = 0.0;
    acc->ay = 0.0;
    useEm->AddComponent<Acceleration>(playerEntity, acc);

    auto controller = std::make_shared<PlayerController>();
    controller->moveLeft = false;
    controller->moveRight = false;
    useEm->AddComponent<PlayerController>(playerEntity, controller);

    auto bounds = std::make_shared<MovementBounds>();
    bounds->minX = -5.0;
    bounds->maxX = 5.0;
    bounds->clampX = true;
    useEm->AddComponent<MovementBounds>(playerEntity, bounds);

    inputLeft = false;
    inputRight = false;

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;
}

void Simulation::Update(double dt) {
    if (dt <= 0.0) {
        return;
    }

    EntityManager* useEm = activeEm ? activeEm : &em;

    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        controller->moveLeft = inputLeft;
        controller->moveRight = inputRight;
    }

    systemManager.UpdateAll(*useEm, dt);

    if (auto* p = useEm->GetComponent<Position>(playerEntity)) {
        position = p->x;
    }
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->x : 0.0;
}

void Simulation::SetPlayerInput(bool left, bool right) {
    inputLeft = left;
    inputRight = right;
}


