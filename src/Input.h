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

class Input {
public:
    static void Init();
    static void Shutdown();
    // Returns -1 if no key, otherwise ASCII code of key.
    static int PollKey();
    
    // New methods for hold-based input
    static bool IsKeyHeld(char key);
    static void UpdateKeyState();
    
    // Set the GLFW window for input (needed for GLFW input functions)
    static void SetGLFWWindow(void* window) { glfwWindow = window; }
    
    // Arrow key support for camera movement
    static bool IsArrowKeyHeld(int arrowKey); // GLFW_KEY_UP, GLFW_KEY_DOWN, etc.
    
private:
    static bool keyStates[256]; // Track which keys are currently held
    static void* glfwWindow; // GLFW window pointer
};

#endif // INPUT_H
