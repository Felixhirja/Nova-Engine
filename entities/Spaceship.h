#pragma once

#include "../engine/EntityCommon.h"

/**
 * Spaceship: Generic spaceship actor
 * Loads configuration from JSON and integrates with ECS
 */
class Spaceship : public IActor {
public:
    Spaceship() = default;
    virtual ~Spaceship() = default;

    // IActor interface
    void Initialize() override {
        // Add ViewportID component for automatic rendering
        if (auto* em = context_.GetEntityManager()) {
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(context_.GetEntity(), vp);
        }
    }
    
    std::string GetName() const override { return "Spaceship"; }

private:
    // Spaceship-specific properties
    std::string name_ = "Spaceship";
    double speed_ = 100.0;
    double health_ = 800.0;
    double shield_ = 400.0;
    std::string model_ = "generic_ship";
    std::string faction_ = "neutral";
};
