#ifdef USE_GLFW
#include "PostProcessPipeline.h"
#include <iostream>
#include <cmath>

// OpenGL FBO extension definitions for legacy OpenGL
// These should be available on any reasonably modern system
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_COLOR_ATTACHMENT0_EXT               0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT                0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT            0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_CLAMP_TO_EDGE                       0x812F
#define GL_DEPTH_COMPONENT24                   0x81A6
#endif

// Function pointers for FBO extensions (loaded dynamically)
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = nullptr;
static PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = nullptr;
static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = nullptr;
static PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = nullptr;
static PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = nullptr;
static PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = nullptr;
static PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = nullptr;

// Helper function to load GL extensions
static bool LoadFBOExtensions() {
    static bool loaded = false;
    static bool available = false;
    
    if (loaded) return available;
    loaded = true;
    
    // Try to load FBO extension functions
#ifdef _WIN32
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
    glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
    glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
#else
    // For Linux/Unix, use glXGetProcAddress
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)glXGetProcAddress((const GLubyte*)"glGenFramebuffersEXT");
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)glXGetProcAddress((const GLubyte*)"glDeleteFramebuffersEXT");
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)glXGetProcAddress((const GLubyte*)"glBindFramebufferEXT");
    glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)glXGetProcAddress((const GLubyte*)"glFramebufferTexture2DEXT");
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatusEXT");
    glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)glXGetProcAddress((const GLubyte*)"glGenRenderbuffersEXT");
    glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)glXGetProcAddress((const GLubyte*)"glDeleteRenderbuffersEXT");
    glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)glXGetProcAddress((const GLubyte*)"glBindRenderbufferEXT");
    glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)glXGetProcAddress((const GLubyte*)"glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)glXGetProcAddress((const GLubyte*)"glFramebufferRenderbufferEXT");
#endif
    
    available = (glGenFramebuffersEXT != nullptr &&
                 glDeleteFramebuffersEXT != nullptr &&
                 glBindFramebufferEXT != nullptr &&
                 glFramebufferTexture2DEXT != nullptr &&
                 glCheckFramebufferStatusEXT != nullptr &&
                 glGenRenderbuffersEXT != nullptr &&
                 glDeleteRenderbuffersEXT != nullptr &&
                 glBindRenderbufferEXT != nullptr &&
                 glRenderbufferStorageEXT != nullptr &&
                 glFramebufferRenderbufferEXT != nullptr);
    
    if (!available) {
        std::cerr << "FBO extensions not available" << std::endl;
    }
    
    return available;
}

PostProcessPipeline::PostProcessPipeline()
    : initialized_(false)
    , width_(0)
    , height_(0)
    , sceneFBO_(0)
    , sceneTexture_(0)
    , sceneDepthRBO_(0)
    , brightFBO_(0)
    , brightTexture_(0)
    , bloomEnabled_(false)
    , letterboxEnabled_(true)
    , letterboxHeight_(0.1f)
    , bloomIntensity_(0.8f)
    , bloomThreshold_(0.7f)
{
    blurFBO_[0] = blurFBO_[1] = 0;
    blurTexture_[0] = blurTexture_[1] = 0;
}

PostProcessPipeline::~PostProcessPipeline() {
    Shutdown();
}

bool PostProcessPipeline::Init(int width, int height) {
    if (initialized_) {
        Shutdown();
    }
    
    // Load FBO extensions first
    if (!LoadFBOExtensions()) {
        std::cerr << "FBO extensions not supported, post-processing disabled" << std::endl;
        return false;
    }

    width_ = width;
    height_ = height;

    // Check for framebuffer object support (should be core in OpenGL 3.0+)
    // For legacy OpenGL 1.x/2.x, we'd need to check for GL_EXT_framebuffer_object
    // For now, we'll attempt creation and handle failure gracefully

    // Create main scene framebuffer with depth
    if (!CreateFramebuffer(&sceneFBO_, &sceneTexture_, &sceneDepthRBO_, width, height, true)) {
        std::cerr << "Failed to create scene framebuffer" << std::endl;
        return false;
    }

    // Create bright pass framebuffer (no depth needed)
    if (!CreateFramebuffer(&brightFBO_, &brightTexture_, nullptr, width / 2, height / 2, false)) {
        std::cerr << "Failed to create bright pass framebuffer" << std::endl;
        Shutdown();
        return false;
    }

    // Create blur ping-pong framebuffers (no depth needed)
    if (!CreateFramebuffer(&blurFBO_[0], &blurTexture_[0], nullptr, width / 4, height / 4, false)) {
        std::cerr << "Failed to create blur framebuffer 0" << std::endl;
        Shutdown();
        return false;
    }

    if (!CreateFramebuffer(&blurFBO_[1], &blurTexture_[1], nullptr, width / 4, height / 4, false)) {
        std::cerr << "Failed to create blur framebuffer 1" << std::endl;
        Shutdown();
        return false;
    }

    initialized_ = true;
    std::cout << "PostProcessPipeline initialized (" << width << "x" << height << ")" << std::endl;
    return true;
}

