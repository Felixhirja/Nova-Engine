#include "ConfigEditor.h"
#include "AssetPipeline.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace nova::config {

// ============================================================================
// ConfigTemplate Implementation
// ============================================================================

ConfigTemplate::ConfigTemplate(const std::string& name, const std::string& description)
    : name_(name), description_(description) {
}

void ConfigTemplate::SetBaseConfig(const simplejson::JsonObject& config) {
    baseConfig_ = config;
}

void ConfigTemplate::AddVariable(const std::string& name, const std::string& description,
                                const simplejson::JsonValue& defaultValue) {
    EditorField variable;
    variable.name = name;
    variable.displayName = name;
    variable.description = description;
    variable.defaultValue = defaultValue;
    
    // Determine type from default value
    if (defaultValue.IsString()) {
        variable.type = EditorField::Type::String;
    } else if (defaultValue.IsNumber()) {
        variable.type = EditorField::Type::Number;
    } else if (defaultValue.IsBoolean()) {
        variable.type = EditorField::Type::Boolean;
    } else {
        variable.type = EditorField::Type::String; // Default fallback
    }
    
    variables_.push_back(variable);
}

simplejson::JsonObject ConfigTemplate::Generate(
    const std::unordered_map<std::string, simplejson::JsonValue>& variables) const {
    
    simplejson::JsonObject result = baseConfig_;
    
    // Replace template variables in the config
    for (const auto& [varName, varValue] : variables) {
        // Simple variable replacement - in a real implementation,
        // you'd want a more sophisticated template engine
        // For now, just set direct properties
        result[varName] = varValue;
    }
    
    return result;
}

// ============================================================================
// ConfigHistory Implementation
// ============================================================================

void ConfigHistory::PushEdit(const std::string& action,
                            const simplejson::JsonObject& before,
                            const simplejson::JsonObject& after) {
    // Remove any redo entries if we're in the middle of history
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }
    
    HistoryEntry entry;
    entry.action = action;
    entry.configBefore = before;
    entry.configAfter = after;
    entry.timestamp = std::chrono::steady_clock::now();
    
    history_.push_back(entry);
    currentIndex_ = history_.size();
    
    // Limit history size
    if (history_.size() > MAX_HISTORY) {
        history_.erase(history_.begin());
        currentIndex_ = history_.size();
    }
}

const ConfigHistory::HistoryEntry* ConfigHistory::Undo() {
    if (!CanUndo()) return nullptr;
    
    currentIndex_--;
    return &history_[currentIndex_];
}

const ConfigHistory::HistoryEntry* ConfigHistory::Redo() {
    if (!CanRedo()) return nullptr;
    
    const auto* entry = &history_[currentIndex_];
    currentIndex_++;
    return entry;
}

void ConfigHistory::Clear() {
    history_.clear();
    currentIndex_ = 0;
}

// ============================================================================
// ConfigEditor Implementation
// ============================================================================

ConfigEditor::ConfigEditor() : ui_(std::make_unique<ConfigEditorUI>(this)) {
    LoadBuiltinTemplates();
}

ConfigEditor::~ConfigEditor() = default;

bool ConfigEditor::Initialize() {
    std::cout << "[ConfigEditor] Initializing visual configuration editor..." << std::endl;
    
    // Initialize validation system
    if (!schema::SchemaRegistry::Instance().GetSchemaIds().empty()) {
        std::cout << "[ConfigEditor] Found " << schema::SchemaRegistry::Instance().GetSchemaIds().size() 
                  << " registered schemas" << std::endl;
    }
    
    // Set up auto-save timer
    lastAutoSave_ = std::chrono::steady_clock::now();
    
    std::cout << "[ConfigEditor] Configuration editor initialized successfully" << std::endl;
    return true;
}

void ConfigEditor::Shutdown() {
    if (hasUnsavedChanges_) {
        std::cout << "[ConfigEditor] Warning: Shutting down with unsaved changes" << std::endl;
    }
    
    // Save any pending auto-save
    if (autoSaveEnabled_ && hasUnsavedChanges_ && !currentFilePath_.empty()) {
        SaveConfig();
    }
}

