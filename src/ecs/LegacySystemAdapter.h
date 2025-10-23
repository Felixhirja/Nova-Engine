#pragma once

#include "EntityManager.h"
#include "System.h"
#include "SystemSchedulerV2.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace ecs {

template<typename LegacySystem>
class LegacySystemAdapter : public SystemV2 {
public:
    explicit LegacySystemAdapter(EntityManager& facade)
        : legacySystem_(std::make_unique<LegacySystem>()),
          facade_(facade),
          name_(typeid(LegacySystem).name()) {}

    void Update(EntityManagerV2& entityManager, double dt) override {
        (void)entityManager;
        if (!facade_.UsingArchetypeStorage()) {
            throw std::runtime_error("LegacySystemAdapter requires archetype facade to be enabled");
        }
        legacySystem_->Update(facade_, dt);
    }

    const char* GetName() const override { return name_.c_str(); }

private:
    std::unique_ptr<LegacySystem> legacySystem_;
    EntityManager& facade_;
    std::string name_;
};

} // namespace ecs

