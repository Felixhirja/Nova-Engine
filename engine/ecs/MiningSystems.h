#pragma once

#include "MiningComponents.h"
#include "EntityManager.h"
#include "Components.h"
#include "PlanetaryComponents.h"
#include <glm/glm.hpp>
#include <random>
#include <algorithm>

namespace Nova {

class MiningSystem {
private:
    std::mt19937 rng_;
    
public:
    MiningSystem() : rng_(std::random_device{}()) {}
    
    // Update laser drilling operations
    void UpdateLaserDrills(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<LaserDrillComponent>();
        for (auto entity : entities) {
            auto* drill = em->GetComponent<LaserDrillComponent>(entity);
            if (!drill) continue;
            
            // Heat management
            if (drill->active) {
                drill->currentHeat += drill->heatGeneration * deltaTime;
                drill->power -= drill->powerConsumption * deltaTime;
                
                if (drill->currentHeat >= drill->maxHeat) {
                    drill->overheated = true;
                    drill->active = false;
                }
                
                if (drill->power <= 0.0f) {
                    drill->active = false;
                    drill->power = 0.0f;
                }
            } else {
                // Passive cooling
                drill->currentHeat = std::max(0.0f, 
                    drill->currentHeat - drill->coolingRate * static_cast<float>(deltaTime));
                
                if (drill->currentHeat < drill->maxHeat * 0.5f) {
                    drill->overheated = false;
                }
            }
            
            // Mining operation
            if (drill->active && drill->targetEntityID >= 0) {
                PerformLaserMining(em, entity, drill, deltaTime);
            }
        }
    }
    
    // Update mechanical extractors
    void UpdateExtractors(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<ExtractorComponent>();
        for (auto entity : entities) {
            auto* extractor = em->GetComponent<ExtractorComponent>(entity);
            if (!extractor || !extractor->active) continue;
            
            if (extractor->targetEntityID >= 0) {
                PerformMechanicalExtraction(em, entity, extractor, deltaTime);
            }
        }
    }
    
    // Update prospecting scans
    void UpdateProspectors(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<ProspectorComponent>();
        for (auto entity : entities) {
            auto* prospector = em->GetComponent<ProspectorComponent>(entity);
            if (!prospector || !prospector->scanning) continue;
            
            auto* position = em->GetComponent<Position>(entity);
            if (!position) continue;
            
            prospector->scanProgress += static_cast<float>(deltaTime) / prospector->scanTime;
            
            if (prospector->scanProgress >= 1.0f) {
                // Scan complete - detect nearby deposits
                ScanForDeposits(em, entity, prospector, position);
                prospector->scanning = false;
                prospector->scanProgress = 0.0f;
            }
        }
    }
    
    // Update refineries
    void UpdateRefineries(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<RefineryComponent>();
        for (auto entity : entities) {
            auto* refinery = em->GetComponent<RefineryComponent>(entity);
            if (!refinery || !refinery->active || refinery->inputAmount <= 0.0f) continue;
            
            float processAmount = refinery->processingRate * static_cast<float>(deltaTime);
            processAmount = std::min(processAmount, refinery->inputAmount);
            
            refinery->inputAmount -= processAmount;
            refinery->outputAmount += processAmount * refinery->efficiency;
            
            // Update progress
            refinery->processingProgress = 1.0f - (refinery->inputAmount / 
                (refinery->inputAmount + refinery->outputAmount));
            
            if (refinery->inputAmount <= 0.0f) {
                refinery->active = false;
                refinery->processingProgress = 1.0f;
            }
        }
    }
    