void ConfigEditor::Update(double deltaTime [[maybe_unused]]) {
    // Check for auto-save
    if (autoSaveEnabled_) {
        CheckAutoSave();
    }
    
    // Perform continuous validation if enabled
    if (validationMode_ == ValidationMode::Continuous && hasUnsavedChanges_) {
        ValidateCurrentConfig();
    }
}

bool ConfigEditor::LoadConfig(const std::string& filePath, const std::string& schemaId) {
    std::cout << "[ConfigEditor] Loading config from: " << filePath << std::endl;
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ConfigEditor] Failed to open file: " << filePath << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    auto parseResult = simplejson::Parse(content);
    if (!parseResult.success) {
        std::cerr << "[ConfigEditor] Failed to parse JSON: " << parseResult.errorMessage << std::endl;
        return false;
    }
    
    if (!parseResult.value.IsObject()) {
        std::cerr << "[ConfigEditor] JSON root is not an object" << std::endl;
        return false;
    }
    
    // Store previous config for history
    simplejson::JsonObject previousConfig = currentConfig_;
    
    currentConfig_ = parseResult.value.AsObject();
    currentFilePath_ = filePath;
    hasUnsavedChanges_ = false;
    
    // Determine schema if not provided
    if (!schemaId.empty()) {
        currentSchemaId_ = schemaId;
    } else {
        // Try to infer schema from config or filename
        auto it = currentConfig_.find("entityType");
        if (it != currentConfig_.end() && it->second.IsString()) {
            currentSchemaId_ = it->second.AsString() + "_config";
        }
    }
    
    // Load schema and generate form
    if (!currentSchemaId_.empty()) {
        LoadSchema(currentSchemaId_);
    }
    
    // Add to history
    if (!previousConfig.empty()) {
        history_.PushEdit("Load Config", previousConfig, currentConfig_);
    }
    
    // Validate if enabled
    if (validationMode_ != ValidationMode::None) {
        ValidateCurrentConfig();
    }
    
    std::cout << "[ConfigEditor] Successfully loaded config with " << currentConfig_.size() 
              << " properties" << std::endl;
    return true;
}

bool ConfigEditor::SaveConfig(const std::string& filePath) {
    std::string targetPath = filePath.empty() ? currentFilePath_ : filePath;
    
    if (targetPath.empty()) {
        std::cerr << "[ConfigEditor] No file path specified for save" << std::endl;
        return false;
    }
    
    // Validate before saving if enabled
    if (validationMode_ == ValidationMode::OnSave) {
        if (!ValidateCurrentConfig() || !lastValidation_.success) {
            std::cerr << "[ConfigEditor] Cannot save: validation failed" << std::endl;
            return false;
        }
    }
    
    std::ofstream file(targetPath);
    if (!file.is_open()) {
        std::cerr << "[ConfigEditor] Failed to open file for writing: " << targetPath << std::endl;
        return false;
    }
    
    // Convert config to JSON string
    auto jsonValue = simplejson::JsonValue(currentConfig_);
    file << simplejson::Serialize(jsonValue, true); // Pretty print
    file.close();
    
    currentFilePath_ = targetPath;
    hasUnsavedChanges_ = false;
    lastAutoSave_ = std::chrono::steady_clock::now();
    
    // Trigger hot reload if enabled
    if (hotReloadEnabled_) {
        // Integration with asset pipeline hot reloading
        // AssetPipeline::HotReloadManager::GetInstance().NotifyFileChanged(targetPath);
    }
    
    // Trigger save callback
    if (saveCallback_) {
        saveCallback_(targetPath);
    }
    
    std::cout << "[ConfigEditor] Successfully saved config to: " << targetPath << std::endl;
    return true;
}

bool ConfigEditor::NewConfig(const std::string& schemaId) {
    std::cout << "[ConfigEditor] Creating new config with schema: " << schemaId << std::endl;
    
    // Store previous config for history
    simplejson::JsonObject previousConfig = currentConfig_;
    
    currentConfig_.clear();
    currentSchemaId_ = schemaId;
    currentFilePath_.clear();
    hasUnsavedChanges_ = true;
    
    // Load schema and generate form
    if (!LoadSchema(schemaId)) {
        std::cerr << "[ConfigEditor] Failed to load schema: " << schemaId << std::endl;
        return false;
    }
    
    // Add to history
    if (!previousConfig.empty()) {
        history_.PushEdit("New Config", previousConfig, currentConfig_);
    }
    
    return true;
}

