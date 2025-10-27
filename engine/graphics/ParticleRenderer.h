#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>

// Forward declarations
struct Particle;
class Camera;
class ShaderProgram;

// Modern GPU-based particle renderer using VAO/VBO
// Replaces immediate mode glBegin/glEnd with efficient batched rendering
class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();
    
    // Delete copy constructor/assignment (OpenGL resources not copyable)
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;
    
    // Initialize OpenGL resources (VAO, VBO)
    bool Init();
    
    // Render particles with given camera
    // particles: Vector of particle data from VisualFeedbackSystem
    // camera: Camera for projection matrix and distance-based sizing
    void Render(const std::vector<Particle>& particles, const Camera* camera);
    
    // Cleanup OpenGL resources
    void Cleanup();
    
    // Get statistics
    int GetLastRenderCount() const { return lastRenderCount_; }
    
private:
    // Vertex data for GPU (interleaved format)
    struct ParticleVertex {
        float x, y, z;       // Position
        float r, g, b, a;    // Color
        float size;          // Point size
    };
    
    // OpenGL resources
    GLuint vao_ = 0;           // Vertex Array Object
    GLuint vbo_ = 0;           // Vertex Buffer Object
    size_t vboCapacity_ = 0;   // Current VBO capacity (in vertices)
    
    // Shader program for rendering particles
    std::unique_ptr<ShaderProgram> shader_;
    
    // Cached matrices
    float viewMatrix_[16];
    float projectionMatrix_[16];

    // Statistics
    int lastRenderCount_ = 0;
    
    // Helper: Resize VBO if needed
    void EnsureCapacity(size_t requiredVertices);
    
    // Helper: Build vertex data from particles
    void BuildVertexData(const std::vector<Particle>& particles, 
                        const Camera* camera,
                        std::vector<ParticleVertex>& outVertices);
};
