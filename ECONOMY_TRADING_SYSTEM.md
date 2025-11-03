# Economy & Trading System Documentation

## Overview

The Nova Engine Economy & Trading System provides a comprehensive framework for implementing dynamic markets, trading mechanics, and economic simulation in your space game. This system is inspired by games like Elite Dangerous, Star Citizen, and X4: Foundations.

## Features

### ✅ Implemented Core Features

- **Dynamic Market System**: Supply/demand pricing with automatic price adjustments
- **Commodity Types**: 27 predefined commodities across 10 categories
- **Trade Execution**: Buy/sell transactions with validation
- **Cargo Management**: Inventory system with volume and mass constraints
- **Trade Routes**: Profitable route discovery and analysis
- **Economic Zones**: Regional modifiers affecting prices and taxes
- **Market Analytics**: Price history and trend tracking
- **Autonomous Traders**: NPC traders with AI-driven behavior
- **Banking System**: Accounts, transactions, and financial tracking
- **Black Market**: Illegal trading with risk/reward mechanics
- **Contract System**: Mission generation framework
- **Economic Events**: Market-affecting events (booms, recessions, shortages)

### 🚧 Planned Features

- **Player-owned Trading Stations**: Build and manage your own markets
- **Advanced Contracts**: Courier missions, smuggling runs, trade circuits
- **Loans & Investments**: Financial instruments for players
- **Market Manipulation**: Price fixing and market warfare
- **Insurance System**: Cargo and ship insurance
- **Banking Services**: Loans, interest, credit ratings
- **Supply Chains**: Production chains between stations
- **Faction Economics**: Faction-based trade relations

## Architecture

### Component Structure

The system uses Nova Engine's ECS architecture with the following components:

#### Core Components

```cpp
// CargoHold - Entities that can carry goods
struct CargoHold {
    double maxCapacity;
    double usedCapacity;
    std::vector<CargoSlot> cargo;
    bool allowsContraband;
};

// MarketInventory - Trading stations/shops
struct MarketInventory {
    std::map<std::string, int> stock;
    std::map<std::string, double> prices;
    std::map<std::string, double> buyPrices;
    double cashReserve;
    std::string marketType;
};

// BankAccount - Financial tracking
struct BankAccount {
    double balance;
    double creditLimit;
    double debt;
    double interestRate;
};
```

#### Advanced Components

- **EconomicZone**: Regional economic modifiers
- **TradeRoute**: Established trade paths with profit calculations
- **Contract**: Trading missions and courier jobs
- **Investment**: Player investments in stations/trade routes
- **PriceHistory**: Historical price data for analytics
- **EconomicEvent**: Market events affecting prices
- **BlackMarket**: Illegal trading locations
- **TraderReputation**: NPC trader behavior tracking

### System Architecture

The economy is managed by several interconnected systems:

1. **MarketPricingSystem**: Calculates dynamic prices
2. **TradeSystem**: Executes buy/sell transactions
3. **TradeRouteSystem**: Analyzes and manages trade routes
4. **ContractSystem**: Generates and validates missions
5. **EconomicEventSystem**: Triggers market events
6. **BankingSystem**: Handles loans and interest
7. **MarketAnalyticsSystem**: Tracks market data
8. **BlackMarketSystem**: Manages illegal trading
9. **CargoManagementSystem**: Handles cargo operations

## Commodity System

### Commodity Categories

1. **Raw Materials** - Basic resources (ore, minerals)
2. **Manufactured Goods** - Processed items (electronics, machinery)
3. **Luxuries** - High-value consumer goods (wine, jewelry, art)
4. **Contraband** - Illegal goods (weapons, drugs, slaves)
5. **Fuel** - Ship fuel and energy sources
6. **Food** - Consumables for stations
7. **Technology** - Advanced tech items (software, processors)
8. **Medical** - Medical supplies and pharmaceuticals
9. **Weapons** - Armaments and munitions
10. **Data** - Information commodities

### Predefined Commodities

The system includes 27 commodities out of the box:

#### Raw Materials
- Iron Ore (50 cr)
- Copper Ore (75 cr)
- Titanium Ore (200 cr)
- Platinum Ore (500 cr)
- Rare Crystals (800 cr)

#### Manufactured Goods
- Refined Metals (150 cr)
- Electronics (300 cr)
- Machinery (450 cr)
- Hull Plates (250 cr)

