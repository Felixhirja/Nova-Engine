#pragma once

#include "System.h"
#include "ai/BehaviorTree.h"
#include "DeterministicRandom.h"

#include <unordered_map>

class BehaviorTreeSystem : public System {
public:
    BehaviorTreeSystem();

    void Update(EntityManager& entityManager, double dt) override;
    const char* GetName() const override { return "BehaviorTreeSystem"; }

    void SetRandomManager(DeterministicRandom* random) { random_ = random; }

private:
    ai::BehaviorTreeLibrary library_;
    DeterministicRandom* random_ = nullptr;
    std::unordered_map<unsigned int, ai::BehaviorNodePtr> activeTrees_;

    void EnsureDefaultTrees();
};

