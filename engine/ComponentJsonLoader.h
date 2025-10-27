#pragma once

#include "ShipAssembly.h"

#include <string>
#include <vector>

namespace ComponentJsonLoader {

// Loads all component blueprints from JSON files in the specified directory.
// Returns true if at least one component was loaded successfully.
bool LoadComponentsFromDirectory(const std::string& directoryPath);

// Hot-reloadable version that only reloads files that have changed since last load.
// Returns true if loading was successful (even if no files changed).
bool LoadComponentsFromDirectoryHotReload(const std::string& directoryPath);

// Loads a single component blueprint from a JSON file.
// Returns true if the component was loaded successfully.
bool LoadComponentFromFile(const std::string& filePath, ShipComponentBlueprint& outBlueprint);

} // namespace ComponentJsonLoader