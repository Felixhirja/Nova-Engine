#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <glad/glad.h>
#include <string>
#include <unordered_map>

/**
 * ShaderProgram - Manages GLSL shader compilation, linking, and uniform setting.
 * 
 * Supports:
 * - Vertex and fragment shaders (required)
 * - Geometry shaders (optional, future)
 * - Automatic uniform location caching
 * - Error reporting with line numbers
 * - Hot-reloading support
 * 
 * Example usage:
 *   ShaderProgram shader;
 *   if (shader.LoadFromFiles("basic.vert", "basic.frag")) {
 *       shader.Use();
 *       shader.SetUniform("modelMatrix", modelMatrix);
 *       shader.SetUniform("lightPos", lightPosition);
 *       // Draw geometry...
 *   }
 */
class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    /**
     * Load and compile shaders from file paths.
     * Returns true if successful, false on error.
     */
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * Load and compile shaders from source strings.
     * Useful for embedded/procedural shaders.
     */
    bool LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

    /**
     * Bind this shader program for rendering.
     * Call before drawing geometry that uses this shader.
     */
    void Use() const;

    /**
     * Unbind the current shader program (return to fixed-function).
     */
    static void Unuse();

    /**
     * Get the OpenGL program ID.
     */
    GLuint GetProgramID() const { return programID_; }

    /**
     * Check if the shader is valid and ready to use.
     */
    bool IsValid() const { return programID_ != 0; }

    /**
     * Get compilation/linking error log.
     */
    const std::string& GetErrorLog() const { return errorLog_; }

    /**
     * Reload shaders from disk (hot-reloading for development).
     */
    bool Reload();

    // ===== Uniform Setters =====
    // Automatically cache uniform locations for performance

    void SetUniform(const std::string& name, int value);
    void SetUniform(const std::string& name, float value);
    void SetUniform(const std::string& name, float x, float y);
    void SetUniform(const std::string& name, float x, float y, float z);
    void SetUniform(const std::string& name, float x, float y, float z, float w);
    
    // Matrix uniforms (4x4 matrices in column-major order)
    void SetUniformMatrix4(const std::string& name, const float* matrix);
    
    // Texture uniforms (texture unit index)
    void SetUniformTexture(const std::string& name, int textureUnit);

    /**
     * Delete the shader program and free resources.
     */
    void Cleanup();

private:
    GLuint programID_;
    GLuint vertexShaderID_;
    GLuint fragmentShaderID_;

    std::string vertexPath_;
    std::string fragmentPath_;
    std::string errorLog_;

    // Cached uniform locations for fast access
    mutable std::unordered_map<std::string, GLint> uniformLocationCache_;

    /**
     * Compile a shader from source code.
     * Returns shader ID on success, 0 on failure.
     */
    GLuint CompileShader(GLenum shaderType, const std::string& source, const std::string& path);

    /**
     * Link vertex and fragment shaders into a program.
     * Returns true on success, false on failure.
     */
    bool LinkProgram();

    /**
     * Get or cache uniform location.
     */
    GLint GetUniformLocation(const std::string& name) const;

    /**
     * Read shader source from file.
     */
    static std::string ReadFile(const std::string& path);
};
