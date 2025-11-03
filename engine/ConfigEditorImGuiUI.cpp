#include "ConfigEditorImGuiUI.h"
#include "ConfigEditor.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>

// ImGui includes - these would need to be properly set up in your project
#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#else
// Stub implementations for when ImGui is not available
struct ImVec2 { float x, y; ImVec2(float x = 0, float y = 0) : x(x), y(y) {} };
struct ImVec4 { float x, y, z, w; ImVec4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {} };
#define IM_COL32(R,G,B,A) 0
#endif

namespace nova::config {

ConfigEditorImGuiUI::ConfigEditorImGuiUI(ConfigEditor* editor)
    : ConfigEditorUI(editor)
    , editor_(editor) {
    LoadState();
}

ConfigEditorImGuiUI::~ConfigEditorImGuiUI() {
    SaveState();
}

bool ConfigEditorImGuiUI::Initialize() {
    std::cout << "[ConfigEditorUI] Initializing ImGui-based UI..." << std::endl;
    
    #ifdef USE_IMGUI
    // Create ImGui context and initialize backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls

    // Initialize platform/renderer backends. Use current GLFW context if available.
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) {
        // Fallback: try to find a window via viewport if available (not accessible here)
        std::cerr << "[ConfigEditorUI] Warning: no current GLFW context for ImGui_Init" << std::endl;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // Use a reasonable GLSL version string; the project should match its GL setup
    ImGui_ImplOpenGL3_Init("#version 130");

    // Set up ImGui style and fonts
    SetupColors();
    SetupFonts();
    ApplyTheme(currentTheme_);

    std::cout << "[ConfigEditorUI] ImGui UI initialized successfully" << std::endl;
    return true;
    #else
    std::cout << "[ConfigEditorUI] ImGui not available, using stub implementation" << std::endl;
    return true;
    #endif
}

void ConfigEditorImGuiUI::Shutdown() {
    SaveState();
#ifdef USE_IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
}

void ConfigEditorImGuiUI::Render() {
    if (!visible_ || !editor_) return;
    
    #ifdef USE_IMGUI
    // Handle keyboard shortcuts first
    HandleKeyboardShortcuts();
    
    // Update notifications
    UpdateNotifications(ImGui::GetIO().DeltaTime);
    
    // Update performance metrics
    UpdateMetrics();
    
    // Render main window
    RenderMainWindow();
    
    // Render floating dialogs
    RenderFileDialogs();
    RenderNewConfigDialog();
    RenderTemplateDialog();
    RenderValidationDetailsDialog();
    
    // Render notifications
    RenderNotifications();
    
    // Handle drag and drop
    HandleDragDrop();
    #else
    // Stub rendering for non-ImGui builds
    std::cout << "[ConfigEditorUI] Rendering UI (ImGui not available)" << std::endl;
    #endif
}

#ifdef USE_IMGUI

void ConfigEditorImGuiUI::RenderMainWindow() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    #ifdef IMGUI_HAS_VIEWPORT
    ImGui::SetNextWindowViewport(viewport->ID);
    #endif
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_MenuBar;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    bool open = true;
    if (ImGui::Begin("Nova Engine Config Editor", &open, windowFlags)) {
        if (!open) {
            visible_ = false;
        }
        
        // Menu bar
        RenderMenuBar();
        
        // Toolbar
        RenderToolbar();
        
        // Main content area
        ImGui::BeginChild("ContentArea", ImVec2(0, -25)); // Leave space for status bar
        {
            // Left panel
            ImGui::BeginChild("LeftPanel", ImVec2(state_.leftPanelWidth, 0), true);
            RenderLeftPanel();
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // Center panel
            float centerWidth = ImGui::GetContentRegionAvail().x;
            if (state_.showValidationPanel || state_.showHistoryPanel) {
                centerWidth -= state_.rightPanelWidth + ImGui::GetStyle().ItemSpacing.x;
            }
            
            ImGui::BeginChild("CenterPanel", ImVec2(centerWidth, 0), true);
            RenderCenterPanel();
            ImGui::EndChild();
            
            // Right panel (conditional)
            if (state_.showValidationPanel || state_.showHistoryPanel) {
                ImGui::SameLine();
                ImGui::BeginChild("RightPanel", ImVec2(state_.rightPanelWidth, 0), true);
                RenderRightPanel();
                ImGui::EndChild();
            }
        }
        ImGui::EndChild();
        
        // Status bar
        RenderStatusBar();
    }
    ImGui::End();
    
