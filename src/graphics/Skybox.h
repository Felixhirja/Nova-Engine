#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <glad/glad.h>
#include "ShaderProgram.h"
#include <memory>
#include <string>

/**
 * Skybox - Renders an immersive 360° environment using a cubemap.
 * 
 * Features:
 * - Cubemap loading from 6 texture files
 * - Procedural starfield generation
 * - Optimized rendering (always at infinite distance)
 * - HDR support (future)
 * 
 * Usage:
 *   Skybox skybox;
 *   skybox.LoadProceduralStarfield();  // Or LoadCubemap(...)
 *   
 *   // In render loop:
 *   skybox.Render(viewMatrix, projectionMatrix);
 */
class Skybox {
public:
    Skybox();
    ~Skybox();

    /**
     * Load a cubemap from 6 image files.
     * Face order: +X, -X, +Y, -Y, +Z, -Z (right, left, top, bottom, front, back)
     * 
     * @param faces Array of 6 file paths
     * @return true on success
     */
    bool LoadCubemap(const std::string faces[6]);

    /**
     * Generate a procedural starfield skybox (no textures required).
     * Creates a realistic star-filled space environment.
     * 
     * @param starDensity How many stars (0.001 = sparse, 0.005 = dense)
     * @param starBrightness Overall brightness multiplier
     * @return true on success
     */
    bool LoadProceduralStarfield(float starDensity = 0.002f, float starBrightness = 1.0f);

    /**
     * Render the skybox.
     * Call this FIRST in your render loop (before any other geometry).
     * The skybox will automatically render behind everything else.
     * 
     * @param viewMatrix Camera view matrix
     * @param projectionMatrix Camera projection matrix
     */
    void Render(const float* viewMatrix, const float* projectionMatrix);

    /**
     * Clean up resources.
     */
    void Cleanup();

    /**
     * Check if skybox is loaded and ready.
     */
    bool IsLoaded() const { return cubemapTexture_ != 0 || useProceduralStarfield_; }

    /**
     * Set time for animated effects (e.g., star twinkling).
     * Call once per frame with elapsed time in seconds.
     */
    void SetTime(float time) { time_ = time; }

    /**
     * Set star parameters for procedural starfield.
     */
    void SetStarDensity(float density) { starDensity_ = density; }
    void SetStarBrightness(float brightness) { starBrightness_ = brightness; }

private:
    GLuint cubemapTexture_;
    GLuint cubeVAO_;
    GLuint cubeVBO_;

    std::unique_ptr<ShaderProgram> shader_;
    bool useProceduralStarfield_;

    // Procedural starfield parameters
    float starDensity_;
    float starBrightness_;
    float time_;

    /**
     * Initialize the cube mesh for skybox rendering.
     */
    void InitCubeMesh();

    /**
     * Load a single texture from file (helper for cubemap).
     */
    bool LoadTextureFromFile(const std::string& path, GLenum target);
};
