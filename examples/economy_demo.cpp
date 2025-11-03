/**
 * Economy System Demonstration
 * 
 * This example shows how to use the Nova Engine economy & trading system:
 * - Initialize commodity database
 * - Create trading stations with markets
 * - Spawn NPC traders
 * - Execute buy/sell transactions
 * - Analyze trade routes
 * - Trigger economic events
 */

#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/EconomyComponents.h"
#include "../engine/ecs/EconomySystems.h"
#include "../entities/TradeStation.h"
#include "../entities/Trader.h"
#include <iostream>
#include <iomanip>

void PrintSeparator() {
    std::cout << "\n" << std::string(60, '=') << "\n" << std::endl;
}

void DemoCommodityDatabase() {
    std::cout << "=== COMMODITY DATABASE DEMO ===" << std::endl;
    
    auto& db = CommodityDatabase::Get();
    db.Initialize();
    
    auto commodities = db.GetAllCommodityIds();
    std::cout << "Total commodities: " << commodities.size() << std::endl;
    
    std::cout << "\nSample commodities:" << std::endl;
    for (int i = 0; i < 5 && i < static_cast<int>(commodities.size()); ++i) {
        auto* commodity = db.GetCommodity(commodities[i]);
        if (commodity) {
            std::cout << "  - " << commodity->name 
                      << " (" << commodity->id << ")"
                      << " | Base Price: " << commodity->basePrice << " cr"
                      << " | Volume: " << commodity->volume
                      << " | Legal: " << (commodity->isLegal ? "Yes" : "No")
                      << std::endl;
        }
    }
    
    // Show contraband
    std::cout << "\nContraband items:" << std::endl;
    auto contraband = db.GetCommoditiesByType(CommodityType::Contraband);
    for (auto* item : contraband) {
        std::cout << "  - " << item->name 
                  << " | Price: " << item->basePrice << " cr"
                  << " | Danger: " << item->dangerLevel << "/5"
                  << std::endl;
    }
}

void DemoMarketCreation(EntityManager& em) {
    std::cout << "\n=== MARKET CREATION DEMO ===" << std::endl;
    
    // Create a general trading station
    auto stationEntity = em.CreateEntity();
    
    // Add market inventory
    auto market = std::make_shared<MarketInventory>();
    market->cashReserve = 100000.0;
    market->marketType = "general";
    market->priceVolatility = 0.1;
    
    // Stock some commodities
    market->stock["ore_iron"] = 200;
    market->stock["fuel_hydrogen"] = 500;
    market->stock["food_basic"] = 300;
    market->stock["components_electronics"] = 50;
    
    // Initialize prices
    auto& db = CommodityDatabase::Get();
    for (const auto& [commodityId, stock] : market->stock) {
        auto* commodity = db.GetCommodity(commodityId);
        if (commodity) {
            market->prices[commodityId] = commodity->basePrice;
            market->buyPrices[commodityId] = commodity->basePrice * 0.8;
        }
    }
    
    em.AddComponent<MarketInventory>(stationEntity, market);
    
    // Add economic zone
    auto zone = std::make_shared<EconomicZone>();
    zone->zoneName = "Frontier Sector";
    zone->economicStrength = 1.2;  // 20% stronger economy
    zone->taxRate = 0.05;
    em.AddComponent<EconomicZone>(stationEntity, zone);
    
    std::cout << "Created trading station with market" << std::endl;
    std::cout << "Market type: " << market->marketType << std::endl;
    std::cout << "Cash reserve: " << market->cashReserve << " cr" << std::endl;
    std::cout << "Commodities in stock: " << market->stock.size() << std::endl;
}

void DemoTrading(EntityManager& em, uint32_t playerEntity, uint32_t stationEntity) {
    std::cout << "\n=== TRADING DEMO ===" << std::endl;
    
    TradeSystem tradeSystem;
    
    // Give player initial funds and cargo hold
    auto bank = std::make_shared<BankAccount>();
    bank->balance = 10000.0;
    em.AddComponent<BankAccount>(playerEntity, bank);
    
    auto cargo = std::make_shared<CargoHold>();
    cargo->maxCapacity = 100.0;
    cargo->usedCapacity = 0.0;
    em.AddComponent<CargoHold>(playerEntity, cargo);
    
    std::cout << "Player starting balance: " << bank->balance << " cr" << std::endl;
    std::cout << "Player cargo capacity: " << cargo->maxCapacity << std::endl;
    
    // Buy some iron ore
    std::cout << "\nAttempting to buy 10x Iron Ore..." << std::endl;
    auto buyResult = tradeSystem.BuyCommodity(em, playerEntity, stationEntity, 
                                             "ore_iron", 10);
    
    if (buyResult.success) {
        std::cout << "✓ Purchase successful!" << std::endl;
        std::cout << "  Total cost: " << buyResult.totalCost << " cr" << std::endl;
        std::cout << "  Tax: " << buyResult.tax << " cr" << std::endl;
        std::cout << "  New balance: " << bank->balance << " cr" << std::endl;
        std::cout << "  Cargo used: " << cargo->usedCapacity << "/" 
                  << cargo->maxCapacity << std::endl;
    } else {
        std::cout << "✗ Purchase failed: " << buyResult.message << std::endl;
    }
    
    // Try to sell it back
    std::cout << "\nAttempting to sell 5x Iron Ore..." << std::endl;
    auto sellResult = tradeSystem.SellCommodity(em, playerEntity, stationEntity, 
                                               "ore_iron", 5);
    
    if (sellResult.success) {
        std::cout << "✓ Sale successful!" << std::endl;
        std::cout << "  Revenue: " << sellResult.totalCost << " cr" << std::endl;
        std::cout << "  Tax: " << sellResult.tax << " cr" << std::endl;
        std::cout << "  New balance: " << bank->balance << " cr" << std::endl;
    } else {
        std::cout << "✗ Sale failed: " << sellResult.message << std::endl;
    }
}