    ImGui::PopStyleVar(3);
}

void ConfigEditorImGuiUI::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Config", "Ctrl+N")) {
                state_.showNewConfigDialog = true;
            }
            if (ImGui::MenuItem("Open Config", "Ctrl+O")) {
                state_.showOpenDialog = true;
            }
            
            ImGui::Separator();
            
            bool hasFile = !editor_->GetCurrentFile().empty();
            if (ImGui::MenuItem("Save", "Ctrl+S", false, hasFile)) {
                editor_->SaveConfig();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, hasFile)) {
                state_.showSaveDialog = true;
            }
            
            ImGui::Separator();
            
            // Recent files
            if (ImGui::BeginMenu("Recent Files")) {
                auto recentFiles = GetRecentFiles();
                for (const auto& file : recentFiles) {
                    if (ImGui::MenuItem(file.c_str())) {
                        LoadFile(file);
                    }
                }
                if (recentFiles.empty()) {
                    ImGui::TextDisabled("No recent files");
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                visible_ = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            bool canUndo = editor_->CanUndo();
            bool canRedo = editor_->CanRedo();
            
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
                editor_->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
                editor_->Redo();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Clear History")) {
                editor_->ClearHistory();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Validation Panel", nullptr, &state_.showValidationPanel);
            ImGui::MenuItem("History Panel", nullptr, &state_.showHistoryPanel);
            ImGui::MenuItem("Template Panel", nullptr, &state_.showTemplatePanel);
            
            ImGui::Separator();
            
            ImGui::MenuItem("JSON Editor", nullptr, &state_.showJsonEditor);
            ImGui::MenuItem("Live Preview", nullptr, &state_.showLivePreview);
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, currentTheme_ == Theme::Dark)) {
                    ApplyTheme(Theme::Dark);
                }
                if (ImGui::MenuItem("Light", nullptr, currentTheme_ == Theme::Light)) {
                    ApplyTheme(Theme::Light);
                }
                if (ImGui::MenuItem("Classic", nullptr, currentTheme_ == Theme::Classic)) {
                    ApplyTheme(Theme::Classic);
                }
                if (ImGui::MenuItem("Nova", nullptr, currentTheme_ == Theme::Nova)) {
                    ApplyTheme(Theme::Nova);
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Validate Config", "F5")) {
                editor_->ValidateCurrentConfig();
            }
            
            ImGui::Separator();
            
            ImGui::MenuItem("Auto Validate", nullptr, &state_.autoValidate);
            
            if (ImGui::MenuItem("Format JSON")) {
                // Format current JSON
                auto jsonStr = simplejson::Serialize(simplejson::JsonValue(editor_->GetCurrentConfig()), true);
                // Update editor with formatted JSON
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Reset UI Layout")) {
                ResetUIState();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Show about dialog
            }
            if (ImGui::MenuItem("Keyboard Shortcuts")) {
                // Show shortcuts help
            }
            if (ImGui::MenuItem("Documentation")) {
                // Open documentation
            }
            
            ImGui::Separator();
            
            ImGui::MenuItem("Show Demo Window", nullptr, &state_.showDemoWindow);
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void ConfigEditorImGuiUI::RenderToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    
    if (ImGui::Button("New")) {
        state_.showNewConfigDialog = true;
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Open")) {
        state_.showOpenDialog = true;
    }
    ImGui::SameLine();
    
    bool hasFile = !editor_->GetCurrentFile().empty();
    if (ImGui::Button("Save") && hasFile) {
        editor_->SaveConfig();
    }
    ImGui::SameLine();
    
    ImGui::Separator();
    ImGui::SameLine();
    
    bool canUndo = editor_->CanUndo();
    bool canRedo = editor_->CanRedo();
    
    if (ImGui::Button("Undo") && canUndo) {
        editor_->Undo();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Redo") && canRedo) {
        editor_->Redo();
    }
    ImGui::SameLine();
    
    ImGui::Separator();
    ImGui::SameLine();
    
    if (ImGui::Button("Validate")) {
        editor_->ValidateCurrentConfig();
    }
    ImGui::SameLine();
    
    // Validation status indicator
    const auto& validation = editor_->GetLastValidation();
    if (validation.success) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Valid");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%zu validation errors", validation.errors.size());
            ImGui::EndTooltip();
        }
    }
    
    ImGui::PopStyleVar(2);
}

