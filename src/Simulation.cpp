

#include "Simulation.h"
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

    // Create player entity in ECS
    playerEntity = useEm->CreateEntity();
    auto pos = std::make_shared<Position>();
    pos->x = 0.0; pos->y = 0.0;
    useEm->AddComponent<Position>(playerEntity, pos);

    auto vel = std::make_shared<Velocity>();
    vel->vx = 0.0; vel->vy = 0.0;
    useEm->AddComponent<Velocity>(playerEntity, vel);

    inputLeft = false;
    inputRight = false;

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;
}

void Simulation::Update(double dt) {
    if (dt <= 0.0) {
        return;
    }

    EntityManager* useEm = activeEm ? activeEm : &em;

    // advance global simulation position
    auto v = useEm->GetComponent<Velocity>(playerEntity);
    auto p = useEm->GetComponent<Position>(playerEntity);
    if (v && p) {
        constexpr double acceleration = 4.0;
        constexpr double maxPosition = 5.0;
        constexpr double minPosition = -5.0;

        double ax = 0.0;
        if (inputLeft && !inputRight) {
            ax = -acceleration;
        } else if (inputRight && !inputLeft) {
            ax = acceleration;
        }

        v->vx += ax * dt;

        if (!inputLeft && !inputRight) {
            constexpr double damping = acceleration;
            if (v->vx > 0.0) {
                v->vx = std::max(0.0, v->vx - damping * dt);
            } else if (v->vx < 0.0) {
                v->vx = std::min(0.0, v->vx + damping * dt);
            }
        }

        p->x += v->vx * dt;
        p->y += v->vy * dt;

        if (p->x > maxPosition) {
            p->x = maxPosition;
            if (v->vx > 0.0) {
                v->vx = 0.0;
            }
        } else if (p->x < minPosition) {
            p->x = minPosition;
            if (v->vx < 0.0) {
                v->vx = 0.0;
            }
        }

        position = p->x; // mirror into simple position for compatibility
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


