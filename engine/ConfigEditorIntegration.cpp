#include "ConfigEditorIntegration.h"
#include "ConfigSystem.h"
#include <GLFW/glfw3.h>
#include <iostream>
// Optionally install ImGui-based UI when available
#ifdef USE_IMGUI
#include "ConfigEditorImGuiUI.h"
#endif

namespace nova::config {

// Global instance
static std::unique_ptr<ConfigEditorIntegration> g_configEditorIntegration;

ConfigEditorIntegration::ConfigEditorIntegration() 
    : editor_(std::make_unique<ConfigEditor>()) {
}

ConfigEditorIntegration::~ConfigEditorIntegration() = default;

bool ConfigEditorIntegration::Initialize() {
    std::cout << "[ConfigEditorIntegration] Initializing config editor integration..." << std::endl;
    
    if (!editor_->Initialize()) {
        std::cerr << "[ConfigEditorIntegration] Failed to initialize config editor" << std::endl;
        return false;
    }

#ifdef USE_IMGUI
    // If ImGui support is compiled in, swap the editor UI to the ImGui implementation
    try {
        auto imguiUI = std::make_unique<ConfigEditorImGuiUI>(editor_.get());
        if (imguiUI->Initialize()) {
            editor_->SetCustomUI(std::move(imguiUI));
            std::cout << "[ConfigEditorIntegration] Installed ImGui-based editor UI" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ConfigEditorIntegration] Failed to create ImGui UI: " << e.what() << std::endl;
    }
#endif
    
    // Set up callbacks
    editor_->SetValidationCallback([this](const schema::ValidationResult& result) {
        OnValidationResult(result);
    });
    
    editor_->SetSaveCallback([this](const std::string& filePath) {
        OnConfigSaved(filePath);
    });
    
    // Enable hot reload by default in debug builds
    #ifdef _DEBUG
    EnableHotReload();
    #endif
    
    std::cout << "[ConfigEditorIntegration] Config editor integration initialized" << std::endl;
    return true;
}

void ConfigEditorIntegration::Shutdown() {
    if (editor_) {
        editor_->Shutdown();
    }
    
    UnregisterEngineCallbacks();
}

void ConfigEditorIntegration::Update(double deltaTime) {
    if (editor_) {
        editor_->Update(deltaTime);
    }
}

void ConfigEditorIntegration::AttachToMainLoop(MainLoop* mainLoop) {
    mainLoop_ = mainLoop;
    RegisterEngineCallbacks();
}

void ConfigEditorIntegration::AttachToSimulation(Simulation* simulation) {
    simulation_ = simulation;
}

void ConfigEditorIntegration::HandleKeyPress(int key, int scancode, int action, int mods) {
    // Check for config editor hotkeys
    // Use GLFW named constants (via precompiled header) for clarity
    if (action == GLFW_PRESS) {
        // F12 to toggle config editor
        if (key == GLFW_KEY_F12) {
            ToggleEditor();
            return;
        }

        // Ctrl+O to open config
        if (key == GLFW_KEY_O && (mods & GLFW_MOD_CONTROL)) {
            if (IsEditorVisible()) {
                // Show file dialog or quick load
                std::cout << "[ConfigEditor] Quick load shortcut activated" << std::endl;
            }
            return;
        }

        // Ctrl+S to save config
        if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
            if (IsEditorVisible() && editor_->HasUnsavedChanges()) {
                editor_->SaveConfig();
            }
            return;
        }

        // Ctrl+N to new config
        if (key == GLFW_KEY_N && (mods & GLFW_MOD_CONTROL)) {
            if (IsEditorVisible()) {
                editor_->NewConfig("actor_config");
            }
            return;
        }

        // Ctrl+Z for undo
        if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL)) {
            if (IsEditorVisible()) {
                editor_->Undo();
            }
            return;
        }

        // Ctrl+Y for redo
        if (key == GLFW_KEY_Y && (mods & GLFW_MOD_CONTROL)) {
            if (IsEditorVisible()) {
                editor_->Redo();
            }
            return;
        }
    }
    
    // If editor is visible and has focus, consume input
    if (IsEditorVisible() && editorHasFocus_) {
        // Input is handled by the UI system
        return;
    }
}

void ConfigEditorIntegration::HandleMouseButton(int button, int action, int mods) {
    if (IsEditorVisible()) {
        // Determine if click is within editor bounds
        // For now, assume editor consumes all mouse input when visible
        editorHasFocus_ = true;
    }
}

void ConfigEditorIntegration::HandleMouseMove(double xpos, double ypos) {
    // Update editor focus based on mouse position
    if (IsEditorVisible()) {
        // In a real implementation, check if mouse is over editor window
        // For now, assume editor has focus when visible
    }
}

