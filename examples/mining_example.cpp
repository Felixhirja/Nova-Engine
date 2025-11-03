/**
 * Mining System Example
 * 
 * Demonstrates the complete mining and resource extraction system including:
 * - Asteroid field generation
 * - Mining vessels with laser drills
 * - Automated mining drones
 * - Resource prospecting
 * - Refining operations
 * - Environmental hazards
 * - Trading at mining stations
 */

#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"
#include "../engine/ecs/MiningComponents.h"
#include "../engine/ecs/MiningSystems.h"
#include "../engine/ecs/PlanetaryComponents.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

// Simulated entity factory for example
Entity CreateMiningVessel(EntityManager* em, Nova::MiningVesselComponent::VesselClass vesselClass) {
    Entity vessel = em->CreateEntity();
    
    em->AddComponent<Position>(vessel, {0, 0, 0});
    em->AddComponent<Velocity>(vessel, {0, 0, 0});
    
    Nova::MiningVesselComponent mining;
    mining.vesselClass = vesselClass;
    mining.crewCapacity = 1 + static_cast<int>(vesselClass) * 2;
    mining.currentCrew = mining.crewCapacity;
    mining.laserDrillSlots = 1 + static_cast<int>(vesselClass);
    mining.cargoHolds = 1 + static_cast<int>(vesselClass);
    mining.certified = true;
    em->AddComponent<Nova::MiningVesselComponent>(vessel, mining);
    
    Nova::ResourceCargoComponent cargo;
    cargo.capacity = 1000.0f * mining.cargoHolds;
    em->AddComponent<Nova::ResourceCargoComponent>(vessel, cargo);
    
    Nova::LaserDrillComponent drill;
    drill.power = 100.0f;
    drill.miningRate = 12.0f;
    drill.range = 50.0f;
    em->AddComponent<Nova::LaserDrillComponent>(vessel, drill);
    
    Nova::ProspectorComponent prospector;
    prospector.scanRange = 500.0f;
    prospector.scanResolution = 0.7f;
    em->AddComponent<Nova::ProspectorComponent>(vessel, prospector);
    
    em->AddComponent<Nova::MiningStatsComponent>(vessel, {});
    em->AddComponent<Nova::ToolDurabilityComponent>(vessel, {});
    
    em->AddComponent<Health>(vessel, {100.0f, 100.0f});
    
    return vessel;
}

Entity CreateAsteroid(EntityManager* em, const glm::vec3& pos, Nova::ResourceType resource, float quantity) {
    Entity asteroid = em->CreateEntity();
    
    em->AddComponent<Position>(asteroid, {pos.x, pos.y, pos.z});
    em->AddComponent<Velocity>(asteroid, {0, 0, 0});
    
    Nova::EnhancedResourceDepositComponent deposit;
    deposit.primaryResource = resource;
    deposit.primaryQuantity = quantity;
    deposit.secondaryQuantity = quantity * 0.1f;
    deposit.density = 0.8f;
    deposit.miningDifficulty = 0.3f;
    deposit.hardness = 0.5f;
    deposit.radius = std::sqrt(quantity / 100.0f);
    deposit.position = pos;
    em->AddComponent<Nova::EnhancedResourceDepositComponent>(asteroid, deposit);
    
    em->AddComponent<Health>(asteroid, {quantity / 50.0f, quantity / 50.0f});
    
    return asteroid;
}

Entity CreateMiningDrone(EntityManager* em, Entity mothership, Nova::ResourceType targetResource) {
    Entity drone = em->CreateEntity();
    
    em->AddComponent<Position>(drone, {0, 0, 0});
    em->AddComponent<Velocity>(drone, {0, 0, 0});
    
    Nova::MiningDroneComponent droneComp;
    droneComp.mothershipID = mothership;
    droneComp.targetResource = targetResource;
    droneComp.autonomy = 3600.0f;
    droneComp.remainingPower = 3600.0f;
    droneComp.miningRate = 3.0f;
    droneComp.cargoCapacity = 200.0f;
    droneComp.searchRadius = 1000.0f;
    em->AddComponent<Nova::MiningDroneComponent>(drone, droneComp);
    
    Nova::ResourceCargoComponent cargo;
    cargo.capacity = 200.0f;
    em->AddComponent<Nova::ResourceCargoComponent>(drone, cargo);
    
    Nova::ExtractorComponent extractor;
    extractor.miningRate = 3.0f;
    extractor.range = 5.0f;
    em->AddComponent<Nova::ExtractorComponent>(drone, extractor);
    
    em->AddComponent<Health>(drone, {50.0f, 50.0f});
    
    return drone;
}

