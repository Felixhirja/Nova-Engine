#include "NavigationGridBuilder.h"

#include <cmath>

namespace navigation {

NavigationGrid NavigationGridBuilder::BuildFromBounds(const MovementBounds& bounds, double cellSize) const {
    NavigationGrid grid;
    grid.cellSize = (cellSize > 0.0) ? cellSize : 1.0;

    double width = bounds.maxX - bounds.minX;
    double height = bounds.maxY - bounds.minY;

    if (!std::isfinite(width) || width <= 0.0) {
        width = 50.0;
    }
    if (!std::isfinite(height) || height <= 0.0) {
        height = 50.0;
    }

    grid.width = static_cast<int>(std::ceil(width / grid.cellSize));
    grid.height = static_cast<int>(std::ceil(height / grid.cellSize));
    grid.layers = 1;

    grid.origin.x = bounds.minX;
    grid.origin.y = bounds.minY;
    grid.origin.z = bounds.minZ;

    grid.walkableMask.resize(static_cast<size_t>(grid.width * grid.height), 1u);
    return grid;
}

} // namespace navigation

