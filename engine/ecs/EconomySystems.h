#pragma once

#include "EconomyComponents.h"
#include "Components.h"
#include "EntityManager.h"
#include "System.h"
#include <random>
#include <algorithm>
#include <cmath>

// ============================================================================
// ECONOMY SYSTEMS
// ============================================================================

/**
 * CommodityDatabase: Global commodity definitions
 */
class CommodityDatabase {
public:
    static CommodityDatabase& Get() {
        static CommodityDatabase instance;
        return instance;
    }
    
    void Initialize();
    const CommodityItem* GetCommodity(const std::string& id) const;
    std::vector<std::string> GetAllCommodityIds() const;
    std::vector<const CommodityItem*> GetCommoditiesByType(CommodityType type) const;
    
private:
    CommodityDatabase() = default;
    std::unordered_map<std::string, CommodityItem> commodities_;
    
    void AddCommodity(const CommodityItem& item);
};

/**
 * MarketPricingSystem: Handles dynamic supply/demand pricing
 */
class MarketPricingSystem : public System {
public:
    MarketPricingSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Calculate price based on supply, demand, and market conditions
    double CalculatePrice(const std::string& commodityId, 
                         const MarketInventory& market,
                         const EconomicZone* zone = nullptr) const;
    
    // Update prices based on recent transactions
    void UpdateMarketPrices(EntityManager& em, uint32_t marketEntity);
    
    // Generate random price fluctuations
    void SimulatePriceVolatility(EntityManager& em);
    
private:
    std::mt19937 rng_;
    double updateAccumulator_ = 0.0;
    const double updateInterval_ = 60.0;  // Update every minute
};

/**
 * TradeSystem: Handles buy/sell transactions
 */
class TradeSystem : public System {
public:
    TradeSystem() : System(SystemType::Gameplay) {}
    
    struct TradeResult {
        bool success = false;
        std::string message;
        double totalCost = 0.0;
        double tax = 0.0;
    };
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Execute a buy transaction
    TradeResult BuyCommodity(EntityManager& em,
                            uint32_t buyerEntity,
                            uint32_t marketEntity,
                            const std::string& commodityId,
                            int quantity);
    
    // Execute a sell transaction
    TradeResult SellCommodity(EntityManager& em,
                             uint32_t sellerEntity,
                             uint32_t marketEntity,
                             const std::string& commodityId,
                             int quantity);
    
    // Transfer cargo between entities
    bool TransferCargo(EntityManager& em,
                      uint32_t fromEntity,
                      uint32_t toEntity,
                      const std::string& commodityId,
                      int quantity);
    
    // Calculate trade route profit
    double CalculateRouteProfit(EntityManager& em,
                               uint32_t startMarket,
                               uint32_t endMarket,
                               const std::string& commodityId,
                               int quantity) const;
};

/**
 * TradeRouteSystem: Manages and analyzes trade routes
 */
class TradeRouteSystem : public System {
public:
    TradeRouteSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Find profitable trade routes
    std::vector<TradeRoute> FindProfitableRoutes(EntityManager& em,
                                                  uint32_t playerEntity,
                                                  int maxRoutes = 10);
    
    // Calculate route risk based on distance and region
    double CalculateRouteRisk(EntityManager& em,
                             uint32_t startStation,
                             uint32_t endStation) const;
    
    // Update existing trade routes
    void RefreshTradeRoutes(EntityManager& em);
    
private:
    double routeUpdateTimer_ = 0.0;
    const double routeUpdateInterval_ = 300.0;  // Every 5 minutes
};

/**
 * ContractSystem: Generates and manages trading contracts
 */
class ContractSystem : public System {
public:
    ContractSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Generate random contracts for a station
    void GenerateContracts(EntityManager& em, uint32_t stationEntity, int count = 5);
    
    // Check if a contract is complete
    bool CheckContractCompletion(EntityManager& em, uint32_t contractEntity);
    
    // Apply contract rewards/penalties
    void CompleteContract(EntityManager& em, uint32_t contractEntity, uint32_t playerEntity);
    
    // Generate a specific type of contract
    Contract CreateContract(EntityManager& em,
                          Contract::Type type,
                          uint32_t stationEntity) const;
    
private:
    std::mt19937 rng_;
    double generationTimer_ = 0.0;
    const double generationInterval_ = 600.0;  // Generate new contracts every 10 minutes
};

/**
 * EconomicEventSystem: Manages market events (booms, recessions, etc.)
 */
