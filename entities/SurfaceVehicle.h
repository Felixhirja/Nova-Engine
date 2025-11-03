#pragma once

#include "../engine/EntityCommon.h"

class SurfaceVehicle : public IActor {
public:
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Basic components
            Position pos(0, 0, 0);
            em->AddComponent<Position>(entity, pos);
            Velocity vel(0, 0, 0);
            em->AddComponent<Velocity>(entity, vel);
            
            // Vehicle-specific
            Nova::SurfaceVehicleComponent vehicle;
            vehicle.type = Nova::SurfaceVehicleComponent::VehicleType::Rover;
            vehicle.fuel = 100.0f;
            vehicle.maxSpeed = 25.0f;
            vehicle.passengerCapacity = 4;
            vehicle.cargoCapacity = 1000.0f;
            em->AddComponent<Nova::SurfaceVehicleComponent>(entity, vehicle);
            
            // Rendering
            DrawComponent draw;
            draw.mode = RenderMode::Mesh3D;
            draw.color = {0.8f, 0.8f, 0.2f, 1.0f};
            draw.scale = {3.0f, 2.0f, 5.0f};
            em->AddComponent<DrawComponent>(entity, draw);
            
            // Physics
            Health health;
            health.current = 100.0;
            health.maximum = 100.0;
            em->AddComponent<Health>(entity, health);
        }
    }
    
    void Update(double deltaTime) override {
        // Vehicle-specific update logic
    }
    
    std::string GetName() const override {
        return "SurfaceVehicle";
    }
};
