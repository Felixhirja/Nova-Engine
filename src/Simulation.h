#ifndef SIMULATION_H
#define SIMULATION_H

#include "ecs/EntityManager.h"
#include "ecs/Components.h"

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
    // Set player input state
    void SetPlayerInput(bool left, bool right);

private:
    // Basic simulation state implemented with ECS
    EntityManager em;
    Entity playerEntity = 0;
    // fallback scalar for global sim position if needed
    double position = 0.0;
    bool inputLeft;
    bool inputRight;
};

#endif // SIMULATION_H
