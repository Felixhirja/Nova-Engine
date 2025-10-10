#ifndef SIMULATION_H
#define SIMULATION_H

#include "ecs/Components.h"
#include "ecs/EntityManager.h"
#include "ecs/System.h"

class Simulation {
public:
    Simulation();
    ~Simulation();

    void Init(EntityManager* em = nullptr);
    // Update the simulation by dt seconds
    void Update(double dt);
    // Simple debug: get position
    double GetPosition() const;
    double GetPlayerX() const;
    double GetPlayerY() const;
    double GetPlayerZ() const;
    Entity GetPlayerEntity() const { return playerEntity; }
    // Set player input state
    void SetPlayerInput(bool forward, bool backward, bool up, bool down, bool strafeLeft, bool strafeRight, double cameraYaw);
    void SetUseThrustMode(bool thrustMode);

private:
    // Basic simulation state implemented with ECS
    EntityManager em;
    EntityManager* activeEm = nullptr;
    Entity playerEntity = 0;
    // fallback scalar for global sim position if needed
    double position = 0.0;
    bool inputForward;
    bool inputBackward;
    bool inputUp;
    bool inputDown;
    bool inputStrafeLeft;
    bool inputStrafeRight;
    double inputCameraYaw;
    bool prevJumpHeld;
    bool useThrustMode;
    bool inputLeft;
    bool inputRight;
    SystemManager systemManager;
};

#endif // SIMULATION_H
