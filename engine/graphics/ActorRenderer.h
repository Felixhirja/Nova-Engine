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

    // Render all drawable actors
    void Render(ecs::EntityManagerV2& entityManager, const Camera* camera = nullptr);

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

    // Iterate through all entities with DrawComponent
    entityManager.ForEach<DrawComponent, Position>(
        [this, camera](ecs::EntityHandle entity, DrawComponent& draw, Position& position) {
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

inline void ActorRenderer::RenderSprite2D(const DrawComponent& draw, const Position& position) {
    // TODO: Implement 2D sprite rendering
    // This would use the spriteShader_ and render a quad with the texture
    // Position would be projected to screen space
}

inline void ActorRenderer::RenderBillboard(const DrawComponent& draw, const Position& position, const Camera* camera) {
    // TODO: Implement billboard rendering
    // Billboards always face the camera
    // Useful for projectiles, particles, etc.
}

inline void ActorRenderer::RenderMesh3D(const DrawComponent& draw, const Position& position) {
    // TODO: Implement 3D mesh rendering
    // This would render the actual 3D model of spaceships, stations, etc.
}

inline void ActorRenderer::RenderParticles(const DrawComponent& draw, const Position& position) {
    // TODO: Implement particle system rendering
    // For engine trails, explosions, etc.
}

inline void ActorRenderer::RenderWireframe(const DrawComponent& draw, const Position& position) {
    // TODO: Implement wireframe debug rendering
    // Useful for debugging collision shapes, bounding boxes, etc.
}

inline void ActorRenderer::RenderDebugInfo(const DrawComponent& draw, const Position& position) {
    // TODO: Render bounding boxes, collision shapes, etc.
    // This would use immediate mode or debug lines
}

inline void ActorRenderer::Cleanup() {
    // TODO: Clean up shaders, VAOs, VBOs
    initialized_ = false;
}