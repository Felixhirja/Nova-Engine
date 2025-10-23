#include "../src/ecs/EntityManager.h"
#include "../src/ecs/Components.h"

#include <cassert>
#include <iostream>

int main() {
    EntityManager em;
    Entity entity = em.CreateEntity();

    auto position = std::make_shared<Position>();
    position->x = 42.0;
    position->y = -3.0;
    em.AddComponent<Position>(entity, position);

    assert(!em.UsingArchetypeStorage());

    em.EnableArchetypeFacade();
    assert(em.UsingArchetypeStorage());

    auto* migratedPosition = em.GetComponent<Position>(entity);
    assert(migratedPosition != nullptr);
    assert(migratedPosition->x == 42.0);
    assert(migratedPosition->y == -3.0);

    migratedPosition->x = 10.0;
    auto* postUpdatePosition = em.GetComponent<Position>(entity);
    assert(postUpdatePosition != nullptr);
    assert(postUpdatePosition->x == 10.0);

    auto velocity = std::make_shared<Velocity>();
    velocity->vx = 5.0;
    velocity->vy = 1.5;
    em.AddComponent<Velocity>(entity, velocity);

    auto* migratedVelocity = em.GetComponent<Velocity>(entity);
    assert(migratedVelocity != nullptr);
    assert(migratedVelocity->vx == 5.0);
    assert(migratedVelocity->vy == 1.5);

    em.RemoveComponent<Velocity>(entity);
    assert(!em.HasComponent<Velocity>(entity));

    em.DestroyEntity(entity);
    assert(!em.IsAlive(entity));

    std::cout << "Entity migration regression passed" << std::endl;
    return 0;
}

