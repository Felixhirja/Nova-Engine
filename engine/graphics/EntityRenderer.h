#pragma once

#include "../ecs/EntityManager.h"
#include "../ecs/Components.h"
#include "ShaderProgram.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>

// Forward declarations
class Camera;

/**
 * EntityRenderer: System for rendering entities with DrawComponents
 * Processes all entities with DrawComponent and renders them according to their render mode
 */
class EntityRenderer {
public:
    EntityRenderer();
    ~EntityRenderer();

    // Initialize the renderer (load shaders, etc.)
    bool Initialize();

    // Render all drawable actors (V2 API)
    void Render(ecs::EntityManagerV2& entityManager, const Camera* camera = nullptr);
    
    // Render all drawable actors (Legacy API)
    void RenderLegacy(EntityManager& entityManager, const Camera* camera = nullptr);

    // Asset loading methods (on-demand)
    bool LoadMesh(int meshHandle, const std::string& filepath);
    bool LoadTexture(int textureHandle, const std::string& filepath);
    void UnloadMesh(int meshHandle);
    void UnloadTexture(int textureHandle);

    // Cleanup resources
    void Cleanup();

private:
    // Rendering methods for different modes
    void RenderSprite2D(const DrawComponent& draw, const Position& position);
    void RenderBillboard(const DrawComponent& draw, const Position& position, const Camera* camera);
    void RenderMesh3D(const DrawComponent& draw, const Position& position);
    void RenderParticles(const DrawComponent& draw, const Position& position);
    void RenderWireframe(const DrawComponent& draw, const Position& position);
    void RenderDebugInfo(const DrawComponent& draw, const Position& position);

    // Shader programs
    std::unique_ptr<ShaderProgram> spriteShader_;
    std::unique_ptr<ShaderProgram> billboardShader_;
    std::unique_ptr<ShaderProgram> meshShader_;

    // VAO/VBO for simple geometry
    unsigned int quadVAO_ = 0;
    unsigned int quadVBO_ = 0;
    unsigned int quadEBO_ = 0;

    // Asset storage
    std::unordered_map<int, unsigned int> meshVAOs_;      // meshHandle -> VAO
    std::unordered_map<int, unsigned int> textureIds_;    // textureHandle -> GL texture ID
    std::unordered_map<int, int> meshVertexCounts_;       // meshHandle -> vertex count

    bool initialized_ = false;
};

// Inline implementations

inline EntityRenderer::EntityRenderer() = default;

inline EntityRenderer::~EntityRenderer() {
    Cleanup();
}

inline bool EntityRenderer::Initialize() {
    if (initialized_) return true;

    // TODO: Load shaders and create VAOs/VBOs
    // For now, just mark as initialized
    initialized_ = true;
    return true;
}

inline void EntityRenderer::Render(ecs::EntityManagerV2& entityManager, const Camera* camera) {
    if (!initialized_) return;

    static int debugCounter = 0;
    if (++debugCounter % 120 == 0) {
        std::cout << "EntityRenderer::Render called" << std::endl;
    }

    // Iterate through all entities with DrawComponent
    int entityCount = 0;
    entityManager.ForEach<DrawComponent, Position>(
        [this, camera, &entityCount](ecs::EntityHandle entity, DrawComponent& draw, Position& position) {
            (void)entity;
            entityCount++;
            
            if (!draw.visible) return;

            // Update animation if needed
            if (draw.animated) {
                draw.UpdateAnimation(1.0f / 60.0f); // Assume 60 FPS
            }

            // Render based on mode
            switch (draw.mode) {
                case DrawComponent::RenderMode::Sprite2D:
                    RenderSprite2D(draw, position);
                    break;
                case DrawComponent::RenderMode::Billboard:
                    RenderBillboard(draw, position, camera);
                    break;
                case DrawComponent::RenderMode::Mesh3D:
                    RenderMesh3D(draw, position);
                    break;
                case DrawComponent::RenderMode::Particles:
                    RenderParticles(draw, position);
                    break;
                case DrawComponent::RenderMode::Wireframe:
                    RenderWireframe(draw, position);
                    break;
                case DrawComponent::RenderMode::Custom:
                    if (draw.customRenderCallback) {
                        draw.customRenderCallback(draw, position);
                    }
                    break;
                case DrawComponent::RenderMode::None:
                default:
                    break;
            }

            // Render debug info if requested
            if (draw.showBoundingBox || draw.showCollisionShape) {
                RenderDebugInfo(draw, position);
            }
        }
    );
    
    static int debugCounter2 = 0;
    if (++debugCounter2 % 120 == 0) {
        std::cout << "EntityRenderer: Found " << entityCount << " entities with DrawComponent+Position" << std::endl;
    }
}

