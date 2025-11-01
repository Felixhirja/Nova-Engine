#include "SpaceshipCatalog.h"

#include "SimpleJson.h"
#include "../ecs/ShipAssembly.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_map>

namespace {

using FileTimePoint = std::filesystem::file_time_type;

struct RangeConstraint {
    double minValue;
    double maxValue;
};

struct HardpointConstraint {
    SlotSize expectedSize;
    int expectedCount;
};

struct SlotConstraint {
    SlotSize expectedSize;
    int expectedCount;
};

struct TaxonomyConstraint {
    RangeConstraint massTons;
    RangeConstraint crew;
    RangeConstraint powerBudget;
    std::unordered_map<HardpointCategory, HardpointConstraint> hardpoints;
    std::unordered_map<ComponentSlotCategory, SlotConstraint> slots;
};

struct CatalogState {
    std::vector<SpaceshipClassCatalogEntry> entries;
    std::vector<std::string> validationErrors;
    bool loaded = false;
    bool hotReloadEnabled = false;
    std::unordered_map<std::string, FileTimePoint> fileTimes;
};

CatalogState& State() {
    static CatalogState state;
    return state;
}

const std::filesystem::path kCatalogDirectory{"assets/ships"};

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::optional<SpaceshipClassType> ParseClassType(const std::string& value) {
    std::string lower = ToLower(value);
    if (lower == "fighter") return SpaceshipClassType::Fighter;
    if (lower == "freighter") return SpaceshipClassType::Freighter;
    if (lower == "explorer") return SpaceshipClassType::Explorer;
    if (lower == "industrial") return SpaceshipClassType::Industrial;
    if (lower == "corvette") return SpaceshipClassType::Corvette;
    if (lower == "cruiser") return SpaceshipClassType::Cruiser;
    if (lower == "capital") return SpaceshipClassType::Capital;
    return std::nullopt;
}

std::optional<HardpointCategory> ParseHardpointCategory(const std::string& value) {
    std::string lower = ToLower(value);
    if (lower == "primaryweapon") return HardpointCategory::PrimaryWeapon;
    if (lower == "utility") return HardpointCategory::Utility;
    if (lower == "module") return HardpointCategory::Module;
    return std::nullopt;
}

std::optional<ComponentSlotCategory> ParseSlotCategory(const std::string& value) {
    std::string lower = ToLower(value);
    if (lower == "powerplant") return ComponentSlotCategory::PowerPlant;
    if (lower == "mainthruster") return ComponentSlotCategory::MainThruster;
    if (lower == "maneuverthruster") return ComponentSlotCategory::ManeuverThruster;
    if (lower == "shield") return ComponentSlotCategory::Shield;
    if (lower == "weapon") return ComponentSlotCategory::Weapon;
    if (lower == "sensor") return ComponentSlotCategory::Sensor;
    if (lower == "support") return ComponentSlotCategory::Support;
    if (lower == "cargo") return ComponentSlotCategory::Cargo;
    if (lower == "crewquarters") return ComponentSlotCategory::CrewQuarters;
    if (lower == "industrial") return ComponentSlotCategory::Industrial;
    if (lower == "hangar") return ComponentSlotCategory::Hangar;
    if (lower == "computer") return ComponentSlotCategory::Computer;
    return std::nullopt;
}

std::optional<SlotSize> ParseSlotSize(const std::string& value) {
    std::string lower = ToLower(value);
    if (lower == "xs") return SlotSize::XS;
    if (lower == "small") return SlotSize::Small;
    if (lower == "medium") return SlotSize::Medium;
    if (lower == "large") return SlotSize::Large;
    if (lower == "xl") return SlotSize::XL;
    if (lower == "xxl") return SlotSize::XXL;
    return std::nullopt;
}

std::string ToString(HardpointCategory category) {
    switch (category) {
        case HardpointCategory::PrimaryWeapon: return "PrimaryWeapon";
        case HardpointCategory::Utility: return "Utility";
        case HardpointCategory::Module: return "Module";
    }
    return "Unknown";
}

std::string SlotSizeToString(SlotSize size) {
    return std::string(ToString(size));
}

const std::unordered_map<SpaceshipClassType, TaxonomyConstraint>& Taxonomy() {
    static const std::unordered_map<SpaceshipClassType, TaxonomyConstraint> constraints = {
        {SpaceshipClassType::Fighter,
         {RangeConstraint{25.0, 35.0}, RangeConstraint{1.0, 2.0}, RangeConstraint{8.0, 12.0},
          {{HardpointCategory::PrimaryWeapon, {SlotSize::Small, 2}},
           {HardpointCategory::Utility, {SlotSize::XS, 1}},
           {HardpointCategory::Module, {SlotSize::Small, 1}}},
          {{ComponentSlotCategory::PowerPlant, {SlotSize::Small, 1}},
           {ComponentSlotCategory::MainThruster, {SlotSize::Small, 1}},
           {ComponentSlotCategory::ManeuverThruster, {SlotSize::XS, 4}},
           {ComponentSlotCategory::Shield, {SlotSize::Small, 1}},
           {ComponentSlotCategory::Weapon, {SlotSize::Small, 2}},
           {ComponentSlotCategory::Sensor, {SlotSize::Small, 1}},
           {ComponentSlotCategory::Support, {SlotSize::XS, 1}}}}},
        {SpaceshipClassType::Freighter,
         {RangeConstraint{90.0, 120.0}, RangeConstraint{2.0, 4.0}, RangeConstraint{18.0, 26.0},
          {{HardpointCategory::PrimaryWeapon, {SlotSize::Medium, 1}},
           {HardpointCategory::Utility, {SlotSize::Small, 2}},
           {HardpointCategory::Module, {SlotSize::Medium, 3}}},
          {{ComponentSlotCategory::PowerPlant, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::MainThruster, {SlotSize::Medium, 2}},
           {ComponentSlotCategory::ManeuverThruster, {SlotSize::Small, 6}},
           {ComponentSlotCategory::Shield, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::Cargo, {SlotSize::Large, 3}},
           {ComponentSlotCategory::CrewQuarters, {SlotSize::Small, 1}},
           {ComponentSlotCategory::Sensor, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::Support, {SlotSize::Medium, 1}}}}},
        {SpaceshipClassType::Explorer,
         {RangeConstraint{80.0, 95.0}, RangeConstraint{3.0, 5.0}, RangeConstraint{16.0, 22.0},
          {{HardpointCategory::PrimaryWeapon, {SlotSize::Medium, 1}},
           {HardpointCategory::Utility, {SlotSize::Small, 3}},
           {HardpointCategory::Module, {SlotSize::Medium, 3}}},
          {{ComponentSlotCategory::PowerPlant, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::MainThruster, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::ManeuverThruster, {SlotSize::Small, 6}},
           {ComponentSlotCategory::Shield, {SlotSize::Medium, 1}},
           {ComponentSlotCategory::Sensor, {SlotSize::Large, 2}},
           {ComponentSlotCategory::Support, {SlotSize::Medium, 2}},
           {ComponentSlotCategory::CrewQuarters, {SlotSize::Small, 1}},
           {ComponentSlotCategory::Cargo, {SlotSize::Medium, 1}}}}},
        {SpaceshipClassType::Industrial,
         {RangeConstraint{140.0, 180.0}, RangeConstraint{4.0, 6.0}, RangeConstraint{24.0, 34.0},
          {{HardpointCategory::PrimaryWeapon, {SlotSize::Medium, 2}},
           {HardpointCategory::Utility, {SlotSize::Medium, 2}},
           {HardpointCategory::Module, {SlotSize::Large, 4}}},
          {{ComponentSlotCategory::PowerPlant, {SlotSize::Large, 1}},
           {ComponentSlotCategory::MainThruster, {SlotSize::Large, 2}},
           {ComponentSlotCategory::ManeuverThruster, {SlotSize::Medium, 8}},
           {ComponentSlotCategory::Shield, {SlotSize::Large, 1}},
           {ComponentSlotCategory::Industrial, {SlotSize::Large, 4}},
           {ComponentSlotCategory::Cargo, {SlotSize::Large, 2}},
           {ComponentSlotCategory::Support, {SlotSize::Medium, 2}},
           {ComponentSlotCategory::CrewQuarters, {SlotSize::Medium, 1}}}}},
        {SpaceshipClassType::Capital,
         {RangeConstraint{600.0, 950.0}, RangeConstraint{8.0, 18.0}, RangeConstraint{60.0, 120.0},
          {{HardpointCategory::PrimaryWeapon, {SlotSize::XL, 6}},
           {HardpointCategory::Utility, {SlotSize::Large, 4}},
           {HardpointCategory::Module, {SlotSize::XL, 6}}},
          {{ComponentSlotCategory::PowerPlant, {SlotSize::XL, 2}},
           {ComponentSlotCategory::MainThruster, {SlotSize::XL, 4}},
           {ComponentSlotCategory::ManeuverThruster, {SlotSize::Large, 12}},
           {ComponentSlotCategory::Shield, {SlotSize::XL, 2}},
           {ComponentSlotCategory::Hangar, {SlotSize::XL, 2}},
           {ComponentSlotCategory::Support, {SlotSize::Large, 4}},
           {ComponentSlotCategory::Sensor, {SlotSize::Large, 2}},
           {ComponentSlotCategory::CrewQuarters, {SlotSize::Large, 3}},
           {ComponentSlotCategory::Industrial, {SlotSize::Large, 1}}}}}
    };
    return constraints;
}

void AppendError(const std::filesystem::path& path, const std::string& message) {
    State().validationErrors.push_back(path.string() + ": " + message);
}

std::vector<std::string> BuildSlotIds(const std::vector<ComponentSlotSpec>& specs) {
    std::vector<std::string> slotIds;
    slotIds.reserve(32);
    std::unordered_map<ComponentSlotCategory, int> counters;
    for (const auto& spec : specs) {
        int start = counters[spec.category];
        for (int i = 0; i < spec.count; ++i) {
            slotIds.push_back(std::string(ToString(spec.category)) + "_" + std::to_string(start + i));
        }
        counters[spec.category] = start + spec.count;
    }
    return slotIds;
}

void ValidateEntryAgainstTaxonomy(const SpaceshipClassCatalogEntry& entry, const std::filesystem::path& sourcePath) {
    auto taxonomyIt = Taxonomy().find(entry.type);
    if (taxonomyIt == Taxonomy().end()) {
        AppendError(sourcePath, "No taxonomy constraint registered for class type");
        return;
    }
    const auto& constraint = taxonomyIt->second;

    auto withinRange = [&](double value, const RangeConstraint& range) {
        return value >= range.minValue && value <= range.maxValue;
    };

    if (!withinRange(entry.baseline.minMassTons, constraint.massTons) ||
        !withinRange(entry.baseline.maxMassTons, constraint.massTons)) {
        std::ostringstream oss;
        oss << "Mass range " << entry.baseline.minMassTons << "-" << entry.baseline.maxMassTons
            << " tons violates taxonomy (" << constraint.massTons.minValue << "-"
            << constraint.massTons.maxValue << ")";
        AppendError(sourcePath, oss.str());
    }

    if (!withinRange(static_cast<double>(entry.baseline.minCrew), constraint.crew) ||
        !withinRange(static_cast<double>(entry.baseline.maxCrew), constraint.crew)) {
        std::ostringstream oss;
        oss << "Crew range " << entry.baseline.minCrew << "-" << entry.baseline.maxCrew
            << " violates taxonomy (" << constraint.crew.minValue << "-" << constraint.crew.maxValue << ")";
        AppendError(sourcePath, oss.str());
    }

    if (!withinRange(entry.baseline.minPowerBudgetMW, constraint.powerBudget) ||
        !withinRange(entry.baseline.maxPowerBudgetMW, constraint.powerBudget)) {
        std::ostringstream oss;
        oss << "Power budget " << entry.baseline.minPowerBudgetMW << "-" << entry.baseline.maxPowerBudgetMW
            << "MW violates taxonomy (" << constraint.powerBudget.minValue << "-"
            << constraint.powerBudget.maxValue << ")";
        AppendError(sourcePath, oss.str());
    }

    std::unordered_map<HardpointCategory, HardpointSpec> hardpointLookup;
    for (const auto& hardpoint : entry.hardpoints) {
        hardpointLookup[hardpoint.category] = hardpoint;
    }
    for (const auto& [category, expected] : constraint.hardpoints) {
        auto it = hardpointLookup.find(category);
        if (it == hardpointLookup.end()) {
            AppendError(sourcePath, "Missing hardpoint category " + ToString(category));
            continue;
        }
        if (it->second.count != expected.expectedCount) {
            std::ostringstream oss;
            oss << "Hardpoint count mismatch for " << ToString(category) << ": expected "
                << expected.expectedCount << " found " << it->second.count;
            AppendError(sourcePath, oss.str());
        }
        if (it->second.size != expected.expectedSize) {
            std::ostringstream oss;
            oss << "Hardpoint size mismatch for " << ToString(category) << ": expected "
                << SlotSizeToString(expected.expectedSize) << " found " << SlotSizeToString(it->second.size);
            AppendError(sourcePath, oss.str());
        }
    }

    std::unordered_map<ComponentSlotCategory, ComponentSlotSpec> slotLookup;
    for (const auto& slot : entry.componentSlots) {
        slotLookup[slot.category] = slot;
    }
    for (const auto& [category, expected] : constraint.slots) {
        auto it = slotLookup.find(category);
        if (it == slotLookup.end()) {
            AppendError(sourcePath, std::string("Missing component slot category ") + ToString(category));
            continue;
        }
        if (it->second.count != expected.expectedCount) {
            std::ostringstream oss;
            oss << "Slot count mismatch for " << ToString(category) << ": expected "
                << expected.expectedCount << " found " << it->second.count;
            AppendError(sourcePath, oss.str());
        }
        if (it->second.size != expected.expectedSize) {
            std::ostringstream oss;
            oss << "Slot size mismatch for " << ToString(category) << ": expected "
                << SlotSizeToString(expected.expectedSize) << " found " << SlotSizeToString(it->second.size);
            AppendError(sourcePath, oss.str());
        }
    }

    auto slotIds = BuildSlotIds(entry.componentSlots);
    for (const auto& loadout : entry.defaultLoadouts) {
        if (loadout.components.size() > slotIds.size()) {
            std::ostringstream oss;
            oss << "Default loadout '" << loadout.name << "' assigns " << loadout.components.size()
                << " components exceeding available slots " << slotIds.size();
            AppendError(sourcePath, oss.str());
        }
    }

    if (entry.progression.empty()) {
        AppendError(sourcePath, "Progression tiers are empty");
    } else {
        int expectedTier = entry.progression.front().tier;
        for (const auto& tier : entry.progression) {
            if (tier.tier != expectedTier) {
                std::ostringstream oss;
                oss << "Progression tiers must be sequential. Expected tier " << expectedTier
                    << " found " << tier.tier;
                AppendError(sourcePath, oss.str());
                expectedTier = tier.tier;
            }
            ++expectedTier;
        }
    }

    if (entry.progressionMetadata.minLevel < 1 || entry.progressionMetadata.minLevel > 40) {
        std::ostringstream oss;
        oss << "Progression metadata minLevel " << entry.progressionMetadata.minLevel
            << " outside supported range (1-40)";
        AppendError(sourcePath, oss.str());
    }
    if (entry.progressionMetadata.blueprintCost < 0) {
        AppendError(sourcePath, "Blueprint cost cannot be negative");
    }

    // Validate variant deltas do not remove more than available
    for (const auto& variant : entry.variants) {
        for (const auto& delta : variant.hardpointDeltas) {
            auto it = hardpointLookup.find(delta.category);
            int baselineCount = (it != hardpointLookup.end()) ? it->second.count : 0;
            if (baselineCount + delta.countDelta < 0) {
                std::ostringstream oss;
                oss << "Variant '" << variant.codename << "' removes too many "
                    << ToString(delta.category) << " hardpoints";
                AppendError(sourcePath, oss.str());
            }
        }
        for (const auto& delta : variant.slotDeltas) {
            auto it = slotLookup.find(delta.category);
            int baselineCount = (it != slotLookup.end()) ? it->second.count : 0;
            if (baselineCount + delta.countDelta < 0) {
                std::ostringstream oss;
                oss << "Variant '" << variant.codename << "' removes too many "
                    << ToString(delta.category) << " slots";
                AppendError(sourcePath, oss.str());
            }
        }
    }
}

bool ParseConceptSummary(const simplejson::JsonValue& value, SpaceshipConceptSummary& summary) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto pitchIt = object.find("elevatorPitch");
    if (pitchIt == object.end() || pitchIt->second.type() != simplejson::JsonValue::Type::String) {
        return false;
    }
    summary.elevatorPitch = pitchIt->second.AsString();
    auto hooksIt = object.find("gameplayHooks");
    if (hooksIt != object.end() && hooksIt->second.type() == simplejson::JsonValue::Type::Array) {
        for (const auto& hookValue : hooksIt->second.AsArray()) {
            if (hookValue.type() == simplejson::JsonValue::Type::String) {
                summary.gameplayHooks.push_back(hookValue.AsString());
            }
        }
    }
    return true;
}

