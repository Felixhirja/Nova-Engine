#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>

// Include glad BEFORE GLFW to prevent GL header conflicts
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "engine/graphics/ShaderManager.h"
#include "engine/graphics/ShaderProgram.h"
#include "engine/graphics/Material.h"
#include "engine/graphics/MaterialLibrary.h"

using namespace Nova;

// Simple camera
struct Camera {
    glm::vec3 position = glm::vec3(0.0f, 2.0f, 10.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float fov = 45.0f;
    float aspectRatio = 16.0f / 9.0f;
    
    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(position, target, up);
    }
    
    glm::mat4 GetProjectionMatrix() const {
        return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    }
    
    glm::mat4 GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }
};

// Simple sphere mesh generation
struct SphereMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    
    void Generate(int segments = 32, int rings = 16) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        // Generate vertices
        for (int ring = 0; ring <= rings; ++ring) {
            float phi = glm::pi<float>() * float(ring) / float(rings);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            for (int seg = 0; seg <= segments; ++seg) {
                float theta = 2.0f * glm::pi<float>() * float(seg) / float(segments);
                float sinTheta = sin(theta);
                float cosTheta = cos(theta);
                
                // Position
                float x = sinPhi * cosTheta;
                float y = cosPhi;
                float z = sinPhi * sinTheta;
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                
                // Normal (same as position for unit sphere)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                
                // TexCoord
                vertices.push_back(float(seg) / float(segments));
                vertices.push_back(float(ring) / float(rings));
                
                // Tangent (approximation)
                vertices.push_back(-sinTheta);
                vertices.push_back(0.0f);
                vertices.push_back(cosTheta);
                
                // Bitangent (approximation)
                vertices.push_back(cosPhi * cosTheta);
                vertices.push_back(-sinPhi);
                vertices.push_back(cosPhi * sinTheta);
            }
        }
        
        // Generate indices
        for (int ring = 0; ring < rings; ++ring) {
            for (int seg = 0; seg < segments; ++seg) {
                int current = ring * (segments + 1) + seg;
                int next = current + segments + 1;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        
        indexCount = indices.size();
        
        // Create VAO, VBO, EBO
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        
        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        
        // TexCoord attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        
        // Tangent attribute
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        
        // Bitangent attribute
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
        
        glBindVertexArray(0);
    }
    
    void Draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void Cleanup() {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
    }
};

int main() {
    std::cout << "=== PBR Material Test Scene ===" << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "PBR Test Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Create shader manager and load PBR shader
    ShaderManager shaderManager;
    auto pbrShader = shaderManager.LoadShader(
        "pbr",
        "shaders/pbr/pbr.vert",
        "shaders/pbr/pbr.frag"
    );
    
    if (!pbrShader) {
        std::cerr << "Failed to load PBR shader!" << std::endl;
        return -1;
    }
    
    std::cout << "✓ PBR shader loaded successfully" << std::endl;
    
    // Create sphere mesh
    SphereMesh sphere;
    sphere.Generate();
    std::cout << "✓ Sphere mesh generated" << std::endl;
    
    // Create materials with varying roughness and metalness
    std::vector<std::shared_ptr<Material>> materials;
    
    // Row 1: Varying metalness (roughness = 0.2)
    for (int i = 0; i < 5; ++i) {
        auto mat = std::make_shared<Material>("metal_" + std::to_string(i));
        mat->SetBaseColor(glm::vec3(1.0f, 0.86f, 0.57f)); // Gold color
        mat->SetRoughness(0.2f);
        mat->SetMetalness(float(i) / 4.0f);
        materials.push_back(mat);
    }
    
    // Row 2: Varying roughness (metalness = 0.0)
    for (int i = 0; i < 5; ++i) {
        auto mat = std::make_shared<Material>("dielectric_" + std::to_string(i));
        mat->SetBaseColor(glm::vec3(0.8f, 0.2f, 0.2f)); // Red color
        mat->SetRoughness(float(i) / 4.0f);
        mat->SetMetalness(0.0f);
        materials.push_back(mat);
    }
    
    // Row 3: Varying roughness (metalness = 1.0)
    for (int i = 0; i < 5; ++i) {
        auto mat = std::make_shared<Material>("metal_rough_" + std::to_string(i));
        mat->SetBaseColor(glm::vec3(0.2f, 0.2f, 0.8f)); // Blue color
        mat->SetRoughness(float(i) / 4.0f);
        mat->SetMetalness(1.0f);
        materials.push_back(mat);
    }
    
    std::cout << "✓ Created " << materials.size() << " test materials" << std::endl;
    
    // Setup camera
    Camera camera;
    camera.aspectRatio = 1280.0f / 720.0f;
    
    // Setup lights
    glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f, 10.0f, 10.0f),
        glm::vec3(10.0f, 10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3(10.0f, -10.0f, 10.0f)
    };
    
    glm::vec3 lightColors[] = {
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f)
    };
    
    float lightIntensities[] = { 300.0f, 300.0f, 300.0f, 300.0f };
    
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "Arrow Keys - Rotate camera" << std::endl;
    std::cout << "\n=== Rendering ===" << std::endl;
    
    float cameraAngle = 0.0f;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Handle input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            cameraAngle -= 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            cameraAngle += 0.02f;
        }
        
        // Update camera position
        float radius = 10.0f;
        camera.position.x = radius * sin(cameraAngle);
        camera.position.z = radius * cos(cameraAngle);
        
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use PBR shader
        pbrShader->Use();
        
        // Set camera uniforms
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = camera.GetProjectionMatrix();
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();
        pbrShader->SetUniformMatrix4("u_View", glm::value_ptr(view));
        pbrShader->SetUniformMatrix4("u_Projection", glm::value_ptr(proj));
        pbrShader->SetUniformMatrix4("u_ViewProjection", glm::value_ptr(viewProj));
        pbrShader->SetUniform("u_CameraPos", camera.position.x, camera.position.y, camera.position.z);
        
        // Set lighting uniforms
        pbrShader->SetUniform("u_LightCount", 4);
        for (int i = 0; i < 4; ++i) {
            std::string posName = "u_LightPositions[" + std::to_string(i) + "]";
            std::string colorName = "u_LightColors[" + std::to_string(i) + "]";
            std::string intensityName = "u_LightIntensities[" + std::to_string(i) + "]";
            
            pbrShader->SetUniform(posName.c_str(), lightPositions[i].x, lightPositions[i].y, lightPositions[i].z);
            pbrShader->SetUniform(colorName.c_str(), lightColors[i].x, lightColors[i].y, lightColors[i].z);
            pbrShader->SetUniform(intensityName.c_str(), lightIntensities[i]);
        }
        
        // Set ambient lighting
        pbrShader->SetUniform("u_AmbientColor", 0.03f, 0.03f, 0.03f);
        pbrShader->SetUniform("u_AmbientIntensity", 1.0f);
        
        // Render spheres in a grid
        int matIndex = 0;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (matIndex >= materials.size()) break;
                
                // Calculate position
                float x = (col - 2.0f) * 2.5f;
                float y = (row - 1.0f) * 2.5f;
                float z = 0.0f;
                
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                pbrShader->SetUniformMatrix4("u_Model", glm::value_ptr(model));
                
                // Bind material and draw
                materials[matIndex]->Bind(pbrShader.get());
                sphere.Draw();
                materials[matIndex]->Unbind();
                
                matIndex++;
            }
        }
        
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    sphere.Cleanup();
    glfwTerminate();
    
    std::cout << "\n✓ Test completed successfully" << std::endl;
    return 0;
}
