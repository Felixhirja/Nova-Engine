#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ContentFramework.h"

namespace NovaEngine {

// Composition strategies for merging content
enum class CompositionStrategy {
    Override,    // Later values override earlier ones
    Merge,       // Merge collections (arrays, objects)
    Add,         // Add numeric values
    Multiply,    // Multiply numeric values
    Min,         // Take minimum value
    Max,         // Take maximum value
    Concatenate, // Concatenate strings
    Custom       // Custom composition function
};

// Field composition rule
struct FieldCompositionRule {
    std::string fieldPath;  // JSON path to field (e.g., "stats.health")
    CompositionStrategy strategy;
    std::function<simplejson::JsonValue(const std::vector<simplejson::JsonValue>&)> customFunc;
};

// Content compositor - combines multiple content definitions
class ContentCompositor {
public:
    static ContentCompositor& Instance() {
        static ContentCompositor instance;
        return instance;
    }
    
    // Register composition rules for a content type
    void RegisterRule(const std::string& contentType, const FieldCompositionRule& rule);
    void ClearRules(const std::string& contentType);
    
    // Compose multiple content definitions
    std::unique_ptr<ContentDefinition> Compose(
        const std::vector<const ContentDefinition*>& bases,
        const std::string& newId) const;
    
    // Compose with explicit rules
    std::unique_ptr<ContentDefinition> ComposeWithRules(
        const std::vector<const ContentDefinition*>& bases,
        const std::string& newId,
        const std::vector<FieldCompositionRule>& rules) const;
    
    // Helper for creating variants
    std::unique_ptr<ContentDefinition> CreateVariant(
        const ContentDefinition& base,
        const std::string& newId,
        const simplejson::JsonValue& overrides) const;
    
private:
    ContentCompositor() = default;
    
    simplejson::JsonValue ComposeJson(const std::vector<simplejson::JsonValue>& jsons,
                          const std::string& contentType) const;
    
    simplejson::JsonValue ApplyCompositionRule(const std::vector<simplejson::JsonValue>& values,
                                   const FieldCompositionRule& rule) const;
    
    std::unordered_map<std::string, std::vector<FieldCompositionRule>> rules_;
};

// Content inheritance system
class ContentInheritance {
public:
    static ContentInheritance& Instance() {
        static ContentInheritance instance;
        return instance;
    }
    
    // Set up inheritance hierarchy
    void SetBaseContent(const std::string& derivedId, const std::string& baseId);
    void RemoveInheritance(const std::string& derivedId);
    
    // Query hierarchy
    std::string GetBaseContent(const std::string& contentId) const;
    std::vector<std::string> GetDerivedContent(const std::string& contentId) const;
    std::vector<std::string> GetInheritanceChain(const std::string& contentId) const;
    
    // Resolve inherited content
    std::unique_ptr<ContentDefinition> ResolveInheritance(
        const std::string& contentId) const;
    
    // Check inheritance validity
    bool IsValidInheritance(const std::string& derivedId, 
                           const std::string& baseId) const;
    
private:
    ContentInheritance() = default;
    
    std::unordered_map<std::string, std::string> baseMap_;  // derived -> base
    std::unordered_map<std::string, std::vector<std::string>> derivedMap_;  // base -> derived[]
};

// Content template system
class ContentTemplate {
public:
    struct TemplateParameter {
        std::string name;
        std::string type;  // "string", "number", "boolean", "object"
        simplejson::JsonValue defaultValue;
        std::string description;
    };
    
    ContentTemplate(const std::string& id, const std::string& type)
        : id_(id), type_(type) {}
    
    void AddParameter(const TemplateParameter& param) {
        parameters_.push_back(param);
    }
    
    void SetTemplate(const simplejson::JsonValue& templateJson) {
        template_ = templateJson;
    }
    
    // Instantiate template with parameters
    std::unique_ptr<ContentDefinition> Instantiate(
        const std::string& newId,
        const std::unordered_map<std::string, simplejson::JsonValue>& params) const;
    
    const std::vector<TemplateParameter>& GetParameters() const { 
        return parameters_; 
    }
    
    const std::string& GetId() const { return id_; }
    const std::string& GetType() const { return type_; }
    
private:
    std::string id_;
    std::string type_;
    std::vector<TemplateParameter> parameters_;
    simplejson::JsonValue template_;
    
    simplejson::JsonValue ResolveTemplate(const simplejson::JsonValue& json,
                              const std::unordered_map<std::string, simplejson::JsonValue>& params) const;
    
    friend class ContentTemplateRegistry;
};

// Content template registry
class ContentTemplateRegistry {
public:
    static ContentTemplateRegistry& Instance() {
        static ContentTemplateRegistry instance;
        return instance;
    }
    
    void RegisterTemplate(std::shared_ptr<ContentTemplate> tmpl);
    std::shared_ptr<ContentTemplate> GetTemplate(const std::string& id) const;
    
    std::vector<std::string> GetAllTemplates() const;
    std::vector<std::string> GetTemplatesByType(const std::string& type) const;
    
    // Instantiate from template
    std::unique_ptr<ContentDefinition> InstantiateTemplate(
        const std::string& templateId,
        const std::string& newId,
        const std::unordered_map<std::string, simplejson::JsonValue>& params) const;
    
private:
    ContentTemplateRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<ContentTemplate>> templates_;
};

// Composition builder for fluent API
class CompositionBuilder {
public:
    CompositionBuilder(const std::string& newId) : newId_(newId) {}
    
    CompositionBuilder& AddBase(const std::string& baseId) {
        baseIds_.push_back(baseId);
        return *this;
    }
    
    CompositionBuilder& WithStrategy(const std::string& field, 
                                     CompositionStrategy strategy) {
        FieldCompositionRule rule;
        rule.fieldPath = field;
        rule.strategy = strategy;
        rules_.push_back(rule);
        return *this;
    }
    
    CompositionBuilder& WithCustom(const std::string& field,
                                   std::function<simplejson::JsonValue(const std::vector<simplejson::JsonValue>&)> func) {
        FieldCompositionRule rule;
        rule.fieldPath = field;
        rule.strategy = CompositionStrategy::Custom;
        rule.customFunc = func;
        rules_.push_back(rule);
        return *this;
    }
    
    std::unique_ptr<ContentDefinition> Build() const;
    
private:
    std::string newId_;
    std::vector<std::string> baseIds_;
    std::vector<FieldCompositionRule> rules_;
};

} // namespace NovaEngine