bool ParseBaseline(const simplejson::JsonValue& value, SpaceshipBaselineSpec& baseline) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto getNumber = [&](const char* key, double& out) -> bool {
        auto it = object.find(key);
        if (it == object.end() || it->second.type() != simplejson::JsonValue::Type::Number) {
            return false;
        }
        out = it->second.AsNumber();
        return true;
    };
    auto getInt = [&](const char* key, int& out) -> bool {
        auto it = object.find(key);
        if (it == object.end() || it->second.type() != simplejson::JsonValue::Type::Number) {
            return false;
        }
        out = static_cast<int>(it->second.AsNumber());
        return true;
    };

    return getNumber("minMassTons", baseline.minMassTons) &&
           getNumber("maxMassTons", baseline.maxMassTons) &&
           getInt("minCrew", baseline.minCrew) &&
           getInt("maxCrew", baseline.maxCrew) &&
           getNumber("minPowerBudgetMW", baseline.minPowerBudgetMW) &&
           getNumber("maxPowerBudgetMW", baseline.maxPowerBudgetMW);
}

bool ParseHardpointSpec(const simplejson::JsonValue& value, HardpointSpec& spec) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto categoryIt = object.find("category");
    auto sizeIt = object.find("size");
    auto countIt = object.find("count");
    if (categoryIt == object.end() || sizeIt == object.end() || countIt == object.end()) {
        return false;
    }
    if (categoryIt->second.type() != simplejson::JsonValue::Type::String ||
        sizeIt->second.type() != simplejson::JsonValue::Type::String ||
        countIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    auto categoryOpt = ParseHardpointCategory(categoryIt->second.AsString());
    auto sizeOpt = ParseSlotSize(sizeIt->second.AsString());
    if (!categoryOpt || !sizeOpt) {
        return false;
    }
    spec.category = *categoryOpt;
    spec.size = *sizeOpt;
    spec.count = static_cast<int>(countIt->second.AsNumber());

    auto notesIt = object.find("notes");
    if (notesIt != object.end() && notesIt->second.type() == simplejson::JsonValue::Type::String) {
        spec.notes = notesIt->second.AsString();
    }
    return true;
}

