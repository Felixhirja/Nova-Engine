#include "../engine/ecs/LocomotionSystem.h"
#include "../engine/ecs/PlayerControlSystem.h"
#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"

#include <cmath>
#include <iostream>
#include <memory>

namespace {
bool NearlyEqual(double a, double b, double epsilon = 1e-2) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    EntityManager em;
    Entity player = em.CreateEntity();

    auto position = std::make_shared<Position>();
    auto velocity = std::make_shared<Velocity>();
    auto controller = std::make_shared<PlayerController>();
    auto physics = std::make_shared<PlayerPhysics>();
    auto movement = std::make_shared<MovementParameters>();
    auto locomotion = std::make_shared<LocomotionStateMachine>();
    auto collision = std::make_shared<CollisionInfo>();

    physics->isGrounded = true;
    physics->gravity = -9.8;
    physics->jumpImpulse = 6.0;
    locomotion->stamina = locomotion->maxStamina;
    locomotion->heat = 0.0;
    locomotion->baseJumpImpulse = physics->jumpImpulse;
    locomotion->activeSurfaceType = locomotion->defaultSurfaceType;

    em.AddComponent<Position>(player, position);
    em.AddComponent<Velocity>(player, velocity);
    em.AddComponent<PlayerController>(player, controller);
    em.AddComponent<PlayerPhysics>(player, physics);
    em.AddComponent<MovementParameters>(player, movement);
    em.AddComponent<LocomotionStateMachine>(player, locomotion);
    em.AddComponent<CollisionInfo>(player, collision);

    LocomotionSystem locomotionSystem;

    const double dt = 0.016;

    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Idle) {
        std::cerr << "Expected idle state initially" << std::endl;
        return 1;
    }

    controller->moveForward = true;
    velocity->vy = locomotion->walkSpeedThreshold + 0.25;
    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Walk) {
        std::cerr << "Expected walk state when moving" << std::endl;
        return 2;
    }

    controller->sprint = true;
    locomotion->stamina = locomotion->maxStamina;
    velocity->vy = locomotion->sprintSpeedThreshold + 0.5;
    double preSprintStamina = locomotion->stamina;
    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Sprint) {
        std::cerr << "Expected sprint state when sprinting" << std::endl;
        return 3;
    }
    if (!(locomotion->stamina < preSprintStamina)) {
        std::cerr << "Expected sprint to drain stamina" << std::endl;
        return 4;
    }

    controller->moveForward = false;
    controller->sprint = false;
    controller->crouch = true;
    velocity->vy = 0.0;
    physics->isGrounded = true;
    for (int i = 0; i < 6; ++i) {
        locomotionSystem.Update(em, dt);
    }
    if (locomotion->currentState != LocomotionStateMachine::State::Crouch) {
        std::cerr << "Expected crouch state" << std::endl;
        return 5;
    }
    if (!NearlyEqual(locomotion->currentCameraOffset, locomotion->crouchCameraOffset, 0.05)) {
        std::cerr << "Crouch camera offset did not settle" << std::endl;
        return 6;
    }

    controller->crouch = false;
    controller->slide = true;
    locomotion->slideCooldownTimer = 0.0;
    locomotion->slideTimer = 0.0;
    velocity->vy = locomotion->slideSpeedThreshold + 0.75;
    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Slide) {
        std::cerr << "Expected slide state" << std::endl;
        return 7;
    }
    if (!(locomotion->slideTimer > 0.0)) {
        std::cerr << "Slide should start timer" << std::endl;
        return 8;
    }

    controller->slide = false;
    physics->isGrounded = false;
    locomotion->wasGrounded = true;
    velocity->vz = 1.0;
    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Airborne) {
        std::cerr << "Expected airborne state" << std::endl;
        return 9;
    }

    physics->isGrounded = true;
    locomotion->wasGrounded = false;
    velocity->vz = -2.0;
    locomotionSystem.Update(em, dt);
    if (locomotion->currentState != LocomotionStateMachine::State::Landing) {
        std::cerr << "Expected landing state" << std::endl;
        return 10;
    }

    Entity hazard = em.CreateEntity();
    auto hazardSurface = std::make_shared<EnvironmentSurface>();
    hazardSurface->surfaceType = LocomotionSurfaceType::Spacewalk;
    hazardSurface->overridesProfile = true;
    hazardSurface->movementProfile.gravityMultiplier = 0.2;
    hazardSurface->movementProfile.accelerationMultiplier = 0.5;
    hazardSurface->movementProfile.decelerationMultiplier = 0.5;
    hazardSurface->movementProfile.maxSpeedMultiplier = 0.6;
    hazardSurface->isHazard = true;
    hazardSurface->hazardModifier.speedMultiplier = 0.7;
    hazardSurface->hazardModifier.accelerationMultiplier = 0.7;
    hazardSurface->hazardModifier.gravityMultiplier = 0.3;
    hazardSurface->hazardModifier.heatGainRate = 100.0;
    em.AddComponent<EnvironmentSurface>(hazard, hazardSurface);

    collision->contacts.clear();
    CollisionInfo::Contact contact;
    contact.otherEntity = hazard;
    contact.normalZ = 1.0;
    collision->contacts.push_back(contact);
    collision->collisionCount = static_cast<int>(collision->contacts.size());
    controller->moveForward = false;
    controller->sprint = false;
    controller->crouch = false;
    velocity->vx = 0.0;
    velocity->vy = 0.0;
    physics->isGrounded = true;
    locomotion->heat = 0.0;
    locomotionSystem.Update(em, dt);

    if (locomotion->activeSurfaceType != LocomotionSurfaceType::Spacewalk) {
        std::cerr << "Expected spacewalk surface activation" << std::endl;
        return 11;
    }
    if (!(locomotion->runtimeMaxSpeedMultiplier < 1.0)) {
        std::cerr << "Expected hazard to reduce max speed" << std::endl;
        return 12;
    }
    double expectedGravityScale = hazardSurface->movementProfile.gravityMultiplier *
                                  hazardSurface->hazardModifier.gravityMultiplier;
    if (!NearlyEqual(locomotion->runtimeGravityMultiplier, expectedGravityScale, 1e-2)) {
        std::cerr << "Hazard gravity multiplier mismatch" << std::endl;
        return 13;
    }
    if (!(locomotion->heat > 0.0)) {
        std::cerr << "Hazard should add heat" << std::endl;
        return 14;
    }

    collision->contacts.clear();
    collision->collisionCount = 0;
    controller->boost = true;
    double heatBeforeBoost = locomotion->heat;
    locomotionSystem.Update(em, dt);
    if (!locomotion->boostActive) {
        std::cerr << "Boost should activate" << std::endl;
        return 15;
    }
    if (!(locomotion->heat > heatBeforeBoost)) {
        std::cerr << "Boost should add heat" << std::endl;
        return 16;
    }

    controller->boost = false;
    double boostedHeat = locomotion->heat;
    for (int i = 0; i < 60; ++i) {
        locomotionSystem.Update(em, dt);
    }
    if (!(locomotion->heat < boostedHeat)) {
        std::cerr << "Heat should dissipate over time" << std::endl;
        return 17;
    }

    std::cout << "Locomotion state machine tests passed" << std::endl;
    return 0;
}
