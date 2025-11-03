#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <queue>
#include <atomic>

/**
 * Nova Engine Asset Workflow System
 * 
 * Comprehensive workflow management for asset development teams:
 * - Asset creation and editing tools
 * - Automated import/export pipelines
 * - Review and approval workflows
 * - Team collaboration features
 * - Version control integration
 * - Automated processing workflows
 * - Quality assurance tools
 * - Documentation generation
 * - Training and onboarding materials
 */

namespace AssetWorkflow {

using namespace AssetPipeline;
using time_point = std::chrono::system_clock::time_point;
using duration = std::chrono::milliseconds;

// ============================================================================
// Asset Workflow Types
// ============================================================================

enum class WorkflowState {
    Draft,           // Initial creation
    InProgress,      // Being worked on
    PendingReview,   // Ready for review
    InReview,        // Under review
    ChangesRequested,// Needs revisions
    Approved,        // Approved for use
    Published,       // Available in production
    Archived         // No longer in use
};

enum class AssetSource {
    Internal,        // Created in-house
    External,        // From external source
    Generated,       // Procedurally generated
    Imported,        // Imported from file
    Modified         // Modified from existing
};

enum class ReviewStatus {
    Pending,
    InProgress,
    Approved,
    Rejected,
    NeedsChanges
};

enum class QualityLevel {
    Placeholder,     // Temp placeholder
    Draft,           // First draft
    Production,      // Production quality
    Final,           // Final/polished
    Gold             // Gold master
};

enum class AutomationRule {
    OnImport,        // Run on asset import
    OnModify,        // Run when modified
    OnReview,        // Run before review
    OnPublish,       // Run before publish
    Scheduled        // Run on schedule
};

struct AssetCreationInfo {
    std::string path;
    std::string creator;
    std::string description;
    AssetSource source = AssetSource::Internal;
    time_point creation_time;
    std::unordered_map<std::string, std::string> metadata;
};

struct AssetReview {
    std::string asset_path;
    std::string reviewer;
    ReviewStatus status = ReviewStatus::Pending;
    time_point review_time;
    std::string comments;
    std::vector<std::string> issues;
    std::vector<std::string> suggestions;
    int rating = 0; // 1-5 stars
};

struct AssetVersion {
    int version_number;
    std::string author;
    time_point timestamp;
    std::string commit_hash;
    std::string description;
    std::vector<std::string> changes;
    size_t file_size;
};

struct AssetCollaboration {
    std::string asset_path;
    std::string owner;
    std::vector<std::string> contributors;
    std::vector<std::string> editors;
    std::vector<std::string> reviewers;
    bool locked = false;
    std::string locked_by;
    time_point lock_time;
};

struct ImportTask {
    std::string source_path;
    std::string destination_path;
    AssetType type;
    std::unordered_map<std::string, std::string> import_options;
    std::function<bool(const std::string&)> validator;
    std::function<void(const std::string&)> post_process;
};

struct ExportTask {
    std::string asset_path;
    std::string export_path;
    Platform target_platform;
    std::unordered_map<std::string, std::string> export_options;
    std::function<bool(const std::string&)> pre_export_hook;
};

struct AutomationTask {
    std::string name;
    AutomationRule trigger;
    std::vector<std::string> affected_paths;
    std::function<bool(const std::string&)> action;
    time_point last_run;
    bool enabled = true;
};

struct QualityCheck {
    std::string name;
    std::function<bool(const AssetMetadata&)> check;
    std::string description;
    bool required = true;
};

struct TrainingMaterial {
    std::string title;
    std::string description;
    std::string content;
    std::vector<std::string> related_assets;
    std::vector<std::string> tags;
};

// ============================================================================
// Asset Creation Tools
// ============================================================================

class AssetCreationTools {
public:
    static AssetCreationTools& GetInstance();
    
    // Create new asset from template
    bool CreateFromTemplate(const std::string& template_name,
                           const std::string& output_path,
                           const AssetCreationInfo& info);
    
    // Create blank asset
    bool CreateBlankAsset(const std::string& path,
                         AssetType type,
                         const AssetCreationInfo& info);
    
    // Clone existing asset
    bool CloneAsset(const std::string& source_path,
                   const std::string& dest_path,
                   const AssetCreationInfo& info);
    
    // Register asset template
    void RegisterTemplate(const std::string& name,
                         AssetType type,
                         const std::string& template_path);
    
    // Get available templates
    std::vector<std::string> GetTemplates(AssetType type) const;
    
