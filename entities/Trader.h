#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/EconomyComponents.h"
#include "../engine/ecs/EconomySystems.h"

/**
 * Trader: NPC trader ship with autonomous trading behavior
 * 
 * Features:
 * - Autonomous trade route navigation
 * - Buy low, sell high AI
 * - Cargo management
 * - Contract completion
 * - Faction reputation system
 * 
 * Configuration file: assets/actors/ships/trader.json
 */
class Trader : public IActor {
public:
    enum class TraderState {
        Idle,
        TravelingToMarket,
        Docked,
        Trading,
        OnContract
    };

    Trader() = default;
    virtual ~Trader() = default;

    void Initialize() override {
        LoadConfiguration();
        SetupComponents();
        InitializeTrader();
        
        std::cout << "[Trader] Initialized: " << name_ 
                  << " (cargo: " << cargoCapacity_ << ")" << std::endl;
    }
    
    void Update(double dt) override {
        UpdateTrading(dt);
        UpdateNavigation(dt);
        UpdateState(dt);
    }
    
    std::string GetName() const override { return name_; }

    // Trader operations
    TraderState GetState() const { return state_; }
    void SetTargetMarket(uint32_t marketEntity) { targetMarket_ = marketEntity; }
    
    // Check profitability
    bool IsRouteProfitable(uint32_t startMarket, uint32_t endMarket, const std::string& commodityId) {
        if (auto* em = context_.GetEntityManager()) {
            TradeSystem tradeSystem;
            double profit = tradeSystem.CalculateRouteProfit(*em, startMarket, endMarket, 
                                                            commodityId, 10);
            return profit > minProfitThreshold_;
        }
        return false;
    }

private:
    // Configuration
    std::string name_ = "Trader Ship";
    double cargoCapacity_ = 100.0;
    double credits_ = 5000.0;
    std::string faction_ = "independent";
    std::vector<std::string> specialization_;  // Preferred commodities
    bool canCarryContraband_ = false;
    
    // State
    TraderState state_ = TraderState::Idle;
    uint32_t currentMarket_ = 0;
    uint32_t targetMarket_ = 0;
    double stateTimer_ = 0.0;
    double minProfitThreshold_ = 100.0;
    
    // Systems
    TradeSystem tradeSystem_;
    TradeRouteSystem routeSystem_;
    CargoManagementSystem cargoSystem_;
    
    void LoadConfiguration() {
        try {
            auto config = ActorConfig::LoadFromFile("assets/actors/ships/trader.json");
            
            if (config) {
                name_ = ActorConfig::GetString(*config, "name", "Trader Ship");
                cargoCapacity_ = ActorConfig::GetNumber(*config, "cargoCapacity", 100.0);
                credits_ = ActorConfig::GetNumber(*config, "credits", 5000.0);
                faction_ = ActorConfig::GetString(*config, "faction", "independent");
                canCarryContraband_ = ActorConfig::GetBoolean(*config, "canCarryContraband", false);
            }
        } catch (const std::exception& e) {
            std::cout << "[Trader] Config error: " << e.what() << std::endl;
        }
    }

