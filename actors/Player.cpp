#pragma once
#include "../src/ecs/EntityManager.h"
#include "../src/ecs/Components.h"

class Player {
public:
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
    template<typename T>
    T* Get();

    template<typename T>
    const T* Get() const;

    EntityManager* entityManager_{nullptr};
    Entity entity_{0};
};

template<typename T>
T* Player::Get() {
    if (!IsBound()) {
        return nullptr;
    }
    return entityManager_->GetComponent<T>(entity_);
}

template<typename T>
const T* Player::Get() const {
    if (!IsBound()) {
        return nullptr;
    }
    return entityManager_->GetComponent<T>(entity_);
}
