

#include "Simulation.h"
#include <iostream>
#include <memory>
#include <cmath>

Simulation::Simulation() : entityManager(nullptr), inputForward(false), inputBackward(false), inputUp(false), inputDown(false), inputStrafeLeft(false), inputStrafeRight(false), inputCameraYaw(0.0) {}

Simulation::~Simulation() {}

void Simulation::Init(EntityManager* externalEm) {
    position = 0.0;
    std::cout << "Simulation initialized. position=" << position << std::endl;

    entityManager = externalEm;
    if (!entityManager) {
        std::cerr << "Simulation: No EntityManager provided!" << std::endl;
        return;
    }

    // Create player entity in ECS
    playerEntity = entityManager->CreateEntity();
    auto pos = std::make_shared<Position>();
    pos->x = 0.0; pos->y = 0.0; pos->z = 0.0;
    entityManager->AddComponent<Position>(playerEntity, pos);

    auto vel = std::make_shared<Velocity>();
    vel->vx = 1.0; vel->vy = 0.0; vel->vz = 0.0;
    entityManager->AddComponent<Velocity>(playerEntity, vel);

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;
}

void Simulation::Update(double dt) {
    if (!entityManager) return;
    
    // advance global simulation position
    auto v = entityManager->GetComponent<Velocity>(playerEntity);
    auto p = entityManager->GetComponent<Position>(playerEntity);
    if (v && p) {
        // Compute desired velocity in local space
        double localVx = 0.0, localVy = 0.0, localVz = 0.0;
        double speed = 5.0; // movement speed
        if (inputForward) localVx += speed;
        if (inputBackward) localVx -= speed;
        if (inputUp) localVy += speed;
        if (inputDown) localVy -= speed;
        if (inputStrafeLeft) localVz -= speed;
        if (inputStrafeRight) localVz += speed;
        
        // Rotate by camera yaw to get world velocity
        double cosYaw = cos(inputCameraYaw);
        double sinYaw = sin(inputCameraYaw);
        double worldVx = localVx * cosYaw - localVz * sinYaw;
        double worldVz = localVx * sinYaw + localVz * cosYaw;
        double worldVy = localVy; // up/down not rotated
        
        // Set velocity
        v->vx = worldVx;
        v->vy = worldVy;
        v->vz = worldVz;
        
        // Update position
        p->x += v->vx * dt;
        p->y += v->vy * dt;
        p->z += v->vz * dt;
        position = p->x; // mirror into simple position for compatibility
    }
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    if (!entityManager) return 0.0;
    auto p = entityManager->GetComponent<Position>(playerEntity);
    return p ? p->x : 0.0;
}

double Simulation::GetPlayerY() const {
    if (!entityManager) return 0.0;
    auto p = entityManager->GetComponent<Position>(playerEntity);
    return p ? p->y : 0.0;
}

double Simulation::GetPlayerZ() const {
    if (!entityManager) return 0.0;
    auto p = entityManager->GetComponent<Position>(playerEntity);
    return p ? p->z : 0.0;
}

void Simulation::SetPlayerInput(bool forward, bool backward, bool up, bool down, bool strafeLeft, bool strafeRight, double cameraYaw) {
    inputForward = forward;
    inputBackward = backward;
    inputUp = up;
    inputDown = down;
    inputStrafeLeft = strafeLeft;
    inputStrafeRight = strafeRight;
    inputCameraYaw = cameraYaw;
}


