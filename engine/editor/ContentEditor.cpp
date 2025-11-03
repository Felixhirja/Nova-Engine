#include "ContentEditor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace ContentManagement {

ContentEditor::ContentEditor()
    : editorOpen_(false)
    , previewEnabled_(false)
    , showValidationErrors_(true)
    , filterType_(ContentType::Custom) {
}

ContentEditor::~ContentEditor() {
    SaveAll();
}

bool ContentEditor::Initialize() {
    // Register default schemas
    RegisterSchema({
        "Ship",
        ContentType::Ship,
        "Spacecraft configuration",
        {
            {"name", FieldType::String},
            {"class", FieldType::String},
            {"maxSpeed", FieldType::Float},
            {"maxHealth", FieldType::Float},
            {"position", FieldType::Vector3}
        },
        {},
        {},
        {{"maxSpeed", {0.0f, 1000.0f}}, {"maxHealth", {0.0f, 10000.0f}}}
    });
    
    return true;
}

void ContentEditor::Update(float deltaTime) {
    // Auto-save dirty content periodically
    static float autoSaveTimer = 0.0f;
    autoSaveTimer += deltaTime;
    
    if (autoSaveTimer >= 60.0f) {  // Auto-save every 60 seconds
        autoSaveTimer = 0.0f;
        SaveAll();
    }
}

void ContentEditor::Render() {
    if (!editorOpen_) return;
    
    RenderContentBrowser();
    RenderContentEditor();
    if (previewEnabled_) {
        RenderPreviewPanel();
    }
}

bool ContentEditor::LoadContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open content file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Parse JSON (simplified - in real implementation use proper JSON parser)
    auto jsonData = std::make_unique<simplejson::JsonObject>();
    
    // Generate ID from filename
    std::filesystem::path path(filePath);
    std::string contentId = path.stem().string();
    
    ContentItem item;
    item.id = contentId;
    item.filePath = filePath;
    item.type = ContentType::Custom;
    item.data = std::move(jsonData);
    item.lastModified = std::filesystem::last_write_time(filePath);
    item.isDirty = false;
    item.isValid = true;
    
    contentItems_[contentId] = std::move(item);
    
    std::cout << "Loaded content: " << contentId << " from " << filePath << std::endl;
    return true;
}

bool ContentEditor::LoadContentDirectory(const std::string& directory, ContentType /*type*/) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Content directory does not exist: " << directory << std::endl;
        return false;
    }
    
    int loadedCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            if (LoadContent(entry.path().string())) {
                loadedCount++;
            }
        }
    }
    
    std::cout << "Loaded " << loadedCount << " content files from " << directory << std::endl;
    return loadedCount > 0;
}

void ContentEditor::ReloadAll() {
    std::vector<std::string> filePaths;
    for (const auto& [id, item] : contentItems_) {
        filePaths.push_back(item.filePath);
    }
    
    contentItems_.clear();
    
    for (const auto& filePath : filePaths) {
        LoadContent(filePath);
    }
}

std::string ContentEditor::CreateContent(ContentType type, const std::string& templateName) {
    std::string contentId = GenerateUniqueId(type);
    
    ContentItem item;
    item.id = contentId;
    item.type = type;
    item.data = std::make_unique<simplejson::JsonObject>();
    item.isDirty = true;
    item.isValid = false;
    item.lastModified = std::filesystem::file_time_type::clock::now();
    
    // Initialize with template if provided
    if (!templateName.empty()) {
        // Template loading would happen here
    }
    
    contentItems_[contentId] = std::move(item);
    
    if (onContentChanged_) {
        onContentChanged_(contentId);
    }
    
    return contentId;
}

bool ContentEditor::SaveContent(const std::string& contentId) {
    auto it = contentItems_.find(contentId);
    if (it == contentItems_.end()) {
        return false;
    }
    
    auto& item = it->second;
    
    // Determine file path
    if (item.filePath.empty()) {
        item.filePath = "assets/content/" + GetContentTypeString(item.type) + "/" + contentId + ".json";
    }
    
    // Create directory if needed
    std::filesystem::path path(item.filePath);
    std::filesystem::create_directories(path.parent_path());
    
    // Write JSON to file (simplified)
    std::ofstream file(item.filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to save content: " << item.filePath << std::endl;
        return false;
    }
    
    file << "{ \"id\": \"" << contentId << "\" }";  // Simplified JSON output
    file.close();
    
    item.isDirty = false;
    item.lastModified = std::filesystem::last_write_time(item.filePath);
    
    if (onContentSaved_) {
        onContentSaved_(contentId);
    }
    
    std::cout << "Saved content: " << contentId << std::endl;
    return true;
}