Entity CreateMiningStation(EntityManager* em, const glm::vec3& pos) {
    Entity station = em->CreateEntity();
    
    em->AddComponent<Position>(station, {pos.x, pos.y, pos.z});
    
    Nova::SurfaceBaseComponent base;
    base.type = Nova::SurfaceBaseComponent::BaseType::MiningStation;
    base.name = "Mining Hub Alpha";
    base.population = 100;
    base.hasMarket = true;
    base.hasRefueling = true;
    base.hasRepair = true;
    em->AddComponent<Nova::SurfaceBaseComponent>(station, base);
    
    Nova::ResourceCargoComponent cargo;
    cargo.capacity = 100000.0f;
    em->AddComponent<Nova::ResourceCargoComponent>(station, cargo);
    
    Nova::RefineryComponent refinery;
    refinery.type = Nova::RefineryComponent::RefineryType::AdvancedRefinery;
    refinery.processingRate = 20.0f;
    refinery.efficiency = 0.9f;
    em->AddComponent<Nova::RefineryComponent>(station, refinery);
    
    Nova::ResourceMarketComponent market;
    market.buyPrices[Nova::ResourceType::IronOre] = 10.0f;
    market.buyPrices[Nova::ResourceType::CopperOre] = 15.0f;
    market.buyPrices[Nova::ResourceType::TitaniumOre] = 50.0f;
    market.buyPrices[Nova::ResourceType::PlatinumOre] = 200.0f;
    market.sellPrices[Nova::ResourceType::Steel] = 25.0f;
    market.sellPrices[Nova::ResourceType::Electronics] = 50.0f;
    em->AddComponent<Nova::ResourceMarketComponent>(station, market);
    
    return station;
}

void PrintMiningStatus(EntityManager* em, Entity vessel) {
    auto* cargo = em->GetComponent<Nova::ResourceCargoComponent>(vessel);
    auto* stats = em->GetComponent<Nova::MiningStatsComponent>(vessel);
    auto* drill = em->GetComponent<Nova::LaserDrillComponent>(vessel);
    auto* durability = em->GetComponent<Nova::ToolDurabilityComponent>(vessel);
    
    std::cout << "\n=== Mining Vessel Status ===" << std::endl;
    
    if (cargo) {
        std::cout << "Cargo: " << std::fixed << std::setprecision(1) 
                  << cargo->currentMass << " / " << cargo->capacity << " kg ("
                  << (cargo->currentMass / cargo->capacity * 100.0f) << "%)" << std::endl;
        
        std::cout << "Resources:" << std::endl;
        for (const auto& [type, amount] : cargo->resources) {
            if (amount > 0.0f) {
                std::cout << "  - Type " << static_cast<int>(type) << ": " 
                          << amount << " kg" << std::endl;
            }
        }
    }
    
    if (stats) {
        std::cout << "Total Mined: " << stats->totalMinedMass << " kg" << std::endl;
        std::cout << "Session Mined: " << stats->sessionMinedMass << " kg" << std::endl;
    }
    
    if (drill) {
        std::cout << "Drill Power: " << drill->power << "%" << std::endl;
        std::cout << "Drill Heat: " << drill->currentHeat << " / " 
                  << drill->maxHeat << " (";
        std::cout << (drill->overheated ? "OVERHEATED" : "Normal") << ")" << std::endl;
        std::cout << "Drill Status: " << (drill->active ? "ACTIVE" : "Idle") << std::endl;
    }
    
    if (durability) {
        std::cout << "Tool Condition: " << durability->condition << "%" << std::endl;
    }
}

void PrintDepositStatus(EntityManager* em, Entity asteroid) {
    auto* deposit = em->GetComponent<Nova::EnhancedResourceDepositComponent>(asteroid);
    auto* pos = em->GetComponent<Position>(asteroid);
    
    if (deposit && pos) {
        std::cout << "\nAsteroid at (" << pos->x << ", " << pos->y << ", " << pos->z << ")" << std::endl;
        std::cout << "  Resource: Type " << static_cast<int>(deposit->primaryResource) << std::endl;
        std::cout << "  Quantity: " << deposit->primaryQuantity << " kg" << std::endl;
        std::cout << "  Discovered: " << (deposit->discovered ? "Yes" : "No") << std::endl;
        std::cout << "  Surveyed: " << (deposit->surveyed ? "Yes" : "No") << std::endl;
    }
}

