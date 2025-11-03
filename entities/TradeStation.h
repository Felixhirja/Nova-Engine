#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/EconomyComponents.h"
#include "../engine/ecs/EconomySystems.h"

/**
 * TradeStation: Trading hub with market functionality
 * 
 * Implements full economy system with:
 * - Dynamic pricing based on supply/demand
 * - Commodity trading (buy/sell)
 * - Contract generation and management
 * - Economic events affecting prices
 * - Black market for contraband
 * - Market analytics and price history
 * 
 * Configuration file: assets/actors/world/trade_station.json
 */
class TradeStation : public IActor {
public:
    TradeStation() = default;
    virtual ~TradeStation() = default;

    void Initialize() override {
        LoadConfiguration();
        SetupComponents();
        InitializeMarket();
        
        std::cout << "[TradeStation] Initialized: " << name_ 
                  << " (type: " << marketType_ << ")" << std::endl;
    }
    
    void Update(double dt) override {
        UpdateMarket(dt);
        UpdateContracts(dt);
    }
    
    std::string GetName() const override { return name_; }

    // Market operations
    const MarketInventory* GetMarket() const {
        if (auto* em = context_.GetEntityManager()) {
            return em->GetComponent<MarketInventory>(context_.GetEntity());
        }
        return nullptr;
    }
    
    // Check if commodity is available
    bool HasCommodity(const std::string& commodityId, int quantity = 1) const {
        auto* market = GetMarket();
        if (!market) return false;
        
        auto it = market->stock.find(commodityId);
        return (it != market->stock.end() && it->second >= quantity);
    }
    
    // Get current price
    double GetPrice(const std::string& commodityId) const {
        auto* market = GetMarket();
        if (!market) return 0.0;
        
        auto it = market->prices.find(commodityId);
        return (it != market->prices.end()) ? it->second : 0.0;
    }

private:
    // Configuration
    std::string name_ = "Trade Station";
    std::string marketType_ = "general";    // general, industrial, luxury, military
    double cashReserve_ = 100000.0;
    std::string faction_ = "neutral";
    bool hasBlackMarket_ = false;
    
    // Market system instances
    MarketPricingSystem pricingSystem_;
    TradeSystem tradeSystem_;
    ContractSystem contractSystem_;
    
    void LoadConfiguration() {
        try {
            auto config = ActorConfig::LoadFromFile("assets/actors/world/trade_station.json");
            
            if (config) {
                name_ = ActorConfig::GetString(*config, "name", "Trade Station");
                marketType_ = ActorConfig::GetString(*config, "marketType", "general");
                cashReserve_ = ActorConfig::GetNumber(*config, "cashReserve", 100000.0);
                faction_ = ActorConfig::GetString(*config, "faction", "neutral");
                hasBlackMarket_ = ActorConfig::GetBoolean(*config, "hasBlackMarket", false);
                
                std::cout << "[TradeStation] Configuration loaded successfully" << std::endl;
            } else {
                std::cout << "[TradeStation] Using default configuration" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[TradeStation] Config load error: " << e.what() << std::endl;
        }
    }

    void SetupComponents() {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Add ViewportID for rendering
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(entity, vp);
            
            // Add Position
            auto pos = std::make_shared<Position>(0.0, 0.0, 0.0);
            em->AddComponent<Position>(entity, pos);
            
            // Add Velocity (stations are stationary)
            auto vel = std::make_shared<Velocity>(0.0, 0.0, 0.0);
            em->AddComponent<Velocity>(entity, vel);
            
            // Add DrawComponent for visual
            auto draw = std::make_shared<DrawComponent>();
            draw->mode = DrawComponent::RenderMode::Mesh3D;
            draw->visible = true;
            draw->renderLayer = 2;
            draw->meshHandle = 0;
            draw->SetTint(0.3f, 0.6f, 0.9f);  // Blue station
            em->AddComponent<DrawComponent>(entity, draw);
            
            std::cout << "[TradeStation] Basic components configured" << std::endl;
        }
    }

