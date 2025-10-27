#include "ShipAssembly.h"

#include "ComponentJsonLoader.h"

#include <algorithm>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <iomanip>
#include <cmath>
#include <iostream>

namespace {

int SizeRank(SlotSize size) {
    switch (size) {
        case SlotSize::XS: return 0;
        case SlotSize::Small: return 1;
        case SlotSize::Medium: return 2;
        case SlotSize::Large: return 3;
        case SlotSize::XL: return 4;
        case SlotSize::XXL: return 5;
    }
    return 0;
}

struct ComponentRegistry {
    std::vector<ShipComponentBlueprint> components;
    std::unordered_map<std::string, std::size_t> index;
};

struct HullRegistry {
    std::vector<ShipHullBlueprint> hulls;
    std::unordered_map<std::string, std::size_t> index;
};

ComponentRegistry& GetComponentRegistry() {
    static ComponentRegistry registry;
    return registry;
}

HullRegistry& GetHullRegistry() {
    static HullRegistry registry;
    return registry;
}

void RegisterDefaultComponents();
void RegisterDefaultHulls();

void EnsureComponentDefaultsInitialized() {
    static std::once_flag flag;
    std::call_once(flag, [](){
        RegisterDefaultComponents();
    });
}

void EnsureHullDefaultsInitialized() {
    static std::once_flag flag;
    std::call_once(flag, [](){
        RegisterDefaultHulls();
    });
}

std::string DescribeSlot(const HullSlot& slot) {
    std::ostringstream oss;
    oss << "slot '" << slot.slotId << "' (" << ToString(slot.category)
        << ", size " << ToString(slot.size) << ")";
    return oss.str();
}

std::string DescribeComponent(const ShipComponentBlueprint& component) {
    std::ostringstream oss;
    oss << "component '" << component.displayName << "' (" << component.id
        << ", size " << ToString(component.size) << ")";
    return oss.str();
}

std::string FormatSlotLabel(const ShipHullBlueprint* hull, const std::string& slotId) {
    if (!hull) {
        return "slot '" + slotId + "'";
    }
    auto it = std::find_if(hull->slots.begin(), hull->slots.end(), [&](const HullSlot& slot) {
        return slot.slotId == slotId;
    });
    if (it == hull->slots.end()) {
        return "slot '" + slotId + "'";
    }
    std::ostringstream oss;
    oss << ToString(it->category) << " slot '" << slotId << "'";
    oss << " (" << ToString(it->size);
    if (!it->notes.empty()) {
        oss << ", " << it->notes;
    }
    oss << ")";
    return oss.str();
}

std::string FormatComponentLabel(const std::string& componentId) {
    const auto* blueprint = ShipComponentCatalog::Find(componentId);
    if (!blueprint) {
        return componentId;
    }
    std::ostringstream oss;
    oss << blueprint->displayName << " [" << blueprint->id << "]";
    return oss.str();
}

std::string JoinComponentLabels(const std::vector<std::string>& componentIds) {
    if (componentIds.empty()) {
        return {};
    }
    if (componentIds.size() == 1) {
        return FormatComponentLabel(componentIds.front());
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < componentIds.size(); ++i) {
        if (i > 0) {
            if (componentIds.size() > 2) {
                oss << ", ";
            } else {
                oss << " ";
            }
        }
        if (i + 1 == componentIds.size()) {
            oss << "or ";
        }
        oss << FormatComponentLabel(componentIds[i]);
    }
    return oss.str();
}

std::vector<std::string> FindCompatibleComponentIds(const HullSlot& slot, std::size_t limit = 5) {
    std::vector<std::string> result;
    result.reserve(limit);
    const auto& allComponents = ShipComponentCatalog::All();
    for (const auto& component : allComponents) {
        if (component.category != slot.category) {
            continue;
        }
        if (!SlotSizeFits(slot.size, component.size)) {
            continue;
        }
        result.push_back(component.id);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

ShipHullBlueprint ExpandDefinition(const SpaceshipClassDefinition& def, const std::string& idSuffix) {
    ShipHullBlueprint blueprint;
    blueprint.id = idSuffix;
    blueprint.classType = def.type;
    blueprint.displayName = def.displayName + " Hull";
    blueprint.baseMassTons = def.baseline.minMassTons;
    blueprint.structuralIntegrity = def.baseline.maxMassTons * 10.0; // placeholder strength metric
    blueprint.baseCrewRequired = def.baseline.minCrew;
    blueprint.baseCrewCapacity = def.baseline.maxCrew;
    blueprint.baseHeatGenerationMW = 0.0;
    blueprint.baseHeatDissipationMW = def.baseline.maxPowerBudgetMW;

    for (const auto& spec : def.componentSlots) {
        for (int i = 0; i < spec.count; ++i) {
            HullSlot slot;
            // Ensure string concatenation (ToString may return const char*)
            slot.slotId = std::string(ToString(spec.category)) + "_" + std::to_string(i);
            slot.category = spec.category;
            slot.size = spec.size;
            slot.notes = spec.notes;
            slot.required = true;
            blueprint.slots.push_back(slot);
        }
    }
    return blueprint;
}

void RegisterDefaultComponents() {
    auto& registry = GetComponentRegistry();
    registry.components.clear();
    registry.index.clear();

    // Skip JSON loading for now to avoid issues
    // if (ComponentJsonLoader::LoadComponentsFromDirectory("assets/components")) {
    //     // Successfully loaded from JSON
    //     return;
    // }

    // Fallback to hardcoded defaults if JSON loading fails
    auto add = [&](ShipComponentBlueprint blueprint) {
        registry.index[blueprint.id] = registry.components.size();
        registry.components.push_back(std::move(blueprint));
    };

    // PowerPlant components
    {
        ShipComponentBlueprint comp;
        comp.id = "fusion_core_mk1";
        comp.displayName = "Fusion Core Mk.I";
        comp.description = "Baseline fighter fusion core.";
        comp.category = ComponentSlotCategory::PowerPlant;
        comp.size = SlotSize::Small;
        comp.massTons = 6.5;
        comp.powerOutputMW = 10.0;
        comp.powerDrawMW = 0.2;
        comp.heatGenerationMW = 2.5;
        comp.heatDissipationMW = 1.5;
        comp.crewRequired = 1;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.I";
        comp.minPowerEnvelopeMW = 0.0;
        comp.maxPowerEnvelopeMW = 1000.0;
        comp.optimalPowerEnvelopeMW = 50.0;
        add(std::move(comp));
    }

    // MainThruster components
    {
        ShipComponentBlueprint comp;
        comp.id = "main_thruster_viper";
        comp.displayName = "Viper Main Thruster";
        comp.description = "High thrust ratio for fighters.";
        comp.category = ComponentSlotCategory::MainThruster;
        comp.size = SlotSize::Small;
        comp.massTons = 4.0;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 4.0;
        comp.thrustKN = 220.0;
        comp.heatGenerationMW = 5.0;
        comp.heatDissipationMW = 1.0;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.I";
        comp.minPowerEnvelopeMW = 5.0;
        comp.maxPowerEnvelopeMW = 25.0;
        comp.optimalPowerEnvelopeMW = 10.0;
        add(std::move(comp));
    }

    // ManeuverThruster components
    {
        ShipComponentBlueprint comp;
        comp.id = "rcs_cluster_micro";
        comp.displayName = "Micro RCS Cluster";
        comp.description = "Reaction control thrusters for fine maneuvers.";
        comp.category = ComponentSlotCategory::ManeuverThruster;
        comp.size = SlotSize::XS;
        comp.massTons = 0.8;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 0.5;
        comp.thrustKN = 35.0;
        comp.heatGenerationMW = 0.3;
        comp.heatDissipationMW = 0.3;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        add(std::move(comp));
    }

    // Shield components
    {
        ShipComponentBlueprint comp;
        comp.id = "shield_array_light";
        comp.displayName = "Light Shield Array";
        comp.description = "Directional shield generator for fighters.";
        comp.category = ComponentSlotCategory::Shield;
        comp.size = SlotSize::Small;
        comp.massTons = 3.2;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 2.5;
        comp.heatGenerationMW = 3.0;
        comp.heatDissipationMW = 0.5;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.shieldCapacityMJ = 150.0;
        comp.shieldRechargeRateMJPerSec = 5.0;
        comp.shieldRechargeDelaySeconds = 3.0;
        comp.shieldDamageAbsorption = 0.8;
        add(std::move(comp));
    }

    // Weapon components
    {
        ShipComponentBlueprint comp;
        comp.id = "weapon_twin_cannon";
        comp.displayName = "Twin Cannon";
        comp.description = "Rapid-fire projectile weapon for fighters.";
        comp.category = ComponentSlotCategory::Weapon;
        comp.size = SlotSize::Small;
        comp.massTons = 3.5;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 2.0;
        comp.heatGenerationMW = 2.5;
        comp.heatDissipationMW = 1.0;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.I";
        comp.minPowerEnvelopeMW = 5.0;
        comp.maxPowerEnvelopeMW = 25.0;
        comp.optimalPowerEnvelopeMW = 10.0;
        comp.weaponDamagePerShot = 15.0;
        comp.weaponRangeKm = 5.0;
        comp.weaponFireRatePerSecond = 10.0;
        comp.weaponAmmoCapacity = 200;
        comp.weaponAmmoType = "projectile";
        comp.weaponIsTurret = false;
        comp.weaponProjectileSpeedKmPerSec = 2.0;
        add(std::move(comp));
    }

    // Support components
    {
        ShipComponentBlueprint comp;
        comp.id = "support_life_pod";
        comp.displayName = "Emergency Life Support Pod";
        comp.description = "Sustains crew during hull breaches.";
        comp.category = ComponentSlotCategory::Support;
        comp.size = SlotSize::XS;
        comp.massTons = 1.2;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 0.6;
        comp.heatGenerationMW = 0.1;
        comp.heatDissipationMW = 0.5;
        comp.crewRequired = 0;
        comp.crewSupport = 2;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        add(std::move(comp));
    }

    // Additional support component for testing
    {
        ShipComponentBlueprint comp;
        comp.id = "support_basic";
        comp.displayName = "Basic Support Module";
        comp.description = "Minimal support systems.";
        comp.category = ComponentSlotCategory::Support;
        comp.size = SlotSize::XS;
        comp.massTons = 0.8;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 0.3;
        comp.heatGenerationMW = 0.05;
        comp.heatDissipationMW = 0.3;
        comp.crewRequired = 0;
        comp.crewSupport = 0; // No crew support for testing shortfall
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        add(std::move(comp));
    }

    // Sensor components
    {
        ShipComponentBlueprint comp;
        comp.id = "sensor_targeting_mk1";
        comp.displayName = "Combat Sensor Suite";
        comp.description = "Targeting computer with enhanced tracking.";
        comp.category = ComponentSlotCategory::Sensor;
        comp.size = SlotSize::Small;
        comp.massTons = 1.4;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 1.2;
        comp.heatGenerationMW = 1.5;
        comp.heatDissipationMW = 0.5;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        add(std::move(comp));
    }

    // Additional components for test coverage
    {
        ShipComponentBlueprint comp;
        comp.id = "weapon_defensive_turret";
        comp.displayName = "Defensive Turret";
        comp.description = "Automated defensive weapon system.";
        comp.category = ComponentSlotCategory::Weapon;
        comp.size = SlotSize::Small;
        comp.massTons = 5.0;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 3.0;
        comp.heatGenerationMW = 4.0;
        comp.heatDissipationMW = 1.5;
        comp.crewRequired = 2;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.I";
        comp.minPowerEnvelopeMW = 8.0;
        comp.maxPowerEnvelopeMW = 30.0;
        comp.optimalPowerEnvelopeMW = 15.0;
        comp.weaponDamagePerShot = 20.0;
        comp.weaponRangeKm = 8.0;
        comp.weaponFireRatePerSecond = 5.0;
        comp.weaponAmmoCapacity = 150;
        comp.weaponAmmoType = "projectile";
        comp.weaponIsTurret = true;
        comp.weaponProjectileSpeedKmPerSec = 3.0;
        add(std::move(comp));
    }

    {
        ShipComponentBlueprint comp;
        comp.id = "main_thruster_freighter";
        comp.displayName = "Freighter Main Thruster";
        comp.description = "Heavy-duty thruster for cargo vessels.";
        comp.category = ComponentSlotCategory::MainThruster;
        comp.size = SlotSize::Small;
        comp.massTons = 8.0;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 8.0;
        comp.thrustKN = 350.0;
        comp.heatGenerationMW = 8.0;
        comp.heatDissipationMW = 2.0;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.II";
        comp.minPowerEnvelopeMW = 10.0;
        comp.maxPowerEnvelopeMW = 40.0;
        comp.optimalPowerEnvelopeMW = 20.0;
        add(std::move(comp));
    }

    {
        ShipComponentBlueprint comp;
        comp.id = "fusion_core_mk2";
        comp.displayName = "Fusion Core Mk.II";
        comp.description = "Advanced fusion reactor with higher output.";
        comp.category = ComponentSlotCategory::PowerPlant;
        comp.size = SlotSize::Small;
        comp.massTons = 7.0;
        comp.powerOutputMW = 18.0;
        comp.powerDrawMW = 0.3;
        comp.heatGenerationMW = 3.0;
        comp.heatDissipationMW = 2.0;
        comp.crewRequired = 1;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 2;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.II";
        comp.minPowerEnvelopeMW = 0.0;
        comp.maxPowerEnvelopeMW = 1000.0;
        comp.optimalPowerEnvelopeMW = 60.0;
        add(std::move(comp));
    }

    {
        ShipComponentBlueprint comp;
        comp.id = "cargo_rack_standard";
        comp.displayName = "Standard Cargo Rack";
        comp.description = "Basic cargo storage system.";
        comp.category = ComponentSlotCategory::Cargo;
        comp.size = SlotSize::Large;
        comp.massTons = 2.0;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 0.5;
        comp.heatGenerationMW = 0.2;
        comp.heatDissipationMW = 0.8;
        comp.crewRequired = 0;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 1;
        comp.manufacturer = "Nova Dynamics";
        add(std::move(comp));
    }

    {
        ShipComponentBlueprint comp;
        comp.id = "weapon_beam_array";
        comp.displayName = "Beam Array";
        comp.description = "High-energy beam weapon system.";
        comp.category = ComponentSlotCategory::Weapon;
        comp.size = SlotSize::Medium;
        comp.massTons = 6.0;
        comp.powerOutputMW = 0.0;
        comp.powerDrawMW = 5.0;
        comp.heatGenerationMW = 8.0;
        comp.heatDissipationMW = 2.0;
        comp.crewRequired = 1;
        comp.crewSupport = 0;
        comp.schemaVersion = 1;
        comp.techTier = 2;
        comp.manufacturer = "Nova Dynamics";
        comp.manufacturerLineage = "Mk.II";
        comp.minPowerEnvelopeMW = 12.0;
        comp.maxPowerEnvelopeMW = 50.0;
        comp.optimalPowerEnvelopeMW = 25.0;
        comp.weaponDamagePerShot = 35.0;
        comp.weaponRangeKm = 12.0;
        comp.weaponFireRatePerSecond = 3.0;
        comp.weaponAmmoCapacity = 0; // Energy weapon
        comp.weaponAmmoType = "energy";
        comp.weaponIsTurret = false;
        comp.weaponProjectileSpeedKmPerSec = 300000.0; // Speed of light approximation
        add(std::move(comp));
    }
}

void RegisterDefaultHulls() {
    auto& registry = GetHullRegistry();
    registry.hulls.clear();
    registry.index.clear();

    // Hardcode fighter hull instead of using SpaceshipCatalog to avoid initialization issues
    ShipHullBlueprint fighterHull;
    fighterHull.id = "fighter_mk1";
    fighterHull.classType = SpaceshipClassType::Fighter;
    fighterHull.displayName = "Fighter Hull";
    fighterHull.baseMassTons = 25.0f;
    fighterHull.structuralIntegrity = 250.0f;
    fighterHull.baseCrewRequired = 1;
    fighterHull.baseCrewCapacity = 2;
    fighterHull.baseHeatGenerationMW = 0.0;
    fighterHull.baseHeatDissipationMW = 12.0f;

    // Add slots based on fighter definition
    fighterHull.slots.push_back(HullSlot{"PowerPlant_0", ComponentSlotCategory::PowerPlant, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"MainThruster_0", ComponentSlotCategory::MainThruster, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"ManeuverThruster_0", ComponentSlotCategory::ManeuverThruster, SlotSize::XS, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"ManeuverThruster_1", ComponentSlotCategory::ManeuverThruster, SlotSize::XS, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"ManeuverThruster_2", ComponentSlotCategory::ManeuverThruster, SlotSize::XS, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"ManeuverThruster_3", ComponentSlotCategory::ManeuverThruster, SlotSize::XS, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"Shield_0", ComponentSlotCategory::Shield, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"Weapon_0", ComponentSlotCategory::Weapon, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"Weapon_1", ComponentSlotCategory::Weapon, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"Sensor_0", ComponentSlotCategory::Sensor, SlotSize::Small, "", true, {}});
    fighterHull.slots.push_back(HullSlot{"Support_0", ComponentSlotCategory::Support, SlotSize::XS, "", true, {}});

    // Set up adjacency relationships for fighter hull
    if (fighterHull.slots.size() >= 3) {
        // Power plant is adjacent to weapons
        fighterHull.slots[0].adjacentSlotIds = {"Weapon_0", "Weapon_1"}; // PowerPlant_0 adjacent to weapons

        // Weapons are adjacent to each other and power plant
        fighterHull.slots[7].adjacentSlotIds = {"PowerPlant_0", "Weapon_1"}; // Weapon_0 adjacent to power and Weapon_1
        fighterHull.slots[8].adjacentSlotIds = {"PowerPlant_0", "Weapon_0"}; // Weapon_1 adjacent to power and Weapon_0
    }

    registry.index[fighterHull.id] = registry.hulls.size();
    registry.hulls.push_back(std::move(fighterHull));

    // Hardcode freighter hull
    ShipHullBlueprint freighterHull;
    freighterHull.id = "freighter_mk1";
    freighterHull.classType = SpaceshipClassType::Freighter;
    freighterHull.displayName = "Freighter Hull";
    freighterHull.baseMassTons = 90.0f;
    freighterHull.structuralIntegrity = 900.0f;
    freighterHull.baseCrewRequired = 2;
    freighterHull.baseCrewCapacity = 4;
    freighterHull.baseHeatGenerationMW = 0.0;
    freighterHull.baseHeatDissipationMW = 26.0f;

    // Add slots for freighter
    freighterHull.slots.push_back(HullSlot{"PowerPlant_0", ComponentSlotCategory::PowerPlant, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"MainThruster_0", ComponentSlotCategory::MainThruster, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"MainThruster_1", ComponentSlotCategory::MainThruster, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"ManeuverThruster_0", ComponentSlotCategory::ManeuverThruster, SlotSize::Small, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"ManeuverThruster_1", ComponentSlotCategory::ManeuverThruster, SlotSize::Small, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"ManeuverThruster_2", ComponentSlotCategory::ManeuverThruster, SlotSize::Small, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"Shield_0", ComponentSlotCategory::Shield, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"Weapon_0", ComponentSlotCategory::Weapon, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"Sensor_0", ComponentSlotCategory::Sensor, SlotSize::Medium, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"Cargo_0", ComponentSlotCategory::Cargo, SlotSize::Large, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"CrewQuarters_0", ComponentSlotCategory::CrewQuarters, SlotSize::Small, "", true, {}});
    freighterHull.slots.push_back(HullSlot{"Support_0", ComponentSlotCategory::Support, SlotSize::Medium, "", true, {}});

    // Set up adjacency for freighter
    if (freighterHull.slots.size() >= 5) {
        // Power plant central, weapons adjacent to each other, cargo adjacent to power
        freighterHull.slots[0].adjacentSlotIds = {"Weapon_0", "Cargo_0"}; // PowerPlant_0
        freighterHull.slots[7].adjacentSlotIds = {"PowerPlant_0"}; // Weapon_0
        freighterHull.slots[9].adjacentSlotIds = {"PowerPlant_0"}; // Cargo_0
    }

    registry.index[freighterHull.id] = registry.hulls.size();
    registry.hulls.push_back(std::move(freighterHull));
}

} // namespace

std::vector<RankedComponentSuggestion> FindRankedComponentSuggestions(const HullSlot& slot,
                                                                       const std::vector<std::string>& existingManufacturers,
                                                                       std::size_t limit) {
    std::vector<RankedComponentSuggestion> suggestions;
    const auto& allComponents = ShipComponentCatalog::All();

    for (const auto& component : allComponents) {
        if (component.category != slot.category) {
            continue;
        }
        if (!SlotSizeFits(slot.size, component.size)) {
            continue;
        }

        double fitScore = 0.0;
        std::string reasoning = "Compatible component";

        // Size fit scoring (0.0 to 0.4)
        int slotRank = SizeRank(slot.size);
        int componentRank = SizeRank(component.size);
        double sizeFitScore = 0.4 * (1.0 - static_cast<double>(slotRank - componentRank) / static_cast<double>(slotRank));
        fitScore += std::max(0.0, sizeFitScore);

        // Manufacturer preference (0.0 to 0.3)
        bool manufacturerMatch = !existingManufacturers.empty() &&
                                std::find(existingManufacturers.begin(), existingManufacturers.end(), component.manufacturer) != existingManufacturers.end();
        if (manufacturerMatch) {
            fitScore += 0.3;
            reasoning += ", preferred manufacturer";
        }

        // Power efficiency scoring (0.0 to 0.2)
        double powerEfficiency = 0.0;
        if (component.powerDrawMW > 0.0) {
            if (component.category == ComponentSlotCategory::PowerPlant) {
                // For power plants, higher output per mass is better
                powerEfficiency = std::min(1.0, component.powerOutputMW / (component.massTons * 10.0));
            } else {
                // For other components, lower power draw per mass is better
                powerEfficiency = std::max(0.0, 1.0 - (component.powerDrawMW / (component.massTons * 2.0)));
            }
        }
        fitScore += 0.2 * powerEfficiency;

        // Performance scoring based on category (0.0 to 0.1)
        double performanceScore = 0.0;
        switch (component.category) {
            case ComponentSlotCategory::MainThruster:
                performanceScore = std::min(1.0, component.thrustKN / 500.0); // Normalize to reasonable max
                break;
            case ComponentSlotCategory::Shield:
                performanceScore = std::min(1.0, component.shieldCapacityMJ / 200.0);
                break;
            case ComponentSlotCategory::Weapon:
                performanceScore = std::min(1.0, component.weaponDamagePerShot / 50.0);
                break;
            default:
                performanceScore = 0.5; // Neutral for other categories
                break;
        }
        fitScore += 0.1 * performanceScore;

        suggestions.push_back({component.id, fitScore, reasoning});
    }

    // Sort by fit score (highest first)
    std::sort(suggestions.begin(), suggestions.end(),
              [](const RankedComponentSuggestion& a, const RankedComponentSuggestion& b) {
                  return a.fitScore > b.fitScore;
              });

    // Limit results
    if (suggestions.size() > limit) {
        suggestions.resize(limit);
    }

    return suggestions;
}

// Soft compatibility rule checking functions

// Check manufacturer lineage compatibility
bool CheckManufacturerLineageCompatibility(const std::vector<std::string>& installedManufacturers,
                                          const std::vector<std::string>& installedLineages,
                                          const ShipComponentBlueprint& component) {
    if (installedManufacturers.empty() || installedLineages.empty()) {
        return true; // No existing components to check against
    }

    // Check if component's manufacturer has any lineage matches
    bool manufacturerMatch = std::find(installedManufacturers.begin(), installedManufacturers.end(),
                                       component.manufacturer) != installedManufacturers.end();

    // Check if component's lineage matches any installed lineages
    bool lineageMatch = std::find(installedLineages.begin(), installedLineages.end(),
                                  component.manufacturerLineage) != installedLineages.end();

    // If manufacturer matches but lineage doesn't, it's a soft incompatibility
    if (manufacturerMatch && !lineageMatch && !component.manufacturerLineage.empty()) {
        return false;
    }

    return true;
}

// Check power envelope compatibility
bool CheckPowerEnvelopeCompatibility(double totalPowerOutputMW, const ShipComponentBlueprint& component) {
    if (component.category == ComponentSlotCategory::PowerPlant) {
        return true; // Power plants don't have power envelope requirements
    }

    // Check if the total power output is within the component's acceptable range
    return totalPowerOutputMW >= component.minPowerEnvelopeMW &&
           totalPowerOutputMW <= component.maxPowerEnvelopeMW;
}

// Check slot adjacency compatibility
bool CheckSlotAdjacencyCompatibility(const HullSlot& slot,
                                     const std::unordered_map<std::string, const ShipComponentBlueprint*>& resolvedComponents,
                                     const ShipComponentBlueprint& component) {
    // Check required adjacent slots
    for (ComponentSlotCategory requiredCategory : component.requiredAdjacentSlots) {
        bool foundAdjacent = false;
        for (const std::string& adjacentSlotId : slot.adjacentSlotIds) {
            auto it = resolvedComponents.find(adjacentSlotId);
            if (it != resolvedComponents.end() && it->second->category == requiredCategory) {
                foundAdjacent = true;
                break;
            }
        }
        if (!foundAdjacent) {
            return false; // Required adjacent component not found
        }
    }

    // Check incompatible adjacent slots
    for (ComponentSlotCategory incompatibleCategory : component.incompatibleAdjacentSlots) {
        for (const std::string& adjacentSlotId : slot.adjacentSlotIds) {
            auto it = resolvedComponents.find(adjacentSlotId);
            if (it != resolvedComponents.end() && it->second->category == incompatibleCategory) {
                return false; // Incompatible adjacent component found
            }
        }
    }

    return true;
}

// Apply soft compatibility rules and generate warnings
void ApplySoftCompatibilityRules(ShipAssemblyResult& result,
                                 const std::unordered_map<std::string, const ShipComponentBlueprint*>& resolvedComponents) {
    // Collect manufacturer and lineage information
    std::vector<std::string> installedManufacturers;
    std::vector<std::string> installedLineages;
    double totalPowerOutputMW = 0.0;

    for (const auto& pair : resolvedComponents) {
        const auto* component = pair.second;
        if (!component->manufacturer.empty()) {
            installedManufacturers.push_back(component->manufacturer);
        }
        if (!component->manufacturerLineage.empty()) {
            installedLineages.push_back(component->manufacturerLineage);
        }
        if (component->category == ComponentSlotCategory::PowerPlant) {
            totalPowerOutputMW += component->powerOutputMW;
        }
    }

    // Remove duplicates
    std::sort(installedManufacturers.begin(), installedManufacturers.end());
    installedManufacturers.erase(std::unique(installedManufacturers.begin(), installedManufacturers.end()),
                                installedManufacturers.end());

    std::sort(installedLineages.begin(), installedLineages.end());
    installedLineages.erase(std::unique(installedLineages.begin(), installedLineages.end()),
                          installedLineages.end());

    // Check each component for soft compatibility issues
    for (const auto& pair : resolvedComponents) {
        const std::string& slotId = pair.first;
        const auto* component = pair.second;

        // Find the slot information
        const HullSlot* slot = nullptr;
        if (result.hull) {
            for (const auto& hullSlot : result.hull->slots) {
                if (hullSlot.slotId == slotId) {
                    slot = &hullSlot;
                    break;
                }
            }
        }

        // Check manufacturer lineage compatibility
        if (!CheckManufacturerLineageCompatibility(installedManufacturers, installedLineages, *component)) {
            std::string message = "Manufacturer lineage mismatch: " + component->displayName +
                                " uses '" + component->manufacturerLineage + "' lineage, but ship uses " +
                                (!installedLineages.empty() ? "'" + installedLineages[0] + "'" : "different") + " lineage(s).";
            result.diagnostics.AddMessage(DiagnosticSeverity::Warning,
                                        DiagnosticReasonCode::COMPATIBILITY_MANUFACTURER_MISMATCH,
                                        message, slotId, {component->id});
        }

        // Check power envelope compatibility
        if (!CheckPowerEnvelopeCompatibility(totalPowerOutputMW, *component)) {
            std::ostringstream oss;
            oss << "Power envelope mismatch: " << component->displayName
                << " expects " << component->minPowerEnvelopeMW << "-" << component->maxPowerEnvelopeMW
                << " MW reactor output, but ship provides " << totalPowerOutputMW << " MW.";
            result.diagnostics.AddMessage(DiagnosticSeverity::Warning,
                                        DiagnosticReasonCode::COMPATIBILITY_POWER_ENVELOPE_MISMATCH,
                                        oss.str(), slotId, {component->id});
        }

        // Check slot adjacency compatibility
        if (slot && !CheckSlotAdjacencyCompatibility(*slot, resolvedComponents, *component)) {
            std::string message = "Slot adjacency issue: " + component->displayName +
                                " has adjacency requirements that are not satisfied.";
            result.diagnostics.AddMessage(DiagnosticSeverity::Warning,
                                        DiagnosticReasonCode::COMPATIBILITY_SLOT_ADJACENCY_ISSUE,
                                        message, slotId, {component->id});
        }
    }
}

void ShipAssemblyDiagnostics::AddError(const std::string& msg) {
    errors.push_back(msg);
}

void ShipAssemblyDiagnostics::AddWarning(const std::string& msg) {
    warnings.push_back(msg);
}

void ShipAssemblyDiagnostics::AddSuggestion(const std::string& slotId,
                                            const std::string& reason,
                                            std::vector<std::string> suggestedComponentIds) {
    ComponentSuggestion suggestion;
    suggestion.slotId = slotId;
    suggestion.reason = reason;
    suggestion.suggestedComponentIds = std::move(suggestedComponentIds);

    // Generate basic ranked suggestions for backward compatibility
    for (const auto& compId : suggestion.suggestedComponentIds) {
        RankedComponentSuggestion ranked;
        ranked.componentId = compId;
        ranked.fitScore = 0.5; // Default neutral score
        ranked.reasoning = "Compatible component";
        suggestion.rankedSuggestions.push_back(ranked);
    }

    suggestions.push_back(std::move(suggestion));
}

void ShipAssemblyDiagnostics::AddMessage(DiagnosticSeverity severity, DiagnosticReasonCode reasonCode,
                                         const std::string& message, const std::string& slotId,
                                         const std::vector<std::string>& relatedComponents) {
    DiagnosticMessage msg;
    msg.severity = severity;
    msg.reasonCode = reasonCode;
    msg.message = message;
    msg.slotId = slotId;
    msg.relatedComponents = relatedComponents;
    messages.push_back(std::move(msg));

    // Maintain legacy compatibility
    if (severity == DiagnosticSeverity::Error) {
        errors.push_back(message);
    } else if (severity == DiagnosticSeverity::Warning) {
        warnings.push_back(message);
    }
}

std::vector<std::string> ShipAssemblyDiagnostics::BuildUserFacingMessages(const ShipHullBlueprint* hull) const {
    std::vector<std::string> resultMessages;

    // Add structured messages with severity prefixes
    for (const auto& msg : messages) {
        std::string prefix;
        switch (msg.severity) {
            case DiagnosticSeverity::Error:
                prefix = "Error";
                break;
            case DiagnosticSeverity::Warning:
                prefix = "Warning";
                break;
            case DiagnosticSeverity::Info:
                prefix = "Info";
                break;
        }

        std::string fullMessage = prefix + ": " + msg.message;

        // Add slot context if available
        if (!msg.slotId.empty()) {
            fullMessage += " (slot: " + FormatSlotLabel(hull, msg.slotId) + ")";
        }

        // Add reason code for debugging/automation
        fullMessage += " [Code: " + std::to_string(static_cast<int>(msg.reasonCode)) + "]";

        resultMessages.push_back(fullMessage);
    }

    // Add legacy suggestions (maintaining backward compatibility)
    for (const auto& suggestion : suggestions) {
        std::ostringstream oss;
        oss << "Suggestion for " << FormatSlotLabel(hull, suggestion.slotId) << ": " << suggestion.reason;
        if (!suggestion.suggestedComponentIds.empty()) {
            oss << ". Try installing " << JoinComponentLabels(suggestion.suggestedComponentIds);
        }
        // Add ranked suggestions if available
        if (!suggestion.rankedSuggestions.empty()) {
            oss << " (Ranked by fit: ";
            for (size_t i = 0; i < suggestion.rankedSuggestions.size(); ++i) {
                const auto& ranked = suggestion.rankedSuggestions[i];
                if (i > 0) oss << ", ";
                const auto* comp = ShipComponentCatalog::Find(ranked.componentId);
                if (comp) {
                    oss << comp->displayName << " (" << std::fixed << std::setprecision(1) << ranked.fitScore * 100 << "%)";
                }
            }
            oss << ")";
        }
        resultMessages.push_back(oss.str());
    }

    return resultMessages;
}

const ShipComponentBlueprint* ShipComponentCatalog::Find(const std::string& id) {
    EnsureDefaults();
    auto& registry = GetComponentRegistry();
    auto it = registry.index.find(id);
    if (it == registry.index.end()) return nullptr;
    return &registry.components[it->second];
}

const ShipComponentBlueprint& ShipComponentCatalog::Get(const std::string& id) {
    const auto* result = Find(id);
    if (!result) {
        throw std::out_of_range("ShipComponentCatalog::Get - unknown component id " + id);
    }
    return *result;
}

const std::vector<ShipComponentBlueprint>& ShipComponentCatalog::All() {
    EnsureDefaults();
    return GetComponentRegistry().components;
}

void ShipComponentCatalog::Register(const ShipComponentBlueprint& blueprint) {
    auto& registry = GetComponentRegistry();
    registry.index[blueprint.id] = registry.components.size();
    registry.components.push_back(blueprint);
}

void ShipComponentCatalog::Clear() {
    auto& registry = GetComponentRegistry();
    registry.components.clear();
    registry.index.clear();
}

void ShipComponentCatalog::Reload() {
    Clear();
    // Force reload from JSON files with hot-reload capability
    ComponentJsonLoader::LoadComponentsFromDirectoryHotReload("assets/components");
}

void ShipComponentCatalog::EnsureDefaults() {
    EnsureComponentDefaultsInitialized();
}

const ShipHullBlueprint* ShipHullCatalog::Find(const std::string& id) {
    EnsureDefaults();
    auto& registry = GetHullRegistry();
    auto it = registry.index.find(id);
    if (it == registry.index.end()) return nullptr;
    return &registry.hulls[it->second];
}

const ShipHullBlueprint& ShipHullCatalog::Get(const std::string& id) {
    const auto* hull = Find(id);
    if (!hull) {
        throw std::out_of_range("ShipHullCatalog::Get - unknown hull id " + id);
    }
    return *hull;
}

const std::vector<ShipHullBlueprint>& ShipHullCatalog::All() {
    EnsureDefaults();
    return GetHullRegistry().hulls;
}

void ShipHullCatalog::Register(const ShipHullBlueprint& blueprint) {
    auto& registry = GetHullRegistry();
    registry.index[blueprint.id] = registry.hulls.size();
    registry.hulls.push_back(blueprint);
}

void ShipHullCatalog::Clear() {
    auto& registry = GetHullRegistry();
    registry.hulls.clear();
    registry.index.clear();
}

void ShipHullCatalog::EnsureDefaults() {
    EnsureHullDefaultsInitialized();
}

bool SlotSizeFits(SlotSize slotSize, SlotSize componentSize) {
    return SizeRank(componentSize) <= SizeRank(slotSize);
}

ShipAssemblyResult ShipAssembler::Assemble(const ShipAssemblyRequest& request) {
    ShipAssemblyResult result;

    const ShipHullBlueprint* hull = ShipHullCatalog::Find(request.hullId);
    if (!hull) {
        result.diagnostics.AddMessage(DiagnosticSeverity::Error, DiagnosticReasonCode::INVALID_HULL_ID,
                                      "Unknown hull id: " + request.hullId);
        return result;
    }

    result.hull = hull;
    result.performance.massTons = hull->baseMassTons;
    result.performance.totalThrustKN = 0.0;
    result.performance.mainThrustKN = 0.0;
    result.performance.maneuverThrustKN = 0.0;
    result.performance.powerOutputMW = 0.0;
    result.performance.powerDrawMW = 0.0;
    result.performance.heatGenerationMW = hull->baseHeatGenerationMW;
    result.performance.heatDissipationMW = hull->baseHeatDissipationMW;
    result.performance.crewRequired = hull->baseCrewRequired;
    result.performance.crewCapacity = hull->baseCrewCapacity;

    result.totalMassTons = result.performance.massTons;
    result.totalPowerOutputMW = result.performance.powerOutputMW;
    result.totalPowerDrawMW = result.performance.powerDrawMW;
    result.totalThrustKN = result.performance.totalThrustKN;
    result.mainThrustKN = result.performance.mainThrustKN;
    result.maneuverThrustKN = result.performance.maneuverThrustKN;
    result.totalHeatGenerationMW = result.performance.heatGenerationMW;
    result.totalHeatDissipationMW = result.performance.heatDissipationMW;
    result.crewRequired = result.performance.crewRequired;
    result.crewCapacity = result.performance.crewCapacity;

    std::unordered_map<std::string, const ShipComponentBlueprint*> resolvedComponents;
    std::vector<std::string> existingManufacturers;

    for (const auto& slot : hull->slots) {
        auto it = request.slotAssignments.find(slot.slotId);
        if (it == request.slotAssignments.end()) {
            if (slot.required) {
                result.diagnostics.AddMessage(DiagnosticSeverity::Error, DiagnosticReasonCode::SLOT_MISSING_REQUIRED_COMPONENT,
                                              "Required " + DescribeSlot(slot) + " has no assigned component.",
                                              slot.slotId);
                auto rankedSuggestions = FindRankedComponentSuggestions(slot, existingManufacturers);
                std::vector<std::string> componentIds;
                for (const auto& suggestion : rankedSuggestions) {
                    componentIds.push_back(suggestion.componentId);
                }
                result.diagnostics.AddSuggestion(slot.slotId, "Required slot empty", std::move(componentIds));
            } else {
                result.diagnostics.AddMessage(DiagnosticSeverity::Warning, DiagnosticReasonCode::SLOT_MISSING_REQUIRED_COMPONENT,
                                              "Optional " + DescribeSlot(slot) + " left unfilled.",
                                              slot.slotId);
            }
            continue;
        }
        const auto* blueprint = ShipComponentCatalog::Find(it->second);
        if (!blueprint) {
            result.diagnostics.AddMessage(DiagnosticSeverity::Error, DiagnosticReasonCode::COMPONENT_NOT_FOUND,
                                          "Unknown component id '" + it->second + "' assigned to " + DescribeSlot(slot) + ".",
                                          slot.slotId, {it->second});
            auto rankedSuggestions = FindRankedComponentSuggestions(slot, existingManufacturers);
            std::vector<std::string> componentIds;
            for (const auto& suggestion : rankedSuggestions) {
                componentIds.push_back(suggestion.componentId);
            }
            result.diagnostics.AddSuggestion(slot.slotId, "Component id not found", std::move(componentIds));
            continue;
        }

        // Track manufacturer for future suggestions
        if (!blueprint->manufacturer.empty()) {
            existingManufacturers.push_back(blueprint->manufacturer);
        }

        if (blueprint->category != slot.category) {
            std::ostringstream oss;
            oss << "Category mismatch: " << DescribeComponent(*blueprint)
                << " cannot occupy " << DescribeSlot(slot) << ".";
            result.diagnostics.AddMessage(DiagnosticSeverity::Error, DiagnosticReasonCode::SLOT_CATEGORY_MISMATCH,
                                          oss.str(), slot.slotId, {blueprint->id});
            auto rankedSuggestions = FindRankedComponentSuggestions(slot, existingManufacturers);
            std::vector<std::string> componentIds;
            for (const auto& suggestion : rankedSuggestions) {
                componentIds.push_back(suggestion.componentId);
            }
            result.diagnostics.AddSuggestion(slot.slotId, "Category mismatch", std::move(componentIds));
            continue;
        }
        if (!SlotSizeFits(slot.size, blueprint->size)) {
            std::ostringstream oss;
            oss << "Size mismatch: " << DescribeComponent(*blueprint)
                << " does not fit within " << DescribeSlot(slot) << ".";
            result.diagnostics.AddMessage(DiagnosticSeverity::Error, DiagnosticReasonCode::SLOT_SIZE_MISMATCH,
                                          oss.str(), slot.slotId, {blueprint->id});
            auto rankedSuggestions = FindRankedComponentSuggestions(slot, existingManufacturers);
            std::vector<std::string> componentIds;
            for (const auto& suggestion : rankedSuggestions) {
                componentIds.push_back(suggestion.componentId);
            }
            result.diagnostics.AddSuggestion(slot.slotId, "Size mismatch", std::move(componentIds));
            continue;
        }
        resolvedComponents[slot.slotId] = blueprint;
    }

    // Detect extra assignments not present on hull
    for (const auto& kv : request.slotAssignments) {
        auto found = std::find_if(hull->slots.begin(), hull->slots.end(), [&](const HullSlot& slot){ return slot.slotId == kv.first; });
        if (found == hull->slots.end()) {
            result.diagnostics.AddMessage(DiagnosticSeverity::Warning, DiagnosticReasonCode::SLOT_UNUSED_ASSIGNMENT,
                                          "Unused assignment for slot " + kv.first + " (slot not present on hull)",
                                          kv.first, {kv.second});
        }
    }

    if (result.diagnostics.HasErrors()) {
        return result;
    }

    // Apply soft compatibility rules after all components are resolved
    ApplySoftCompatibilityRules(result, resolvedComponents);

    for (const auto& slot : hull->slots) {
        auto resolvedIt = resolvedComponents.find(slot.slotId);
        if (resolvedIt == resolvedComponents.end()) {
            continue;
        }
        const auto* blueprint = resolvedIt->second;
        AssembledComponent assembled{slot.slotId, blueprint};
        result.components.push_back(assembled);
        result.performance.massTons += blueprint->massTons;
        result.performance.powerOutputMW += blueprint->powerOutputMW;
        result.performance.powerDrawMW += blueprint->powerDrawMW;
        result.performance.totalThrustKN += blueprint->thrustKN;
        result.performance.heatGenerationMW += blueprint->heatGenerationMW;
        result.performance.heatDissipationMW += blueprint->heatDissipationMW;
        result.performance.crewRequired += blueprint->crewRequired;
        result.performance.crewCapacity += blueprint->crewSupport;

        SubsystemSummary& summary = result.subsystems[blueprint->category];
        summary.category = blueprint->category;
        summary.components.push_back(assembled);
        summary.totalMassTons += blueprint->massTons;
        summary.totalPowerOutputMW += blueprint->powerOutputMW;
        summary.totalPowerDrawMW += blueprint->powerDrawMW;
        summary.totalThrustKN += blueprint->thrustKN;
        summary.totalHeatGenerationMW += blueprint->heatGenerationMW;
        summary.totalHeatDissipationMW += blueprint->heatDissipationMW;
        summary.crewRequired += blueprint->crewRequired;
        summary.crewSupport += blueprint->crewSupport;

        switch (blueprint->category) {
            case ComponentSlotCategory::MainThruster:
                result.performance.mainThrustKN += blueprint->thrustKN;
                break;
            case ComponentSlotCategory::ManeuverThruster:
                result.performance.maneuverThrustKN += blueprint->thrustKN;
                break;
            case ComponentSlotCategory::Sensor:
            case ComponentSlotCategory::Computer:
                result.avionicsModuleCount += 1;
                result.avionicsPowerDrawMW += blueprint->powerDrawMW;
                break;
            default:
                break;
        }
    }

    result.totalMassTons = result.performance.massTons;
    result.totalPowerOutputMW = result.performance.powerOutputMW;
    result.totalPowerDrawMW = result.performance.powerDrawMW;
    result.totalThrustKN = result.performance.totalThrustKN;
    result.mainThrustKN = result.performance.mainThrustKN;
    result.maneuverThrustKN = result.performance.maneuverThrustKN;
    result.totalHeatGenerationMW = result.performance.heatGenerationMW;
    result.totalHeatDissipationMW = result.performance.heatDissipationMW;
    result.crewRequired = result.performance.crewRequired;
    result.crewCapacity = result.performance.crewCapacity;

    result.availablePowerMW = result.performance.NetPowerMW();

    if (result.availablePowerMW < 0.0) {
        std::ostringstream oss;
        oss << "Net power deficit: output " << result.totalPowerOutputMW << " MW < draw " << result.totalPowerDrawMW << " MW";
        result.diagnostics.AddMessage(DiagnosticSeverity::Warning, DiagnosticReasonCode::PERFORMANCE_POWER_DEFICIT,
                                      oss.str());
    }

    if (result.NetHeatMW() < 0.0) {
        std::ostringstream oss;
        oss << "Heat accumulation risk: dissipation " << result.totalHeatDissipationMW
            << " MW < generation " << result.totalHeatGenerationMW << " MW";
        result.diagnostics.AddMessage(DiagnosticSeverity::Warning, DiagnosticReasonCode::PERFORMANCE_HEAT_ACCUMULATION,
                                      oss.str());
    }

    double crewUtilization = result.CrewUtilization();
    if (crewUtilization > 1.0 || !std::isfinite(crewUtilization)) {
        std::ostringstream oss;
        oss << "Crew shortfall: required " << result.crewRequired << " personnel, capacity " << result.crewCapacity;
        result.diagnostics.AddMessage(DiagnosticSeverity::Warning, DiagnosticReasonCode::PERFORMANCE_CREW_SHORTFALL,
                                      oss.str());
    }

    return result;
}

const SubsystemSummary* ShipAssemblyResult::GetSubsystem(ComponentSlotCategory category) const {
    auto it = subsystems.find(category);
    if (it == subsystems.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string ShipAssemblyResult::Serialize() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"hull\":\"" << (hull ? hull->id : "") << "\"";
    oss << ",\"components\":[";
    for (std::size_t i = 0; i < components.size(); ++i) {
        const auto& comp = components[i];
        oss << "{\"slot\":\"" << comp.slotId << "\",\"component\":\"" << (comp.blueprint ? comp.blueprint->id : "") << "\"}";
        if (i + 1 < components.size()) oss << ",";
    }
    oss << "]";
    oss << ",\"stats\":{"
        << "\"massTons\":" << totalMassTons << ","
        << "\"powerOutputMW\":" << totalPowerOutputMW << ","
        << "\"powerDrawMW\":" << totalPowerDrawMW << ","
        << "\"netPowerMW\":" << NetPowerMW() << ","
        << "\"thrustKN\":" << totalThrustKN << ","
        << "\"mainThrustKN\":" << mainThrustKN << ","
        << "\"maneuverThrustKN\":" << maneuverThrustKN << ","
        << "\"avionicsModules\":" << avionicsModuleCount << ","
        << "\"avionicsPowerDrawMW\":" << avionicsPowerDrawMW << ","
        << "\"thrustToMass\":" << ThrustToMassRatio() << ","
        << "\"heatGenerationMW\":" << totalHeatGenerationMW << ","
        << "\"heatDissipationMW\":" << totalHeatDissipationMW << ","
        << "\"netHeatMW\":" << NetHeatMW() << ","
        << "\"crewRequired\":" << crewRequired << ","
        << "\"crewCapacity\":" << crewCapacity << ","
        << "\"crewUtilization\":" << CrewUtilization() << "}";

    if (!subsystems.empty()) {
        oss << ",\"subsystems\":{";
        bool firstSubsystem = true;
        for (const auto& entry : subsystems) {
            if (!firstSubsystem) {
                oss << ",";
            }
            firstSubsystem = false;
            const auto category = entry.first;
            const auto& summary = entry.second;
            oss << "\"" << ToString(category) << "\":{";
            oss << "\"massTons\":" << summary.totalMassTons << ",";
            oss << "\"powerOutputMW\":" << summary.totalPowerOutputMW << ",";
            oss << "\"powerDrawMW\":" << summary.totalPowerDrawMW << ",";
            oss << "\"thrustKN\":" << summary.totalThrustKN << ",";
            oss << "\"heatGenerationMW\":" << summary.totalHeatGenerationMW << ",";
            oss << "\"heatDissipationMW\":" << summary.totalHeatDissipationMW << ",";
            oss << "\"crewRequired\":" << summary.crewRequired << ",";
            oss << "\"crewSupport\":" << summary.crewSupport << ",";
            oss << "\"components\":[";
            for (std::size_t i = 0; i < summary.components.size(); ++i) {
                const auto& comp = summary.components[i];
                oss << "{\"slot\":\"" << comp.slotId << "\",\"component\":\"" << (comp.blueprint ? comp.blueprint->id : "") << "\"}";
                if (i + 1 < summary.components.size()) {
                    oss << ",";
                }
            }
            oss << "]}";
        }
        oss << "}";
    }
    if (!diagnostics.errors.empty() || !diagnostics.warnings.empty() || !diagnostics.suggestions.empty()) {
        oss << ",\"diagnostics\":{";
        oss << "\"errors\":[";
        for (std::size_t i = 0; i < diagnostics.errors.size(); ++i) {
            oss << "\"" << diagnostics.errors[i] << "\"";
            if (i + 1 < diagnostics.errors.size()) oss << ",";
        }
        oss << "]";
        oss << ",\"warnings\":[";
        for (std::size_t i = 0; i < diagnostics.warnings.size(); ++i) {
            oss << "\"" << diagnostics.warnings[i] << "\"";
            if (i + 1 < diagnostics.warnings.size()) oss << ",";
        }
        oss << "]";
        if (!diagnostics.suggestions.empty()) {
            oss << ",\"suggestions\":[";
            for (std::size_t i = 0; i < diagnostics.suggestions.size(); ++i) {
                const auto& suggestion = diagnostics.suggestions[i];
                oss << "{\"slot\":\"" << suggestion.slotId
                    << "\",\"reason\":\"" << suggestion.reason << "\",\"components\":[";
                for (std::size_t j = 0; j < suggestion.suggestedComponentIds.size(); ++j) {
                    oss << "\"" << suggestion.suggestedComponentIds[j] << "\"";
                    if (j + 1 < suggestion.suggestedComponentIds.size()) {
                        oss << ",";
                    }
                }
                oss << "]";
                if (i + 1 < diagnostics.suggestions.size()) {
                    oss << ",";
                }
            }
            oss << "]";
        }
        oss << "}";
    }
    oss << "}";
    return oss.str();
}
