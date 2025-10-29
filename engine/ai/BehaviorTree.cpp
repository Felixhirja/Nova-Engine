#include "BehaviorTree.h"

namespace ai {

BehaviorStatus SequenceNode::Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) {
    BehaviorStatus last = BehaviorStatus::Success;
    for (auto& child : children_) {
        if (!child) continue;
        BehaviorStatus status = child->Tick(entity, entityManager, random);
        if (status == BehaviorStatus::Failure) {
            return BehaviorStatus::Failure;
        }
        if (status == BehaviorStatus::Running) {
            last = BehaviorStatus::Running;
        }
    }
    return last;
}

BehaviorStatus SelectorNode::Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) {
    for (auto& child : children_) {
        if (!child) continue;
        BehaviorStatus status = child->Tick(entity, entityManager, random);
        if (status == BehaviorStatus::Success || status == BehaviorStatus::Running) {
            return status;
        }
    }
    return BehaviorStatus::Failure;
}

ManeuverNode::ManeuverNode(std::function<void(NavigationState&)> maneuver)
    : maneuver_(std::move(maneuver)) {}

BehaviorStatus ManeuverNode::Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) {
    auto* nav = entityManager.GetComponent<NavigationState>(entity);
    if (!nav) {
        return BehaviorStatus::Failure;
    }
    if (!nav->hasTarget) {
        return BehaviorStatus::Failure;
    }
    if (maneuver_) {
        maneuver_(*nav);
    }
    return BehaviorStatus::Success;
}

BehaviorStatus TargetingNode::Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) {
    auto* aiState = entityManager.GetComponent<AIBehavior>(entity);
    if (!aiState) {
        return BehaviorStatus::Failure;
    }

    if (!aiState->targetEntity.IsValid() || !entityManager.IsAlive(static_cast<Entity>(aiState->targetEntity.Index()))) {
        auto candidates = entityManager.GetAllWith<AIBehavior>();
        if (candidates.size() <= 1) {
            aiState->targetEntity = ecs::EntityHandle::Null();
            return BehaviorStatus::Failure;
        }
        size_t index = static_cast<size_t>(random.NextInt(0, static_cast<int>(candidates.size() - 1)));
        auto chosen = candidates[index];
        if (chosen.first == entity) {
            index = (index + 1) % candidates.size();
            chosen = candidates[index];
        }
        aiState->targetEntity = ecs::EntityHandle(chosen.first);
        aiState->decisionTimer = 0.0f;
    }
    return BehaviorStatus::Success;
}

BehaviorTreeDefinition::BehaviorTreeDefinition(BehaviorNodePtr root)
    : root_(std::move(root)) {}

BehaviorStatus BehaviorTreeDefinition::Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) {
    if (!root_) {
        return BehaviorStatus::Failure;
    }
    return root_->Tick(entity, entityManager, random);
}

void BehaviorTreeLibrary::RegisterTree(const std::string& id, BehaviorNodePtr root) {
    trees_[id] = std::move(root);
}

BehaviorNodePtr BehaviorTreeLibrary::GetTree(const std::string& id) const {
    auto it = trees_.find(id);
    if (it == trees_.end()) {
        return nullptr;
    }
    return it->second;
}

bool BehaviorTreeLibrary::HasTree(const std::string& id) const {
    return trees_.count(id) > 0;
}

} // namespace ai

