#include "ShipAssembly.h"

#include "Spaceship.h"

#include <algorithm>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <iomanip>
#include <cmath>

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
            slot.slotId = ToString(spec.category) + "_" + std::to_string(i);
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

    auto add = [&](const ShipComponentBlueprint& blueprint) {
        registry.index[blueprint.id] = registry.components.size();
        registry.components.push_back(blueprint);
    };

    add({"fusion_core_mk1", "Fusion Core Mk.I", "Baseline fighter fusion core.", ComponentSlotCategory::PowerPlant, SlotSize::Small, 6.5, 10.0, 0.2, 0.0, 2.5, 1.5, 1, 0});
    add({"fusion_core_mk2", "Fusion Core Mk.II", "Enhanced output for larger hulls.", ComponentSlotCategory::PowerPlant, SlotSize::Medium, 11.0, 18.0, 0.3, 0.0, 6.0, 2.5, 2, 0});
    add({"main_thruster_viper", "Viper Main Thruster", "High thrust ratio for fighters.", ComponentSlotCategory::MainThruster, SlotSize::Small, 4.0, 0.0, 4.0, 220.0, 5.0, 1.0, 0, 0});
    add({"main_thruster_freighter", "Atlas Drive", "Cargo-optimized main thruster.", ComponentSlotCategory::MainThruster, SlotSize::Medium, 12.0, 0.0, 6.0, 320.0, 10.0, 2.0, 1, 0});
    add({"rcs_cluster_micro", "Micro RCS Cluster", "Reaction control thrusters for fine maneuvers.", ComponentSlotCategory::ManeuverThruster, SlotSize::XS, 0.8, 0.0, 0.5, 35.0, 0.3, 0.3, 0, 0});
    add({"shield_array_light", "Light Shield Array", "Directional shield generator for fighters.", ComponentSlotCategory::Shield, SlotSize::Small, 3.2, 0.0, 2.5, 0.0, 3.0, 0.5, 0, 0, 0.0, 0.0, 0.0, 0, "", false, 0.0, 0.0, 150.0, 5.0, 3.0, 0.8});
    add({"shield_array_medium", "Medium Shield Array", "Balanced shield system for freighters and explorers.", ComponentSlotCategory::Shield, SlotSize::Medium, 6.5, 0.0, 4.0, 0.0, 5.0, 1.0, 1, 0, 0.0, 0.0, 0.0, 0, "", false, 0.0, 0.0, 300.0, 8.0, 4.0, 0.85});
    add({"shield_array_heavy", "Heavy Shield Array", "Capital-grade shield with rapid recharge.", ComponentSlotCategory::Shield, SlotSize::Large, 12.0, 0.0, 8.0, 0.0, 10.0, 2.0, 2, 0, 0.0, 0.0, 0.0, 0, "", false, 0.0, 0.0, 600.0, 12.0, 5.0, 0.9});
    add({"weapon_cooling_cannon", "Cannon Cooling Rack", "Stabilizes twin cannon mounts.", ComponentSlotCategory::Weapon, SlotSize::Small, 2.8, 0.0, 1.5, 0.0, 0.8, 1.2, 0, 0, 0.0, 0.0, 0.0, 0, "", false, 0.0, 0.0});
    add({"weapon_twin_cannon", "Twin Cannon", "Rapid-fire projectile weapon for fighters.", ComponentSlotCategory::Weapon, SlotSize::Small, 3.5, 0.0, 2.0, 0.0, 2.5, 1.0, 0, 0, 15.0, 5.0, 10.0, 200, "projectile", false, 0.0, 2.0});
    add({"weapon_missile_launcher", "Missile Launcher", "Guided missile system for fighters.", ComponentSlotCategory::Weapon, SlotSize::Small, 4.0, 0.0, 3.0, 0.0, 3.0, 1.5, 0, 0, 50.0, 10.0, 2.0, 8, "missile", false, 0.0, 1.5});
    add({"weapon_defensive_turret", "Defensive Turret", "Rotating cannon for freighters and explorers.", ComponentSlotCategory::Weapon, SlotSize::Medium, 8.0, 0.0, 4.0, 0.0, 4.0, 2.0, 1, 0, 20.0, 8.0, 5.0, 100, "projectile", true, 60.0, 1.8});
    add({"weapon_beam_array", "Beam Array", "Energy weapon for capital ships.", ComponentSlotCategory::Weapon, SlotSize::Large, 12.0, 0.0, 8.0, 0.0, 10.0, 3.0, 2, 0, 30.0, 15.0, 1.0, 50, "energy", true, 30.0, 300.0});
    add({"cargo_rack_standard", "Cargo Rack", "Standard modular cargo rack.", ComponentSlotCategory::Cargo, SlotSize::Large, 15.0, 0.0, 1.0, 0.0, 0.4, 0.5, 2, 0});
    add({"support_life_pod", "Emergency Life Support Pod", "Sustains crew during hull breaches.", ComponentSlotCategory::Support, SlotSize::XS, 1.2, 0.0, 0.6, 0.0, 0.1, 0.5, 0, 2});
    add({"sensor_targeting_mk1", "Combat Sensor Suite", "Targeting computer with enhanced tracking.", ComponentSlotCategory::Sensor, SlotSize::Small, 1.4, 0.0, 1.2, 0.0, 1.5, 0.5, 0, 0});
}