#### Fuel
- Hydrogen Fuel (100 cr)
- Antimatter (2000 cr)

#### Luxuries
- Vintage Wine (350 cr)
- Jewelry (1200 cr)
- Artwork (5000 cr)

#### Contraband
- Illegal Weapons (1500 cr, danger: 5)
- Narcotics (2500 cr, danger: 4)
- Slaves (8000 cr, danger: 5)

#### Food
- Basic Food (50 cr)
- Luxury Food (200 cr)

#### Technology
- Software (400 cr)
- Quantum Processors (1800 cr)

#### Medical
- Medical Supplies (300 cr)
- Advanced Medicines (900 cr)

#### Weapons
- Small Arms (500 cr)
- Heavy Weapons (2000 cr)

#### Data
- Corporate Data (600 cr)
- Scientific Data (1000 cr)

## Trading Mechanics

### Price Calculation

Prices are dynamically calculated based on:

1. **Base Price**: Commodity's intrinsic value
2. **Supply Factor**: 
   - Low stock (< 10): +50% price
   - Medium stock (10-50): +20% price
   - High stock (200-500): -20% price
   - Very high stock (> 500): -40% price
3. **Economic Zone**: Regional modifiers
4. **Exports/Imports**: -30% for exports, +30% for imports
5. **Volatility**: Random ±5% fluctuation

### Buy Transaction Flow

```cpp
// Player wants to buy goods
TradeSystem::TradeResult result = tradeSystem.BuyCommodity(
    em,                    // EntityManager
    playerEntity,          // Buyer
    stationEntity,         // Market
    "ore_iron",           // Commodity ID
    10                     // Quantity
);

if (result.success) {
    std::cout << "Purchase successful! Total: " << result.totalCost 
              << " + Tax: " << result.tax << std::endl;
}
```

### Sell Transaction Flow

```cpp
// Player wants to sell goods
TradeSystem::TradeResult result = tradeSystem.SellCommodity(
    em,
    playerEntity,
    stationEntity,
    "ore_iron",
    10
);

if (result.success) {
    std::cout << "Sale successful! Revenue: " << result.totalCost << std::endl;
}
```

## Trade Routes

### Finding Profitable Routes

The system can automatically discover profitable trade routes:

```cpp
TradeRouteSystem routeSystem;
auto routes = routeSystem.FindProfitableRoutes(
    em,
    playerEntity,
    10  // Max routes to return
);

for (const auto& route : routes) {
    std::cout << "Route: " << route.startStation 
              << " -> " << route.endStation
              << " | Commodity: " << route.commodityId
              << " | Profit: " << route.profitMargin
              << " | Risk: " << (route.risk * 100) << "%" << std::endl;
}
```

### Route Properties

- **Profit Margin**: Expected profit from the route
- **Risk**: Danger level (0-1, affects by pirates, distance)
- **Distance**: Travel distance
- **Estimated Time**: Journey duration
- **Popularity**: How many traders use this route

## Market Types

### General Markets
- Diverse inventory
- Standard pricing
- All commodity types
- Good for general trading

### Industrial Markets
- Focus on raw materials and manufactured goods
- Higher stock levels
- Good for bulk trading
- Exports: Refined metals, machinery
- Imports: Luxury goods, food

### Luxury Markets
- High-end goods only
- Low stock, high prices
- Premium products
- Exports: Artwork, jewelry
- Imports: Raw materials

### Military Markets
- Weapons and fuel focus
- High security
- Restricted access may apply
- Exports: Weapons, ammunition
- Imports: Electronics, food

## Economic Events

Events that affect market prices:

1. **Boom**: Economic prosperity (+20% to all prices)
2. **Recession**: Economic downturn (-20% to all prices)
3. **Shortage**: Supply shortage (+50% to specific commodity)
4. **Surplus**: Oversupply (-30% to specific commodity)
5. **Blockade**: Trade restricted (+100% to affected goods)
6. **Discovery**: New resource found (-40% to related commodity)
7. **Disaster**: Production loss (+80% to affected goods)
8. **War Demand**: Military consumption spike (+60% to weapons/fuel)
9. **Tech Breakthrough**: New production method (-25% to manufactured goods)
10. **Scandal**: Price manipulation (±random% to commodity)

## Integration Guide

### Setting Up a Trading Station

