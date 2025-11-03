#pragma once

#include "JsonSchema.h"
#include "SimpleJson.h"
#include "ConfigSystem.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace nova::config {

/**
 * Visual Configuration Editor for Nova Engine
 * 
 * Provides a comprehensive visual editor for actor configurations with:
 * - Real-time JSON schema validation
 * - Form-based editing with type-safe inputs
 * - Live preview and hot reloading integration
 * - Template system for rapid configuration creation
 * - Undo/redo support with history tracking
 * - Auto-save functionality
 * - Integration with existing asset pipeline
 */

// Forward declarations
class ConfigEditorUI;
class ConfigTemplate;
class ConfigHistory;

/**
 * Configuration field definition for visual editing
 */
struct EditorField {
    enum class Type {
        String,
        Number,
        Boolean,
        Array,
        Object,
        Enum,
        Color,
        Vector3,
        File
    };
    
    std::string name;
    std::string displayName;
    std::string description;
    Type type;
    bool required = false;
    bool readOnly = false;
    
    // Type-specific constraints
    double minValue = 0.0;
    double maxValue = 100.0;
    std::vector<std::string> enumValues;
    std::string fileFilter;
    
    // Default value
    simplejson::JsonValue defaultValue;
    
    // Validation callback
    std::function<bool(const simplejson::JsonValue&)> validator;
};

/**
 * Configuration section for organizing fields
 */
struct EditorSection {
    std::string name;
    std::string displayName;
    std::string description;
    bool collapsible = true;
    bool collapsed = false;
    std::vector<EditorField> fields;
    std::vector<EditorSection> subsections;
};

/**
 * Configuration template for rapid creation
 */
class ConfigTemplate {
public:
    ConfigTemplate(const std::string& name, const std::string& description);
    
    void SetBaseConfig(const simplejson::JsonObject& config);
    void AddVariable(const std::string& name, const std::string& description, 
                    const simplejson::JsonValue& defaultValue);
    
    simplejson::JsonObject Generate(const std::unordered_map<std::string, simplejson::JsonValue>& variables) const;
    
    const std::string& GetName() const { return name_; }
    const std::string& GetDescription() const { return description_; }
    const std::vector<EditorField>& GetVariables() const { return variables_; }

private:
    std::string name_;
    std::string description_;
    simplejson::JsonObject baseConfig_;
    std::vector<EditorField> variables_;
};

/**
 * Configuration edit history for undo/redo
 */
class ConfigHistory {
public:
    struct HistoryEntry {
        std::string action;
        simplejson::JsonObject configBefore;
        simplejson::JsonObject configAfter;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    void PushEdit(const std::string& action, 
                  const simplejson::JsonObject& before,
                  const simplejson::JsonObject& after);
    
    bool CanUndo() const { return currentIndex_ > 0; }
    bool CanRedo() const { return currentIndex_ < history_.size(); }
    
    const HistoryEntry* Undo();
    const HistoryEntry* Redo();
    
    void Clear();
    const std::vector<HistoryEntry>& GetHistory() const { return history_; }

private:
    std::vector<HistoryEntry> history_;
    size_t currentIndex_ = 0;
    static constexpr size_t MAX_HISTORY = 100;
};

/**
 * Main Configuration Editor class
 */
class ConfigEditor {
public:
    enum class ValidationMode {
        None,
        OnChange,
        OnSave,
        Continuous
    };
    
    ConfigEditor();
    ~ConfigEditor();
    
    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(double deltaTime);
    
    // Configuration management
    bool LoadConfig(const std::string& filePath, const std::string& schemaId = "");
    bool SaveConfig(const std::string& filePath = "");
    bool NewConfig(const std::string& schemaId);
    
    // Schema and form generation
    bool LoadSchema(const std::string& schemaId);
    EditorSection GenerateFormFromSchema(const std::string& schemaId);
    const EditorSection& GetCurrentForm() const { return currentForm_; }
    
    // Template system
    void RegisterTemplate(std::unique_ptr<ConfigTemplate> configTemplate);
    bool LoadFromTemplate(const std::string& templateName, 
                         const std::unordered_map<std::string, simplejson::JsonValue>& variables);
    const std::vector<std::unique_ptr<ConfigTemplate>>& GetTemplates() const { return templates_; }
    
    // Validation
    void SetValidationMode(ValidationMode mode) { validationMode_ = mode; }
    ValidationMode GetValidationMode() const { return validationMode_; }
    bool ValidateCurrentConfig();
    const schema::ValidationResult& GetLastValidation() const { return lastValidation_; }
    
    // History and undo/redo
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void ClearHistory() { history_.Clear(); }
    
    // Configuration access
    const simplejson::JsonObject& GetCurrentConfig() const { return currentConfig_; }
    bool SetFieldValue(const std::string& fieldPath, const simplejson::JsonValue& value);
    simplejson::JsonValue GetFieldValue(const std::string& fieldPath) const;
    
