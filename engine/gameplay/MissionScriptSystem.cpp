#include "MissionScriptSystem.h"

#include "ecs/Components.h"
#include "ecs/EntityManager.h"

#include <unordered_map>

namespace {
MissionObjective::State EvaluateObjective(const MissionObjective& objective,
                                          const std::unordered_map<std::string, ScriptedTrigger*>& triggers) {
    if (objective.state == MissionObjective::State::Completed ||
        objective.state == MissionObjective::State::Failed) {
        return objective.state;
    }

    bool success = true;
    for (const auto& trigger : objective.successConditions) {
        auto it = triggers.find(trigger.id);
        if (it == triggers.end() || (it->second && it->second->active)) {
            success = false;
            break;
        }
    }

    if (success && !objective.successConditions.empty()) {
        return MissionObjective::State::Completed;
    }

    for (const auto& trigger : objective.failureConditions) {
        auto it = triggers.find(trigger.id);
        if (it != triggers.end() && it->second && !it->second->active) {
            return MissionObjective::State::Failed;
        }
    }

    return objective.state;
}
}

void MissionScriptSystem::Update(EntityManager& entityManager, double dt) {
    (void)dt;

    auto triggerComponents = entityManager.GetAllWith<ScriptedTrigger>();
    std::unordered_map<std::string, ScriptedTrigger*> triggerMap;
    for (auto& [entity, trigger] : triggerComponents) {
        if (!trigger) continue;
        triggerMap.emplace(trigger->id, trigger.get());
    }

    auto missionStates = entityManager.GetAllWith<MissionState>();
    auto missionObjectives = entityManager.GetAllWith<MissionObjective>();

    for (auto& [entity, mission] : missionStates) {
        if (!mission) continue;
        if (mission->failed || mission->completed) {
            continue;
        }

        MissionObjective* activeObjective = nullptr;
        for (auto& [objEntity, objective] : missionObjectives) {
            if (!objective) continue;
            if (objective->state == MissionObjective::State::Completed ||
                objective->state == MissionObjective::State::Failed) {
                continue;
            }
            if (mission->objectiveOrder.empty() ||
                mission->objectiveOrder.front() == objective->id) {
                activeObjective = objective.get();
                break;
            }
        }

        if (activeObjective) {
            activeObjective->state = MissionObjective::State::Active;
            mission->objectiveStates[activeObjective->id] = MissionObjective::State::Active;
            MissionObjective::State result = EvaluateObjective(*activeObjective, triggerMap);
            activeObjective->state = result;
            mission->objectiveStates[activeObjective->id] = result;
            if (result == MissionObjective::State::Completed) {
                if (!mission->objectiveOrder.empty()) {
                    mission->objectiveOrder.pop_front();
                }
            } else if (result == MissionObjective::State::Failed) {
                mission->failed = true;
            }
        }

        if (mission->objectiveOrder.empty()) {
            mission->completed = !mission->failed;
        }
    }
}