bool ConfigEditor::LoadSchema(const std::string& schemaId) {
    currentSchemaId_ = schemaId;
    currentForm_ = GenerateFormFromSchema(schemaId);
    
    if (currentForm_.fields.empty() && currentForm_.subsections.empty()) {
        std::cerr << "[ConfigEditor] No form generated for schema: " << schemaId << std::endl;
        return false;
    }
    
    std::cout << "[ConfigEditor] Generated form with " << currentForm_.fields.size() 
              << " fields and " << currentForm_.subsections.size() << " sections" << std::endl;
    return true;
}

EditorSection ConfigEditor::GenerateFormFromSchema(const std::string& schemaId) {
    EditorSection root;
    root.name = "root";
    root.displayName = "Configuration";
    
    auto* schema = schema::SchemaRegistry::Instance().GetSchema(schemaId);
    if (!schema) {
        std::cerr << "[ConfigEditor] Schema not found: " << schemaId << std::endl;
        return root;
    }
    
    // This is a simplified implementation - in practice, you'd need to parse
    // the schema properties and create appropriate form fields
    // For now, create some common sections based on known schema patterns
    
    // Basic properties section
    EditorSection basic;
    basic.name = "basic";
    basic.displayName = "Basic Properties";
    basic.description = "Fundamental configuration properties";
    
    // Add common fields
    EditorField nameField;
    nameField.name = "name";
    nameField.displayName = "Name";
    nameField.description = "Display name of the entity";
    nameField.type = EditorField::Type::String;
    nameField.required = true;
    basic.fields.push_back(nameField);
    
    EditorField descField;
    descField.name = "description";
    descField.displayName = "Description";
    descField.description = "Description of the entity's purpose";
    descField.type = EditorField::Type::String;
    basic.fields.push_back(descField);
    
    root.subsections.push_back(basic);
    
    // Gameplay section
    EditorSection gameplay;
    gameplay.name = "gameplay";
    gameplay.displayName = "Gameplay Properties";
    gameplay.description = "Gameplay-related configuration";
    
    EditorField healthField;
    healthField.name = "health";
    healthField.displayName = "Health";
    healthField.description = "Maximum health points";
    healthField.type = EditorField::Type::Number;
    healthField.minValue = 0;
    healthField.maxValue = 100000;
    healthField.defaultValue = simplejson::JsonValue(100.0);
    gameplay.fields.push_back(healthField);
    
    root.subsections.push_back(gameplay);
    
    return root;
}

void ConfigEditor::RegisterTemplate(std::unique_ptr<ConfigTemplate> configTemplate) {
    templates_.push_back(std::move(configTemplate));
}

bool ConfigEditor::LoadFromTemplate(const std::string& templateName,
                                   const std::unordered_map<std::string, simplejson::JsonValue>& variables) {
    auto it = std::find_if(templates_.begin(), templates_.end(),
        [&templateName](const auto& tmpl) { return tmpl->GetName() == templateName; });
    
    if (it == templates_.end()) {
        std::cerr << "[ConfigEditor] Template not found: " << templateName << std::endl;
        return false;
    }
    
    // Store previous config for history
    simplejson::JsonObject previousConfig = currentConfig_;
    
    currentConfig_ = (*it)->Generate(variables);
    hasUnsavedChanges_ = true;
    
    // Add to history
    history_.PushEdit("Load Template: " + templateName, previousConfig, currentConfig_);
    
    // Validate if enabled
    if (validationMode_ != ValidationMode::None) {
        ValidateCurrentConfig();
    }
    
    std::cout << "[ConfigEditor] Loaded template: " << templateName << std::endl;
    return true;
}