class EconomicEventSystem : public System {
public:
    EconomicEventSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Trigger a random economic event
    void TriggerRandomEvent(EntityManager& em);
    
    // Create a specific event
    uint32_t CreateEvent(EntityManager& em,
                        EconomicEvent::EventType type,
                        const std::string& commodityId,
                        const std::vector<uint32_t>& affectedStations,
                        double magnitude,
                        double duration);
    
    // Apply event effects to markets
    void ApplyEventEffects(EntityManager& em, uint32_t eventEntity);
    
private:
    std::mt19937 rng_;
    double eventTimer_ = 0.0;
    const double minEventInterval_ = 1800.0;  // At least 30 minutes between events
    const double maxEventInterval_ = 7200.0;  // At most 2 hours between events
    double nextEventTime_ = 3600.0;
};

/**
 * BankingSystem: Handles loans, interest, and investments
 */
class BankingSystem : public System {
public:
    BankingSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Process loan application
    bool ApplyForLoan(EntityManager& em,
                     uint32_t applicantEntity,
                     double amount,
                     double interestRate,
                     double term);
    
    // Make loan payment
    bool MakeLoanPayment(EntityManager& em,
                        uint32_t borrowerEntity,
                        double amount);
    
    // Create investment
    uint32_t CreateInvestment(EntityManager& em,
                             uint32_t investorEntity,
                             const std::string& investmentType,
                             uint32_t targetEntity,
                             double amount,
                             double expectedReturn,
                             double maturityTime);
    
    // Process investment maturity and returns
    void ProcessInvestments(EntityManager& em);
    
private:
    double interestAccumulator_ = 0.0;
    const double interestInterval_ = 86400.0;  // Daily interest calculation
};

/**
 * MarketAnalyticsSystem: Tracks and analyzes market data
 */
class MarketAnalyticsSystem : public System {
public:
    MarketAnalyticsSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Record a transaction for analytics
    void RecordTransaction(EntityManager& em,
                          const std::string& commodityId,
                          double price,
                          int quantity,
                          uint32_t marketEntity);
    
    // Generate market report
    std::string GenerateMarketReport(EntityManager& em, uint32_t marketEntity) const;
    
    // Get price forecast
    double ForecastPrice(EntityManager& em,
                        const std::string& commodityId,
                        uint32_t marketEntity,
                        double hoursAhead) const;
    
private:
    void UpdateStatistics(EntityManager& em);
    double analyticsTimer_ = 0.0;
    const double analyticsInterval_ = 600.0;  // Update every 10 minutes
};

/**
 * BlackMarketSystem: Handles illegal trading and contraband
 */
class BlackMarketSystem : public System {
public:
    BlackMarketSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Attempt to access black market
    bool AttemptBlackMarketAccess(EntityManager& em,
                                  uint32_t playerEntity,
                                  uint32_t blackMarketEntity);
    
    // Buy contraband
    TradeSystem::TradeResult BuyContraband(EntityManager& em,
                                          uint32_t buyerEntity,
                                          uint32_t blackMarketEntity,
                                          const std::string& commodityId,
                                          int quantity);
    
    // Check for law enforcement detection
    bool CheckDetection(EntityManager& em, uint32_t playerEntity);
    
    // Apply detection consequences
    void ApplyDetectionPenalty(EntityManager& em, uint32_t playerEntity);
    
private:
    std::mt19937 rng_;
    double heatDecayTimer_ = 0.0;
    const double heatDecayInterval_ = 600.0;  // Heat level decays every 10 minutes
};

/**
 * CargoManagementSystem: Manages cargo loading/unloading
 */
class CargoManagementSystem : public System {
public:
    CargoManagementSystem() : System(SystemType::Gameplay) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
    // Add cargo to hold
    bool AddCargo(CargoHold& hold,
                 const std::string& commodityId,
                 int quantity,
                 double price,
                 const std::string& origin);
    
    // Remove cargo from hold
    int RemoveCargo(CargoHold& hold,
                   const std::string& commodityId,
                   int quantity);
    
    // Get cargo quantity
    int GetCargoQuantity(const CargoHold& hold, const std::string& commodityId) const;
    
    // Calculate total cargo value
    double CalculateCargoValue(const CargoHold& hold, EntityManager& em, uint32_t marketEntity) const;
    
    // Optimize cargo arrangement
    void OptimizeCargo(CargoHold& hold);
};
