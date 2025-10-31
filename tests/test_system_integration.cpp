#include "../engine/ecs/System.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <memory>

// Mock systems with dependencies
class MockPhysicsSystem : public System {
public:
    void Update(EntityManager& entityManager, double dt) override {
        // Simulate physics update
        updated = true;
    }
    const char* GetName() const override { return "MockPhysicsSystem"; }
    bool updated = false;
};

class AISystem : public System {
public:
    AISystem(MockPhysicsSystem& physics) : physics_(physics) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated) {
            std::cerr << "AISystem updated before MockPhysicsSystem" << std::endl;
            orderError = true;
        }
        updated = true;
    }
    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        return {ecs::SystemDependency::Requires<MockPhysicsSystem>()};
    }
    const char* GetName() const override { return "AISystem"; }
    MockPhysicsSystem& physics_;
    bool updated = false;
    bool orderError = false;
};

class RenderSystem : public System {
public:
    RenderSystem(MockPhysicsSystem& physics, AISystem& ai) : physics_(physics), ai_(ai) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated || !ai_.updated) {
            std::cerr << "RenderSystem updated before dependencies" << std::endl;
            orderError = true;
        }
        updated = true;
    }
    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        return {ecs::SystemDependency::Requires<MockPhysicsSystem>(),
                ecs::SystemDependency::Requires<AISystem>()};
    }
    ecs::UpdatePhase GetUpdatePhase() const override {
        return ecs::UpdatePhase::RenderPrep;
    }
    const char* GetName() const override { return "RenderSystem"; }
    MockPhysicsSystem& physics_;
    AISystem& ai_;
    bool updated = false;
    bool orderError = false;
};

bool TestDependencyOrder() {
    SystemManager manager;
    EntityManager em;

    // Register in dependency order
    MockPhysicsSystem& physics = manager.RegisterSystem<MockPhysicsSystem>();
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

    const auto& metadata = manager.GetRegisteredSystemMetadata();
    bool renderMetadataVerified = false;
    for (const auto& entry : metadata) {
        if (entry.name == "RenderSystem") {
            if (entry.phase != ecs::UpdatePhase::RenderPrep) {
                std::cerr << "RenderSystem should be registered in RenderPrep phase" << std::endl;
                return false;
            }
            if (entry.systemDependencies.size() != 2) {
                std::cerr << "RenderSystem should declare two dependencies" << std::endl;
                return false;
            }
            renderMetadataVerified = true;
            break;
        }
    }

    if (!renderMetadataVerified) {
        std::cerr << "RenderSystem metadata not found" << std::endl;
        return false;
    }

    return true;
}

bool TestComplexGraph() {
    SystemManager manager;
    EntityManager em;

    // Simulate a more complex graph
    MockPhysicsSystem& physics = manager.RegisterSystem<MockPhysicsSystem>();
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