void ConfigEditorIntegration::SetEditorVisible(bool visible) {
    if (editor_) {
        editor_->SetUIVisible(visible);
        
        if (visible) {
            std::cout << "[ConfigEditor] Config editor opened" << std::endl;
        } else {
            std::cout << "[ConfigEditor] Config editor closed" << std::endl;
            editorHasFocus_ = false;
        }
    }
}

bool ConfigEditorIntegration::IsEditorVisible() const {
    return editor_ ? editor_->IsUIVisible() : false;
}

void ConfigEditorIntegration::EnableHotReload() {
    if (editor_) {
        editor_->EnableHotReload();
        std::cout << "[ConfigEditor] Hot reload enabled" << std::endl;
    }
}

void ConfigEditorIntegration::DisableHotReload() {
    if (editor_) {
        editor_->DisableHotReload();
        std::cout << "[ConfigEditor] Hot reload disabled" << std::endl;
    }
}

bool ConfigEditorIntegration::QuickLoadConfig(const std::string& filePath) {
    if (!editor_) return false;
    
    // Try to determine schema from file path
    std::string schemaId;
    if (filePath.find("station") != std::string::npos) {
        schemaId = "station_config";
    } else if (filePath.find("ship") != std::string::npos) {
        schemaId = "ship_config";
    } else {
        schemaId = "actor_config"; // Default
    }
    
    bool success = editor_->LoadConfig(filePath, schemaId);
    if (success) {
        SetEditorVisible(true);
    }
    
    return success;
}

bool ConfigEditorIntegration::QuickEditConfig(const std::string& configType) {
    if (!editor_) return false;
    
    bool success = editor_->NewConfig(configType + "_config");
    if (success) {
        SetEditorVisible(true);
    }
    
    return success;
}

void ConfigEditorIntegration::ShowValidationErrors() {
    if (!editor_) return;
    
    const auto& validation = editor_->GetLastValidation();
    if (!validation.success) {
        std::cout << "[ConfigEditor] Validation Errors:" << std::endl;
        for (const auto& error : validation.errors) {
            std::cout << "  - " << error.path << ": " << error.message << std::endl;
        }
    } else {
        std::cout << "[ConfigEditor] Configuration is valid" << std::endl;
    }
}

void ConfigEditorIntegration::OnConfigFileChanged(const std::string& filePath) {
    std::cout << "[ConfigEditor] Config file changed: " << filePath << std::endl;
    
    // If this file is currently being edited, offer to reload
    if (editor_ && editor_->GetCurrentFile() == filePath) {
        std::cout << "[ConfigEditor] Current file was modified externally" << std::endl;
        // In a real implementation, show a dialog asking if user wants to reload
    }
}

void ConfigEditorIntegration::OnValidationResult(const schema::ValidationResult& result) {
    if (result.success) {
        std::cout << "[ConfigEditor] Validation passed" << std::endl;
    } else {
        std::cout << "[ConfigEditor] Validation failed with " << result.errors.size() 
                  << " errors" << std::endl;
        
        // In development builds, show detailed errors
        #ifdef _DEBUG
        for (const auto& error : result.errors) {
            std::cout << "  [" << error.path << "] " << error.message << std::endl;
        }
        #endif
    }
}

void ConfigEditorIntegration::OnConfigSaved(const std::string& filePath) {
    std::cout << "[ConfigEditor] Configuration saved to: " << filePath << std::endl;
    
    // Notify simulation of config changes if applicable
    if (simulation_) {
        // Signal that configs may have changed
        std::cout << "[ConfigEditor] Notifying simulation of config changes" << std::endl;
    }
}

void ConfigEditorIntegration::RegisterEngineCallbacks() {
    // Register callbacks with main loop for input handling
    // This would depend on your specific input system implementation
    std::cout << "[ConfigEditor] Registered engine callbacks" << std::endl;
}

void ConfigEditorIntegration::UnregisterEngineCallbacks() {
    // Unregister callbacks
    std::cout << "[ConfigEditor] Unregistered engine callbacks" << std::endl;
}

} // namespace nova::config

// Global access functions
namespace nova {

config::ConfigEditor& GetConfigEditor() {
    if (!config::g_configEditorIntegration) {
        config::g_configEditorIntegration = std::make_unique<config::ConfigEditorIntegration>();
        config::g_configEditorIntegration->Initialize();
    }
    return config::g_configEditorIntegration->GetEditor();
}

config::ConfigEditorIntegration& GetConfigEditorIntegration() {
    if (!config::g_configEditorIntegration) {
        config::g_configEditorIntegration = std::make_unique<config::ConfigEditorIntegration>();
        config::g_configEditorIntegration->Initialize();
    }
    return *config::g_configEditorIntegration;
}

} // namespace nova