    // File operations
    void SetCurrentFile(const std::string& filePath) { currentFilePath_ = filePath; }
    const std::string& GetCurrentFile() const { return currentFilePath_; }
    bool HasUnsavedChanges() const { return hasUnsavedChanges_; }
    
    // Auto-save
    void EnableAutoSave(int intervalSeconds = 60) { 
        autoSaveEnabled_ = true; 
        autoSaveInterval_ = std::chrono::seconds(intervalSeconds);
    }
    void DisableAutoSave() { autoSaveEnabled_ = false; }
    
    // Hot reloading integration
    void EnableHotReload() { hotReloadEnabled_ = true; }
    void DisableHotReload() { hotReloadEnabled_ = false; }
    
    // Callbacks
    using ValidationCallback = std::function<void(const schema::ValidationResult&)>;
    using ChangeCallback = std::function<void(const std::string& fieldPath, const simplejson::JsonValue&)>;
    using SaveCallback = std::function<void(const std::string& filePath)>;
    
    void SetValidationCallback(ValidationCallback callback) { validationCallback_ = callback; }
    void SetChangeCallback(ChangeCallback callback) { changeCallback_ = callback; }
    void SetSaveCallback(SaveCallback callback) { saveCallback_ = callback; }
    
    // UI Integration (for ImGui or other UI systems)
    void RenderUI();
    bool IsUIVisible() const { return uiVisible_; }
    void SetUIVisible(bool visible) { uiVisible_ = visible; }
    // Replace the default UI implementation (e.g. swap in ImGui UI)
    void SetCustomUI(std::unique_ptr<ConfigEditorUI> ui);

private:
    // Current state
    simplejson::JsonObject currentConfig_;
    std::string currentSchemaId_;
    std::string currentFilePath_;
    EditorSection currentForm_;
    bool hasUnsavedChanges_ = false;
    
    // Schema and validation
    schema::ValidationResult lastValidation_;
    ValidationMode validationMode_ = ValidationMode::OnChange;
    
    // Templates
    std::vector<std::unique_ptr<ConfigTemplate>> templates_;
    
    // History
    ConfigHistory history_;
    
    // Auto-save
    bool autoSaveEnabled_ = false;
    std::chrono::seconds autoSaveInterval_{60};
    std::chrono::steady_clock::time_point lastAutoSave_;
    
    // Hot reloading
    bool hotReloadEnabled_ = false;
    
    // UI state
    bool uiVisible_ = false;
    std::unique_ptr<ConfigEditorUI> ui_;
    
    // Callbacks
    ValidationCallback validationCallback_;
    ChangeCallback changeCallback_;
    SaveCallback saveCallback_;
    
    // Internal methods
    void MarkDirty();
    void CheckAutoSave();
    void OnConfigChanged(const std::string& fieldPath, const simplejson::JsonValue& value);
    void LoadBuiltinTemplates();
    
    // Form generation helpers
    EditorField CreateFieldFromSchemaProperty(const std::string& name, 
                                             const schema::SchemaProperty& property);
    EditorField::Type MapSchemaTypeToEditorType(schema::SchemaProperty::Type schemaType);
    
    // JSON path utilities
    void SetValueAtPath(simplejson::JsonObject& config, const std::string& path, 
                       const simplejson::JsonValue& value);
    simplejson::JsonValue GetValueAtPath(const simplejson::JsonObject& config, 
                                        const std::string& path) const;
    std::vector<std::string> SplitPath(const std::string& path) const;
};

/**
 * Configuration Editor UI implementation (ImGui-based)
 */
class ConfigEditorUI {
public:
    ConfigEditorUI(ConfigEditor* editor);
    ~ConfigEditorUI();
    
    virtual bool Initialize() { return true; }
    virtual void Shutdown() {}
    virtual void Render();
    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }

private:
    ConfigEditor* editor_;
    bool visible_ = false;
    
    // UI state
    char searchBuffer_[256] = "";
    bool showValidationPanel_ = true;
    bool showHistoryPanel_ = false;
    bool showTemplatePanel_ = false;
    
    // Rendering methods
    void RenderMenuBar();
    void RenderToolbar();
    void RenderMainEditor();
    void RenderValidationPanel();
    void RenderHistoryPanel();
    void RenderTemplatePanel();
    void RenderStatusBar();
    
    // Form rendering
    void RenderSection(const EditorSection& section, const std::string& pathPrefix = "");
    void RenderField(const EditorField& field, const std::string& fieldPath);
    bool RenderFieldInput(const EditorField& field, const std::string& fieldPath, 
                         simplejson::JsonValue& value);
    
    // Utility methods
    void ShowFileDialog(bool save);
    void ShowTemplateDialog();
    void ShowValidationDetails();
};

} // namespace nova::config