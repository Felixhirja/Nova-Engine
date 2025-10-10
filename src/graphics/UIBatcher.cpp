#include "UIBatcher.h"
#include <algorithm>

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
    
    return true;
}

void UIBatcher::Cleanup() {
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
    
    // Check vertex capacity
    if (requiredVertices > vboCapacity_) {
        vboCapacity_ = std::max(requiredVertices, vboCapacity_ + vboCapacity_ / 2);
        needResize = true;
    }
    
    // Check index capacity
    if (requiredIndices > iboCapacity_) {
        iboCapacity_ = std::max(requiredIndices, iboCapacity_ + iboCapacity_ / 2);
        needResize = true;
    }
    
    if (!needResize) {
        return;
    }
    
    // Reallocate VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vboCapacity_ * sizeof(UIVertex),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Reallocate IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboCapacity_ * sizeof(GLuint),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void UIBatcher::Begin(int screenWidth, int screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    // Clear batch
    vertices_.clear();
    indices_.clear();
    quadCount_ = 0;
}

void UIBatcher::AddQuad(float x, float y, float width, float height,
                       float r, float g, float b, float a) {
    // Calculate quad corners
    // Note: Caller is responsible for setting up correct projection
    // (top-left origin vs bottom-left origin)
    float x1 = x;
    float y1 = y;
    float x2 = x + width;
    float y2 = y + height;
    
    // Add 4 vertices (order doesn't matter since we're using indexed rendering)
    GLuint baseIndex = static_cast<GLuint>(vertices_.size());
    
    vertices_.push_back({x1, y1, r, g, b, a});  // Top-left (or bottom-left)
    vertices_.push_back({x2, y1, r, g, b, a});  // Top-right (or bottom-right)
    vertices_.push_back({x2, y2, r, g, b, a});  // Bottom-right (or top-right)
    vertices_.push_back({x1, y2, r, g, b, a});  // Bottom-left (or top-left)
    
    // Add 6 indices for 2 triangles (quad)
    // Triangle 1: 0-1-2
    indices_.push_back(baseIndex + 0);
    indices_.push_back(baseIndex + 1);
    indices_.push_back(baseIndex + 2);
    
    // Triangle 2: 0-2-3
    indices_.push_back(baseIndex + 0);
    indices_.push_back(baseIndex + 2);
    indices_.push_back(baseIndex + 3);
    
    quadCount_++;
}

void UIBatcher::Flush() {
    if (vertices_.empty() || vao_ == 0) {
        lastRenderCount_ = 0;
        return;
    }
    
    // Ensure buffers have enough capacity
    EnsureCapacity(vertices_.size(), indices_.size());
    
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
    
    // Bind VAO and draw
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()),
                  GL_UNSIGNED_INT, nullptr);
    
    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    lastRenderCount_ = quadCount_;
    
    // Clear batch for next frame
    vertices_.clear();
    indices_.clear();
    quadCount_ = 0;
}
