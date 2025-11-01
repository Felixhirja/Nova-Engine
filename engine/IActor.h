#pragma once

#include <string>
#include <memory>

// Forward declarations
class ActorContext;

/**
 * IActor: Base interface for all game actors
 * Actors represent gameplay objects that integrate with the ECS
 */
class IActor {
public:
    IActor() = default;
    virtual ~IActor() = default;

    // Delete copy operations
    IActor(const IActor&) = delete;
    IActor& operator=(const IActor&) = delete;

    // Allow move operations
    IActor(IActor&&) = default;
    IActor& operator=(IActor&&) = default;

    /**
     * Attach this actor to an ECS context
     * Called once after construction to establish ECS integration
     */
    virtual void AttachContext(ActorContext context) {
        context_ = std::move(context);
    }

    /**
     * Initialize the actor with specific configuration
     * Called after AttachContext to set up actor-specific state
     */
    virtual void Initialize() = 0;

    /**
     * Get the actor's ECS context
     */
    const ActorContext& Context() const { return context_; }

    /**
     * Get the actor's name/type for debugging
     */
    virtual std::string GetName() const = 0;

    /**
     * Update the actor (called each frame)
     */
    virtual void Update(double dt) {
        (void)dt; // Default: no-op
    }

protected:
    ActorContext context_;
};

// Actor factory function type
using ActorFactory = std::unique_ptr<IActor>(*)();
