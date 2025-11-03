#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include "ContentFramework.h"

namespace NovaEngine {

// Version information
struct ContentVersion {
    std::string version;  // Semantic version (e.g., "1.2.3")
    std::string author;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::string hash;  // Content hash for integrity
    SimpleJson snapshot;  // Full content snapshot
};

// Version control system for content
class ContentVersionControl {
public:
    static ContentVersionControl& Instance() {
        static ContentVersionControl instance;
        return instance;
    }
    
    // Version management
    std::string CommitContent(const ContentDefinition& content, 
                             const std::string& message,
                             const std::string& author);
    
    bool RevertToVersion(const std::string& contentId, const std::string& version);
    std::unique_ptr<ContentDefinition> GetVersion(const std::string& contentId, 
                                                   const std::string& version) const;
    
    // History
    std::vector<ContentVersion> GetHistory(const std::string& contentId) const;
    size_t GetVersionCount(const std::string& contentId) const;
    
    // Comparison
    struct ContentDiff {
        std::string field;
        std::string oldValue;
        std::string newValue;
        std::string changeType;  // "added", "removed", "modified"
    };
    
    std::vector<ContentDiff> CompareVersions(const std::string& contentId,
                                            const std::string& version1,
                                            const std::string& version2) const;
    
    std::vector<ContentDiff> CompareToCurrent(const std::string& contentId,
                                             const std::string& version) const;
    
    // Branch management
    void CreateBranch(const std::string& branchName, const std::string& fromBranch = "main");
    void SwitchBranch(const std::string& branchName);
    std::vector<std::string> GetBranches() const;
    std::string GetCurrentBranch() const;
    
    // Merge
    bool MergeBranch(const std::string& sourceBranch, const std::string& targetBranch);
    
    // Tags
    void CreateTag(const std::string& tagName, const std::string& contentId, 
                  const std::string& version);
    std::string GetTaggedVersion(const std::string& tagName) const;
    
    // Export/Import
    bool ExportHistory(const std::string& contentId, const std::string& filepath) const;
    bool ImportHistory(const std::string& contentId, const std::string& filepath);
    
private:
    ContentVersionControl() = default;
    
    std::string ComputeHash(const SimpleJson& json) const;
    
    // contentId -> versions
    std::unordered_map<std::string, std::vector<ContentVersion>> history_;
    
    // Branch management
    std::string currentBranch_ = "main";
    std::unordered_map<std::string, std::string> branches_;  // branch -> parent
    
    // Tags
    std::unordered_map<std::string, std::pair<std::string, std::string>> tags_;  // tag -> (contentId, version)
};

// Content migration system for version upgrades
class ContentMigration {
public:
    using MigrationFunc = std::function<bool(SimpleJson&, std::vector<std::string>&)>;
    
    struct Migration {
        std::string fromVersion;
        std::string toVersion;
        std::string description;
        MigrationFunc migrateFunc;
    };
    
    static ContentMigration& Instance() {
        static ContentMigration instance;
        return instance;
    }
    
    // Register migration
    void RegisterMigration(const std::string& contentType, const Migration& migration);
    
    // Execute migration
    bool MigrateContent(ContentDefinition& content, 
                       const std::string& targetVersion,
                       std::vector<std::string>& messages) const;
    
    bool MigrateAllContent(const std::string& contentType,
                          const std::string& targetVersion,
                          std::unordered_map<std::string, std::vector<std::string>>& results);
    
    // Query migrations
    std::vector<Migration> GetMigrationPath(const std::string& contentType,
                                           const std::string& fromVersion,
                                           const std::string& toVersion) const;
    
private:
    ContentMigration() = default;
    
    // contentType -> migrations
    std::unordered_map<std::string, std::vector<Migration>> migrations_;
};

// Content changelog generator
class ContentChangelog {
public:
    static ContentChangelog& Instance() {
        static ContentChangelog instance;
        return instance;
    }
    
    struct ChangelogEntry {
        std::string version;
        std::chrono::system_clock::time_point date;
        std::string author;
        std::vector<std::string> added;
        std::vector<std::string> changed;
        std::vector<std::string> removed;
        std::vector<std::string> fixed;
    };
    
    // Generate changelog from version history
    std::vector<ChangelogEntry> GenerateChangelog(const std::string& contentId) const;
    
    // Export changelog
    std::string ExportMarkdown(const std::vector<ChangelogEntry>& changelog) const;
    bool SaveChangelog(const std::string& contentId, const std::string& filepath) const;
    
private:
    ContentChangelog() = default;
};

// Semantic versioning utilities
class SemanticVersion {
public:
    SemanticVersion(const std::string& version);
    SemanticVersion(int major, int minor, int patch);
    
    int GetMajor() const { return major_; }
    int GetMinor() const { return minor_; }
    int GetPatch() const { return patch_; }
    
    std::string ToString() const;
    
    bool operator<(const SemanticVersion& other) const;
    bool operator>(const SemanticVersion& other) const;
    bool operator==(const SemanticVersion& other) const;
    bool operator<=(const SemanticVersion& other) const;
    bool operator>=(const SemanticVersion& other) const;
    
    SemanticVersion BumpMajor() const;
    SemanticVersion BumpMinor() const;
    SemanticVersion BumpPatch() const;
    
    bool IsCompatible(const SemanticVersion& other) const;  // Same major version
    
private:
    int major_ = 0;
    int minor_ = 0;
    int patch_ = 0;
};

} // namespace NovaEngine
