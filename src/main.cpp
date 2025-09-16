#include "MainLoop.h"
#include <iostream>

int main() {
    MainLoop engine;
    engine.Init();
    // Run the main loop demo for 5 seconds
    engine.MainLoopFunc(5);
    engine.Shutdown();
    return 0;
}
