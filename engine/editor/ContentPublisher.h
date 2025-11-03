#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace ContentManagement {

/**
 * ContentPublisher: Publishing pipeline for content releases
 * 
 * Features:
 * - Multi-stage publishing workflow (dev -> staging -> production)
 * - Content bundling and packaging
 * - Incremental updates and patches
 * - Release scheduling and automation
 * - Rollback capabilities
 * - Publishing approvals and gates
 */
class ContentPublisher {
public:
    enum class PublishStage {
        Development,
        Testing,
        Staging,
        Production,
        Archived
    };

    enum class PublishStatus {
        Pending,
        InProgress,
        Success,
        Failed,
        Rolled_Back
    };

    struct PublishTarget {
        std::string id;
        std::string name;
        PublishStage stage;
        std::string url;
        std::string apiKey;
        std::unordered_map<std::string, std::string> metadata;
        bool requiresApproval;
    };

    struct ContentBundle {
        std::string bundleId;
        std::string name;
        std::string version;
        std::vector<std::string> contentIds;
        std::vector<std::string> dependencies;
        std::chrono::system_clock::time_point createdTime;
        std::string author;
        size_t totalSize;
        std::string checksum;
    };

    struct PublishJob {
        std::string jobId;
        ContentBundle bundle;
        std::vector<PublishTarget> targets;
        PublishStatus status;
        std::string errorMessage;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        float progress;  // 0.0 to 1.0
        std::vector<std::string> logs;
        bool requiresApproval;
        std::vector<std::string> approvers;
        std::vector<std::string> approvals;
    };

    struct ReleaseSchedule {
        std::string scheduleId;
        std::string bundleId;
        std::chrono::system_clock::time_point scheduledTime;
        std::vector<PublishTarget> targets;
        bool autoPublish;
        bool notifyOnComplete;
        std::vector<std::string> notificationRecipients;
    };

    ContentPublisher();
    ~ContentPublisher();

    // Bundle Management
    std::string CreateBundle(const std::string& name, const std::vector<std::string>& contentIds, const std::string& author);
    bool AddToBundle(const std::string& bundleId, const std::string& contentId);
    bool RemoveFromBundle(const std::string& bundleId, const std::string& contentId);
    bool ValidateBundle(const std::string& bundleId, std::vector<std::string>& errors) const;
    
    const ContentBundle* GetBundle(const std::string& bundleId) const;
    std::vector<ContentBundle> GetAllBundles() const;
    bool DeleteBundle(const std::string& bundleId);
    
    // Publishing
    std::string PublishBundle(const std::string& bundleId, const std::vector<PublishTarget>& targets);
    std::string PublishIncremental(const std::string& baseBundleId, const std::vector<std::string>& changedContentIds, const std::vector<PublishTarget>& targets);
    bool CancelPublish(const std::string& jobId);
    bool RollbackPublish(const std::string& jobId);
    
    // Job Management
    const PublishJob* GetPublishJob(const std::string& jobId) const;
    std::vector<PublishJob> GetActiveJobs() const;
    std::vector<PublishJob> GetJobHistory(int maxJobs = 50) const;
    PublishStatus GetJobStatus(const std::string& jobId) const;
    float GetJobProgress(const std::string& jobId) const;
    
    // Approvals
    bool RequestApproval(const std::string& jobId, const std::vector<std::string>& approvers);
    bool ApprovePublish(const std::string& jobId, const std::string& approver, const std::string& comments = "");
    bool RejectPublish(const std::string& jobId, const std::string& approver, const std::string& reason);
    std::vector<std::string> GetPendingApprovals(const std::string& approver) const;
    
    // Scheduling
    std::string SchedulePublish(const std::string& bundleId, const std::chrono::system_clock::time_point& scheduledTime, const std::vector<PublishTarget>& targets);
    bool CancelScheduledPublish(const std::string& scheduleId);
    bool ReschedulePublish(const std::string& scheduleId, const std::chrono::system_clock::time_point& newTime);
    
    std::vector<ReleaseSchedule> GetScheduledPublishes() const;
    void ProcessScheduledPublishes();  // Called by update loop
    
    // Target Management
    void RegisterPublishTarget(const PublishTarget& target);
    void UpdatePublishTarget(const std::string& targetId, const PublishTarget& target);
    void RemovePublishTarget(const std::string& targetId);
    
    std::vector<PublishTarget> GetPublishTargets(PublishStage stage = PublishStage::Production) const;
    const PublishTarget* GetPublishTarget(const std::string& targetId) const;
    
    // Packaging
    bool ExportBundle(const std::string& bundleId, const std::string& outputPath, const std::string& format = "zip");
    std::string ImportBundle(const std::string& filePath);
    bool GeneratePatch(const std::string& fromBundleId, const std::string& toBundleId, const std::string& outputPath);
    
    // Validation
    bool ValidateTargetConnection(const std::string& targetId, std::string& errorMessage) const;
    bool TestPublish(const std::string& bundleId, const std::string& targetId, std::vector<std::string>& warnings) const;
    
    // Monitoring
    void SetOnPublishStarted(std::function<void(const std::string&)> callback);
    void SetOnPublishCompleted(std::function<void(const std::string&, bool)> callback);
    void SetOnPublishProgress(std::function<void(const std::string&, float)> callback);
    
    // Reporting
    std::string GeneratePublishReport(const std::string& jobId) const;
    std::string GenerateReleaseNotes(const std::string& bundleId) const;
    void ExportPublishHistory(const std::string& outputPath) const;
    
    // UI Integration
    void RenderPublishDashboard();
    void RenderBundleCreator();
    void RenderPublishQueue();
    void RenderScheduledPublishes();
    
private:
    bool PublishToTarget(const ContentBundle& bundle, const PublishTarget& target, PublishJob& job);
    bool UploadContent(const std::string& contentId, const PublishTarget& target, PublishJob& job);
    std::string CalculateChecksum(const std::vector<std::string>& contentIds) const;
    
    std::unordered_map<std::string, ContentBundle> bundles_;
    std::unordered_map<std::string, PublishJob> publishJobs_;
    std::unordered_map<std::string, ReleaseSchedule> schedules_;
    std::unordered_map<std::string, PublishTarget> targets_;
    
    std::function<void(const std::string&)> onPublishStarted_;
    std::function<void(const std::string&, bool)> onPublishCompleted_;
    std::function<void(const std::string&, float)> onPublishProgress_;
};

} // namespace ContentManagement
