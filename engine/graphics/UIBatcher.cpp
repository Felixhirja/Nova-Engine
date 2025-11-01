#include "UIBatcher.h"
#include "ShaderProgram.h"

#include <algorithm>
#include <cstddef>

UIBatcher::UIBatcher() {
    // Constructor - resources initialized in Init()
}

UIBatcher::~UIBatcher() {
    Cleanup();
}

bool UIBatcher::Init() {
    // Generate VAO
    glGenVertexArrays(1, &vao_);
    if (vao_ == 0) {
        return false;
    }
    
    // Generate VBO
    glGenBuffers(1, &vbo_);
    if (vbo_ == 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
        return false;
    }
    
    // Generate IBO (Index Buffer Object for quads)
    glGenBuffers(1, &ibo_);
    if (ibo_ == 0) {
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
        vbo_ = 0;
        vao_ = 0;
        return false;
    }
    
    // Bind VAO
    glBindVertexArray(vao_);
    
    // Bind and setup VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Define vertex format (interleaved)
    // Attribute 0: Position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), 
                         (void*)offsetof(UIVertex, x));
    
    // Attribute 1: Color (vec4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                         (void*)offsetof(UIVertex, r));
    
    // Bind IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    
    // Unbind VAO (keeps IBO bound to VAO)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    // Initial capacity: 100 quads = 400 vertices, 600 indices
    vboCapacity_ = 400;
    iboCapacity_ = 600;
    
    // Allocate VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vboCapacity_ * sizeof(UIVertex), 
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Allocate IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboCapacity_ * sizeof(GLuint),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    shader_ = std::make_unique<ShaderProgram>();
    if (!shader_->LoadFromFiles("shaders/core/ui_batcher.vert", "shaders/core/ui_batcher.frag")) {
        Cleanup();
        return false;
    }

    return true;
}

void UIBatcher::Cleanup() {
    if (shader_) {
        shader_->Cleanup();
        shader_.reset();
    }

    if (ibo_ != 0) {
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }
    
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    
    vboCapacity_ = 0;
    iboCapacity_ = 0;
    quadCount_ = 0;
    lastRenderCount_ = 0;
}

void UIBatcher::EnsureCapacity(size_t requiredVertices, size_t requiredIndices) {
    bool needResize = false;
    
    if (requiredVertices > vboCapacity_) {
        vboCapacity_ = std::max(requiredVertices, vboCapacity_ + vboCapacity_ / 2);
        if (vboCapacity_ == 0) {
            vboCapacity_ = std::max(requiredVertices, size_t(400));
        }
        needResize = true;
    }
    
    if (requiredIndices > iboCapacity_) {
        iboCapacity_ = std::max(requiredIndices, iboCapacity_ + iboCapacity_ / 2);
        if (iboCapacity_ == 0) {
            iboCapacity_ = std::max(requiredIndices, size_t(600));
        }
        needResize = true;
    }
    
    if (!needResize) {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vboCapacity_ * sizeof(UIVertex),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboCapacity_ * sizeof(GLuint),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void UIBatcher::Begin(int screenWidth, int screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    projectionDirty_ = true;

    // Clear batch
    vertices_.clear();
    indices_.clear();
    quadCount_ = 0;
}

void UIBatcher::AddQuad(float x, float y, float width, float height,
                       float r, float g, float b, float a) {
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }
    
    const float x1 = x;
    const float y1 = y;
    const float x2 = x + width;
    const float y2 = y + height;
    
    const GLuint baseIndex = static_cast<GLuint>(vertices_.size());
    
    vertices_.push_back({x1, y1, r, g, b, a});
    vertices_.push_back({x2, y1, r, g, b, a});
    vertices_.push_back({x2, y2, r, g, b, a});
    vertices_.push_back({x1, y2, r, g, b, a});
    
    indices_.push_back(baseIndex + 0);
    indices_.push_back(baseIndex + 1);
    indices_.push_back(baseIndex + 2);
    
    indices_.push_back(baseIndex + 0);
    indices_.push_back(baseIndex + 2);
    indices_.push_back(baseIndex + 3);
    
    quadCount_++;
}

void UIBatcher::AddRectOutline(float x, float y, float width, float height,
                               float thickness,
                               float r, float g, float b, float a) {
    if (thickness <= 0.0f || width <= 0.0f || height <= 0.0f) {
        return;
    }
    
    const float clampedThickness = std::min(thickness, std::min(width, height) * 0.5f);
    
    AddQuad(x, y, width, clampedThickness, r, g, b, a);
    AddQuad(x, y + height - clampedThickness, width, clampedThickness, r, g, b, a);
    AddQuad(x, y, clampedThickness, height, r, g, b, a);
    AddQuad(x + width - clampedThickness, y, clampedThickness, height, r, g, b, a);
}

void UIBatcher::AddTriangle(float x1, float y1,
                            float x2, float y2,
                            float x3, float y3,
                            float r, float g, float b, float a) {
    GLuint baseIndex = static_cast<GLuint>(vertices_.size());
    vertices_.push_back({x1, y1, r, g, b, a});
    vertices_.push_back({x2, y2, r, g, b, a});
    vertices_.push_back({x3, y3, r, g, b, a});

    indices_.push_back(baseIndex + 0);
    indices_.push_back(baseIndex + 1);
    indices_.push_back(baseIndex + 2);
}

void UIBatcher::Flush() {
    if (vertices_.empty() || vao_ == 0 || !shader_) {
        lastRenderCount_ = 0;
        return;
    }

    // Ensure buffers have enough capacity
    EnsureCapacity(vertices_.size(), indices_.size());

    UpdateProjectionMatrix();

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                   vertices_.size() * sizeof(UIVertex),
                   vertices_.data());
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                   indices_.size() * sizeof(GLuint),
                   indices_.data());
    
    shader_->Use();
    shader_->SetUniformMatrix4("uProjection", projectionMatrix_.data());

    // Bind VAO and draw
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()),
                  GL_UNSIGNED_INT, nullptr);

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    ShaderProgram::Unuse();

    lastRenderCount_ = quadCount_;

    // Clear batch for next frame
    vertices_.clear();
    indices_.clear();
    quadCount_ = 0;
}

void UIBatcher::UpdateProjectionMatrix() {
    if (!projectionDirty_) {
        return;
    }

    if (screenWidth_ <= 0 || screenHeight_ <= 0) {
        projectionMatrix_ = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        projectionDirty_ = false;
        return;
    }

    const float left = 0.0f;
    const float right = static_cast<float>(screenWidth_);
    const float top = 0.0f;
    const float bottom = static_cast<float>(screenHeight_);
    const float nearPlane = -1.0f;
    const float farPlane = 1.0f;

    const float rl = right - left;
    const float tb = top - bottom;
    const float fn = farPlane - nearPlane;

    if (rl == 0.0f || tb == 0.0f || fn == 0.0f) {
        projectionMatrix_ = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        projectionDirty_ = false;
        return;
    }

    projectionMatrix_ = {
        2.0f / rl,                0.0f,                0.0f,                 0.0f,
        0.0f,                     2.0f / tb,           0.0f,                 0.0f,
        0.0f,                     0.0f,               -2.0f / fn,            0.0f,
       -(right + left) / rl,     -(top + bottom) / tb, -(farPlane + nearPlane) / fn, 1.0f,
    };

    projectionDirty_ = false;
}
