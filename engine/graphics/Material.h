#define _USE_MATH_DEFINES
#include <cmath>

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "ShaderProgram.h"

namespace Nova {

struct MaterialParameters {
    glm::vec3 baseColor = glm::vec3(1.0f);
    float roughness = 0.5f;
    float metalness = 0.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float alpha = 1.0f;

    // Custom scalar parameters
    std::unordered_map<std::string, float> scalars;
    // Custom vector parameters
    std::unordered_map<std::string, glm::vec3> vectors;
};

struct TextureSlot {
    std::string name;
    std::string path;
    GLuint textureId = 0;
    bool loaded = false;
};

class Material {
public:
    Material(const std::string& name);
    ~Material();

    // Load from JSON descriptor
    bool LoadFromJson(const std::string& jsonPath);

    // Parameter access
    void SetBaseColor(const glm::vec3& color) { m_parameters.baseColor = color; }
    void SetRoughness(float roughness) { m_parameters.roughness = roughness; }
    void SetMetalness(float metalness) { m_parameters.metalness = metalness; }
    void SetEmissive(const glm::vec3& emissive) { m_parameters.emissive = emissive; }
    void SetAlpha(float alpha) { m_parameters.alpha = alpha; }

    const glm::vec3& GetBaseColor() const { return m_parameters.baseColor; }
    float GetRoughness() const { return m_parameters.roughness; }
    float GetMetalness() const { return m_parameters.metalness; }
    const glm::vec3& GetEmissive() const { return m_parameters.emissive; }
    float GetAlpha() const { return m_parameters.alpha; }

    // Texture management
    void AddTexture(const std::string& slotName, const std::string& texturePath);
    bool HasTexture(const std::string& slotName) const;
    GLuint GetTexture(const std::string& slotName) const;

    // Shader binding
    void Bind(ShaderProgram* shader) const;
    void Unbind() const;

    // State
    const std::string& GetName() const { return m_name; }
    bool IsLoaded() const { return m_loaded; }
    void SetShaderName(const std::string& shaderName) { m_shaderName = shaderName; }
    const std::string& GetShaderName() const { return m_shaderName; }

private:
    bool LoadTextures();
    bool LoadTexture(TextureSlot& slot);

    std::string m_name;
    std::string m_shaderName = "shaders/core/basic";
    MaterialParameters m_parameters;
    std::unordered_map<std::string, TextureSlot> m_textures;
    bool m_loaded = false;

    // Cached uniform locations
    mutable std::unordered_map<std::string, GLint> m_uniformLocations;
};

} // namespace Nova