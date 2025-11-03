#pragma once
// Precompiled header for Nova Engine
// Include frequently used system headers to speed up compilation

// Standard library headers
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>

// Math headers
#include <cmath>
#include <random>

// OpenGL headers (but not GLU due to callback issues)
#ifdef USE_GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Note: GLU excluded from PCH due to Windows callback definition conflicts
#endif

// ImGui headers
#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

// Windows specific (minimal set)
#ifdef _WIN32
#include <windows.h>
#endif