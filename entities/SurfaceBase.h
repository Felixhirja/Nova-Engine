#pragma once

#include "../engine/EntityCommon.h"

class SurfaceBase : public IActor {
public:
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position
            Position pos(0, 0, 0);
            em->AddComponent<Position>(entity, pos);
            
            // Base configuration
            Nova::SurfaceBaseComponent base;
            base.type = Nova::SurfaceBaseComponent::BaseType::Outpost;
            base.name = "Frontier Outpost Alpha";
            base.integrity = 100.0f;
            base.population = 50;
            base.powered = true;
            base.lifeSupportOnline = true;
            base.hasRefueling = true;
            base.hasRepair = true;
            base.hasMedical = true;
            base.hasMarket = true;
            em->AddComponent<Nova::SurfaceBaseComponent>(entity, base);
            
            // Visual
            DrawComponent draw;
            draw.mode = RenderMode::Mesh3D;
            draw.color = {0.7f, 0.7f, 0.8f, 1.0f};
            draw.scale = {20.0f, 10.0f, 20.0f};
            em->AddComponent<DrawComponent>(entity, draw);
            
            // Health
            Health health;
            health.current = 1000.0;
            health.maximum = 1000.0;
            em->AddComponent<Health>(entity, health);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            auto* base = em->GetComponent<Nova::SurfaceBaseComponent>(entity);
            auto* health = em->GetComponent<Health>(entity);
            
            if (base && health) {
                // Update base integrity based on health
                base->integrity = (health->current / health->maximum) * 100.0f;
                
                // Critical systems offline if integrity too low
                if (base->integrity < 30.0f) {
                    base->lifeSupportOnline = false;
                    base->powered = false;
                }
            }
        }
    }
    
    std::string GetName() const override {
        return "SurfaceBase";
    }
};
