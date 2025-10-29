#include "Spaceship.h"

Entity SpawnSpaceship(EntityManager& entityManager, const SpaceshipSpawnBundle& bundle) {
    Entity entity = entityManager.CreateEntity();
    auto& tag = entityManager.EmplaceComponent<SpaceshipTag>(entity);
    tag.classId = bundle.classId;
    tag.displayName = bundle.displayName;
    tag.loadoutIndex = bundle.loadoutIndex;
    tag.playerControlled = bundle.playerControlled;

    if (!bundle.displayName.empty()) {
        auto& name = entityManager.EmplaceComponent<Name>(entity);
        name.value = bundle.displayName;
    }
    return entity;
}
