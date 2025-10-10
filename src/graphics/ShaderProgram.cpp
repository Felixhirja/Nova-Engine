#include "ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>

// OpenGL shader function definitions
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

// Define GLchar if not already defined
#ifndef GLchar
typedef char GLchar;
#endif

// Function pointers for shader extensions (loaded dynamically)
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

static PFNGLCREATESHADERPROC glCreateShader = nullptr;
static PFNGLDELETESHADERPROC glDeleteShader = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
static PFNGLATTACHSHADERPROC glAttachShader = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
static PFNGLUNIFORM1IPROC glUniform1i = nullptr;
static PFNGLUNIFORM1FPROC glUniform1f = nullptr;
static PFNGLUNIFORM2FPROC glUniform2f = nullptr;
static PFNGLUNIFORM3FPROC glUniform3f = nullptr;
static PFNGLUNIFORM4FPROC glUniform4f = nullptr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;

// Helper function to load shader extensions
static bool LoadShaderExtensions() {
    static bool loaded = false;
    static bool available = false;
    
    if (loaded) return available;
    loaded = true;
    
    // Try to load shader extension functions
#ifdef _WIN32
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
#else
    // For Linux/Unix, use glXGetProcAddress
    glCreateShader = (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte*)"glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte*)"glDeleteShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)glXGetProcAddress((const GLubyte*)"glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte*)"glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)glXGetProcAddress((const GLubyte*)"glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glDeleteProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glXGetProcAddress((const GLubyte*)"glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)glXGetProcAddress((const GLubyte*)"glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)glXGetProcAddress((const GLubyte*)"glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)glXGetProcAddress((const GLubyte*)"glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)glXGetProcAddress((const GLubyte*)"glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)glXGetProcAddress((const GLubyte*)"glUniform4f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix4fv");
#endif
    
    available = (glCreateShader != nullptr &&
                 glDeleteShader != nullptr &&
                 glShaderSource != nullptr &&
                 glCompileShader != nullptr &&
                 glGetShaderiv != nullptr &&
                 glGetShaderInfoLog != nullptr &&
                 glCreateProgram != nullptr &&
                 glDeleteProgram != nullptr &&
                 glAttachShader != nullptr &&
                 glLinkProgram != nullptr &&
                 glGetProgramiv != nullptr &&
                 glGetProgramInfoLog != nullptr &&
                 glUseProgram != nullptr &&
                 glGetUniformLocation != nullptr &&
                 glUniform1i != nullptr &&
                 glUniform1f != nullptr &&
                 glUniform2f != nullptr &&
                 glUniform3f != nullptr &&
                 glUniform4f != nullptr &&
                 glUniformMatrix4fv != nullptr);
    
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
    if (!LoadShaderExtensions()) {
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
    if (!LoadShaderExtensions()) {
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

    std::cout << "Shader compiled successfully";
    if (!vertexPath_.empty()) {
        std::cout << " (" << vertexPath_ << ", " << fragmentPath_ << ")";
    }
    std::cout << std::endl;

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
    programID_ = glCreateProgram();
    if (programID_ == 0) {
        errorLog_ = "Failed to create shader program";
        return false;
    }

    glAttachShader(programID_, vertexShaderID_);
    glAttachShader(programID_, fragmentShaderID_);
    glLinkProgram(programID_);

    // Check link status
    GLint success = 0;
    glGetProgramiv(programID_, GL_LINK_STATUS, &success);

    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(programID_, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::string infoLog(logLength, '\0');
            glGetProgramInfoLog(programID_, logLength, nullptr, &infoLog[0]);
            errorLog_ = "Shader linking failed:\n" + infoLog;
        } else {
            errorLog_ = "Shader linking failed (no error log available)";
        }

        glDeleteProgram(programID_);
        programID_ = 0;
        std::cerr << errorLog_ << std::endl;
        return false;
    }

    // Shaders can be detached and deleted after successful linking
    // The program retains the compiled code
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
    if (glUseProgram != nullptr) {
        glUseProgram(0);
    }
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
