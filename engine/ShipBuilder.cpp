#include "ShipBuilder.h"
#include "SimpleJson.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace ShipBuilding {

ShipBuilder::ShipBuilder() {
    std::cout << "[ShipBuilder] Initializing ship building system..." << std::endl;
}

ShipBuilder::~ShipBuilder() = default;

// === Core Build Functions ===

std::shared_ptr<ShipLoadout> ShipBuilder::CreateShip(const std::string& hullId) {
    auto it = hullCatalog_.find(hullId);
    if (it == hullCatalog_.end()) {
        std::cerr << "[ShipBuilder] Hull not found: " << hullId << std::endl;
        return nullptr;
    }
    
    auto ship = std::make_shared<ShipLoadout>();
    ship->id = "ship_" + std::to_string(rand());  // Generate unique ID
    ship->hull = it->second;
    ship->name = it->second->name;
    ship->customName = "";
    
    std::cout << "[ShipBuilder] Created ship from hull: " << hullId << std::endl;
    return ship;
}

bool ShipBuilder::InstallComponent(ShipLoadout& ship, 
                                  const std::string& hardpointId,
                                  const std::string& componentId) {
    // Find hardpoint
    Hardpoint* hardpoint = nullptr;
    for (auto& hp : ship.hull->hardpoints) {
        if (hp.id == hardpointId) {
            hardpoint = &hp;
            break;
        }
    }
    
    if (!hardpoint) {
        std::cerr << "[ShipBuilder] Hardpoint not found: " << hardpointId << std::endl;
        return false;
    }
    
    if (hardpoint->occupied) {
        std::cerr << "[ShipBuilder] Hardpoint already occupied: " << hardpointId << std::endl;
        return false;
    }
    
    // Find component
    auto compIt = componentCatalog_.find(componentId);
    if (compIt == componentCatalog_.end()) {
        std::cerr << "[ShipBuilder] Component not found: " << componentId << std::endl;
        return false;
    }
    
    auto component = compIt->second;
    
    // Check compatibility
    if (!CheckHardpointCompatibility(*hardpoint, *component)) {
        std::cerr << "[ShipBuilder] Component incompatible with hardpoint" << std::endl;
        return false;
    }
    
    if (!CheckComponentRequirements(ship, *component)) {
        std::cerr << "[ShipBuilder] Component requirements not met" << std::endl;
        return false;
    }
    
    // Install component
    hardpoint->installedComponent = component;
    hardpoint->occupied = true;
    ship.components[hardpointId] = component;
    
    // Invalidate cached metrics
    if (ship.cachedMetrics) {
        delete ship.cachedMetrics;
        ship.cachedMetrics = nullptr;
    }
    
    std::cout << "[ShipBuilder] Installed " << component->name 
              << " in hardpoint " << hardpointId << std::endl;
    return true;
}

bool ShipBuilder::RemoveComponent(ShipLoadout& ship, const std::string& hardpointId) {
    // Find hardpoint
    Hardpoint* hardpoint = nullptr;
    for (auto& hp : ship.hull->hardpoints) {
        if (hp.id == hardpointId) {
            hardpoint = &hp;
            break;
        }
    }
    
    if (!hardpoint || !hardpoint->occupied) {
        return false;
    }
    
    hardpoint->installedComponent = nullptr;
    hardpoint->occupied = false;
    ship.components.erase(hardpointId);
    
    // Invalidate cached metrics
    if (ship.cachedMetrics) {
        delete ship.cachedMetrics;
        ship.cachedMetrics = nullptr;
    }
    
    std::cout << "[ShipBuilder] Removed component from hardpoint " << hardpointId << std::endl;
    return true;
}