bool ParseComponentSlotSpec(const simplejson::JsonValue& value, ComponentSlotSpec& spec) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto categoryIt = object.find("category");
    auto sizeIt = object.find("size");
    auto countIt = object.find("count");
    if (categoryIt == object.end() || sizeIt == object.end() || countIt == object.end()) {
        return false;
    }
    if (categoryIt->second.type() != simplejson::JsonValue::Type::String ||
        sizeIt->second.type() != simplejson::JsonValue::Type::String ||
        countIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    auto categoryOpt = ParseSlotCategory(categoryIt->second.AsString());
    auto sizeOpt = ParseSlotSize(sizeIt->second.AsString());
    if (!categoryOpt || !sizeOpt) {
        return false;
    }
    spec.category = *categoryOpt;
    spec.size = *sizeOpt;
    spec.count = static_cast<int>(countIt->second.AsNumber());
    auto notesIt = object.find("notes");
    if (notesIt != object.end() && notesIt->second.type() == simplejson::JsonValue::Type::String) {
        spec.notes = notesIt->second.AsString();
    }
    return true;
}

bool ParseProgressionTier(const simplejson::JsonValue& value, ProgressionTier& tier) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto tierIt = object.find("tier");
    auto nameIt = object.find("name");
    auto descIt = object.find("description");
    if (tierIt == object.end() || nameIt == object.end() || descIt == object.end()) {
        return false;
    }
    if (tierIt->second.type() != simplejson::JsonValue::Type::Number ||
        nameIt->second.type() != simplejson::JsonValue::Type::String ||
        descIt->second.type() != simplejson::JsonValue::Type::String) {
        return false;
    }
    tier.tier = static_cast<int>(tierIt->second.AsNumber());
    tier.name = nameIt->second.AsString();
    tier.description = descIt->second.AsString();
    return true;
}

