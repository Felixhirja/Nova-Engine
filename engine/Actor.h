#pragma once

#include <string_view>

#include "ActorContext.h"

class IActor {
public:
    virtual ~IActor() = default;

    virtual std::string_view TypeName() const = 0;
    virtual bool IsBound() const = 0;
    virtual ecs::EntityHandle GetEntity() const = 0;
    virtual void AttachContext(const ActorContext& context) = 0;
    virtual const ActorContext& Context() const = 0;
};
