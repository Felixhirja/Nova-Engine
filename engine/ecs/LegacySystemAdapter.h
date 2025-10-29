#pragma once

#include "EntityManager.h"
#include "EntityManagerFacade.h"
#include "System.h"
#include "SystemSchedulerV2.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

namespace ecs {

struct LegacySystemAdapterConfig {
    UpdatePhase phase = UpdatePhase::Simulation;
    std::vector<ComponentDependency> componentDependencies;
    std::vector<SystemDependency> systemDependencies;
};

template<typename LegacySystem>
class LegacySystemAdapter : public SystemV2 {
public:
    LegacySystemAdapter(EntityManager& facade, LegacySystemAdapterConfig config = {})
        : legacySystem_(std::make_unique<LegacySystem>()),
          facade_(facade),
          name_(typeid(LegacySystem).name()),
          config_(std::move(config)) {}

    void Update(EntityManagerV2& entityManager, double dt) override {
        (void)entityManager;
        if (!facade_.UsingArchetypeStorage()) {
            throw std::runtime_error("LegacySystemAdapter requires archetype facade to be enabled");
        }
        DispatchUpdate(dt);
    }

    std::vector<ComponentDependency> GetDependencies() const override {
        return config_.componentDependencies;
    }

    std::vector<SystemDependency> GetSystemDependencies() const override {
        return config_.systemDependencies;
    }

    UpdatePhase GetUpdatePhase() const override { return config_.phase; }

    const char* GetName() const override { return name_.c_str(); }

private:
    template<typename SystemType, typename = void>
    struct SupportsLegacyUpdate : std::false_type {};

    template<typename SystemType>
    struct SupportsLegacyUpdate<SystemType,
                                std::void_t<decltype(std::declval<SystemType&>().Update(std::declval<::EntityManager&>(),
                                                                                         std::declval<double>()))>>
        : std::true_type {};

    template<typename SystemType, typename = void>
    struct SupportsFacadeUpdate : std::false_type {};

    template<typename SystemType>
    struct SupportsFacadeUpdate<SystemType,
                                std::void_t<decltype(std::declval<SystemType&>().Update(std::declval<EntityManagerFacade&>(),
                                                                                         std::declval<double>()))>>
        : std::true_type {};

    void DispatchUpdate(double dt) {
        if constexpr (SupportsFacadeUpdate<LegacySystem>::value) {
            EntityManagerFacade facadeAdapter(facade_);
            legacySystem_->Update(facadeAdapter, dt);
        } else {
            static_assert(SupportsLegacyUpdate<LegacySystem>::value, "LegacySystemAdapter requires Update(EntityManager&, double) or Update(EntityManagerFacade&, double)");
            legacySystem_->Update(facade_, dt);
        }
    }

    std::unique_ptr<LegacySystem> legacySystem_;
    EntityManager& facade_;
    std::string name_;
    LegacySystemAdapterConfig config_;
};

} // namespace ecs

