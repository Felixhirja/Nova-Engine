#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include <unordered_map>

namespace ContentManagement {

/**
 * ContentEditor: Visual editor for game content and configurations
 * 
 * Provides a complete content editing interface with:
 * - Visual JSON editing with schema validation
 * - Real-time preview of content changes
 * - Template-based content creation
 * - Content browsing and search
 * - Integration with undo/redo system
 */
class ContentEditor {
public:
    enum class ContentType {
        Ship,
        Station,
        Weapon,
        Module,
        Actor,
        World,
        Configuration,
        Custom
    };

    enum class FieldType {
        String,
        Integer,
        Float,
        Boolean,
        Vector3,
        Color,
        Reference,  // Reference to another content
        Array,
        Object
    };

    struct ContentSchema {
        std::string name;
        ContentType type;
        std::string description;
        std::vector<std::pair<std::string, FieldType>> fields;
        std::unordered_map<std::string, std::string> fieldDescriptions;
        std::unordered_map<std::string, std::vector<std::string>> fieldOptions;  // For enum-like fields
        std::unordered_map<std::string, std::pair<float, float>> fieldRanges;    // Min/max for numeric fields
    };

    struct ContentItem {
        std::string id;
        std::string filePath;
        ContentType type;
        std::unique_ptr<simplejson::JsonObject> data;
        std::filesystem::file_time_type lastModified;
        bool isDirty;
        bool isValid;
        std::vector<std::string> validationErrors;
    };

    ContentEditor();
    ~ContentEditor();

    // Lifecycle
    bool Initialize();
    void Update(float deltaTime);
    void Render();

    // Content Loading
    bool LoadContent(const std::string& filePath);
    bool LoadContentDirectory(const std::string& directory, ContentType type);
    void ReloadAll();

    // Content Creation
    std::string CreateContent(ContentType type, const std::string& templateName = "");
    bool SaveContent(const std::string& contentId);
    bool SaveAll();

    // Content Editing
    void SelectContent(const std::string& contentId);
    ContentItem* GetSelectedContent();
    bool SetField(const std::string& fieldPath, const simplejson::JsonValue& value);
    simplejson::JsonValue GetField(const std::string& fieldPath) const;

    // Content Management
    bool DeleteContent(const std::string& contentId);
    bool DuplicateContent(const std::string& contentId, const std::string& newId);
    bool RenameContent(const std::string& contentId, const std::string& newId);

    // Search & Filter
    std::vector<std::string> SearchContent(const std::string& query) const;
    std::vector<std::string> FilterByType(ContentType type) const;
    std::vector<std::string> GetRecentlyModified(int count = 10) const;

    // Schema Management
    void RegisterSchema(const ContentSchema& schema);
    const ContentSchema* GetSchema(ContentType type) const;
    bool ValidateAgainstSchema(const std::string& contentId, std::vector<std::string>& errors) const;

    // Preview
    void EnablePreview(bool enable);
    void UpdatePreview();

    // Integration
    void SetOnContentChanged(std::function<void(const std::string&)> callback);
    void SetOnContentSaved(std::function<void(const std::string&)> callback);

    // UI State
    bool IsEditorOpen() const { return editorOpen_; }
    void SetEditorOpen(bool open) { editorOpen_ = open; }
    bool HasUnsavedChanges() const;

private:
    void RenderContentBrowser();
    void RenderContentEditor();
    void RenderPreviewPanel();
    void RenderFieldEditor(const std::string& fieldName, FieldType type, simplejson::JsonValue& value);
    
    bool ValidateField(const std::string& fieldName, const simplejson::JsonValue& value, const ContentSchema* schema, std::vector<std::string>& errors) const;
    std::string GenerateUniqueId(ContentType type) const;
    std::string GetContentTypeString(ContentType type) const;
    
    std::unordered_map<std::string, ContentItem> contentItems_;
    std::unordered_map<ContentType, ContentSchema> schemas_;
    std::string selectedContentId_;
    bool editorOpen_;
    bool previewEnabled_;
    
    std::function<void(const std::string&)> onContentChanged_;
    std::function<void(const std::string&)> onContentSaved_;
    
    std::string searchQuery_;
    ContentType filterType_;
    bool showValidationErrors_;
public:
    // Additional helper methods
    int GetContentCount() const;
    ContentItem* GetContent(const std::string& contentId);
    void SetSaveCallback(std::function<void(const std::string&)> callback);
    void SetLoadCallback(std::function<void(const std::string&)> callback);
};

} // namespace ContentManagement
