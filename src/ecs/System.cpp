#include "System.h"
#include "EntityManager.h"

void SystemManager::Clear() {
    systems.clear();
}

void SystemManager::UpdateAll(EntityManager& entityManager, double dt) {
    for (auto& system : systems) {
        if (system) {
            system->Update(entityManager, dt);
        }
    }
}
