
#include "Simulation.h"
#include <iostream>

Simulation::Simulation() : position(0.0), velocity(1.0), player(nullptr) {}

Simulation::~Simulation() {
    if (player) {
        delete player;
        player = nullptr;
    }
}

void Simulation::Init() {
    position = 0.0;
    velocity = 1.0; // 1 unit per second
    std::cout << "Simulation initialized. position=" << position << " velocity=" << velocity << std::endl;
    player = new Player();
    player->Init();
}

void Simulation::Update(double dt) {
    position += velocity * dt;
    if (player) player->Update(dt);
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    return player ? player->GetX() : 0.0;
}