bool ConfigEditor::ValidateCurrentConfig() {
    if (currentSchemaId_.empty()) {
        std::cout << "[ConfigEditor] No schema loaded for validation" << std::endl;
        return true; // No schema means no validation needed
    }
    
    auto* schema = schema::SchemaRegistry::Instance().GetSchema(currentSchemaId_);
    if (!schema) {
        std::cerr << "[ConfigEditor] Schema not found for validation: " << currentSchemaId_ << std::endl;
        return false;
    }
    
    auto jsonValue = simplejson::JsonValue(currentConfig_);
    lastValidation_ = schema->Validate(jsonValue);
    
    // Trigger validation callback
    if (validationCallback_) {
        validationCallback_(lastValidation_);
    }
    
    if (lastValidation_.success) {
        std::cout << "[ConfigEditor] Validation passed" << std::endl;
    } else {
        std::cout << "[ConfigEditor] Validation failed with " << lastValidation_.errors.size() 
                  << " errors" << std::endl;
    }
    
    return lastValidation_.success;
}

bool ConfigEditor::Undo() {
    const auto* entry = history_.Undo();
    if (!entry) return false;
    
    currentConfig_ = entry->configBefore;
    hasUnsavedChanges_ = true;
    
    std::cout << "[ConfigEditor] Undid: " << entry->action << std::endl;
    return true;
}

bool ConfigEditor::Redo() {
    const auto* entry = history_.Redo();
    if (!entry) return false;
    
    currentConfig_ = entry->configAfter;
    hasUnsavedChanges_ = true;
    
    std::cout << "[ConfigEditor] Redid: " << entry->action << std::endl;
    return true;
}

bool ConfigEditor::CanUndo() const {
    return history_.CanUndo();
}

bool ConfigEditor::CanRedo() const {
    return history_.CanRedo();
}

bool ConfigEditor::SetFieldValue(const std::string& fieldPath, const simplejson::JsonValue& value) {
    simplejson::JsonObject previousConfig = currentConfig_;
    
    SetValueAtPath(currentConfig_, fieldPath, value);
    hasUnsavedChanges_ = true;
    
    // Add to history
    history_.PushEdit("Edit Field: " + fieldPath, previousConfig, currentConfig_);
    
    // Trigger change callback
    OnConfigChanged(fieldPath, value);
    
    // Validate if enabled
    if (validationMode_ == ValidationMode::OnChange) {
        ValidateCurrentConfig();
    }
    
    return true;
}

simplejson::JsonValue ConfigEditor::GetFieldValue(const std::string& fieldPath) const {
    return GetValueAtPath(currentConfig_, fieldPath);
}

void ConfigEditor::RenderUI() {
    if (ui_ && uiVisible_) {
        ui_->Render();
    }
}

void ConfigEditor::SetCustomUI(std::unique_ptr<ConfigEditorUI> ui) {
    if (!ui) return;
    // Shutdown existing UI if present
    if (ui_) {
        ui_->Shutdown();
    }
    ui_ = std::move(ui);
    // Initialize the new UI if possible
    ui_->Initialize();
}

void ConfigEditor::MarkDirty() {
    hasUnsavedChanges_ = true;
}

void ConfigEditor::CheckAutoSave() {
    if (!autoSaveEnabled_ || !hasUnsavedChanges_ || currentFilePath_.empty()) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now - lastAutoSave_ >= autoSaveInterval_) {
        std::cout << "[ConfigEditor] Performing auto-save..." << std::endl;
        SaveConfig();
    }
}

void ConfigEditor::OnConfigChanged(const std::string& fieldPath, const simplejson::JsonValue& value) {
    if (changeCallback_) {
        changeCallback_(fieldPath, value);
    }
}

