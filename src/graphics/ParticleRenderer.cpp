#include "ParticleRenderer.h"
#include "../VisualFeedbackSystem.h"  // For Particle struct
#include "../Camera.h"
#include <cmath>
#include <algorithm>

ParticleRenderer::ParticleRenderer() {
    // Constructor - resources initialized in Init()
}

ParticleRenderer::~ParticleRenderer() {
    Cleanup();
}

bool ParticleRenderer::Init() {
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
    
    // Bind VAO
    glBindVertexArray(vao_);
    
    // Bind and setup VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Define vertex format (interleaved)
    // Attribute 0: Position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), 
                         (void*)offsetof(ParticleVertex, x));
    
    // Attribute 1: Color (vec4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                         (void*)offsetof(ParticleVertex, r));
    
    // Attribute 2: Size (float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                         (void*)offsetof(ParticleVertex, size));
    
    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Initial capacity: 1000 particles
    vboCapacity_ = 1000;
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vboCapacity_ * sizeof(ParticleVertex), 
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    return true;
}

void ParticleRenderer::Cleanup() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    
    vboCapacity_ = 0;
    lastRenderCount_ = 0;
}

void ParticleRenderer::EnsureCapacity(size_t requiredVertices) {
    if (requiredVertices <= vboCapacity_) {
        return;  // Sufficient capacity
    }
    
    // Grow by 1.5x or to required size, whichever is larger
    size_t newCapacity = std::max(requiredVertices, vboCapacity_ + vboCapacity_ / 2);
    
    // Reallocate VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, newCapacity * sizeof(ParticleVertex),
                nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    vboCapacity_ = newCapacity;
}

void ParticleRenderer::BuildVertexData(const std::vector<Particle>& particles,
                                       const Camera* camera,
                                       std::vector<ParticleVertex>& outVertices) {
    outVertices.clear();
    outVertices.reserve(particles.size());
    
    for (const auto& p : particles) {
        if (!p.IsAlive()) {
            continue;  // Skip dead particles
        }
        
        ParticleVertex v;
        v.x = static_cast<float>(p.x);
        v.y = static_cast<float>(p.y);
        v.z = static_cast<float>(p.z);
        v.r = p.r;
        v.g = p.g;
        v.b = p.b;
        v.a = p.a;
        
        // Calculate size based on distance to camera (perspective sizing)
        if (camera) {
            double dx = p.x - camera->x();
            double dy = p.y - camera->y();
            double dz = p.z - camera->z();
            double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            // Size based on distance (closer = bigger on screen)
            float screenSize = p.size * 10.0f / static_cast<float>(dist + 1.0);
            v.size = std::max(1.0f, std::min(20.0f, screenSize));
        } else {
            v.size = p.size * 10.0f;
        }
        
        outVertices.push_back(v);
    }
}

void ParticleRenderer::Render(const std::vector<Particle>& particles, const Camera* camera) {
    if (particles.empty() || vao_ == 0 || vbo_ == 0) {
        lastRenderCount_ = 0;
        return;
    }
    
    // Build vertex data
    std::vector<ParticleVertex> vertices;
    BuildVertexData(particles, camera, vertices);
    
    if (vertices.empty()) {
        lastRenderCount_ = 0;
        return;
    }
    
    // Ensure VBO has enough capacity
    EnsureCapacity(vertices.size());
    
    // Upload vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   vertices.size() * sizeof(ParticleVertex),
                   vertices.data());
    
    // Setup render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow
    glDepthMask(GL_FALSE);  // Don't write to depth buffer
    
    // Enable point sprites for modern OpenGL
    glEnable(GL_PROGRAM_POINT_SIZE);  // Let shader control point size
    
    // Bind VAO and draw
    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size()));
    
    // Restore state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Default blending
    
    lastRenderCount_ = static_cast<int>(vertices.size());
}