    void InitializeMarket() {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Initialize commodity database (once globally)
            static bool dbInitialized = false;
            if (!dbInitialized) {
                CommodityDatabase::Get().Initialize();
                dbInitialized = true;
            }
            
            // Add MarketInventory component
            auto market = std::make_shared<MarketInventory>();
            market->cashReserve = cashReserve_;
            market->marketType = marketType_;
            market->priceVolatility = 0.1;
            market->refreshRate = 3600.0;
            
            // Stock market based on type
            InitializeStock(*market);
            
            em->AddComponent<MarketInventory>(entity, market);
            
            // Add EconomicZone component
            auto zone = std::make_shared<EconomicZone>();
            zone->zoneName = name_;
            zone->economicStrength = 1.0;
            zone->supplyMultiplier = 1.0;
            zone->demandMultiplier = 1.0;
            zone->taxRate = 0.05;
            zone->isBlackMarket = false;
            em->AddComponent<EconomicZone>(entity, zone);
            
            // Add PriceHistory for tracking
            auto history = std::make_shared<PriceHistory>();
            history->maxHistorySize = 100;
            em->AddComponent<PriceHistory>(entity, history);
            
            // Add black market if enabled
            if (hasBlackMarket_) {
                auto blackMarket = std::make_shared<BlackMarket>();
                blackMarket->discoveryRisk = 0.3;
                blackMarket->priceMarkup = 1.5;
                blackMarket->availableContraband = {
                    "contraband_weapons",
                    "contraband_drugs"
                };
                em->AddComponent<BlackMarket>(entity, blackMarket);
            }
            
            // Initialize prices
            pricingSystem_.UpdateMarketPrices(*em, entity);
            
            std::cout << "[TradeStation] Market initialized with " 
                      << market->stock.size() << " commodities" << std::endl;
        }
    }

    void InitializeStock(MarketInventory& market) {
        auto& db = CommodityDatabase::Get();
        
        if (marketType_ == "general") {
            // General markets have diverse inventory
            market.stock["ore_iron"] = 200;
            market.stock["ore_copper"] = 150;
            market.stock["fuel_hydrogen"] = 500;
            market.stock["food_basic"] = 300;
            market.stock["metal_refined"] = 100;
            market.stock["components_electronics"] = 50;
            market.stock["medical_supplies"] = 80;
        } else if (marketType_ == "industrial") {
            // Industrial markets focus on raw materials and manufactured goods
            market.stock["ore_iron"] = 500;
            market.stock["ore_copper"] = 300;
            market.stock["ore_titanium"] = 150;
            market.stock["metal_refined"] = 400;
            market.stock["components_machinery"] = 200;
            market.stock["hull_plates"] = 100;
            market.stock["fuel_hydrogen"] = 300;
        } else if (marketType_ == "luxury") {
            // Luxury markets for high-end goods
            market.stock["luxury_wine"] = 50;
            market.stock["luxury_jewelry"] = 30;
            market.stock["luxury_artwork"] = 10;
            market.stock["food_luxury"] = 100;
            market.stock["tech_processors"] = 40;
            market.stock["medical_advanced"] = 25;
        } else if (marketType_ == "military") {
            // Military markets for weapons and fuel
            market.stock["weapons_small"] = 150;
            market.stock["weapons_heavy"] = 80;
            market.stock["fuel_antimatter"] = 50;
            market.stock["components_electronics"] = 200;
            market.stock["hull_plates"] = 300;
            market.stock["medical_supplies"] = 150;
        }
        
        // Initialize all prices to base values
        for (const auto& [commodityId, stock] : market.stock) {
            auto* commodity = db.GetCommodity(commodityId);
            if (commodity) {
                market.prices[commodityId] = commodity->basePrice;
                market.buyPrices[commodityId] = commodity->basePrice * 0.8;
            }
        }
    }

    void UpdateMarket(double dt) {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Update pricing system
            pricingSystem_.Update(*em, dt);
            
            // Refresh stock periodically
            auto* market = em->GetComponent<MarketInventory>(entity);
            if (market) {
                market->lastRefresh += dt;
                if (market->lastRefresh >= market->refreshRate) {
                    market->lastRefresh = 0.0;
                    ReplenishStock(*market);
                }
            }
        }
    }

    void UpdateContracts(double dt) {
        if (auto* em = context_.GetEntityManager()) {
            contractSystem_.Update(*em, dt);
        }
    }

    void ReplenishStock(MarketInventory& market) {
        // Slowly replenish stock based on market type
        for (auto& [commodityId, stock] : market.stock) {
            auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
            if (!commodity) continue;
            
            // Determine max stock based on market type
            int maxStock = 100;
            if (marketType_ == "industrial" && commodity->type == CommodityType::RawMaterials) {
                maxStock = 500;
            } else if (marketType_ == "luxury" && commodity->type == CommodityType::Luxuries) {
                maxStock = 50;
            } else if (marketType_ == "military" && commodity->type == CommodityType::Weapons) {
                maxStock = 200;
            }
            
            // Replenish 10% of max stock
            if (stock < maxStock) {
                int replenishment = static_cast<int>(maxStock * 0.1);
                stock = std::min(stock + replenishment, maxStock);
            }
        }
    }
};
