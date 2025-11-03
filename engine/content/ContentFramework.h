#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <any>
#include "../SimpleJson.h"

namespace NovaEngine {

// Forward declarations
class ContentSchema;
class ContentValidator;
class ContentDependencyGraph;
class ContentCompositor;

// Content metadata for tracking and analytics
struct ContentMetadata {
    std::string id;
    std::string type;
    std::string version;
    std::string author;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> customFields;
    
    uint64_t loadCount = 0;
    uint64_t usageCount = 0;
    std::chrono::milliseconds totalLoadTime{0};
};

// Content definition - unified interface for all content
class ContentDefinition {
public:
    ContentDefinition(const std::string& id, const std::string& type)
        : id_(id), type_(type) {}
    virtual ~ContentDefinition() = default;
    
    const std::string& GetId() const { return id_; }
    const std::string& GetType() const { return type_; }
    
    void SetMetadata(const ContentMetadata& metadata) { metadata_ = metadata; }
    const ContentMetadata& GetMetadata() const { return metadata_; }
    ContentMetadata& GetMetadata() { return metadata_; }
    
    // Serialization
    virtual simplejson::JsonValue ToJson() const = 0;
    virtual bool FromJson(const simplejson::JsonValue& json) = 0;
    
    // Validation
    virtual bool Validate(std::vector<std::string>& errors) const = 0;
    
    // Dependencies
    virtual std::vector<std::string> GetDependencies() const = 0;
    
    // Clone for composition
    virtual std::unique_ptr<ContentDefinition> Clone() const = 0;
    
protected:
    std::string id_;
    std::string type_;
    ContentMetadata metadata_;
};

// Content registry - manages all content definitions
class ContentRegistry {
public:
    static ContentRegistry& Instance() {
        static ContentRegistry instance;
        return instance;
    }
    
    // Registration
    void RegisterContent(std::unique_ptr<ContentDefinition> content);
    void UnregisterContent(const std::string& id);
    
    // Retrieval
    ContentDefinition* GetContent(const std::string& id);
    const ContentDefinition* GetContent(const std::string& id) const;
    
    std::vector<ContentDefinition*> GetContentByType(const std::string& type);
    std::vector<const ContentDefinition*> GetContentByType(const std::string& type) const;
    
    // Query
    std::vector<ContentDefinition*> QueryContent(
        std::function<bool(const ContentDefinition*)> predicate);
    
    // Lifecycle
    void Clear();
    size_t GetContentCount() const { return content_.size(); }
    
    // Analytics
    const ContentMetadata* GetMetadata(const std::string& id) const;
    void RecordLoad(const std::string& id, std::chrono::milliseconds loadTime);
    void RecordUsage(const std::string& id);
    
private:
    ContentRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<ContentDefinition>> content_;
};

// Content factory for type registration
using ContentFactoryFunc = std::function<std::unique_ptr<ContentDefinition>(const std::string&)>;

class ContentFactory {
public:
    static ContentFactory& Instance() {
        static ContentFactory instance;
        return instance;
    }
    
    void RegisterType(const std::string& type, ContentFactoryFunc factory);
    std::unique_ptr<ContentDefinition> Create(const std::string& type, const std::string& id);
    
    bool IsTypeRegistered(const std::string& type) const;
    std::vector<std::string> GetRegisteredTypes() const;
    
private:
    ContentFactory() = default;
    std::unordered_map<std::string, ContentFactoryFunc> factories_;
};

// Auto-registration helper
template<typename T>
class ContentTypeRegistrar {
public:
    ContentTypeRegistrar(const std::string& typeName) {
        ContentFactory::Instance().RegisterType(typeName,
            [](const std::string& id) -> std::unique_ptr<ContentDefinition> {
                return std::make_unique<T>(id);
            });
    }
};

#define REGISTER_CONTENT_TYPE(TypeClass, TypeName) \
    static NovaEngine::ContentTypeRegistrar<TypeClass> g_##TypeClass##_registrar(TypeName)

// Content loading/saving
class ContentLoader {
public:
    static std::unique_ptr<ContentDefinition> LoadFromFile(const std::string& filepath);
    static bool SaveToFile(const ContentDefinition& content, const std::string& filepath);
    
    static std::unique_ptr<ContentDefinition> LoadFromJson(const simplejson::JsonValue& json);
    static simplejson::JsonValue SaveToJson(const ContentDefinition& content);
    
    // Batch operations
    static std::vector<std::unique_ptr<ContentDefinition>> LoadDirectory(
        const std::string& dirPath, bool recursive = true);
    static bool SaveDirectory(
        const std::vector<ContentDefinition*>& content, const std::string& dirPath);
};

// Content framework - main interface
class ContentFramework {
public:
    static ContentFramework& Instance() {
        static ContentFramework instance;
        return instance;
    }
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Content management
    bool LoadContent(const std::string& filepath);
    bool LoadContentDirectory(const std::string& dirPath, bool recursive = true);
    bool ReloadContent(const std::string& id);
    bool UnloadContent(const std::string& id);
    
    // Access
    ContentDefinition* GetContent(const std::string& id);
    template<typename T>
    T* GetContentAs(const std::string& id) {
        auto* content = GetContent(id);
        return content ? dynamic_cast<T*>(content) : nullptr;
    }
    
    // Validation
    bool ValidateContent(const std::string& id, std::vector<std::string>& errors);
    bool ValidateAllContent(std::unordered_map<std::string, std::vector<std::string>>& errorMap);
    
    // Dependencies
    std::vector<std::string> GetDependencies(const std::string& id);
    std::vector<std::string> GetDependents(const std::string& id);
    bool CheckDependencyCycles(std::vector<std::vector<std::string>>& cycles);
    
    // Composition
    std::unique_ptr<ContentDefinition> ComposeContent(
        const std::vector<std::string>& baseIds, const std::string& newId);
    
    // Analytics
    struct ContentStats {
        size_t totalContent = 0;
        size_t totalLoads = 0;
        size_t totalUsage = 0;
        std::unordered_map<std::string, size_t> contentByType;
        std::vector<std::pair<std::string, uint64_t>> mostUsed;
        std::vector<std::pair<std::string, uint64_t>> mostLoaded;
    };
    
    ContentStats GetContentStats() const;
    void ExportAnalytics(const std::string& filepath) const;
    
    // Documentation
    void GenerateDocumentation(const std::string& outputDir) const;
    
private:
    ContentFramework() = default;
    bool initialized_ = false;
    std::unordered_map<std::string, std::string> contentPaths_;
};

} // namespace NovaEngine
