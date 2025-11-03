#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/MiningComponents.h"

class MiningDrone : public IActor {
private:
    int mothershipID_;
    Nova::ResourceType targetResource_;
    
public:
    MiningDrone(int mothershipID = -1, 
                Nova::ResourceType targetResource = Nova::ResourceType::IronOre)
        : mothershipID_(mothershipID), targetResource_(targetResource) {}
    
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position
            em->AddComponent<Position>(entity, {0, 0, 0});
            em->AddComponent<Velocity>(entity, {0, 0, 0});
            
            // Drone component
            Nova::MiningDroneComponent drone;
            drone.mode = Nova::MiningDroneComponent::DroneMode::Idle;
            drone.mothershipID = mothershipID_;
            drone.autonomy = 3600.0f; // 1 hour operation
            drone.remainingPower = drone.autonomy;
            drone.miningRate = 3.0f; // Slower than manned ships
            drone.cargoCapacity = 200.0f;
            drone.currentCargo = 0.0f;
            drone.targetResource = targetResource_;
            drone.searchRadius = 1000.0f;
            drone.returnWhenFull = true;
            drone.avoidHazards = true;
            drone.riskTolerance = 0.3f;
            
            em->AddComponent<Nova::MiningDroneComponent>(entity, drone);
            
            // Small cargo hold
            Nova::ResourceCargoComponent cargo;
            cargo.capacity = 200.0f;
            cargo.currentMass = 0.0f;
            em->AddComponent<Nova::ResourceCargoComponent>(entity, cargo);
            
            // Basic extractor
            Nova::ExtractorComponent extractor;
            extractor.type = Nova::ExtractorComponent::ExtractorType::BasicDrill;
            extractor.durability = 100.0f;
            extractor.wearRate = 0.05f;
            extractor.miningRate = 3.0f;
            extractor.powerConsumption = 5.0f;
            extractor.range = 5.0f;
            em->AddComponent<Nova::ExtractorComponent>(entity, extractor);
            
            // Health
            Health health;
            health.max = 50.0f;
            health.current = health.max;
            em->AddComponent<Health>(entity, health);
            
            // Visual
            DrawComponent draw;
            draw.mode = DrawComponent::RenderMode::Mesh3D;
            draw.SetTint(0.5f, 0.8f, 0.9f); // Light blue for drone
            draw.SetScale(2.0f, 1.0f, 2.0f); // Small
            em->AddComponent<DrawComponent>(entity, draw);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            auto* drone = em->GetComponent<Nova::MiningDroneComponent>(entity);
            auto* draw = em->GetComponent<DrawComponent>(entity);
            auto* extractor = em->GetComponent<Nova::ExtractorComponent>(entity);
            
            if (drone && draw) {
                // Color indicates mode
                switch (drone->mode) {
                    case Nova::MiningDroneComponent::DroneMode::Idle:
                        draw->SetTint(0.5f, 0.5f, 0.5f); // Gray
                        break;
                    case Nova::MiningDroneComponent::DroneMode::Prospecting:
                        draw->SetTint(0.5f, 0.8f, 0.5f); // Green
                        break;
                    case Nova::MiningDroneComponent::DroneMode::Mining:
                        draw->SetTint(0.9f, 0.9f, 0.3f); // Yellow (working)
                        if (extractor) extractor->active = true;
                        break;
                    case Nova::MiningDroneComponent::DroneMode::Returning:
                        draw->SetTint(0.3f, 0.5f, 0.9f); // Blue
                        if (extractor) extractor->active = false;
                        break;
                    case Nova::MiningDroneComponent::DroneMode::Recharging:
                        draw->SetTint(0.9f, 0.5f, 0.3f); // Orange
                        if (extractor) extractor->active = false;
                        break;
                }
                
                // Flash if low power
                if (drone->remainingPower < drone->autonomy * 0.15f) {
                    float flash = static_cast<float>(std::sin(context_.GetTime() * 8.0));
                    draw->tintR = std::min(1.0f, draw->tintR + flash * 0.3f);
                }
            }
        }
    }
    
    std::string GetName() const override {
        return "Mining Drone";
    }
};
