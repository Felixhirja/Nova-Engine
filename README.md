# Star-Engine

A 3D game engine built with C++17, OpenGL, and GLFW.

## Features

- 3D rendering with OpenGL immediate mode
- Entity-component system (ECS)
- Camera system with mouse look
- Input handling (keyboard and mouse)
- Resource management
- Fixed-timestep game loop

## Dependencies

- GLFW 3.x
- OpenGL
- GLU
- MinGW-w64 (on Windows)

## Building

### Windows (MSYS2/MinGW)

1. Install MSYS2 from [MSYS2 website](https://www.msys2.org/)

2. Install dependencies:

   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-glu
   ```

3. Build the engine:

   ```bash
   make
   ```

### Linux

1. Install dependencies:

   ```bash
   sudo apt-get install build-essential libglfw3-dev libglu1-mesa-dev
   ```

2. Build the engine:

   ```bash
   make
   ```

## Running

```bash
./star-engine
```

Use WASD to move and the mouse to look around. Press Q to quit.

## Controls

- `W` / `S`: Move forward / backward
- `A` / `D`: Strafe left / right
- `Space`: Jump (jump mode) or ascend thruster (thrust mode)
- `C`: Descend when in thrust mode
- `T`: Toggle between jump gravity mode and continuous thrust mode
- `Q`: Quit the application

## Architecture

- `src/main.cpp`: Entry point
- `src/MainLoop.cpp`: Game loop and timing
- `src/Viewport3D.cpp`: Window and rendering management
- `src/Input.cpp`: Input handling
- `src/Simulation.cpp`: Game logic and entities
- `src/ResourceManager.cpp`: Asset loading
- `src/ecs/`: Entity-component system

## Migration from SDL

This engine was previously using SDL2 for windowing and input. It has been migrated to GLFW for better 3D graphics performance and cross-platform compatibility. The build system automatically detects GLFW and compiles with OpenGL support.
