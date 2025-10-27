#ifndef ENGINE_BOOTSTRAP_H
#define ENGINE_BOOTSTRAP_H

#include <string>
#include <vector>

#include "ShipAssembly.h"
#include "ActorContext.h"

class ResourceManager;
class EntityManager;

namespace ecs {
class SystemSchedulerV2;
}

class EngineBootstrap {
public:
    struct Result {
        ShipAssemblyResult hudAssembly;
        ActorContext actorContext;
        std::vector<std::string> actorTypes;
    };

    Result Run(ResourceManager& resourceManager,
               EntityManager& entityManager,
               ecs::SystemSchedulerV2* scheduler) const;
};

#endif // ENGINE_BOOTSTRAP_H
