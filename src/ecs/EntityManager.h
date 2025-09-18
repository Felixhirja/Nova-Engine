#pragma once
#include <cassert>
#include <memory>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "Component.h"

using Entity = int;

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity e);

    bool IsAlive(Entity e) const;

    template<typename T>
    void AddComponent(Entity e, std::shared_ptr<T> comp) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        assert(IsAlive(e) && "Adding component to non-existent entity");
        auto &map = components[std::type_index(typeid(T))];
        map[e] = std::move(comp);
    }

    template<typename T, typename... Args>
    T& EmplaceComponent(Entity e, Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        AddComponent<T>(e, ptr);
        return *ptr;
    }

    template<typename T>
    void RemoveComponent(Entity e) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return;
        it->second.erase(e);
    }

    template<typename T>
    bool HasComponent(Entity e) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return false;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return false;
        return it->second.find(e) != it->second.end();
    }

    template<typename T>
    T* GetComponent(Entity e) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return nullptr;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto &map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<T*>(jt->second.get());
    }

    template<typename T>
    const T* GetComponent(Entity e) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        if (!IsAlive(e)) return nullptr;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return nullptr;
        auto &map = it->second;
        auto jt = map.find(e);
        if (jt == map.end()) return nullptr;
        return static_cast<const T*>(jt->second.get());
    }

    template<typename T>
    std::vector<std::pair<Entity, T*>> GetAllWith() {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        std::vector<std::pair<Entity, T*>> out;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return out;
        for (auto &kv : it->second) {
            out.emplace_back(kv.first, static_cast<T*>(kv.second.get()));
        }
        return out;
    }

    template<typename T>
    std::vector<std::pair<Entity, const T*>> GetAllWith() const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        std::vector<std::pair<Entity, const T*>> out;
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return out;
        for (const auto &kv : it->second) {
            out.emplace_back(kv.first, static_cast<const T*>(kv.second.get()));
        }
        return out;
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return;
        for (auto &kv : it->second) {
            Entity ent = kv.first;
            auto* first = static_cast<T*>(kv.second.get());
            if (!first) continue;
            if constexpr (sizeof...(Ts) == 0) {
                func(ent, *first);
            } else {
                auto rest = std::tuple<Ts*...>{GetComponent<Ts>(ent)...};
                bool all = true;
                std::apply([&](auto*... ptrs) {
                    ((all = all && (ptrs != nullptr)), ...);
                }, rest);
                if (!all) continue;
                std::apply([&](auto*... ptrs) {
                    func(ent, *first, *ptrs...);
                }, rest);
            }
        }
    }

    template<typename T, typename... Ts, typename Func>
    void ForEach(Func&& func) const {
        static_assert(std::is_base_of<Component, T>::value,
                      "Component must derive from Component base class");
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) return;
        for (const auto &kv : it->second) {
            Entity ent = kv.first;
            auto* first = static_cast<const T*>(kv.second.get());
            if (!first) continue;
            if constexpr (sizeof...(Ts) == 0) {
                func(ent, *first);
            } else {
                auto rest = std::tuple<const Ts*...>{GetComponent<Ts>(ent)...};
                bool all = true;
                std::apply([&](auto*... ptrs) {
                    ((all = all && (ptrs != nullptr)), ...);
                }, rest);
                if (!all) continue;
                std::apply([&](auto*... ptrs) {
                    func(ent, *first, *ptrs...);
                }, rest);
            }
        }
    }

    void Clear();

private:
    Entity nextEntity = 1;
    std::unordered_set<Entity> aliveEntities;
    std::vector<Entity> freeEntities;
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::shared_ptr<Component>>> components;
};
