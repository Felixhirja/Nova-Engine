#include "EntityManager.h"

EntityManager::EntityManager() {}
EntityManager::~EntityManager() {}

Entity EntityManager::CreateEntity() {
    Entity e = 0;
    if (!freeEntities.empty()) {
        e = freeEntities.back();
        freeEntities.pop_back();
    } else {
        e = nextEntity++;
    }
    aliveEntities.insert(e);
    return e;
}

void EntityManager::DestroyEntity(Entity e) {
    if (aliveEntities.erase(e) == 0) {
        return;
    }
    for (auto &kv : components) {
        kv.second.erase(e);
    }
    freeEntities.push_back(e);
}

bool EntityManager::IsAlive(Entity e) const {
    return aliveEntities.find(e) != aliveEntities.end();
}

void EntityManager::Clear() {
    components.clear();
    aliveEntities.clear();
    freeEntities.clear();
    nextEntity = 1;
}
