#pragma once

#include <string>
#include <vector>

// Core taxonomy enumerations

enum class SpaceshipClassType {
    Fighter,
    Freighter,
    Explorer,
    Industrial,
    Capital
};

enum class HardpointCategory {
    PrimaryWeapon,
    Utility,
    Module
};

enum class ComponentSlotCategory {
    PowerPlant,
    MainThruster,
    ManeuverThruster,
    Shield,
    Weapon,
    Cargo,
    CrewQuarters,
    Sensor,
    Industrial,
    Support,
    Hangar,
    Computer
};

enum class SlotSize {
    XS,
    Small,
    Medium,
    Large,
    XL,
    XXL
};

struct HardpointSpec {
    HardpointCategory category;
    SlotSize size;
    int count;
    std::string notes;
};

struct ComponentSlotSpec {
    ComponentSlotCategory category;
    SlotSize size;
    int count;
    std::string notes;
};

struct BaselineStats {
    float minMassTons;
    float maxMassTons;
    int minCrew;
    int maxCrew;
    float minPowerBudgetMW;
    float maxPowerBudgetMW;
};

struct ProgressionTier {
    int tier;
    std::string name;
    std::string description;
};

struct FactionVariant {
    std::string faction;
    std::string codename;
    std::string description;
};

struct ConceptBrief {
    std::string elevatorPitch;
    std::vector<std::string> gameplayHooks;
};

struct SpaceshipClassDefinition {
    SpaceshipClassType type;
    std::string displayName;
    ConceptBrief conceptSummary;
    BaselineStats baseline;
    std::vector<HardpointSpec> hardpoints;
    std::vector<ComponentSlotSpec> componentSlots;
    std::vector<ProgressionTier> progression;
    std::vector<FactionVariant> variants;
};

// Registry exposing taxonomy to the rest of the engine
class SpaceshipCatalog {
public:
    static const SpaceshipClassDefinition& GetDefinition(SpaceshipClassType type);
    static const std::vector<SpaceshipClassDefinition>& All();
};

// Utility helpers
std::string ToString(SpaceshipClassType type);
std::string ToString(HardpointCategory category);
std::string ToString(ComponentSlotCategory category);
std::string ToString(SlotSize size);
