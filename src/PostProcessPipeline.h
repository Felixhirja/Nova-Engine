#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef USE_GLFW
#include <glad/glad.h>
#else
#include <cstdint>
using GLuint = unsigned int;
using GLenum = unsigned int;
using GLsizei = int;
using GLboolean = unsigned char;
#endif

#if defined(USE_GLFW) && !defined(_WIN32)
// For Linux/Unix
#include <GL/glx.h>
#endif

/**
 * PostProcessPipeline manages framebuffer-based post-processing effects.
 * 
 * Features:
 * - Render-to-texture via framebuffer objects (FBO)
 * - Letterbox overlay for cinematic HUD presentation
 * - Simple bloom effect toggle
 * - Modular pass system for future effects
 */
class PostProcessPipeline {
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    /**
     * Initialize the pipeline with the given viewport dimensions.
     * Creates framebuffer objects and required textures.
     */
    bool Init(int width, int height);

    /**
     * Resize internal buffers when viewport changes.
     */
    void Resize(int width, int height);

    /**
     * Begin rendering to the offscreen framebuffer.
     * Call this before your main scene render.
     */
    void BeginScene();

    /**
     * End scene rendering and apply post-process effects.
     * This renders the final composited image to the screen.
     */
    void EndScene();

    /**
     * Enable/disable bloom effect.
     */
    void SetBloomEnabled(bool enabled) { bloomEnabled_ = enabled; }
    bool IsBloomEnabled() const { return bloomEnabled_; }

    /**
     * Enable/disable letterbox overlay.
     */
    void SetLetterboxEnabled(bool enabled) { letterboxEnabled_ = enabled; }
    bool IsLetterboxEnabled() const { return letterboxEnabled_; }

    /**
     * Set letterbox bar height as a fraction of screen height (e.g., 0.1 = 10%).
     */
    void SetLetterboxHeight(float height) { letterboxHeight_ = height; }

    /**
     * Set bloom intensity (0.0 = none, 1.0 = full).
     */
    void SetBloomIntensity(float intensity) { bloomIntensity_ = intensity; }

    /**
     * Set bloom threshold (brightness level for bloom trigger).
     */
    void SetBloomThreshold(float threshold) { bloomThreshold_ = threshold; }

    /**
     * Cleanup resources.
     */
    void Shutdown();

    /**
     * Check if the pipeline is initialized and ready.
     */
    bool IsInitialized() const { return initialized_; }

private:
    bool initialized_;
    int width_;
    int height_;

    // Framebuffer objects
    GLuint sceneFBO_;        // Main scene render target
    GLuint sceneTexture_;    // Color texture for scene
    GLuint sceneDepthRBO_;   // Depth renderbuffer for scene

    GLuint brightFBO_;       // Bright pass extraction
    GLuint brightTexture_;   // Bright areas only

    GLuint blurFBO_[2];      // Ping-pong blur buffers
    GLuint blurTexture_[2];  // Blur textures

    // Post-process settings
    bool bloomEnabled_;
    bool letterboxEnabled_;
    float letterboxHeight_;
    float bloomIntensity_;
    float bloomThreshold_;

    // Helper methods
    bool CreateFramebuffer(GLuint* fbo, GLuint* texture, GLuint* depthRBO, int width, int height, bool withDepth);
    void DeleteFramebuffer(GLuint* fbo, GLuint* texture, GLuint* depthRBO);
    
    void RenderQuad();  // Render fullscreen quad for post-processing
    void ApplyBrightPass();  // Extract bright areas
    void ApplyBlur(int passes);  // Gaussian blur
    void CompositeToScreen();  // Final composite
    void DrawLetterbox();  // Draw letterbox bars
};
