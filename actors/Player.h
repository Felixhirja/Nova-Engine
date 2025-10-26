#ifndef PLAYER_H
#define PLAYER_H

#include "../src/ecs/EntityManager.h"
#include "../src/ecs/Components.h"

class Player {
public:
    Player() = default;

    void AttachManager(EntityManager* manager);
    void BindEntity(Entity entity);
    void Clear();

    bool IsBound() const;
    Entity GetEntity() const;

    Position* PositionComponent();
    const Position* PositionComponent() const;
    Velocity* VelocityComponent();
    MovementParameters* MovementParametersComponent();
    MovementBounds* MovementBoundsComponent();
    PlayerController* ControllerComponent();
    PlayerPhysics* PhysicsComponent();

    double X() const;
    double Y() const;
    double Z() const;
    void SetPosition(double x, double y, double z);

private:
    EntityManager* entityManager_ = nullptr;
    Entity entity_ = 0;
};

#endif // PLAYER_H
