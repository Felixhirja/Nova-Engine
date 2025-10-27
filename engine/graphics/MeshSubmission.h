#pragma once

#include <array>
#include <vector>

class Mesh;

struct MeshSubmission {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int vertexStrideBytes = 0;
    bool hasColor = false;
    int colorOffsetBytes = 0;
    bool hasTexCoord = false;
    int texCoordOffsetBytes = 0;
    int texCoordComponents = 2;
    int colorComponentCount = 3;
};

struct SpriteQuadDescriptor {
    float width = 1.0f;
    float height = 1.0f;
    float depth = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    bool anchorCenter = true;
    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct SpriteSheetDescriptor {
    int frameWidth = 0;
    int frameHeight = 0;
    int frameIndex = 0;
    int frameCount = 1;
    int columns = 1;
    float pixelsPerUnit = 1.0f;
    float depth = 0.0f;
    bool anchorCenter = true;
    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};

class MeshSubmissionBuilder {
public:
    static MeshSubmission FromMesh(const Mesh& mesh);
    static MeshSubmission SpriteQuad(const SpriteQuadDescriptor& desc);
    static MeshSubmission SpriteFrame(const SpriteSheetDescriptor& desc);

private:
    static MeshSubmission BuildSpriteSubmission(float width,
                                                float height,
                                                float depth,
                                                float u0,
                                                float v0,
                                                float u1,
                                                float v1,
                                                bool anchorCenter,
                                                const std::array<float, 4>& color);
};
