#pragma once

#include <glad/glad.h>
#include <vector>

// Modern GPU-based UI quad batcher using VAO/VBO
// Replaces immediate mode glBegin(GL_QUADS) with efficient batched rendering
// Designed for 2D screen-space orthographic UI elements
class UIBatcher {
public:
    UIBatcher();
    ~UIBatcher();
    
    // Delete copy constructor/assignment (OpenGL resources not copyable)
    UIBatcher(const UIBatcher&) = delete;
    UIBatcher& operator=(const UIBatcher&) = delete;
    
    // Initialize OpenGL resources (VAO, VBO, IBO)
    bool Init();
    
    // Begin accumulating UI quads (call at start of frame)
    void Begin(int screenWidth, int screenHeight);
    
    // Add a colored quad to the batch
    // x, y: Bottom-left corner in screen space
    // width, height: Quad dimensions in pixels
    // r, g, b, a: Color (0.0-1.0 range)
    void AddQuad(float x, float y, float width, float height,
                float r, float g, float b, float a = 1.0f);

    // Add a rectangle outline (border) using 4 thin quads
    // thickness: border thickness in pixels
    void AddRectOutline(float x, float y, float width, float height,
                        float thickness,
                        float r, float g, float b, float a = 1.0f);

    // Add a triangle (2D) to the batch
    void AddTriangle(float x1, float y1,
                     float x2, float y2,
                     float x3, float y3,
                     float r, float g, float b, float a = 1.0f);
    
    // Render all accumulated quads and clear batch (call at end of frame)
    void Flush();
    
    // Cleanup OpenGL resources
    void Cleanup();
    
    // Get statistics
    int GetQuadCount() const { return quadCount_; }
    int GetLastRenderCount() const { return lastRenderCount_; }
    
private:
    // Vertex data for GPU (interleaved format)
    struct UIVertex {
        float x, y;          // Position (screen space)
        float r, g, b, a;    // Color
    };
    
    // OpenGL resources
    GLuint vao_ = 0;           // Vertex Array Object
    GLuint vbo_ = 0;           // Vertex Buffer Object
    GLuint ibo_ = 0;           // Index Buffer Object (for quads)
    size_t vboCapacity_ = 0;   // Current VBO capacity (in vertices)
    size_t iboCapacity_ = 0;   // Current IBO capacity (in indices)
    
    // Batch state
    std::vector<UIVertex> vertices_;
    std::vector<GLuint> indices_;
    int quadCount_ = 0;
    int lastRenderCount_ = 0;
    
    // Screen dimensions for orthographic projection
    int screenWidth_ = 0;
    int screenHeight_ = 0;
    
    // Helper: Resize buffers if needed
    void EnsureCapacity(size_t requiredVertices, size_t requiredIndices);
};