```cpp
// In your game initialization
auto stationEntity = em.CreateEntity();
auto station = std::make_unique<TradeStation>();
station->Initialize();  // Auto-loads config and sets up market

// Access market data
auto* market = station->GetMarket();
bool hasIron = station->HasCommodity("ore_iron", 10);
double ironPrice = station->GetPrice("ore_iron");
```

### Creating an NPC Trader

```cpp
auto traderEntity = em.CreateEntity();
auto trader = std::make_unique<Trader>();
trader->Initialize();  // Auto-loads config and sets up cargo/bank

// Trader will automatically:
// 1. Find profitable routes
// 2. Navigate to markets
// 3. Buy low, sell high
// 4. Complete contracts
```

### Player Trading UI

```cpp
// Example UI pseudo-code
void ShowTradeInterface(uint32_t stationEntity) {
    auto* market = em.GetComponent<MarketInventory>(stationEntity);
    
    for (const auto& [commodityId, stock] : market->stock) {
        auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
        double price = market->prices[commodityId];
        
        ImGui::Text("%s", commodity->name.c_str());
        ImGui::Text("Stock: %d", stock);
        ImGui::Text("Price: %.2f cr", price);
        
        if (ImGui::Button("Buy")) {
            tradeSystem.BuyCommodity(em, playerEntity, stationEntity, 
                                    commodityId, 1);
        }
        
        if (ImGui::Button("Sell")) {
            tradeSystem.SellCommodity(em, playerEntity, stationEntity, 
                                     commodityId, 1);
        }
    }
}
```

## Black Market System

### Accessing Black Markets

Black markets are hidden markets for contraband:

```cpp
BlackMarketSystem blackMarketSystem;

// Try to access (may fail based on discovery risk)
bool accessed = blackMarketSystem.AttemptBlackMarketAccess(
    em,
    playerEntity,
    blackMarketEntity
);

if (accessed) {
    // Buy contraband at markup prices
    auto result = blackMarketSystem.BuyContraband(
        em, playerEntity, blackMarketEntity,
        "contraband_weapons", 5
    );
    
    if (result.success) {
        std::cout << "Purchased illegal weapons!" << std::endl;
    }
}

// Check for law enforcement detection
if (blackMarketSystem.CheckDetection(em, playerEntity)) {
    std::cout << "Caught by authorities!" << std::endl;
    blackMarketSystem.ApplyDetectionPenalty(em, playerEntity);
}
```

### Contraband Properties

- **Higher Prices**: 1.5x markup on black market
- **Discovery Risk**: 30% chance of detection per transaction
- **Heat Level**: Accumulates with transactions, decays over time
- **Penalties**: Fines, confiscation, reputation loss

## Configuration Files

### Trade Station Configuration

Create `assets/actors/world/trade_station.json`:

```json
{
    "name": "Frontier Trading Post",
    "marketType": "general",
    "cashReserve": 250000,
    "faction": "independent",
    "hasBlackMarket": true,
    "position": {
        "x": 100,
        "y": 0,
        "z": 200
    }
}
```

### Trader Ship Configuration

Create `assets/actors/ships/trader.json`:

```json
{
    "name": "Merchant Hauler",
    "cargoCapacity": 150,
    "credits": 10000,
    "faction": "traders_guild",
    "canCarryContraband": false,
    "specialization": [
        "ore_iron",
        "fuel_hydrogen",
        "food_basic"
    ]
}
```

## Performance Considerations

### Optimization Tips

1. **Market Updates**: Markets update prices every 60 seconds, not every frame
2. **Route Calculations**: Trade routes refresh every 5 minutes
3. **Event Generation**: Economic events occur 30min - 2hr apart
4. **Price History**: Limited to 100 entries per commodity
5. **Cargo Consolidation**: Use `OptimizeCargo()` to merge duplicate slots

### Memory Usage

- Each MarketInventory: ~2-5 KB (depending on stock variety)
- Each CargoHold: ~1-3 KB (depending on cargo)
- Each PriceHistory: ~10-20 KB (100 price points × N commodities)
- Commodity Database: ~5 KB (27 commodities)

## Advanced Usage

### Custom Commodities

Add your own commodities to the database:

```cpp
void InitializeCustomCommodities() {
    auto& db = CommodityDatabase::Get();
    
    db.AddCommodity({
        "exotic_spice",           // ID
        "Exotic Spice",          // Name
        CommodityType::Luxuries, // Type
        1500.0,                  // Base price
        0.3,                     // Volume
        0.2,                     // Mass
        true,                    // Is legal
        2,                       // Danger level
        "Rare spice from outer rim planets"
    });
}
```

