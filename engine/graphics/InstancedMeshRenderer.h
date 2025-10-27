#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Material.h"
#include "ShaderProgram.h"

namespace Nova {

struct MeshHandle {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei vertexCount = 0;
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_INT;
};

struct InstanceData {
    glm::mat4 modelMatrix;
    glm::vec3 colorTint = glm::vec3(1.0f);
    float customScalar = 0.0f;
};

class InstancedMeshRenderer {
public:
    InstancedMeshRenderer();
    ~InstancedMeshRenderer();

    // Initialize the renderer
    bool Initialize();

    // Submit an instance for rendering
    void Submit(const MeshHandle& mesh, const std::shared_ptr<Material>& material,
                const glm::mat4& transform, const glm::vec3& colorTint = glm::vec3(1.0f),
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
    std::unique_ptr<ShaderProgram> m_shader;

    // Instance buffer layout
    static constexpr size_t INSTANCE_DATA_SIZE = sizeof(glm::mat4) + sizeof(glm::vec3) + sizeof(float);

    void SetupInstanceBuffer(Batch& batch);
    void UpdateInstanceBuffer(Batch& batch);
};

} // namespace Nova