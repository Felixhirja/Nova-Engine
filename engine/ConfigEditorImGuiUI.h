#pragma once

#include "ConfigEditor.h"
#include <string>
#include <vector>
#include <unordered_map>

// ImGui type definitions - full definitions for struct members
#ifdef USE_IMGUI
#include <imgui.h>
#else
// Stub declarations for when ImGui is not available
struct ImGuiContext {};
struct ImVec2 { float x, y; ImVec2(float x = 0, float y = 0) : x(x), y(y) {} };
struct ImVec4 { float x, y, z, w; ImVec4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {} };
#endif

namespace nova::config {

/**
 * ImGui-based UI implementation for the Configuration Editor
 * 
 * Provides a complete visual interface with:
 * - Form-based editing with appropriate input controls
 * - Real-time validation feedback
 * - Template selection and management
 * - File operations with native dialogs
 * - Undo/redo history visualization
 * - Syntax-highlighted JSON editor
 * - Live preview capabilities
 */
class ConfigEditorImGuiUI : public ConfigEditorUI {
public:
    ConfigEditorImGuiUI(ConfigEditor* editor);
    ~ConfigEditorImGuiUI();
    
    // UI lifecycle
    bool Initialize();
    void Shutdown();
    void Render();
    
    // Visibility control
    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }
    
    // UI state management
    void ResetUIState();
    void SaveUILayout();
    void LoadUILayout();

private:
    ConfigEditor* editor_;
    bool visible_ = false;
    
    // UI State
    struct UIState {
        bool showDemoWindow = false;
        bool showValidationPanel = true;
        bool showHistoryPanel = false;
        bool showTemplatePanel = false;
        bool showJsonEditor = false;
        bool showLivePreview = false;
        
        // Window dimensions and positions
        ImVec2 mainWindowSize{1200, 800};
        ImVec2 mainWindowPos{100, 100};
        
        // Panel sizes
        float leftPanelWidth = 250.0f;
        float rightPanelWidth = 300.0f;
        float bottomPanelHeight = 200.0f;
        
        // Search and filter
        char searchBuffer[256] = "";
        std::string selectedCategory = "";
        
        // Form state
        std::unordered_map<std::string, bool> sectionCollapsed;
        std::string focusedField = "";
        
        // File dialog state
        bool showOpenDialog = false;
        bool showSaveDialog = false;
        bool showNewConfigDialog = false;
        
        // Validation state
        bool autoValidate = true;
        bool showValidationDetails = true;
        
        // Editor preferences
        bool prettyPrintJson = true;
        bool showLineNumbers = true;
        int fontSize = 14;
        
        // Template state
        std::string selectedTemplate = "";
        std::unordered_map<std::string, std::string> templateVariables;
    } state_;
    
    // Rendering methods
    void RenderMainWindow();
    void RenderMenuBar();
    void RenderToolbar();
    void RenderLeftPanel();
    void RenderCenterPanel();
    void RenderRightPanel();
    void RenderBottomPanel();
    void RenderStatusBar();
    
    // Form rendering
    void RenderFormEditor();
    void RenderSection(const EditorSection& section, const std::string& pathPrefix = "");
    void RenderField(const EditorField& field, const std::string& fieldPath);
    bool RenderFieldInput(const EditorField& field, const std::string& fieldPath);
    
    // Specialized field renderers
    void RenderStringField(const EditorField& field, const std::string& fieldPath);
    void RenderNumberField(const EditorField& field, const std::string& fieldPath);
    void RenderBooleanField(const EditorField& field, const std::string& fieldPath);
    void RenderEnumField(const EditorField& field, const std::string& fieldPath);
    void RenderArrayField(const EditorField& field, const std::string& fieldPath);
    void RenderObjectField(const EditorField& field, const std::string& fieldPath);
    void RenderColorField(const EditorField& field, const std::string& fieldPath);
    void RenderVector3Field(const EditorField& field, const std::string& fieldPath);
    void RenderFileField(const EditorField& field, const std::string& fieldPath);
    
    // Panel implementations
    void RenderSchemaSelector();
    void RenderRecentFiles();
    void RenderTemplateList();
    void RenderValidationPanel();
    void RenderHistoryPanel();
    void RenderJsonEditor();
    void RenderLivePreview();
    
    // Dialog implementations
    void RenderFileDialogs();
    void RenderNewConfigDialog();
    void RenderTemplateDialog();
    void RenderValidationDetailsDialog();
    void RenderAboutDialog();
    
    // Utility methods
    void ApplyTheme();
    void RenderTooltip(const std::string& text);
    void RenderHelpMarker(const std::string& text);
    bool RenderConfirmDialog(const std::string& title, const std::string& message);
    void ShowNotification(const std::string& message, float duration = 3.0f);
    
    // Input handling
    void HandleKeyboardShortcuts();
    void HandleDragDrop();
    
    // Field value helpers
    simplejson::JsonValue GetFieldValue(const std::string& fieldPath) const;
    void SetFieldValue(const std::string& fieldPath, const simplejson::JsonValue& value);
    std::string GetFieldDisplayName(const std::string& fieldPath) const;
    
    // Validation helpers
    void RenderValidationIcon(bool isValid, const std::string& error = "");
    void RenderValidationMessage(const schema::ValidationResult& result);
    ImVec4 GetValidationColor(bool isValid) const;
    
    // JSON editor helpers
    void RenderJsonSyntaxHighlighting(const std::string& json);
    bool ValidateJsonSyntax(const std::string& json, std::string& error);
    std::string FormatJson(const std::string& json, bool prettyPrint = true);
    
    // Template helpers
    void RenderTemplateVariable(const EditorField& variable, const std::string& templateName);
    bool ApplyTemplate(const std::string& templateName, 
                      const std::unordered_map<std::string, simplejson::JsonValue>& variables);
    
    // File operation helpers
    bool SaveFile(const std::string& filePath);
    bool LoadFile(const std::string& filePath);
    std::vector<std::string> GetRecentFiles() const;
    void AddToRecentFiles(const std::string& filePath);
    
    // UI styling
    void PushFieldStyle(const EditorField& field);
    void PopFieldStyle();
    void SetupColors();
    void SetupFonts();
    
    // State persistence
    void SaveState();
    void LoadState();
    
    // Debug and development
    void RenderDebugInfo();
    void RenderMetrics();
    
    // Notification system
    struct Notification {
        std::string message;
        float timeRemaining;
        ImVec4 color;
    };
    std::vector<Notification> notifications_;
    void UpdateNotifications(float deltaTime);
    void RenderNotifications();
    
    // Theme and styling
    enum class Theme {
        Dark,
        Light,
        Classic,
        Nova  // Custom Nova Engine theme
    };
    Theme currentTheme_ = Theme::Nova;
    void ApplyTheme(Theme theme);
    
    // Custom widgets
    bool ColorEdit3WithAlpha(const char* label, float* col, bool* hasAlpha = nullptr);
    bool Vector3Input(const char* label, float* values, const char* format = "%.3f");
    bool FileSelector(const char* label, std::string& filePath, const char* filter = nullptr);
    bool SearchableCombo(const char* label, std::string& current, const std::vector<std::string>& items);
    
    // Performance monitoring
    struct PerformanceMetrics {
        float frameTime = 0.0f;
        float renderTime = 0.0f;
        int triangleCount = 0;
        int drawCalls = 0;
    } metrics_;
    
    void UpdateMetrics();
};

} // namespace nova::config