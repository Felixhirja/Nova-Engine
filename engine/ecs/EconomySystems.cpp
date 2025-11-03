#include "EconomySystems.h"
#include <iostream>
#include <algorithm>
#include <numeric>

// ============================================================================
// CommodityDatabase Implementation
// ============================================================================

void CommodityDatabase::Initialize() {
    // Raw Materials
    AddCommodity({"ore_iron", "Iron Ore", CommodityType::RawMaterials, 50.0, 1.0, 2.0, true, 0, "Common metallic ore"});
    AddCommodity({"ore_copper", "Copper Ore", CommodityType::RawMaterials, 75.0, 1.0, 2.0, true, 0, "Conductive metal ore"});
    AddCommodity({"ore_titanium", "Titanium Ore", CommodityType::RawMaterials, 200.0, 1.0, 1.5, true, 0, "Lightweight strong metal"});
    AddCommodity({"ore_platinum", "Platinum Ore", CommodityType::RawMaterials, 500.0, 1.0, 3.0, true, 0, "Rare precious metal"});
    AddCommodity({"crystal_rare", "Rare Crystals", CommodityType::RawMaterials, 800.0, 0.5, 0.5, true, 0, "Valuable crystalline formations"});
    
    // Manufactured Goods
    AddCommodity({"metal_refined", "Refined Metals", CommodityType::Manufactured, 150.0, 1.0, 2.0, true, 0, "Processed metal alloys"});
    AddCommodity({"components_electronics", "Electronics", CommodityType::Manufactured, 300.0, 0.5, 0.3, true, 0, "Electronic components"});
    AddCommodity({"components_machinery", "Machinery", CommodityType::Manufactured, 450.0, 2.0, 3.0, true, 0, "Industrial machinery"});
    AddCommodity({"hull_plates", "Hull Plates", CommodityType::Manufactured, 250.0, 3.0, 5.0, true, 0, "Reinforced ship plating"});
    
    // Fuel
    AddCommodity({"fuel_hydrogen", "Hydrogen Fuel", CommodityType::Fuel, 100.0, 2.0, 1.0, true, 0, "Common ship fuel"});
    AddCommodity({"fuel_antimatter", "Antimatter", CommodityType::Fuel, 2000.0, 0.5, 0.1, true, 3, "Exotic high-energy fuel"});
    
    // Luxuries
    AddCommodity({"luxury_wine", "Vintage Wine", CommodityType::Luxuries, 350.0, 0.5, 0.5, true, 0, "Aged alcoholic beverage"});
    AddCommodity({"luxury_jewelry", "Jewelry", CommodityType::Luxuries, 1200.0, 0.1, 0.1, true, 1, "Precious gems and metals"});
    AddCommodity({"luxury_artwork", "Artwork", CommodityType::Luxuries, 5000.0, 1.0, 0.5, true, 1, "Cultural artifacts"});
    
    // Contraband
    AddCommodity({"contraband_weapons", "Illegal Weapons", CommodityType::Contraband, 1500.0, 1.0, 2.0, false, 5, "Prohibited armaments"});
    AddCommodity({"contraband_drugs", "Narcotics", CommodityType::Contraband, 2500.0, 0.5, 0.3, false, 4, "Illegal substances"});
    AddCommodity({"contraband_slaves", "Slaves", CommodityType::Contraband, 8000.0, 2.0, 1.5, false, 5, "Trafficked persons"});
    
    // Food
    AddCommodity({"food_basic", "Basic Food", CommodityType::Food, 50.0, 1.0, 1.0, true, 0, "Standard rations"});
    AddCommodity({"food_luxury", "Luxury Food", CommodityType::Food, 200.0, 0.5, 0.5, true, 0, "Gourmet cuisine"});
    
    // Technology
    AddCommodity({"tech_software", "Software", CommodityType::Technology, 400.0, 0.1, 0.0, true, 0, "Digital programs"});
    AddCommodity({"tech_processors", "Quantum Processors", CommodityType::Technology, 1800.0, 0.2, 0.1, true, 1, "Advanced computing hardware"});
    
    // Medical
    AddCommodity({"medical_supplies", "Medical Supplies", CommodityType::Medical, 300.0, 0.5, 0.5, true, 0, "Basic medical equipment"});
    AddCommodity({"medical_advanced", "Advanced Medicines", CommodityType::Medical, 900.0, 0.3, 0.2, true, 1, "Cutting-edge pharmaceuticals"});
    
    // Weapons
    AddCommodity({"weapons_small", "Small Arms", CommodityType::Weapons, 500.0, 0.5, 1.0, true, 2, "Personal defense weapons"});
    AddCommodity({"weapons_heavy", "Heavy Weapons", CommodityType::Weapons, 2000.0, 2.0, 5.0, true, 3, "Military-grade armaments"});
    
    // Data
    AddCommodity({"data_corporate", "Corporate Data", CommodityType::Data, 600.0, 0.0, 0.0, true, 1, "Business intelligence"});
    AddCommodity({"data_scientific", "Scientific Data", CommodityType::Data, 1000.0, 0.0, 0.0, true, 0, "Research findings"});
    
    std::cout << "[CommodityDatabase] Initialized with " << commodities_.size() << " commodities" << std::endl;
}

