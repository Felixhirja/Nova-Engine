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
void* Input::glfwWindow = nullptr;

void Input::Init() {}

void Input::Shutdown() {}

int Input::PollKey() {
#ifdef USE_GLFW
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        // Check for key presses (GLFW doesn't have a direct poll function like SDL)
        // We'll check specific keys that the application uses
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) return 27;
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
            if (kc == SDLK_a) return 'a';
            if (kc == SDLK_d) return 'd';
            if (kc == SDLK_SPACE) return ' ';
            if (kc == SDLK_q) return 'q';
            if (kc == SDLK_z) return 'z';
            if (kc == SDLK_x) return 'x';
        }
    }
#endif
    return -1;
}

bool Input::IsKeyHeld(char key) {
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
            case 'q': case 'Q': glfwKey = GLFW_KEY_Q; break;
            case ' ': glfwKey = GLFW_KEY_SPACE; break;
            default: return false;
        }
        return glfwGetKey(window, glfwKey) == GLFW_PRESS;
    }
#endif
#ifdef USE_SDL
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (!state) return false;
    
    SDL_Keycode kc = SDLK_UNKNOWN;e
    switch (key) {
        case 'a': case 'A': kc = SDLK_a; break;
        case 'd': case 'D': kc = SDLK_d; break;
        case ' ': kc = SDLK_SPACE; break;
        case 'q': case 'Q': kc = SDLK_q; break;
        case 'z': case 'Z': kc = SDLK_z; break;
        case 'x': case 'X': kc = SDLK_x; break;
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
    // Just poll events to keep SDL happy, but we use SDL_GetKeyboardState for held keys now
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // Log window close event
            std::ofstream log("sdl_diag.log", std::ios::app);
            log << "SDL_QUIT event received (window closed)" << std::endl;
            log.close();
        }
    }
#endif
}

bool Input::IsArrowKeyHeld(int arrowKey) {
#ifdef USE_GLFW
    if (glfwWindow) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        return glfwGetKey(window, arrowKey) == GLFW_PRESS;
    }
#endif
#ifdef USE_SDL
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (!state) return false;
    
    SDL_Keycode kc = SDLK_UNKNOWN;
    switch (arrowKey) {
        case GLFW_KEY_UP: kc = SDLK_UP; break;
        case GLFW_KEY_DOWN: kc = SDLK_DOWN; break;
        case GLFW_KEY_LEFT: kc = SDLK_LEFT; break;
        case GLFW_KEY_RIGHT: kc = SDLK_RIGHT; break;
        default: return false;
    }
    
    return state[SDL_GetScancodeFromKey(kc)] != 0;
#else
    return false;
#endif
}
