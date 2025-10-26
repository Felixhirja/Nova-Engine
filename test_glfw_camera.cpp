#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
constexpr int kInitialWindowWidth = 1280;
constexpr int kInitialWindowHeight = 720;
constexpr float kNearPlane = 0.1f;
constexpr float kFarPlane = 200.0f;
constexpr float kDefaultSpeed = 5.0f;
constexpr float kDefaultSensitivity = 0.08f;
constexpr float kMinFov = 30.0f;
constexpr float kMaxFov = 90.0f;

int gFramebufferWidth = kInitialWindowWidth;
int gFramebufferHeight = kInitialWindowHeight;
bool gCursorCaptured = true;
bool gTabWasDown = false;
bool gFirstMouseEvent = true;
double gLastCursorX = 0.0;
double gLastCursorY = 0.0;

// Simple first-person style camera that keeps view space aligned with -Z.
class Camera
{
public:
    Camera() = default;

    glm::mat4 viewMatrix() const
    {
        return glm::lookAt(m_position, m_position + forward(), worldUp);
    }

    void processKeyboard(GLFWwindow* window, float deltaSeconds)
    {
        const float velocity = m_speed * deltaSeconds;
        const glm::vec3 forwardVector = forward();
        glm::vec3 forwardXZ(forwardVector.x, 0.0f, forwardVector.z);
        const float forwardLenSq = glm::dot(forwardXZ, forwardXZ);
        if (forwardLenSq > 1e-6f)
        {
            forwardXZ = glm::normalize(forwardXZ);
        }
        else
        {
            forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f);
        }

        glm::vec3 right = glm::cross(forwardXZ, worldUp);
        if (glm::dot(right, right) > 1e-6f)
        {
            right = glm::normalize(right);
        }
        else
        {
            right = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            m_position += forwardXZ * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            m_position -= forwardXZ * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            m_position -= right * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            m_position += right * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            m_position += worldUp * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            m_position -= worldUp * velocity;
        }
    }

    void processMouseDelta(float xOffset, float yOffset)
    {
        m_yaw += xOffset * m_sensitivity;
        m_pitch -= yOffset * m_sensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    void processScroll(double yOffset)
    {
        m_fov -= static_cast<float>(yOffset);
        m_fov = std::clamp(m_fov, kMinFov, kMaxFov);
    }

    glm::vec3 position() const
    {
        return m_position;
    }

    float fovDegrees() const
    {
        return m_fov;
    }

private:
    glm::vec3 forward() const
    {
        const float yawRadians = glm::radians(m_yaw);
        const float pitchRadians = glm::radians(m_pitch);

        glm::vec3 direction;
        direction.x = std::cos(yawRadians) * std::cos(pitchRadians);
        direction.y = std::sin(pitchRadians);
        direction.z = std::sin(yawRadians) * std::cos(pitchRadians);
        return glm::normalize(direction);
    }

    glm::vec3 m_position{0.0f, 0.0f, 3.0f};
    float m_yaw{-90.0f};
    float m_pitch{0.0f};
    float m_speed{kDefaultSpeed};
    float m_sensitivity{kDefaultSensitivity};
    float m_fov{60.0f};
    const glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
};

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    gFramebufferWidth = width;
    gFramebufferHeight = height;
    glViewport(0, 0, width, height);
}

double gPendingMouseDX = 0.0;
double gPendingMouseDY = 0.0;
GLuint gCubeVbo = 0;

void mousePositionCallbackWithDelta(GLFWwindow*, double xpos, double ypos)
{
    if (!gCursorCaptured)
    {
        gFirstMouseEvent = true;
        return;
    }

    if (gFirstMouseEvent)
    {
        gLastCursorX = xpos;
        gLastCursorY = ypos;
        gFirstMouseEvent = false;
    }

    gPendingMouseDX += (xpos - gLastCursorX);
    gPendingMouseDY += (ypos - gLastCursorY);
    // Deltas are applied after polling to keep mouse look frame-rate independent.
    gLastCursorX = xpos;
    gLastCursorY = ypos;
}

double gScrollDelta = 0.0;

void scrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    gScrollDelta += yoffset;
}

bool initializeGlfw()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog(static_cast<size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog(static_cast<size_t>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
        std::cerr << "Program linking failed: " << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// Upload a unit cube with per-face normals for basic Lambert shading.
GLuint createCube()
{
    constexpr std::array<float, 6 * 6 * 6> vertices = {
        // Positions          // Normals
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    gCubeVbo = vbo;

    return vao;
}

void toggleCursorCapture(GLFWwindow* window)
{
    gCursorCaptured = !gCursorCaptured;
    if (gCursorCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        gFirstMouseEvent = true;
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

} // namespace

int main()
{
    if (!initializeGlfw())
    {
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(kInitialWindowWidth, kInitialWindowHeight, "GLFW Camera Demo", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to load GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSwapInterval(1); // Enable VSync for tear-free presentation.
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mousePositionCallbackWithDelta);
    glfwSetScrollCallback(window, scrollCallback);

    glEnable(GL_DEPTH_TEST);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Camera camera;

    constexpr char vertexShaderSource[] = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProj;

        out vec3 vNormal;
        out vec3 vPosWorld;

        void main()
        {
            vec4 worldPos = uModel * vec4(aPos, 1.0);
            vPosWorld = worldPos.xyz;
            vNormal = mat3(transpose(inverse(uModel))) * aNormal;
            gl_Position = uProj * uView * worldPos;
        }
    )GLSL";

    constexpr char fragmentShaderSource[] = R"GLSL(
        #version 330 core
        in vec3 vNormal;
        in vec3 vPosWorld;

        uniform vec3 uLightDir;
        uniform vec3 uAlbedo;

        out vec4 FragColor;

        void main()
        {
            vec3 normal = normalize(vNormal);
            vec3 lightDir = normalize(-uLightDir);
            float NdotL = max(dot(normal, lightDir), 0.0);
            vec3 color = uAlbedo * (0.1 + 0.9 * NdotL);
            FragColor = vec4(color, 1.0);
        }
    )GLSL";

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLuint shaderProgram = linkProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (shaderProgram == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLuint cubeVao = createCube();

    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.6f));
    glm::vec3 albedo = glm::vec3(0.8f, 0.6f, 0.3f);

    float lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        const bool tabIsDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabIsDown && !gTabWasDown)
        {
            toggleCursorCapture(window);
        }
        gTabWasDown = tabIsDown;

        camera.processKeyboard(window, deltaTime);
        if (gCursorCaptured && (gPendingMouseDX != 0.0 || gPendingMouseDY != 0.0))
        {
            camera.processMouseDelta(static_cast<float>(gPendingMouseDX), static_cast<float>(gPendingMouseDY));
            gPendingMouseDX = 0.0;
            gPendingMouseDY = 0.0;
        }

        if (gScrollDelta != 0.0)
        {
            camera.processScroll(gScrollDelta);
            gScrollDelta = 0.0;
        }

        glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        const glm::mat4 model = glm::mat4(1.0f);
        const glm::mat4 view = camera.viewMatrix();
        const float aspect = static_cast<float>(gFramebufferWidth) / static_cast<float>(std::max(gFramebufferHeight, 1));
        const glm::mat4 projection = glm::perspective(glm::radians(camera.fovDegrees()), aspect, kNearPlane, kFarPlane);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "uLightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(shaderProgram, "uAlbedo"), 1, glm::value_ptr(albedo));

        glBindVertexArray(cubeVao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &cubeVao);
    if (gCubeVbo != 0)
    {
        glDeleteBuffers(1, &gCubeVbo);
        gCubeVbo = 0;
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