void CommodityDatabase::AddCommodity(const CommodityItem& item) {
    commodities_[item.id] = item;
}

const CommodityItem* CommodityDatabase::GetCommodity(const std::string& id) const {
    auto it = commodities_.find(id);
    return (it != commodities_.end()) ? &it->second : nullptr;
}

std::vector<std::string> CommodityDatabase::GetAllCommodityIds() const {
    std::vector<std::string> ids;
    ids.reserve(commodities_.size());
    for (const auto& pair : commodities_) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<const CommodityItem*> CommodityDatabase::GetCommoditiesByType(CommodityType type) const {
    std::vector<const CommodityItem*> result;
    for (const auto& pair : commodities_) {
        if (pair.second.type == type) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

// ============================================================================
// MarketPricingSystem Implementation
// ============================================================================

void MarketPricingSystem::Update(EntityManager& em, double deltaTime) {
    updateAccumulator_ += deltaTime;
    
    if (updateAccumulator_ >= updateInterval_) {
        updateAccumulator_ = 0.0;
        SimulatePriceVolatility(em);
    }
}

double MarketPricingSystem::CalculatePrice(const std::string& commodityId,
                                          const MarketInventory& market,
                                          const EconomicZone* zone) const {
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) return 0.0;
    
    double basePrice = commodity->basePrice;
    
    // Supply/demand adjustment
    auto stockIt = market.stock.find(commodityId);
    int stock = (stockIt != market.stock.end()) ? stockIt->second : 0;
    
    // Low stock = higher prices
    double supplyFactor = 1.0;
    if (stock < 10) {
        supplyFactor = 1.5;
    } else if (stock < 50) {
        supplyFactor = 1.2;
    } else if (stock > 200) {
        supplyFactor = 0.8;
    } else if (stock > 500) {
        supplyFactor = 0.6;
    }
    
    // Economic zone modifiers
    double zoneFactor = 1.0;
    if (zone) {
        zoneFactor *= zone->economicStrength;
        
        // Check if this commodity is an import/export
        bool isExport = std::find(zone->primaryExports.begin(),
                                 zone->primaryExports.end(),
                                 commodityId) != zone->primaryExports.end();
        bool isImport = std::find(zone->primaryImports.begin(),
                                 zone->primaryImports.end(),
                                 commodityId) != zone->primaryImports.end();
        
        if (isExport) zoneFactor *= 0.7;  // Cheap to buy exports
        if (isImport) zoneFactor *= 1.3;  // Expensive to buy imports
    }
    
    return basePrice * supplyFactor * zoneFactor;
}

void MarketPricingSystem::UpdateMarketPrices(EntityManager& em, uint32_t marketEntity) {
    auto* market = em.GetComponent<MarketInventory>(marketEntity);
    auto* zone = em.GetComponent<EconomicZone>(marketEntity);
    
    if (!market) return;
    
    for (auto& [commodityId, stock] : market->stock) {
        double newPrice = CalculatePrice(commodityId, *market, zone);
        market->prices[commodityId] = newPrice;
        market->buyPrices[commodityId] = newPrice * 0.8;  // Buy back at 80% of sell price
    }
}

void MarketPricingSystem::SimulatePriceVolatility(EntityManager& em) {
    std::uniform_real_distribution<double> dist(-0.05, 0.05);
    
    em.ForEach<MarketInventory>([&](EntityHandle entity, MarketInventory& market) {
        
        for (auto& [commodityId, price] : market.prices) {
            double variance = dist(rng_) * market.priceVolatility;
            price *= (1.0 + variance);
            
            // Ensure price doesn't go too extreme
            auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
            if (commodity) {
                price = std::max(price, commodity->basePrice * 0.3);
                price = std::min(price, commodity->basePrice * 3.0);
            }
        }
        
        UpdateMarketPrices(em, entity);
    });
}

// ============================================================================
// TradeSystem Implementation
// ============================================================================

void TradeSystem::Update(EntityManager& em, double deltaTime) {
    // Passive system - mainly called via explicit trade calls
    (void)em;
    (void)deltaTime;
}

TradeSystem::TradeResult TradeSystem::BuyCommodity(EntityManager& em,
                                                   uint32_t buyerEntity,
                                                   uint32_t marketEntity,
                                                   const std::string& commodityId,
                                                   int quantity) {
    TradeResult result;
    
    auto* buyerAccount = em.GetComponent<BankAccount>(EntityHandle(buyerEntity, 0));
    auto* buyerTradeCargo = em.GetComponent<TradeCargo>(EntityHandle(buyerEntity, 0));
    auto* market = em.GetComponent<MarketInventory>(EntityHandle(marketEntity, 0));
    auto* zone = em.GetComponent<EconomicZone>(EntityHandle(marketEntity, 0));
    
    if (!buyerAccount || !buyerCargo || !market) {
        result.message = "Missing required components";
        return result;
    }
    
    // Check market stock
    auto stockIt = market->stock.find(commodityId);
    if (stockIt == market->stock.end() || stockIt->second < quantity) {
        result.message = "Insufficient stock at market";
        return result;
    }
    
    // Check commodity validity
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) {
        result.message = "Unknown commodity";
        return result;
    }
    
    // Check cargo space
    double requiredSpace = commodity->volume * quantity;
    if (!buyerCargo->HasSpace(requiredSpace)) {
        result.message = "Insufficient cargo space";
        return result;
    }
    
    // Calculate cost
    auto priceIt = market->prices.find(commodityId);
    double unitPrice = (priceIt != market->prices.end()) ? priceIt->second : commodity->basePrice;
    result.totalCost = unitPrice * quantity;
    result.tax = (zone ? zone->taxRate : 0.05) * result.totalCost;
    double finalCost = result.totalCost + result.tax;
    
    // Check funds
    if (buyerAccount->balance < finalCost) {
        result.message = "Insufficient funds";
        return result;
    }
    
    // Execute transaction
    buyerAccount->balance -= finalCost;
    market->stock[commodityId] -= quantity;
    market->cashReserve += result.totalCost;
    
    // Add to cargo
    CargoSlot slot;
    slot.commodityId = commodityId;
    slot.quantity = quantity;
    slot.purchasePrice = unitPrice;
    slot.origin = "Market";  // TODO: Get actual station name
    buyerCargo->cargo.push_back(slot);
    buyerCargo->usedCapacity += requiredSpace;
    
    result.success = true;
    result.message = "Purchase successful";
    
    std::cout << "[TradeSystem] Bought " << quantity << "x " << commodityId 
              << " for " << finalCost << " credits" << std::endl;
    
    return result;
}

TradeSystem::TradeResult TradeSystem::SellCommodity(EntityManager& em,
                                                    uint32_t sellerEntity,
                                                    uint32_t marketEntity,
                                                    const std::string& commodityId,
                                                    int quantity) {
    TradeResult result;
    
    auto* sellerAccount = em.GetComponent<BankAccount>(sellerEntity);
    auto* sellerCargo = em.GetComponent<CargoHold>(sellerEntity);
    auto* market = em.GetComponent<MarketInventory>(marketEntity);
    auto* zone = em.GetComponent<EconomicZone>(marketEntity);
    
    if (!sellerAccount || !sellerCargo || !market) {
        result.message = "Missing required components";
        return result;
    }
    
    // Check if seller has the commodity
    int availableQty = 0;
    for (const auto& slot : sellerCargo->cargo) {
        if (slot.commodityId == commodityId) {
            availableQty += slot.quantity;
        }
    }
    
    if (availableQty < quantity) {
        result.message = "Insufficient cargo to sell";
        return result;
    }
    
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) {
        result.message = "Unknown commodity";
        return result;
    }
    
    // Calculate sell price (buy-back price)
    auto buyPriceIt = market->buyPrices.find(commodityId);
    double unitPrice = (buyPriceIt != market->buyPrices.end()) ? buyPriceIt->second : commodity->basePrice * 0.8;
    result.totalCost = unitPrice * quantity;
    result.tax = (zone ? zone->taxRate : 0.05) * result.totalCost;
    double finalPayment = result.totalCost - result.tax;
    
    // Check market can afford it
    if (market->cashReserve < finalPayment) {
        result.message = "Market cannot afford purchase";
        return result;
    }
    
    // Execute transaction
    sellerAccount->balance += finalPayment;
    market->stock[commodityId] += quantity;
    market->cashReserve -= finalPayment;
    
    // Remove from cargo
    int remaining = quantity;
    double freedSpace = commodity->volume * quantity;
    auto it = sellerCargo->cargo.begin();
    while (it != sellerCargo->cargo.end() && remaining > 0) {
        if (it->commodityId == commodityId) {
            if (it->quantity <= remaining) {
                remaining -= it->quantity;
                it = sellerCargo->cargo.erase(it);
            } else {
                it->quantity -= remaining;
                remaining = 0;
                ++it;
            }
        } else {
            ++it;
        }
    }
    sellerCargo->usedCapacity -= freedSpace;
    
    result.success = true;
    result.message = "Sale successful";
    
    std::cout << "[TradeSystem] Sold " << quantity << "x " << commodityId 
              << " for " << finalPayment << " credits" << std::endl;
    
    return result;
}

