# ImGui Integration for Nova Engine

This directory contains the ImGui (Dear ImGui) library integration for Nova Engine's visual configuration editor.

## Setup

To set up ImGui for development:

```powershell
# Run the setup script
.\scripts\setup_imgui.ps1

# Or manually specify a version
.\scripts\setup_imgui.ps1 -Version "v1.90.4"
```

## Structure

```
lib/imgui/
├── include/           # ImGui header files
│   ├── imgui.h       # Core ImGui header
│   ├── imconfig.h    # Configuration
│   ├── imgui_impl_glfw.h    # GLFW backend
│   ├── imgui_impl_opengl3.h # OpenGL3 backend
│   └── imgui_stdlib.h       # C++ std::string support
├── src/              # ImGui source files
│   ├── imgui.cpp     # Core implementation
│   ├── imgui_demo.cpp        # Demo window
│   ├── imgui_draw.cpp        # Drawing/rendering
│   ├── imgui_tables.cpp      # Tables widget
│   ├── imgui_widgets.cpp     # Standard widgets
│   ├── imgui_impl_glfw.cpp   # GLFW backend
│   ├── imgui_impl_opengl3.cpp # OpenGL3 backend
│   └── imgui_stdlib.cpp      # C++ std::string support
└── VERSION           # Version information
```

## Build Integration

The Makefile automatically detects ImGui availability:

- **ImGui Available**: Builds with `-DUSE_IMGUI` flag, includes ImGui objects
- **ImGui Missing**: Builds without ImGui, ConfigEditor UI uses stub implementations

## Usage in Code

```cpp
#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

// Full ImGui functionality available
#else
// Stub implementations for builds without ImGui
#endif
```

## Dependencies

ImGui requires:
- **GLFW 3.x**: For windowing and input (already available in Nova Engine)
- **OpenGL 3.0+**: For rendering (already available in Nova Engine)
- **C++11 or later**: For compilation (Nova Engine uses C++17)

## Features Used

Nova Engine's config editor uses these ImGui features:

- **Core Widgets**: InputText, InputFloat, Checkbox, Combo, etc.
- **Layout**: Windows, child regions, columns, tables
- **Styling**: Colors, fonts, themes
- **Advanced**: Custom widgets, drag & drop, tooltips
- **Backends**: GLFW for input, OpenGL3 for rendering

## Performance

ImGui is designed for:
- **Immediate Mode**: Rebuilds UI every frame
- **Low Overhead**: Minimal state, fast rendering
- **Debug UI**: Perfect for editor tools and debug interfaces

Nova Engine's config editor is designed to be used during development and configuration, not in the main game loop, so the immediate mode approach is ideal.

## Customization

To customize ImGui for Nova Engine:

1. **Styling**: Modify theme colors in `ConfigEditorImGuiUI::ApplyTheme()`
2. **Fonts**: Add custom fonts in `ConfigEditorImGuiUI::SetupFonts()`
3. **Widgets**: Create custom widgets in the UI implementation

## Troubleshooting

### ImGui Not Found
```
ImGui not found; building without ImGui UI support
```
**Solution**: Run `.\scripts\setup_imgui.ps1` to download and install ImGui.

### Compilation Errors
- Ensure GLFW is properly installed
- Check that OpenGL headers are available
- Verify C++17 support in compiler

### Runtime Issues
- Ensure OpenGL context is created before ImGui initialization
- Check that GLFW is properly initialized
- Verify vertex/fragment shaders are available for OpenGL3 backend

## License

ImGui is licensed under the MIT License. See the ImGui repository for full license details:
https://github.com/ocornut/imgui

Nova Engine's ImGui integration is part of Nova Engine's license.