bool ParsePassiveBuff(const simplejson::JsonValue& value, PassiveBuff& buff) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto typeIt = object.find("type");
    auto valueIt = object.find("value");
    if (typeIt == object.end() || valueIt == object.end()) {
        return false;
    }
    if (typeIt->second.type() != simplejson::JsonValue::Type::String ||
        valueIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    buff.type = typeIt->second.AsString();
    buff.value = valueIt->second.AsNumber();
    return true;
}

bool ParseHardpointDelta(const simplejson::JsonValue& value, HardpointDelta& delta) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto categoryIt = object.find("category");
    auto countIt = object.find("countDelta");
    if (categoryIt == object.end() || countIt == object.end() ||
        categoryIt->second.type() != simplejson::JsonValue::Type::String ||
        countIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    auto categoryOpt = ParseHardpointCategory(categoryIt->second.AsString());
    if (!categoryOpt) {
        return false;
    }
    delta.category = *categoryOpt;
    delta.countDelta = static_cast<int>(countIt->second.AsNumber());
    auto sizeIt = object.find("sizeDelta");
    if (sizeIt != object.end() && sizeIt->second.type() == simplejson::JsonValue::Type::String) {
        auto sizeOpt = ParseSlotSize(sizeIt->second.AsString());
        if (!sizeOpt) {
            return false;
        }
        delta.sizeDelta = sizeOpt;
    }
    return true;
}

