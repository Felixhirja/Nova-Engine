#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <filesystem>
#include <chrono>

namespace ContentManagement {

/**
 * ContentTemplateSystem: Template system for rapid content creation
 * 
 * Features:
 * - Pre-defined templates for common content types
 * - Template inheritance and composition
 * - Variable substitution and parameterization
 * - Template versioning and migration
 * - Custom template creation from existing content
 */
class ContentTemplateSystem {
public:
    struct TemplateVariable {
        std::string name;
        std::string type;  // "string", "number", "boolean", "reference"
        std::string defaultValue;
        std::string description;
        bool required;
        std::vector<std::string> allowedValues;  // For enum-like variables
    };

    struct ContentTemplate {
        std::string id;
        std::string name;
        std::string description;
        std::string category;  // "ship", "weapon", "station", etc.
        std::vector<TemplateVariable> variables;
        std::unique_ptr<simplejson::JsonObject> baseContent;
        std::string parentTemplateId;  // For template inheritance
        std::vector<std::string> tags;
        int version;
        std::string author;
        std::chrono::system_clock::time_point createdDate;
    };

    struct TemplateInstance {
        std::string templateId;
        std::unordered_map<std::string, std::string> variableValues;
        std::unique_ptr<simplejson::JsonObject> generatedContent;
    };

    ContentTemplateSystem();
    ~ContentTemplateSystem();

    // Template Management
    bool LoadTemplates(const std::string& directory);
    bool SaveTemplate(const ContentTemplate& templ);
    bool DeleteTemplate(const std::string& templateId);
    
    void RegisterTemplate(const ContentTemplate& templ);
    const ContentTemplate* GetTemplate(const std::string& templateId) const;
    std::vector<std::string> GetAllTemplateIds() const;
    std::vector<std::string> GetTemplatesByCategory(const std::string& category) const;
    std::vector<std::string> GetTemplatesByTag(const std::string& tag) const;
    
    // Template Creation
    ContentTemplate CreateTemplateFromContent(const simplejson::JsonObject& content, const std::string& name, const std::string& category);
    ContentTemplate CreateEmptyTemplate(const std::string& name, const std::string& category);
    bool DeriveTemplate(const std::string& parentTemplateId, const std::string& newTemplateId, const std::string& name);
    
    // Content Generation
    std::unique_ptr<simplejson::JsonObject> InstantiateTemplate(const std::string& templateId, const std::unordered_map<std::string, std::string>& variables);
    bool ValidateVariables(const std::string& templateId, const std::unordered_map<std::string, std::string>& variables, std::vector<std::string>& errors) const;
    
    // Variable Management
    std::vector<TemplateVariable> GetTemplateVariables(const std::string& templateId) const;
    bool AddVariable(const std::string& templateId, const TemplateVariable& variable);
    bool RemoveVariable(const std::string& templateId, const std::string& variableName);
    
    // Template Editing
    bool UpdateTemplateBase(const std::string& templateId, const simplejson::JsonObject& newBase);
    bool UpdateTemplateMetadata(const std::string& templateId, const std::string& name, const std::string& description);
    
    // Search & Discovery
    std::vector<std::string> SearchTemplates(const std::string& query) const;
    std::vector<std::string> GetRecentTemplates(int count = 10) const;
    std::vector<std::string> GetPopularTemplates(int count = 10) const;
    
    // Template Preview
    std::string GeneratePreview(const std::string& templateId, const std::unordered_map<std::string, std::string>& variables) const;
    
    // Template Versioning
    bool MigrateTemplate(const std::string& templateId, int toVersion);
    std::vector<int> GetAvailableVersions(const std::string& templateId) const;
    
    // UI Integration
    void RenderTemplateSelector();
    void RenderTemplateEditor(const std::string& templateId);
    void RenderVariableInputs(const std::string& templateId, std::unordered_map<std::string, std::string>& variables);

private:
    std::unique_ptr<simplejson::JsonObject> ProcessTemplate(const ContentTemplate* templ, const std::unordered_map<std::string, std::string>& variables) const;
    void SubstituteVariables(simplejson::JsonObject& content, const std::unordered_map<std::string, std::string>& variables) const;
    void SubstituteInValue(simplejson::JsonValue& value, const std::unordered_map<std::string, std::string>& variables) const;
    
    std::vector<TemplateVariable> ExtractVariables(const simplejson::JsonObject& content) const;
    bool InheritsFrom(const std::string& templateId, const std::string& parentId) const;
    
    std::unordered_map<std::string, ContentTemplate> templates_;
    std::unordered_map<std::string, int> templateUsageCount_;  // For popularity tracking
};

} // namespace ContentManagement
