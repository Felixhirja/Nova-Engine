#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

// -----------------------------------------------------------------------------
// Ship assembly core enums and helpers (self-contained; no external deps)
// -----------------------------------------------------------------------------

// High-level ship class taxonomy (minimal set for now)
enum class SpaceshipClassType {
    Fighter,
    Freighter,
    Explorer,
    Industrial,
    Corvette,
    Cruiser,
    Capital
};

// Slot size standardization for component fit checks
enum class SlotSize { XS, Small, Medium, Large, XL, XXL };

// Component categories that can occupy hull slots
enum class ComponentSlotCategory {
    PowerPlant,
    MainThruster,
    ManeuverThruster,
    Shield,
    Weapon,
    Sensor,
    Support,
    Cargo,
    CrewQuarters,
    Industrial,
    Hangar,
    Computer
};

// Human-readable conversions
inline const char* ToString(SpaceshipClassType t) {
    switch (t) {
        case SpaceshipClassType::Fighter:  return "Fighter";
        case SpaceshipClassType::Freighter:return "Freighter";
        case SpaceshipClassType::Explorer: return "Explorer";
        case SpaceshipClassType::Industrial: return "Industrial";
        case SpaceshipClassType::Corvette: return "Corvette";
        case SpaceshipClassType::Cruiser:  return "Cruiser";
        case SpaceshipClassType::Capital:  return "Capital";
    }
    return "Unknown";
}

inline const char* ToString(SlotSize s) {
    switch (s) {
        case SlotSize::XS:     return "XS";
        case SlotSize::Small:  return "Small";
        case SlotSize::Medium: return "Medium";
        case SlotSize::Large:  return "Large";
        case SlotSize::XL:     return "XL";
        case SlotSize::XXL:    return "XXL";
    }
    return "Unknown";
}

inline const char* ToString(ComponentSlotCategory c) {
    switch (c) {
        case ComponentSlotCategory::PowerPlant:       return "PowerPlant";
        case ComponentSlotCategory::MainThruster:     return "MainThruster";
        case ComponentSlotCategory::ManeuverThruster: return "ManeuverThruster";
        case ComponentSlotCategory::Shield:           return "Shield";
        case ComponentSlotCategory::Weapon:           return "Weapon";
        case ComponentSlotCategory::Sensor:           return "Sensor";
        case ComponentSlotCategory::Support:          return "Support";
        case ComponentSlotCategory::Cargo:            return "Cargo";
        case ComponentSlotCategory::CrewQuarters:     return "CrewQuarters";
        case ComponentSlotCategory::Industrial:       return "Industrial";
        case ComponentSlotCategory::Hangar:           return "Hangar";
        case ComponentSlotCategory::Computer:         return "Computer";
    }
    return "Unknown";
}

// -----------------------------------------------------------------------------
// Minimal Spaceship class definition schema (used by ExpandDefinition)
// -----------------------------------------------------------------------------

struct SpaceshipBaselineSpec {
    double minMassTons = 0.0;
    double maxMassTons = 0.0;
    int minCrew = 0;
    int maxCrew = 0;
    double minPowerBudgetMW = 0.0;
    double maxPowerBudgetMW = 0.0;
};

struct ComponentSlotSpec {
    ComponentSlotCategory category = ComponentSlotCategory::Support;
    SlotSize size = SlotSize::Small;
    int count = 1;
    std::string notes;
};

struct ShipAssemblyRequest;

struct SpaceshipClassDefinition {
    SpaceshipClassType type = SpaceshipClassType::Fighter;
    std::string displayName;
    SpaceshipBaselineSpec baseline;
    std::vector<ComponentSlotSpec> componentSlots;
    std::vector<ShipAssemblyRequest> defaultLoadouts;
};

// Diagnostic severity levels for ship assembly validation
enum class DiagnosticSeverity {
    Error,   // Fatal issues that prevent assembly
    Warning, // Advisory issues that may cause problems
    Info     // Informational messages and suggestions
};

// Structured reason codes for diagnostic messages
enum class DiagnosticReasonCode {
    // Hull-related errors
    INVALID_HULL_ID,
    HULL_NOT_FOUND,

    // Slot-related errors
    SLOT_MISSING_REQUIRED_COMPONENT,
    SLOT_UNUSED_ASSIGNMENT,
    SLOT_CATEGORY_MISMATCH,
    SLOT_SIZE_MISMATCH,

    // Component-related errors
    COMPONENT_NOT_FOUND,
    COMPONENT_UNKNOWN_ID,

    // Performance-related warnings
    PERFORMANCE_POWER_DEFICIT,
    PERFORMANCE_HEAT_ACCUMULATION,
    PERFORMANCE_CREW_SHORTFALL,

