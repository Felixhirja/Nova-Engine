#include "ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Helper function to ensure required OpenGL functionality is available.
static bool EnsureShaderSupport() {
    static bool checked = false;
    static bool available = false;

    if (checked) {
        return available;
    }

    checked = true;
    available = GLAD_GL_VERSION_2_0 != 0;

    if (!available) {
        std::cerr << "Shader extensions not available (requires OpenGL 2.0+)" << std::endl;
    }

    return available;
}

ShaderProgram::ShaderProgram()
    : programID_(0)
    , vertexShaderID_(0)
    , fragmentShaderID_(0)
{
}

ShaderProgram::~ShaderProgram() {
    Cleanup();
}

bool ShaderProgram::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    if (!EnsureShaderSupport()) {
        errorLog_ = "Shader extensions not supported (OpenGL 2.0+ required)";
        return false;
    }

    // Store paths for hot-reloading
    vertexPath_ = vertexPath;
    fragmentPath_ = fragmentPath;

    // Read shader sources
    std::string vertexSrc = ReadFile(vertexPath);
    std::string fragmentSrc = ReadFile(fragmentPath);

    if (vertexSrc.empty()) {
        errorLog_ = "Failed to read vertex shader: " + vertexPath;
        return false;
    }

    if (fragmentSrc.empty()) {
        errorLog_ = "Failed to read fragment shader: " + fragmentPath;
        return false;
    }

    return LoadFromSource(vertexSrc, fragmentSrc);
}

bool ShaderProgram::LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
    if (!EnsureShaderSupport()) {
        errorLog_ = "Shader extensions not supported (OpenGL 2.0+ required)";
        return false;
    }

    // Clean up existing shaders if any
    Cleanup();

    // Compile vertex shader
    vertexShaderID_ = CompileShader(GL_VERTEX_SHADER, vertexSrc, vertexPath_);
    if (vertexShaderID_ == 0) {
        return false;
    }

    // Compile fragment shader
    fragmentShaderID_ = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath_);
    if (fragmentShaderID_ == 0) {
        glDeleteShader(vertexShaderID_);
        vertexShaderID_ = 0;
        return false;
    }

    // Link program
    if (!LinkProgram()) {
        glDeleteShader(vertexShaderID_);
        glDeleteShader(fragmentShaderID_);
        vertexShaderID_ = 0;
        fragmentShaderID_ = 0;
        return false;
    }

    return true;
}

GLuint ShaderProgram::CompileShader(GLenum shaderType, const std::string& source, const std::string& path) {
    GLuint shaderID = glCreateShader(shaderType);
    if (shaderID == 0) {
        errorLog_ = "Failed to create shader object";
        return 0;
    }

    const char* sourceCStr = source.c_str();
    glShaderSource(shaderID, 1, &sourceCStr, nullptr);
    glCompileShader(shaderID);

    // Check compilation status
    GLint success = 0;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::string infoLog(logLength, '\0');
            glGetShaderInfoLog(shaderID, logLength, nullptr, &infoLog[0]);
            
            errorLog_ = "Shader compilation failed";
            if (!path.empty()) {
                errorLog_ += " (" + path + ")";
            }
            errorLog_ += ":\n" + infoLog;
        } else {
            errorLog_ = "Shader compilation failed (no error log available)";
        }

        glDeleteShader(shaderID);
        std::cerr << errorLog_ << std::endl;
        return 0;
    }

    return shaderID;
}

bool ShaderProgram::LinkProgram() {
    // Create program if not already created
    if (programID_ == 0) {
        programID_ = glCreateProgram();
        if (programID_ == 0) {
            errorLog_ = "Failed to create shader program";
            return false;
        }
    }

    // Attach shaders
    glAttachShader(programID_, vertexShaderID_);
    glAttachShader(programID_, fragmentShaderID_);

    glLinkProgram(programID_);

    GLint success = 0;
    glGetProgramiv(programID_, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(programID_, GL_INFO_LOG_LENGTH, &logLength);
        
        if (logLength > 0) {
            std::string infoLog(logLength, '\0');
            glGetProgramInfoLog(programID_, logLength, nullptr, &infoLog[0]);
            errorLog_ = "Shader program linking failed:\n" + infoLog;
        } else {
            errorLog_ = "Shader program linking failed (no error log available)";
        }
        
        std::cerr << errorLog_ << std::endl;
        return false;
    }

    // Detach and delete shaders after linking
    glDetachShader(programID_, vertexShaderID_);
    glDetachShader(programID_, fragmentShaderID_);
    glDeleteShader(vertexShaderID_);
    glDeleteShader(fragmentShaderID_);
    vertexShaderID_ = 0;
    fragmentShaderID_ = 0;

    return true;
}

void ShaderProgram::Use() const {
    if (programID_ != 0) {
        glUseProgram(programID_);
    }
}

void ShaderProgram::Unuse() {
    glUseProgram(0);
}

bool ShaderProgram::Reload() {
    if (vertexPath_.empty() || fragmentPath_.empty()) {
        errorLog_ = "Cannot reload: shader was not loaded from files";
        return false;
    }

    return LoadFromFiles(vertexPath_, fragmentPath_);
}

GLint ShaderProgram::GetUniformLocation(const std::string& name) const {
    // Check cache first
    auto it = uniformLocationCache_.find(name);
    if (it != uniformLocationCache_.end()) {
        return it->second;
    }

    // Query OpenGL
    GLint location = glGetUniformLocation(programID_, name.c_str());
    if (location == -1) {
        // Uniform not found (not an error - might be optimized out)
        // Still cache it to avoid repeated queries
    }
    
    uniformLocationCache_[name] = location;
    return location;
}

void ShaderProgram::SetUniform(const std::string& name, int value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void ShaderProgram::SetUniform(const std::string& name, float value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void ShaderProgram::SetUniform(const std::string& name, float x, float y) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void ShaderProgram::SetUniform(const std::string& name, float x, float y, float z) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void ShaderProgram::SetUniform(const std::string& name, float x, float y, float z, float w) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void ShaderProgram::SetUniformMatrix4(const std::string& name, const float* matrix) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    }
}

void ShaderProgram::SetUniformTexture(const std::string& name, int textureUnit) {
    SetUniform(name, textureUnit);
}

void ShaderProgram::Cleanup() {
    if (vertexShaderID_ != 0) {
        glDeleteShader(vertexShaderID_);
        vertexShaderID_ = 0;
    }
    
    if (fragmentShaderID_ != 0) {
        glDeleteShader(fragmentShaderID_);
        fragmentShaderID_ = 0;
    }
    
    if (programID_ != 0) {
        glDeleteProgram(programID_);
        programID_ = 0;
    }
    
    uniformLocationCache_.clear();
}

std::string ShaderProgram::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
