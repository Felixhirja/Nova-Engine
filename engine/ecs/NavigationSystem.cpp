#include "NavigationSystem.h"

#include "ecs/EntityManager.h"

void NavigationSystem::Update(EntityManager& entityManager, double dt) {
    (void)dt;
    auto movements = entityManager.GetAllWith<MovementBounds>();
    for (auto& [entity, bounds] : movements) {
        if (!bounds) continue;
        NavigationGrid grid = builder_.BuildFromBounds(*bounds, 2.0);
        if (auto* existing = entityManager.GetComponent<NavigationGrid>(entity)) {
            *existing = grid;
        } else {
            entityManager.EmplaceComponent<NavigationGrid>(entity) = grid;
        }
    }
}