void ConfigEditorImGuiUI::RenderLeftPanel() {
    if (ImGui::CollapsingHeader("Schema & Type", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderSchemaSelector();
    }
    
    if (ImGui::CollapsingHeader("Recent Files")) {
        RenderRecentFiles();
    }
    
    if (ImGui::CollapsingHeader("Templates")) {
        RenderTemplateList();
    }
}

void ConfigEditorImGuiUI::RenderCenterPanel() {
    if (state_.showJsonEditor) {
        if (ImGui::BeginTabBar("EditorTabs")) {
            if (ImGui::BeginTabItem("Visual Editor")) {
                RenderFormEditor();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("JSON Editor")) {
                RenderJsonEditor();
                ImGui::EndTabItem();
            }
            
            if (state_.showLivePreview && ImGui::BeginTabItem("Live Preview")) {
                RenderLivePreview();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    } else {
        RenderFormEditor();
    }
}

void ConfigEditorImGuiUI::RenderRightPanel() {
    if (state_.showValidationPanel && ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderValidationPanel();
    }
    
    if (state_.showHistoryPanel && ImGui::CollapsingHeader("History")) {
        RenderHistoryPanel();
    }
}

void ConfigEditorImGuiUI::RenderFormEditor() {
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);
    
    if (editor_->GetCurrentConfig().empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No configuration loaded");
        ImGui::Text("Use File > New or File > Open to begin editing");
    } else {
        // Render search bar
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search fields...", state_.searchBuffer, sizeof(state_.searchBuffer));
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Render form sections
        const auto& form = editor_->GetCurrentForm();
        for (const auto& section : form.subsections) {
            RenderSection(section);
        }
        
        // Render root-level fields
        for (const auto& field : form.fields) {
            RenderField(field, field.name);
        }
    }
    
    ImGui::PopStyleVar();
}

void ConfigEditorImGuiUI::RenderSection(const EditorSection& section, const std::string& pathPrefix) {
    std::string sectionPath = pathPrefix.empty() ? section.name : pathPrefix + "." + section.name;
    
    // Check if section matches search
    bool matchesSearch = strlen(state_.searchBuffer) == 0 || 
                        section.displayName.find(state_.searchBuffer) != std::string::npos ||
                        section.description.find(state_.searchBuffer) != std::string::npos;
    
    if (!matchesSearch) return;
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!section.collapsible) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    
    bool isOpen = ImGui::TreeNodeEx(section.displayName.c_str(), flags);
    
    // Show description as tooltip
    if (ImGui::IsItemHovered() && !section.description.empty()) {
        RenderTooltip(section.description);
    }
    
    if (isOpen) {
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 15.0f);
        
        // Render fields in this section
        for (const auto& field : section.fields) {
            std::string fieldPath = sectionPath.empty() ? field.name : sectionPath + "." + field.name;
            RenderField(field, fieldPath);
        }
        
        // Render subsections
        for (const auto& subsection : section.subsections) {
            RenderSection(subsection, sectionPath);
        }
        
        ImGui::PopStyleVar();
        ImGui::TreePop();
    }
}

