#pragma once

#include "Spaceship.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

// Blueprint describing an individual ship component that can occupy a slot
struct ShipComponentBlueprint {
    std::string id;
    std::string displayName;
    std::string description;
    ComponentSlotCategory category;
    SlotSize size;
    double massTons = 0.0;
    double powerOutputMW = 0.0;
    double powerDrawMW = 0.0;
    double thrustKN = 0.0;
    double heatGenerationMW = 0.0;
    double heatDissipationMW = 0.0;
    int crewRequired = 0;
    int crewSupport = 0;
    // Weapon-specific fields (only relevant if category == Weapon)
    double weaponDamagePerShot = 0.0;
    double weaponRangeKm = 0.0;
    double weaponFireRatePerSecond = 0.0;
    int weaponAmmoCapacity = 0;
    std::string weaponAmmoType; // "projectile", "energy", "missile", etc.
    bool weaponIsTurret = false;
    double weaponTrackingSpeedDegPerSec = 0.0;
    double weaponProjectileSpeedKmPerSec = 0.0;
    // Shield-specific fields (only relevant if category == Shield)
    double shieldCapacityMJ = 0.0;           // Maximum shield energy in megajoules
    double shieldRechargeRateMJPerSec = 0.0; // Shield recharge rate per second
    double shieldRechargeDelaySeconds = 0.0; // Delay before recharge starts after taking damage
    double shieldDamageAbsorption = 1.0;     // Fraction of damage absorbed (0.0-1.0, 1.0 = full absorption)
};

// Expanded, uniquely identified slot on a hull
struct HullSlot {
    std::string slotId;
    ComponentSlotCategory category;
    SlotSize size;
    std::string notes;
    bool required = true;
};

// Definition for an assemble-able hull archetype
struct ShipHullBlueprint {
    std::string id;
    SpaceshipClassType classType;
    std::string displayName;
    double baseMassTons = 0.0;
    double structuralIntegrity = 0.0;
    std::vector<HullSlot> slots;
    int baseCrewRequired = 0;
    int baseCrewCapacity = 0;
    double baseHeatGenerationMW = 0.0;
    double baseHeatDissipationMW = 0.0;
};

struct ShipAssemblyRequest {
    std::string hullId;
    std::unordered_map<std::string, std::string> slotAssignments; // slotId -> componentId
};

struct ComponentSuggestion {
    std::string slotId;
    std::string reason;
    std::vector<std::string> suggestedComponentIds;
};

struct ShipAssemblyDiagnostics {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<ComponentSuggestion> suggestions;

    void AddError(const std::string& msg);
    void AddWarning(const std::string& msg);
    void AddSuggestion(const std::string& slotId,
                       const std::string& reason,
                       std::vector<std::string> suggestedComponentIds);
    bool HasErrors() const { return !errors.empty(); }
};

struct AssembledComponent {
    std::string slotId;
    const ShipComponentBlueprint* blueprint = nullptr;
};

struct ComponentSlotCategoryHash {
    std::size_t operator()(ComponentSlotCategory category) const noexcept {
        return static_cast<std::size_t>(category);
    }
};

struct SubsystemSummary {
    ComponentSlotCategory category;
    std::vector<AssembledComponent> components;
    double totalMassTons = 0.0;
    double totalPowerOutputMW = 0.0;
    double totalPowerDrawMW = 0.0;
    double totalThrustKN = 0.0;
    double totalHeatGenerationMW = 0.0;
    double totalHeatDissipationMW = 0.0;
    int crewRequired = 0;
    int crewSupport = 0;
};

struct ShipPerformanceMetrics {
    double massTons = 0.0;
    double totalThrustKN = 0.0;
    double mainThrustKN = 0.0;
    double maneuverThrustKN = 0.0;
    double powerOutputMW = 0.0;
    double powerDrawMW = 0.0;
    double heatGenerationMW = 0.0;
    double heatDissipationMW = 0.0;
    int crewRequired = 0;
    int crewCapacity = 0;

    double NetPowerMW() const { return powerOutputMW - powerDrawMW; }
    double NetHeatMW() const { return heatDissipationMW - heatGenerationMW; }
    double ThrustToMassRatio() const { return massTons > 0.0 ? totalThrustKN / massTons : 0.0; }
    double CrewUtilization() const {
        if (crewCapacity <= 0) {
            return crewRequired > 0 ? std::numeric_limits<double>::infinity() : 0.0;
        }
        return static_cast<double>(crewRequired) / static_cast<double>(crewCapacity);
    }
};

struct ShipAssemblyResult {
    const ShipHullBlueprint* hull = nullptr;
    std::vector<AssembledComponent> components;
    double totalMassTons = 0.0;
    double totalPowerOutputMW = 0.0;
    double totalPowerDrawMW = 0.0;
    double totalThrustKN = 0.0;
    double availablePowerMW = 0.0;
    double mainThrustKN = 0.0;
    double maneuverThrustKN = 0.0;
    double totalHeatGenerationMW = 0.0;
    double totalHeatDissipationMW = 0.0;
    int crewRequired = 0;
    int crewCapacity = 0;
    int avionicsModuleCount = 0;
    double avionicsPowerDrawMW = 0.0;
    std::unordered_map<ComponentSlotCategory, SubsystemSummary, ComponentSlotCategoryHash> subsystems;
    ShipAssemblyDiagnostics diagnostics;
    ShipPerformanceMetrics performance;

    bool IsValid() const { return hull != nullptr && !diagnostics.HasErrors(); }
    double NetPowerMW() const { return performance.NetPowerMW(); }
    double ThrustToMassRatio() const { return performance.ThrustToMassRatio(); }
    double NetHeatMW() const { return performance.NetHeatMW(); }
    double CrewUtilization() const { return performance.CrewUtilization(); }
    bool HasSubsystem(ComponentSlotCategory category) const { return subsystems.find(category) != subsystems.end(); }
    const SubsystemSummary* GetSubsystem(ComponentSlotCategory category) const;
    std::string Serialize() const;
};

class ShipComponentCatalog {
public:
    static const ShipComponentBlueprint* Find(const std::string& id);
    static const ShipComponentBlueprint& Get(const std::string& id);
    static const std::vector<ShipComponentBlueprint>& All();
    static void Register(const ShipComponentBlueprint& blueprint);
    static void Clear();
private:
    static void EnsureDefaults();
};

class ShipHullCatalog {
public:
    static const ShipHullBlueprint* Find(const std::string& id);
    static const ShipHullBlueprint& Get(const std::string& id);
    static const std::vector<ShipHullBlueprint>& All();
    static void Register(const ShipHullBlueprint& blueprint);
    static void Clear();
private:
    static void EnsureDefaults();
};

class ShipAssembler {
public:
    static ShipAssemblyResult Assemble(const ShipAssemblyRequest& request);
};

bool SlotSizeFits(SlotSize slotSize, SlotSize componentSize);