bool ContentEditor::SaveAll() {
    bool allSuccess = true;
    for (const auto& [id, item] : contentItems_) {
        if (item.isDirty) {
            if (!SaveContent(id)) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

void ContentEditor::SelectContent(const std::string& contentId) {
    if (contentItems_.find(contentId) != contentItems_.end()) {
        selectedContentId_ = contentId;
        
        if (onContentChanged_) {
            onContentChanged_(contentId);
        }
    }
}

ContentEditor::ContentItem* ContentEditor::GetSelectedContent() {
    if (selectedContentId_.empty()) {
        return nullptr;
    }
    
    auto it = contentItems_.find(selectedContentId_);
    return (it != contentItems_.end()) ? &it->second : nullptr;
}

bool ContentEditor::SetField(const std::string& fieldPath, const simplejson::JsonValue& value) {
    auto content = GetSelectedContent();
    if (!content || !content->data) {
        return false;
    }
    
    // Parse field path (e.g., "stats.maxSpeed")
    // Simplified implementation
    (*content->data)[fieldPath] = value;
    
    content->isDirty = true;
    
    if (onContentChanged_) {
        onContentChanged_(selectedContentId_);
    }
    
    return true;
}

simplejson::JsonValue ContentEditor::GetField(const std::string& fieldPath) const {
    if (selectedContentId_.empty()) {
        return simplejson::JsonValue();
    }
    
    auto it = contentItems_.find(selectedContentId_);
    if (it == contentItems_.end() || !it->second.data) {
        return simplejson::JsonValue();
    }
    
    auto& data = *it->second.data;
    auto fieldIt = data.find(fieldPath);
    if (fieldIt != data.end()) {
        return fieldIt->second;
    }
    
    return simplejson::JsonValue();
}

bool ContentEditor::DeleteContent(const std::string& contentId) {
    auto it = contentItems_.find(contentId);
    if (it == contentItems_.end()) {
        return false;
    }
    
    // Delete file if exists
    if (!it->second.filePath.empty() && std::filesystem::exists(it->second.filePath)) {
        std::filesystem::remove(it->second.filePath);
    }
    
    contentItems_.erase(it);
    
    if (selectedContentId_ == contentId) {
        selectedContentId_.clear();
    }
    
    return true;
}

bool ContentEditor::DuplicateContent(const std::string& contentId, const std::string& newId) {
    auto it = contentItems_.find(contentId);
    if (it == contentItems_.end()) {
        return false;
    }
    
    // Copy content item (excluding unique_ptr fields)
    ContentItem newItem;
    newItem.id = newId;
    newItem.type = it->second.type;
    newItem.id = newId;
    newItem.filePath.clear();
    newItem.isDirty = true;
    
    contentItems_[newId] = std::move(newItem);
    return true;
}

bool ContentEditor::RenameContent(const std::string& contentId, const std::string& newId) {
    auto it = contentItems_.find(contentId);
    if (it == contentItems_.end()) {
        return false;
    }
    
    ContentItem item = std::move(it->second);
    item.id = newId;
    item.isDirty = true;
    
    contentItems_.erase(it);
    contentItems_[newId] = std::move(item);
    
    if (selectedContentId_ == contentId) {
        selectedContentId_ = newId;
    }
    
    return true;
}

std::vector<std::string> ContentEditor::SearchContent(const std::string& query) const {
    std::vector<std::string> results;
    
    for (const auto& [id, item] : contentItems_) {
        if (id.find(query) != std::string::npos) {
            results.push_back(id);
        }
    }
    
    return results;
}

std::vector<std::string> ContentEditor::FilterByType(ContentType type) const {
    std::vector<std::string> results;
    
    for (const auto& [id, item] : contentItems_) {
        if (item.type == type) {
            results.push_back(id);
        }
    }
    
    return results;
}

std::vector<std::string> ContentEditor::GetRecentlyModified(int count) const {
    std::vector<std::pair<std::string, std::filesystem::file_time_type>> items;
    
    for (const auto& [id, item] : contentItems_) {
        items.push_back({id, item.lastModified});
    }
    
    std::sort(items.begin(), items.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> results;
    for (int i = 0; i < std::min(count, static_cast<int>(items.size())); ++i) {
        results.push_back(items[i].first);
    }
    
    return results;
}

void ContentEditor::RegisterSchema(const ContentSchema& schema) {
    schemas_[schema.type] = schema;
}

const ContentEditor::ContentSchema* ContentEditor::GetSchema(ContentType type) const {
    auto it = schemas_.find(type);
    return (it != schemas_.end()) ? &it->second : nullptr;
}

bool ContentEditor::ValidateAgainstSchema(const std::string& contentId, std::vector<std::string>& errors) const {
    auto it = contentItems_.find(contentId);
    if (it == contentItems_.end()) {
        errors.push_back("Content not found");
        return false;
    }
    
    const auto& item = it->second;
    const auto* schema = GetSchema(item.type);
    if (!schema) {
        errors.push_back("No schema registered for content type");
        return false;
    }
    
    // Validate each field
    if (item.data) {
        for (const auto& [fieldName, fieldType] : schema->fields) {
            auto fieldIt = item.data->find(fieldName);
            if (fieldIt == item.data->end()) {
                errors.push_back("Missing required field: " + fieldName);
                continue;
            }
            
            if (!ValidateField(fieldName, fieldIt->second, schema, errors)) {
                // Error already added
            }
        }
    }
    
    return errors.empty();
}

void ContentEditor::EnablePreview(bool enable) {
    previewEnabled_ = enable;
}

void ContentEditor::UpdatePreview() {
    // Preview update logic would go here
}

void ContentEditor::SetOnContentChanged(std::function<void(const std::string&)> callback) {
    onContentChanged_ = callback;
}

void ContentEditor::SetOnContentSaved(std::function<void(const std::string&)> callback) {
    onContentSaved_ = callback;
}

bool ContentEditor::HasUnsavedChanges() const {
    return std::any_of(contentItems_.begin(), contentItems_.end(),
        [](const auto& pair) { return pair.second.isDirty; });
}

void ContentEditor::RenderContentBrowser() {
    // ImGui rendering would go here
    // This is a placeholder for the UI implementation
}

void ContentEditor::RenderContentEditor() {
    // ImGui rendering would go here
}

void ContentEditor::RenderPreviewPanel() {
    // ImGui rendering would go here
}

void ContentEditor::RenderFieldEditor(const std::string& /*fieldName*/, FieldType /*type*/, simplejson::JsonValue& /*value*/) {
    // ImGui field editor would go here
}

bool ContentEditor::ValidateField(
    const std::string& fieldName, 
    const simplejson::JsonValue& value, 
    const ContentSchema* schema, 
    std::vector<std::string>& errors) const {
    
    // Basic type checking
    auto fieldIt = std::find_if(schema->fields.begin(), schema->fields.end(),
        [&fieldName](const auto& pair) { return pair.first == fieldName; });
    
    if (fieldIt == schema->fields.end()) {
        return true;  // Field not in schema
    }
    
    FieldType expectedType = fieldIt->second;
    
    // Validate type
    switch (expectedType) {
        case FieldType::String:
            if (!value.IsString()) {
                errors.push_back(fieldName + " must be a string");
                return false;
            }
            break;
        case FieldType::Integer:
        case FieldType::Float:
            if (!value.IsNumber()) {
                errors.push_back(fieldName + " must be a number");
                return false;
            }
            break;
        case FieldType::Boolean:
            if (!value.IsBoolean()) {
                errors.push_back(fieldName + " must be a boolean");
                return false;
            }
            break;
        case FieldType::Array:
            if (!value.IsArray()) {
                errors.push_back(fieldName + " must be an array");
                return false;
            }
            break;
        case FieldType::Object:
            if (!value.IsObject()) {
                errors.push_back(fieldName + " must be an object");
                return false;
            }
            break;
        default:
            break;
    }
    
    // Validate ranges for numeric fields
    if (value.IsNumber()) {
        auto rangeIt = schema->fieldRanges.find(fieldName);
        if (rangeIt != schema->fieldRanges.end()) {
            double numValue = value.AsNumber();
            if (numValue < rangeIt->second.first || numValue > rangeIt->second.second) {
                errors.push_back(fieldName + " must be between " + 
                    std::to_string(rangeIt->second.first) + " and " + 
                    std::to_string(rangeIt->second.second));
                return false;
            }
        }
    }
    
    return true;
}

std::string ContentEditor::GenerateUniqueId(ContentType type) const {
    std::string prefix = GetContentTypeString(type);
    std::string id;
    int counter = 1;
    
    do {
        id = prefix + "_" + std::to_string(counter++);
    } while (contentItems_.find(id) != contentItems_.end());
    
    return id;
}

std::string ContentEditor::GetContentTypeString(ContentType type) const {
    switch (type) {
        case ContentType::Ship: return "ship";
        case ContentType::Station: return "station";
        case ContentType::Weapon: return "weapon";
        case ContentType::Module: return "module";
        case ContentType::Actor: return "actor";
        case ContentType::World: return "world";
        case ContentType::Configuration: return "config";
        case ContentType::Custom: return "custom";
        default: return "unknown";
    }
}

int ContentEditor::GetContentCount() const {
    return static_cast<int>(contentItems_.size());
}

ContentEditor::ContentItem* ContentEditor::GetContent(const std::string& contentId) {
    auto it = contentItems_.find(contentId);
    return (it != contentItems_.end()) ? &it->second : nullptr;
}

void ContentEditor::SetSaveCallback(std::function<void(const std::string&)> callback) {
    onContentSaved_ = callback;
}

void ContentEditor::SetLoadCallback(std::function<void(const std::string&)> callback) {
    onContentChanged_ = callback;
}

} // namespace ContentManagement
