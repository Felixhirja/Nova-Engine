#pragma once

#include "ActorContext.h"
#include "ecs/EntityManager.h"

// Template method implementations for ActorContext
// Simple, beginner-friendly component access

template<typename T>
T* ActorContext::GetComponent() const {
    if (!entityManager_) return nullptr;
    return entityManager_->GetComponent<T>(entity_);
}

template<typename T>
void ActorContext::AddComponent(T component) const {
    if (!entityManager_) return;
    entityManager_->AddComponent<T>(entity_, component);
}