    // Update mining drones
    void UpdateMiningDrones(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<MiningDroneComponent>();
        for (auto entity : entities) {
            auto* drone = em->GetComponent<MiningDroneComponent>(entity);
            if (!drone) continue;
            
            drone->remainingPower -= static_cast<float>(deltaTime);
            
            switch (drone->mode) {
                case MiningDroneComponent::DroneMode::Prospecting:
                    UpdateDroneProspecting(em, entity, drone, deltaTime);
                    break;
                    
                case MiningDroneComponent::DroneMode::Mining:
                    UpdateDroneMining(em, entity, drone, deltaTime);
                    break;
                    
                case MiningDroneComponent::DroneMode::Returning:
                    UpdateDroneReturning(em, entity, drone, deltaTime);
                    break;
                    
                case MiningDroneComponent::DroneMode::Recharging:
                    if (drone->remainingPower >= drone->autonomy) {
                        drone->mode = MiningDroneComponent::DroneMode::Idle;
                    }
                    break;
                    
                default:
                    break;
            }
            
            // Auto return on low power or full cargo
            if (drone->remainingPower <= drone->autonomy * 0.2f || 
                drone->currentCargo >= drone->cargoCapacity) {
                if (drone->mode != MiningDroneComponent::DroneMode::Returning) {
                    drone->mode = MiningDroneComponent::DroneMode::Returning;
                }
            }
        }
    }
    
    // Update environmental hazards
    void UpdateMiningHazards(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto hazards = em->GetEntitiesWithComponent<MiningHazardComponent>();
        for (auto hazardEntity : hazards) {
            auto* hazard = em->GetComponent<MiningHazardComponent>(hazardEntity);
            if (!hazard) continue;
            
            // Update cycle time for intermittent hazards
            if (hazard->intermittent) {
                hazard->currentCycleTime += static_cast<float>(deltaTime);
                if (hazard->currentCycleTime >= hazard->cycleTime) {
                    hazard->active = !hazard->active;
                    hazard->currentCycleTime = 0.0f;
                }
            }
            
            if (!hazard->active) continue;
            
            auto* hazardPos = em->GetComponent<Position>(hazardEntity);
            if (!hazardPos) continue;
            
            // Apply damage to nearby entities
            ApplyHazardDamage(em, hazard, hazardPos, deltaTime);
        }
    }
    
    // Update mining claims
    void UpdateMiningClaims(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<MiningClaimComponent>();
        for (auto entity : entities) {
            auto* claim = em->GetComponent<MiningClaimComponent>(entity);
            if (!claim) continue;
            
            claim->timeRemaining -= static_cast<float>(deltaTime);
            
            if (claim->timeRemaining <= 0.0f && !claim->contested) {
                // Claim expired - could remove component or transfer ownership
                claim->claimantID = "";
                claim->registered = false;
            }
        }
    }
    
    // Update tool durability
    void UpdateToolDurability(EntityManager* em, double deltaTime) {
        if (!em) return;
        
        auto entities = em->GetEntitiesWithComponent<ToolDurabilityComponent>();
        for (auto entity : entities) {
            auto* durability = em->GetComponent<ToolDurabilityComponent>(entity);
            if (!durability) continue;
            
            // Check if tool is in use
            bool inUse = false;
            if (auto* drill = em->GetComponent<LaserDrillComponent>(entity)) {
                inUse = drill->active;
            } else if (auto* extractor = em->GetComponent<ExtractorComponent>(entity)) {
                inUse = extractor->active;
            }
            
            if (inUse) {
                durability->condition -= durability->degradationRate * static_cast<float>(deltaTime);
                durability->condition = std::max(0.0f, durability->condition);
                
                if (durability->condition < 50.0f) {
                    durability->needsMaintenance = true;
                }
                
                if (durability->condition <= 0.0f) {
                    durability->broken = true;
                }
                
                // Apply efficiency penalty
                durability->efficiencyPenalty = 1.0f - (durability->condition / 100.0f);
            }
        }
    }
    
    // Generate resource deposit
    Entity CreateResourceDeposit(EntityManager* em, const glm::vec3& position, 
                                 ResourceType primary, float quantity) {
        if (!em) return -1;
        
        Entity deposit = em->CreateEntity();
        
        // Position
        em->AddComponent<Position>(deposit, {position.x, position.y, position.z});
        
        // Resource deposit
        EnhancedResourceDepositComponent res;
        res.primaryResource = primary;
        res.primaryQuantity = quantity;
        res.position = position;
        res.radius = std::sqrt(quantity / 100.0f); // Size based on quantity
        res.discovered = false;
        res.surveyed = false;
        
        // Random secondary resource
        std::uniform_int_distribution<int> typeDist(0, 4);
        res.secondaryResource = static_cast<ResourceType>(typeDist(rng_));
        res.secondaryQuantity = quantity * 0.15f; // 15% secondary
        
        // Random properties
        std::uniform_real_distribution<float> floatDist(0.0f, 1.0f);
        res.density = 0.3f + floatDist(rng_) * 0.7f;
        res.miningDifficulty = floatDist(rng_);
        res.hardness = floatDist(rng_);
        res.temperature = 100.0f + floatDist(rng_) * 300.0f;
        
        em->AddComponent<EnhancedResourceDepositComponent>(deposit, res);
        
        return deposit;
    }
    
