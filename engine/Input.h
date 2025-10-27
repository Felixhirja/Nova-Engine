#ifndef INPUT_H
#define INPUT_H

// Minimal cross-platform non-blocking console input helper for demo purposes.
// Uses termios on POSIX.

#ifdef USE_SDL
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#elif defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#endif
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif
#include <iostream>

// GLFW scroll callback function (global)
#ifdef USE_GLFW
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
#endif

class Input {
public:
    static void Init();
    static void Shutdown();
    // Returns -1 if no key, otherwise ASCII code of key.
    static int PollKey();
    
    // New methods for hold-based input
    static bool IsKeyHeld(char key);
    static void UpdateKeyState();
    
#ifdef USE_GLFW
    // Set the GLFW window for input (needed for GLFW input functions)
    static void SetGLFWWindow(void* window) { 
        glfwWindow = window; 
        if (window) {
            std::cout << "Setting GLFW scroll callback" << std::endl;
            glfwSetScrollCallback(static_cast<GLFWwindow*>(window), scroll_callback);
        }
    }
    
    // Arrow key support for camera movement
    static bool IsArrowKeyHeld(int arrowKey); // GLFW_KEY_UP, GLFW_KEY_DOWN, etc.
#endif
    
    // Set the SDL window for input (needed for SDL focus checks)
    static void SetSDLWindow(void* window) { sdlWindow = window; }
    
    // Mouse wheel support
    static double GetMouseWheelDelta();
    static void ResetMouseWheelDelta();
    static void AddMouseWheelDelta(double delta);
    
private:
    static bool keyStates[256]; // Track which keys are currently held
#ifdef USE_GLFW
    static void* glfwWindow; // GLFW window pointer
#endif
    static void* sdlWindow; // SDL window pointer
    static double mouseWheelDelta; // Accumulated mouse wheel delta
    
    // Helper method to check if window has focus
    static bool HasWindowFocus();
};

#endif // INPUT_H
