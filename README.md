# Nova-Engine

A 3D game engine built with C++17, OpenGL, and GLFW. The project focuses on
experimentation with physically inspired flight controls, modular spacecraft
assembly, and a data-driven entity component system. It currently targets
desktop platforms and ships with a portable MinGW toolchain for Windows users.

## Features

- 3D rendering with OpenGL immediate mode
- Entity-component system (ECS)
- Camera system with mouse look
- Input handling (keyboard and mouse)
- Resource management
- Fixed-timestep game loop

## Prerequisites

Nova-Engine relies on an OpenGL 3.x capable GPU and the following libraries:

- GLFW 3.x (windowing, input)
- OpenGL / GLU
- GLAD (core OpenGL function loader, bundled in `lib/glad/`)
- MinGW-w64 toolchain (on Windows)

> **Tip:** Pre-built DLLs for Windows are tracked in the repository under the
> `lib/` directory. They are primarily meant for local testing and continuous
> integration artifacts.

## Getting started

### Clone the repository

```bash
git clone https://github.com/<your-account>/Nova-Engine.git
cd Nova-Engine
```

### Install system dependencies

#### Windows (MSYS2/MinGW)

1. Install MSYS2 from [msys2.org](https://www.msys2.org/).
2. Open the *MSYS2 MinGW 64-bit* shell and install packages:

   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-glu
   ```

#### Linux

```bash
sudo apt-get update
sudo apt-get install build-essential libglfw3-dev libglu1-mesa-dev
```

### Build

Run `make` from the repository root. The Makefile auto-detects GLFW via
`pkg-config` and falls back to bundled DLLs when cross-compiling on Windows.

```bash
make
```

Artifacts (executables, tests, cached objects) are written alongside the source
tree, so running `make clean` resets the build output.

## Running

Launch the executable directly or use the helper script on Windows:

```bash
# Linux / macOS
./nova-engine

# Windows
run_engine.bat
```

Use WASD to move and the mouse to look around. Press Q to quit.

## Controls

- `W` / `S`: Move forward / backward
- `A` / `D`: Strafe left / right
- `Space`: Jump (jump mode) or ascend thruster (thrust mode)
- `C`: Descend when in thrust mode
- `T`: Toggle between jump gravity mode and continuous thrust mode
- `Q`: Quit the application

Additional debug bindings exist in `src/Input.cpp`. Check the source for the
latest experimental controls and gameplay toggles.

## Architecture highlights

- `src/main.cpp`: Entry point
- `src/MainLoop.cpp`: Game loop and timing
- `src/Viewport3D.cpp`: Window and rendering management
- `src/Input.cpp`: Input handling
- `src/Simulation.cpp`: Game logic and entities
- `src/ResourceManager.cpp`: Asset loading
- `src/ecs/`: Entity-component system
- `src/graphics/`: Rendering pipeline, post-processing, and text systems

## Repository layout

The repository root concentrates on build scripts, source code, and high-level
documentation. Supporting assets are grouped into dedicated folders:

- `artifacts/`: Archived screenshots, capture dumps, logs, and other diagnostics
- `assets/`: Game-ready assets that are loaded at runtime
- `docs/`: Design notes, guides, and background documentation. Historical to-do
  notes live in `docs/todo/`
- `lib/`: Bundled Windows runtime DLLs for convenience when running locally
- `tests/`: Automated and manual test sources and binaries

## Testing

The repository includes a suite of lightweight simulations that exercise camera
logic, ECS behavior, and rendering utilities. Build all tests with:

```bash
make test
```

Each target is emitted to `tests/<test_name>` and can be launched individually.
Use `make clean` to remove generated binaries.

## Migration from SDL

This engine previously used SDL2 for windowing and input. It has been migrated
to GLFW for better 3D graphics performance and cross-platform compatibility. The
build system automatically detects GLFW and compiles with OpenGL support when
available.
