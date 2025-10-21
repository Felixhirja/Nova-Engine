# Nova-Engine Modding System Design

**Document Version:** 1.0  
**Date:** October 10, 2025  
**Status:** Design Phase

## Executive Summary

This document outlines a comprehensive modding system for Nova-Engine that will enable community-created content including custom ships, solar systems, missions, and gameplay modifications. The system is designed to be:

- **Safe**: Sandboxed scripting with controlled API access
- **Performant**: Hot-reloading without full engine restarts
- **Extensible**: Plugin architecture supporting multiple languages
- **User-friendly**: Visual tools for non-programmers
- **Discoverable**: Built-in mod browser and workshop integration

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Plugin System](#plugin-system)
3. [Scripting System](#scripting-system)
4. [Asset Hot-Reloading](#asset-hot-reloading)
5. [Custom Entity Creation Tools](#custom-entity-creation-tools)
6. [Level Editor](#level-editor)
7. [Implementation Roadmap](#implementation-roadmap)
8. [Technical Specifications](#technical-specifications)

---

## Architecture Overview

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Nova-Engine Core                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────┐  ┌──────────────┐  ┌─────────────────┐│
│  │ Plugin Manager │  │ Script Engine│  │ Asset Watcher   ││
│  │                │  │              │  │                 ││
│  │ • Load/Unload  │  │ • Lua VM     │  │ • File Monitor  ││
│  │ • Versioning   │  │ • Sandboxing │  │ • Auto-Reload   ││
│  │ • Dependencies │  │ • Hot-Reload │  │ • Validation    ││
│  └────────────────┘  └──────────────┘  └─────────────────┘│
│                                                              │
│  ┌────────────────────────────────────────────────────────┐│
│  │           Modding API (C++ ↔ Scripts)                  ││
│  │                                                          ││
│  │  • Entity Creation/Manipulation                         ││
│  │  • Component Registration                               ││
│  │  • System Hooks                                         ││
│  │  • Event System                                         ││
│  │  • Resource Management                                  ││
│  └────────────────────────────────────────────────────────┘│
│                                                              │
│  ┌──────────────┐  ┌────────────────┐  ┌────────────────┐ │
│  │ Visual Tools │  │ Level Editor   │  │ Ship Designer  │ │
│  │              │  │                │  │                │ │
│  │ • Entity Ed. │  │ • 3D Viewport  │  │ • Module Grid  │ │
│  │ • Property   │  │ • Brush Tools  │  │ • Stats View   │ │
│  │   Inspector  │  │ • Prefabs      │  │ • Test Flight  │ │
│  └──────────────┘  └────────────────┘  └────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

```
Mod Package (.starmod)
        ↓
  Plugin Manager
        ↓
    ┌───┴───┐
    ↓       ↓
Scripts   Assets
    ↓       ↓
  Lua VM  Resource Manager
    ↓       ↓
 ECS V2   Rendering
    ↓       ↓
  Gameplay Output
```

---

## 1. Plugin System

### 1.1 Plugin Architecture

**Purpose:** Load external code modules (DLLs/SOs) that extend engine functionality without recompiling.

**Design Pattern:** Interface-based with version checking

```cpp
// src/modding/IPlugin.h
#pragma once
#include <string>
#include <vector>

namespace NovaEngine {
namespace Modding {

// Plugin ABI version for compatibility checking
#define STAR_PLUGIN_ABI_VERSION 1

enum class PluginType {
    Gameplay,      // New game mechanics
    Graphics,      // Rendering enhancements
    Audio,         // Sound/music systems
    Tools,         // Editor extensions
    Content        // Ships, missions, assets
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    
    // Dependencies (other plugin names)
    std::vector<std::string> dependencies;
    
    // Minimum engine version required
    std::string minEngineVersion;
};

class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin lifecycle
    virtual bool OnLoad() = 0;
    virtual void OnUnload() = 0;
    virtual void OnEnable() = 0;
    virtual void OnDisable() = 0;
    
    // Metadata
    virtual PluginInfo GetInfo() const = 0;
    virtual int GetABIVersion() const { return STAR_PLUGIN_ABI_VERSION; }
    
    // Update hooks
    virtual void OnUpdate(double deltaTime) {}
    virtual void OnFixedUpdate(double fixedDeltaTime) {}
    virtual void OnRender() {}
};

// Plugin entry point (must be exported by plugin DLL)
typedef IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IPlugin*);

#define STAR_PLUGIN_EXPORT extern "C" __declspec(dllexport)

#define STAR_DECLARE_PLUGIN(PluginClass) \
    STAR_PLUGIN_EXPORT IPlugin* CreatePlugin() { \
        return new PluginClass(); \
    } \
    STAR_PLUGIN_EXPORT void DestroyPlugin(IPlugin* plugin) { \
        delete plugin; \
    }

} // namespace Modding
} // namespace NovaEngine
```

### 1.2 Plugin Manager

```cpp
// src/modding/PluginManager.h
#pragma once
#include "IPlugin.h"
#include <unordered_map>
#include <memory>

namespace NovaEngine {
namespace Modding {

class PluginManager {
public:
    static PluginManager& Instance();
    
    // Plugin loading
    bool LoadPlugin(const std::string& path);
    void UnloadPlugin(const std::string& name);
    void UnloadAllPlugins();
    
    // Plugin management
    bool EnablePlugin(const std::string& name);
    bool DisablePlugin(const std::string& name);
    bool IsPluginLoaded(const std::string& name) const;
    
    // Query plugins
    std::vector<std::string> GetLoadedPlugins() const;
    const PluginInfo* GetPluginInfo(const std::string& name) const;
    
    // Update hooks (called by MainLoop)
    void UpdateAllPlugins(double deltaTime);
    void FixedUpdateAllPlugins(double fixedDeltaTime);
    void RenderAllPlugins();
    
private:
    PluginManager() = default;
    
    struct PluginHandle {
        IPlugin* instance;
        void* libraryHandle;  // HMODULE on Windows, void* on Unix
        PluginInfo info;
        bool enabled;
        
        CreatePluginFunc createFunc;
        DestroyPluginFunc destroyFunc;
    };
    
    std::unordered_map<std::string, std::unique_ptr<PluginHandle>> plugins_;
    
    // Dependency resolution
    bool CheckDependencies(const PluginInfo& info) const;
    std::vector<std::string> ResolveLoadOrder(const std::vector<std::string>& pluginNames);
};

} // namespace Modding
} // namespace NovaEngine
```

**Key Features:**
- **Dynamic Loading**: Use `LoadLibrary`/`dlopen` to load plugins at runtime
- **Version Checking**: Verify ABI compatibility before loading
- **Dependency Resolution**: Ensure plugins load in correct order
- **Safe Unloading**: Proper cleanup to prevent memory leaks
- **Hot-Reloading**: Unload/reload plugins without engine restart

**Example Plugin:**

```cpp
// mods/example_plugin/ExamplePlugin.cpp
#include "modding/IPlugin.h"
#include <iostream>

using namespace NovaEngine::Modding;

class ExamplePlugin : public IPlugin {
public:
    bool OnLoad() override {
        std::cout << "ExamplePlugin: Loading..." << std::endl;
        return true;
    }
    
    void OnUnload() override {
        std::cout << "ExamplePlugin: Unloading..." << std::endl;
    }
    
    void OnEnable() override {
        std::cout << "ExamplePlugin: Enabled" << std::endl;
    }
    
    void OnDisable() override {
        std::cout << "ExamplePlugin: Disabled" << std::endl;
    }
    
    PluginInfo GetInfo() const override {
        return {
            "Example Plugin",
            "1.0.0",
            "Nova-Engine Team",
            "Demonstrates plugin system",
            PluginType::Gameplay,
            {},  // No dependencies
            "1.0.0"  // Min engine version
        };
    }
    
    void OnUpdate(double deltaTime) override {
        // Custom game logic here
    }
};

STAR_DECLARE_PLUGIN(ExamplePlugin)
```

---

## 2. Scripting System

### 2.1 Lua Integration

**Why Lua?**
- **Lightweight**: Small footprint (~200KB)
- **Fast**: LuaJIT provides near-native performance
- **Embeddable**: Designed for game engines
- **Sandboxable**: Easy to restrict dangerous operations
- **Popular**: Large modding community familiar with Lua

**Alternative:** Python (heavier but more features) or C# (Mono runtime)

### 2.2 Script Manager

```cpp
// src/modding/ScriptManager.h
#pragma once
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>
#include <unordered_map>
#include <functional>

namespace NovaEngine {
namespace Modding {

class ScriptManager {
public:
    static ScriptManager& Instance();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Script execution
    bool LoadScript(const std::string& path);
    bool ExecuteScript(const std::string& code);
    bool CallFunction(const std::string& functionName, int numArgs = 0);
    
    // Hot-reloading
    void ReloadScript(const std::string& path);
    void ReloadAllScripts();
    
    // Sandbox control
    void EnableSandbox(bool enable);
    void AllowFileAccess(bool allow);
    void AllowNetworkAccess(bool allow);
    
    // Lua state access
    lua_State* GetLuaState() { return L_; }
    
    // Register C++ functions to Lua
    void RegisterFunction(const std::string& name, lua_CFunction func);
    void RegisterClass(const std::string& className);
    
private:
    ScriptManager() = default;
    ~ScriptManager() { Shutdown(); }
    
    lua_State* L_ = nullptr;
    std::unordered_map<std::string, std::string> loadedScripts_;
    
    // Sandboxing
    void SetupSandbox();
    void RegisterEngineFunctions();
};

} // namespace Modding
} // namespace NovaEngine
```

### 2.3 Lua API Bindings

**Core Engine API exposed to Lua:**

```lua
-- mods/example_mod/main.lua

-- Entity creation
local player = Engine.CreateEntity("player_ship")
Engine.AddComponent(player, "Position", {x = 0, y = 0, z = 0})
Engine.AddComponent(player, "Velocity", {x = 0, y = 0, z = 0})

-- Component access
local pos = Engine.GetComponent(player, "Position")
print("Player position:", pos.x, pos.y, pos.z)

-- System hooks
function OnUpdate(deltaTime)
    -- Update player velocity
    local vel = Engine.GetComponent(player, "Velocity")
    if Input.IsKeyDown("W") then
        vel.z = vel.z - 10 * deltaTime
    end
end

-- Event handlers
Engine.RegisterEventHandler("OnEntityCollision", function(entity1, entity2)
    print("Collision between", entity1, "and", entity2)
end)

-- Resource loading
local shipTexture = Engine.LoadTexture("mods/my_mod/assets/ship.png")
Engine.SetTexture(player, shipTexture)

-- Custom ship components
Engine.RegisterComponent("CustomWeapon", {
    damage = 100,
    fireRate = 2.0,
    ammo = 50
})
```

**Implementation using sol2 (C++ Lua binding library):**

```cpp
// src/modding/LuaBindings.cpp
#include "ScriptManager.h"
#include "ecs/EntityManagerV2.h"
#include <sol/sol.hpp>

void RegisterEngineFunctions() {
    sol::state& lua = *reinterpret_cast<sol::state*>(
        ScriptManager::Instance().GetLuaState()
    );
    
    // Entity management
    lua["Engine"] = lua.create_table();
    lua["Engine"]["CreateEntity"] = [](const std::string& name) {
        return EntityManagerV2::Instance().CreateEntity();
    };
    
    lua["Engine"]["DestroyEntity"] = [](EntityHandle entity) {
        EntityManagerV2::Instance().DestroyEntity(entity);
    };
    
    // Component management
    lua["Engine"]["AddComponent"] = [](EntityHandle entity, 
                                       const std::string& componentType,
                                       sol::table data) {
        // Parse component from Lua table
        // Add to entity
    };
    
    lua["Engine"]["GetComponent"] = [](EntityHandle entity,
                                       const std::string& componentType) {
        // Return component as Lua table
    };
    
    // Input bindings
    lua["Input"] = lua.create_table();
    lua["Input"]["IsKeyDown"] = [](const std::string& key) {
        return Input::IsKeyDown(key);
    };
    
    // Resource management
    lua["Engine"]["LoadTexture"] = [](const std::string& path) {
        return ResourceManager::Instance().Load(path);
    };
}
```

### 2.4 Script Sandboxing

**Security Restrictions:**

```cpp
void SetupSandbox() {
    // Disable dangerous functions
    const char* dangerousFunctions[] = {
        "os.execute",
        "os.remove",
        "os.rename",
        "io.popen",
        "loadfile",
        "dofile",
        nullptr
    };
    
    for (int i = 0; dangerousFunctions[i]; ++i) {
        lua_pushnil(L_);
        lua_setglobal(L_, dangerousFunctions[i]);
    }
    
    // Restrict file I/O to mod directory
    // Limit memory usage (lua_setallocf)
    // Timeout long-running scripts
}
```

---

## 3. Asset Hot-Reloading

### 3.1 File System Watcher

```cpp
// src/modding/AssetWatcher.h
#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>

namespace NovaEngine {
namespace Modding {

enum class AssetType {
    Texture,
    Model,
    Script,
    Audio,
    Config,
    Unknown
};

struct AssetChangeEvent {
    std::string path;
    AssetType type;
    enum { Added, Modified, Removed } action;
};

using AssetChangeCallback = std::function<void(const AssetChangeEvent&)>;

class AssetWatcher {
public:
    static AssetWatcher& Instance();
    
    // Directory monitoring
    void WatchDirectory(const std::string& path, bool recursive = true);
    void UnwatchDirectory(const std::string& path);
    
    // Start/stop watching
    void Start();
    void Stop();
    bool IsWatching() const { return watching_; }
    
    // Callbacks
    void RegisterCallback(AssetType type, AssetChangeCallback callback);
    void UnregisterCallback(AssetType type);
    
private:
    AssetWatcher() = default;
    ~AssetWatcher() { Stop(); }
    
    void WatchThreadFunc();
    AssetType DetectAssetType(const std::string& path) const;
    
    std::atomic<bool> watching_{false};
    std::thread watchThread_;
    
    std::vector<std::string> watchedDirs_;
    std::unordered_map<AssetType, std::vector<AssetChangeCallback>> callbacks_;
    
#ifdef _WIN32
    void* directoryHandle_;  // HANDLE for ReadDirectoryChangesW
#else
    int inotifyFd_;  // inotify file descriptor on Linux
#endif
};

} // namespace Modding
} // namespace NovaEngine
```

### 3.2 Hot-Reload Implementation

**Texture Hot-Reload:**

```cpp
void ResourceManager::ReloadTexture(int handle) {
    std::string path = GetPathForHandle(handle);
    
    // Unload old texture
    if (textures_.find(handle) != textures_.end()) {
        glDeleteTextures(1, &textures_[handle]);
    }
    
    // Load new texture
    LoadTextureFromFile(path, handle);
    
    // All existing references still valid (handle unchanged)
    std::cout << "Hot-reloaded texture: " << path << std::endl;
}
```

**Script Hot-Reload:**

```cpp
void ScriptManager::ReloadScript(const std::string& path) {
    // Save current state (if needed)
    // ...
    
    // Re-execute script file
    if (luaL_dofile(L_, path.c_str()) != LUA_OK) {
        const char* error = lua_tostring(L_, -1);
        std::cerr << "Script reload error: " << error << std::endl;
        lua_pop(L_, 1);
    } else {
        std::cout << "Hot-reloaded script: " << path << std::endl;
    }
}
```

**Model/Mesh Hot-Reload:**

```cpp
void MeshManager::ReloadMesh(const std::string& name) {
    // Rebuild GPU buffers
    auto& mesh = meshes_[name];
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ibo);
    
    // Reload from file
    LoadMeshFromFile(name);
}
```

### 3.3 Change Detection

```cpp
// Platform-specific file watching
#ifdef _WIN32
// Use ReadDirectoryChangesW
void WatchThreadFunc() {
    HANDLE hDir = CreateFile(
        watchDir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    
    while (watching_) {
        ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,  // Watch subtree
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL
        );
        
        // Process changes...
    }
}
#else
// Use inotify on Linux
void WatchThreadFunc() {
    int fd = inotify_init();
    int wd = inotify_add_watch(fd, watchDir.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    
    while (watching_) {
        char buffer[4096];
        int length = read(fd, buffer, sizeof(buffer));
        
        // Process events...
    }
}
#endif
```

---

## 4. Custom Entity Creation Tools

### 4.1 Entity Template System

```cpp
// src/modding/EntityTemplate.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "ecs/EntityManagerV2.h"

namespace NovaEngine {
namespace Modding {

struct ComponentTemplate {
    std::string type;
    std::unordered_map<std::string, std::string> properties;
};

struct EntityTemplate {
    std::string name;
    std::string description;
    std::string icon;
    
    std::vector<ComponentTemplate> components;
    std::vector<std::string> tags;
    
    // Metadata
    std::string author;
    std::string version;
};

class EntityTemplateManager {
public:
    static EntityTemplateManager& Instance();
    
    // Template management
    bool LoadTemplate(const std::string& path);
    bool SaveTemplate(const std::string& path, const EntityTemplate& templ);
    
    // Instantiation
    EntityHandle CreateFromTemplate(const std::string& templateName);
    
    // Query templates
    std::vector<std::string> GetAllTemplates() const;
    const EntityTemplate* GetTemplate(const std::string& name) const;
    
    // Template modification
    void RegisterTemplate(const std::string& name, const EntityTemplate& templ);
    void UnregisterTemplate(const std::string& name);
    
private:
    std::unordered_map<std::string, EntityTemplate> templates_;
};

} // namespace Modding
} // namespace NovaEngine
```

### 4.2 JSON Entity Templates

**Example Template Format:**

```json
// mods/ships/fighter_template.json
{
    "name": "Light Fighter",
    "description": "Fast, agile combat ship",
    "icon": "mods/ships/icons/fighter.png",
    "author": "Community Mod Pack",
    "version": "1.0.0",
    "components": [
        {
            "type": "Position",
            "properties": {
                "x": "0.0",
                "y": "0.0",
                "z": "0.0"
            }
        },
        {
            "type": "Velocity",
            "properties": {
                "x": "0.0",
                "y": "0.0",
                "z": "0.0"
            }
        },
        {
            "type": "ShipStats",
            "properties": {
                "maxSpeed": "150.0",
                "acceleration": "50.0",
                "hull": "100.0",
                "shields": "50.0"
            }
        },
        {
            "type": "MeshRenderer",
            "properties": {
                "mesh": "mods/ships/models/fighter.obj",
                "texture": "mods/ships/textures/fighter.png"
            }
        }
    ],
    "tags": ["ship", "fighter", "player_controllable"]
}
```

### 4.3 Visual Entity Editor

**ImGui-based Property Inspector:**

```cpp
// src/modding/EntityEditor.h
#pragma once
#include "imgui.h"
#include "EntityTemplate.h"

class EntityEditor {
public:
    void Render();
    
    // Template editing
    void SetEditingTemplate(EntityTemplate* templ);
    EntityTemplate* GetEditingTemplate() { return currentTemplate_; }
    
    // Component manipulation
    void AddComponent(const std::string& type);
    void RemoveComponent(int index);
    void EditComponentProperty(int componentIndex, 
                              const std::string& propertyName,
                              const std::string& value);
    
private:
    EntityTemplate* currentTemplate_ = nullptr;
    int selectedComponentIndex_ = -1;
    
    void RenderComponentList();
    void RenderPropertyInspector();
    void RenderAddComponentPopup();
};
```

**UI Layout:**

```
┌────────────────────────────────────────────────────┐
│  Entity Template Editor                            │
├────────────────┬───────────────────────────────────┤
│  Templates     │  Properties                       │
│                │                                   │
│  > Ships       │  Name: [Light Fighter         ]  │
│    • Fighter   │  Description: [Fast, agile... ]  │
│    • Bomber    │  Icon: [Browse...            ]  │
│    • Freighter │                                   │
│  > Stations    │  Components:                      │
│  > Planets     │  ┌─────────────────────────────┐ │
│  > Asteroids   │  │ ☑ Position                  │ │
│                │  │   x: [0.0  ]  y: [0.0  ]   │ │
│  [New Template]│  │   z: [0.0  ]                │ │
│  [Save]        │  │                              │ │
│  [Load]        │  │ ☑ Velocity                  │ │
│                │  │   x: [0.0  ]  y: [0.0  ]   │ │
│                │  │   z: [0.0  ]                │ │
│                │  │                              │ │
│                │  │ ☑ ShipStats                 │ │
│                │  │   maxSpeed: [150.0      ]   │ │
│                │  │   acceleration: [50.0   ]   │ │
│                │  │   hull: [100.0         ]   │ │
│                │  │   shields: [50.0       ]   │ │
│                │  │                              │ │
│                │  │ [+ Add Component ▼]         │ │
│                │  └─────────────────────────────┘ │
│                │                                   │
│                │  [Create Instance] [Apply Changes]│
└────────────────┴───────────────────────────────────┘
```

---

## 5. Level Editor

### 5.1 Editor Architecture

```cpp
// src/modding/LevelEditor.h
#pragma once
#include "Camera.h"
#include "ecs/EntityManagerV2.h"
#include <vector>

namespace NovaEngine {
namespace Modding {

enum class EditorMode {
    Select,
    Translate,
    Rotate,
    Scale,
    Paint
};

enum class GridSnapping {
    None,
    Quarter,  // 0.25 units
    Half,     // 0.5 units
    One,      // 1.0 units
    Five,     // 5.0 units
    Ten       // 10.0 units
};

class LevelEditor {
public:
    LevelEditor();
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Update loop
    void Update(double deltaTime);
    void Render();
    
    // Level management
    bool NewLevel();
    bool LoadLevel(const std::string& path);
    bool SaveLevel(const std::string& path);
    
    // Entity manipulation
    void SelectEntity(EntityHandle entity);
    void DeselectAll();
    std::vector<EntityHandle> GetSelectedEntities() const;
    
    void TranslateSelection(float dx, float dy, float dz);
    void RotateSelection(float angleX, float angleY, float angleZ);
    void ScaleSelection(float sx, float sy, float sz);
    void DeleteSelection();
    void DuplicateSelection();
    
    // Placement
    EntityHandle PlaceEntity(const std::string& templateName, 
                            float x, float y, float z);
    
    // Editor state
    void SetMode(EditorMode mode) { mode_ = mode; }
    EditorMode GetMode() const { return mode_; }
    
    void SetGridSnapping(GridSnapping snap) { gridSnap_ = snap; }
    GridSnapping GetGridSnapping() const { return gridSnap_; }
    
    void SetShowGrid(bool show) { showGrid_ = show; }
    bool IsShowingGrid() const { return showGrid_; }
    
private:
    EditorMode mode_ = EditorMode::Select;
    GridSnapping gridSnap_ = GridSnapping::One;
    bool showGrid_ = true;
    
    std::vector<EntityHandle> selectedEntities_;
    Camera editorCamera_;
    
    // Gizmo rendering
    void RenderGizmos();
    void RenderGrid();
    void RenderSelectionOutlines();
    
    // Input handling
    void HandleMousePicking();
    void HandleKeyboardShortcuts();
    void HandleGizmoInteraction();
    
    // Snapping
    float SnapToGrid(float value) const;
};

} // namespace Modding
} // namespace NovaEngine
```

### 5.2 Level File Format

**JSON-based level format:**

```json
// levels/custom_battle.json
{
    "name": "Custom Battle Arena",
    "version": "1.0.0",
    "author": "PlayerName",
    "description": "PvP combat arena with asteroids",
    
    "settings": {
        "ambientLight": [0.2, 0.2, 0.3],
        "gravity": [0.0, 0.0, 0.0],
        "bounds": {
            "min": [-1000, -1000, -1000],
            "max": [1000, 1000, 1000]
        }
    },
    
    "entities": [
        {
            "template": "asteroid_large",
            "transform": {
                "position": [100, 0, -200],
                "rotation": [0, 45, 0],
                "scale": [2.0, 2.0, 2.0]
            },
            "components": {
                "Rigidbody": {
                    "mass": 1000.0,
                    "angularVelocity": [0.1, 0.2, 0.0]
                }
            }
        },
        {
            "template": "spawn_point",
            "transform": {
                "position": [-500, 0, 0],
                "rotation": [0, 0, 0],
                "scale": [1, 1, 1]
            },
            "tags": ["player_spawn", "team_red"]
        }
    ],
    
    "scripts": [
        "levels/custom_battle/game_mode.lua",
        "levels/custom_battle/respawn_logic.lua"
    ]
}
```

### 5.3 Editor UI

**Dockable panels using ImGui:**

```cpp
void LevelEditor::RenderUI() {
    ImGui::DockSpaceOverViewport();
    
    // Menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) NewLevel();
            if (ImGui::MenuItem("Open", "Ctrl+O")) OpenLevelDialog();
            if (ImGui::MenuItem("Save", "Ctrl+S")) SaveLevel(currentLevelPath_);
            if (ImGui::MenuItem("Save As...")) SaveLevelDialog();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) CloseEditor();
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) Redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) DuplicateSelection();
            if (ImGui::MenuItem("Delete", "Del")) DeleteSelection();
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Toolbar
    RenderToolbar();
    
    // Hierarchy panel (entity list)
    RenderHierarchyPanel();
    
    // Inspector panel (entity properties)
    RenderInspectorPanel();
    
    // Asset browser panel
    RenderAssetBrowserPanel();
    
    // 3D Viewport (OpenGL render target)
    RenderViewportPanel();
}
```

**Keyboard Shortcuts:**
- `W` - Translate mode
- `E` - Rotate mode
- `R` - Scale mode
- `Q` - Select mode
- `Ctrl+D` - Duplicate
- `Del` - Delete
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `F` - Focus on selection
- `Ctrl+S` - Save
- `Ctrl+G` - Toggle grid snapping

---

## 6. Implementation Roadmap

### Phase 1: Foundation (2-3 weeks)

**Week 1: Plugin System**
- [ ] Implement `IPlugin` interface
- [ ] Create `PluginManager` with dynamic loading
- [ ] Add plugin versioning and dependency resolution
- [ ] Create example plugin
- [ ] Test hot-reloading
- **Deliverable:** Working plugin system with example

**Week 2: Script Integration**
- [ ] Integrate Lua (LuaJIT or standard Lua)
- [ ] Implement `ScriptManager`
- [ ] Create C++ ↔ Lua bindings (using sol2 or custom)
- [ ] Set up script sandboxing
- [ ] Add script hot-reloading
- **Deliverable:** Lua scripts can create entities and modify components

**Week 3: Asset Hot-Reloading**
- [ ] Implement file system watcher (platform-specific)
- [ ] Add texture hot-reloading
- [ ] Add mesh hot-reloading
- [ ] Add script hot-reloading integration
- [ ] Create mod directory structure
- **Deliverable:** Assets reload automatically when changed

### Phase 2: Content Tools (3-4 weeks)

**Week 4-5: Entity Template System**
- [ ] Design JSON template format
- [ ] Implement `EntityTemplate` and `EntityTemplateManager`
- [ ] Add JSON parsing (using nlohmann/json or similar)
- [ ] Create template instantiation
- [ ] Add prefab/template browser UI
- **Deliverable:** Create entities from JSON templates

**Week 6-7: Visual Entity Editor**
- [ ] Integrate ImGui if not already present
- [ ] Create entity property inspector
- [ ] Add component add/remove UI
- [ ] Implement visual component editors (position sliders, etc.)
- [ ] Add template save/load UI
- **Deliverable:** Visual tool for creating entity templates

### Phase 3: Level Editor (4-5 weeks)

**Week 8-9: Core Editor**
- [ ] Implement editor camera controls
- [ ] Add entity selection (mouse picking)
- [ ] Implement gizmos (translate/rotate/scale)
- [ ] Add grid rendering and snapping
- [ ] Create toolbar and mode switching
- **Deliverable:** Basic 3D level editing

**Week 10-11: Advanced Features**
- [ ] Add undo/redo system
- [ ] Implement entity duplication
- [ ] Create asset browser panel
- [ ] Add level save/load (JSON format)
- [ ] Implement entity hierarchy panel
- **Deliverable:** Full-featured level editor

**Week 12: Polish & Integration**
- [ ] Add keyboard shortcuts
- [ ] Create editor preferences system
- [ ] Add scene preview mode (play-in-editor)
- [ ] Write documentation and tutorials
- [ ] Create example levels
- **Deliverable:** Production-ready level editor

### Phase 4: Modding Ecosystem (2-3 weeks)

**Week 13-14: Mod Packaging**
- [ ] Design `.starmod` package format (ZIP archive)
- [ ] Create mod manifest schema
- [ ] Implement mod installer/uninstaller
- [ ] Add mod conflict detection
- [ ] Create mod upload/download system
- **Deliverable:** Complete mod packaging system

**Week 15: Community Features**
- [ ] Create in-game mod browser
- [ ] Add mod rating/review system
- [ ] Implement Steam Workshop integration (optional)
- [ ] Create modding documentation site
- [ ] Set up example mod repository
- **Deliverable:** Community modding platform

---

## 7. Technical Specifications

### 7.1 Dependencies

**Required Libraries:**

```makefile
# Add to Makefile
MODDING_LIBS = -llua54 -limgui -lnlohmann_json

# Lua (scripting)
LUA_CFLAGS := $(shell pkg-config --cflags lua5.4)
LUA_LIBS := $(shell pkg-config --libs lua5.4)

# ImGui (UI)
IMGUI_CFLAGS := -Ivendor/imgui
IMGUI_SRC := vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp \
             vendor/imgui/imgui_widgets.cpp vendor/imgui/imgui_tables.cpp \
             vendor/imgui/backends/imgui_impl_glfw.cpp \
             vendor/imgui/backends/imgui_impl_opengl3.cpp

# JSON (parsing)
JSON_CFLAGS := -Ivendor/json/include
```

**Installation:**

```powershell
# MSYS2/MinGW (Windows)
pacman -S mingw-w64-x86_64-lua
pacman -S mingw-w64-x86_64-nlohmann-json

# Download ImGui manually
cd vendor
git clone https://github.com/ocornut/imgui.git
```

### 7.2 Directory Structure

```
Nova-Engine/
├── src/
│   ├── modding/
│   │   ├── IPlugin.h
│   │   ├── PluginManager.h
│   │   ├── PluginManager.cpp
│   │   ├── ScriptManager.h
│   │   ├── ScriptManager.cpp
│   │   ├── LuaBindings.cpp
│   │   ├── AssetWatcher.h
│   │   ├── AssetWatcher.cpp
│   │   ├── EntityTemplate.h
│   │   ├── EntityTemplate.cpp
│   │   ├── EntityEditor.h
│   │   ├── EntityEditor.cpp
│   │   ├── LevelEditor.h
│   │   └── LevelEditor.cpp
│   └── ...
├── mods/
│   ├── example_mod/
│   │   ├── mod.json          # Mod manifest
│   │   ├── main.lua          # Entry point script
│   │   ├── assets/
│   │   │   ├── textures/
│   │   │   ├── models/
│   │   │   └── audio/
│   │   ├── scripts/
│   │   └── templates/
│   └── community_ships/
│       └── ...
├── levels/
│   ├── tutorial.json
│   ├── combat_arena.json
│   └── ...
├── plugins/
│   ├── example_plugin.dll
│   ├── graphics_enhanced.dll
│   └── ...
├── vendor/
│   ├── imgui/
│   ├── json/
│   └── sol2/
└── docs/
    └── modding_system_design.md  # This document
```

### 7.3 Mod Package Format

**`.starmod` Structure (ZIP archive):**

```
my_custom_ship.starmod
├── mod.json                 # Manifest
├── main.lua                 # Entry point
├── assets/
│   ├── textures/
│   │   └── ship_texture.png
│   └── models/
│       └── ship_model.obj
├── scripts/
│   ├── ship_behavior.lua
│   └── weapon_system.lua
└── templates/
    └── custom_fighter.json
```

**mod.json schema:**

```json
{
    "name": "Custom Fighter Mod",
    "version": "1.0.0",
    "author": "ModAuthor",
    "description": "Adds a new custom fighter ship",
    "engine_version": "1.0.0",
    
    "dependencies": [
        "community_weapons_pack"
    ],
    
    "entry_point": "main.lua",
    
    "assets": {
        "textures": ["assets/textures/*.png"],
        "models": ["assets/models/*.obj"],
        "audio": []
    },
    
    "templates": [
        "templates/custom_fighter.json"
    ],
    
    "permissions": {
        "file_access": false,
        "network_access": false,
        "system_access": false
    }
}
```

---

## 8. Integration with Existing Systems

### 8.1 ECS V2 Integration

**Component Registration from Mods:**

```lua
-- mods/example/main.lua
Engine.RegisterComponent("CustomThrusters", {
    thrustPower = 150.0,
    fuelConsumption = 2.5,
    heatGeneration = 10.0
})

-- Create entity with mod component
local ship = Engine.CreateEntity("modded_ship")
Engine.AddComponent(ship, "CustomThrusters", {
    thrustPower = 200.0
})
```

**System Registration from Plugins:**

```cpp
// ExamplePlugin.cpp
class CustomThrusterSystem : public SystemV2 {
public:
    void Update(EntityManagerV2& em, double deltaTime) override {
        em.ForEach<CustomThrusters, Velocity>([&](EntityHandle e, 
                                                   CustomThrusters& thrusters,
                                                   Velocity& vel) {
            // Apply thrust
            vel.z += thrusters.thrustPower * deltaTime;
        });
    }
};

bool ExamplePlugin::OnLoad() {
    // Register custom system
    SystemSchedulerV2::Instance().RegisterSystem(
        std::make_unique<CustomThrusterSystem>()
    );
    return true;
}
```

### 8.2 Ship Assembly Integration

**Custom Ship Modules from Mods:**

```json
// mods/weapons/templates/plasma_cannon.json
{
    "type": "component",
    "category": "weapon",
    "name": "Plasma Cannon Mk2",
    "stats": {
        "damage": 75.0,
        "fireRate": 1.5,
        "powerDraw": 25.0,
        "heatGeneration": 15.0
    },
    "model": "mods/weapons/models/plasma_cannon.obj",
    "texture": "mods/weapons/textures/plasma_cannon.png",
    "mount_size": "medium",
    "hardpoint_offset": [0, 0.5, 1.2]
}
```

### 8.3 TextRenderer Integration

**Custom HUD Elements from Scripts:**

```lua
-- mods/hud_extended/hud_overlay.lua
function OnRenderHUD()
    -- Use TextRenderer for custom HUD
    TextRenderer.Render(10, 700, "MOD: Enhanced HUD v1.0", 
                       {r=0, g=255, b=255}, "small")
    
    -- Custom ship stats
    local shipTemp = Engine.GetComponent(player, "Temperature")
    TextRenderer.RenderF(10, 680, "Engine Temp: %.1f°C", 
                        {r=255, g=255, b=0}, "medium", 
                        shipTemp.value)
end

Engine.RegisterHook("RenderHUD", OnRenderHUD)
```

---

## 9. Example Use Cases

### 9.1 Custom Ship Mod

**Mod Structure:**
```
mods/battlecruiser_pack/
├── mod.json
├── main.lua
├── templates/
│   └── battlecruiser.json
├── assets/
│   ├── models/
│   │   └── battlecruiser.obj
│   └── textures/
│       └── battlecruiser.png
└── scripts/
    └── battlecruiser_ai.lua
```

**main.lua:**
```lua
-- Register custom ship template
Engine.LoadTemplate("mods/battlecruiser_pack/templates/battlecruiser.json")

-- Add to ship selector
Engine.AddToShipMenu("Battlecruiser", "battlecruiser")

print("Battlecruiser Pack loaded successfully!")
```

### 9.2 New Game Mode Plugin

**C++ Plugin:**
```cpp
// plugins/capture_the_flag/CaptureTheFlagMode.cpp
class CaptureTheFlagMode : public IPlugin {
public:
    bool OnLoad() override {
        // Register game mode
        GameModeManager::RegisterMode("capture_the_flag", this);
        
        // Load flag entities
        flagRed_ = EntityTemplateManager::Instance()
            .CreateFromTemplate("ctf_flag_red");
        flagBlue_ = EntityTemplateManager::Instance()
            .CreateFromTemplate("ctf_flag_blue");
        
        return true;
    }
    
    void OnUpdate(double deltaTime) override {
        // Game logic: check if players captured flags
        CheckFlagCapture();
        UpdateScoreboard();
    }
    
private:
    EntityHandle flagRed_;
    EntityHandle flagBlue_;
    int scoreRed_ = 0;
    int scoreBlue_ = 0;
};

STAR_DECLARE_PLUGIN(CaptureTheFlagMode)
```

### 9.3 Visual Effects Mod

**Lua Script:**
```lua
-- mods/enhanced_explosions/explosion_effects.lua

-- Override default explosion effect
function OnShipDestroyed(ship)
    local pos = Engine.GetComponent(ship, "Position")
    
    -- Spawn multiple particle emitters
    for i = 1, 10 do
        local emitter = Engine.CreateEntity("particle_emitter")
        Engine.AddComponent(emitter, "Position", pos)
        Engine.AddComponent(emitter, "ParticleSystem", {
            texture = "mods/enhanced_explosions/particle.png",
            lifetime = 2.0,
            count = 100,
            color = {r = 255, g = 128, b = 0}
        })
    end
    
    -- Play custom explosion sound
    Engine.PlaySound("mods/enhanced_explosions/explosion.wav", pos)
end

Engine.RegisterEventHandler("ShipDestroyed", OnShipDestroyed)
```

---

## 10. Security Considerations

### 10.1 Sandbox Restrictions

**What Mods CAN Do:**
- ✅ Create/modify entities and components
- ✅ Register new component types
- ✅ Hook into game events
- ✅ Load assets from their mod directory
- ✅ Render UI elements
- ✅ Access player input
- ✅ Communicate with other mods (event system)

**What Mods CANNOT Do:**
- ❌ Access arbitrary file system paths
- ❌ Make network connections (without permission)
- ❌ Execute system commands
- ❌ Load native code (except through plugins)
- ❌ Modify engine core files
- ❌ Access memory outside sandbox

### 10.2 Permission System

```json
// mod.json
{
    "permissions": {
        "file_access": false,        // Read/write mod directory only
        "network_access": false,     // HTTP requests
        "system_access": false,      // Native plugins
        "extended_memory": false     // Large memory allocations
    }
}
```

User must approve mods requesting dangerous permissions.

---

## 11. Performance Considerations

### 11.1 Hot-Reloading Performance

- **File watching**: Runs in background thread, ~1-5ms overhead
- **Asset reloading**: Depends on asset size (textures: 10-100ms)
- **Script reloading**: ~1-10ms per script
- **Impact**: Minimal during gameplay, noticeable during development

### 11.2 Script Performance

- **LuaJIT**: 5-20x faster than standard Lua, near-native performance
- **C++ bindings**: Use sol2 for zero-overhead function calls
- **Optimization**: Cache frequently accessed components
- **Best practice**: Heavy computation in C++ plugins, logic in Lua

### 11.3 Plugin Performance

- **Native code**: Same performance as engine code
- **Virtual calls**: Minimal overhead (~1-2 CPU cycles)
- **Memory**: Plugins share engine memory pool
- **Threading**: Plugins can use ECS V2 parallel systems

---

## 12. Documentation & Community

### 12.1 Modding Documentation

**Required Documentation:**

1. **Modding API Reference** - Complete Lua API documentation
2. **Plugin Development Guide** - C++ plugin creation
3. **Entity Template Tutorial** - JSON template format
4. **Level Editor Manual** - Editor usage guide
5. **Mod Publishing Guide** - How to package and distribute mods

### 12.2 Example Mods

**Ship Mod Tutorial:**
```
docs/tutorials/01_creating_your_first_ship_mod.md
docs/tutorials/02_adding_custom_weapons.md
docs/tutorials/03_ship_ai_behavior.md
```

**Plugin Tutorial:**
```
docs/tutorials/04_creating_a_plugin.md
docs/tutorials/05_custom_systems.md
docs/tutorials/06_advanced_scripting.md
```

### 12.3 Mod Repository

**GitHub Repository Structure:**
```
nova-engine-mods/
├── README.md
├── ships/
│   ├── battlecruiser_pack/
│   ├── stealth_fighters/
│   └── mining_vessels/
├── weapons/
│   └── energy_weapons_extended/
├── game_modes/
│   ├── capture_the_flag/
│   └── racing/
└── visual_enhancements/
    ├── hd_textures/
    └── particle_effects/
```

---

## 13. Testing Strategy

### 13.1 Unit Tests

```cpp
// tests/test_plugin_manager.cpp
TEST(PluginManager, LoadsValidPlugin) {
    auto& pm = PluginManager::Instance();
    ASSERT_TRUE(pm.LoadPlugin("plugins/example_plugin.dll"));
    ASSERT_TRUE(pm.IsPluginLoaded("Example Plugin"));
}

TEST(PluginManager, RejectsIncompatibleABI) {
    auto& pm = PluginManager::Instance();
    ASSERT_FALSE(pm.LoadPlugin("plugins/old_abi_plugin.dll"));
}

TEST(ScriptManager, ExecutesLuaScript) {
    auto& sm = ScriptManager::Instance();
    ASSERT_TRUE(sm.ExecuteScript("x = 5 + 3"));
    // Check result...
}

TEST(EntityTemplateManager, InstantiatesTemplate) {
    auto& etm = EntityTemplateManager::Instance();
    EntityHandle e = etm.CreateFromTemplate("test_entity");
    ASSERT_TRUE(e.IsValid());
}
```

### 13.2 Integration Tests

```cpp
// tests/test_modding_integration.cpp
TEST(ModdingIntegration, PluginCanAccessECS) {
    // Load plugin that creates entities
    PluginManager::Instance().LoadPlugin("plugins/test_plugin.dll");
    
    // Plugin should have created test entity
    auto& em = EntityManagerV2::Instance();
    bool found = false;
    em.ForEach<Position>([&](EntityHandle e, Position& pos) {
        if (pos.x == 100.0f) found = true;
    });
    ASSERT_TRUE(found);
}

TEST(ModdingIntegration, ScriptHotReloadWorks) {
    // Load script
    ScriptManager::Instance().LoadScript("test_script.lua");
    
    // Modify script file
    ModifyFile("test_script.lua");
    
    // Trigger reload
    ScriptManager::Instance().ReloadScript("test_script.lua");
    
    // Verify changes took effect
    // ...
}
```

---

## 14. Future Enhancements

### Phase 5: Advanced Features (Future)

**Visual Scripting:**
- Node-based visual scripting editor
- Blueprints-style system (like Unreal Engine)
- No coding required for basic mods

**Multi-Language Support:**
- Python scripting (via pybind11)
- C# scripting (via Mono)
- JavaScript (via V8)

**Mod Marketplace:**
- In-game mod store
- Paid mod support
- Mod revenue sharing

**Collaborative Editing:**
- Multi-user level editing
- Real-time collaboration
- Cloud-saved projects

**Advanced Debugging:**
- Lua debugger integration (ZeroBrane Studio)
- Performance profiler for scripts
- Memory leak detection for mods

---

## 15. Summary

This modding system design provides:

1. **Plugin System**: Native C++ extensions for maximum performance
2. **Scripting System**: Lua integration for accessible modding
3. **Asset Hot-Reloading**: Rapid iteration during development
4. **Entity Templates**: JSON-based entity creation
5. **Visual Tools**: ImGui-based editors for non-programmers
6. **Level Editor**: Full 3D scene editing capabilities

**Estimated Implementation Time:** 12-15 weeks for complete system

**Dependencies:**
- Lua/LuaJIT: Scripting engine
- sol2/sol3: C++ Lua bindings
- ImGui: UI framework
- nlohmann/json: JSON parsing
- Platform file watchers: Hot-reloading

**Benefits:**
- ✅ Thriving modding community
- ✅ Extended content lifetime
- ✅ User-generated content
- ✅ Faster prototyping for developers
- ✅ Reduced development burden

**Next Steps:**
1. Review this design document
2. Prioritize features for Phase 1
3. Set up dependency libraries
4. Begin plugin system implementation

---

**Document End**

*For questions or suggestions, contact the Nova-Engine development team.*
