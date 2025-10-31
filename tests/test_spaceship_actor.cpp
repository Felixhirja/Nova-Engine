#include "actors/Spaceship.h"

#include <iostream>

int main() {
    SpaceshipCatalog::EnableHotReload(false);
    SpaceshipCatalog::Reload();

    const auto& catalog = SpaceshipCatalog::All();
    if (catalog.empty()) {
        std::cerr << "ERROR: Spaceship catalog failed to load" << std::endl;
        return 1;
    }

    const auto& errors = SpaceshipCatalog::ValidationErrors();
    if (!errors.empty()) {
        std::cerr << "ERROR: Catalog validation reported issues:" << std::endl;
        for (const auto& err : errors) {
            std::cerr << "  - " << err << std::endl;
        }
        return 1;
    }

    auto fighter = SpaceshipCatalog::FindById("fighter");
    if (!fighter) {
        std::cerr << "ERROR: Fighter entry missing" << std::endl;
        return 1;
    }

    auto requests = SpaceshipCatalog::BuildDefaultLoadoutRequests(*fighter);
    if (requests.empty()) {
        std::cerr << "ERROR: Fighter missing default loadouts" << std::endl;
        return 1;
    }

    auto bundle = SpaceshipCatalog::BuildSpawnBundle(*fighter, fighter->defaultLoadouts.front(), 0);
    if (bundle.assemblyRequest.slotAssignments.empty()) {
        std::cerr << "ERROR: Spawn bundle missing slot assignments" << std::endl;
        return 1;
    }

    auto variantLayout = SpaceshipCatalog::ResolveVariantLayout(*fighter, fighter->variants.front());
    if (variantLayout.hardpoints.empty()) {
        std::cerr << "ERROR: Variant resolution produced empty hardpoint layout" << std::endl;
        return 1;
    }

    std::cout << "Spaceship catalog loaded " << catalog.size() << " entries" << std::endl;
    std::cout << "Spawn bundle ready for class '" << bundle.classId << "' with "
              << bundle.assemblyRequest.slotAssignments.size() << " components" << std::endl;

    return 0;
}
