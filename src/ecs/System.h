#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class EntityManager;

class System {
public:
    virtual ~System() = default;
    virtual void Update(EntityManager& entityManager, double dt) = 0;
};

class SystemManager {
public:
    SystemManager() = default;
    ~SystemManager() = default;

    template<typename T, typename... Args>
    T& RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of<System, T>::value, "System must derive from System base class");
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        systems.emplace_back(std::move(system));
        return ref;
    }

    void Clear();
    void UpdateAll(EntityManager& entityManager, double dt);

private:
    std::vector<std::unique_ptr<System>> systems;
};
