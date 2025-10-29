#pragma once

#include "Archetype.h"

#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ecs {

class TransitionPlan {
public:
    TransitionPlan(Archetype* destination, const Archetype* source);

    void QueueEntity(size_t sourceIndex);
    void Execute();

private:
    struct CopyRange {
        size_t srcStart;
        size_t count;
    };

    struct OperationSet {
        bool trivial = false;
        std::vector<CopyRange> ranges;
    };

    Archetype* destination_ = nullptr;
    const Archetype* source_ = nullptr;
    std::vector<std::type_index> orderedTypes_;
    std::unordered_map<std::type_index, OperationSet> operations_;
};

} // namespace ecs