bool ParseSlotDelta(const simplejson::JsonValue& value, SlotDelta& delta) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto categoryIt = object.find("category");
    auto countIt = object.find("countDelta");
    if (categoryIt == object.end() || countIt == object.end() ||
        categoryIt->second.type() != simplejson::JsonValue::Type::String ||
        countIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    auto categoryOpt = ParseSlotCategory(categoryIt->second.AsString());
    if (!categoryOpt) {
        return false;
    }
    delta.category = *categoryOpt;
    delta.countDelta = static_cast<int>(countIt->second.AsNumber());
    auto sizeIt = object.find("size");
    if (sizeIt != object.end() && sizeIt->second.type() == simplejson::JsonValue::Type::String) {
        auto sizeOpt = ParseSlotSize(sizeIt->second.AsString());
        if (!sizeOpt) {
            return false;
        }
        delta.size = sizeOpt;
    }
    return true;
}

bool ParseVariant(const simplejson::JsonValue& value, VariantSpec& variant) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto factionIt = object.find("faction");
    auto codenameIt = object.find("codename");
    auto descIt = object.find("description");
    if (factionIt == object.end() || codenameIt == object.end() || descIt == object.end()) {
        return false;
    }
    if (factionIt->second.type() != simplejson::JsonValue::Type::String ||
        codenameIt->second.type() != simplejson::JsonValue::Type::String ||
        descIt->second.type() != simplejson::JsonValue::Type::String) {
        return false;
    }
    variant.faction = factionIt->second.AsString();
    variant.codename = codenameIt->second.AsString();
    variant.description = descIt->second.AsString();

    auto hpIt = object.find("hardpointDeltas");
    if (hpIt != object.end() && hpIt->second.type() == simplejson::JsonValue::Type::Array) {
        for (const auto& hpValue : hpIt->second.AsArray()) {
            HardpointDelta delta;
            if (ParseHardpointDelta(hpValue, delta)) {
                variant.hardpointDeltas.push_back(delta);
            }
        }
    }
    auto slotIt = object.find("slotDeltas");
    if (slotIt != object.end() && slotIt->second.type() == simplejson::JsonValue::Type::Array) {
        for (const auto& slotValue : slotIt->second.AsArray()) {
            SlotDelta delta;
            if (ParseSlotDelta(slotValue, delta)) {
                variant.slotDeltas.push_back(delta);
            }
        }
    }
    auto buffIt = object.find("passiveBuffs");
    if (buffIt != object.end() && buffIt->second.type() == simplejson::JsonValue::Type::Array) {
        for (const auto& buffValue : buffIt->second.AsArray()) {
            PassiveBuff buff;
            if (ParsePassiveBuff(buffValue, buff)) {
                variant.passiveBuffs.push_back(buff);
            }
        }
    }
    return true;
}

bool ParseProgressionMetadata(const simplejson::JsonValue& value, ProgressionMetadata& metadata) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto levelIt = object.find("minLevel");
    auto repIt = object.find("factionReputation");
    auto blueprintIt = object.find("blueprintCost");
    if (levelIt == object.end() || repIt == object.end() || blueprintIt == object.end()) {
        return false;
    }
    if (levelIt->second.type() != simplejson::JsonValue::Type::Number ||
        repIt->second.type() != simplejson::JsonValue::Type::Number ||
        blueprintIt->second.type() != simplejson::JsonValue::Type::Number) {
        return false;
    }
    metadata.minLevel = static_cast<int>(levelIt->second.AsNumber());
    metadata.factionReputation = static_cast<int>(repIt->second.AsNumber());
    metadata.blueprintCost = static_cast<int>(blueprintIt->second.AsNumber());
    return true;
}

