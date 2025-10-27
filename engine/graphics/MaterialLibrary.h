#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include "Material.h"

namespace Nova {

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary();

    // Initialize the library
    bool Initialize();

    // Load a material from file (lazy loading)
    std::shared_ptr<Material> LoadMaterial(const std::string& materialPath);

    // Get a material by path (returns existing if already loaded)
    std::shared_ptr<Material> GetMaterial(const std::string& materialPath) const;

    // Create a material programmatically
    std::shared_ptr<Material> CreateMaterial(const std::string& name);

    // Unload unused materials
    void UnloadUnusedMaterials();

    // Get library statistics
    size_t GetLoadedMaterialCount() const;
    size_t GetTotalReferenceCount() const;

private:
    std::unordered_map<std::string, std::weak_ptr<Material>> m_materials;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_ownedMaterials;

    // Helper to clean up expired weak pointers
    void CleanupExpiredMaterials();
};

} // namespace Nova