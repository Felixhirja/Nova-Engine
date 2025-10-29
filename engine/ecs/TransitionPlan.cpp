#include "TransitionPlan.h"

#include "ComponentTraits.h"

#include <cassert>

namespace ecs {

TransitionPlan::TransitionPlan(Archetype* destination, const Archetype* source)
    : destination_(destination), source_(source) {
    assert(destination_ != nullptr && source_ != nullptr);

    const auto& fromSignature = source_->GetSignature();
    const auto& toSignature = destination_->GetSignature();

    for (const auto& typeIndex : fromSignature.types) {
        if (!toSignature.Contains(typeIndex)) {
            continue;
        }

        orderedTypes_.push_back(typeIndex);

        ComponentArray* dstArray = destination_->GetComponentArrayRaw(typeIndex);
        const ComponentArray* srcArray = source_->GetComponentArrayRaw(typeIndex);
        assert(dstArray != nullptr && srcArray != nullptr);

        OperationSet opSet;
        opSet.trivial = dstArray->IsTriviallyCopyable() && srcArray->IsTriviallyCopyable();
        operations_.emplace(typeIndex, opSet);
    }
}

void TransitionPlan::QueueEntity(size_t sourceIndex) {
    for (const auto& typeIndex : orderedTypes_) {
        auto& opSet = operations_.at(typeIndex);

        if (!opSet.ranges.empty() && opSet.trivial) {
            CopyRange& lastRange = opSet.ranges.back();
            if (lastRange.srcStart + lastRange.count == sourceIndex) {
                lastRange.count += 1;
                continue;
            }
        }

        opSet.ranges.push_back(CopyRange{sourceIndex, 1});
    }
}

void TransitionPlan::Execute() {
    for (const auto& typeIndex : orderedTypes_) {
        auto& opSet = operations_.at(typeIndex);

        for (const CopyRange& range : opSet.ranges) {
            if (opSet.trivial) {
                destination_->CopyComponentBlockFrom(source_, range.srcStart, range.count, typeIndex, true);
            } else {
                destination_->CopyComponentBlockFrom(source_, range.srcStart, range.count, typeIndex, false);
            }
        }
    }

    // Sanity check integrity after executing the plan
    assert(destination_->ValidateIntegrity() && "Destination archetype out of sync after transition");
}

} // namespace ecs

