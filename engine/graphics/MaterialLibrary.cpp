#include "MaterialLibrary.h"
#include <iostream>
#include <algorithm>

namespace Nova {

MaterialLibrary::MaterialLibrary() {
}

MaterialLibrary::~MaterialLibrary() {
    // Materials will be automatically cleaned up due to shared_ptr
}

bool MaterialLibrary::Initialize() {
    // Create a default material
    auto defaultMaterial = CreateMaterial("default");
    if (defaultMaterial) {
        defaultMaterial->SetBaseColor(glm::vec3(0.8f, 0.8f, 0.8f));
        defaultMaterial->SetRoughness(0.5f);
        defaultMaterial->SetMetalness(0.0f);
    }

    return true;
}

std::shared_ptr<Material> MaterialLibrary::LoadMaterial(const std::string& materialPath) {
    // Check if already loaded
    auto existing = GetMaterial(materialPath);
    if (existing) {
        return existing;
    }

    // Create new material
    auto material = std::make_shared<Material>(materialPath);
    if (material->LoadFromJson(materialPath)) {
        m_ownedMaterials[materialPath] = material;
        m_materials[materialPath] = material;
        return material;
    }

    std::cerr << "Failed to load material: " << materialPath << std::endl;
    return nullptr;
}

std::shared_ptr<Material> MaterialLibrary::GetMaterial(const std::string& materialPath) const {
    auto it = m_materials.find(materialPath);
    if (it != m_materials.end()) {
        // Try to lock the weak pointer
        if (auto material = it->second.lock()) {
            return material;
        }
    }
    return nullptr;
}

std::shared_ptr<Material> MaterialLibrary::CreateMaterial(const std::string& name) {
    auto material = std::make_shared<Material>(name);
    m_ownedMaterials[name] = material;
    m_materials[name] = material;
    return material;
}

void MaterialLibrary::UnloadUnusedMaterials() {
    CleanupExpiredMaterials();

    // Remove materials that are no longer referenced
    for (auto it = m_ownedMaterials.begin(); it != m_ownedMaterials.end(); ) {
        if (it->second.use_count() <= 1) { // Only the library owns it
            it = m_ownedMaterials.erase(it);
        } else {
            ++it;
        }
    }
}

size_t MaterialLibrary::GetLoadedMaterialCount() const {
    return m_ownedMaterials.size();
}

size_t MaterialLibrary::GetTotalReferenceCount() const {
    size_t totalRefs = 0;
    for (const auto& pair : m_ownedMaterials) {
        totalRefs += pair.second.use_count();
    }
    return totalRefs;
}

void MaterialLibrary::CleanupExpiredMaterials() {
    for (auto it = m_materials.begin(); it != m_materials.end(); ) {
        if (it->second.expired()) {
            it = m_materials.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Nova