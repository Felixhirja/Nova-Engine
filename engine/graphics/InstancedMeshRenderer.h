#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstddef>
#include <type_traits>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "Material.h"
#include "ShaderProgram.h"

namespace Nova {

class ShaderManager;

struct MeshHandle {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei vertexCount = 0;
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_INT;
};

struct InstanceData {
    glm::mat4 modelMatrix;           // column-major world transform (meters)
    glm::vec4 colorTint = glm::vec4(1.0f); // RGBA tint, normalized [0,1]
    float customScalar = 0.0f;       // unit-less per-instance parameter
};

static_assert(std::is_standard_layout_v<InstanceData>,
              "InstanceData must be standard layout for GPU uploads");

class InstancedMeshRenderer {
public:
    InstancedMeshRenderer();
    ~InstancedMeshRenderer();

    // Initialize the renderer
    bool Initialize(ShaderManager* shaderManager = nullptr);

    // Submit an instance for rendering
    void Submit(const MeshHandle& mesh, const std::shared_ptr<Material>& material,
                const glm::mat4& transform, const glm::vec3& colorTint = glm::vec3(1.0f),
                float customScalar = 0.0f);
    void Submit(const MeshHandle& mesh, const std::shared_ptr<Material>& material,
                const glm::mat4& transform, const glm::vec4& colorTint,
                float customScalar = 0.0f);

    // Flush all batched instances to GPU
    void Flush(const glm::mat4& viewProjectionMatrix);

    // Clear all submitted instances
    void Clear();

    // Get statistics
    size_t GetInstanceCount() const;
    size_t GetBatchCount() const;

private:
    struct BatchKey {
        MeshHandle mesh;
        std::shared_ptr<Material> material;

        bool operator==(const BatchKey& other) const {
            return mesh.vao == other.mesh.vao && material == other.material;
        }
    };

    struct BatchKeyHash {
        std::size_t operator()(const BatchKey& key) const {
            std::size_t h1 = std::hash<GLuint>()(key.mesh.vao);
            std::size_t h2 = std::hash<std::shared_ptr<Material>>()(key.material);
            return h1 ^ (h2 << 1);
        }
    };

    struct Batch {
        std::vector<InstanceData> instances;
        GLuint instanceVBO = 0;
        bool dirty = true;
    };

    std::unordered_map<BatchKey, Batch, BatchKeyHash> m_batches;
    std::shared_ptr<ShaderProgram> m_shader;
    ShaderManager* shaderManager_ = nullptr;
    std::string shaderName_ = "core.basic";

    // Instance buffer layout
    static constexpr std::size_t INSTANCE_DATA_SIZE = sizeof(InstanceData);

    void SetupInstanceBuffer(Batch& batch);
    void UpdateInstanceBuffer(Batch& batch);
};

} // namespace Nova