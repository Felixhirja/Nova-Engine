#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <string>
#include <memory>

class Viewport3D;
class Simulation;
class ResourceManager;
class EntityManager;

class Camera;

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
    std::unique_ptr<Viewport3D> viewport;
    std::unique_ptr<Simulation> simulation;
    std::unique_ptr<ResourceManager> resourceManager;
    // Camera instance
    std::unique_ptr<Camera> camera;
    // Canonical ECS entity manager
    std::unique_ptr<EntityManager> entityManager;
};

#endif // MAIN_LOOP_H
