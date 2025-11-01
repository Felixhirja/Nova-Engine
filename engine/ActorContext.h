#pragma once

// ActorContext depends on EntityManager being fully defined
// This header MUST be included AFTER "ecs/EntityManager.h"
#ifndef ECS_ENTITY_MANAGER_H
#error "ActorContext.h requires ecs/EntityManager.h to be included first"
#endif

#include <memory>

// Forward declare Entity type from global namespace
using Entity = int;

/**
 * ActorContext: Simple ECS integration for actors
 * Beginner-friendly interface for accessing entity components
 * 
 * REQUIREMENT: Must include "ecs/EntityManager.h" before this header
 */
class ActorContext {
public:
    ActorContext() = default;
    ActorContext(::EntityManager& em, Entity entity)
        : entityManager_(&em), entity_(entity) {}

    // Simple accessors
    ::EntityManager* GetEntityManager() const { return entityManager_; }
    Entity GetEntity() const { return entity_; }

    // Convenience template methods (defined in ActorContextImpl.h)
    template<typename T>
    T* GetComponent() const;

    template<typename T>
    void AddComponent(T component) const;

private:
    ::EntityManager* entityManager_ = nullptr;
    Entity entity_ = 0;
};