bool ShipBuilder::ValidateShip(const ShipLoadout& ship,
                               std::vector<std::string>& errors,
                               std::vector<std::string>& warnings) {
    errors.clear();
    warnings.clear();
    
    // Check power balance
    double powerGen = ship.hull->basePower;
    double powerDraw = CalculatePowerConsumption(ship);
    
    if (powerDraw > powerGen) {
        errors.push_back("Insufficient power: " + std::to_string(powerDraw - powerGen) + " MW deficit");
    } else if (powerDraw > powerGen * 0.9) {
        warnings.push_back("Power usage at " + std::to_string((powerDraw / powerGen) * 100) + "%");
    }
    
    // Check thermal balance
    double cooling = ship.hull->baseCooling;
    double heat = CalculateHeatGeneration(ship);
    
    if (heat > cooling) {
        errors.push_back("Insufficient cooling: " + std::to_string(heat - cooling) + " thermal units over capacity");
    } else if (heat > cooling * 0.9) {
        warnings.push_back("Heat generation at " + std::to_string((heat / cooling) * 100) + "%");
    }
    
    // Check for required components
    bool hasPowerPlant = false;
    bool hasEngine = false;
    
    for (const auto& [hpId, comp] : ship.components) {
        if (comp->type == ComponentType::PowerPlant) hasPowerPlant = true;
        if (comp->type == ComponentType::Engine) hasEngine = true;
    }
    
    if (!hasPowerPlant && ship.hull->className != "Drone") {
        warnings.push_back("No power plant installed - using hull power only");
    }
    
    if (!hasEngine) {
        errors.push_back("No engine installed - ship cannot move");
    }
    
    return errors.empty();
}

PerformanceMetrics ShipBuilder::CalculatePerformance(const ShipLoadout& ship) {
    PerformanceMetrics metrics;
    
    // Mass calculation
    metrics.totalMass = CalculateTotalMass(ship);
    
    // Power and thermal
    metrics.powerGeneration = ship.hull->basePower;
    metrics.powerConsumption = CalculatePowerConsumption(ship);
    metrics.powerBalance = metrics.powerGeneration - metrics.powerConsumption;
    
    metrics.coolingCapacity = ship.hull->baseCooling;
    metrics.heatGeneration = CalculateHeatGeneration(ship);
    metrics.thermalBalance = metrics.coolingCapacity - metrics.heatGeneration;
    
    // Propulsion
    double totalThrust = 0.0;
    double totalThrusterPower = 0.0;
    
    for (const auto& [hpId, comp] : ship.components) {
        if (comp->type == ComponentType::Engine) {
            auto thrustIt = comp->stats.find("thrust");
            if (thrustIt != comp->stats.end()) {
                totalThrust += thrustIt->second;
            }
        }
        if (comp->type == ComponentType::Thruster) {
            auto powerIt = comp->stats.find("maneuver_power");
            if (powerIt != comp->stats.end()) {
                totalThrusterPower += powerIt->second;
            }
        }
    }
    
    if (metrics.totalMass > 0.0) {
        metrics.acceleration = totalThrust / metrics.totalMass;  // m/s²
        metrics.maxSpeed = metrics.acceleration * 100.0;  // Simplified
        metrics.maneuverability = (totalThrusterPower / metrics.totalMass) * 10.0;  // deg/s
    }
    
    // Combat stats
    for (const auto& [hpId, comp] : ship.components) {
        if (comp->type == ComponentType::Weapon) {
            auto dpsIt = comp->stats.find("dps");
            if (dpsIt != comp->stats.end()) {
                metrics.totalFirepower += dpsIt->second;
            }
        }
        if (comp->type == ComponentType::Shield) {
            auto strengthIt = comp->stats.find("strength");
            if (strengthIt != comp->stats.end()) {
                metrics.shieldStrength += strengthIt->second;
            }
        }
        if (comp->type == ComponentType::Sensor) {
            auto rangeIt = comp->stats.find("range");
            if (rangeIt != comp->stats.end()) {
                metrics.sensorRange = std::max(metrics.sensorRange, rangeIt->second);
            }
        }
    }
    
    metrics.armorRating = ship.hull->baseArmor;
    
    // Capacity
    metrics.cargoCapacity = ship.hull->cargoCapacity;
    metrics.fuelCapacity = ship.hull->fuelCapacity;
    
    // Cost
    metrics.totalCost = ship.hull->cost;
    for (const auto& [hpId, comp] : ship.components) {
        metrics.totalCost += comp->cost;
    }
    metrics.maintenanceCost = metrics.totalCost * 0.01;  // 1% per cycle
    
    // Validation warnings
    ValidateShip(ship, metrics.errors, metrics.warnings);
    
    return metrics;
}

