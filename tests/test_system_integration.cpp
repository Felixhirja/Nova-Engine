#include "../engine/ecs/System.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <memory>

// Mock systems using UnifiedSystem with different SystemTypes
class MockPhysicsSystem : public UnifiedSystem {
public:
    MockPhysicsSystem() : UnifiedSystem(SystemType::Physics) {}
    void Update(EntityManager& entityManager, double dt) override {
        // Simulate physics update
        updated = true;
        // Do not call UnifiedSystem::Update to avoid dispatch
    }
    bool updated = false;
};

class AISystem : public UnifiedSystem {
public:
    AISystem(MockPhysicsSystem& physics) : UnifiedSystem(SystemType::BehaviorTree), physics_(physics) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated) {
            std::cerr << "AISystem updated before MockPhysicsSystem" << std::endl;
            orderError = true;
        }
        updated = true;
        // Do not call UnifiedSystem::Update to avoid dispatch
    }
    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        return {ecs::SystemDependency::Requires<MockPhysicsSystem>()};
    }
    MockPhysicsSystem& physics_;
    bool updated = false;
    bool orderError = false;
};

class RenderSystem : public UnifiedSystem {
public:
    RenderSystem(MockPhysicsSystem& physics, AISystem& ai) : UnifiedSystem(SystemType::Animation), physics_(physics), ai_(ai) {}
    void Update(EntityManager& entityManager, double dt) override {
        if (!physics_.updated || !ai_.updated) {
            std::cerr << "RenderSystem updated before dependencies" << std::endl;
            orderError = true;
        }
        updated = true;
        // Do not call UnifiedSystem::Update to avoid dispatch
    }
    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        return {ecs::SystemDependency::Requires<MockPhysicsSystem>(),
                ecs::SystemDependency::Requires<AISystem>()};
    }
    ecs::UpdatePhase GetUpdatePhase() const override {
        return ecs::UpdatePhase::RenderPrep;
    }
    MockPhysicsSystem& physics_;
    AISystem& ai_;
    bool updated = false;
    bool orderError = false;
};

bool TestDependencyOrder() {
    SystemManager manager;
    EntityManager em;

    // Create system instances directly
    MockPhysicsSystem physics;
    AISystem ai(physics);
    RenderSystem render(physics, ai);

    // Register systems using RegisterSystem template method
    MockPhysicsSystem& physicsRef = manager.RegisterSystem<MockPhysicsSystem>();
    AISystem& aiRef = manager.RegisterSystem<AISystem>(physicsRef);
    RenderSystem& renderRef = manager.RegisterSystem<RenderSystem>(physicsRef, aiRef);

    manager.UpdateAll(em, 1.0);

    if (!physicsRef.updated || !aiRef.updated || !renderRef.updated) {
        std::cerr << "All systems should be updated" << std::endl;
        std::cerr << "Physics updated: " << physicsRef.updated << std::endl;
        std::cerr << "AI updated: " << aiRef.updated << std::endl;
        std::cerr << "Render updated: " << renderRef.updated << std::endl;
        return false;
    }

    if (aiRef.orderError || renderRef.orderError) {
        std::cerr << "Dependency order not respected" << std::endl;
        return false;
    }

    const auto& metadata = manager.GetRegisteredSystemMetadata();
    bool renderMetadataVerified = false;
    for (const auto& entry : metadata) {
        if (entry.name == "AnimationSystem") {
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
    class AudioSystem : public UnifiedSystem {
    public:
        AudioSystem() : UnifiedSystem(SystemType::Animation) {}
        void Update(EntityManager& entityManager, double dt) override {
            updated = true;
            // Do not call UnifiedSystem::Update to avoid dispatch
        }
        bool updated = false;
    };

    AudioSystem& audio = manager.RegisterSystem<AudioSystem>();

    manager.UpdateAll(em, 1.0);

    if (!physics.updated || !ai.updated || !render.updated || !audio.updated) {
        std::cerr << "All systems should be updated" << std::endl;
        std::cerr << "Physics updated: " << physics.updated << std::endl;
        std::cerr << "AI updated: " << ai.updated << std::endl;
        std::cerr << "Render updated: " << render.updated << std::endl;
        std::cerr << "Audio updated: " << audio.updated << std::endl;
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