#include "engine/ecs/EntityManagerV2.h"
#include "engine/ecs/Components.h"
#include "engine/SpaceshipTagUtils.h"
#include "engine/SpaceshipTagIter.h"
#include <iostream>
#include <vector>

int main() {
    ecs::EntityManagerV2 em;

    // Create a couple of bare entities and just tag them as spaceships
    ecs::EntityHandle a = em.CreateEntity();
    ecs::EntityHandle b = em.CreateEntity();

    TagEntityAsSpaceship(em, a, "fighter_alpha", "Fighter Alpha", 0, true);
    TagEntityAsSpaceship(em, b, "freighter_beta", "Freighter Beta", 1, false);

    std::vector<std::string> names;
    spaceship::ForEach(em, [&](ecs::EntityHandle e, SpaceshipTag& tag) {
        names.push_back(tag.displayName);
        if (!em.IsAlive(e)) {
            std::cerr << "Iterated a dead entity!" << std::endl;
            std::exit(1);
        }
    });

    if (names.size() != 2) {
        std::cerr << "Expected 2 SpaceshipTag entities, got " << names.size() << std::endl;
        return 1;
    }

    std::cout << "SpaceshipTag iteration ok: " << names[0] << ", " << names[1] << std::endl;
    return 0;
}