void ConfigEditorImGuiUI::RenderField(const EditorField& field, const std::string& fieldPath) {
    // Check if field matches search
    bool matchesSearch = strlen(state_.searchBuffer) == 0 || 
                        field.displayName.find(state_.searchBuffer) != std::string::npos ||
                        field.description.find(state_.searchBuffer) != std::string::npos ||
                        field.name.find(state_.searchBuffer) != std::string::npos;
    
    if (!matchesSearch) return;
    
    ImGui::PushID(fieldPath.c_str());
    
    // Apply field styling
    PushFieldStyle(field);
    
    // Render the field based on its type
    bool changed = RenderFieldInput(field, fieldPath);
    
    // Show validation status
    if (state_.autoValidate) {
        auto currentValue = GetFieldValue(fieldPath);
        bool isValid = true;
        std::string error;
        
        if (field.validator) {
            isValid = field.validator(currentValue);
            if (!isValid) {
                error = "Custom validation failed";
            }
        }
        
        ImGui::SameLine();
        RenderValidationIcon(isValid, error);
    }
    
    // Show help marker for description
    if (!field.description.empty()) {
        ImGui::SameLine();
        RenderHelpMarker(field.description);
    }
    
    // Handle field changes
    if (changed && state_.autoValidate) {
        editor_->ValidateCurrentConfig();
    }
    
    PopFieldStyle();
    ImGui::PopID();
}

bool ConfigEditorImGuiUI::RenderFieldInput(const EditorField& field, const std::string& fieldPath) {
    switch (field.type) {
        case EditorField::Type::String:
            RenderStringField(field, fieldPath);
            return true;
            
        case EditorField::Type::Number:
            RenderNumberField(field, fieldPath);
            return true;
            
        case EditorField::Type::Boolean:
            RenderBooleanField(field, fieldPath);
            return true;
            
        case EditorField::Type::Enum:
            RenderEnumField(field, fieldPath);
            return true;
            
        case EditorField::Type::Array:
            RenderArrayField(field, fieldPath);
            return true;
            
        case EditorField::Type::Object:
            RenderObjectField(field, fieldPath);
            return true;
            
        case EditorField::Type::Color:
            RenderColorField(field, fieldPath);
            return true;
            
        case EditorField::Type::Vector3:
            RenderVector3Field(field, fieldPath);
            return true;
            
        case EditorField::Type::File:
            RenderFileField(field, fieldPath);
            return true;
            
        default:
            ImGui::TextDisabled("Unsupported field type");
            return false;
    }
}

void ConfigEditorImGuiUI::RenderStringField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    std::string strValue = value.IsString() ? value.AsString() : "";
    
    char buffer[1024];
    strncpy(buffer, strValue.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    ImGui::Text("%s", field.displayName.c_str());
    if (field.required) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "*");
    }
    
    ImGui::PushItemWidth(-1);
    if (ImGui::InputText(("##" + fieldPath).c_str(), buffer, sizeof(buffer))) {
        SetFieldValue(fieldPath, simplejson::JsonValue(std::string(buffer)));
    }
    ImGui::PopItemWidth();
}

void ConfigEditorImGuiUI::RenderNumberField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    double numValue = value.IsNumber() ? value.AsNumber() : 0.0;
    
    ImGui::Text("%s", field.displayName.c_str());
    if (field.required) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "*");
    }
    
    ImGui::PushItemWidth(-1);
    
    // Use appropriate input based on range
    if (field.minValue != field.maxValue) {
        float floatValue = static_cast<float>(numValue);
        if (ImGui::SliderFloat(("##" + fieldPath).c_str(), &floatValue, 
                              static_cast<float>(field.minValue), 
                              static_cast<float>(field.maxValue))) {
            SetFieldValue(fieldPath, simplejson::JsonValue(static_cast<double>(floatValue)));
        }
    } else {
        double doubleValue = numValue;
        if (ImGui::InputDouble(("##" + fieldPath).c_str(), &doubleValue)) {
            SetFieldValue(fieldPath, simplejson::JsonValue(doubleValue));
        }
    }
    
    ImGui::PopItemWidth();
}

void ConfigEditorImGuiUI::RenderBooleanField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    bool boolValue = value.IsBoolean() ? value.AsBoolean() : false;
    
    if (ImGui::Checkbox(field.displayName.c_str(), &boolValue)) {
        SetFieldValue(fieldPath, simplejson::JsonValue(boolValue));
    }
}

