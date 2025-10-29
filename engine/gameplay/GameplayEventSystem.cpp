#include "GameplayEventSystem.h"

#include "ecs/EntityManager.h"
#include "ecs/Components.h"

#include <algorithm>

namespace {
void ApplyDamage(Entity entity, EntityManager& entityManager, const DamageEvent& damage) {
    if (auto* vitals = entityManager.GetComponent<PlayerVitals>(entity)) {
        vitals->health = std::max(0.0, vitals->health - damage.amount);
    }
}

void ApplyStatusEffect(Entity entity, EntityManager& entityManager, const StatusEffectEvent& event) {
    if (auto* existing = entityManager.GetComponent<StatusEffect>(entity)) {
        if (existing->id == event.effectId) {
            existing->magnitude = event.magnitude;
            existing->duration = event.duration;
            existing->elapsed = 0.0;
            return;
        }
    }
    auto& effect = entityManager.EmplaceComponent<StatusEffect>(entity);
    effect.id = event.effectId;
    effect.magnitude = event.magnitude;
    effect.duration = event.duration;
    effect.elapsed = 0.0;
}
}

void GameplayEventSystem::Update(EntityManager& entityManager, double dt) {
    auto buffers = entityManager.GetAllWith<GameplayEventBuffer>();
    for (auto& [entity, buffer] : buffers) {
        if (!buffer) {
            continue;
        }
        auto events = buffer->ConsumeAll();
        for (const auto& event : events) {
            switch (event.type) {
                case GameplayEvent::Type::Damage:
                    if (const auto* payload = std::get_if<DamageEvent>(&event.payload)) {
                        ApplyDamage(entity, entityManager, *payload);
                    }
                    break;
                case GameplayEvent::Type::StatusEffectApplied:
                    if (const auto* payload = std::get_if<StatusEffectEvent>(&event.payload)) {
                        ApplyStatusEffect(entity, entityManager, *payload);
                    }
                    break;
                case GameplayEvent::Type::TriggerActivated:
                    if (const auto* triggerId = std::get_if<std::string>(&event.payload)) {
                        if (auto* trigger = entityManager.GetComponent<ScriptedTrigger>(entity)) {
                            if (trigger->id == *triggerId) {
                                trigger->active = !trigger->oneShot;
                            }
                        }
                    }
                    break;
            }
        }
    }

    auto effects = entityManager.GetAllWith<StatusEffect>();
    for (auto& [entity, effect] : effects) {
        if (!effect) continue;
        effect->elapsed += dt;
        if (effect->duration > 0.0 && effect->elapsed >= effect->duration) {
            entityManager.RemoveComponent<StatusEffect>(entity);
        }
    }
}