void PostProcessPipeline::Resize(int width, int height) {
    if (!initialized_) return;
    
    width_ = width;
    height_ = height;

    // Recreate framebuffers with new dimensions
    DeleteFramebuffer(&sceneFBO_, &sceneTexture_, &sceneDepthRBO_);
    DeleteFramebuffer(&brightFBO_, &brightTexture_, nullptr);
    DeleteFramebuffer(&blurFBO_[0], &blurTexture_[0], nullptr);
    DeleteFramebuffer(&blurFBO_[1], &blurTexture_[1], nullptr);

    CreateFramebuffer(&sceneFBO_, &sceneTexture_, &sceneDepthRBO_, width, height, true);
    CreateFramebuffer(&brightFBO_, &brightTexture_, nullptr, width / 2, height / 2, false);
    CreateFramebuffer(&blurFBO_[0], &blurTexture_[0], nullptr, width / 4, height / 4, false);
    CreateFramebuffer(&blurFBO_[1], &blurTexture_[1], nullptr, width / 4, height / 4, false);
}

void PostProcessPipeline::BeginScene() {
    if (!initialized_) return;

    // Bind scene framebuffer for rendering
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sceneFBO_);
    glViewport(0, 0, width_, height_);
    
    // Clear the framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessPipeline::EndScene() {
    if (!initialized_) return;

    // Unbind framebuffer (render to screen)
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, width_, height_);

    // Apply post-process effects
    if (bloomEnabled_) {
        ApplyBrightPass();
        ApplyBlur(2);  // 2 passes of blur
    }

    // Composite final image to screen
    CompositeToScreen();

    // Draw letterbox overlay if enabled
    if (letterboxEnabled_) {
        DrawLetterbox();
    }
}

void PostProcessPipeline::Shutdown() {
    if (!initialized_) return;

    DeleteFramebuffer(&sceneFBO_, &sceneTexture_, &sceneDepthRBO_);
    DeleteFramebuffer(&brightFBO_, &brightTexture_, nullptr);
    DeleteFramebuffer(&blurFBO_[0], &blurTexture_[0], nullptr);
    DeleteFramebuffer(&blurFBO_[1], &blurTexture_[1], nullptr);

    initialized_ = false;
}

bool PostProcessPipeline::CreateFramebuffer(GLuint* fbo, GLuint* texture, GLuint* depthRBO, int width, int height, bool withDepth) {
    // Generate framebuffer
    glGenFramebuffersEXT(1, fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *fbo);

    // Create color texture
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Attach color texture
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, *texture, 0);

    // Create depth renderbuffer if requested
    if (withDepth && depthRBO) {
        glGenRenderbuffersEXT(1, depthRBO);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, *depthRBO);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, *depthRBO);
    }

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        std::cerr << "Framebuffer incomplete, status: 0x" << std::hex << status << std::dec << std::endl;
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        return false;
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return true;
}

void PostProcessPipeline::DeleteFramebuffer(GLuint* fbo, GLuint* texture, GLuint* depthRBO) {
    if (fbo && *fbo) {
        glDeleteFramebuffersEXT(1, fbo);
        *fbo = 0;
    }
    if (texture && *texture) {
        glDeleteTextures(1, texture);
        *texture = 0;
    }
    if (depthRBO && *depthRBO) {
        glDeleteRenderbuffersEXT(1, depthRBO);
        *depthRBO = 0;
    }
}

void PostProcessPipeline::RenderQuad() {
    // Render a fullscreen quad using immediate mode (compatible with legacy OpenGL)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
}