bool ParseDefaultLoadout(const simplejson::JsonValue& value, DefaultLoadout& loadout) {
    if (value.type() != simplejson::JsonValue::Type::Object) {
        return false;
    }
    const auto& object = value.AsObject();
    auto nameIt = object.find("name");
    auto descIt = object.find("description");
    auto compIt = object.find("components");
    if (nameIt == object.end() || descIt == object.end() || compIt == object.end()) {
        return false;
    }
    if (nameIt->second.type() != simplejson::JsonValue::Type::String ||
        descIt->second.type() != simplejson::JsonValue::Type::String ||
        compIt->second.type() != simplejson::JsonValue::Type::Array) {
        return false;
    }
    loadout.name = nameIt->second.AsString();
    loadout.description = descIt->second.AsString();
    for (const auto& componentValue : compIt->second.AsArray()) {
        if (componentValue.type() == simplejson::JsonValue::Type::String) {
            loadout.components.push_back(componentValue.AsString());
        }
    }
    return true;
}

bool ParseCatalogEntry(const std::filesystem::path& path, const simplejson::JsonValue& rootValue,
                       SpaceshipClassCatalogEntry& entry) {
    if (rootValue.type() != simplejson::JsonValue::Type::Object) {
        AppendError(path, "Root JSON value must be an object");
        return false;
    }
    const auto& root = rootValue.AsObject();

    auto typeIt = root.find("type");
    if (typeIt == root.end() || typeIt->second.type() != simplejson::JsonValue::Type::String) {
        AppendError(path, "Missing or invalid 'type'");
        return false;
    }
    auto classType = ParseClassType(typeIt->second.AsString());
    if (!classType) {
        AppendError(path, "Unknown class type '" + typeIt->second.AsString() + "'");
        return false;
    }
    entry.type = *classType;

    auto displayNameIt = root.find("displayName");
    if (displayNameIt == root.end() || displayNameIt->second.type() != simplejson::JsonValue::Type::String) {
        AppendError(path, "Missing 'displayName'");
        return false;
    }
    entry.displayName = displayNameIt->second.AsString();

    auto conceptIt = root.find("conceptSummary");
    if (conceptIt == root.end() || !ParseConceptSummary(conceptIt->second, entry.conceptSummary)) {
        AppendError(path, "Missing or invalid conceptSummary");
        return false;
    }

    auto baselineIt = root.find("baseline");
    if (baselineIt == root.end() || !ParseBaseline(baselineIt->second, entry.baseline)) {
        AppendError(path, "Missing or invalid baseline");
        return false;
    }

    auto hardpointsIt = root.find("hardpoints");
    if (hardpointsIt == root.end() || hardpointsIt->second.type() != simplejson::JsonValue::Type::Array) {
        AppendError(path, "Missing hardpoints array");
        return false;
    }
    for (const auto& hpValue : hardpointsIt->second.AsArray()) {
        HardpointSpec spec;
        if (!ParseHardpointSpec(hpValue, spec)) {
            AppendError(path, "Invalid hardpoint spec encountered");
            return false;
        }
        entry.hardpoints.push_back(spec);
    }

    auto slotIt = root.find("componentSlots");
    if (slotIt == root.end() || slotIt->second.type() != simplejson::JsonValue::Type::Array) {
        AppendError(path, "Missing componentSlots array");
        return false;
    }
    for (const auto& slotValue : slotIt->second.AsArray()) {
        ComponentSlotSpec spec;
        if (!ParseComponentSlotSpec(slotValue, spec)) {
            AppendError(path, "Invalid component slot specification");
            return false;
        }
        entry.componentSlots.push_back(spec);
    }

    auto progressionIt = root.find("progression");
    if (progressionIt == root.end() || progressionIt->second.type() != simplejson::JsonValue::Type::Array) {
        AppendError(path, "Missing progression array");
        return false;
    }
    for (const auto& tierValue : progressionIt->second.AsArray()) {
        ProgressionTier tier;
        if (!ParseProgressionTier(tierValue, tier)) {
            AppendError(path, "Invalid progression tier");
            return false;
        }
        entry.progression.push_back(tier);
    }

    auto variantsIt = root.find("variants");
    if (variantsIt != root.end() && variantsIt->second.type() == simplejson::JsonValue::Type::Array) {
        for (const auto& variantValue : variantsIt->second.AsArray()) {
            VariantSpec variant;
            if (!ParseVariant(variantValue, variant)) {
                AppendError(path, "Invalid variant specification");
                return false;
            }
            entry.variants.push_back(variant);
        }
    }

    auto metadataIt = root.find("progressionMetadata");
    if (metadataIt == root.end() || !ParseProgressionMetadata(metadataIt->second, entry.progressionMetadata)) {
        AppendError(path, "Missing progressionMetadata");
        return false;
    }

    auto loadoutsIt = root.find("defaultLoadouts");
    if (loadoutsIt == root.end() || loadoutsIt->second.type() != simplejson::JsonValue::Type::Array) {
        AppendError(path, "Missing defaultLoadouts array");
        return false;
    }
    for (const auto& loadoutValue : loadoutsIt->second.AsArray()) {
        DefaultLoadout loadout;
        if (!ParseDefaultLoadout(loadoutValue, loadout)) {
            AppendError(path, "Invalid default loadout definition");
            return false;
        }
        entry.defaultLoadouts.push_back(loadout);
    }

    auto idIt = root.find("id");
    if (idIt != root.end() && idIt->second.type() == simplejson::JsonValue::Type::String) {
        entry.id = idIt->second.AsString();
    } else {
        entry.id = path.stem().string();
    }
    return true;
}

