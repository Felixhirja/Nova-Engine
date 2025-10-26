# GLFW Free-Fly Camera Demo

The standalone sample in `test_glfw_camera.cpp` opens a 1280×720 window, enables
VSync and depth testing, and renders a single Lambert-lit cube with a
first-person camera. Controls match the engine conventions:

- Mouse look (cursor captured) for yaw / pitch; clamp pitch to ±89°
- `W`/`S` forward / backward relative to yaw
- `A`/`D` strafe
- `Space` ascend, `LeftCtrl` descend
- Mouse wheel adjusts FOV in the `[30°, 90°]` range
- `Tab` toggles cursor capture, `Esc` closes the window

The code uses GLFW, GLAD (core OpenGL loader), and GLM. GLAD sources are already
vendored under `lib/glad/`, but you can regenerate them if you prefer different
options.

## Dependencies

- [GLFW 3.x](https://www.glfw.org/)
- [GLM](https://github.com/g-truc/glm)
- OpenGL 3.3 core profile capable driver
- GLAD 2.0+ code generator (only required if you want to regenerate the loader)

## Fetching GLAD (optional)

The repository ships with GLAD output targeting OpenGL core 4.6
(`lib/glad/src/glad.c` and `lib/glad/include/glad/glad.h`). If you want to
refresh or customize the loader:

```powershell
# Install the generator
python -m pip install glad2

# Rebuild the loader in-place (adjust the API/profile if needed)
glad --out-path lib/glad --api gl=4.6 --profile core --generator c
```

The command above overwrites the existing loader sources with fresh output.

## Build & run (Windows, MSVC)

The example assumes you have GLFW and GLM installed (for example via
[vcpkg](https://github.com/microsoft/vcpkg) or the precompiled binaries from
GLFW). Set the include/lib paths accordingly.

```powershell
# From a "x64 Native Tools Command Prompt for VS 2022"
cd C:\Users\felix\Documents\GitHub\Nova-Engine

# Compile the GLAD loader once (produces glad.obj)
cl /nologo /c /Ilib\glad\include lib\glad\src\glad.c

# Build the demo (adjust include/lib directories to match your GLFW + GLM install)
cl test_glfw_camera.cpp glad.obj ^
    /EHsc /std:c++17 /MD ^
    /Ilib\glad\include ^
    /I"C:\path\to\glm" ^
    /I"C:\path\to\glfw\include" ^
    /link ^
    /LIBPATH:"C:\path\to\glfw\lib-vc2022" glfw3.lib opengl32.lib

# Launch
.\nGLFW_CAMERA.EXE
```

> **Tip:** If you installed GLFW via vcpkg, replace the include and library
> paths above with the directories under `%VCPKG_ROOT%\installed\x64-windows`. Run
> `vcpkg integrate install` to add those paths to MSVC automatically.

## Build & run (Linux, g++)

Make sure the GLFW, GLM, and OpenGL development packages are installed. On
Ubuntu/Debian derivatives:

```bash
sudo apt-get install libglfw3-dev libglm-dev
```

Then build and run the sample:

```bash
cd ~/Documents/GitHub/Nova-Engine

g++ -std=c++17 test_glfw_camera.cpp lib/glad/src/glad.c \
    -Ilib/glad/include \
    -lglfw -ldl -lGL -lpthread -o glfw_camera_demo

./glfw_camera_demo
```

If your distribution splits GLM headers into a non-default include path, append
`-I/path/to/glm` to the compile command.
