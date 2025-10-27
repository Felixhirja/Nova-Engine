#include "../engine/ecs/System.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <memory>

// Mock systems with dependencies
class PhysicsSystem : public System {
public:
    void Update(EntityManager& entityManager, double dt) override {
        // Simulate physics update
        updated = true;
    }
    bool updated = false;
};

class AISystem : public System {
public:
    AISystem(PhysicsSystem& physics) : physics_(physics) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated) {
            std::cerr << "AISystem updated before PhysicsSystem" << std::endl;
            orderError = true;
        }
        updated = true;
    }
    PhysicsSystem& physics_;
    bool updated = false;
    bool orderError = false;
};

class RenderSystem : public System {
public:
    RenderSystem(PhysicsSystem& physics, AISystem& ai) : physics_(physics), ai_(ai) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated || !ai_.updated) {
            std::cerr << "RenderSystem updated before dependencies" << std::endl;
            orderError = true;
        }
        updated = true;
    }
    PhysicsSystem& physics_;
    AISystem& ai_;
    bool updated = false;
    bool orderError = false;
};

bool TestDependencyOrder() {
    SystemManager manager;
    EntityManager em;

    // Register in dependency order
    PhysicsSystem& physics = manager.RegisterSystem<PhysicsSystem>();
    AISystem& ai = manager.RegisterSystem<AISystem>(physics);
    RenderSystem& render = manager.RegisterSystem<RenderSystem>(physics, ai);

    manager.UpdateAll(em, 1.0);

    if (!physics.updated || !ai.updated || !render.updated) {
        std::cerr << "All systems should be updated" << std::endl;
        return false;
    }

    if (ai.orderError || render.orderError) {
        std::cerr << "Dependency order not respected" << std::endl;
        return false;
    }

    return true;
}

bool TestComplexGraph() {
    SystemManager manager;
    EntityManager em;

    // Simulate a more complex graph
    PhysicsSystem& physics = manager.RegisterSystem<PhysicsSystem>();
    AISystem& ai = manager.RegisterSystem<AISystem>(physics);
    RenderSystem& render = manager.RegisterSystem<RenderSystem>(physics, ai);

    // Add another independent system
    class AudioSystem : public System {
    public:
        void Update(EntityManager& entityManager, double dt) override {
            updated = true;
        }
        bool updated = false;
    };

    AudioSystem& audio = manager.RegisterSystem<AudioSystem>();

    manager.UpdateAll(em, 1.0);

    if (!physics.updated || !ai.updated || !render.updated || !audio.updated) {
        std::cerr << "All systems should be updated" << std::endl;
        return false;
    }

    return true;
}

int main() {
    if (!TestDependencyOrder()) {
        return 1;
    }

    if (!TestComplexGraph()) {
        return 2;
    }

    std::cout << "SystemManager integration tests passed." << std::endl;
    return 0;
}