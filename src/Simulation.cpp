

#include "Simulation.h"
#include <iostream>
#include <memory>

Simulation::Simulation() : inputLeft(false), inputRight(false) {}

Simulation::~Simulation() {}

void Simulation::Init(EntityManager* externalEm) {
    position = 0.0;
    std::cout << "Simulation initialized. position=" << position << std::endl;

    EntityManager* useEm = externalEm ? externalEm : &em;

    // Create player entity in ECS
    playerEntity = useEm->CreateEntity();
    auto pos = std::make_shared<Position>();
    pos->x = 0.0; pos->y = 0.0;
    useEm->AddComponent<Position>(playerEntity, pos);

    auto vel = std::make_shared<Velocity>();
    vel->vx = 1.0; vel->vy = 0.0;
    useEm->AddComponent<Velocity>(playerEntity, vel);

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;
}

void Simulation::Update(double dt) {
    // advance global simulation position
    auto v = em.GetComponent<Velocity>(playerEntity);
    auto p = em.GetComponent<Position>(playerEntity);
    if (v && p) {
        // simple input: left/right adjust vx
        if (inputLeft) v->vx = -std::abs(v->vx);
        if (inputRight) v->vx = std::abs(v->vx);
        p->x += v->vx * dt;
        p->y += v->vy * dt;
        position = p->x; // mirror into simple position for compatibility
    }
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    auto p = em.GetComponent<Position>(playerEntity);
    return p ? p->x : 0.0;
}

void Simulation::SetPlayerInput(bool left, bool right) {
    inputLeft = left;
    inputRight = right;
}


