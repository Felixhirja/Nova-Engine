#pragma once

#include "ecs/Components.h"

namespace navigation {

class NavigationGridBuilder {
public:
    NavigationGrid BuildFromBounds(const MovementBounds& bounds, double cellSize = 1.0) const;
};

} // namespace navigation

