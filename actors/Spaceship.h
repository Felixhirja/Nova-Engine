#pragma once

#include "ecs/ShipAssembly.h"

#include <optional>
#include <string>
#include <vector>

// Runtime catalog describing the high level spaceship classes that the game
// exposes to tools, UI, and simulation systems.  The catalog intentionally
// mirrors the design documentation located under docs/spaceship_taxonomy.md so
// that designers can iterate on paper without drifting from the runtime data.

enum class HardpointCategory {
    PrimaryWeapon,
    Utility,
    Module
};

struct HardpointSpec {
    HardpointCategory category = HardpointCategory::PrimaryWeapon;
    SlotSize size = SlotSize::Small;
    int count = 0;
    std::string notes;
};

struct SpaceshipConceptSummary {
    std::string elevatorPitch;
    std::vector<std::string> gameplayHooks;
};

struct ProgressionTier {
    int tier = 0;
    std::string name;
    std::string description;
};

struct PassiveBuff {
    std::string type;
    double value = 0.0;
};

struct HardpointDelta {
    HardpointCategory category = HardpointCategory::PrimaryWeapon;
    std::optional<SlotSize> sizeDelta;
    int countDelta = 0;
};

struct SlotDelta {
    ComponentSlotCategory category = ComponentSlotCategory::Support;
    std::optional<SlotSize> size;
    int countDelta = 0;
};

struct VariantSpec {
    std::string faction;
    std::string codename;
    std::string description;
    std::vector<HardpointDelta> hardpointDeltas;
    std::vector<SlotDelta> slotDeltas;
    std::vector<PassiveBuff> passiveBuffs;
};

struct ProgressionMetadata {
    int minLevel = 0;
    int factionReputation = 0;
    int blueprintCost = 0;
};

struct DefaultLoadout {
    std::string name;
    std::string description;
    std::vector<std::string> components;
};

struct SpaceshipClassCatalogEntry {
    std::string id;
    SpaceshipClassType type = SpaceshipClassType::Fighter;
    std::string displayName;
    SpaceshipConceptSummary conceptSummary;
    SpaceshipBaselineSpec baseline;
    std::vector<HardpointSpec> hardpoints;
    std::vector<ComponentSlotSpec> componentSlots;
    std::vector<ProgressionTier> progression;
    std::vector<VariantSpec> variants;
    ProgressionMetadata progressionMetadata;
    std::vector<DefaultLoadout> defaultLoadouts;
};

class SpaceshipCatalog {
public:
    static const std::vector<SpaceshipClassCatalogEntry>& All();
    static const SpaceshipClassCatalogEntry* FindById(const std::string& id);
};

