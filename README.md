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

Nova-Engine relies on a GPU/driver stack that supports at least the OpenGL 4.3
Core Profile (4.6 recommended) and the following libraries:

- GLFW 3.x (windowing, input)
- OpenGL / GLU
- GLAD (core OpenGL function loader, bundled in `lib/glad/`)
- MinGW-w64 toolchain (on Windows)

Keeping GPU drivers up to date is strongly advised—outdated drivers frequently
report support for legacy OpenGL 2.x contexts only, which prevents GLFW from
initializing modern rendering paths.

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

3. Before building or launching on Windows, verify that all required runtime
   DLLs are available in the repository root:

   ```powershell
   pwsh -File scripts/check_dlls.ps1
   ```

   The script copies missing DLLs from your MSYS2 installation when possible
   and surfaces actionable errors for unresolved dependencies.

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
scripts\\run_engine.bat
```

Use WASD to move and the mouse to look around. Press Q to quit.

## Controls

### Movement

- `W` / `S`: Move forward / backward
- `A` / `D`: Strafe left / right
- `Space`: Ascend (thrust mode) or jump (gravity mode)
- `C`: Descend (thrust mode)

### Camera & View

- **Mouse**: Look around (pitch/yaw)
- **Mouse Wheel**: Zoom in/out
- **Arrow Keys + Space**: Move camera up/down/left/right
- `Z` / `X`: Zoom in/out (alternative to mouse wheel)
- `1` / `2` / `3`: Apply camera preset positions

### Gameplay

- `T`: Toggle between jump gravity mode and continuous thrust mode
- `Tab`: Toggle target lock mode (mouse controls targeting reticle)
- `P`: Pause/unpause simulation

### Visual Effects

- `B`: Toggle bloom post-processing effect
- `L`: Toggle letterbox HUD overlay

### Development & Debug

- `I`: Toggle ECS inspector (entity component system debugger)
- `[` / `]`: Cycle through ECS inspector filters
- `0`: Clear ECS inspector filter
- `F8`: Toggle world coordinate axes (debug only)
- `F9`: Toggle mini axes gizmo (debug only)
- `F11`: Toggle fullscreen mode

### System

- `Q` / `Esc`: Quit the application

Additional experimental controls may exist in `engine/Input.cpp`. The HUD displays control hints for the first 5 seconds of gameplay.

## Architecture highlights

- `engine/main.cpp`: Entry point
- `engine/MainLoop.cpp`: Game loop and timing
- `engine/Viewport3D.cpp`: Window and rendering management
- `engine/Input.cpp`: Input handling
- `engine/Simulation.cpp`: Game logic and entities
- `engine/ResourceManager.cpp`: Asset loading
- `engine/ecs/`: Entity-component system
- `engine/graphics/`: Rendering pipeline, post-processing, and text systems

## Repository layout

The repository root concentrates on build scripts, source code, and high-level
documentation. Supporting assets are grouped into dedicated folders:

- `artifacts/`: Archived screenshots, capture dumps, logs, and other diagnostics
- `assets/`: Game-ready assets that are loaded at runtime
- `docs/`: Design notes, guides, and background documentation. Historical to-do
  notes live in `docs/todo/`
- `scripts/`: Helper scripts for running builds, tests, and asset pipelines
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
