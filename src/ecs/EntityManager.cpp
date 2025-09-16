#include "EntityManager.h"

EntityManager::EntityManager() {}
EntityManager::~EntityManager() {}

Entity EntityManager::CreateEntity() {
    return nextEntity++;
}

void EntityManager::DestroyEntity(Entity e) {
    for (auto &kv : components) {
        kv.second.erase(e);
    }
}