void ConfigEditor::LoadBuiltinTemplates() {
    // Station template
    auto stationTemplate = std::make_unique<ConfigTemplate>(
        "Basic Station", "Template for creating basic station configurations");
    
    simplejson::JsonObject stationBase;
    stationBase["entityType"] = simplejson::JsonValue(std::string("station"));
    stationBase["category"] = simplejson::JsonValue(std::string("world"));
    stationBase["type"] = simplejson::JsonValue(std::string("trading"));
    stationBase["dockingCapacity"] = simplejson::JsonValue(5.0);
    
    stationTemplate->SetBaseConfig(stationBase);
    stationTemplate->AddVariable("name", "Station name", simplejson::JsonValue(std::string("New Station")));
    stationTemplate->AddVariable("description", "Station description", 
                                simplejson::JsonValue(std::string("A new trading station")));
    
    RegisterTemplate(std::move(stationTemplate));
    
    // Ship template
    auto shipTemplate = std::make_unique<ConfigTemplate>(
        "Basic Ship", "Template for creating basic ship configurations");
    
    simplejson::JsonObject shipBase;
    shipBase["entityType"] = simplejson::JsonValue(std::string("ship"));
    shipBase["category"] = simplejson::JsonValue(std::string("ship"));
    
    simplejson::JsonObject gameplay;
    gameplay["health"] = simplejson::JsonValue(100.0);
    gameplay["speed"] = simplejson::JsonValue(50.0);
    shipBase["gameplay"] = simplejson::JsonValue(gameplay);
    
    shipTemplate->SetBaseConfig(shipBase);
    shipTemplate->AddVariable("name", "Ship name", simplejson::JsonValue(std::string("New Ship")));
    shipTemplate->AddVariable("shipClass", "Ship class", simplejson::JsonValue(std::string("fighter")));
    
    RegisterTemplate(std::move(shipTemplate));
    
    std::cout << "[ConfigEditor] Loaded " << templates_.size() << " built-in templates" << std::endl;
}

void ConfigEditor::SetValueAtPath(simplejson::JsonObject& config, const std::string& path,
                                 const simplejson::JsonValue& value) {
    auto pathParts = SplitPath(path);
    
    simplejson::JsonObject* current = &config;
    
    // Navigate to the parent object
    for (size_t i = 0; i < pathParts.size() - 1; ++i) {
        auto it = current->find(pathParts[i]);
        if (it == current->end() || !it->second.IsObject()) {
            // Create intermediate objects as needed
            (*current)[pathParts[i]] = simplejson::JsonValue(simplejson::JsonObject{});
        }
        current = &(*current)[pathParts[i]].AsObject();
    }
    
    // Set the final value
    (*current)[pathParts.back()] = value;
}

simplejson::JsonValue ConfigEditor::GetValueAtPath(const simplejson::JsonObject& config,
                                                  const std::string& path) const {
    auto pathParts = SplitPath(path);
    
    const simplejson::JsonObject* current = &config;
    
    for (size_t i = 0; i < pathParts.size() - 1; ++i) {
        auto it = current->find(pathParts[i]);
        if (it == current->end() || !it->second.IsObject()) {
            return simplejson::JsonValue(); // null
        }
        current = &it->second.AsObject();
    }
    
    auto it = current->find(pathParts.back());
    return (it != current->end()) ? it->second : simplejson::JsonValue();
}

std::vector<std::string> ConfigEditor::SplitPath(const std::string& path) const {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

// ============================================================================
// ConfigEditorUI Implementation (Basic stub - would need ImGui)
// ============================================================================

ConfigEditorUI::ConfigEditorUI(ConfigEditor* editor) : editor_(editor) {
}

ConfigEditorUI::~ConfigEditorUI() = default;

void ConfigEditorUI::Render() {
    // This would be implemented with ImGui or another UI library
    // For now, just a placeholder that shows we're rendering
    if (!visible_) return;
    
    // In a real implementation, this would render:
    // - Menu bar with File, Edit, View options
    // - Toolbar with New, Load, Save, Validate buttons
    // - Main editor area with form fields
    // - Validation panel showing errors/warnings
    // - Status bar with file info and validation status
    
    std::cout << "[ConfigEditorUI] Rendering UI (placeholder)" << std::endl;
}

void ConfigEditorUI::RenderMenuBar() {
    // Menu bar implementation
}

void ConfigEditorUI::RenderToolbar() {
    // Toolbar implementation
}

void ConfigEditorUI::RenderMainEditor() {
    // Main editor form implementation
}

void ConfigEditorUI::RenderValidationPanel() {
    // Validation results panel
}

void ConfigEditorUI::RenderHistoryPanel() {
    // Undo/redo history panel
}

void ConfigEditorUI::RenderTemplatePanel() {
    // Template selection panel
}

void ConfigEditorUI::RenderStatusBar() {
    // Status bar implementation
}

} // namespace nova::config