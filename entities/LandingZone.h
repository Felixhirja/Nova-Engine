#pragma once

#include "../engine/EntityCommon.h"

class LandingZone : public IActor {
public:
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position and marker
            Position pos(0, 0, 0);
            em->AddComponent<Position>(entity, pos);
            
            // Landing zone configuration
            Nova::LandingZoneComponent zone;
            zone.type = Nova::LandingZoneComponent::ZoneType::Spaceport;
            zone.radius = 100.0f;
            zone.cleared = true;
            zone.maxShipSize = 4;
            zone.hasBeacon = true;
            zone.controlled = true;
            em->AddComponent<Nova::LandingZoneComponent>(entity, zone);
            
            // Visual marker
            DrawComponent draw;
            draw.mode = RenderMode::Billboard;
            draw.color = {0.0f, 1.0f, 0.0f, 0.7f};
            draw.scale = {zone.radius, zone.radius, 1.0f};
            em->AddComponent<DrawComponent>(entity, draw);
        }
    }
    
    void Update(double deltaTime) override {
        // Landing zone management logic
    }
    
    std::string GetName() const override {
        return "LandingZone";
    }
};
