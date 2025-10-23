#include "../src/ecs/Components.h"
#include "../src/ecs/EntityManager.h"
#include "../src/ecs/EntityManagerFacade.h"
#include "../src/ecs/EntityManagerV2.h"

#include <cassert>
#include <iostream>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

int main() {
    {
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
    }

    {
        EntityManager legacy;
        Entity e1 = legacy.CreateEntity();
        Entity e2 = legacy.CreateEntity();
        Entity e3 = legacy.CreateEntity();

        auto p1 = std::make_shared<Position>();
        p1->x = 1.0;
        legacy.AddComponent<Position>(e1, p1);

        auto v1 = std::make_shared<Velocity>();
        v1->vx = 2.0;
        legacy.AddComponent<Velocity>(e1, v1);

        auto locomotion = std::make_shared<LocomotionStateMachine>();
        locomotion->currentState = LocomotionStateMachine::State::Walk;
        legacy.AddComponent<LocomotionStateMachine>(e1, locomotion);

        auto p2 = std::make_shared<Position>();
        p2->x = -4.0;
        p2->y = 3.5;
        legacy.AddComponent<Position>(e2, p2);

        auto controller = std::make_shared<PlayerController>();
        controller->moveForward = true;
        legacy.AddComponent<PlayerController>(e2, controller);

        auto p3 = std::make_shared<Position>();
        p3->x = 9.0;
        p3->z = 1.5;
        legacy.AddComponent<Position>(e3, p3);

        auto projectile = std::make_shared<Projectile>();
        projectile->ownerEntity = static_cast<int>(e1);
        legacy.AddComponent<Projectile>(e3, projectile);

        ecs::EntityManagerV2 modern;
        std::unordered_map<Entity, ecs::EntityHandle> legacyToModern;
        std::unordered_map<uint32_t, Entity> modernToLegacy;
        std::unordered_set<std::type_index> unsupported;

        legacy.MigrateToArchetypeManager(modern, legacyToModern, modernToLegacy, unsupported);

        assert(legacyToModern.size() == 3);
        assert(modernToLegacy.size() == 3);
        assert(unsupported.find(std::type_index(typeid(Projectile))) != unsupported.end());

        size_t positionCount = 0;
        modern.ForEach<Position>([&](ecs::EntityHandle handle, Position& pos) {
            ++positionCount;
            auto legacyIt = modernToLegacy.find(handle.value);
            assert(legacyIt != modernToLegacy.end());
            if (legacyIt->second == e1) {
                assert(pos.x == 1.0);
            }
            if (legacyIt->second == e2) {
                assert(pos.x == -4.0);
                assert(pos.y == 3.5);
            }
            if (legacyIt->second == e3) {
                assert(pos.x == 9.0);
                assert(pos.z == 1.5);
            }
        });
        assert(positionCount == 3);

        size_t locomotionCount = 0;
        modern.ForEach<LocomotionStateMachine>([&](ecs::EntityHandle handle, LocomotionStateMachine& loc) {
            ++locomotionCount;
            auto legacyIt = modernToLegacy.find(handle.value);
            assert(legacyIt != modernToLegacy.end());
            assert(legacyIt->second == e1);
            assert(loc.currentState == LocomotionStateMachine::State::Walk);
        });
        assert(locomotionCount == 1);
    }

    {
        EntityManager legacy;
        ecs::EntityManagerFacade facade(legacy);

        Entity e = facade.CreateEntity();
        auto name = std::make_shared<Name>();
        name->value = "Facaded";
        facade.AddComponent<Name>(e, name);

        auto* stored = facade.GetComponent<Name>(e);
        assert(stored != nullptr);
        assert(stored->value == "Facaded");

        size_t facadeCount = 0;
        facade.ForEach<Name>([&](Entity entity, Name& comp) {
            if (entity == e) {
                ++facadeCount;
                comp.value = "Updated";
            }
        });
        assert(facadeCount == 1);

        size_t archetypeCount = 0;
        legacy.GetArchetypeManager().ForEach<Name>([&](ecs::EntityHandle, Name& comp) {
            if (comp.value == "Updated") {
                ++archetypeCount;
            }
        });
        assert(archetypeCount == 1);

        facade.RemoveComponent<Name>(e);
        assert(!facade.HasComponent<Name>(e));
    }

    std::cout << "Entity migration regressions passed" << std::endl;
    return 0;
}

