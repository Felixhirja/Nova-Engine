#include "MeshSubmission.h"

#include "../Mesh.h"

#include <algorithm>

namespace {
constexpr int kPositionComponentCount = 3;
constexpr int kColorComponentCount = 4;
constexpr int kUVComponentCount = 2;
}

MeshSubmission MeshSubmissionBuilder::FromMesh(const Mesh& mesh)
{
    MeshSubmission submission;

    const auto& vertices = mesh.Vertices();
    const auto& indices = mesh.Indices();

    const bool hasColor = (mesh.Attributes() & MeshAttribute_Color) != 0;
    const bool hasTexCoord = (mesh.Attributes() & MeshAttribute_TexCoord) != 0;

    submission.hasColor = hasColor;
    submission.hasTexCoord = hasTexCoord;
    submission.colorComponentCount = hasColor ? kColorComponentCount : 0;

    int floatsPerVertex = kPositionComponentCount;
    submission.colorOffsetBytes = hasColor ? floatsPerVertex * static_cast<int>(sizeof(float)) : 0;
    if (hasColor) {
        floatsPerVertex += kColorComponentCount;
    }

    submission.texCoordOffsetBytes = hasTexCoord ? floatsPerVertex * static_cast<int>(sizeof(float)) : 0;
    if (hasTexCoord) {
        floatsPerVertex += kUVComponentCount;
    }

    submission.vertexStrideBytes = floatsPerVertex * static_cast<int>(sizeof(float));

    submission.vertices.reserve(vertices.size() * static_cast<std::size_t>(floatsPerVertex));
    for (const auto& v : vertices) {
        submission.vertices.push_back(v.px);
        submission.vertices.push_back(v.py);
        submission.vertices.push_back(v.pz);

        if (hasColor) {
            submission.vertices.push_back(v.r);
            submission.vertices.push_back(v.g);
            submission.vertices.push_back(v.b);
            submission.vertices.push_back(v.a);
        }

        if (hasTexCoord) {
            submission.vertices.push_back(v.u);
            submission.vertices.push_back(v.v);
        }
    }

    submission.indices.assign(indices.begin(), indices.end());

    return submission;
}

MeshSubmission MeshSubmissionBuilder::SpriteQuad(const SpriteQuadDescriptor& desc)
{
    return BuildSpriteSubmission(desc.width,
                                 desc.height,
                                 desc.depth,
                                 desc.u0,
                                 desc.v0,
                                 desc.u1,
                                 desc.v1,
                                 desc.anchorCenter,
                                 desc.color);
}

MeshSubmission MeshSubmissionBuilder::SpriteFrame(const SpriteSheetDescriptor& desc)
{
    SpriteSheetDescriptor safe = desc;
    if (safe.columns <= 0) {
        safe.columns = 1;
    }
    if (safe.frameCount <= 0) {
        safe.frameCount = 1;
    }
    safe.frameIndex = std::clamp(safe.frameIndex, 0, safe.frameCount - 1);

    const int rows = (safe.frameCount + safe.columns - 1) / safe.columns;
    const float pixelsPerUnit = (safe.pixelsPerUnit > 0.0f) ? safe.pixelsPerUnit : 1.0f;

    const float width = static_cast<float>(safe.frameWidth) / pixelsPerUnit;
    const float height = static_cast<float>(safe.frameHeight) / pixelsPerUnit;

    const int column = safe.frameIndex % safe.columns;
    const int row = safe.frameIndex / safe.columns;

    const float sheetWidth = static_cast<float>(safe.columns * safe.frameWidth);
    const float sheetHeight = static_cast<float>(std::max(1, rows) * safe.frameHeight);

    const float u0 = sheetWidth > 0.0f ? (static_cast<float>(column * safe.frameWidth) / sheetWidth) : 0.0f;
    const float u1 = sheetWidth > 0.0f ? (static_cast<float>((column + 1) * safe.frameWidth) / sheetWidth) : 1.0f;
    const float v0 = sheetHeight > 0.0f ? (static_cast<float>(row * safe.frameHeight) / sheetHeight) : 0.0f;
    const float v1 = sheetHeight > 0.0f ? (static_cast<float>((row + 1) * safe.frameHeight) / sheetHeight) : 1.0f;

    return BuildSpriteSubmission(width,
                                 height,
                                 safe.depth,
                                 u0,
                                 v0,
                                 u1,
                                 v1,
                                 safe.anchorCenter,
                                 safe.color);
}

MeshSubmission MeshSubmissionBuilder::BuildSpriteSubmission(float width,
                                                            float height,
                                                            float depth,
                                                            float u0,
                                                            float v0,
                                                            float u1,
                                                            float v1,
                                                            bool anchorCenter,
                                                            const std::array<float, 4>& color)
{
    MeshSubmission submission;
    submission.hasColor = true;
    submission.hasTexCoord = true;
    submission.colorComponentCount = kColorComponentCount;

    const float minX = anchorCenter ? -width * 0.5f : 0.0f;
    const float maxX = anchorCenter ? width * 0.5f : width;
    const float minY = anchorCenter ? -height * 0.5f : 0.0f;
    const float maxY = anchorCenter ? height * 0.5f : height;

    const int floatsPerVertex = kPositionComponentCount + kColorComponentCount + kUVComponentCount;
    submission.vertexStrideBytes = floatsPerVertex * static_cast<int>(sizeof(float));
    submission.colorOffsetBytes = kPositionComponentCount * static_cast<int>(sizeof(float));
    submission.texCoordOffsetBytes = (kPositionComponentCount + kColorComponentCount) * static_cast<int>(sizeof(float));
    submission.texCoordComponents = kUVComponentCount;

    submission.vertices.reserve(4 * floatsPerVertex);

    const auto pushVertex = [&](float x, float y, float z, float u, float v) {
        submission.vertices.push_back(x);
        submission.vertices.push_back(y);
        submission.vertices.push_back(z);
        submission.vertices.push_back(color[0]);
        submission.vertices.push_back(color[1]);
        submission.vertices.push_back(color[2]);
        submission.vertices.push_back(color[3]);
        submission.vertices.push_back(u);
        submission.vertices.push_back(v);
    };

    // Bottom-left
    pushVertex(minX, minY, depth, u0, v0);
    // Bottom-right
    pushVertex(maxX, minY, depth, u1, v0);
    // Top-left
    pushVertex(minX, maxY, depth, u0, v1);
    // Top-right
    pushVertex(maxX, maxY, depth, u1, v1);

    submission.indices = {0, 1, 2, 2, 1, 3};

    return submission;
}