    // Compatibility warnings (soft rules)
    COMPATIBILITY_MANUFACTURER_MISMATCH,
    COMPATIBILITY_POWER_ENVELOPE_MISMATCH,
    COMPATIBILITY_SLOT_ADJACENCY_ISSUE,

    // Suggestions
    SUGGESTION_COMPATIBLE_REPLACEMENT,
    SUGGESTION_SIZE_UPGRADE,
    SUGGESTION_POWER_OPTIMIZATION
};

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

    // Schema versioning and compatibility metadata
    int schemaVersion = 1;                    // Blueprint schema version for compatibility
    int techTier = 1;                         // Technology tier requirement (1 = basic, higher = advanced)
    std::string manufacturer;                  // Component manufacturer (e.g., "Nova Dynamics", "Stellar Forge")
    std::string manufacturerLineage;           // Manufacturer product line (e.g., "Mk.I", "Enterprise", "Titan")
    std::vector<std::string> factionRestrictions; // Faction IDs that can use this component (empty = all factions)

    // Power envelope compatibility (for non-power-plant components)
    double minPowerEnvelopeMW = 0.0;          // Minimum reactor power output this component works well with
    double maxPowerEnvelopeMW = 1000.0;       // Maximum reactor power output this component works well with
    double optimalPowerEnvelopeMW = 50.0;     // Optimal reactor power output for this component

    // Adjacency requirements (for components that need to be near other types)
    std::vector<ComponentSlotCategory> requiredAdjacentSlots; // Component types that should be adjacent
    std::vector<ComponentSlotCategory> incompatibleAdjacentSlots; // Component types that should NOT be adjacent

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
    std::vector<std::string> adjacentSlotIds; // IDs of slots that are physically adjacent to this one
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

// Structured diagnostic message with severity and reason code
struct DiagnosticMessage {
    DiagnosticSeverity severity;
    DiagnosticReasonCode reasonCode;
    std::string message;
    std::string slotId; // Empty if not slot-specific
    std::vector<std::string> relatedComponents; // Component IDs related to this diagnostic
};

// Ranked component suggestion with fit score
struct RankedComponentSuggestion {
    std::string componentId;
    double fitScore; // 0.0 to 1.0, higher is better fit
    std::string reasoning; // Why this component is suggested
};

struct ComponentSuggestion {
    std::string slotId;
    std::string reason;
    std::vector<std::string> suggestedComponentIds;
    std::vector<RankedComponentSuggestion> rankedSuggestions; // New ranked suggestions
};

struct ShipAssemblyDiagnostics {
    std::vector<std::string> errors; // Legacy support - will be deprecated
    std::vector<std::string> warnings; // Legacy support - will be deprecated
    std::vector<ComponentSuggestion> suggestions;
    std::vector<DiagnosticMessage> messages; // New structured messages

    void AddError(const std::string& msg);
    void AddWarning(const std::string& msg);
    void AddSuggestion(const std::string& slotId,
                       const std::string& reason,
                       std::vector<std::string> suggestedComponentIds);
    void AddMessage(DiagnosticSeverity severity, DiagnosticReasonCode reasonCode,
                    const std::string& message, const std::string& slotId = "",
                    const std::vector<std::string>& relatedComponents = {});
    std::vector<std::string> BuildUserFacingMessages(const ShipHullBlueprint* hull = nullptr) const;
    std::vector<DiagnosticMessage> GetStructuredMessages() const { return messages; }
    bool HasErrors() const {
        if (!errors.empty()) return true;
        for (const auto& msg : messages) {
            if (msg.severity == DiagnosticSeverity::Error) return true;
        }
        return false;
    }
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
    static void Reload(); // Hot-reload components from JSON files
    static void EnsureDefaults(); // Made public for debugging
private:
};

class ShipHullCatalog {
public:
    static const ShipHullBlueprint* Find(const std::string& id);
    static const ShipHullBlueprint& Get(const std::string& id);
    static const std::vector<ShipHullBlueprint>& All();
    static void Register(const ShipHullBlueprint& blueprint);
    static void Clear();
    static void EnsureDefaults(); // Made public for debugging
private:
};

class ShipAssembler {
public:
    static ShipAssemblyResult Assemble(const ShipAssemblyRequest& request);
};

bool SlotSizeFits(SlotSize slotSize, SlotSize componentSize);
std::vector<std::string> FindCompatibleComponentIds(const HullSlot& slot, std::size_t limit = 5);
std::vector<RankedComponentSuggestion> FindRankedComponentSuggestions(const HullSlot& slot,
                                                                       const std::vector<std::string>& existingManufacturers = {},
                                                                       std::size_t limit = 5);