    // Create mining vessel
    Entity CreateMiningVessel(EntityManager* em, const glm::vec3& position,
                             MiningVesselComponent::VesselClass vesselClass) {
        if (!em) return -1;
        
        Entity vessel = em->CreateEntity();
        
        em->AddComponent<Position>(vessel, {position.x, position.y, position.z});
        em->AddComponent<Velocity>(vessel, {0, 0, 0});
        
        MiningVesselComponent miningVessel;
        miningVessel.vesselClass = vesselClass;
        
        // Configure based on class
        switch (vesselClass) {
            case MiningVesselComponent::VesselClass::SoloMiner:
                miningVessel.crewCapacity = 1;
                miningVessel.laserDrillSlots = 1;
                miningVessel.cargoHolds = 1;
                break;
            case MiningVesselComponent::VesselClass::IndustrialMiner:
                miningVessel.crewCapacity = 3;
                miningVessel.laserDrillSlots = 2;
                miningVessel.extractorSlots = 1;
                miningVessel.cargoHolds = 2;
                break;
            case MiningVesselComponent::VesselClass::MiningBarge:
                miningVessel.crewCapacity = 8;
                miningVessel.laserDrillSlots = 4;
                miningVessel.extractorSlots = 2;
                miningVessel.refinerySlots = 1;
                miningVessel.cargoHolds = 4;
                break;
            case MiningVesselComponent::VesselClass::Mothership:
                miningVessel.crewCapacity = 20;
                miningVessel.laserDrillSlots = 6;
                miningVessel.extractorSlots = 4;
                miningVessel.refinerySlots = 2;
                miningVessel.cargoHolds = 8;
                break;
        }
        
        em->AddComponent<MiningVesselComponent>(vessel, miningVessel);
        
        // Add cargo hold
        ResourceCargoComponent cargo;
        cargo.capacity = 1000.0f * miningVessel.cargoHolds;
        em->AddComponent<ResourceCargoComponent>(vessel, cargo);
        
        // Add mining stats
        em->AddComponent<MiningStatsComponent>(vessel, {});
        
        return vessel;
    }
    
private:
    void PerformLaserMining(EntityManager* em, Entity minerEntity, 
                           LaserDrillComponent* drill, double deltaTime) {
        auto* target = em->GetComponent<EnhancedResourceDepositComponent>(drill->targetEntityID);
        if (!target || target->primaryQuantity <= 0.0f) {
            drill->active = false;
            drill->targetEntityID = -1;
            return;
        }
        
        // Calculate effective mining rate
        float effectiveRate = drill->miningRate * drill->efficiency;
        effectiveRate *= (1.0f - target->miningDifficulty * 0.5f); // Difficulty penalty
        
        // Apply durability penalty if present
        if (auto* durability = em->GetComponent<ToolDurabilityComponent>(minerEntity)) {
            effectiveRate *= (1.0f - durability->efficiencyPenalty);
        }
        
        float minedAmount = effectiveRate * static_cast<float>(deltaTime);
        minedAmount = std::min(minedAmount, target->primaryQuantity);
        
        target->primaryQuantity -= minedAmount;
        
        // Add to cargo
        if (auto* cargo = em->GetComponent<ResourceCargoComponent>(minerEntity)) {
            if (cargo->currentMass + minedAmount <= cargo->capacity) {
                cargo->resources[target->primaryResource] += minedAmount;
                cargo->currentMass += minedAmount;
                
                // Update stats
                if (auto* stats = em->GetComponent<MiningStatsComponent>(minerEntity)) {
                    stats->sessionMinedMass += minedAmount;
                    stats->totalMinedMass += minedAmount;
                    stats->resourcesMinedByType[target->primaryResource] += minedAmount;
                }
            }
        }
        
        // Chance to mine secondary resource
        std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
        if (chanceDist(rng_) < 0.2f && target->secondaryQuantity > 0.0f) {
            float secondaryAmount = minedAmount * 0.1f;
            secondaryAmount = std::min(secondaryAmount, target->secondaryQuantity);
            target->secondaryQuantity -= secondaryAmount;
            
            if (auto* cargo = em->GetComponent<ResourceCargoComponent>(minerEntity)) {
                if (cargo->currentMass + secondaryAmount <= cargo->capacity) {
                    cargo->resources[target->secondaryResource] += secondaryAmount;
                    cargo->currentMass += secondaryAmount;
                }
            }
        }
    }
    