    // Get creation history
    std::vector<AssetCreationInfo> GetCreationHistory(const std::string& creator) const;
    
    // Set default creator
    void SetDefaultCreator(const std::string& creator);
    
private:
    AssetCreationTools() = default;
    
    std::unordered_map<std::string, std::string> templates_;
    std::vector<AssetCreationInfo> creation_history_;
    std::string default_creator_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Import Pipeline
// ============================================================================

class AssetImportPipeline {
public:
    static AssetImportPipeline& GetInstance();
    
    // Import single asset
    bool ImportAsset(const ImportTask& task);
    
    // Batch import
    bool ImportBatch(const std::vector<ImportTask>& tasks);
    
    // Auto-detect and import
    bool AutoImport(const std::string& source_dir,
                   const std::string& dest_dir);
    
    // Register import validator
    void RegisterValidator(AssetType type,
                          std::function<bool(const std::string&)> validator);
    
    // Register post-import processor
    void RegisterPostProcessor(AssetType type,
                              std::function<void(const std::string&)> processor);
    
    // Get import statistics
    struct ImportStats {
        size_t total_imports = 0;
        size_t successful_imports = 0;
        size_t failed_imports = 0;
        duration total_time{0};
        duration average_time{0};
    };
    ImportStats GetImportStats() const;
    
    // Get import log
    std::vector<std::string> GetImportLog() const;
    
private:
    AssetImportPipeline() = default;
    
    std::unordered_map<AssetType, std::function<bool(const std::string&)>> validators_;
    std::unordered_map<AssetType, std::function<void(const std::string&)>> post_processors_;
    std::vector<std::string> import_log_;
    ImportStats stats_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Export Pipeline
// ============================================================================

class AssetExportPipeline {
public:
    static AssetExportPipeline& GetInstance();
    
    // Export single asset
    bool ExportAsset(const ExportTask& task);
    
    // Batch export
    bool ExportBatch(const std::vector<ExportTask>& tasks);
    
    // Export for platform
    bool ExportForPlatform(const std::string& asset_path,
                          Platform platform,
                          const std::string& output_dir);
    
    // Export all assets
    bool ExportAll(const std::string& output_dir,
                  Platform platform = Platform::All);
    
    // Register export processor
    void RegisterExportProcessor(Platform platform,
                                AssetType type,
                                std::function<bool(const std::string&, const std::string&)> processor);
    
    // Get export statistics
    struct ExportStats {
        size_t total_exports = 0;
        size_t successful_exports = 0;
        size_t failed_exports = 0;
        duration total_time{0};
    };
    ExportStats GetExportStats() const;
    
private:
    AssetExportPipeline() = default;
    
    using ProcessorKey = std::pair<Platform, AssetType>;
    struct ProcessorKeyHash {
        size_t operator()(const ProcessorKey& k) const {
            return std::hash<int>()(static_cast<int>(k.first)) ^
                   std::hash<int>()(static_cast<int>(k.second));
        }
    };
    
    std::unordered_map<ProcessorKey, 
                      std::function<bool(const std::string&, const std::string&)>,
                      ProcessorKeyHash> processors_;
    ExportStats stats_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Review System
// ============================================================================

class AssetReviewSystem {
public:
    static AssetReviewSystem& GetInstance();
    
    // Submit asset for review
    bool SubmitForReview(const std::string& asset_path,
                        const std::string& reviewer);
    
    // Create review
    bool CreateReview(const AssetReview& review);
    
    // Update review
    bool UpdateReview(const std::string& asset_path,
                     const AssetReview& review);
    
    // Get reviews for asset
    std::vector<AssetReview> GetReviews(const std::string& asset_path) const;
    
    // Get pending reviews
    std::vector<std::string> GetPendingReviews(const std::string& reviewer) const;
    
    // Approve asset
    bool ApproveAsset(const std::string& asset_path,
                     const std::string& reviewer,
                     const std::string& comments = "");
    
    // Reject asset
    bool RejectAsset(const std::string& asset_path,
                    const std::string& reviewer,
                    const std::string& reason);
    
    // Request changes
    bool RequestChanges(const std::string& asset_path,
                       const std::string& reviewer,
                       const std::vector<std::string>& changes);
    
    // Get review statistics
    struct ReviewStats {
        size_t total_reviews = 0;
        size_t approved = 0;
        size_t rejected = 0;
        size_t pending = 0;
        duration average_review_time{0};
    };
    ReviewStats GetReviewStats() const;
    
private:
    AssetReviewSystem() = default;
    