void ConfigEditorImGuiUI::RenderEnumField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    std::string currentValue = value.IsString() ? value.AsString() : "";
    
    ImGui::Text("%s", field.displayName.c_str());
    if (field.required) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "*");
    }
    
    ImGui::PushItemWidth(-1);
    if (ImGui::BeginCombo(("##" + fieldPath).c_str(), currentValue.c_str())) {
        for (const auto& option : field.enumValues) {
            bool isSelected = (currentValue == option);
            if (ImGui::Selectable(option.c_str(), isSelected)) {
                SetFieldValue(fieldPath, simplejson::JsonValue(option));
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

void ConfigEditorImGuiUI::RenderArrayField(const EditorField& field, const std::string& fieldPath) {
    ImGui::Text("%s", field.displayName.c_str());
    
    auto value = GetFieldValue(fieldPath);
    simplejson::JsonArray array;
    if (value.IsArray()) {
        array = value.AsArray();
    }
    
    ImGui::Indent();
    
    // Render array items
    for (size_t i = 0; i < array.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        
        std::string itemPath = fieldPath + "[" + std::to_string(i) + "]";
        ImGui::Text("Item %zu:", i);
        ImGui::SameLine();
        
        // Simple string input for array items (could be enhanced for complex types)
        std::string itemValue = array[i].IsString() ? array[i].AsString() : "";
        char buffer[256];
        strncpy(buffer, itemValue.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        
        ImGui::PushItemWidth(-50);
        if (ImGui::InputText("##item", buffer, sizeof(buffer))) {
            array[i] = simplejson::JsonValue(std::string(buffer));
            SetFieldValue(fieldPath, simplejson::JsonValue(array));
        }
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            array.erase(array.begin() + i);
            SetFieldValue(fieldPath, simplejson::JsonValue(array));
        }
        
        ImGui::PopID();
    }
    
    // Add new item button
    if (ImGui::Button("Add Item")) {
        array.push_back(simplejson::JsonValue(std::string("")));
        SetFieldValue(fieldPath, simplejson::JsonValue(array));
    }
    
    ImGui::Unindent();
}

void ConfigEditorImGuiUI::RenderObjectField(const EditorField& field, const std::string& fieldPath) {
    ImGui::Text("%s", field.displayName.c_str());
    
    // For object fields, we'd recursively render the object's properties
    // This is a simplified implementation
    auto value = GetFieldValue(fieldPath);
    if (value.IsObject()) {
        ImGui::Indent();
        ImGui::TextDisabled("Object editing not fully implemented");
        ImGui::Unindent();
    }
}

void ConfigEditorImGuiUI::RenderColorField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    float color[3] = {1.0f, 1.0f, 1.0f};
    
    if (value.IsArray()) {
        auto arr = value.AsArray();
        if (arr.size() >= 3) {
            color[0] = arr[0].IsNumber() ? static_cast<float>(arr[0].AsNumber()) : 1.0f;
            color[1] = arr[1].IsNumber() ? static_cast<float>(arr[1].AsNumber()) : 1.0f;
            color[2] = arr[2].IsNumber() ? static_cast<float>(arr[2].AsNumber()) : 1.0f;
        }
    }
    
    ImGui::Text("%s", field.displayName.c_str());
    ImGui::PushItemWidth(-1);
    if (ImGui::ColorEdit3(("##" + fieldPath).c_str(), color)) {
        simplejson::JsonArray colorArray;
        colorArray.push_back(simplejson::JsonValue(static_cast<double>(color[0])));
        colorArray.push_back(simplejson::JsonValue(static_cast<double>(color[1])));
        colorArray.push_back(simplejson::JsonValue(static_cast<double>(color[2])));
        SetFieldValue(fieldPath, simplejson::JsonValue(colorArray));
    }
    ImGui::PopItemWidth();
}

void ConfigEditorImGuiUI::RenderVector3Field(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    float vec[3] = {0.0f, 0.0f, 0.0f};
    
    if (value.IsObject()) {
        auto obj = value.AsObject();
        auto x = obj.find("x");
        auto y = obj.find("y");
        auto z = obj.find("z");
        
        if (x != obj.end() && x->second.IsNumber()) vec[0] = static_cast<float>(x->second.AsNumber());
        if (y != obj.end() && y->second.IsNumber()) vec[1] = static_cast<float>(y->second.AsNumber());
        if (z != obj.end() && z->second.IsNumber()) vec[2] = static_cast<float>(z->second.AsNumber());
    }
    
    ImGui::Text("%s", field.displayName.c_str());
    ImGui::PushItemWidth(-1);
    if (ImGui::InputFloat3(("##" + fieldPath).c_str(), vec)) {
        simplejson::JsonObject vecObj;
        vecObj["x"] = simplejson::JsonValue(static_cast<double>(vec[0]));
        vecObj["y"] = simplejson::JsonValue(static_cast<double>(vec[1]));
        vecObj["z"] = simplejson::JsonValue(static_cast<double>(vec[2]));
        SetFieldValue(fieldPath, simplejson::JsonValue(vecObj));
    }
    ImGui::PopItemWidth();
}

void ConfigEditorImGuiUI::RenderFileField(const EditorField& field, const std::string& fieldPath) {
    auto value = GetFieldValue(fieldPath);
    std::string filePath = value.IsString() ? value.AsString() : "";
    
    ImGui::Text("%s", field.displayName.c_str());
    
    char buffer[512];
    strncpy(buffer, filePath.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    ImGui::PushItemWidth(-80);
    if (ImGui::InputText(("##" + fieldPath).c_str(), buffer, sizeof(buffer))) {
        SetFieldValue(fieldPath, simplejson::JsonValue(std::string(buffer)));
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        // Open file dialog - would need platform-specific implementation
        // For now, just a placeholder
        std::cout << "[ConfigEditor] File browser not implemented" << std::endl;
    }
}

// Additional helper method implementations would go here...
// For brevity, I'm showing the key rendering methods above.

void ConfigEditorImGuiUI::RenderValidationPanel() {
    const auto& validation = editor_->GetLastValidation();
    
    if (validation.success) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Configuration is valid");
        ImGui::Text("No errors found");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Validation failed");
        ImGui::Text("%zu errors found:", validation.errors.size());
        
        ImGui::Separator();
        
        for (size_t i = 0; i < validation.errors.size(); ++i) {
            const auto& error = validation.errors[i];
            
            ImGui::PushID(static_cast<int>(i));
            
            // Error icon and message
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error:");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", error.message.c_str());
            
            // Path
            if (!error.path.empty()) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path: %s", error.path.c_str());
            }
            
            // Suggestion
            if (!error.suggestion.empty()) {
                ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Suggestion: %s", error.suggestion.c_str());
            }
            
            if (i < validation.errors.size() - 1) {
                ImGui::Separator();
            }
            
            ImGui::PopID();
        }
    }
}

