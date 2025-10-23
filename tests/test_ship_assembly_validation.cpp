#include "../src/ShipAssembly.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

ShipAssemblyRequest BuildValidRequest(const ShipHullBlueprint& hull) {
    ShipAssemblyRequest request;
    request.hullId = hull.id;
    auto assign = [](const HullSlot& slot) -> std::string {
        switch (slot.category) {
            case ComponentSlotCategory::PowerPlant:
                return "fusion_core_mk1";
            case ComponentSlotCategory::MainThruster:
                return "main_thruster_viper";
            case ComponentSlotCategory::ManeuverThruster:
                return "rcs_cluster_micro";
            case ComponentSlotCategory::Shield:
                return "shield_array_light";
            case ComponentSlotCategory::Weapon:
                return "weapon_twin_cannon";
            case ComponentSlotCategory::Sensor:
                return "sensor_targeting_mk1";
            case ComponentSlotCategory::Support:
                return "support_life_pod";
            case ComponentSlotCategory::Cargo:
                return "cargo_rack_standard";
            case ComponentSlotCategory::CrewQuarters:
                return "support_life_pod";
            case ComponentSlotCategory::Industrial:
                return "weapon_beam_array";
            case ComponentSlotCategory::Hangar:
            case ComponentSlotCategory::Computer:
                return "sensor_targeting_mk1";
        }
        return {};
    };

    for (const auto& slot : hull.slots) {
        std::string choice = assign(slot);
        if (!choice.empty()) {
            request.slotAssignments[slot.slotId] = choice;
        }
    }
    return request;
}

bool ContainsMessage(const std::vector<std::string>& messages, const std::string& needle) {
    return std::any_of(messages.begin(), messages.end(), [&](const std::string& msg) {
        return msg.find(needle) != std::string::npos;
    });
}

bool SuggestionIncludes(const ShipAssemblyResult& result,
                        const std::string& slotId,
                        const std::string& componentId) {
    return std::any_of(result.diagnostics.suggestions.begin(),
                       result.diagnostics.suggestions.end(),
                       [&](const ComponentSuggestion& suggestion) {
                           if (suggestion.slotId != slotId) {
                               return false;
                           }
                           return std::find(suggestion.suggestedComponentIds.begin(),
                                            suggestion.suggestedComponentIds.end(),
                                            componentId) != suggestion.suggestedComponentIds.end();
                       });
}

bool TestMissingRequiredAssignments() {
    ShipAssemblyRequest request;
    request.hullId = "fighter_mk1";

    ShipAssemblyResult result = ShipAssembler::Assemble(request);
    if (result.IsValid()) {
        std::cerr << "Assembly unexpectedly succeeded without assignments" << std::endl;
        return false;
    }
    if (!ContainsMessage(result.diagnostics.errors, "Required")) {
        std::cerr << "Expected missing slot errors were not reported" << std::endl;
        return false;
    }
    if (!SuggestionIncludes(result, "PowerPlant_0", "fusion_core_mk1")) {
        std::cerr << "Expected power plant suggestion missing" << std::endl;
        return false;
    }
    return true;
}

bool TestCategoryMismatchDetection() {
    const ShipHullBlueprint* hull = ShipHullCatalog::Find("fighter_mk1");
    if (!hull) {
        std::cerr << "Fighter hull blueprint not found" << std::endl;
        return false;
    }

    ShipAssemblyRequest request = BuildValidRequest(*hull);
    request.slotAssignments["Weapon_0"] = "fusion_core_mk1"; // Wrong category on purpose

    ShipAssemblyResult result = ShipAssembler::Assemble(request);
    if (result.IsValid()) {
        std::cerr << "Category mismatch should invalidate assembly" << std::endl;
        return false;
    }
    if (!ContainsMessage(result.diagnostics.errors, "Category mismatch")) {
        std::cerr << "Category mismatch error message missing" << std::endl;
        return false;
    }
    if (!SuggestionIncludes(result, "Weapon_0", "weapon_twin_cannon")) {
        std::cerr << "Expected compatible weapon suggestion missing" << std::endl;
        return false;
    }
    return true;
}

bool TestSizeMismatchDetection() {
    const ShipHullBlueprint* hull = ShipHullCatalog::Find("fighter_mk1");
    if (!hull) {
        std::cerr << "Fighter hull blueprint not found" << std::endl;
        return false;
    }

    ShipAssemblyRequest request = BuildValidRequest(*hull);
    request.slotAssignments["Weapon_1"] = "weapon_defensive_turret"; // Medium weapon in small slot

    ShipAssemblyResult result = ShipAssembler::Assemble(request);
    if (result.IsValid()) {
        std::cerr << "Size mismatch should invalidate assembly" << std::endl;
        return false;
    }
    if (!ContainsMessage(result.diagnostics.errors, "Size mismatch")) {
        std::cerr << "Size mismatch error message missing" << std::endl;
        return false;
    }
    if (!SuggestionIncludes(result, "Weapon_1", "weapon_twin_cannon")) {
        std::cerr << "Expected small-weapon suggestion missing" << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main() {
    if (!TestMissingRequiredAssignments()) {
        return 1;
    }
    if (!TestCategoryMismatchDetection()) {
        return 2;
    }
    if (!TestSizeMismatchDetection()) {
        return 3;
    }

    std::cout << "Ship assembly validation tests passed." << std::endl;
    return 0;
}
