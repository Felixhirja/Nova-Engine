#pragma once

#include "Component.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// ============================================================================
// ECONOMY & TRADING COMPONENTS
// ============================================================================

/**
 * CommodityType: Types of tradeable goods
 */
enum class CommodityType {
    RawMaterials,      // Ore, minerals, raw resources
    Manufactured,      // Processed goods, components
    Luxuries,          // High-value consumer goods
    Contraband,        // Illegal goods
    Fuel,             // Ship fuel and energy
    Food,             // Consumables for stations
    Technology,       // Advanced tech items
    Medical,          // Medical supplies
    Weapons,          // Armaments
    Data              // Information commodities
};

/**
 * CommodityItem: Individual commodity with properties
 */
struct CommodityItem {
    std::string id;
    std::string name;
    CommodityType type;
    double basePrice = 100.0;
    double volume = 1.0;        // Cargo space required
    double mass = 1.0;          // Weight
    bool isLegal = true;
    int dangerLevel = 0;        // 0-5 risk factor
    std::string description;
};

/**
 * CargoSlot: Individual cargo slot in inventory
 */
struct CargoSlot {
    std::string commodityId;
    int quantity = 0;
    double purchasePrice = 0.0;
    std::string origin;         // Where it was purchased
    double timestamp = 0.0;     // When it was acquired
};

/**
 * TradeCargo: Enhanced cargo component for trading (uses existing CargoHold)
 * This extends the basic CargoHold with trading-specific functionality
 */
struct TradeCargo : public Component {
    std::vector<CargoSlot> tradeSlots;   // Trading cargo slots
    bool allowsContraband = false;       // Can carry illegal goods
    
    // Helper to calculate used capacity for trading goods
    double CalculateUsedCapacity() const {
        double total = 0.0;
        for (const auto& slot : tradeSlots) {
            // Will be calculated based on commodity volume
            total += slot.quantity;  // Simplified for now
        }
        return total;
    }
};

/**
 * MarketInventory: Component for trading stations/shops
 */
struct MarketInventory : public Component {
    std::map<std::string, int> stock;           // commodity_id -> quantity
    std::map<std::string, double> prices;       // commodity_id -> current price
    std::map<std::string, double> buyPrices;    // commodity_id -> buy-back price
    double cashReserve = 100000.0;              // Station's buying power
    std::string marketType = "general";         // Specialization
    double priceVolatility = 0.1;               // Market price variance
    double refreshRate = 3600.0;                // Stock refresh time (seconds)
    double lastRefresh = 0.0;
};

/**
 * EconomicZone: Component for regions with economic properties
 */
struct EconomicZone : public Component {
    std::string zoneName;
    double economicStrength = 1.0;      // GDP multiplier
    double supplyMultiplier = 1.0;      // Production rate
    double demandMultiplier = 1.0;      // Consumption rate
    std::vector<std::string> primaryExports;
    std::vector<std::string> primaryImports;
    double taxRate = 0.05;              // Transaction tax
    bool isBlackMarket = false;
};

/**
 * TradeRoute: Component for established trade paths
 */
struct TradeRoute : public Component {
    uint32_t startStation = 0;
    uint32_t endStation = 0;
    std::string commodityId;
    double profitMargin = 0.0;          // Expected profit percentage
    double risk = 0.0;                  // 0-1 danger level
    double distance = 0.0;
    double estimatedTime = 0.0;         // Travel time in seconds
    bool isActive = true;
    int popularity = 0;                 // How many traders use this
};

/**
 * TraderReputation: Component for tracking NPC trader behavior
 */
struct TraderReputation : public Component {
    std::map<std::string, double> factionRep;  // faction_id -> reputation
    double reliability = 1.0;           // 0-1, affects mission generation
    int successfulTrades = 0;
    int failedTrades = 0;
    std::vector<std::string> specializations;  // Preferred commodities
    bool isPirate = false;
};

/**
 * Contract: Trading/courier mission component
 */
struct Contract : public Component {
    enum class Type {
        Delivery,           // Deliver goods
        Courier,           // Transport data/package
        Purchase,          // Buy specific commodity
        Sell,             // Sell specific commodity
        TradeRoute,       // Complete a trade circuit
        Smuggling         // Illegal delivery
    };
    
    Type contractType = Type::Delivery;
    std::string clientName;
    std::string commodityId;
    int quantity = 0;
    uint32_t originStation = 0;
    uint32_t destinationStation = 0;
    double reward = 0.0;
    double penalty = 0.0;               // Failure penalty
    double deadline = 0.0;              // Time limit
    double timeRemaining = 0.0;
    bool isCompleted = false;
    bool isFailed = false;
    int dangerRating = 0;               // Expected threats
};

/**
 * BankAccount: Component for financial tracking
 */
