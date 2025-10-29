#include "InstancedMeshRenderer.h"
#include <iostream>
#include <cstddef>

namespace Nova {

InstancedMeshRenderer::InstancedMeshRenderer() {
}

InstancedMeshRenderer::~InstancedMeshRenderer() {
    Clear();
}

bool InstancedMeshRenderer::Initialize() {
    // Create the instanced shader
    m_shader = std::make_unique<ShaderProgram>();
    if (!m_shader->LoadFromFiles("shaders/core/basic.vert", "shaders/core/basic.frag")) {
        std::cerr << "Failed to load instanced rendering shader" << std::endl;
        return false;
    }

    return true;
}

void InstancedMeshRenderer::Submit(const MeshHandle& mesh, const std::shared_ptr<Material>& material,
                                  const glm::mat4& transform, const glm::vec3& colorTint, float customScalar) {
    Submit(mesh, material, transform, glm::vec4(colorTint, 1.0f), customScalar);
}

void InstancedMeshRenderer::Submit(const MeshHandle& mesh, const std::shared_ptr<Material>& material,
                                  const glm::mat4& transform, const glm::vec4& colorTint, float customScalar) {
    BatchKey key{mesh, material};

    auto& batch = m_batches[key];
    batch.instances.push_back({transform, colorTint, customScalar});
    batch.dirty = true;
}

void InstancedMeshRenderer::Flush(const glm::mat4& viewProjectionMatrix) {
    if (!m_shader) return;

    m_shader->Use();

    // Set view-projection matrix
    m_shader->SetUniformMatrix4("uViewProjection", &viewProjectionMatrix[0][0]);

    for (auto& pair : m_batches) {
        const BatchKey& key = pair.first;
        Batch& batch = pair.second;

        if (batch.instances.empty()) continue;

        // Setup instance buffer if needed
        if (batch.instanceVBO == 0) {
            SetupInstanceBuffer(batch);
        }

        // Update instance buffer if dirty
        if (batch.dirty) {
            UpdateInstanceBuffer(batch);
            batch.dirty = false;
        }

        // Bind mesh
        glBindVertexArray(key.mesh.vao);

        // Bind material
        if (key.material) {
            key.material->Bind(m_shader.get());
        }

        // Bind instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);

        // Setup instance attribute pointers
        const GLsizei stride = static_cast<GLsizei>(INSTANCE_DATA_SIZE);
        const std::size_t matrixOffset = offsetof(InstanceData, modelMatrix);
        const std::size_t colorOffset = offsetof(InstanceData, colorTint);
        const std::size_t scalarOffset = offsetof(InstanceData, customScalar);

        // Model matrix (4 vec4s)
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE,
                                stride, reinterpret_cast<const void*>(matrixOffset + i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1); // Advance per instance
        }

        // Color tint (vec4)
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE,
                            stride, reinterpret_cast<const void*>(colorOffset));
        glVertexAttribDivisor(7, 1);

        // Custom scalar (float)
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE,
                            stride, reinterpret_cast<const void*>(scalarOffset));
        glVertexAttribDivisor(8, 1);

        // Draw instances
        if (key.mesh.ebo != 0) {
            glDrawElementsInstanced(GL_TRIANGLES, key.mesh.indexCount, key.mesh.indexType,
                                  nullptr, static_cast<GLsizei>(batch.instances.size()));
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, key.mesh.vertexCount,
                                static_cast<GLsizei>(batch.instances.size()));
        }

        // Unbind material
        if (key.material) {
            key.material->Unbind();
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void InstancedMeshRenderer::Clear() {
    for (auto& pair : m_batches) {
        if (pair.second.instanceVBO != 0) {
            glDeleteBuffers(1, &pair.second.instanceVBO);
        }
    }
    m_batches.clear();
}

size_t InstancedMeshRenderer::GetInstanceCount() const {
    size_t total = 0;
    for (const auto& pair : m_batches) {
        total += pair.second.instances.size();
    }
    return total;
}

size_t InstancedMeshRenderer::GetBatchCount() const {
    return m_batches.size();
}

void InstancedMeshRenderer::SetupInstanceBuffer(Batch& batch) {
    glGenBuffers(1, &batch.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);
    // Allocate buffer (will be updated in UpdateInstanceBuffer)
    const GLsizeiptr bufferSize = static_cast<GLsizeiptr>(batch.instances.size() * INSTANCE_DATA_SIZE);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstancedMeshRenderer::UpdateInstanceBuffer(Batch& batch) {
    if (batch.instanceVBO == 0 || batch.instances.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);

    const GLsizeiptr byteSize = static_cast<GLsizeiptr>(batch.instances.size() * INSTANCE_DATA_SIZE);
    glBufferData(GL_ARRAY_BUFFER, byteSize, batch.instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace Nova