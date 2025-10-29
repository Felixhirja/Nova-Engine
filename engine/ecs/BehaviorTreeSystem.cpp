#include "BehaviorTreeSystem.h"

#include "ecs/EntityManager.h"

BehaviorTreeSystem::BehaviorTreeSystem() {
    EnsureDefaultTrees();
}

void BehaviorTreeSystem::EnsureDefaultTrees() {
    if (!library_.HasTree("default")) {
        auto sequence = std::make_shared<ai::SequenceNode>();
        sequence->AddChild(std::make_shared<ai::TargetingNode>());
        sequence->AddChild(std::make_shared<ai::ManeuverNode>([](NavigationState& nav) {
            nav.throttle = 1.0f;
        }));
        library_.RegisterTree("default", sequence);
    }
}

void BehaviorTreeSystem::Update(EntityManager& entityManager, double dt) {
    (void)dt;
    EnsureDefaultTrees();

    DeterministicRandom fallbackRandom;
    if (!random_) {
        fallbackRandom.SetGlobalSeed(0u);
    }

    auto behaviors = entityManager.GetAllWith<BehaviorTreeHandle>();
    for (auto& [entity, handle] : behaviors) {
        if (!handle) {
            continue;
        }

        std::string treeId = handle->treeId.empty() ? std::string("default") : handle->treeId;
        auto tree = library_.GetTree(treeId);
        if (!tree) {
            continue;
        }

        activeTrees_[entity] = tree;

        DeterministicRandom& rng = random_ ? *random_ : fallbackRandom;
        ai::BehaviorTreeDefinition definition(tree);
        definition.Tick(entity, entityManager, rng);
    }

    // Clean up entities that no longer exist
    for (auto it = activeTrees_.begin(); it != activeTrees_.end();) {
        if (!entityManager.IsAlive(it->first)) {
            it = activeTrees_.erase(it);
        } else {
            ++it;
        }
    }
}

