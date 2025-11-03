#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/MiningComponents.h"

class MiningVessel : public IActor {
private:
    Nova::MiningVesselComponent::VesselClass vesselClass_;
    
public:
    MiningVessel(Nova::MiningVesselComponent::VesselClass vesselClass = 
                 Nova::MiningVesselComponent::VesselClass::SoloMiner)
        : vesselClass_(vesselClass) {}
    
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Core components
            em->AddComponent<Position>(entity, {0, 0, 0});
            em->AddComponent<Velocity>(entity, {0, 0, 0});
            
            // Mining vessel designation
            Nova::MiningVesselComponent vessel;
            vessel.vesselClass = vesselClass_;
            
            // Configure based on class
            switch (vesselClass_) {
                case Nova::MiningVesselComponent::VesselClass::SoloMiner:
                    vessel.crewCapacity = 1;
                    vessel.currentCrew = 1;
                    vessel.laserDrillSlots = 1;
                    vessel.cargoHolds = 1;
                    vessel.fuelConsumption = 1.0f;
                    vessel.maintenanceCost = 50.0f;
                    break;
                    
                case Nova::MiningVesselComponent::VesselClass::IndustrialMiner:
                    vessel.crewCapacity = 3;
                    vessel.currentCrew = 2;
                    vessel.laserDrillSlots = 2;
                    vessel.extractorSlots = 1;
                    vessel.cargoHolds = 2;
                    vessel.fuelConsumption = 2.5f;
                    vessel.maintenanceCost = 150.0f;
                    break;
                    
                case Nova::MiningVesselComponent::VesselClass::MiningBarge:
                    vessel.crewCapacity = 8;
                    vessel.currentCrew = 6;
                    vessel.laserDrillSlots = 4;
                    vessel.extractorSlots = 2;
                    vessel.refinerySlots = 1;
                    vessel.cargoHolds = 4;
                    vessel.fuelConsumption = 5.0f;
                    vessel.maintenanceCost = 400.0f;
                    break;
                    
                case Nova::MiningVesselComponent::VesselClass::Mothership:
                    vessel.crewCapacity = 20;
                    vessel.currentCrew = 15;
                    vessel.laserDrillSlots = 6;
                    vessel.extractorSlots = 4;
                    vessel.refinerySlots = 2;
                    vessel.cargoHolds = 8;
                    vessel.fuelConsumption = 10.0f;
                    vessel.maintenanceCost = 1000.0f;
                    break;
            }
            
            vessel.certified = true;
            em->AddComponent<Nova::MiningVesselComponent>(entity, vessel);
            
            // Cargo hold
            Nova::ResourceCargoComponent cargo;
            cargo.capacity = 1000.0f * vessel.cargoHolds;
            cargo.currentMass = 0.0f;
            cargo.autoSort = true;
            cargo.compressed = false;
            cargo.compressionRatio = 1.0f;
            cargo.transferRate = 10.0f;
            cargo.transferring = false;
            em->AddComponent<Nova::ResourceCargoComponent>(entity, cargo);
            
            // Laser drill (if has slots)
            if (vessel.laserDrillSlots > 0) {
                Nova::LaserDrillComponent drill;
                drill.power = 100.0f;
                drill.maxPower = 100.0f;
                drill.powerConsumption = 15.0f;
                drill.miningRate = 12.0f * vessel.laserDrillSlots;
                drill.efficiency = 1.0f;
                drill.range = 50.0f;
                drill.drillLevel = 1;
                em->AddComponent<Nova::LaserDrillComponent>(entity, drill);
                
                // Tool durability
                Nova::ToolDurabilityComponent durability;
                durability.condition = 100.0f;
                durability.degradationRate = 0.01f;
                durability.repairCost = 500.0f;
                em->AddComponent<Nova::ToolDurabilityComponent>(entity, durability);
            }
            
            // Prospector
            Nova::ProspectorComponent prospector;
            prospector.scanRange = 500.0f;
            prospector.scanResolution = 0.6f;
            prospector.energyConsumption = 5.0f;
            prospector.scanTime = 10.0f;
            prospector.canAnalyzeComposition = true;
            em->AddComponent<Nova::ProspectorComponent>(entity, prospector);
            
            // Refinery (if has slots)
            if (vessel.refinerySlots > 0) {
                Nova::RefineryComponent refinery;
                refinery.type = Nova::RefineryComponent::RefineryType::BasicSmelter;
                refinery.processingRate = 5.0f;
                refinery.efficiency = 0.75f;
                refinery.inputStorageMax = 5000.0f;
                refinery.outputStorageMax = 4000.0f;
                em->AddComponent<Nova::RefineryComponent>(entity, refinery);
            }
            
            // Mining statistics
            em->AddComponent<Nova::MiningStatsComponent>(entity, {});
            
            // Health
            Health health;
            health.max = 100.0f + (static_cast<int>(vesselClass_) * 50.0f);
            health.current = health.max;
            em->AddComponent<Health>(entity, health);
            
            // Visual representation
            DrawComponent draw;
            draw.mode = DrawComponent::RenderMode::Mesh3D;
            draw.SetTint(0.7f, 0.7f, 0.2f); // Yellowish for mining vessel
            
            // Size based on class
            float size = 5.0f + static_cast<float>(static_cast<int>(vesselClass_)) * 5.0f;
            draw.SetScale(size, size * 0.6f, size);
            em->AddComponent<DrawComponent>(entity, draw);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Update mining stats display color based on cargo
            auto* cargo = em->GetComponent<Nova::ResourceCargoComponent>(entity);
            auto* draw = em->GetComponent<DrawComponent>(entity);
            
            if (cargo && draw) {
                float fillPercent = cargo->currentMass / cargo->capacity;
                
                // Shift color as cargo fills (yellow to orange)
                draw->tintR = 0.7f + fillPercent * 0.3f;
                draw->tintG = 0.7f - fillPercent * 0.3f;
                
                // Flash if nearly full
                if (fillPercent > 0.95f) {
                    float flash = static_cast<float>(std::sin(context_.GetTime() * 5.0));
                    draw->tintB = 0.2f + flash * 0.3f;
                }
            }
            
            // Check if tools need maintenance
            if (auto* durability = em->GetComponent<Nova::ToolDurabilityComponent>(entity)) {
                if (durability->condition < 20.0f && !durability->broken) {
                    // Could trigger a warning event here
                }
                
                if (durability->broken) {
                    // Disable drill
                    if (auto* drill = em->GetComponent<Nova::LaserDrillComponent>(entity)) {
                        drill->active = false;
                    }
                }
            }
        }
    }
    
    std::string GetName() const override {
        switch (vesselClass_) {
            case Nova::MiningVesselComponent::VesselClass::SoloMiner:
                return "Solo Mining Vessel";
            case Nova::MiningVesselComponent::VesselClass::IndustrialMiner:
                return "Industrial Mining Ship";
            case Nova::MiningVesselComponent::VesselClass::MiningBarge:
                return "Mining Barge";
            case Nova::MiningVesselComponent::VesselClass::Mothership:
                return "Mining Mothership";
            default:
                return "Mining Vessel";
        }
    }
};