// Stub implementations for remaining methods
void ConfigEditorImGuiUI::RenderStatusBar() {
    ImGui::Text("Status: %s | File: %s", 
               editor_->HasUnsavedChanges() ? "Modified" : "Saved",
               editor_->GetCurrentFile().empty() ? "Untitled" : editor_->GetCurrentFile().c_str());
}

void ConfigEditorImGuiUI::HandleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            state_.showNewConfigDialog = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            state_.showOpenDialog = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (!editor_->GetCurrentFile().empty()) {
                editor_->SaveConfig();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            editor_->Undo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            editor_->Redo();
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        editor_->ValidateCurrentConfig();
    }
}

// More stub implementations for all the declared methods...
// In a full implementation, these would contain the actual UI code

#else // !USE_IMGUI

// Stub implementations for when ImGui is not available
void ConfigEditorImGuiUI::RenderMainWindow() {}
void ConfigEditorImGuiUI::RenderMenuBar() {}
void ConfigEditorImGuiUI::RenderToolbar() {}
void ConfigEditorImGuiUI::RenderLeftPanel() {}
void ConfigEditorImGuiUI::RenderCenterPanel() {}
void ConfigEditorImGuiUI::RenderRightPanel() {}
void ConfigEditorImGuiUI::RenderFormEditor() {}
void ConfigEditorImGuiUI::RenderValidationPanel() {}
void ConfigEditorImGuiUI::RenderStatusBar() {}
void ConfigEditorImGuiUI::HandleKeyboardShortcuts() {}