inline void EntityRenderer::RenderLegacy(EntityManager& entityManager, const Camera* camera) {
    if (!initialized_) return;

    // Iterate through all entities with DrawComponent
    entityManager.ForEach<DrawComponent, Position>(
        [this, camera](Entity entity, DrawComponent& draw, Position& position) {
            (void)entity;
            
            if (!draw.visible) return;

            // Update animation if needed
            if (draw.animated) {
                draw.UpdateAnimation(1.0f / 60.0f); // Assume 60 FPS
            }

            // Render based on mode
            switch (draw.mode) {
                case DrawComponent::RenderMode::Sprite2D:
                    RenderSprite2D(draw, position);
                    break;
                case DrawComponent::RenderMode::Billboard:
                    RenderBillboard(draw, position, camera);
                    break;
                case DrawComponent::RenderMode::Mesh3D:
                    RenderMesh3D(draw, position);
                    break;
                case DrawComponent::RenderMode::Particles:
                    RenderParticles(draw, position);
                    break;
                case DrawComponent::RenderMode::Wireframe:
                    RenderWireframe(draw, position);
                    break;
                case DrawComponent::RenderMode::Custom:
                    if (draw.customRenderCallback) {
                        draw.customRenderCallback(draw, position);
                    }
                    break;
                case DrawComponent::RenderMode::None:
                default:
                    break;
            }

            // Render debug info if requested
            if (draw.showBoundingBox || draw.showCollisionShape) {
                RenderDebugInfo(draw, position);
            }
        }
    );
}

inline void EntityRenderer::RenderSprite2D(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement 2D sprite rendering
    // This would use the spriteShader_ and render a quad with the texture
    // Position would be projected to screen space
}

inline void EntityRenderer::RenderBillboard(const DrawComponent& draw, const Position& position, const Camera* camera) {
    (void)draw;
    (void)position;
    (void)camera;
    // TODO: Implement billboard rendering
    // Billboards always face the camera
    // Useful for projectiles, particles, etc.
}

inline void EntityRenderer::RenderMesh3D(const DrawComponent& draw, const Position& position) {
    // Ensure proper OpenGL state for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    
    // Fallback: Render a simple colored cube
    glPushMatrix();
    glTranslatef(static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z));
    
    float scale = draw.meshScale;
    glScalef(scale, scale, scale);
    
    // Apply tint color
    glColor3f(draw.tintR, draw.tintG, draw.tintB);
    
    // Draw a simple cube using immediate mode (fallback when no mesh loaded)
    glBegin(GL_QUADS);
    
    // Front face
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    
    // Back face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    
    // Top face
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    
    // Bottom face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    
    // Right face
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    
    // Left face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    
    glEnd();
    
    glPopMatrix();
    
    // Reset color
    glColor3f(1.0f, 1.0f, 1.0f);
}

inline void EntityRenderer::RenderParticles(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement particle system rendering
    // For engine trails, explosions, etc.
}

inline void EntityRenderer::RenderWireframe(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement wireframe debug rendering
    // Useful for debugging collision shapes, bounding boxes, etc.
}

inline void EntityRenderer::RenderDebugInfo(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Render bounding boxes, collision shapes, etc.
    // This would use immediate mode or debug lines
}

inline void EntityRenderer::Cleanup() {
    // Clean up loaded meshes
    for (auto& [handle, vao] : meshVAOs_) {
        glDeleteVertexArrays(1, &vao);
    }
    meshVAOs_.clear();
    meshVertexCounts_.clear();

    // Clean up loaded textures
    for (auto& [handle, texId] : textureIds_) {
        glDeleteTextures(1, &texId);
    }
    textureIds_.clear();

    // TODO: Clean up shaders, VAOs, VBOs
    initialized_ = false;
}

inline bool EntityRenderer::LoadMesh(int meshHandle, const std::string& filepath) {
    // Check if already loaded
    if (meshVAOs_.find(meshHandle) != meshVAOs_.end()) {
        return true; // Already loaded
    }

    // TODO: Implement mesh loading from file
    // 1. Parse mesh file (OBJ, GLTF, etc.)
    // 2. Create VAO/VBO/EBO
    // 3. Upload vertex data to GPU
    // 4. Store VAO and vertex count
    
    (void)filepath;
    std::cout << "EntityRenderer::LoadMesh: Loading mesh " << meshHandle << " from " << filepath << " (not implemented)\n";
    return false;
}

inline bool EntityRenderer::LoadTexture(int textureHandle, const std::string& filepath) {
    // Check if already loaded
    if (textureIds_.find(textureHandle) != textureIds_.end()) {
        return true; // Already loaded
    }

    // TODO: Implement texture loading from file
    // 1. Load image file (PNG, JPG, etc.)
    // 2. Create OpenGL texture
    // 3. Upload image data to GPU
    // 4. Set texture parameters
    
    (void)filepath;
    std::cout << "EntityRenderer::LoadTexture: Loading texture " << textureHandle << " from " << filepath << " (not implemented)\n";
    return false;
}

inline void EntityRenderer::UnloadMesh(int meshHandle) {
    auto it = meshVAOs_.find(meshHandle);
    if (it != meshVAOs_.end()) {
        glDeleteVertexArrays(1, &it->second);
        meshVAOs_.erase(it);
        meshVertexCounts_.erase(meshHandle);
    }
}

inline void EntityRenderer::UnloadTexture(int textureHandle) {
    auto it = textureIds_.find(textureHandle);
    if (it != textureIds_.end()) {
        glDeleteTextures(1, &it->second);
        textureIds_.erase(it);
    }
}
