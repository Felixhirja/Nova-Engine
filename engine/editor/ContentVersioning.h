#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace ContentManagement {

/**
 * ContentVersioning: Version control for content updates
 * 
 * Features:
 * - Full version history for all content
 * - Diff and comparison tools
 * - Rollback and restore
 * - Branch and merge support
 * - Conflict resolution
 * - Change annotations and comments
 */
class ContentVersioning {
public:
    struct ContentVersion {
        std::string versionId;
        std::string contentId;
        int versionNumber;
        std::unique_ptr<simplejson::JsonObject> snapshot;
        std::string author;
        std::string commitMessage;
        std::chrono::system_clock::time_point timestamp;
        std::vector<std::string> tags;
        std::string parentVersionId;
        bool isMilestone;
    };

    struct ContentDiff {
        std::string field;
        std::string oldValue;
        std::string newValue;
        std::string changeType;  // "added", "removed", "modified"
    };

    struct ChangeSet {
        std::string changeSetId;
        std::vector<std::string> contentIds;
        std::string description;
        std::string author;
        std::chrono::system_clock::time_point timestamp;
        bool isCommitted;
    };

    struct Branch {
        std::string branchId;
        std::string name;
        std::string description;
        std::string baseVersionId;
        std::string author;
        std::chrono::system_clock::time_point createdTime;
        bool isActive;
    };

    ContentVersioning();
    ~ContentVersioning();

    // Version Management
    std::string CommitVersion(const std::string& contentId, const simplejson::JsonObject& content, const std::string& message, const std::string& author);
    bool RestoreVersion(const std::string& contentId, const std::string& versionId);
    bool DeleteVersion(const std::string& versionId);
    
    const ContentVersion* GetVersion(const std::string& versionId) const;
    std::vector<ContentVersion> GetVersionHistory(const std::string& contentId, int maxVersions = -1) const;
    ContentVersion GetLatestVersion(const std::string& contentId) const;
    ContentVersion GetVersionByNumber(const std::string& contentId, int versionNumber) const;
    
    // Comparison
    std::vector<ContentDiff> CompareVersions(const std::string& versionId1, const std::string& versionId2) const;
    std::vector<ContentDiff> CompareWithCurrent(const std::string& contentId, const std::string& versionId) const;
    std::string GenerateDiffReport(const std::vector<ContentDiff>& diffs) const;
    
    // Rollback
    bool RollbackToVersion(const std::string& contentId, const std::string& versionId);
    bool RollbackToTime(const std::string& contentId, const std::chrono::system_clock::time_point& timestamp);
    bool RollbackChanges(const std::string& contentId, int versionCount = 1);
    
    // Branching
    std::string CreateBranch(const std::string& name, const std::string& description, const std::string& baseVersionId, const std::string& author);
    bool SwitchBranch(const std::string& branchId);
    bool MergeBranch(const std::string& sourceBranchId, const std::string& targetBranchId, std::vector<std::string>& conflicts);
    bool DeleteBranch(const std::string& branchId);
    
    std::vector<Branch> GetAllBranches() const;
    Branch GetCurrentBranch() const;
    
    // Change Sets
    std::string CreateChangeSet(const std::string& description, const std::string& author);
    void AddToChangeSet(const std::string& changeSetId, const std::string& contentId);
    bool CommitChangeSet(const std::string& changeSetId);
    bool RevertChangeSet(const std::string& changeSetId);
    
    std::vector<ChangeSet> GetPendingChangeSets() const;
    const ChangeSet* GetChangeSet(const std::string& changeSetId) const;
    
    // Tagging
    void TagVersion(const std::string& versionId, const std::string& tag);
    void RemoveTag(const std::string& versionId, const std::string& tag);
    std::vector<std::string> GetVersionsByTag(const std::string& tag) const;
    
    // Milestones
    void MarkAsMilestone(const std::string& versionId, bool isMilestone = true);
    std::vector<ContentVersion> GetMilestones(const std::string& contentId) const;
    
    // Search & Query
    std::vector<ContentVersion> SearchVersions(const std::string& query) const;
    std::vector<ContentVersion> GetVersionsByAuthor(const std::string& author) const;
    std::vector<ContentVersion> GetVersionsByDateRange(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) const;
    
    // Statistics
    int GetVersionCount(const std::string& contentId) const;
    std::vector<std::pair<std::string, int>> GetMostModifiedContent(int count = 10) const;
    std::vector<std::pair<std::string, int>> GetAuthorContributions() const;
    
    // Conflict Resolution
    bool HasConflicts(const std::string& contentId) const;
    std::vector<ContentDiff> GetConflicts(const std::string& contentId) const;
    bool ResolveConflict(const std::string& contentId, const std::string& field, const std::string& resolution);
    
    // Persistence
    bool SaveVersionHistory(const std::string& outputPath) const;
    bool LoadVersionHistory(const std::string& inputPath);
    
    // Cleanup
    void PruneOldVersions(int keepVersions);
    void CompressHistory(const std::string& contentId);
    
    // UI Integration
    void RenderVersionHistory(const std::string& contentId);
    void RenderDiffViewer(const std::string& versionId1, const std::string& versionId2);
    void RenderBranchManager();
    
private:
    std::vector<ContentDiff> GenerateDiff(const simplejson::JsonObject& oldContent, const simplejson::JsonObject& newContent) const;
    void DiffObject(const simplejson::JsonObject& oldObj, const simplejson::JsonObject& newObj, const std::string& prefix, std::vector<ContentDiff>& diffs) const;
    
    std::unique_ptr<simplejson::JsonObject> ApplyDiff(const simplejson::JsonObject& base, const std::vector<ContentDiff>& diffs) const;
    std::unique_ptr<simplejson::JsonObject> MergeContent(const simplejson::JsonObject& base, const simplejson::JsonObject& source, const simplejson::JsonObject& target, std::vector<ContentDiff>& conflicts) const;
    
    std::string GenerateVersionId() const;
    int GetNextVersionNumber(const std::string& contentId) const;
    
    std::unordered_map<std::string, std::vector<ContentVersion>> versionHistory_;  // contentId -> versions
    std::unordered_map<std::string, Branch> branches_;
    std::unordered_map<std::string, ChangeSet> changeSets_;
    std::string currentBranchId_;
};

} // namespace ContentManagement