void PostProcessPipeline::ApplyBrightPass() {
    // Extract bright areas from scene texture
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, brightFBO_);
    glViewport(0, 0, width_ / 2, height_ / 2);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up orthographic projection for fullscreen quad
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Bind scene texture
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);

    // Simple bright pass: sample texture and threshold
    // In a shader-based pipeline, we'd do: color * step(threshold, luminance)
    // Here we'll just render the texture and rely on additive blending later
    glColor4f(bloomIntensity_, bloomIntensity_, bloomIntensity_, 1.0f);
    RenderQuad();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

#else // !USE_GLFW

#include "PostProcessPipeline.h"

PostProcessPipeline::PostProcessPipeline()
    : initialized_(false)
    , width_(0)
    , height_(0)
    , sceneFBO_(0)
    , sceneTexture_(0)
    , sceneDepthRBO_(0)
    , brightFBO_(0)
    , brightTexture_(0)
    , bloomEnabled_(false)
    , letterboxEnabled_(false)
    , letterboxHeight_(0.1f)
    , bloomIntensity_(0.0f)
    , bloomThreshold_(0.0f) {
    blurFBO_[0] = blurFBO_[1] = 0;
    blurTexture_[0] = blurTexture_[1] = 0;
}

PostProcessPipeline::~PostProcessPipeline() = default;

bool PostProcessPipeline::Init(int width, int height) {
    width_ = width;
    height_ = height;
    initialized_ = false;
    return false;
}

void PostProcessPipeline::Resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void PostProcessPipeline::BeginScene() {}

void PostProcessPipeline::EndScene() {}

void PostProcessPipeline::Shutdown() {
    initialized_ = false;
    sceneFBO_ = sceneTexture_ = sceneDepthRBO_ = 0;
    brightFBO_ = brightTexture_ = 0;
    blurFBO_[0] = blurFBO_[1] = 0;
    blurTexture_[0] = blurTexture_[1] = 0;
}

bool PostProcessPipeline::CreateFramebuffer(GLuint*, GLuint*, GLuint*, int, int, bool) {
    return false;
}

void PostProcessPipeline::DeleteFramebuffer(GLuint*, GLuint*, GLuint*) {}

void PostProcessPipeline::RenderQuad() {}

void PostProcessPipeline::ApplyBrightPass() {}

void PostProcessPipeline::ApplyBlur(int) {}

void PostProcessPipeline::CompositeToScreen() {}

void PostProcessPipeline::DrawLetterbox() {}

#endif // USE_GLFW

void PostProcessPipeline::ApplyBlur(int passes) {
    // Simple box blur using ping-pong rendering
    // In a proper implementation, we'd use separated Gaussian blur with shaders
    
    GLuint srcTexture = brightTexture_;
    
    for (int i = 0; i < passes; ++i) {
        // Horizontal blur pass
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blurFBO_[0]);
        glViewport(0, 0, width_ / 4, height_ / 4);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, srcTexture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        RenderQuad();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        // Vertical blur pass
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blurFBO_[1]);
        glViewport(0, 0, width_ / 4, height_ / 4);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBindTexture(GL_TEXTURE_2D, blurTexture_[0]);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        RenderQuad();

        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        srcTexture = blurTexture_[1];
    }
}

void PostProcessPipeline::CompositeToScreen() {
    // Render final composite to screen
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // Render base scene
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RenderQuad();

    // Additive blend bloom on top if enabled
    if (bloomEnabled_) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // Additive blending
        
        glBindTexture(GL_TEXTURE_2D, blurTexture_[1]);
        glColor4f(bloomIntensity_, bloomIntensity_, bloomIntensity_, 1.0f);
        RenderQuad();
        
        glDisable(GL_BLEND);
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void PostProcessPipeline::DrawLetterbox() {
    // Draw black bars at top and bottom for cinematic letterbox effect
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, width_, height_, 0.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    
    // Enable blending for semi-transparent bars if desired
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float barHeight = height_ * letterboxHeight_;
    
    // Top bar
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)width_, 0.0f);
    glVertex2f((float)width_, barHeight);
    glVertex2f(0.0f, barHeight);
    glEnd();

    // Bottom bar
    glBegin(GL_QUADS);
    glVertex2f(0.0f, height_ - barHeight);
    glVertex2f((float)width_, height_ - barHeight);
    glVertex2f((float)width_, (float)height_);
    glVertex2f(0.0f, (float)height_);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