void LoadCatalogFromDisk() {
    CatalogState& state = State();
    state.entries.clear();
    state.validationErrors.clear();

    std::unordered_map<std::string, FileTimePoint> newTimes;
    std::set<std::string> ids;

    if (!std::filesystem::exists(kCatalogDirectory)) {
        AppendError(kCatalogDirectory, "Catalog directory missing");
        state.loaded = true;
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(kCatalogDirectory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        try {
            newTimes[entry.path().string()] = std::filesystem::last_write_time(entry.path());
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore failures to read modification time
        }

        std::ifstream file(entry.path());
        if (!file.good()) {
            AppendError(entry.path(), "Failed to open file");
            continue;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        auto parseResult = simplejson::Parse(buffer.str());
        if (!parseResult.success) {
            AppendError(entry.path(), "JSON parse error at offset " + std::to_string(parseResult.errorOffset));
            continue;
        }

        SpaceshipClassCatalogEntry catalogEntry;
        if (!ParseCatalogEntry(entry.path(), parseResult.value, catalogEntry)) {
            continue;
        }

        if (!ids.insert(catalogEntry.id).second) {
            AppendError(entry.path(), "Duplicate catalog id '" + catalogEntry.id + "'");
            continue;
        }

        ValidateEntryAgainstTaxonomy(catalogEntry, entry.path());
        state.entries.push_back(std::move(catalogEntry));
    }

    std::sort(state.entries.begin(), state.entries.end(), [](const auto& a, const auto& b) {
        return a.id < b.id;
    });

    state.fileTimes = std::move(newTimes);
    state.loaded = true;
}

void EnsureLoaded() {
    if (!State().loaded) {
        LoadCatalogFromDisk();
    }
}

void MaybeReloadForHotReload() {
    CatalogState& state = State();
    if (!state.hotReloadEnabled || !state.loaded) {
        return;
    }

    bool changed = false;
    std::unordered_map<std::string, FileTimePoint> newTimes;
    if (!std::filesystem::exists(kCatalogDirectory)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(kCatalogDirectory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        FileTimePoint currentTime{};
        try {
            currentTime = std::filesystem::last_write_time(entry.path());
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
        std::string key = entry.path().string();
        newTimes[key] = currentTime;
        auto it = state.fileTimes.find(key);
        if (it == state.fileTimes.end() || it->second != currentTime) {
            changed = true;
        }
    }
    if (!changed && state.fileTimes.size() != newTimes.size()) {
        changed = true;
    }
    if (changed) {
        LoadCatalogFromDisk();
    } else {
        state.fileTimes = std::move(newTimes);
    }
}

} // namespace

const std::vector<SpaceshipClassCatalogEntry>& SpaceshipCatalog::All() {
    EnsureLoaded();
    return State().entries;
}

const SpaceshipClassCatalogEntry* SpaceshipCatalog::FindById(const std::string& id) {
    EnsureLoaded();
    const auto& entries = State().entries;
    auto it = std::find_if(entries.begin(), entries.end(), [&](const SpaceshipClassCatalogEntry& entry) {
        return entry.id == id;
    });
    if (it == entries.end()) {
        return nullptr;
    }
    return &(*it);
}

void SpaceshipCatalog::Reload() {
    LoadCatalogFromDisk();
}

void SpaceshipCatalog::EnableHotReload(bool enabled) {
    State().hotReloadEnabled = enabled;
}

void SpaceshipCatalog::TickHotReload() {
    MaybeReloadForHotReload();
}

const std::vector<std::string>& SpaceshipCatalog::ValidationErrors() {
    EnsureLoaded();
    return State().validationErrors;
}

SpaceshipClassDefinition SpaceshipCatalog::BuildClassDefinition(const SpaceshipClassCatalogEntry& entry) {
    SpaceshipClassDefinition definition;
    definition.type = entry.type;
    definition.displayName = entry.displayName;
    definition.baseline = entry.baseline;
    definition.componentSlots = entry.componentSlots;
    definition.defaultLoadouts = BuildDefaultLoadoutRequests(entry);
    return definition;
}

std::vector<ShipAssemblyRequest> SpaceshipCatalog::BuildDefaultLoadoutRequests(const SpaceshipClassCatalogEntry& entry) {
    std::vector<ShipAssemblyRequest> requests;
    auto slotIds = BuildSlotIds(entry.componentSlots);
    requests.reserve(entry.defaultLoadouts.size());
    for (const auto& loadout : entry.defaultLoadouts) {
        ShipAssemblyRequest request;
        request.hullId = entry.id;
        const std::size_t count = std::min(slotIds.size(), loadout.components.size());
        for (std::size_t i = 0; i < count; ++i) {
            request.slotAssignments[slotIds[i]] = loadout.components[i];
        }
        requests.push_back(std::move(request));
    }
    return requests;
}

ResolvedDefaultLoadout SpaceshipCatalog::ResolveDefaultLoadout(const SpaceshipClassCatalogEntry& entry,
                                                               const DefaultLoadout& loadout) {
    ResolvedDefaultLoadout resolved;
    resolved.loadout = &loadout;
    resolved.assemblyRequest.hullId = entry.id;
    auto slotIds = BuildSlotIds(entry.componentSlots);
    const std::size_t count = std::min(slotIds.size(), loadout.components.size());
    for (std::size_t i = 0; i < count; ++i) {
        resolved.assemblyRequest.slotAssignments[slotIds[i]] = loadout.components[i];
    }
    return resolved;
}

std::optional<ShipAssemblyRequest> SpaceshipCatalog::BuildDefaultLoadoutRequest(const std::string& classId,
                                                                               const std::string& loadoutName) {
    const auto* entry = FindById(classId);
    if (!entry) {
        return std::nullopt;
    }
    auto it = std::find_if(entry->defaultLoadouts.begin(), entry->defaultLoadouts.end(), [&](const DefaultLoadout& loadout) {
        return loadout.name == loadoutName;
    });
    if (it == entry->defaultLoadouts.end()) {
        return std::nullopt;
    }
    return ResolveDefaultLoadout(*entry, *it).assemblyRequest;
}

SpaceshipVariantLayout SpaceshipCatalog::ResolveVariantLayout(const SpaceshipClassCatalogEntry& entry,
                                                              const VariantSpec& variant) {
    SpaceshipVariantLayout layout;
    layout.hardpoints = entry.hardpoints;
    layout.componentSlots = entry.componentSlots;

    auto applyHardpointDelta = [&](const HardpointDelta& delta) {
        auto it = std::find_if(layout.hardpoints.begin(), layout.hardpoints.end(), [&](const HardpointSpec& spec) {
            return spec.category == delta.category;
        });
        if (it == layout.hardpoints.end()) {
            if (delta.countDelta > 0) {
                HardpointSpec spec;
                spec.category = delta.category;
                spec.size = delta.sizeDelta.value_or(SlotSize::Small);
                spec.count = delta.countDelta;
                layout.hardpoints.push_back(spec);
            }
            return;
        }
        it->count = std::max(0, it->count + delta.countDelta);
        if (delta.sizeDelta) {
            it->size = *delta.sizeDelta;
        }
    };

    auto applySlotDelta = [&](const SlotDelta& delta) {
        auto it = std::find_if(layout.componentSlots.begin(), layout.componentSlots.end(), [&](const ComponentSlotSpec& spec) {
            return spec.category == delta.category;
        });
        if (it == layout.componentSlots.end()) {
            if (delta.countDelta > 0) {
                ComponentSlotSpec spec;
                spec.category = delta.category;
                spec.size = delta.size.value_or(SlotSize::Small);
                spec.count = delta.countDelta;
                layout.componentSlots.push_back(spec);
            }
            return;
        }
        it->count = std::max(0, it->count + delta.countDelta);
        if (delta.size) {
            it->size = *delta.size;
        }
    };

    for (const auto& delta : variant.hardpointDeltas) {
        applyHardpointDelta(delta);
    }
    for (const auto& delta : variant.slotDeltas) {
        applySlotDelta(delta);
    }

    layout.hardpoints.erase(std::remove_if(layout.hardpoints.begin(), layout.hardpoints.end(), [](const HardpointSpec& spec) {
                                 return spec.count <= 0;
                             }),
                             layout.hardpoints.end());
    layout.componentSlots.erase(
        std::remove_if(layout.componentSlots.begin(), layout.componentSlots.end(), [](const ComponentSlotSpec& spec) {
            return spec.count <= 0;
        }),
        layout.componentSlots.end());

    return layout;
}

SpaceshipSpawnBundle SpaceshipCatalog::BuildSpawnBundle(const SpaceshipClassCatalogEntry& entry,
                                                        const DefaultLoadout& loadout,
                                                        int loadoutIndex,
                                                        const std::string& hullSuffix) {
    auto resolved = ResolveDefaultLoadout(entry, loadout);
    SpaceshipSpawnBundle bundle;
    bundle.assemblyRequest = std::move(resolved.assemblyRequest);
    if (!hullSuffix.empty()) {
        bundle.assemblyRequest.hullId = entry.id + "_" + hullSuffix;
    }
    bundle.classId = entry.id;
    bundle.displayName = entry.displayName;
    bundle.loadoutIndex = loadoutIndex;
    return bundle;
}

Entity SpawnSpaceship(EntityManager& entityManager, const SpaceshipSpawnBundle& bundle) {
    Entity entity = entityManager.CreateEntity();
    auto& tag = entityManager.EmplaceComponent<SpaceshipTag>(entity);
    tag.classId = bundle.classId;
    tag.displayName = bundle.displayName;
    tag.loadoutIndex = bundle.loadoutIndex;
    tag.playerControlled = bundle.playerControlled;

    if (!bundle.displayName.empty()) {
        auto& name = entityManager.EmplaceComponent<Name>(entity);
        name.value = bundle.displayName;
    }
    return entity;
}