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
    shader->SetUniform("u_BaseColor", m_parameters.baseColor.x, m_parameters.baseColor.y, m_parameters.baseColor.z);
    shader->SetUniform("u_Roughness", m_parameters.roughness);
    shader->SetUniform("u_Metalness", m_parameters.metalness);
    shader->SetUniform("u_Emissive", m_parameters.emissive.x, m_parameters.emissive.y, m_parameters.emissive.z);
    shader->SetUniform("u_Alpha", m_parameters.alpha);

    // Bind PBR textures with specific texture units
    int textureUnit = 0;
    
    // Albedo map (unit 0)
    if (HasTexture("albedo")) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, GetTexture("albedo"));
        shader->SetUniform("u_AlbedoMap", textureUnit);
        shader->SetUniform("u_HasAlbedoMap", true);
        textureUnit++;
    } else {
        shader->SetUniform("u_HasAlbedoMap", false);
    }
    
    // Normal map (unit 1)
    if (HasTexture("normal")) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, GetTexture("normal"));
        shader->SetUniform("u_NormalMap", textureUnit);
        shader->SetUniform("u_HasNormalMap", true);
        textureUnit++;
    } else {
        shader->SetUniform("u_HasNormalMap", false);
    }
    
    // Metallic-Roughness map (unit 2)
    if (HasTexture("metallicRoughness")) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, GetTexture("metallicRoughness"));
        shader->SetUniform("u_MetallicRoughnessMap", textureUnit);
        shader->SetUniform("u_HasMetallicRoughnessMap", true);
        textureUnit++;
    } else {
        shader->SetUniform("u_HasMetallicRoughnessMap", false);
    }
    
    // AO map (unit 3)
    if (HasTexture("ao")) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, GetTexture("ao"));
        shader->SetUniform("u_AOMap", textureUnit);
        shader->SetUniform("u_HasAOMap", true);
        textureUnit++;
    } else {
        shader->SetUniform("u_HasAOMap", false);
    }
    
    // Emissive map (unit 4)
    if (HasTexture("emissive")) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, GetTexture("emissive"));
        shader->SetUniform("u_EmissiveMap", textureUnit);
        shader->SetUniform("u_HasEmissiveMap", true);
        textureUnit++;
    } else {
        shader->SetUniform("u_HasEmissiveMap", false);
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