    std::unordered_map<std::string, std::vector<AssetReview>> reviews_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Collaboration
// ============================================================================

class AssetCollaborationManager {
public:
    static AssetCollaborationManager& GetInstance();
    
    // Lock asset for editing
    bool LockAsset(const std::string& asset_path,
                  const std::string& user);
    
    // Unlock asset
    bool UnlockAsset(const std::string& asset_path,
                    const std::string& user);
    
    // Check if asset is locked
    bool IsLocked(const std::string& asset_path) const;
    
    // Get lock owner
    std::string GetLockOwner(const std::string& asset_path) const;
    
    // Add contributor
    bool AddContributor(const std::string& asset_path,
                       const std::string& user);
    
    // Remove contributor
    bool RemoveContributor(const std::string& asset_path,
                          const std::string& user);
    
    // Set asset owner
    bool SetOwner(const std::string& asset_path,
                 const std::string& user);
    
    // Get collaboration info
    AssetCollaboration GetCollaborationInfo(const std::string& asset_path) const;
    
    // Get user's assets
    std::vector<std::string> GetUserAssets(const std::string& user) const;
    
    // Export collaboration report
    bool ExportCollaborationReport(const std::string& output_path) const;
    
private:
    AssetCollaborationManager() = default;
    
    std::unordered_map<std::string, AssetCollaboration> collaborations_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Version Control
// ============================================================================

class AssetVersionControl {
public:
    static AssetVersionControl& GetInstance();
    
    // Initialize VCS integration
    bool Initialize(const std::string& repo_path);
    
    // Commit asset changes
    bool CommitAsset(const std::string& asset_path,
                    const std::string& message,
                    const std::string& author);
    
    // Get asset history
    std::vector<AssetVersion> GetHistory(const std::string& asset_path) const;
    
    // Revert to version
    bool RevertToVersion(const std::string& asset_path,
                        int version_number);
    
    // Compare versions
    std::vector<std::string> CompareVersions(const std::string& asset_path,
                                            int version1,
                                            int version2) const;
    
    // Get current version
    int GetCurrentVersion(const std::string& asset_path) const;
    
    // Tag version
    bool TagVersion(const std::string& asset_path,
                   int version,
                   const std::string& tag);
    
    // Branch asset
    bool BranchAsset(const std::string& asset_path,
                    const std::string& branch_name);
    
    // Merge asset
    bool MergeAsset(const std::string& asset_path,
                   const std::string& source_branch);
    
    // Check for conflicts
    bool HasConflicts(const std::string& asset_path) const;
    
    // Export version history
    bool ExportVersionHistory(const std::string& output_path) const;
    
private:
    AssetVersionControl() = default;
    
    std::string repo_path_;
    std::unordered_map<std::string, std::vector<AssetVersion>> history_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Automation
// ============================================================================

class AssetAutomation {
public:
    static AssetAutomation& GetInstance();
    
    // Register automation task
    bool RegisterTask(const AutomationTask& task);
    
    // Unregister task
    bool UnregisterTask(const std::string& task_name);
    
    // Enable/disable task
    bool EnableTask(const std::string& task_name, bool enabled);
    
    // Run task manually
    bool RunTask(const std::string& task_name,
                const std::string& asset_path);
    
    // Run all tasks for trigger
    bool RunTriggeredTasks(AutomationRule trigger,
                          const std::string& asset_path);
    
    // Schedule task
    bool ScheduleTask(const std::string& task_name,
                     time_point run_time);
    
    // Get scheduled tasks
    std::vector<AutomationTask> GetScheduledTasks() const;
    
    // Update automation (call regularly)
    void Update();
    
    // Get automation statistics
    struct AutomationStats {
        size_t total_tasks = 0;
        size_t successful_runs = 0;
        size_t failed_runs = 0;
        time_point last_run_time;
    };
    AutomationStats GetAutomationStats() const;
    
private:
    AssetAutomation() = default;
    
    std::unordered_map<std::string, AutomationTask> tasks_;
    std::priority_queue<std::pair<time_point, std::string>> scheduled_;
    AutomationStats stats_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Quality Assurance
// ============================================================================

class AssetQualityAssurance {
public:
    static AssetQualityAssurance& GetInstance();
    
    // Register quality check
    void RegisterCheck(const QualityCheck& check);
    
    // Run QA on asset
    struct QAResult {
        bool passed = true;
        std::vector<std::string> passed_checks;
        std::vector<std::string> failed_checks;
        std::vector<std::string> warnings;
        QualityLevel quality_level = QualityLevel::Draft;
    };
    QAResult RunQA(const std::string& asset_path);
    
