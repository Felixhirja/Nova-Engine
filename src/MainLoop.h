#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <string>

class Viewport3D;
class Simulation;

class MainLoop {
public:
    MainLoop();
    ~MainLoop();

    void Init();
    // Runs the main loop. maxSeconds specifies how long the demo runs (0 = run once iteration only).
    void MainLoopFunc(int maxSeconds = 5);
    void Shutdown();

    std::string GetVersion() const;

private:
    bool running;
    std::string version;
    Viewport3D* viewport;
    Simulation* simulation;
};

#endif // MAIN_LOOP_H
