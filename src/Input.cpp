#include "Input.h"

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

// Static member definitions
bool Input::keyStates[256] = {false};
void* Input::sdlWindow = nullptr;
#ifdef USE_GLFW
void* Input::glfwWindow = nullptr;
#endif
double Input::mouseWheelDelta = 0.0;

// GLFW scroll callback
#ifdef USE_GLFW
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    std::cout << "Mouse wheel: x=" << xoffset << ", y=" << yoffset << std::endl;
    Input::AddMouseWheelDelta(yoffset);
}
#endif

void Input::Init() {}

void Input::Shutdown() {}

bool Input::HasWindowFocus() {
#ifdef USE_GLFW
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        return glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    }
#endif
#ifdef USE_SDL
    if (sdlWindow) {
        SDL_Window* window = static_cast<SDL_Window*>(sdlWindow);
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
#endif
    return false;
}

int Input::PollKey() {
    // Only process input if window has focus
    if (!HasWindowFocus()) return -1;
    
#ifdef USE_GLFW
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        // Check for key presses (GLFW doesn't have a direct poll function like SDL)
        // We'll check specific keys that the application uses
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) return 27;
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) return 9; // Tab key
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) return 'a';
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) return 'd';
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) return ' ';
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) return 'q';
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) return 'z';
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) return 'x';
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) return 'p';
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) return 'w';
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) return 's';
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) return 'e';
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) return 'c';
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) return 't';
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) return 'i';
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) return '[';
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) return ']';
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) return '0';
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) return '1';
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) return '2';
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) return '3';
    }
#endif
#ifdef USE_SDL
    // Poll SDL events for input
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // Log event type
        std::ofstream log("sdl_diag.log", std::ios::app);
        log << "SDL Event: " << e.type << std::endl;
        log.close();
        if (e.type == SDL_QUIT) return 'q';
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode kc = e.key.keysym.sym;
            if (kc == SDLK_ESCAPE) return 27;
            if (kc == SDLK_TAB) return 9; // Tab key
            if (kc == SDLK_a) return 'a';
            if (kc == SDLK_d) return 'd';
            if (kc == SDLK_w) return 'w';
            if (kc == SDLK_s) return 's';
            if (kc == SDLK_e) return 'e';
            if (kc == SDLK_c) return 'c';
            if (kc == SDLK_SPACE) return ' ';
            if (kc == SDLK_q) return 'q';
            if (kc == SDLK_z) return 'z';
            if (kc == SDLK_x) return 'x';
            if (kc == SDLK_t) return 't';
            if (kc == SDLK_i) return 'i';
            if (kc == SDLK_LEFTBRACKET) return '[';
            if (kc == SDLK_RIGHTBRACKET) return ']';
            if (kc == SDLK_0) return '0';
            if (kc == SDLK_1) return '1';
            if (kc == SDLK_2) return '2';
            if (kc == SDLK_3) return '3';
        }
    }
#endif
    return -1;
}

bool Input::IsKeyHeld(char key) {
    // Only process input if window has focus
    if (!HasWindowFocus()) return false;
    
#ifdef USE_GLFW
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        int glfwKey = GLFW_KEY_UNKNOWN;
        switch (key) {
            case 'a': case 'A': glfwKey = GLFW_KEY_A; break;
            case 'd': case 'D': glfwKey = GLFW_KEY_D; break;
            case 'w': case 'W': glfwKey = GLFW_KEY_W; break;
            case 's': case 'S': glfwKey = GLFW_KEY_S; break;
            case 'e': case 'E': glfwKey = GLFW_KEY_E; break;
            case 'c': case 'C': glfwKey = GLFW_KEY_C; break;
            case 't': case 'T': glfwKey = GLFW_KEY_T; break;
            case 'q': case 'Q': glfwKey = GLFW_KEY_Q; break;
            case '1': glfwKey = GLFW_KEY_1; break;
            case '2': glfwKey = GLFW_KEY_2; break;
            case '3': glfwKey = GLFW_KEY_3; break;
            case ' ': glfwKey = GLFW_KEY_SPACE; break;
            default: return false;
        }
        return glfwGetKey(window, glfwKey) == GLFW_PRESS;
    }
#endif
#ifdef USE_SDL
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (!state) return false;
    
    SDL_Keycode kc = SDLK_UNKNOWN;
    switch (key) {
        case 'a': case 'A': kc = SDLK_a; break;
        case 'd': case 'D': kc = SDLK_d; break;
        case 'w': case 'W': kc = SDLK_w; break;
        case 's': case 'S': kc = SDLK_s; break;
        case 'e': case 'E': kc = SDLK_e; break;
        case 'c': case 'C': kc = SDLK_c; break;
        case 't': case 'T': kc = SDLK_t; break;
        case ' ': kc = SDLK_SPACE; break;
        case 'q': case 'Q': kc = SDLK_q; break;
        case 'z': case 'Z': kc = SDLK_z; break;
        case 'x': case 'X': kc = SDLK_x; break;
        case '1': kc = SDLK_1; break;
        case '2': kc = SDLK_2; break;
        case '3': kc = SDLK_3; break;
        default: return false;
    }
    
    return state[SDL_GetScancodeFromKey(kc)] != 0;
#else
    return false;
#endif
}

void Input::UpdateKeyState() {
#ifdef USE_GLFW
    // GLFW handles key state directly in glfwGetKey, no need to poll events
    if (glfwWindow) {
        glfwPollEvents();
    }
#endif
#ifdef USE_SDL
    // Poll events to keep SDL happy and handle mouse wheel
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // Log window close event
            std::ofstream log("sdl_diag.log", std::ios::app);
            log << "SDL_QUIT event received (window closed)" << std::endl;
            log.close();
        }
        if (e.type == SDL_MOUSEWHEEL) {
            mouseWheelDelta += e.wheel.y; // Accumulate wheel delta
        }
    }
#endif
}

#ifdef USE_GLFW
bool Input::IsArrowKeyHeld(int arrowKey) {
    // Only process input if window has focus
    if (!HasWindowFocus()) return false;
    
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        return glfwGetKey(window, arrowKey) == GLFW_PRESS;
    }
    return false;
}
#endif

double Input::GetMouseWheelDelta() {
    return mouseWheelDelta;
}

void Input::ResetMouseWheelDelta() {
    mouseWheelDelta = 0.0;
}

void Input::AddMouseWheelDelta(double delta) {
    mouseWheelDelta += delta;
}
