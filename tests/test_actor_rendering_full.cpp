#include <iostream>
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "engine/ecs/EntityManager.h"
#include "engine/ecs/Components.h"
#include "engine/graphics/ActorRenderer.h"

// Simple texture loader for demo purposes
// In a real engine, you'd use STB, SOIL, or similar
GLuint LoadTextureFromColor(float r, float g, float b, int width = 64, int height = 64) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Create a simple colored texture
    unsigned char* data = new unsigned char[width * height * 4];
    for (int i = 0; i < width * height * 4; i += 4) {
        data[i] = (unsigned char)(r * 255);     // R
        data[i+1] = (unsigned char)(g * 255);   // G
        data[i+2] = (unsigned char)(b * 255);   // B
        data[i+3] = 255;                        // A
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] data;
    return textureID;
}

int main(int argc, char* argv[]) {
    std::cout << "Testing ActorRenderer with DrawComponent..." << std::endl;

    // Initialize GLFW for OpenGL context
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 1;
    }

    // Create EntityManager
    ecs::EntityManagerV2 entityManager;

    // Create some test entities with DrawComponents
    std::cout << "Creating test entities..." << std::endl;

    // Fighter entity
    auto fighterEntity = entityManager.CreateEntity();
    auto& fighterDraw = entityManager.AddComponent<DrawComponent>(fighterEntity);
    auto& fighterPos = entityManager.AddComponent<Position>(fighterEntity);

    fighterDraw.mode = DrawComponent::RenderMode::Sprite2D;
    fighterDraw.visible = true;
    fighterDraw.spriteScale = 2.0f;
    fighterDraw.textureHandle = LoadTextureFromColor(0.8f, 0.4f, 0.1f, 64, 32); // Orange fighter
    fighterPos.x = -2.0f;
    fighterPos.y = 1.0f;
    fighterPos.z = 0.0f;

    // Freighter entity
    auto freighterEntity = entityManager.CreateEntity();
    auto& freighterDraw = entityManager.AddComponent<DrawComponent>(freighterEntity);
    auto& freighterPos = entityManager.AddComponent<Position>(freighterEntity);

    freighterDraw.mode = DrawComponent::RenderMode::Sprite2D;
    freighterDraw.visible = true;
    freighterDraw.spriteScale = 3.0f;
    freighterDraw.textureHandle = LoadTextureFromColor(0.5f, 0.5f, 0.5f, 96, 48); // Gray freighter
    freighterPos.x = 2.0f;
    freighterPos.y = -1.0f;
    freighterPos.z = 0.0f;

    // Station entity
    auto stationEntity = entityManager.CreateEntity();
    auto& stationDraw = entityManager.AddComponent<DrawComponent>(stationEntity);
    auto& stationPos = entityManager.AddComponent<Position>(stationEntity);

    stationDraw.mode = DrawComponent::RenderMode::Billboard;
    stationDraw.visible = true;
    stationDraw.spriteScale = 4.0f;
    stationDraw.textureHandle = LoadTextureFromColor(0.7f, 0.7f, 0.9f, 128, 96); // Light blue station
    stationPos.x = 0.0f;
    stationPos.y = 0.0f;
    stationPos.z = -5.0f;

    // Projectile entity
    auto projectileEntity = entityManager.CreateEntity();
    auto& projectileDraw = entityManager.AddComponent<DrawComponent>(projectileEntity);
    auto& projectilePos = entityManager.AddComponent<Position>(projectileEntity);

    projectileDraw.mode = DrawComponent::RenderMode::Billboard;
    projectileDraw.visible = true;
    projectileDraw.spriteScale = 0.5f;
    projectileDraw.textureHandle = LoadTextureFromColor(1.0f, 1.0f, 0.0f, 16, 8); // Yellow projectile
    projectilePos.x = -1.0f;
    projectilePos.y = 0.5f;
    projectilePos.z = -2.0f;

    // Initialize ActorRenderer
    std::cout << "Initializing ActorRenderer..." << std::endl;
    ActorRenderer actorRenderer;
    if (!actorRenderer.Initialize()) {
        std::cerr << "Failed to initialize ActorRenderer" << std::endl;
        return 1;
    }

    // Create a simple camera for billboard rendering
    // In a real engine, this would be a proper Camera class
    struct SimpleCamera {
        float x = 0, y = 0, z = 5;
        float yaw = 0, pitch = 0;
    } camera;

    std::cout << "Rendering test entities..." << std::endl;

    // Set up viewport
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Dark blue background

    // Render a few frames
    for (int frame = 0; frame < 3; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render all drawable entities
        actorRenderer.Render(entityManager, nullptr); // Passing nullptr for camera for now

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::cout << "Rendered frame " << (frame + 1) << std::endl;
    }

    // Cleanup
    actorRenderer.Cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "ActorRenderer test completed successfully!" << std::endl;
    std::cout << "Entities created and rendered:" << std::endl;
    std::cout << "- Fighter (Sprite2D): position (-2, 1, 0), scale 2.0" << std::endl;
    std::cout << "- Freighter (Sprite2D): position (2, -1, 0), scale 3.0" << std::endl;
    std::cout << "- Station (Billboard): position (0, 0, -5), scale 4.0" << std::endl;
    std::cout << "- Projectile (Billboard): position (-1, 0.5, -2), scale 0.5" << std::endl;

    return 0;
}