    void PerformMechanicalExtraction(EntityManager* em, Entity minerEntity,
                                     ExtractorComponent* extractor, double deltaTime) {
        auto* target = em->GetComponent<EnhancedResourceDepositComponent>(extractor->targetEntityID);
        if (!target || target->primaryQuantity <= 0.0f) {
            extractor->active = false;
            extractor->targetEntityID = -1;
            return;
        }
        
        float minedAmount = extractor->miningRate * static_cast<float>(deltaTime);
        minedAmount = std::min(minedAmount, target->primaryQuantity);
        
        target->primaryQuantity -= minedAmount;
        
        // Wear down tool
        extractor->durability -= extractor->wearRate * minedAmount;
        if (extractor->durability <= 0.0f) {
            extractor->durability = 0.0f;
            extractor->active = false;
        }
        
        // Add to cargo
        if (auto* cargo = em->GetComponent<ResourceCargoComponent>(minerEntity)) {
            if (cargo->currentMass + minedAmount <= cargo->capacity) {
                cargo->resources[target->primaryResource] += minedAmount;
                cargo->currentMass += minedAmount;
            }
        }
    }
    
    void ScanForDeposits(EntityManager* em, Entity scannerEntity,
                        ProspectorComponent* prospector, Position* scannerPos) {
        prospector->detectedDeposits.clear();
        prospector->depositValues.clear();
        
        auto deposits = em->GetEntitiesWithComponent<EnhancedResourceDepositComponent>();
        for (auto depositEntity : deposits) {
            auto* deposit = em->GetComponent<EnhancedResourceDepositComponent>(depositEntity);
            auto* depositPos = em->GetComponent<Position>(depositEntity);
            
            if (!deposit || !depositPos) continue;
            
            // Check range
            float dx = static_cast<float>(depositPos->x - scannerPos->x);
            float dy = static_cast<float>(depositPos->y - scannerPos->y);
            float dz = static_cast<float>(depositPos->z - scannerPos->z);
            float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (distance <= prospector->scanRange) {
                deposit->discovered = true;
                deposit->surveyed = true;
                deposit->surveyAccuracy = std::min(1.0f, 
                    prospector->scanResolution + prospector->accuracyBonus);
                
                prospector->detectedDeposits.push_back(depositEntity);
                
                // Estimate value (simplified)
                float value = deposit->primaryQuantity * GetResourceBaseValue(deposit->primaryResource);
                prospector->depositValues[depositEntity] = value;
            }
        }
    }
    
    void UpdateDroneProspecting(EntityManager* em, Entity droneEntity,
                               MiningDroneComponent* drone, double deltaTime) {
        auto* dronePos = em->GetComponent<Position>(droneEntity);
        if (!dronePos) return;
        
        // Search for nearest undiscovered deposit
        Entity nearestDeposit = -1;
        float nearestDist = drone->searchRadius;
        
        auto deposits = em->GetEntitiesWithComponent<EnhancedResourceDepositComponent>();
        for (auto depositEntity : deposits) {
            auto* deposit = em->GetComponent<EnhancedResourceDepositComponent>(depositEntity);
            auto* depositPos = em->GetComponent<Position>(depositEntity);
            
            if (!deposit || !depositPos || deposit->primaryQuantity <= 0.0f) continue;
            if (deposit->primaryResource != drone->targetResource) continue;
            
            float dx = static_cast<float>(depositPos->x - dronePos->x);
            float dy = static_cast<float>(depositPos->y - dronePos->y);
            float dz = static_cast<float>(depositPos->z - dronePos->z);
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestDeposit = depositEntity;
            }
        }
        