// === Preset System ===

std::shared_ptr<ShipLoadout> ShipBuilder::LoadPreset(PresetType preset) {
    auto it = presets_.find(preset);
    if (it != presets_.end()) {
        // Deep copy the preset
        auto ship = std::make_shared<ShipLoadout>(*it->second);
        ship->id = "ship_" + std::to_string(rand());  // New unique ID
        return ship;
    }
    
    std::cerr << "[ShipBuilder] Preset not found: " << static_cast<int>(preset) << std::endl;
    return nullptr;
}

bool ShipBuilder::SaveAsPreset(const ShipLoadout& ship, const std::string& presetName) {
    auto preset = std::make_shared<ShipLoadout>(ship);
    customPresets_[presetName] = preset;
    std::cout << "[ShipBuilder] Saved custom preset: " << presetName << std::endl;
    return true;
}

std::vector<std::pair<PresetType, std::string>> ShipBuilder::GetAvailablePresets() const {
    std::vector<std::pair<PresetType, std::string>> result;
    
    result.push_back({PresetType::Fighter, "Fighter"});
    result.push_back({PresetType::HeavyFighter, "Heavy Fighter"});
    result.push_back({PresetType::Interceptor, "Interceptor"});
    result.push_back({PresetType::Trader, "Trader"});
    result.push_back({PresetType::Freighter, "Freighter"});
    result.push_back({PresetType::Explorer, "Explorer"});
    result.push_back({PresetType::Scout, "Scout"});
    result.push_back({PresetType::Miner, "Miner"});
    result.push_back({PresetType::Salvager, "Salvager"});
    result.push_back({PresetType::Support, "Support"});
    result.push_back({PresetType::Patrol, "Patrol"});
    result.push_back({PresetType::Bomber, "Bomber"});
    
    return result;
}

// === Component Catalog ===

