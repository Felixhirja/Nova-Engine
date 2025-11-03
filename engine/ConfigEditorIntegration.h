#pragma once

#include "ConfigEditor.h"
#include "MainLoop.h"
#include "Simulation.h"
#include <memory>

namespace nova::config {

/**
 * Configuration Editor Integration for Nova Engine
 * 
 * Provides seamless integration of the visual config editor with the main engine systems.
 * Handles initialization, input routing, and hot reloading integration.
 */
class ConfigEditorIntegration {
public:
    ConfigEditorIntegration();
    ~ConfigEditorIntegration();
    
    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(double deltaTime);
    
    // Engine integration
    void AttachToMainLoop(MainLoop* mainLoop);
    void AttachToSimulation(Simulation* simulation);
    
    // Input handling
    void HandleKeyPress(int key, int scancode, int action, int mods);
    void HandleMouseButton(int button, int action, int mods);
    void HandleMouseMove(double xpos, double ypos);
    
    // Config editor access
    ConfigEditor& GetEditor() { return *editor_; }
    const ConfigEditor& GetEditor() const { return *editor_; }
    
    // Visibility control
    void ToggleEditor() { SetEditorVisible(!IsEditorVisible()); }
    void SetEditorVisible(bool visible);
    bool IsEditorVisible() const;
    
    // Hot reloading integration
    void EnableHotReload();
    void DisableHotReload();
    
    // Quick access methods for common operations
    bool QuickLoadConfig(const std::string& filePath);
    bool QuickEditConfig(const std::string& configType);
    void ShowValidationErrors();

private:
    std::unique_ptr<ConfigEditor> editor_;
    MainLoop* mainLoop_ = nullptr;
    Simulation* simulation_ = nullptr;
    
    // Input state
    bool editorHasFocus_ = false;
    
    // Hot reload callbacks
    void OnConfigFileChanged(const std::string& filePath);
    void OnValidationResult(const schema::ValidationResult& result);
    void OnConfigSaved(const std::string& filePath);
    
    // Engine event handlers
    void RegisterEngineCallbacks();
    void UnregisterEngineCallbacks();
};

} // namespace nova::config

// Global convenience functions for easy access from anywhere in the engine
namespace nova {

/**
 * Get the global config editor instance
 */
config::ConfigEditor& GetConfigEditor();

/**
 * Get the global config editor integration
 */
config::ConfigEditorIntegration& GetConfigEditorIntegration();

/**
 * Quick access functions for common config editor operations
 */
inline void OpenConfigEditor() {
    GetConfigEditorIntegration().SetEditorVisible(true);
}

inline void CloseConfigEditor() {
    GetConfigEditorIntegration().SetEditorVisible(false);
}

inline void ToggleConfigEditor() {
    GetConfigEditorIntegration().ToggleEditor();
}

inline bool LoadConfigInEditor(const std::string& filePath) {
    return GetConfigEditorIntegration().QuickLoadConfig(filePath);
}

inline bool EditConfig(const std::string& configType) {
    return GetConfigEditorIntegration().QuickEditConfig(configType);
}

} // namespace nova