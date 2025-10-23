#ifndef ENGINE_BOOTSTRAP_H
#define ENGINE_BOOTSTRAP_H

#include "ShipAssembly.h"

class ResourceManager;
class EntityManager;

class EngineBootstrap {
public:
    struct Result {
        ShipAssemblyResult hudAssembly;
    };

    Result Run(ResourceManager& resourceManager, EntityManager& entityManager) const;
};

#endif // ENGINE_BOOTSTRAP_H