std::vector<std::shared_ptr<ComponentDefinition>> 
ShipBuilder::GetComponentsByType(ComponentType type) {
    std::vector<std::shared_ptr<ComponentDefinition>> result;
    
    for (const auto& [id, comp] : componentCatalog_) {
        if (comp->type == type) {
            result.push_back(comp);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ComponentDefinition>> 
ShipBuilder::GetCompatibleComponents(const ShipLoadout& ship,
                                    const std::string& hardpointId) {
    std::vector<std::shared_ptr<ComponentDefinition>> result;
    
    // Find hardpoint
    const Hardpoint* hardpoint = nullptr;
    for (const auto& hp : ship.hull->hardpoints) {
        if (hp.id == hardpointId) {
            hardpoint = &hp;
            break;
        }
    }
    
    if (!hardpoint) return result;
    
    // Filter components by compatibility
    for (const auto& [id, comp] : componentCatalog_) {
        if (CheckHardpointCompatibility(*hardpoint, *comp)) {
            result.push_back(comp);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ComponentDefinition>> 
ShipBuilder::GetUpgradeOptions(const std::string& componentId) {
    std::vector<std::shared_ptr<ComponentDefinition>> result;
    
    auto it = componentCatalog_.find(componentId);
    if (it == componentCatalog_.end()) return result;
    
    for (const std::string& upgradeId : it->second->upgradesTo) {
        auto upgradeIt = componentCatalog_.find(upgradeId);
        if (upgradeIt != componentCatalog_.end()) {
            result.push_back(upgradeIt->second);
        }
    }
    
    return result;
}

// === Hull Catalog ===

std::vector<std::shared_ptr<ShipHull>> ShipBuilder::GetAvailableHulls() const {
    std::vector<std::shared_ptr<ShipHull>> result;
    
    for (const auto& [id, hull] : hullCatalog_) {
        result.push_back(hull);
    }
    
    return result;
}

std::vector<std::shared_ptr<ShipHull>> 
ShipBuilder::GetHullsByClass(const std::string& className) {
    std::vector<std::shared_ptr<ShipHull>> result;
    
    for (const auto& [id, hull] : hullCatalog_) {
        if (hull->className == className) {
            result.push_back(hull);
        }
    }
    
    return result;
}

// === Hangar Management ===

bool ShipBuilder::AddToHangar(std::shared_ptr<ShipLoadout> ship, 
                              const std::string& playerId) {
    hangars_[playerId].push_back(ship);
    
    // Set as active if first ship
    if (hangars_[playerId].size() == 1) {
        activeShips_[playerId] = ship->id;
    }
    
    std::cout << "[ShipBuilder] Added ship " << ship->customName 
              << " to hangar for player " << playerId << std::endl;
    return true;
}

bool ShipBuilder::RemoveFromHangar(const std::string& shipId, 
                                   const std::string& playerId) {
    auto& ships = hangars_[playerId];
    
    auto it = std::remove_if(ships.begin(), ships.end(),
        [&shipId](const std::shared_ptr<ShipLoadout>& ship) {
            return ship->id == shipId;
        });
    
    if (it != ships.end()) {
        ships.erase(it, ships.end());
        
        // Clear active ship if it was removed
        if (activeShips_[playerId] == shipId) {
            activeShips_.erase(playerId);
            // Set new active ship if available
            if (!ships.empty()) {
                activeShips_[playerId] = ships[0]->id;
            }
        }
        
        std::cout << "[ShipBuilder] Removed ship from hangar" << std::endl;
        return true;
    }
    
    return false;
}

std::vector<std::shared_ptr<ShipLoadout>> 
ShipBuilder::GetHangarShips(const std::string& playerId) {
    return hangars_[playerId];
}

bool ShipBuilder::SetActiveShip(const std::string& shipId, 
                                const std::string& playerId) {
    // Verify ship exists in hangar
    const auto& ships = hangars_[playerId];
    bool found = false;
    
    for (const auto& ship : ships) {
        if (ship->id == shipId) {
            found = true;
            break;
        }
    }
    
    if (!found) return false;
    
    activeShips_[playerId] = shipId;
    std::cout << "[ShipBuilder] Set active ship for player " << playerId << std::endl;
    return true;
}

// === Customization ===

void ShipBuilder::SetShipName(ShipLoadout& ship, const std::string& name) {
    ship.customName = name;
}

void ShipBuilder::SetPaintJob(ShipLoadout& ship,
                              float pr, float pg, float pb,
                              float sr, float sg, float sb) {
    ship.paintJob.primaryR = pr;
    ship.paintJob.primaryG = pg;
    ship.paintJob.primaryB = pb;
    ship.paintJob.secondaryR = sr;
    ship.paintJob.secondaryG = sg;
    ship.paintJob.secondaryB = sb;
}

void ShipBuilder::SetDecal(ShipLoadout& ship, const std::string& decalId) {
    ship.paintJob.decalId = decalId;
}

// === Insurance ===

double ShipBuilder::CalculateInsuranceCost(const ShipLoadout& ship) {
    auto metrics = CalculatePerformance(ship);
    return metrics.totalCost * 0.05;  // 5% of ship value
}

bool ShipBuilder::PurchaseInsurance(ShipLoadout& ship) {
    ship.insuranceValue = CalculatePerformance(ship).totalCost * 0.9;  // 90% payout
    ship.insured = true;
    std::cout << "[ShipBuilder] Purchased insurance for ship " << ship.customName << std::endl;
    return true;
}

bool ShipBuilder::FileInsuranceClaim(const std::string& shipId, 
                                     const std::string& playerId) {
    // TODO: Implement insurance claim system with payout
    std::cout << "[ShipBuilder] Filing insurance claim for ship " << shipId << std::endl;
    return true;
}

// === Data Loading ===

bool ShipBuilder::LoadComponentCatalog(const std::string& jsonPath) {
    std::cout << "[ShipBuilder] Loading component catalog from " << jsonPath << std::endl;
    // TODO: Implement JSON loading
    return true;
}

bool ShipBuilder::LoadHullCatalog(const std::string& jsonPath) {
    std::cout << "[ShipBuilder] Loading hull catalog from " << jsonPath << std::endl;
    // TODO: Implement JSON loading
    return true;
}

bool ShipBuilder::LoadPresets(const std::string& jsonPath) {
    std::cout << "[ShipBuilder] Loading presets from " << jsonPath << std::endl;
    // TODO: Implement JSON loading
    return true;
}

// === Serialization ===

bool ShipBuilder::SaveShip(const ShipLoadout& ship, const std::string& filepath) {
    std::cout << "[ShipBuilder] Saving ship to " << filepath << std::endl;
    // TODO: Implement JSON serialization
    return true;
}

std::shared_ptr<ShipLoadout> ShipBuilder::LoadShip(const std::string& filepath) {
    std::cout << "[ShipBuilder] Loading ship from " << filepath << std::endl;
    // TODO: Implement JSON deserialization
    return nullptr;
}

// === Helper Functions ===

bool ShipBuilder::CheckHardpointCompatibility(const Hardpoint& hardpoint,
                                              const ComponentDefinition& component) {
    // Check size
    if (component.size > hardpoint.maxSize) {
        return false;
    }
    
    // Check type
    switch (hardpoint.type) {
        case HardpointType::Universal:
            return true;
        case HardpointType::Weapon:
            return component.type == ComponentType::Weapon;
        case HardpointType::Engine:
            return component.type == ComponentType::Engine || 
                   component.type == ComponentType::Thruster;
        case HardpointType::Utility:
            return component.type != ComponentType::Weapon;
        case HardpointType::Internal:
            return component.type == ComponentType::Shield ||
                   component.type == ComponentType::PowerPlant ||
                   component.type == ComponentType::Computer ||
                   component.type == ComponentType::LifeSupport;
        case HardpointType::External:
            return component.type == ComponentType::CargoHold ||
                   component.type == ComponentType::FuelTank;
        default:
            return false;
    }
}

bool ShipBuilder::CheckComponentRequirements(const ShipLoadout& ship,
                                             const ComponentDefinition& component) {
    uint32_t flags = component.compatibilityFlags;
    
    // Check if requires power plant
    if (flags & static_cast<uint32_t>(CompatibilityFlags::RequiresPowerPlant)) {
        bool hasPowerPlant = false;
        for (const auto& [hpId, comp] : ship.components) {
            if (comp->type == ComponentType::PowerPlant) {
                hasPowerPlant = true;
                break;
            }
        }
        if (!hasPowerPlant) return false;
    }
    
    // Check if requires computer
    if (flags & static_cast<uint32_t>(CompatibilityFlags::RequiresComputer)) {
        bool hasComputer = false;
        for (const auto& [hpId, comp] : ship.components) {
            if (comp->type == ComponentType::Computer) {
                hasComputer = true;
                break;
            }
        }
        if (!hasComputer) return false;
    }
    
    return true;
}

double ShipBuilder::CalculatePowerConsumption(const ShipLoadout& ship) {
    double total = 0.0;
    for (const auto& [hpId, comp] : ship.components) {
        total += comp->powerDraw;
    }
    return total;
}

double ShipBuilder::CalculateHeatGeneration(const ShipLoadout& ship) {
    double total = 0.0;
    for (const auto& [hpId, comp] : ship.components) {
        total += comp->coolingRequired;
    }
    return total;
}

double ShipBuilder::CalculateTotalMass(const ShipLoadout& ship) {
    double total = ship.hull->baseMass;
    for (const auto& [hpId, comp] : ship.components) {
        total += comp->mass;
    }
    return total;
}

} // namespace ShipBuilding
