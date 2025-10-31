#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    std::cout << "Testing GLFW initialization..." << std::endl;
    
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return 1;
    }
    
    std::cout << "GLFW initialized successfully!" << std::endl;
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    std::cout << "Window created successfully!" << std::endl;
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}
