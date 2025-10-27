#pragma once

#include <string>

#include "ecs/EntityManagerV2.h"
#include "ecs/EntityHandle.h"

namespace ecs {
class SystemSchedulerV2;
}

struct ActorContext {
    ecs::EntityManagerV2* entityManager = nullptr;
    ecs::SystemSchedulerV2* scheduler = nullptr;
    ecs::EntityHandle entity = ecs::EntityHandle::Null();
    std::string debugName;

    bool HasEntity() const {
        return entityManager && entity.IsValid() && entityManager->IsAlive(entity);
    }

    bool HasScheduler() const {
        return scheduler != nullptr;
    }

    void ResetEntity() {
        entity = ecs::EntityHandle::Null();
    }

    template<typename T>
    T* GetComponent() const {
        if (!entityManager || !entity.IsValid()) {
            return nullptr;
        }
        if (!entityManager->IsAlive(entity)) {
            return nullptr;
        }
        return entityManager->GetComponent<T>(entity);
    }

    ActorContext WithEntity(ecs::EntityHandle handle) const {
        ActorContext copy = *this;
        copy.entity = handle;
        return copy;
    }
};
