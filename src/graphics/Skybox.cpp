#include "Skybox.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

// OpenGL extension definitions
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_TEXTURE0 0x84C0
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#endif

// VBO function pointers
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size, const void *data, GLenum usage);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);

static PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
static PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

// Load VBO extensions
static bool LoadVBOExtensions() {
    static bool loaded = false;
    static bool available = false;
    
    if (loaded) return available;
    loaded = true;
    
#ifdef _WIN32
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
#else
    glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferData");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)glXGetProcAddress((const GLubyte*)"glActiveTexture");
#endif
    
    available = (glGenBuffers != nullptr && glBindBuffer != nullptr && glBufferData != nullptr);
    return available;
}

Skybox::Skybox()
    : cubemapTexture_(0)
    , cubeVAO_(0)
    , cubeVBO_(0)
    , useProceduralStarfield_(false)
    , starDensity_(0.002f)
    , starBrightness_(1.0f)
    , time_(0.0f)
{
}

Skybox::~Skybox() {
    Cleanup();
}

void Skybox::InitCubeMesh() {
    // Load VBO extensions if not already loaded
    if (!LoadVBOExtensions()) {
        std::cerr << "VBO extensions not available - skybox will not work" << std::endl;
        return;
    }
    
    // Skybox cube vertices (positions only, no normals/UVs needed)
    float vertices[] = {
        // Positions        
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // Generate and bind VBO
    glGenBuffers(1, &cubeVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::cout << "Skybox cube mesh initialized" << std::endl;
}

bool Skybox::LoadCubemap(const std::string faces[6]) {
    // Clean up existing resources
    if (cubemapTexture_ != 0) {
        glDeleteTextures(1, &cubemapTexture_);
        cubemapTexture_ = 0;
    }

    // Generate cubemap texture
    glGenTextures(1, &cubemapTexture_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture_);

    // Load each face
    GLenum targets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    bool success = true;
    for (int i = 0; i < 6; i++) {
        if (!LoadTextureFromFile(faces[i], targets[i])) {
            std::cerr << "Failed to load cubemap face: " << faces[i] << std::endl;
            success = false;
            break;
        }
    }

    if (!success) {
        glDeleteTextures(1, &cubemapTexture_);
        cubemapTexture_ = 0;
        return false;
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Initialize cube mesh if not done
    if (cubeVBO_ == 0) {
        InitCubeMesh();
    }

    // Load skybox shader
    shader_ = std::make_unique<ShaderProgram>();
    if (!shader_->LoadFromFiles("shaders/skybox/skybox.vert", "shaders/skybox/skybox.frag")) {
        std::cerr << "Failed to load skybox shader" << std::endl;
        return false;
    }

    useProceduralStarfield_ = false;
    std::cout << "Cubemap skybox loaded successfully" << std::endl;
    return true;
}

bool Skybox::LoadProceduralStarfield(float starDensity, float starBrightness) {
    starDensity_ = starDensity;
    starBrightness_ = starBrightness;

    // Initialize cube mesh if not done
    if (cubeVBO_ == 0) {
        InitCubeMesh();
    }

    // Load procedural starfield shader
    shader_ = std::make_unique<ShaderProgram>();
    if (!shader_->LoadFromFiles("shaders/skybox/skybox.vert", "shaders/skybox/starfield.frag")) {
        std::cerr << "Failed to load starfield shader" << std::endl;
        return false;
    }

    useProceduralStarfield_ = true;
    std::cout << "Procedural starfield skybox initialized" << std::endl;
    return true;
}

void Skybox::Render(const float* viewMatrix, const float* projectionMatrix) {
    if (!IsLoaded() || !shader_ || !shader_->IsValid()) {
        return;
    }

    // Disable depth writing (skybox is always behind everything)
    // But keep depth testing enabled
    glDepthMask(GL_FALSE);

    // Use shader
    shader_->Use();

    // Set uniforms
    shader_->SetUniformMatrix4("view", viewMatrix);
    shader_->SetUniformMatrix4("projection", projectionMatrix);

    if (useProceduralStarfield_) {
        // Set procedural starfield parameters
        shader_->SetUniform("time", time_);
        shader_->SetUniform("starDensity", starDensity_);
        shader_->SetUniform("starBrightness", starBrightness_);
    } else {
        // Bind cubemap texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture_);
        shader_->SetUniformTexture("skybox", 0);
    }

    // Draw cube
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Restore depth writing
    glDepthMask(GL_TRUE);

    // Unbind shader
    ShaderProgram::Unuse();
}

void Skybox::Cleanup() {
    if (cubemapTexture_ != 0) {
        glDeleteTextures(1, &cubemapTexture_);
        cubemapTexture_ = 0;
    }

    if (cubeVBO_ != 0) {
        glDeleteBuffers(1, &cubeVBO_);
        cubeVBO_ = 0;
    }

    shader_.reset();
}

bool Skybox::LoadTextureFromFile(const std::string& path, GLenum target) {
    // NOTE: This is a placeholder. In a real implementation, you would:
    // 1. Load image data using a library like stb_image, SOIL, or similar
    // 2. Upload to GPU using glTexImage2D
    
    // For now, we'll create a placeholder colored texture
    std::cerr << "LoadTextureFromFile not fully implemented (requires image loader)" << std::endl;
    std::cerr << "Would load: " << path << std::endl;
    
    // Create a simple colored texture as fallback
    unsigned char color[3];
    if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) { color[0]=255; color[1]=0;   color[2]=0;   } // Red
    if (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X) { color[0]=0;   color[1]=255; color[2]=0;   } // Green
    if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y) { color[0]=0;   color[1]=0;   color[2]=255; } // Blue
    if (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) { color[0]=255; color[1]=255; color[2]=0;   } // Yellow
    if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z) { color[0]=255; color[1]=0;   color[2]=255; } // Magenta
    if (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) { color[0]=0;   color[1]=255; color[2]=255; } // Cyan
    
    // Create 1x1 colored texture
    glTexImage2D(target, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color);
    
    return true;
}