void RegisterDefaultHulls() {
    auto& registry = GetHullRegistry();
    registry.hulls.clear();
    registry.index.clear();

    const auto& fighter = SpaceshipCatalog::GetDefinition(SpaceshipClassType::Fighter);
    ShipHullBlueprint fighterHull = ExpandDefinition(fighter, "fighter_mk1");
    registry.index[fighterHull.id] = registry.hulls.size();
    registry.hulls.push_back(std::move(fighterHull));

    const auto& freighter = SpaceshipCatalog::GetDefinition(SpaceshipClassType::Freighter);
    ShipHullBlueprint freighterHull = ExpandDefinition(freighter, "freighter_mk1");
    registry.index[freighterHull.id] = registry.hulls.size();
    registry.hulls.push_back(std::move(freighterHull));
}

} // namespace

void ShipAssemblyDiagnostics::AddError(const std::string& msg) {
    errors.push_back(msg);
}

void ShipAssemblyDiagnostics::AddWarning(const std::string& msg) {
    warnings.push_back(msg);
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
        result.diagnostics.AddError("Unknown hull id: " + request.hullId);
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

    for (const auto& slot : hull->slots) {
        auto it = request.slotAssignments.find(slot.slotId);
        if (it == request.slotAssignments.end()) {
            if (slot.required) {
                result.diagnostics.AddError("Missing component for slot " + slot.slotId);
            } else {
                result.diagnostics.AddWarning("Optional slot " + slot.slotId + " left unfilled");
            }
            continue;
        }
        const auto* blueprint = ShipComponentCatalog::Find(it->second);
        if (!blueprint) {
            result.diagnostics.AddError("Unknown component id " + it->second + " for slot " + slot.slotId);
            continue;
        }
        if (blueprint->category != slot.category) {
            result.diagnostics.AddError("Component " + blueprint->id + " incompatible with slot " + slot.slotId + " (category mismatch)");
            continue;
        }
        if (!SlotSizeFits(slot.size, blueprint->size)) {
            result.diagnostics.AddError("Component " + blueprint->id + " too large for slot " + slot.slotId);
            continue;
        }
        resolvedComponents[slot.slotId] = blueprint;
    }

    // Detect extra assignments not present on hull
    for (const auto& kv : request.slotAssignments) {
        auto found = std::find_if(hull->slots.begin(), hull->slots.end(), [&](const HullSlot& slot){ return slot.slotId == kv.first; });
        if (found == hull->slots.end()) {
            result.diagnostics.AddWarning("Unused assignment for slot " + kv.first + " (slot not present on hull)");
        }
    }

    if (result.diagnostics.HasErrors()) {
        return result;
    }

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
        result.diagnostics.AddWarning(oss.str());
    }

    if (result.NetHeatMW() < 0.0) {
        std::ostringstream oss;
        oss << "Heat accumulation risk: dissipation " << result.totalHeatDissipationMW
            << " MW < generation " << result.totalHeatGenerationMW << " MW";
        result.diagnostics.AddWarning(oss.str());
    }

    double crewUtilization = result.CrewUtilization();
    if (crewUtilization > 1.0 || !std::isfinite(crewUtilization)) {
        std::ostringstream oss;
        oss << "Crew shortfall: required " << result.crewRequired << " personnel, capacity " << result.crewCapacity;
        result.diagnostics.AddWarning(oss.str());
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
    if (!diagnostics.errors.empty() || !diagnostics.warnings.empty()) {
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
        oss << "]}";
    }
    oss << "}";
    return oss.str();
}