### Custom Market Types

Extend market types for specialized stations:

```cpp
void InitializeResearchStation(MarketInventory& market) {
    // Research stations buy data, sell tech
    market.stock["data_scientific"] = 0;      // Buy data
    market.stock["data_corporate"] = 0;
    market.stock["tech_processors"] = 100;    // Sell tech
    market.stock["tech_software"] = 150;
    market.marketType = "research";
    market.priceVolatility = 0.05;  // More stable prices
}
```

### Economic Simulation

Create complex economic scenarios:

```cpp
void SimulateWartime(EntityManager& em) {
    EconomicEventSystem eventSystem;
    
    // Create war demand event
    std::vector<uint32_t> militaryStations = GetMilitaryStations(em);
    eventSystem.CreateEvent(
        em,
        EconomicEvent::EventType::WarDemand,
        "weapons_heavy",
        militaryStations,
        2.0,    // 200% price increase
        86400.0 // Lasts 24 hours
    );
    
    // Fuel shortage due to blockade
    auto* fuelEvent = eventSystem.CreateEvent(
        em,
        EconomicEvent::EventType::Shortage,
        "fuel_hydrogen",
        GetAllStations(em),
        1.5,
        172800.0 // Lasts 48 hours
    );
}
```

## Testing & Debugging

### Market Inspection

```cpp
void DebugMarket(EntityManager& em, uint32_t marketEntity) {
    auto* market = em.GetComponent<MarketInventory>(marketEntity);
    
    std::cout << "=== Market Debug ===" << std::endl;
    std::cout << "Type: " << market->marketType << std::endl;
    std::cout << "Cash Reserve: " << market->cashReserve << std::endl;
    std::cout << "Stock:" << std::endl;
    
    for (const auto& [id, qty] : market->stock) {
        auto price = market->prices[id];
        std::cout << "  " << id << ": " << qty 
                  << " @ " << price << " cr" << std::endl;
    }
}
```

### Trade Route Analysis

```cpp
void AnalyzeRoutes(EntityManager& em, uint32_t playerEntity) {
    TradeRouteSystem routeSystem;
    auto routes = routeSystem.FindProfitableRoutes(em, playerEntity, 20);
    
    std::cout << "=== Top Trade Routes ===" << std::endl;
    for (size_t i = 0; i < routes.size(); ++i) {
        const auto& route = routes[i];
        std::cout << i+1 << ". " << route.commodityId 
                  << " | Profit: " << route.profitMargin
                  << " | Risk: " << (route.risk * 100) << "%" << std::endl;
    }
}
```

## Future Enhancements

### Roadmap

1. **Q1 2026**: Player-owned stations
2. **Q2 2026**: Advanced contract system with branching missions
3. **Q3 2026**: Banking system with loans and investments
4. **Q4 2026**: Supply chain simulation with production
5. **Q1 2027**: Market manipulation and economic warfare
6. **Q2 2027**: Insurance and risk management systems

### Community Contributions

We welcome contributions to the economy system! Areas of interest:

- Additional commodity types
- New market types
- Economic event scenarios
- AI trader behaviors
- Trade interface improvements

## Troubleshooting

### Common Issues

**Q: Prices never change**
- Ensure MarketPricingSystem is being updated
- Check that price volatility > 0
- Verify economic events are triggering

**Q: Traders won't buy/sell**
- Check BankAccount balance
- Verify CargoHold has space
- Ensure commodity exists in market stock

**Q: Routes show no profit**
- Markets may have similar prices (adjust zone modifiers)
- Check if stock levels affect pricing
- Ensure distance penalties are applied

**Q: Black market always detected**
- Lower discovery risk in BlackMarket component
- Reduce heat level accumulation
- Implement reputation-based detection

## License

The Economy & Trading System is part of Nova Engine and follows the same license terms.

## Credits

- Inspired by Elite Dangerous, Star Citizen, and X4: Foundations
- Designed for Nova Engine's ECS architecture
- Community feedback and contributions welcome

---

For more information, see:
- [Nova Engine Documentation](README.md)
- [ECS System Guide](engine/ecs/README.md)
- [Game Design Document](GAME_DESIGN_DOCUMENT.md)
