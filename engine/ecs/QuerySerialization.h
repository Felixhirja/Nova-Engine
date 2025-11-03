#pragma once

#include "QueryBuilder.h"
#include "EntityManager.h"
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <stdexcept>

namespace ecs {

/**
 * Query Serialization System
 * Save and load queries for persistence, networking, and save/load systems
 */

// Query descriptor for serialization
struct QueryDescriptor {
    std::vector<std::string> requiredComponents;
    std::vector<std::string> excludedComponents;
    std::string predicateExpression;  // Serialized predicate logic
    size_t limit = 0;  // 0 = no limit
    bool parallel = false;
    
    // Serialize to string
    std::string ToString() const {
        std::ostringstream oss;
        
        // Required components
        oss << "WITH:";
        for (size_t i = 0; i < requiredComponents.size(); ++i) {
            if (i > 0) oss << ",";
            oss << requiredComponents[i];
        }
        oss << ";";
        
        // Excluded components
        if (!excludedComponents.empty()) {
            oss << "WITHOUT:";
            for (size_t i = 0; i < excludedComponents.size(); ++i) {
                if (i > 0) oss << ",";
                oss << excludedComponents[i];
            }
            oss << ";";
        }
        
        // Predicate
        if (!predicateExpression.empty()) {
            oss << "WHERE:" << predicateExpression << ";";
        }
        
        // Limit
        if (limit > 0) {
            oss << "LIMIT:" << limit << ";";
        }
        
        // Parallel flag
        if (parallel) {
            oss << "PARALLEL:true;";
        }
        
        return oss.str();
    }
    
    // Deserialize from string
    static QueryDescriptor FromString(const std::string& str) {
        QueryDescriptor desc;
        
        std::istringstream iss(str);
        std::string token;
        
        while (std::getline(iss, token, ';')) {
            if (token.empty()) continue;
            
            size_t colonPos = token.find(':');
            if (colonPos == std::string::npos) continue;
            
            std::string key = token.substr(0, colonPos);
            std::string value = token.substr(colonPos + 1);
            
            if (key == "WITH") {
                desc.requiredComponents = Split(value, ',');
            }
            else if (key == "WITHOUT") {
                desc.excludedComponents = Split(value, ',');
            }
            else if (key == "WHERE") {
                desc.predicateExpression = value;
            }
            else if (key == "LIMIT") {
                desc.limit = std::stoull(value);
            }
            else if (key == "PARALLEL") {
                desc.parallel = (value == "true");
            }
        }
        
        return desc;
    }
    
private:
    static std::vector<std::string> Split(const std::string& str, char delim) {
        std::vector<std::string> result;
        std::istringstream iss(str);
        std::string item;
        
        while (std::getline(iss, item, delim)) {
            if (!item.empty()) {
                result.push_back(item);
            }
        }
        
        return result;
    }
};

// Query serializer for save/load
class QuerySerializer {
public:
    // Serialize query descriptor to string
    static std::string Serialize(const QueryDescriptor& desc) {
        return desc.ToString();
    }
    
    // Deserialize query from string
    static QueryDescriptor Deserialize(const std::string& serialized) {
        return QueryDescriptor::FromString(serialized);
    }
    
    // Save query descriptor to file
    static bool SaveToFile(const QueryDescriptor& desc, const std::string& filename) {
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                return false;
            }
            file << desc.ToString();
            return true;
        }
        catch (...) {
            return false;
        }
    }
    
    // Load query descriptor from file
    static QueryDescriptor LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open query file: " + filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        return QueryDescriptor::FromString(content);
    }
};

// Component type registry for serialization support
class ComponentTypeRegistry {
public:
    static ComponentTypeRegistry& GetInstance() {
        static ComponentTypeRegistry instance;
        return instance;
    }
    
    // Register a component type
    template<typename T>
    void RegisterType(const std::string& name) {
        auto typeIdx = std::type_index(typeid(T));
        nameToType_.emplace(name, typeIdx);
        typeToName_.emplace(typeIdx, name);
    }
    
    // Resolve component name to type
    std::type_index GetType(const std::string& name) const {
        auto it = nameToType_.find(name);
        if (it == nameToType_.end()) {
            throw std::runtime_error("Unknown component type: " + name);
        }
        return it->second;
    }
    
    // Resolve component type to name
    std::string GetName(std::type_index type) const {
        auto it = typeToName_.find(type);
        if (it == typeToName_.end()) {
            throw std::runtime_error("Unregistered component type");
        }
        return it->second;
    }
    
    // Check if type is registered
    bool IsRegistered(const std::string& name) const {
        return nameToType_.find(name) != nameToType_.end();
    }
    
    bool IsRegistered(std::type_index type) const {
        return typeToName_.find(type) != typeToName_.end();
    }
    
private:
    ComponentTypeRegistry() = default;
    
    std::unordered_map<std::string, std::type_index> nameToType_;
    std::unordered_map<std::type_index, std::string> typeToName_;
};

// Auto-registration helper
template<typename T>
class AutoRegisterComponent {
public:
    explicit AutoRegisterComponent(const std::string& name) {
        ComponentTypeRegistry::GetInstance().RegisterType<T>(name);
    }
};

// Macro for easy component registration
#define REGISTER_COMPONENT_TYPE(Type, Name) \
    static ecs::AutoRegisterComponent<Type> Type##_registration(Name)

// Query template for reusable query patterns
class QueryTemplate {
public:
    QueryTemplate() : name_(""), descriptor_() {}
    QueryTemplate(const std::string& name, const QueryDescriptor& descriptor)
        : name_(name), descriptor_(descriptor) {}
    
    const std::string& GetName() const { return name_; }
    const QueryDescriptor& GetDescriptor() const { return descriptor_; }
    
    // Get the descriptor (can be used to execute queries manually)
    const QueryDescriptor& GetDescriptorForExecution() const {
        return descriptor_;
    }
    
private:
    std::string name_;
    QueryDescriptor descriptor_;
};

// Query template library
class QueryTemplateLibrary {
public:
    static QueryTemplateLibrary& GetInstance() {
        static QueryTemplateLibrary instance;
        return instance;
    }
    
    // Add template
    void AddTemplate(const QueryTemplate& tmpl) {
        templates_.emplace(tmpl.GetName(), tmpl);
    }
    
    // Get template
    const QueryTemplate* GetTemplate(const std::string& name) const {
        auto it = templates_.find(name);
        return it != templates_.end() ? &it->second : nullptr;
    }
    
    // List all templates
    std::vector<std::string> ListTemplates() const {
        std::vector<std::string> names;
        for (const auto& [name, tmpl] : templates_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Load templates from file
    bool LoadFromFile(const std::string& filename) {
        // Implementation would parse a configuration file
        // with multiple query templates
        return false;
    }
    
    // Save templates to file
    bool SaveToFile(const std::string& filename) const {
        // Implementation would serialize all templates
        return false;
    }
    
private:
    QueryTemplateLibrary() = default;
    std::unordered_map<std::string, QueryTemplate> templates_;
};

} // namespace ecs
