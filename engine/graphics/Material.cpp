#include "Material.h"
#include "ShaderProgram.h"

namespace Nova {

Material::Material(const std::string& name) : m_name(name) {
}

Material::~Material() {
    // Unload textures
    for (auto& pair : m_textures) {
        if (pair.second.textureId != 0) {
            glDeleteTextures(1, &pair.second.textureId);
        }
    }
}

bool Material::LoadFromJson(const std::string& jsonPath) {
    // TODO: Implement JSON loading when nlohmann/json is available
    // For now, create a basic material
    (void)jsonPath;
    m_loaded = true;
    return true;
}

void Material::AddTexture(const std::string& slotName, const std::string& texturePath) {
    TextureSlot slot;
    slot.name = slotName;
    slot.path = texturePath;
    
    glGenTextures(1, &slot.textureId);
    if (slot.textureId == 0) {
        return;
    }
    
    glBindTexture(GL_TEXTURE_2D, slot.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const unsigned char whitePixel[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, 0);

    slot.loaded = true;
    m_textures[slotName] = slot;
}

bool Material::HasTexture(const std::string& slotName) const {
    auto it = m_textures.find(slotName);
    return it != m_textures.end() && it->second.loaded;
}

GLuint Material::GetTexture(const std::string& slotName) const {
    auto it = m_textures.find(slotName);
    if (it != m_textures.end() && it->second.loaded) {
        return it->second.textureId;
    }
    return 0;
}

bool Material::LoadTextures() {
    bool success = true;
    for (auto& pair : m_textures) {
        if (!LoadTexture(pair.second)) {
            success = false;
        }
    }
    return success;
}

bool Material::LoadTexture(TextureSlot& slot) {
    (void)slot;
    return true;
}

void Material::Bind(ShaderProgram* shader) const {
    if (!shader) return;

    // Bind material parameters
    shader->SetUniform("material.baseColor", m_parameters.baseColor.x, m_parameters.baseColor.y, m_parameters.baseColor.z);
    shader->SetUniform("material.roughness", m_parameters.roughness);
    shader->SetUniform("material.metalness", m_parameters.metalness);
    shader->SetUniform("material.emissive", m_parameters.emissive.x, m_parameters.emissive.y, m_parameters.emissive.z);
    shader->SetUniform("material.alpha", m_parameters.alpha);

    // Bind textures
    int textureUnit = 0;
    for (const auto& pair : m_textures) {
        if (pair.second.loaded) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, pair.second.textureId);

            std::string uniformName = "material." + pair.first;
            shader->SetUniform(uniformName.c_str(), textureUnit);
            textureUnit++;
        }
    }
}

void Material::Unbind() const {
    // Unbind textures
    for (int i = 0; i < static_cast<int>(m_textures.size()); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

} // namespace Nova