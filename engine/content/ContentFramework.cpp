#include "ContentFramework.h"
#include "ContentSchema.h"
#include "ContentValidator.h"
#include "ContentDependencyGraph.h"
#include "ContentCompositor.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace NovaEngine {

// ContentRegistry implementation
void ContentRegistry::RegisterContent(std::unique_ptr<ContentDefinition> content) {
    if (!content) return;
    
    const std::string& id = content->GetId();
    content_[id] = std::move(content);
}

void ContentRegistry::UnregisterContent(const std::string& id) {
    content_.erase(id);
}

ContentDefinition* ContentRegistry::GetContent(const std::string& id) {
    auto it = content_.find(id);
    return it != content_.end() ? it->second.get() : nullptr;
}

const ContentDefinition* ContentRegistry::GetContent(const std::string& id) const {
    auto it = content_.find(id);
    return it != content_.end() ? it->second.get() : nullptr;
}

std::vector<ContentDefinition*> ContentRegistry::GetContentByType(const std::string& type) {
    std::vector<ContentDefinition*> result;
    for (auto& pair : content_) {
        if (pair.second->GetType() == type) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<const ContentDefinition*> ContentRegistry::GetContentByType(const std::string& type) const {
    std::vector<const ContentDefinition*> result;
    for (const auto& pair : content_) {
        if (pair.second->GetType() == type) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<ContentDefinition*> ContentRegistry::QueryContent(
    std::function<bool(const ContentDefinition*)> predicate) {
    std::vector<ContentDefinition*> result;
    for (auto& pair : content_) {
        if (predicate(pair.second.get())) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

void ContentRegistry::Clear() {
    content_.clear();
}

const ContentMetadata* ContentRegistry::GetMetadata(const std::string& id) const {
    auto* content = GetContent(id);
    return content ? &content->GetMetadata() : nullptr;
}

void ContentRegistry::RecordLoad(const std::string& id, std::chrono::milliseconds loadTime) {
    auto* content = GetContent(id);
    if (content) {
        auto& metadata = content->GetMetadata();
        metadata.loadCount++;
        metadata.totalLoadTime += loadTime;
    }
}

void ContentRegistry::RecordUsage(const std::string& id) {
    auto* content = GetContent(id);
    if (content) {
        content->GetMetadata().usageCount++;
    }
}

// ContentFactory implementation
void ContentFactory::RegisterType(const std::string& type, ContentFactoryFunc factory) {
    factories_[type] = factory;
}

std::unique_ptr<ContentDefinition> ContentFactory::Create(const std::string& type, const std::string& id) {
    auto it = factories_.find(type);
    if (it != factories_.end()) {
        return it->second(id);
    }
    return nullptr;
}

bool ContentFactory::IsTypeRegistered(const std::string& type) const {
    return factories_.find(type) != factories_.end();
}

std::vector<std::string> ContentFactory::GetRegisteredTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : factories_) {
        types.push_back(pair.first);
    }
    return types;
}

// ContentLoader implementation
std::unique_ptr<ContentDefinition> ContentLoader::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open content file: " << filepath << std::endl;
        return nullptr;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    SimpleJson json;
    if (!json.Parse(content)) {
        std::cerr << "Failed to parse JSON from: " << filepath << std::endl;
        return nullptr;
    }
    
    return LoadFromJson(json);
}

bool ContentLoader::SaveToFile(const ContentDefinition& content, const std::string& filepath) {
    SimpleJson json = SaveToJson(content);
    std::string jsonStr = json.ToString(true);
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    file << jsonStr;
    file.close();
    return true;
}

std::unique_ptr<ContentDefinition> ContentLoader::LoadFromJson(const SimpleJson& json) {
    auto typeNode = json.Get("type");
    auto idNode = json.Get("id");
    
    if (!typeNode || !idNode) {
        std::cerr << "Content missing type or id field" << std::endl;
        return nullptr;
    }
    
    std::string type = typeNode->AsString();
    std::string id = idNode->AsString();
    
    auto content = ContentFactory::Instance().Create(type, id);
    if (!content) {
        std::cerr << "Unknown content type: " << type << std::endl;
        return nullptr;
    }
    
    if (!content->FromJson(json)) {
        std::cerr << "Failed to deserialize content: " << id << std::endl;
        return nullptr;
    }
    
    return content;
}

SimpleJson ContentLoader::SaveToJson(const ContentDefinition& content) {
    return content.ToJson();
}

std::vector<std::unique_ptr<ContentDefinition>> ContentLoader::LoadDirectory(
    const std::string& dirPath, bool recursive) {
    std::vector<std::unique_ptr<ContentDefinition>> result;
    
    namespace fs = std::filesystem;
    try {
        auto iterator = recursive ? 
            fs::recursive_directory_iterator(dirPath) :
            fs::directory_iterator(dirPath);
        
        for (const auto& entry : iterator) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                auto content = LoadFromFile(entry.path().string());
                if (content) {
                    result.push_back(std::move(content));
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error loading directory: " << e.what() << std::endl;
    }
    
    return result;
}

bool ContentLoader::SaveDirectory(
    const std::vector<ContentDefinition*>& content, const std::string& dirPath) {
    namespace fs = std::filesystem;
    try {
        fs::create_directories(dirPath);
        
        for (const auto* def : content) {
            if (!def) continue;
            
            std::string filename = def->GetId() + ".json";
            std::string filepath = (fs::path(dirPath) / filename).string();
            
            if (!SaveToFile(*def, filepath)) {
                return false;
            }
        }
        
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error saving directory: " << e.what() << std::endl;
        return false;
    }
}

// ContentFramework implementation
void ContentFramework::Initialize() {
    if (initialized_) return;
    
    // Initialize subsystems
    ContentSchemaRegistry::Instance();
    ContentValidatorRegistry::Instance();
    
    initialized_ = true;
}

void ContentFramework::Shutdown() {
    ContentRegistry::Instance().Clear();
    contentPaths_.clear();
    initialized_ = false;
}

bool ContentFramework::LoadContent(const std::string& filepath) {
    auto startTime = std::chrono::steady_clock::now();
    
    auto content = ContentLoader::LoadFromFile(filepath);
    if (!content) {
        return false;
    }
    
    std::string id = content->GetId();
    contentPaths_[id] = filepath;
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    ContentRegistry::Instance().RegisterContent(std::move(content));
    ContentRegistry::Instance().RecordLoad(id, duration);
    
    return true;
}

bool ContentFramework::LoadContentDirectory(const std::string& dirPath, bool recursive) {
    auto contentList = ContentLoader::LoadDirectory(dirPath, recursive);
    
    for (auto& content : contentList) {
        std::string id = content->GetId();
        ContentRegistry::Instance().RegisterContent(std::move(content));
    }
    
    return !contentList.empty();
}

bool ContentFramework::ReloadContent(const std::string& id) {
    auto it = contentPaths_.find(id);
    if (it == contentPaths_.end()) {
        return false;
    }
    
    UnloadContent(id);
    return LoadContent(it->second);
}

bool ContentFramework::UnloadContent(const std::string& id) {
    ContentRegistry::Instance().UnregisterContent(id);
    return true;
}

ContentDefinition* ContentFramework::GetContent(const std::string& id) {
    auto* content = ContentRegistry::Instance().GetContent(id);
    if (content) {
        ContentRegistry::Instance().RecordUsage(id);
    }
    return content;
}

bool ContentFramework::ValidateContent(const std::string& id, std::vector<std::string>& errors) {
    auto* content = ContentRegistry::Instance().GetContent(id);
    if (!content) {
        errors.push_back("Content not found: " + id);
        return false;
    }
    
    return content->Validate(errors);
}

bool ContentFramework::ValidateAllContent(
    std::unordered_map<std::string, std::vector<std::string>>& errorMap) {
    bool allValid = true;
    
    auto& registry = ContentRegistry::Instance();
    for (size_t i = 0; i < registry.GetContentCount(); ++i) {
        // Query all content
        auto allContent = registry.QueryContent([](const ContentDefinition*) { return true; });
        
        for (auto* content : allContent) {
            std::vector<std::string> errors;
            if (!content->Validate(errors)) {
                errorMap[content->GetId()] = errors;
                allValid = false;
            }
        }
    }
    
    return allValid;
}

std::vector<std::string> ContentFramework::GetDependencies(const std::string& id) {
    auto* content = ContentRegistry::Instance().GetContent(id);
    return content ? content->GetDependencies() : std::vector<std::string>();
}

std::vector<std::string> ContentFramework::GetDependents(const std::string& id) {
    return ContentDependencyGraph::Instance().GetDependents(id);
}

bool ContentFramework::CheckDependencyCycles(std::vector<std::vector<std::string>>& cycles) {
    return ContentDependencyGraph::Instance().DetectCycles(cycles);
}

std::unique_ptr<ContentDefinition> ContentFramework::ComposeContent(
    const std::vector<std::string>& baseIds, const std::string& newId) {
    std::vector<const ContentDefinition*> bases;
    
    for (const auto& id : baseIds) {
        auto* content = ContentRegistry::Instance().GetContent(id);
        if (!content) {
            std::cerr << "Base content not found: " << id << std::endl;
            return nullptr;
        }
        bases.push_back(content);
    }
    
    return ContentCompositor::Instance().Compose(bases, newId);
}

ContentFramework::ContentStats ContentFramework::GetContentStats() const {
    ContentStats stats;
    
    auto& registry = ContentRegistry::Instance();
    auto allContent = registry.QueryContent([](const ContentDefinition*) { return true; });
    
    stats.totalContent = allContent.size();
    
    for (const auto* content : allContent) {
        const auto& metadata = content->GetMetadata();
        stats.totalLoads += metadata.loadCount;
        stats.totalUsage += metadata.usageCount;
        stats.contentByType[content->GetType()]++;
        
        stats.mostUsed.push_back({content->GetId(), metadata.usageCount});
        stats.mostLoaded.push_back({content->GetId(), metadata.loadCount});
    }
    
    // Sort by usage/loads
    auto sortFunc = [](const auto& a, const auto& b) { return a.second > b.second; };
    std::sort(stats.mostUsed.begin(), stats.mostUsed.end(), sortFunc);
    std::sort(stats.mostLoaded.begin(), stats.mostLoaded.end(), sortFunc);
    
    // Keep top 10
    if (stats.mostUsed.size() > 10) stats.mostUsed.resize(10);
    if (stats.mostLoaded.size() > 10) stats.mostLoaded.resize(10);
    
    return stats;
}

void ContentFramework::ExportAnalytics(const std::string& filepath) const {
    auto stats = GetContentStats();
    
    SimpleJson json;
    json.Set("totalContent", static_cast<double>(stats.totalContent));
    json.Set("totalLoads", static_cast<double>(stats.totalLoads));
    json.Set("totalUsage", static_cast<double>(stats.totalUsage));
    
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << json.ToString(true);
        file.close();
    }
}

void ContentFramework::GenerateDocumentation(const std::string& outputDir) const {
    // Implementation for documentation generation
    // This would create markdown files for each content type
    std::cout << "Documentation generation not yet implemented" << std::endl;
}

} // namespace NovaEngine
