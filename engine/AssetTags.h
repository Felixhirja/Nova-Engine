#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace AssetPipeline {

/**
 * Asset Tagging System
 * Flexible tagging for asset organization and discovery
 */

class AssetTags {
public:
    static AssetTags& GetInstance();

    // Tag operations
    void AddTag(const std::string& asset_path, const std::string& key, const std::string& value);
    void RemoveTag(const std::string& asset_path, const std::string& key);
    void ClearTags(const std::string& asset_path);
    
    std::optional<std::string> GetTag(const std::string& asset_path, const std::string& key);
    std::unordered_map<std::string, std::string> GetAllTags(const std::string& asset_path);
    bool HasTag(const std::string& asset_path, const std::string& key);
    
    // Bulk tagging
    void AddTagToMany(const std::vector<std::string>& asset_paths, const std::string& key, const std::string& value);
    void RemoveTagFromMany(const std::vector<std::string>& asset_paths, const std::string& key);
    
    // Query by tags
    std::vector<std::string> FindAssetsByTag(const std::string& key, const std::string& value);
    std::vector<std::string> FindAssetsByTags(const std::unordered_map<std::string, std::string>& tags);
    std::vector<std::string> FindAssetsWithTagKey(const std::string& key);
    
    // Tag statistics
    std::unordered_set<std::string> GetAllTagKeys();
    std::unordered_set<std::string> GetAllTagValues(const std::string& key);
    size_t GetTagCount(const std::string& key);
    
    // Predefined tag categories
    struct CommonTags {
        static constexpr const char* CATEGORY = "category";     // UI, Gameplay, Audio, etc.
        static constexpr const char* QUALITY = "quality";       // Low, Medium, High, Ultra
        static constexpr const char* LOD = "lod";               // LOD0, LOD1, LOD2
        static constexpr const char* PLATFORM = "platform";     // Windows, Linux, Web
        static constexpr const char* AUTHOR = "author";         // Creator name
        static constexpr const char* LICENSE = "license";       // Asset license
        static constexpr const char* STATUS = "status";         // WIP, Final, Deprecated
        static constexpr const char* VERSION = "version";       // Version number
        static constexpr const char* FEATURE = "feature";       // Related feature
        static constexpr const char* SCENE = "scene";           // Scene association
    };
    
    // Tag validation
    bool ValidateTag(const std::string& key, const std::string& value);
    void RegisterTagValidator(const std::string& key, std::function<bool(const std::string&)> validator);
    
    // Tag templates
    void ApplyTemplate(const std::string& asset_path, const std::string& template_name);
    void RegisterTemplate(const std::string& name, const std::unordered_map<std::string, std::string>& tags);
    std::vector<std::string> GetTemplates();
    
    // Import/Export
    bool ExportTags(const std::string& file_path);
    bool ImportTags(const std::string& file_path);

private:
    AssetTags() = default;
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> asset_tags_;
    std::unordered_map<std::string, std::function<bool(const std::string&)>> tag_validators_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> tag_templates_;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
