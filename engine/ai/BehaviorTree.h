#pragma once

#include "DeterministicRandom.h"
#include "ecs/Components.h"
#include "ecs/EntityManager.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ai {

enum class BehaviorStatus {
    Success,
    Failure,
    Running
};

class BehaviorNode {
public:
    virtual ~BehaviorNode() = default;
    virtual BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) = 0;
};

using BehaviorNodePtr = std::shared_ptr<BehaviorNode>;

class SequenceNode : public BehaviorNode {
public:
    void AddChild(BehaviorNodePtr child) { children_.push_back(std::move(child)); }
    BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) override;
private:
    std::vector<BehaviorNodePtr> children_;
};

class SelectorNode : public BehaviorNode {
public:
    void AddChild(BehaviorNodePtr child) { children_.push_back(std::move(child)); }
    BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) override;
private:
    std::vector<BehaviorNodePtr> children_;
};

class ManeuverNode : public BehaviorNode {
public:
    explicit ManeuverNode(std::function<void(NavigationState&)> maneuver);
    BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) override;
private:
    std::function<void(NavigationState&)> maneuver_;
};

class TargetingNode : public BehaviorNode {
public:
    TargetingNode() = default;
    BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random) override;
};

class BehaviorTreeDefinition {
public:
    explicit BehaviorTreeDefinition(BehaviorNodePtr root = nullptr);

    BehaviorStatus Tick(Entity entity, EntityManager& entityManager, DeterministicRandom& random);
    BehaviorNodePtr GetRoot() const { return root_; }

private:
    BehaviorNodePtr root_;
};

class BehaviorTreeLibrary {
public:
    void RegisterTree(const std::string& id, BehaviorNodePtr root);
    BehaviorNodePtr GetTree(const std::string& id) const;
    bool HasTree(const std::string& id) const;

private:
    std::unordered_map<std::string, BehaviorNodePtr> trees_;
};

} // namespace ai

