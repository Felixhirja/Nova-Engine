#include "MainLoop.h"
#include "Viewport3D.h"
#include "Input.h"
#include "Simulation.h"
#include <iostream>
#include <chrono>
#include <thread>

MainLoop::MainLoop() : running(false), version("1.0.0"), viewport(nullptr) {}

MainLoop::~MainLoop() {
    Shutdown();
    if (viewport) {
        delete viewport;
        viewport = nullptr;
    }
    if (simulation) {
        delete simulation;
        simulation = nullptr;
    }
}

void MainLoop::Init() {
    std::cout << "Star Engine Initializing..." << std::endl;
    running = true;
    viewport = new Viewport3D();
    viewport->Init();
    simulation = new Simulation();
    simulation->Init();
    Input::Init();
}

void MainLoop::MainLoopFunc(int maxSeconds) {
    if (!running) {
        std::cout << "Engine not initialized!" << std::endl;
        return;
    }

    using clock = std::chrono::high_resolution_clock;

    const double updateHz = 60.0; // fixed update rate
    const std::chrono::duration<double> fixedDt(1.0 / updateHz);
    const double maxFPS = 240.0; // render cap
    const std::chrono::duration<double> minFrameTime(1.0 / maxFPS);

    std::cout << "Star Engine Fixed-Timestep Main Loop (update @ " << updateHz << " Hz)" << std::endl;

    auto demoStart = clock::now();
    auto previous = demoStart;
    std::chrono::duration<double> lag(0);
    int frames = 0;
    auto fpsTimer = demoStart;

    while (true) {
        auto current = clock::now();
        auto elapsed = current - previous;
        previous = current;
        lag += elapsed;

        // Poll input
        int key = Input::PollKey();
        static bool paused = false;
        if (key != -1) {
            if (key == 'q' || key == 'Q') break; // quit
            if (key == ' ') paused = !paused; // toggle pause
            // simple player control: A/D to nudge velocity
            if (key == 'a' || key == 'A') {
                // move player left quickly
                // we adjust player's position directly via simulation update
                if (simulation) {
                    // apply a small backward position change
                    // (real code would change player velocity/input state)
                    // here we simulate by updating with negative delta
                    simulation->Update(-0.02);
                }
            }
            if (key == 'd' || key == 'D') {
                if (simulation) simulation->Update(0.02);
            }
        }

        // Update simulation as many fixed steps as needed
        if (!paused) {
            while (lag >= fixedDt) {
                if (simulation) simulation->Update(fixedDt.count());
                lag -= fixedDt;
            }
        }

        // Render: draw player representation
        if (viewport) {
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            viewport->DrawPlayer(playerX);
        }
        ++frames;

        // Print FPS every second
        if (std::chrono::duration<double>(current - fpsTimer).count() >= 1.0) {
            double simPos = simulation ? simulation->GetPosition() : 0.0;
            double playerX = simulation ? simulation->GetPlayerX() : 0.0;
            std::cout << "FPS: " << frames << "  Simulation pos=" << simPos << "  Player x=" << playerX << std::endl;
            frames = 0;
            fpsTimer = current;
        }

        // Stop after demo duration
        if (maxSeconds > 0) {
            if (std::chrono::duration<double>(current - demoStart).count() >= maxSeconds) break;
        } else {
            break; // run one render if maxSeconds==0
        }

        // Frame cap
        auto frameEnd = clock::now();
        auto frameTime = frameEnd - current;
        if (frameTime < minFrameTime) std::this_thread::sleep_for(minFrameTime - frameTime);
    }
    Input::Shutdown();
}

void MainLoop::Shutdown() {
    if (running) {
        std::cout << "Star Engine Shutting down..." << std::endl;
        running = false;
    }
}

std::string MainLoop::GetVersion() const {
    return version;
}
