#ifndef SIMULATION_H
#define SIMULATION_H

#include "Player.h"

class Simulation {
public:
    Simulation();
    ~Simulation();

    void Init();
    // Update the simulation by dt seconds
    void Update(double dt);
    // Simple debug: get position
    double GetPosition() const;
    double GetPlayerX() const;

private:
    double position; // 1D position for demo
    double velocity; // units per second
    Player* player;
};

#endif // SIMULATION_H
