#pragma once

#include <glad/glad.h>
#include <vector>

// Simple retained-mode 3D line/point batcher for compatibility OpenGL
// Uses client state arrays (no VAOs) and a dynamic VBO updated per flush.
class LineBatcher3D {
public:
    LineBatcher3D() = default;
    ~LineBatcher3D() = default;

    LineBatcher3D(const LineBatcher3D&) = delete;
    LineBatcher3D& operator=(const LineBatcher3D&) = delete;

    bool Init();
    void Begin();

    void AddLine(float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 float r, float g, float b, float a = 1.0f);

    void AddPoint(float x, float y, float z,
                  float r, float g, float b, float a = 1.0f);

    void SetLineWidth(float w) { lineWidth_ = w; }
    void SetPointSize(float s) { pointSize_ = s; }

    void Flush();
    void Cleanup();

private:
    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };

    GLuint vbo_ = 0;
    size_t vboCapacity_ = 0; // in vertices

    std::vector<Vertex> lineVerts_;
    std::vector<Vertex> pointVerts_;

    float lineWidth_ = 1.0f;
    float pointSize_ = 1.0f;

    void EnsureCapacity(size_t requiredVerts);
};