    void SetupComponents() {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Rendering components
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(entity, vp);
            
            auto pos = std::make_shared<Position>(0.0, 0.0, 0.0);
            em->AddComponent<Position>(entity, pos);
            
            auto vel = std::make_shared<Velocity>(0.0, 0.0, 0.0);
            em->AddComponent<Velocity>(entity, vel);
            
            auto draw = std::make_shared<DrawComponent>();
            draw->mode = DrawComponent::RenderMode::Mesh3D;
            draw->visible = true;
            draw->renderLayer = 1;
            draw->meshHandle = 0;
            draw->SetTint(0.6f, 0.4f, 0.2f);  // Brown trader ship
            em->AddComponent<DrawComponent>(entity, draw);
            
            // Physics
            auto physics = std::make_shared<PhysicsBody>();
            physics->mass = 500.0;
            em->AddComponent<PhysicsBody>(entity, physics);
        }
    }

    void InitializeTrader() {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Add CargoHold
            auto cargo = std::make_shared<CargoHold>();
            cargo->maxCapacity = cargoCapacity_;
            cargo->usedCapacity = 0.0;
            cargo->allowsContraband = canCarryContraband_;
            em->AddComponent<CargoHold>(entity, cargo);
            
            // Add BankAccount
            auto bank = std::make_shared<BankAccount>();
            bank->balance = credits_;
            bank->creditLimit = credits_ * 2.0;
            bank->debt = 0.0;
            bank->interestRate = 0.05;
            em->AddComponent<BankAccount>(entity, bank);
            
            // Add TraderReputation
            auto rep = std::make_shared<TraderReputation>();
            rep->reliability = 1.0;
            rep->successfulTrades = 0;
            rep->failedTrades = 0;
            rep->specializations = specialization_;
            rep->isPirate = false;
            em->AddComponent<TraderReputation>(entity, rep);
            
            std::cout << "[Trader] Economy components initialized" << std::endl;
        }
    }

    void UpdateTrading(double dt) {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            switch (state_) {
                case TraderState::Idle:
                    FindTradeOpportunity(*em);
                    break;
                    
                case TraderState::TravelingToMarket:
                    // Navigation handled separately
                    break;
                    
                case TraderState::Docked:
                    stateTimer_ += dt;
                    if (stateTimer_ >= 5.0) {  // Dock for 5 seconds
                        state_ = TraderState::Trading;
                        stateTimer_ = 0.0;
                    }
                    break;
                    
                case TraderState::Trading:
                    ExecuteTrade(*em, entity);
                    break;
                    
                case TraderState::OnContract:
                    // Contract handling
                    break;
            }
        }
    }

    void UpdateNavigation(double dt) {
        if (state_ != TraderState::TravelingToMarket) return;
        
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            auto* pos = em->GetComponent<Position>(entity);
            auto* targetPos = em->GetComponent<Position>(targetMarket_);
            
            if (!pos || !targetPos) return;
            
            // Simple navigation toward target
            double dx = targetPos->x - pos->x;
            double dy = targetPos->y - pos->y;
            double dz = targetPos->z - pos->z;
            double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist < 10.0) {  // Arrived at market
                currentMarket_ = targetMarket_;
                state_ = TraderState::Docked;
                stateTimer_ = 0.0;
                
                auto* vel = em->GetComponent<Velocity>(entity);
                if (vel) {
                    vel->vx = vel->vy = vel->vz = 0.0;
                }
                
                std::cout << "[Trader] Arrived at market" << std::endl;
            } else {
                // Move toward target
                double speed = 5.0;  // units per second
                auto* vel = em->GetComponent<Velocity>(entity);
                if (vel) {
                    vel->vx = (dx / dist) * speed;
                    vel->vy = (dy / dist) * speed;
                    vel->vz = (dz / dist) * speed;
                }
            }
        }
    }

    void UpdateState(double dt) {
        (void)dt;
        // Additional state management logic
    }

    void FindTradeOpportunity(EntityManager& em) {
        // Find profitable trade routes
        auto routes = routeSystem_.FindProfitableRoutes(em, context_.GetEntity(), 5);
        
        if (routes.empty()) {
            std::cout << "[Trader] No profitable routes found" << std::endl;
            return;
        }
        
        // Pick best route considering risk
        const TradeRoute* bestRoute = nullptr;
        double bestScore = 0.0;
        
        for (const auto& route : routes) {
            double score = route.profitMargin * (1.0 - route.risk);
            if (score > bestScore) {
                bestScore = score;
                bestRoute = &route;
            }
        }
        
        if (bestRoute) {
            targetMarket_ = bestRoute->startStation;
            state_ = TraderState::TravelingToMarket;
            
            std::cout << "[Trader] Found route with profit: " 
                      << bestRoute->profitMargin << std::endl;
        }
    }

    void ExecuteTrade(EntityManager& em, uint32_t entity) {
        auto* cargo = em.GetComponent<CargoHold>(entity);
        auto* bank = em.GetComponent<BankAccount>(entity);
        
        if (!cargo || !bank) {
            state_ = TraderState::Idle;
            return;
        }
        
        // If cargo is full, try to sell
        if (cargo->usedCapacity >= cargo->maxCapacity * 0.8) {
            SellCargo(em, entity, currentMarket_);
        }
        // If cargo is empty, try to buy
        else if (cargo->usedCapacity < cargo->maxCapacity * 0.2) {
            BuyCargo(em, entity, currentMarket_);
        }
        
        // After trading, return to idle to find next route
        state_ = TraderState::Idle;
        stateTimer_ = 0.0;
    }

    void BuyCargo(EntityManager& em, uint32_t entity, uint32_t marketEntity) {
        auto* market = em.GetComponent<MarketInventory>(marketEntity);
        if (!market) return;
        
        // Find cheapest commodity in stock
        std::string cheapestId;
        double cheapestPrice = std::numeric_limits<double>::max();
        
        for (const auto& [commodityId, price] : market->prices) {
            auto stockIt = market->stock.find(commodityId);
            if (stockIt != market->stock.end() && stockIt->second > 0) {
                if (price < cheapestPrice) {
                    cheapestPrice = price;
                    cheapestId = commodityId;
                }
            }
        }
        
        if (!cheapestId.empty()) {
            int quantity = std::min(10, market->stock[cheapestId]);
            auto result = tradeSystem_.BuyCommodity(em, entity, marketEntity, 
                                                   cheapestId, quantity);
            
            if (result.success) {
                std::cout << "[Trader] Bought " << quantity << "x " << cheapestId << std::endl;
                
                auto* rep = em.GetComponent<TraderReputation>(entity);
                if (rep) rep->successfulTrades++;
            }
        }
    }

    void SellCargo(EntityManager& em, uint32_t entity, uint32_t marketEntity) {
        auto* cargo = em.GetComponent<CargoHold>(entity);
        if (!cargo || cargo->cargo.empty()) return;
        
        // Sell first commodity in cargo
        const auto& slot = cargo->cargo[0];
        auto result = tradeSystem_.SellCommodity(em, entity, marketEntity, 
                                                slot.commodityId, slot.quantity);
        
        if (result.success) {
            std::cout << "[Trader] Sold " << slot.quantity << "x " 
                      << slot.commodityId << std::endl;
            
            auto* rep = em.GetComponent<TraderReputation>(entity);
            if (rep) rep->successfulTrades++;
        }
    }
};
