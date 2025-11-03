#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/MiningComponents.h"
#include "../engine/ecs/PlanetaryComponents.h"

class MiningStation : public IActor {
private:
    std::string stationName_;
    
public:
    MiningStation(const std::string& name = "Mining Outpost Alpha")
        : stationName_(name) {}
    
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position
            em->AddComponent<Position>(entity, {0, 0, 0});
            
            // Surface base component
            Nova::SurfaceBaseComponent base;
            base.type = Nova::SurfaceBaseComponent::BaseType::MiningStation;
            base.name = stationName_;
            base.integrity = 100.0f;
            base.population = 50;
            base.powered = true;
            base.lifeSupportOnline = true;
            base.oxygenLevel = 100.0f;
            base.powerReserve = 100.0f;
            base.underConstruction = false;
            base.hasRefueling = true;
            base.hasRepair = true;
            base.hasMedical = true;
            base.hasMarket = true;
            
            em->AddComponent<Nova::SurfaceBaseComponent>(entity, base);
            
            // Large cargo storage
            Nova::ResourceCargoComponent cargo;
            cargo.capacity = 50000.0f; // 50 tons storage
            cargo.currentMass = 0.0f;
            cargo.autoSort = true;
            cargo.transferRate = 50.0f; // Fast loading/unloading
            em->AddComponent<Nova::ResourceCargoComponent>(entity, cargo);
            
            // Advanced refinery
            Nova::RefineryComponent refinery;
            refinery.type = Nova::RefineryComponent::RefineryType::AdvancedRefinery;
            refinery.active = false;
            refinery.processingRate = 20.0f; // Much faster
            refinery.efficiency = 0.9f; // Better yield
            refinery.inputStorageMax = 20000.0f;
            refinery.outputStorageMax = 18000.0f;
            
            // Setup common recipes
            refinery.availableRecipes[Nova::ResourceType::IronOre] = Nova::ResourceType::Steel;
            refinery.availableRecipes[Nova::ResourceType::CopperOre] = Nova::ResourceType::Electronics;
            refinery.availableRecipes[Nova::ResourceType::TitaniumOre] = Nova::ResourceType::AdvancedAlloys;
            
            em->AddComponent<Nova::RefineryComponent>(entity, refinery);
            
            // Resource market
            Nova::ResourceMarketComponent market;
            
            // Setup buy prices (station buys from miners)
            market.buyPrices[Nova::ResourceType::IronOre] = 10.0f;
            market.buyPrices[Nova::ResourceType::CopperOre] = 15.0f;
            market.buyPrices[Nova::ResourceType::NickelOre] = 18.0f;
            market.buyPrices[Nova::ResourceType::TitaniumOre] = 50.0f;
            market.buyPrices[Nova::ResourceType::PlatinumOre] = 200.0f;
            market.buyPrices[Nova::ResourceType::GoldOre] = 250.0f;
            market.buyPrices[Nova::ResourceType::RareEarthElements] = 300.0f;
            market.buyPrices[Nova::ResourceType::ExoticCrystals] = 1000.0f;
            market.buyPrices[Nova::ResourceType::WaterIce] = 5.0f;
            market.buyPrices[Nova::ResourceType::Helium3] = 400.0f;
            
            // Sell prices (station sells refined goods)
            market.sellPrices[Nova::ResourceType::Steel] = 25.0f;
            market.sellPrices[Nova::ResourceType::Electronics] = 50.0f;
            market.sellPrices[Nova::ResourceType::AdvancedAlloys] = 120.0f;
            market.sellPrices[Nova::ResourceType::FusionFuel] = 800.0f;
            market.sellPrices[Nova::ResourceType::Nanomaterials] = 2000.0f;
            
            // Initial supply
            market.supply[Nova::ResourceType::Steel] = 1000.0f;
            market.supply[Nova::ResourceType::Electronics] = 500.0f;
            
            market.marketVolatility = 0.1f;
            market.blackMarket = false;
            
            em->AddComponent<Nova::ResourceMarketComponent>(entity, market);
            
            // Mining claim registration (station can register claims)
            Nova::MiningClaimComponent claimRegistry;
            claimRegistry.claimantID = "Station_Registry";
            claimRegistry.claimRadius = 10000.0f; // 10km jurisdiction
            claimRegistry.registered = true;
            em->AddComponent<Nova::MiningClaimComponent>(entity, claimRegistry);
            
            // Landing zone
            Nova::LandingZoneComponent landingZone;
            landingZone.type = Nova::LandingZoneComponent::ZoneType::Spaceport;
            landingZone.radius = 100.0f;
            landingZone.occupied = false;
            landingZone.cleared = true;
            landingZone.maxShipSize = 5; // Can handle capital ships
            landingZone.hasBeacon = true;
            landingZone.controlled = true;
            em->AddComponent<Nova::LandingZoneComponent>(entity, landingZone);
            
            // Health
            Health health;
            health.max = 1000.0f;
            health.current = health.max;
            em->AddComponent<Health>(entity, health);
            
            // Visual
            DrawComponent draw;
            draw.mode = DrawComponent::RenderMode::Mesh3D;
            draw.SetTint(0.8f, 0.8f, 0.9f); // Metallic station
            draw.SetScale(30.0f, 20.0f, 30.0f); // Large structure
            em->AddComponent<DrawComponent>(entity, draw);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            auto* base = em->GetComponent<Nova::SurfaceBaseComponent>(entity);
            auto* market = em->GetComponent<Nova::ResourceMarketComponent>(entity);
            auto* draw = em->GetComponent<DrawComponent>(entity);
            
            if (base && draw) {
                // Visual indicators for station status
                if (!base->powered) {
                    draw->SetTint(0.3f, 0.3f, 0.3f); // Dark if unpowered
                } else if (!base->lifeSupportOnline) {
                    draw->SetTint(0.8f, 0.3f, 0.3f); // Red for life support failure
                } else {
                    // Normal operation - pulsing lights
                    float pulse = 0.8f + static_cast<float>(std::sin(context_.GetTime())) * 0.2f;
                    draw->SetTint(pulse, pulse, 0.9f);
                }
                
                // Show landing zone status with beacon effect
                if (auto* landing = em->GetComponent<Nova::LandingZoneComponent>(entity)) {
                    if (landing->hasBeacon && !landing->occupied) {
                        // Green beacon flash
                        if (static_cast<int>(context_.GetTime() * 2.0) % 2 == 0) {
                            draw->tintG = 1.0f;
                        }
                    }
                }
            }
            
            // Update market prices (simple fluctuation)
            if (market) {
                market->marketVolatility = 0.05f + static_cast<float>(
                    std::sin(context_.GetTime() * 0.1)) * 0.05f;
                
                // Prices fluctuate based on supply
                for (auto& [resourceType, price] : market->buyPrices) {
                    float supplyLevel = market->supply[resourceType];
                    if (supplyLevel > 5000.0f) {
                        // Oversupply - lower prices
                        price *= 0.999f;
                    } else if (supplyLevel < 1000.0f) {
                        // Low supply - increase prices
                        price *= 1.001f;
                    }
                }
            }
            
            // Auto-process refinery if there's input material
            if (auto* refinery = em->GetComponent<Nova::RefineryComponent>(entity)) {
                if (!refinery->active && refinery->inputAmount > 0.0f) {
                    refinery->active = true;
                }
            }
        }
    }
    
    std::string GetName() const override {
        return stationName_;
    }
};
