#include "MainLoop.h"
#include "GameConfigInit.h"
#include <iostream>
#include <fstream>

#ifdef USE_SDL
extern "C" int SDL_main(int argc, char** argv) {
    // Log to file immediately
    std::ofstream log("sdl_diag.log", std::ios::app);
    log << "SDL_main started" << std::endl;
    log.close();
    
    // Initialize Configuration Management System
    std::cout << "Initializing Configuration System..." << std::endl;
    if (!NovaEngine::GameConfigInit::Initialize()) {
        std::cerr << "Failed to initialize configuration system!" << std::endl;
        return 1;
    }
    
    MainLoop engine;
    std::cout << "About to call engine.Init()" << std::endl;
    engine.Init();
    std::cout << "engine.Init() completed, about to call MainLoopFunc" << std::endl;
    // Run the main loop indefinitely until quit
    engine.MainLoopFunc(0);
    std::cout << "MainLoopFunc completed, shutting down" << std::endl;
    engine.Shutdown();
    
    // Shutdown Configuration System with analytics
    NovaEngine::GameConfigInit::Shutdown();
    
    return 0;
}
#ifdef _WIN32
#include <windows.h>
// Provide WinMain to satisfy -mwindows linking; call SDL_main with CRT args
extern "C" int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return SDL_main(__argc, __argv);
}
#endif
#else
int main() {
    // Log to file immediately
    std::ofstream log("sdl_diag.log", std::ios::app);
    log << "main started" << std::endl;
    log.close();
    
    // Initialize Configuration Management System
    std::cout << "Initializing Configuration System..." << std::endl;
    if (!NovaEngine::GameConfigInit::Initialize()) {
        std::cerr << "Failed to initialize configuration system!" << std::endl;
        return 1;
    }
    
    std::cout << "main() started, about to create MainLoop" << std::endl;
    MainLoop engine;
    std::cout << "MainLoop created, about to call engine.Init()" << std::endl;
    engine.Init();
    std::cout << "engine.Init() completed, about to call MainLoopFunc" << std::endl;
    // Run the main loop indefinitely until quit
    engine.MainLoopFunc(0);
    std::cout << "MainLoopFunc completed, shutting down" << std::endl;
    engine.Shutdown();
    
    // Shutdown Configuration System with analytics
    NovaEngine::GameConfigInit::Shutdown();
    
    return 0;
}
#endif
