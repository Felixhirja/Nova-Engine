#ifndef ENGINE_BOOTSTRAP_H
#define ENGINE_BOOTSTRAP_H

#include "ecs/ShipAssembly.h"
#include "ActorContext.h"
#include "BootstrapConfiguration.h"

#include <string>
#include <vector>

class ResourceManager;
class EntityManager;

namespace ecs {
class SystemSchedulerV2;
}

class EngineBootstrap {
public:
    struct InitializationStep {
        std::string name;
        std::string description;
    };

    struct SubsystemStatus {
        std::string name;
        std::string description;
        bool required = true;
        bool enabled = true;
        bool ready = false;
    };

    struct Result {
        ShipAssemblyResult hudAssembly;
        ActorContext actorContext;
        std::vector<std::string> actorTypes;
        BootstrapConfiguration configuration;
        std::vector<SubsystemStatus> subsystemChecklist;
        std::vector<std::string> warnings;
    };

    static const std::vector<InitializationStep>& InitializationSequence();
    static std::vector<SubsystemStatus> BuildSubsystemChecklist(const BootstrapConfiguration& config);

    Result Run(ResourceManager& resourceManager,
               EntityManager& entityManager,
               ecs::SystemSchedulerV2* scheduler) const;

    void Shutdown(ResourceManager& resourceManager,
                  EntityManager& entityManager,
                  ecs::SystemSchedulerV2* scheduler) const;
};

#endif // ENGINE_BOOTSTRAP_H
