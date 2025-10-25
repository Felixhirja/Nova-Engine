#include "LineBatcher3D.h"
#include <algorithm>

bool LineBatcher3D::Init() {
    if (vbo_ == 0) {
        glGenBuffers(1, &vbo_);
        if (vbo_ == 0) return false;
    }
    vboCapacity_ = 0;
    lineVerts_.clear();
    pointVerts_.clear();
    return true;
}

void LineBatcher3D::Begin() {
    lineVerts_.clear();
    pointVerts_.clear();
}

void LineBatcher3D::AddLine(float x1, float y1, float z1,
                            float x2, float y2, float z2,
                            float r, float g, float b, float a) {
    lineVerts_.push_back(Vertex{x1, y1, z1, r, g, b, a});
    lineVerts_.push_back(Vertex{x2, y2, z2, r, g, b, a});
}

void LineBatcher3D::AddPoint(float x, float y, float z,
                             float r, float g, float b, float a) {
    pointVerts_.push_back(Vertex{x, y, z, r, g, b, a});
}

void LineBatcher3D::EnsureCapacity(size_t requiredVerts) {
    if (requiredVerts > vboCapacity_) {
        vboCapacity_ = std::max(requiredVerts, vboCapacity_ * 2 + 256);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vboCapacity_ * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void LineBatcher3D::Flush() {
    // Draw lines
    if (!lineVerts_.empty()) {
        EnsureCapacity(lineVerts_.size());
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineVerts_.size() * sizeof(Vertex), lineVerts_.data());

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(0));
        glColorPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));

        glLineWidth(lineWidth_);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVerts_.size()));
        glLineWidth(1.0f);

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Draw points
    if (!pointVerts_.empty()) {
        EnsureCapacity(pointVerts_.size());
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, pointVerts_.size() * sizeof(Vertex), pointVerts_.data());

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(0));
        glColorPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));

        glPointSize(pointSize_);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(pointVerts_.size()));
        glPointSize(1.0f);

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void LineBatcher3D::Cleanup() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    vboCapacity_ = 0;
    lineVerts_.clear();
    pointVerts_.clear();
}
