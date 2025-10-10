# Star Engine - TODO & Improvements

## Critical Issues Fixed (2025-10-10)

### ✅ Completed
1. **SDL Dependencies Removed** - Eliminated SDL2 and SDL_mixer from build system
2. **Headless Mode Implemented** - Engine can now run without graphics for testing
3. **XInput Crash Fixed** - Disabled problematic XInput preloading that caused crashes
4. **Camera Direction Fixed** - Corrected initial camera yaw from 0 to π/2 (90°) to face player
5. **Frame Limiter Added** - Headless mode now has configurable frame limits

---

## Graphics System Issues

### High Priority
- [ ] **Fix GLFW Initialization Crash**
  - `glfwInit()` crashes on this system
  - Investigate GPU drivers, Windows version compatibility
  - Consider alternative: Try older GLFW version (3.3.x instead of 3.4)
  - Alternative: Try GLEW + native Windows OpenGL context creation

- [ ] **Add Better Error Handling for Graphics Initialization**
  - Catch crashes during `LoadLibraryW` calls
  - Add GPU capability detection before GLFW init
  - Provide informative error messages instead of silent crashes

- [ ] **Test Graphics on Different System**
  - Current system has fundamental OpenGL/GLFW compatibility issues
  - May need different GPU driver version or Windows update

### Medium Priority
- [ ] **Improve Headless Mode**
  - Add visual output options (ASCII art improvements, log file rendering)
  - Add headless performance profiling
  - Create automated test suite using headless mode

- [ ] **Graphics Fallback System**
  - Implement software rendering fallback if OpenGL fails
  - Add 2D-only mode for testing without 3D acceleration

---

## Build System Improvements

### High Priority
- [ ] **Missing DLL Detection**
  - Create pre-run script to check for required DLLs
  - Auto-copy missing DLLs from MSYS2 to project directory
  - List required DLLs in README: `glfw3.dll`, `libfreeglut.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `xinput1_4.dll`

- [ ] **Fix Clean Target in Makefile**
  - Currently uses Unix `rm` command which fails on Windows
  - Add PowerShell-compatible clean or use cross-platform approach

### Medium Priority
- [ ] **Add Build Configuration Options**
  - Debug vs Release builds
  - Optional features (audio, networking, etc.)
  - Profile-guided optimization

- [ ] **Dependency Management**
  - Document all external dependencies clearly
  - Add version requirements for each library
  - Consider using vcpkg or Conan for dependency management

---

## Camera System Improvements

### Completed
- [x] **Fix Initial Camera Direction** - Now looking at player instead of wrong direction

### High Priority
- [ ] **Camera Presets Testing**
  - Verify all camera presets work correctly with new yaw angle
  - Test camera transitions and interpolation
  - Ensure mouse look sensitivity is appropriate

### Medium Priority
- [ ] **Camera System Enhancements**
  - Add camera shake for impacts/explosions
  - Implement cinematic camera paths
  - Add camera collision detection (prevent clipping through objects)

---

## Input System Improvements

### High Priority
- [ ] **XInput Integration Fix**
  - Currently disabled due to crashes
  - Investigate why `LoadLibraryW(L"XInput1_4.dll")` crashes
  - Consider dynamic loading with better error handling
  - Add gamepad support gracefully without crashing

- [ ] **Input in Headless Mode**
  - Add scripted input for automated testing
  - Create input replay system for debugging
  - Add command-line input injection

### Medium Priority
- [ ] **Input Mapping System**
  - User-configurable key bindings
  - Support for multiple input devices
  - Input profiles for different play styles

---

## Testing & Quality Assurance

### High Priority
- [ ] **Automated Testing Suite**
  - Use headless mode for unit tests
  - Test all game systems without graphics
  - Add CI/CD pipeline using headless mode

- [ ] **Performance Testing**
  - Profile headless mode performance
  - Identify bottlenecks in game logic
  - Optimize fixed-timestep loop

### Medium Priority
- [ ] **Error Logging Improvements**
  - More detailed crash reports
  - Add stack traces on crashes
  - Log GPU/driver information on startup

---

## Documentation Needed

### High Priority
- [ ] **Setup Guide for Windows**
  - Complete DLL requirements list
  - MSYS2 setup instructions
  - Troubleshooting common issues (like GLFW crashes)

- [ ] **Headless Mode Documentation**
  - Environment variables: `STAR_ENGINE_HEADLESS`, `STAR_ENGINE_MAX_FRAMES`
  - Use cases for headless mode
  - Examples of automated testing

### Medium Priority
- [ ] **Architecture Documentation**
  - System overview diagrams
  - Component interaction documentation
  - Data flow diagrams

- [ ] **API Documentation**
  - Document all public interfaces
  - Add usage examples
  - Document design decisions

---

## Code Quality Improvements

### Medium Priority
- [ ] **Remove Unused Code**
  - Clean up commented-out SDL fallback code in Viewport3D
  - Remove unused includes
  - Delete obsolete log files and test outputs

- [ ] **Warning Cleanup**
  - Fix unused parameter warnings in Viewport3D
  - Fix overflow warning in MainLoop (GLFW_KEY_UP conversion)
  - Fix unused variable warnings

- [ ] **Code Organization**
  - Split large files (Viewport3D.cpp is 2295 lines)
  - Separate rendering backends into distinct modules
  - Better separation of concerns

---

## Future Features

### Low Priority
- [ ] **Multiple Graphics Backends**
  - Vulkan support
  - DirectX support for Windows
  - Metal support for macOS

- [ ] **Network Multiplayer**
  - Add network stack
  - Client-server architecture
  - Synchronization and prediction

- [ ] **Audio System Restoration**
  - Re-add audio without SDL_mixer dependency
  - Consider alternatives: miniaudio, OpenAL, FMOD
  - Implement without external audio library crashes

---

## Known Issues

### Critical
1. **GLFW crashes during initialization** - Blocks graphics mode on current system
2. **XInput loading crashes** - Disabled, gamepad support unavailable
3. **Exit code 1 on clean shutdown** - Headless mode exits with error despite successful run

### Minor
1. **Shell command errors in output** - "The system cannot find the path specified", "'true' is not recognized"
2. **ASCII rendering verbose** - Player position printed every frame floods console
3. **FPS display only at end** - Should show periodically during run

---

## Environment Variables Added

```powershell
# Enable headless mode (no graphics)
$env:STAR_ENGINE_HEADLESS="1"

# Set frame limit for headless mode (default: 300)
$env:STAR_ENGINE_MAX_FRAMES="60"

# Clear headless mode
Remove-Item Env:\STAR_ENGINE_HEADLESS
Remove-Item Env:\STAR_ENGINE_MAX_FRAMES
```

---

## Notes

- Camera now positioned at `(-8, 0, 6)` with yaw `π/2` looking toward player at origin
- Headless mode successfully runs at 60 FPS with all game systems active
- SDL completely removed from build - no longer a dependency
- All game logic (ECS, physics, simulation) working correctly in headless mode