bool TradeSystem::TransferCargo(EntityManager& em,
                               uint32_t fromEntity,
                               uint32_t toEntity,
                               const std::string& commodityId,
                               int quantity) {
    auto* fromCargo = em.GetComponent<CargoHold>(fromEntity);
    auto* toCargo = em.GetComponent<CargoHold>(toEntity);
    
    if (!fromCargo || !toCargo) return false;
    
    // Check source has cargo
    int available = 0;
    for (const auto& slot : fromCargo->cargo) {
        if (slot.commodityId == commodityId) {
            available += slot.quantity;
        }
    }
    
    if (available < quantity) return false;
    
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) return false;
    
    double requiredSpace = commodity->volume * quantity;
    if (!toCargo->HasSpace(requiredSpace)) return false;
    
    // Transfer
    int remaining = quantity;
    auto it = fromCargo->cargo.begin();
    while (it != fromCargo->cargo.end() && remaining > 0) {
        if (it->commodityId == commodityId) {
            int transferQty = std::min(it->quantity, remaining);
            
            CargoSlot newSlot = *it;
            newSlot.quantity = transferQty;
            toCargo->cargo.push_back(newSlot);
            
            it->quantity -= transferQty;
            remaining -= transferQty;
            
            if (it->quantity <= 0) {
                it = fromCargo->cargo.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    fromCargo->usedCapacity -= requiredSpace;
    toCargo->usedCapacity += requiredSpace;
    
    return true;
}

double TradeSystem::CalculateRouteProfit(EntityManager& em,
                                        uint32_t startMarket,
                                        uint32_t endMarket,
                                        const std::string& commodityId,
                                        int quantity) const {
    auto* market1 = em.GetComponent<MarketInventory>(startMarket);
    auto* market2 = em.GetComponent<MarketInventory>(endMarket);
    
    if (!market1 || !market2) return 0.0;
    
    auto buyPriceIt = market1->prices.find(commodityId);
    auto sellPriceIt = market2->buyPrices.find(commodityId);
    
    if (buyPriceIt == market1->prices.end() || sellPriceIt == market2->buyPrices.end()) {
        return 0.0;
    }
    
    double buyCost = buyPriceIt->second * quantity;
    double sellRevenue = sellPriceIt->second * quantity;
    
    return sellRevenue - buyCost;
}

// ============================================================================
// TradeRouteSystem Implementation
// ============================================================================

void TradeRouteSystem::Update(EntityManager& em, double deltaTime) {
    routeUpdateTimer_ += deltaTime;
    
    if (routeUpdateTimer_ >= routeUpdateInterval_) {
        routeUpdateTimer_ = 0.0;
        RefreshTradeRoutes(em);
    }
}

std::vector<TradeRoute> TradeRouteSystem::FindProfitableRoutes(EntityManager& em,
                                                               uint32_t playerEntity,
                                                               int maxRoutes) {
    std::vector<TradeRoute> routes;
    TradeSystem tradeSystem;
    
    auto markets = em.GetEntitiesWith<MarketInventory>();
    auto commodities = CommodityDatabase::Get().GetAllCommodityIds();
    
    for (size_t i = 0; i < markets.size(); ++i) {
        for (size_t j = i + 1; j < markets.size(); ++j) {
            for (const auto& commodityId : commodities) {
                double profit = tradeSystem.CalculateRouteProfit(em, markets[i], markets[j], commodityId, 10);
                
                if (profit > 0) {
                    TradeRoute route;
                    route.startStation = markets[i];
                    route.endStation = markets[j];
                    route.commodityId = commodityId;
                    route.profitMargin = profit;
                    route.risk = CalculateRouteRisk(em, markets[i], markets[j]);
                    route.isActive = true;
                    
                    routes.push_back(route);
                }
            }
        }
    }
    
    // Sort by profit and return top N
    std::sort(routes.begin(), routes.end(),
             [](const TradeRoute& a, const TradeRoute& b) {
                 return a.profitMargin > b.profitMargin;
             });
    
    if (routes.size() > static_cast<size_t>(maxRoutes)) {
        routes.resize(maxRoutes);
    }
    
    return routes;
}

double TradeRouteSystem::CalculateRouteRisk(EntityManager& em,
                                           uint32_t startStation,
                                           uint32_t endStation) const {
    // Base risk
    double risk = 0.1;
    
    // TODO: Calculate based on:
    // - Distance between stations
    // - Security level of regions
    // - Recent piracy activity
    // - Faction relations
    
    (void)em;
    (void)startStation;
    (void)endStation;
    
    return risk;
}

void TradeRouteSystem::RefreshTradeRoutes(EntityManager& em) {
    std::cout << "[TradeRouteSystem] Refreshing trade routes..." << std::endl;
    
    // Update existing routes
    for (auto entity : em.GetEntitiesWith<TradeRoute>()) {
        auto* route = em.GetComponent<TradeRoute>(entity);
        if (!route) continue;
        
        TradeSystem tradeSystem;
        route->profitMargin = tradeSystem.CalculateRouteProfit(em,
                                                              route->startStation,
                                                              route->endStation,
                                                              route->commodityId,
                                                              10);
        
        route->risk = CalculateRouteRisk(em, route->startStation, route->endStation);
        route->isActive = route->profitMargin > 0;
    }
}

// ============================================================================
// Additional system stubs for compilation
// ============================================================================

void ContractSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

void ContractSystem::GenerateContracts(EntityManager& em, uint32_t stationEntity, int count) {
    (void)em;
    (void)stationEntity;
    (void)count;
}

bool ContractSystem::CheckContractCompletion(EntityManager& em, uint32_t contractEntity) {
    (void)em;
    (void)contractEntity;
    return false;
}

void ContractSystem::CompleteContract(EntityManager& em, uint32_t contractEntity, uint32_t playerEntity) {
    (void)em;
    (void)contractEntity;
    (void)playerEntity;
}

Contract ContractSystem::CreateContract(EntityManager& em, Contract::Type type, uint32_t stationEntity) const {
    (void)em;
    (void)type;
    (void)stationEntity;
    return Contract();
}

void EconomicEventSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

void EconomicEventSystem::TriggerRandomEvent(EntityManager& em) {
    (void)em;
}

uint32_t EconomicEventSystem::CreateEvent(EntityManager& em, EconomicEvent::EventType type,
                                         const std::string& commodityId,
                                         const std::vector<uint32_t>& affectedStations,
                                         double magnitude, double duration) {
    (void)em;
    (void)type;
    (void)commodityId;
    (void)affectedStations;
    (void)magnitude;
    (void)duration;
    return 0;
}

void EconomicEventSystem::ApplyEventEffects(EntityManager& em, uint32_t eventEntity) {
    (void)em;
    (void)eventEntity;
}

void BankingSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

bool BankingSystem::ApplyForLoan(EntityManager& em, uint32_t applicantEntity,
                                double amount, double interestRate, double term) {
    (void)em;
    (void)applicantEntity;
    (void)amount;
    (void)interestRate;
    (void)term;
    return false;
}

bool BankingSystem::MakeLoanPayment(EntityManager& em, uint32_t borrowerEntity, double amount) {
    (void)em;
    (void)borrowerEntity;
    (void)amount;
    return false;
}

uint32_t BankingSystem::CreateInvestment(EntityManager& em, uint32_t investorEntity,
                                        const std::string& investmentType, uint32_t targetEntity,
                                        double amount, double expectedReturn, double maturityTime) {
    (void)em;
    (void)investorEntity;
    (void)investmentType;
    (void)targetEntity;
    (void)amount;
    (void)expectedReturn;
    (void)maturityTime;
    return 0;
}

void BankingSystem::ProcessInvestments(EntityManager& em) {
    (void)em;
}

void MarketAnalyticsSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

void MarketAnalyticsSystem::RecordTransaction(EntityManager& em, const std::string& commodityId,
                                            double price, int quantity, uint32_t marketEntity) {
    (void)em;
    (void)commodityId;
    (void)price;
    (void)quantity;
    (void)marketEntity;
}

std::string MarketAnalyticsSystem::GenerateMarketReport(EntityManager& em, uint32_t marketEntity) const {
    (void)em;
    (void)marketEntity;
    return "Market Report";
}

double MarketAnalyticsSystem::ForecastPrice(EntityManager& em, const std::string& commodityId,
                                           uint32_t marketEntity, double hoursAhead) const {
    (void)em;
    (void)commodityId;
    (void)marketEntity;
    (void)hoursAhead;
    return 0.0;
}

void MarketAnalyticsSystem::UpdateStatistics(EntityManager& em) {
    (void)em;
}

void BlackMarketSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

bool BlackMarketSystem::AttemptBlackMarketAccess(EntityManager& em, uint32_t playerEntity,
                                                 uint32_t blackMarketEntity) {
    (void)em;
    (void)playerEntity;
    (void)blackMarketEntity;
    return false;
}

TradeSystem::TradeResult BlackMarketSystem::BuyContraband(EntityManager& em, uint32_t buyerEntity,
                                                         uint32_t blackMarketEntity,
                                                         const std::string& commodityId,
                                                         int quantity) {
    (void)em;
    (void)buyerEntity;
    (void)blackMarketEntity;
    (void)commodityId;
    (void)quantity;
    return TradeSystem::TradeResult();
}

bool BlackMarketSystem::CheckDetection(EntityManager& em, uint32_t playerEntity) {
    (void)em;
    (void)playerEntity;
    return false;
}

void BlackMarketSystem::ApplyDetectionPenalty(EntityManager& em, uint32_t playerEntity) {
    (void)em;
    (void)playerEntity;
}

void CargoManagementSystem::Update(EntityManager& em, double deltaTime) {
    (void)em;
    (void)deltaTime;
}

bool CargoManagementSystem::AddCargo(CargoHold& hold, const std::string& commodityId,
                                    int quantity, double price, const std::string& origin) {
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) return false;
    
    double requiredSpace = commodity->volume * quantity;
    if (!hold.HasSpace(requiredSpace)) return false;
    
    CargoSlot slot;
    slot.commodityId = commodityId;
    slot.quantity = quantity;
    slot.purchasePrice = price;
    slot.origin = origin;
    hold.cargo.push_back(slot);
    hold.usedCapacity += requiredSpace;
    
    return true;
}

int CargoManagementSystem::RemoveCargo(CargoHold& hold, const std::string& commodityId, int quantity) {
    int removed = 0;
    auto* commodity = CommodityDatabase::Get().GetCommodity(commodityId);
    if (!commodity) return 0;
    
    auto it = hold.cargo.begin();
    while (it != hold.cargo.end() && removed < quantity) {
        if (it->commodityId == commodityId) {
            int toRemove = std::min(it->quantity, quantity - removed);
            it->quantity -= toRemove;
            removed += toRemove;
            hold.usedCapacity -= commodity->volume * toRemove;
            
            if (it->quantity <= 0) {
                it = hold.cargo.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    return removed;
}

int CargoManagementSystem::GetCargoQuantity(const CargoHold& hold, const std::string& commodityId) const {
    int total = 0;
    for (const auto& slot : hold.cargo) {
        if (slot.commodityId == commodityId) {
            total += slot.quantity;
        }
    }
    return total;
}

double CargoManagementSystem::CalculateCargoValue(const CargoHold& hold, EntityManager& em,
                                                  uint32_t marketEntity) const {
    auto* market = em.GetComponent<MarketInventory>(marketEntity);
    double totalValue = 0.0;
    
    for (const auto& slot : hold.cargo) {
        double price = slot.purchasePrice;
        
        if (market) {
            auto priceIt = market->buyPrices.find(slot.commodityId);
            if (priceIt != market->buyPrices.end()) {
                price = priceIt->second;
            }
        }
        
        totalValue += price * slot.quantity;
    }
    
    return totalValue;
}

void CargoManagementSystem::OptimizeCargo(CargoHold& hold) {
    // Consolidate same commodities
    std::map<std::string, std::vector<CargoSlot*>> grouped;
    
    for (auto& slot : hold.cargo) {
        grouped[slot.commodityId].push_back(&slot);
    }
    
    // Merge slots with same commodity
    for (auto& [commodityId, slots] : grouped) {
        if (slots.size() <= 1) continue;
        
        // Keep first slot, merge others into it
        CargoSlot* primary = slots[0];
        for (size_t i = 1; i < slots.size(); ++i) {
            primary->quantity += slots[i]->quantity;
            slots[i]->quantity = 0;  // Mark for removal
        }
    }
    
    // Remove empty slots
    hold.cargo.erase(
        std::remove_if(hold.cargo.begin(), hold.cargo.end(),
                      [](const CargoSlot& slot) { return slot.quantity <= 0; }),
        hold.cargo.end()
    );
}
