#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL3/SDL.h>

int main(int argc, char** argv) {
    std::cout << "test_sdl: Before SDL_Init" << std::endl;
    // Try to force a software video driver to see if that helps initialization
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "software");
    const char* plat = SDL_GetPlatform();
    std::cout << "test_sdl: SDL_GetPlatform='" << (plat?plat:"(null)") << "'" << std::endl;
    int rc = SDL_Init(SDL_INIT_VIDEO);
    std::cout << "test_sdl: SDL_Init rc=" << rc << ", SDL_GetError='" << SDL_GetError() << "'" << std::endl;
#ifdef _WIN32
    std::cout << "test_sdl: Win32 GetLastError=" << GetLastError() << std::endl;
#endif
    SDL_Quit();
    return 0;
}
