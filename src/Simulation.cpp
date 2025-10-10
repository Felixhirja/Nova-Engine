#include "Simulation.h"
#include "ecs/MovementSystem.h"
#include "ecs/PlayerControlSystem.h"
#include "TargetingSystem.h"
#include "WeaponSystem.h"
#include "ShieldSystem.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <cmath>

Simulation::Simulation() : inputForward(false), inputBackward(false), inputUp(false), inputDown(false), inputStrafeLeft(false), inputStrafeRight(false), inputCameraYaw(0.0), prevJumpHeld(false), useThrustMode(false), inputLeft(false), inputRight(false) {
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
    systemManager.RegisterSystem<TargetingSystem>();
    systemManager.RegisterSystem<WeaponSystem>();
    systemManager.RegisterSystem<ShieldSystem>();

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
    acc->az = 0.0;
    useEm->AddComponent<Acceleration>(playerEntity, acc);

    auto controller = std::make_shared<PlayerController>();
    controller->moveLeft = false;
    controller->moveRight = false;
    controller->moveForward = false;
    controller->moveBackward = false;
    controller->moveUp = false;
    controller->moveDown = false;
    controller->strafeLeft = false;
    controller->strafeRight = false;
    controller->cameraYaw = 0.0;
    useEm->AddComponent<PlayerController>(playerEntity, controller);

    auto bounds = std::make_shared<MovementBounds>();
    bounds->minX = -5.0;
    bounds->maxX = 5.0;
    bounds->clampX = true;
    bounds->minY = -5.0;
    bounds->maxY = 5.0;
    bounds->clampY = true;
    bounds->minZ = 0.0;
    bounds->maxZ = 5.0;
    bounds->clampZ = true;
    useEm->AddComponent<MovementBounds>(playerEntity, bounds);

    auto physics = std::make_shared<PlayerPhysics>();
    physics->thrustMode = useThrustMode;
    physics->enableGravity = true;
    physics->isGrounded = true;
    useEm->AddComponent<PlayerPhysics>(playerEntity, physics);

    auto targetLock = std::make_shared<TargetLock>();
    targetLock->targetEntityId = 0;  // No target initially
    targetLock->isLocked = false;    // Start unlocked
    targetLock->offsetX = 0.0;
    targetLock->offsetY = 5.0;
    targetLock->offsetZ = 10.0;
    targetLock->followDistance = 15.0;
    targetLock->followHeight = 5.0;
    useEm->AddComponent<TargetLock>(playerEntity, targetLock);
    inputRight = false;
    inputForward = false;
    inputBackward = false;
    inputUp = false;
    inputDown = false;
    inputStrafeLeft = false;
    inputStrafeRight = false;
    inputCameraYaw = 0.0;
    prevJumpHeld = false;

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;
}

void Simulation::Update(double dt) {
    if (dt <= 0.0) {
        return;
    }

    EntityManager* useEm = activeEm ? activeEm : &em;

    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        bool jumpJustPressed = inputUp && !prevJumpHeld;
        controller->moveLeft = inputLeft;
        controller->moveRight = inputRight;
        controller->moveForward = inputForward;
        controller->moveBackward = inputBackward;
        controller->moveUp = useThrustMode ? inputUp : false;
        controller->moveDown = inputDown;
        controller->strafeLeft = inputStrafeLeft;
        controller->strafeRight = inputStrafeRight;
        controller->cameraYaw = inputCameraYaw;
        controller->thrustMode = useThrustMode;
        controller->jumpRequested = (!useThrustMode && jumpJustPressed);
    }

    if (auto* physics = useEm->GetComponent<PlayerPhysics>(playerEntity)) {
        physics->thrustMode = useThrustMode;
    }

    systemManager.UpdateAll(*useEm, dt);

    if (auto* p = useEm->GetComponent<Position>(playerEntity)) {
        position = p->x;
    }

    prevJumpHeld = inputUp;
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->x : 0.0;
}

double Simulation::GetPlayerY() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->y : 0.0;
}

double Simulation::GetPlayerZ() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
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

void Simulation::SetUseThrustMode(bool thrustMode) {
    useThrustMode = thrustMode;
    EntityManager* useEm = activeEm ? activeEm : &em;
    if (auto* physics = useEm->GetComponent<PlayerPhysics>(playerEntity)) {
        physics->thrustMode = thrustMode;
    }
    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        controller->thrustMode = thrustMode;
    }
}


