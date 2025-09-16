#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <typeindex>
#include <typeinfo>
#include <cassert>
#include "Component.h"

using Entity = int;

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity e);

    template<typename T>
    void AddComponent(Entity e, std::shared_ptr<T> comp) {
        auto &map = components[std::type_index(typeid(T))];
        map[e] = comp;
    }

    template<typename T>
    T* GetComponent(Entity e) {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto &map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<T*>(jt->second.get());
    }

    template<typename T>
    const T* GetComponent(Entity e) const {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto &map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<const T*>(jt->second.get());
    }

    template<typename T>
    std::vector<std::pair<Entity, T*>> GetAllWith() {
        std::vector<std::pair<Entity, T*>> out;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return out;
        for (auto &kv : it->second) {
            out.emplace_back(kv.first, static_cast<T*>(kv.second.get()));
        }
        return out;
    }

private:
    Entity nextEntity = 1;
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::shared_ptr<Component>>> components;
};
