#pragma once

#include <vector>

struct MeshSubmission;

class PrimitiveMesh {
public:
    PrimitiveMesh();
    ~PrimitiveMesh();

    // Upload interleaved vertex data and optional indices.
    // vertexStrideBytes is the byte stride between consecutive vertices.
    // hasColor indicates the vertex layout includes RGB after position.
    void Upload(const std::vector<float>& vertices,
                const std::vector<unsigned int>& indices,
                int vertexStrideBytes,
                bool hasColor = true,
                int colorOffsetBytes = static_cast<int>(sizeof(float) * 3),
                bool hasTexCoord = false,
                int texCoordOffsetBytes = 0,
                int texCoordComponents = 2,
                int colorComponentCount = 3);

    void Upload(const MeshSubmission& submission);

    // Draw the uploaded primitive. If GL is not available this will print a small message.
    void Draw();

    // Release any GPU resources held by this object.
    void Cleanup();

    bool IsInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
#endif
    int indexCount_ = 0;
    int vertexCount_ = 0;
    int strideBytes_ = 0;
    bool hasColor_ = false;
    int colorOffsetBytes_ = 0;
    int colorComponentCount_ = 3;
    bool hasTexCoord_ = false;
    int texCoordOffsetBytes_ = 0;
    int texCoordComponents_ = 2;
#if (defined(USE_GLFW) || defined(USE_SDL)) && !defined(PRIMITIVEMESH_FORCE_NO_GL)
#endif
};