void DemoTradeRoutes(EntityManager& em, uint32_t playerEntity) {
    std::cout << "\n=== TRADE ROUTE DEMO ===" << std::endl;
    
    // Create two stations with different prices
    auto station1 = em.CreateEntity();
    auto market1 = std::make_shared<MarketInventory>();
    market1->stock["ore_iron"] = 500;
    market1->prices["ore_iron"] = 50.0;   // Cheap iron
    market1->buyPrices["ore_iron"] = 40.0;
    em.AddComponent<MarketInventory>(station1, market1);
    
    auto station2 = em.CreateEntity();
    auto market2 = std::make_shared<MarketInventory>();
    market2->stock["ore_iron"] = 10;
    market2->prices["ore_iron"] = 80.0;    // Expensive iron
    market2->buyPrices["ore_iron"] = 70.0;
    em.AddComponent<MarketInventory>(station2, market2);
    
    // Add positions for distance calculation
    em.AddComponent<Position>(station1, std::make_shared<Position>(0, 0, 0));
    em.AddComponent<Position>(station2, std::make_shared<Position>(100, 0, 0));
    
    std::cout << "Created two stations with price differences" << std::endl;
    std::cout << "Station 1 - Iron ore: " << market1->prices["ore_iron"] << " cr" << std::endl;
    std::cout << "Station 2 - Iron ore: " << market2->prices["ore_iron"] << " cr" << std::endl;
    
    // Find profitable routes
    TradeRouteSystem routeSystem;
    auto routes = routeSystem.FindProfitableRoutes(em, playerEntity, 5);
    
    std::cout << "\nProfitable routes found: " << routes.size() << std::endl;
    for (size_t i = 0; i < routes.size(); ++i) {
        const auto& route = routes[i];
        std::cout << "\nRoute " << (i+1) << ":" << std::endl;
        std::cout << "  Commodity: " << route.commodityId << std::endl;
        std::cout << "  Profit: " << route.profitMargin << " cr" << std::endl;
        std::cout << "  Risk: " << std::fixed << std::setprecision(1) 
                  << (route.risk * 100) << "%" << std::endl;
    }
}

void DemoPricing(EntityManager& em, uint32_t stationEntity) {
    std::cout << "\n=== DYNAMIC PRICING DEMO ===" << std::endl;
    
    MarketPricingSystem pricingSystem;
    auto* market = em.GetComponent<MarketInventory>(stationEntity);
    
    if (!market) {
        std::cout << "No market found on station" << std::endl;
        return;
    }
    
    std::cout << "Initial prices:" << std::endl;
    for (const auto& [commodityId, price] : market->prices) {
        std::cout << "  " << commodityId << ": " << std::fixed 
                  << std::setprecision(2) << price << " cr" << std::endl;
    }
    
    // Simulate price volatility
    std::cout << "\nSimulating market volatility..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        pricingSystem.Update(em, 60.0);  // Update with 60 seconds elapsed
        std::cout << "\nAfter " << ((i+1) * 60) << " seconds:" << std::endl;
        
        for (const auto& [commodityId, price] : market->prices) {
            std::cout << "  " << commodityId << ": " << std::fixed 
                      << std::setprecision(2) << price << " cr" << std::endl;
        }
    }
}

void DemoEconomicEvents(EntityManager& em) {
    std::cout << "\n=== ECONOMIC EVENTS DEMO ===" << std::endl;
    
    EconomicEventSystem eventSystem;
    
    // Create a market to affect
    auto stationEntity = em.CreateEntity();
    auto market = std::make_shared<MarketInventory>();
    market->stock["fuel_hydrogen"] = 100;
    market->prices["fuel_hydrogen"] = 100.0;
    em.AddComponent<MarketInventory>(stationEntity, market);
    
    std::cout << "Initial fuel price: " << market->prices["fuel_hydrogen"] << " cr" << std::endl;
    
    // Create shortage event
    std::cout << "\nCreating SHORTAGE event for hydrogen fuel..." << std::endl;
    auto eventEntity = eventSystem.CreateEvent(
        em,
        EconomicEvent::EventType::Shortage,
        "fuel_hydrogen",
        {stationEntity},
        1.5,     // 150% price increase
        3600.0   // Lasts 1 hour
    );
    
    // Apply event
    eventSystem.ApplyEventEffects(em, eventEntity);
    
    std::cout << "Fuel price after shortage: " 
              << market->prices["fuel_hydrogen"] << " cr" << std::endl;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        NOVA ENGINE - ECONOMY & TRADING SYSTEM DEMO        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    EntityManager em;
    
    // Initialize commodity database (required once)
    CommodityDatabase::Get().Initialize();
    
    PrintSeparator();
    DemoCommodityDatabase();
    
    PrintSeparator();
    DemoMarketCreation(em);
    
    PrintSeparator();
    auto playerEntity = em.CreateEntity();
    auto stationEntity = em.GetEntitiesWith<MarketInventory>()[0];
    DemoTrading(em, playerEntity, stationEntity);
    
    PrintSeparator();
    DemoTradeRoutes(em, playerEntity);
    
    PrintSeparator();
    DemoPricing(em, stationEntity);
    
    PrintSeparator();
    DemoEconomicEvents(em);
    
    PrintSeparator();
    std::cout << "\n✓ Economy system demo completed successfully!\n" << std::endl;
    std::cout << "See ECONOMY_TRADING_SYSTEM.md for full documentation.\n" << std::endl;
    
    return 0;
}
