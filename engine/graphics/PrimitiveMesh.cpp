#include "PrimitiveMesh.h"
#include "MeshSubmission.h"
#include <iostream>
#include <cstring>
#include <cstdint>

#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
#include <glad/glad.h>
#endif

PrimitiveMesh::PrimitiveMesh() = default;

PrimitiveMesh::~PrimitiveMesh() {
    Cleanup();
}

void PrimitiveMesh::Upload(const std::vector<float>& vertices,
                           const std::vector<unsigned int>& indices,
                           int vertexStrideBytes,
                           bool hasColor,
                           int colorOffsetBytes,
                           bool hasTexCoord,
                           int texCoordOffsetBytes,
                           int texCoordComponents,
                           int colorComponentCount) {
#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
    // If GL is not active, skip GPU upload but mark initialized so Draw can be called safely.
    // Create VAO/VBO/EBO
    if (vao_ != 0) {
        // already uploaded; replace buffers
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
        glDeleteVertexArrays(1, &vao_);
        vao_ = vbo_ = ebo_ = 0;
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty()) {
        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
        indexCount_ = static_cast<int>(indices.size());
    } else {
        indexCount_ = 0;
    }

    // Configure attributes using fixed-function compatible client state
    strideBytes_ = vertexStrideBytes;
    hasColor_ = hasColor;
    colorOffsetBytes_ = colorOffsetBytes;
    colorComponentCount_ = colorComponentCount;
    hasTexCoord_ = hasTexCoord;
    texCoordOffsetBytes_ = texCoordOffsetBytes;
    texCoordComponents_ = texCoordComponents;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, strideBytes_, reinterpret_cast<const void*>(0));

    if (hasColor_) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(colorComponentCount_, GL_FLOAT, strideBytes_, reinterpret_cast<const void*>(static_cast<uintptr_t>(colorOffsetBytes_)));
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    GLint previousClientTexture = 0;
#ifdef GL_CLIENT_ACTIVE_TEXTURE
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &previousClientTexture);
#endif
    glClientActiveTexture(GL_TEXTURE0);
    if (hasTexCoord_) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(texCoordComponents_, GL_FLOAT, strideBytes_, reinterpret_cast<const void*>(static_cast<uintptr_t>(texCoordOffsetBytes_)));
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
#ifdef GL_CLIENT_ACTIVE_TEXTURE
    glClientActiveTexture(static_cast<GLenum>(previousClientTexture));
#endif

    // Unbind VAO to be safe
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (ebo_ != 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Compute vertex count if indices weren't provided
    if (indexCount_ == 0 && vertexStrideBytes > 0) {
        vertexCount_ = static_cast<int>((vertices.size() * sizeof(float)) / vertexStrideBytes);
    }

    if (indexCount_ > 0 && vertexStrideBytes > 0) {
        vertexCount_ = static_cast<int>((vertices.size() * sizeof(float)) / vertexStrideBytes);
    }

    initialized_ = true;
#else
    // No GL available — keep counts for informational output
    indexCount_ = static_cast<int>(indices.size());
    if (vertexStrideBytes > 0) {
        vertexCount_ = static_cast<int>((vertices.size() * sizeof(float)) / vertexStrideBytes);
    } else {
        vertexCount_ = static_cast<int>(vertices.size());
    }
    strideBytes_ = vertexStrideBytes;
    hasColor_ = hasColor;
    colorOffsetBytes_ = colorOffsetBytes;
    hasTexCoord_ = hasTexCoord;
    texCoordOffsetBytes_ = texCoordOffsetBytes;
    texCoordComponents_ = texCoordComponents;
    colorComponentCount_ = colorComponentCount;
    initialized_ = true;
#endif
}

void PrimitiveMesh::Upload(const MeshSubmission& submission)
{
    Upload(submission.vertices,
           submission.indices,
           submission.vertexStrideBytes,
           submission.hasColor,
           submission.colorOffsetBytes,
           submission.hasTexCoord,
           submission.texCoordOffsetBytes,
           submission.texCoordComponents,
           submission.colorComponentCount);
}

void PrimitiveMesh::Draw() {
#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
    if (!initialized_) {
        std::cout << "PrimitiveMesh::Draw() called but not initialized" << std::endl;
        return;
    }
    if (vao_ == 0) {
        std::cout << "PrimitiveMesh::Draw() - no VAO (GL not initialized?)" << std::endl;
        return;
    }
    glBindVertexArray(vao_);
    if (indexCount_ > 0) {
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    } else if (vertexCount_ > 0) {
        glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
    }
    glBindVertexArray(0);
#else
    // Safe ASCII fallback for headless environments
    std::cout << "PrimitiveMesh::Draw() [ASCII fallback] - indices=" << indexCount_ << " vertices=" << vertexCount_ << std::endl;
#endif
}

void PrimitiveMesh::Cleanup() {
#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (ebo_ != 0) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
#else
    (void)indexCount_;
    (void)vertexCount_;
#endif
    initialized_ = false;
    indexCount_ = 0;
    vertexCount_ = 0;
    strideBytes_ = 0;
    hasColor_ = false;
    colorOffsetBytes_ = 0;
    hasTexCoord_ = false;
    texCoordOffsetBytes_ = 0;
    texCoordComponents_ = 2;
}