struct BankAccount : public Component {
    double balance = 10000.0;
    double creditLimit = 50000.0;
    double debt = 0.0;
    double interestRate = 0.05;         // Annual rate
    std::vector<std::string> transactionHistory;
    double lastInterestUpdate = 0.0;
};

/**
 * Investment: Component for player investments
 */
struct Investment : public Component {
    std::string investmentType;         // "station", "trade_route", "mining_op"
    uint32_t targetEntity = 0;
    double amountInvested = 0.0;
    double currentValue = 0.0;
    double expectedReturn = 0.0;        // Annual percentage
    double maturityTime = 0.0;          // When it can be withdrawn
    bool isActive = true;
};

/**
 * PriceHistory: Component tracking price changes
 */
struct PriceHistory : public Component {
    struct PricePoint {
        double timestamp;
        double price;
        int volume;                     // Trade volume
    };
    
    std::map<std::string, std::vector<PricePoint>> history;  // commodity_id -> prices
    int maxHistorySize = 100;           // Keep last N entries
    
    void AddPrice(const std::string& commodityId, double price, int volume, double timestamp);
    double GetAveragePrice(const std::string& commodityId, int samples = 10) const;
    double GetPriceTrend(const std::string& commodityId, int samples = 10) const;
};

/**
 * EconomicEvent: Component for market events
 */
struct EconomicEvent : public Component {
    enum class EventType {
        Boom,               // Economic prosperity
        Recession,          // Economic downturn
        Shortage,           // Supply shortage
        Surplus,            // Oversupply
        Blockade,           // Trade restricted
        Discovery,          // New resource found
        Disaster,           // Production loss
        WarDemand,         // Military consumption spike
        TechBreakthrough,  // New production method
        Scandal            // Price manipulation
    };
    
    EventType eventType = EventType::Boom;
    std::string affectedCommodity;
    std::vector<uint32_t> affectedStations;
    double magnitude = 1.0;             // Effect strength
    double duration = 0.0;              // How long it lasts
    double timeRemaining = 0.0;
    std::string description;
};

/**
 * PlayerTradingStation: Component for player-owned stations
 */
struct PlayerTradingStation : public Component {
    uint32_t ownerId = 0;               // Player entity
    std::string stationName;
    double constructionCost = 0.0;
    double maintenanceCost = 100.0;     // Per day
    double revenue = 0.0;               // Daily earnings
    int employeeCount = 0;
    int upgradeLevel = 1;
    std::vector<std::string> installedModules;
    double lastMaintenancePayment = 0.0;
};

/**
 * MarketAnalytics: Component for tracking market data
 */
struct MarketAnalytics : public Component {
    struct CommodityStats {
        double avgPrice = 0.0;
        double minPrice = 0.0;
        double maxPrice = 0.0;
        double totalVolume = 0.0;
        int transactions = 0;
        double priceVolatility = 0.0;
    };
    
    std::map<std::string, CommodityStats> statistics;
    double lastUpdateTime = 0.0;
    double updateInterval = 600.0;      // Update every 10 minutes
};

/**
 * BlackMarket: Component for illegal trading locations
 */
struct BlackMarket : public Component {
    double discoveryRisk = 0.3;         // Chance of getting caught
    double priceMarkup = 1.5;           // Price multiplier
    std::vector<std::string> availableContraband;
    std::map<std::string, int> contrabandStock;
    double heatLevel = 0.0;             // Law enforcement attention
    bool isCompromised = false;
};

// Inline implementations

inline void PriceHistory::AddPrice(const std::string& commodityId, double price, int volume, double timestamp) {
    auto& priceVec = history[commodityId];
    priceVec.push_back({timestamp, price, volume});
    
    // Keep history size limited
    if (static_cast<int>(priceVec.size()) > maxHistorySize) {
        priceVec.erase(priceVec.begin());
    }
}

inline double PriceHistory::GetAveragePrice(const std::string& commodityId, int samples) const {
    auto it = history.find(commodityId);
    if (it == history.end() || it->second.empty()) {
        return 0.0;
    }
    
    const auto& prices = it->second;
    int count = std::min(samples, static_cast<int>(prices.size()));
    double sum = 0.0;
    
    for (int i = 0; i < count; ++i) {
        sum += prices[prices.size() - 1 - i].price;
    }
    
    return sum / count;
}

inline double PriceHistory::GetPriceTrend(const std::string& commodityId, int samples) const {
    auto it = history.find(commodityId);
    if (it == history.end() || it->second.size() < 2) {
        return 0.0;
    }
    
    const auto& prices = it->second;
    int count = std::min(samples, static_cast<int>(prices.size()));
    
    if (count < 2) return 0.0;
    
    double oldPrice = prices[prices.size() - count].price;
    double newPrice = prices[prices.size() - 1].price;
    
    return (newPrice - oldPrice) / oldPrice;  // Percentage change
}