    // Run QA on all assets
    std::unordered_map<std::string, QAResult> RunQABatch(
        const std::vector<std::string>& asset_paths);
    
    // Set quality level
    bool SetQualityLevel(const std::string& asset_path, QualityLevel level);
    
    // Get quality level
    QualityLevel GetQualityLevel(const std::string& asset_path) const;
    
    // Get QA statistics
    struct QAStats {
        size_t total_checks = 0;
        size_t passed = 0;
        size_t failed = 0;
        size_t warnings = 0;
    };
    QAStats GetQAStats() const;
    
    // Export QA report
    bool ExportQAReport(const std::string& output_path) const;
    
private:
    AssetQualityAssurance() = default;
    
    std::vector<QualityCheck> checks_;
    std::unordered_map<std::string, QualityLevel> quality_levels_;
    QAStats stats_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Documentation Generator
// ============================================================================

class AssetDocumentationGenerator {
public:
    static AssetDocumentationGenerator& GetInstance();
    
    // Generate documentation for asset
    bool GenerateAssetDoc(const std::string& asset_path,
                         const std::string& output_path);
    
    // Generate workflow documentation
    bool GenerateWorkflowDoc(const std::string& output_path);
    
    // Generate team guide
    bool GenerateTeamGuide(const std::string& output_path);
    
    // Generate asset catalog
    bool GenerateAssetCatalog(const std::string& output_path);
    
    // Add custom section
    void AddCustomSection(const std::string& title,
                         const std::string& content);
    
    // Set documentation template
    void SetTemplate(const std::string& template_path);
    
private:
    AssetDocumentationGenerator() = default;
    
    std::unordered_map<std::string, std::string> custom_sections_;
    std::string template_path_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Training System
// ============================================================================

class AssetTrainingSystem {
public:
    static AssetTrainingSystem& GetInstance();
    
    // Add training material
    bool AddTrainingMaterial(const TrainingMaterial& material);
    
    // Get training materials by tag
    std::vector<TrainingMaterial> GetMaterialsByTag(const std::string& tag) const;
    
    // Get training for asset type
    std::vector<TrainingMaterial> GetTrainingForAssetType(AssetType type) const;
    
    // Generate onboarding guide
    bool GenerateOnboardingGuide(const std::string& output_path);
    
    // Generate best practices guide
    bool GenerateBestPractices(const std::string& output_path);
    
    // Generate quick reference
    bool GenerateQuickReference(const std::string& output_path);
    
    // Export all training materials
    bool ExportAllMaterials(const std::string& output_dir) const;
    
private:
    AssetTrainingSystem() = default;
    
    std::vector<TrainingMaterial> materials_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Workflow Manager (Main Orchestrator)
// ============================================================================

class AssetWorkflowManager {
public:
    static AssetWorkflowManager& GetInstance();
    
    // Initialize workflow system
    bool Initialize(const std::string& assets_dir);
    
    // Shutdown workflow system
    void Shutdown();
    
    // Update workflow (call regularly)
    void Update();
    
    // Get workflow state for asset
    WorkflowState GetAssetState(const std::string& asset_path) const;
    
    // Set workflow state
    bool SetAssetState(const std::string& asset_path, WorkflowState state);
    
    // Move asset through workflow
    bool AdvanceWorkflow(const std::string& asset_path);
    
    // Get workflow statistics
    struct WorkflowStats {
        size_t assets_in_draft = 0;
        size_t assets_in_progress = 0;
        size_t assets_pending_review = 0;
        size_t assets_approved = 0;
        size_t assets_published = 0;
        duration average_workflow_time{0};
    };
    WorkflowStats GetWorkflowStats() const;
    
    // Export comprehensive workflow report
    bool ExportWorkflowReport(const std::string& output_path) const;
    
    // Get all subsystems status
    struct SystemStatus {
        bool creation_tools_ready = false;
        bool import_pipeline_ready = false;
        bool export_pipeline_ready = false;
        bool review_system_ready = false;
        bool collaboration_ready = false;
        bool version_control_ready = false;
        bool automation_ready = false;
        bool qa_ready = false;
        bool documentation_ready = false;
        bool training_ready = false;
    };
    SystemStatus GetSystemStatus() const;
    
private:
    AssetWorkflowManager() = default;
    
    std::string assets_dir_;
    std::unordered_map<std::string, WorkflowState> asset_states_;
    mutable std::mutex mutex_;
    bool initialized_ = false;
};

} // namespace AssetWorkflow