        if (nearestDeposit >= 0) {
            drone->targetDepositID = nearestDeposit;
            drone->mode = MiningDroneComponent::DroneMode::Mining;
        }
    }
    
    void UpdateDroneMining(EntityManager* em, Entity droneEntity,
                          MiningDroneComponent* drone, double deltaTime) {
        auto* target = em->GetComponent<EnhancedResourceDepositComponent>(drone->targetDepositID);
        if (!target || target->primaryQuantity <= 0.0f) {
            drone->mode = MiningDroneComponent::DroneMode::Prospecting;
            drone->targetDepositID = -1;
            return;
        }
        
        float minedAmount = drone->miningRate * static_cast<float>(deltaTime);
        minedAmount = std::min(minedAmount, target->primaryQuantity);
        minedAmount = std::min(minedAmount, drone->cargoCapacity - drone->currentCargo);
        
        target->primaryQuantity -= minedAmount;
        drone->currentCargo += minedAmount;
        
        if (drone->currentCargo >= drone->cargoCapacity * 0.95f) {
            drone->mode = MiningDroneComponent::DroneMode::Returning;
        }
    }
    
    void UpdateDroneReturning(EntityManager* em, Entity droneEntity,
                             MiningDroneComponent* drone, double deltaTime) {
        if (drone->mothershipID < 0) {
            drone->mode = MiningDroneComponent::DroneMode::Idle;
            return;
        }
        
        auto* dronePos = em->GetComponent<Position>(droneEntity);
        auto* mothershipPos = em->GetComponent<Position>(drone->mothershipID);
        
        if (!dronePos || !mothershipPos) return;
        
        float dx = static_cast<float>(mothershipPos->x - dronePos->x);
        float dy = static_cast<float>(mothershipPos->y - dronePos->y);
        float dz = static_cast<float>(mothershipPos->z - dronePos->z);
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist < 50.0f) {
            // Transfer cargo to mothership
            if (auto* mothershipCargo = em->GetComponent<ResourceCargoComponent>(drone->mothershipID)) {
                // Simplified - just add to mothership cargo
                mothershipCargo->currentMass += drone->currentCargo;
                drone->currentCargo = 0.0f;
            }
            
            // Recharge
            drone->mode = MiningDroneComponent::DroneMode::Recharging;
            drone->remainingPower = drone->autonomy;
        }
    }
    
    void ApplyHazardDamage(EntityManager* em, MiningHazardComponent* hazard,
                          Position* hazardPos, double deltaTime) {
        // Find entities within hazard radius
        auto allEntities = em->GetAllEntities();
        for (auto entity : allEntities) {
            auto* entityPos = em->GetComponent<Position>(entity);
            if (!entityPos) continue;
            
            float dx = static_cast<float>(entityPos->x - hazardPos->x);
            float dy = static_cast<float>(entityPos->y - hazardPos->y);
            float dz = static_cast<float>(entityPos->z - hazardPos->z);
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist <= hazard->radius) {
                // Apply damage based on hazard type
                float damage = hazard->damageRate * hazard->intensity * static_cast<float>(deltaTime);
                
                // Apply to health if present
                if (auto* health = em->GetComponent<Health>(entity)) {
                    health->current -= damage;
                }
                
                // Apply to durability if present
                if (auto* durability = em->GetComponent<ToolDurabilityComponent>(entity)) {
                    durability->condition -= damage * 0.1f;
                }
            }
        }
    }
    
    float GetResourceBaseValue(ResourceType type) {
        // Base value per kg in credits
        switch (type) {
            case ResourceType::IronOre: return 10.0f;
            case ResourceType::CopperOre: return 15.0f;
            case ResourceType::TitaniumOre: return 50.0f;
            case ResourceType::PlatinumOre: return 200.0f;
            case ResourceType::GoldOre: return 250.0f;
            case ResourceType::RareEarthElements: return 300.0f;
            case ResourceType::ExoticCrystals: return 1000.0f;
            case ResourceType::Helium3: return 400.0f;
            default: return 20.0f;
        }
    }
};

} // namespace Nova
