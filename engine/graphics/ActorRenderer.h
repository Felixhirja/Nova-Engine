#pragma once

#include "../ecs/EntityManager.h"
#include "../ecs/Components.h"
#include "ShaderProgram.h"
#include <memory>

// Forward declarations
class Camera;

/**
 * ActorRenderer: System for rendering actors with DrawComponents
 * Processes all entities with DrawComponent and renders them according to their render mode
 */
class ActorRenderer {
public:
    ActorRenderer();
    ~ActorRenderer();

    // Initialize the renderer (load shaders, etc.)
    bool Initialize();

    // Render all drawable actors (V2 API)
    void Render(ecs::EntityManagerV2& entityManager, const Camera* camera = nullptr);
    
    // Render all drawable actors (Legacy API)
    void RenderLegacy(EntityManager& entityManager, const Camera* camera = nullptr);

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

    bool initialized_ = false;
};

// Inline implementations

inline ActorRenderer::ActorRenderer() = default;

inline ActorRenderer::~ActorRenderer() {
    Cleanup();
}

inline bool ActorRenderer::Initialize() {
    if (initialized_) return true;

    // TODO: Load shaders and create VAOs/VBOs
    // For now, just mark as initialized
    initialized_ = true;
    return true;
}

inline void ActorRenderer::Render(ecs::EntityManagerV2& entityManager, const Camera* camera) {
    if (!initialized_) return;

    static int debugCounter = 0;
    if (++debugCounter % 120 == 0) {
        std::cout << "ActorRenderer::Render called" << std::endl;
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
        std::cout << "ActorRenderer: Found " << entityCount << " entities with DrawComponent+Position" << std::endl;
    }
}

inline void ActorRenderer::RenderLegacy(EntityManager& entityManager, const Camera* camera) {
    if (!initialized_) return;

    static int debugCounter = 0;
    if (++debugCounter % 120 == 0) {
        std::cout << "ActorRenderer::RenderLegacy called" << std::endl;
    }

    // Iterate through all entities with DrawComponent
    int entityCount = 0;
    entityManager.ForEach<DrawComponent, Position>(
        [this, camera, &entityCount](Entity entity, DrawComponent& draw, Position& position) {
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
        std::cout << "ActorRenderer::RenderLegacy: Found " << entityCount << " entities with DrawComponent+Position" << std::endl;
    }
}

inline void ActorRenderer::RenderSprite2D(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement 2D sprite rendering
    // This would use the spriteShader_ and render a quad with the texture
    // Position would be projected to screen space
}

inline void ActorRenderer::RenderBillboard(const DrawComponent& draw, const Position& position, const Camera* camera) {
    (void)draw;
    (void)position;
    (void)camera;
    // TODO: Implement billboard rendering
    // Billboards always face the camera
    // Useful for projectiles, particles, etc.
}

inline void ActorRenderer::RenderMesh3D(const DrawComponent& draw, const Position& position) {
    static int debugCounter = 0;
    if (++debugCounter % 120 == 0) {
        std::cout << "RenderMesh3D called at pos (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    }
    
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

inline void ActorRenderer::RenderParticles(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement particle system rendering
    // For engine trails, explosions, etc.
}

inline void ActorRenderer::RenderWireframe(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Implement wireframe debug rendering
    // Useful for debugging collision shapes, bounding boxes, etc.
}

inline void ActorRenderer::RenderDebugInfo(const DrawComponent& draw, const Position& position) {
    (void)draw;
    (void)position;
    // TODO: Render bounding boxes, collision shapes, etc.
    // This would use immediate mode or debug lines
}

inline void ActorRenderer::Cleanup() {
    // TODO: Clean up shaders, VAOs, VBOs
    initialized_ = false;
}