int main() {
    std::cout << "=== Mining & Resource Extraction System Demo ===" << std::endl;
    std::cout << std::endl;
    
    EntityManager em;
    Nova::MiningSystem miningSystem;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(4000.0f, 6000.0f);
    std::uniform_int_distribution<int> resourceDist(0, 4);
    std::uniform_real_distribution<float> quantityDist(5000.0f, 15000.0f);
    
    // Create mining station
    std::cout << "Creating mining station..." << std::endl;
    Entity station = CreateMiningStation(&em, {10000, 0, 0});
    
    // Create asteroid belt
    std::cout << "Generating asteroid field..." << std::endl;
    std::vector<Entity> asteroids;
    for (int i = 0; i < 20; i++) {
        float angle = angleDist(gen);
        float radius = radiusDist(gen);
        glm::vec3 pos(radius * cos(angle), 0, radius * sin(angle));
        
        auto resource = static_cast<Nova::ResourceType>(resourceDist(gen));
        float quantity = quantityDist(gen);
        
        Entity asteroid = CreateAsteroid(&em, pos, resource, quantity);
        asteroids.push_back(asteroid);
    }
    std::cout << "Created " << asteroids.size() << " asteroids" << std::endl;
    
    // Create mining vessel
    std::cout << "\nCreating mining vessel..." << std::endl;
    Entity vessel = CreateMiningVessel(&em, Nova::MiningVesselComponent::VesselClass::IndustrialMiner);
    auto* vesselPos = em.GetComponent<Position>(vessel);
    vesselPos->x = 5000;
    vesselPos->y = 0;
    vesselPos->z = 0;
    
    // Create mining drones
    std::cout << "Deploying mining drones..." << std::endl;
    std::vector<Entity> drones;
    for (int i = 0; i < 3; i++) {
        Entity drone = CreateMiningDrone(&em, vessel, Nova::ResourceType::IronOre);
        auto* dronePos = em.GetComponent<Position>(drone);
        dronePos->x = vesselPos->x + (i - 1) * 20;
        dronePos->z = vesselPos->z;
        drones.push_back(drone);
    }
    
    // Start prospecting scan
    std::cout << "\nStarting prospecting scan..." << std::endl;
    auto* prospector = em.GetComponent<Nova::ProspectorComponent>(vessel);
    prospector->scanning = true;
    
    // Simulation loop
    const int numSteps = 100;
    const double deltaTime = 1.0; // 1 second per step
    
    for (int step = 0; step < numSteps; step++) {
        // Update mining systems
        miningSystem.UpdateProspectors(&em, deltaTime);
        miningSystem.UpdateLaserDrills(&em, deltaTime);
        miningSystem.UpdateExtractors(&em, deltaTime);
        miningSystem.UpdateMiningDrones(&em, deltaTime);
        miningSystem.UpdateToolDurability(&em, deltaTime);
        
        // Check if scan completed
        if (step == 10 && prospector->scanProgress >= 1.0f) {
            std::cout << "\nScan complete! Detected " << prospector->detectedDeposits.size() 
                      << " deposits" << std::endl;
            
            // Start mining nearest asteroid
            if (!prospector->detectedDeposits.empty()) {
                Entity targetAsteroid = prospector->detectedDeposits[0];
                auto* drill = em.GetComponent<Nova::LaserDrillComponent>(vessel);
                drill->targetEntityID = targetAsteroid;
                drill->active = true;
                
                std::cout << "Starting mining operation on Asteroid #" << targetAsteroid << std::endl;
            }
        }
        
        // Activate drones after scan
        if (step == 15) {
            std::cout << "\nActivating mining drones..." << std::endl;
            for (auto droneEntity : drones) {
                auto* drone = em.GetComponent<Nova::MiningDroneComponent>(droneEntity);
                drone->mode = Nova::MiningDroneComponent::DroneMode::Prospecting;
            }
        }
        
        // Print status every 20 steps
        if (step > 0 && step % 20 == 0) {
            std::cout << "\n--- Time: " << step << " seconds ---" << std::endl;
            PrintMiningStatus(&em, vessel);
            
            // Show drone status
            int activeDrones = 0;
            for (auto droneEntity : drones) {
                auto* drone = em.GetComponent<Nova::MiningDroneComponent>(droneEntity);
                if (drone && drone->mode == Nova::MiningDroneComponent::DroneMode::Mining) {
                    activeDrones++;
                }
            }
            std::cout << "Active Drones: " << activeDrones << " / " << drones.size() << std::endl;
        }
    }
    
    // Final report
    std::cout << "\n\n=== Final Mining Report ===" << std::endl;
    PrintMiningStatus(&em, vessel);
    
    std::cout << "\n=== Asteroid Status ===" << std::endl;
    int depleted = 0;
    for (auto asteroid : asteroids) {
        auto* deposit = em.GetComponent<Nova::EnhancedResourceDepositComponent>(asteroid);
        if (deposit) {
            if (deposit->primaryQuantity <= 0.0f) {
                depleted++;
            }
            if (deposit->discovered) {
                PrintDepositStatus(&em, asteroid);
            }
        }
    }
    std::cout << "\nDepleted asteroids: " << depleted << " / " << asteroids.size() << std::endl;
    
    // Calculate total value
    auto* cargo = em.GetComponent<Nova::ResourceCargoComponent>(vessel);
    auto* market = em.GetComponent<Nova::ResourceMarketComponent>(station);
    if (cargo && market) {
        float totalValue = 0.0f;
        for (const auto& [type, amount] : cargo->resources) {
            float price = market->buyPrices[type];
            totalValue += amount * price;
        }
        std::cout << "\nTotal cargo value: " << totalValue << " credits" << std::endl;
    }
    
    std::cout << "\n=== Mining Demo Complete ===" << std::endl;
    
    return 0;
}