#endif // USE_IMGUI

// Helper method implementations that don't depend on ImGui
simplejson::JsonValue ConfigEditorImGuiUI::GetFieldValue(const std::string& fieldPath) const {
    return editor_->GetFieldValue(fieldPath);
}

void ConfigEditorImGuiUI::SetFieldValue(const std::string& fieldPath, const simplejson::JsonValue& value) {
    editor_->SetFieldValue(fieldPath, value);
}

void ConfigEditorImGuiUI::UpdateNotifications(float deltaTime) {
    for (auto it = notifications_.begin(); it != notifications_.end();) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0) {
            it = notifications_.erase(it);
        } else {
            ++it;
        }
    }
}

void ConfigEditorImGuiUI::ShowNotification(const std::string& message, float duration) {
    Notification notification;
    notification.message = message;
    notification.timeRemaining = duration;
    notification.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    notifications_.push_back(notification);
}

std::vector<std::string> ConfigEditorImGuiUI::GetRecentFiles() const {
    // This would be loaded from a persistent store
    return {"assets/actors/examples/trading_station_example.json"};
}

void ConfigEditorImGuiUI::SaveState() {
    // Save UI state to file or registry
    std::cout << "[ConfigEditorUI] Saving UI state" << std::endl;
}

void ConfigEditorImGuiUI::LoadState() {
    // Load UI state from file or registry
    std::cout << "[ConfigEditorUI] Loading UI state" << std::endl;
}

void ConfigEditorImGuiUI::ResetUIState() {
    state_ = UIState{}; // Reset to defaults
    std::cout << "[ConfigEditorUI] UI state reset to defaults" << std::endl;
}

void ConfigEditorImGuiUI::UpdateMetrics() {
    #ifdef USE_IMGUI
    metrics_.frameTime = ImGui::GetIO().DeltaTime * 1000.0f; // Convert to ms
    #endif
}

// Stub implementations for remaining methods...
void ConfigEditorImGuiUI::RenderSchemaSelector() {}
void ConfigEditorImGuiUI::RenderRecentFiles() {}
void ConfigEditorImGuiUI::RenderTemplateList() {}
void ConfigEditorImGuiUI::RenderHistoryPanel() {}
void ConfigEditorImGuiUI::RenderJsonEditor() {}
void ConfigEditorImGuiUI::RenderLivePreview() {}
void ConfigEditorImGuiUI::RenderFileDialogs() {}
void ConfigEditorImGuiUI::RenderNewConfigDialog() {}
void ConfigEditorImGuiUI::RenderTemplateDialog() {}
void ConfigEditorImGuiUI::RenderValidationDetailsDialog() {}
void ConfigEditorImGuiUI::RenderNotifications() {}
void ConfigEditorImGuiUI::HandleDragDrop() {}
void ConfigEditorImGuiUI::RenderTooltip(const std::string& text) {}
void ConfigEditorImGuiUI::RenderHelpMarker(const std::string& text) {}
void ConfigEditorImGuiUI::RenderValidationIcon(bool isValid, const std::string& error) {}
void ConfigEditorImGuiUI::PushFieldStyle(const EditorField& field) {}
void ConfigEditorImGuiUI::PopFieldStyle() {}
void ConfigEditorImGuiUI::SetupColors() {}
void ConfigEditorImGuiUI::SetupFonts() {}
void ConfigEditorImGuiUI::ApplyTheme(Theme theme) { currentTheme_ = theme; }

bool ConfigEditorImGuiUI::LoadFile(const std::string& filePath) {
    if (editor_) {
        return editor_->LoadConfig(filePath);
    }
    return false;
}

} // namespace nova::config