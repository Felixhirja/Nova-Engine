#include "AssetWorkflow.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace AssetWorkflow {

// ============================================================================
// Asset Creation Tools Implementation
// ============================================================================

AssetCreationTools& AssetCreationTools::GetInstance() {
    static AssetCreationTools instance;
    return instance;
}

bool AssetCreationTools::CreateFromTemplate(const std::string& template_name,
                                           const std::string& output_path,
                                           const AssetCreationInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = templates_.find(template_name);
    if (it == templates_.end()) {
        std::cerr << "Template not found: " << template_name << "\n";
        return false;
    }
    
    try {
        fs::copy_file(it->second, output_path, fs::copy_options::overwrite_existing);
        creation_history_.push_back(info);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create from template: " << e.what() << "\n";
        return false;
    }
}

bool AssetCreationTools::CreateBlankAsset(const std::string& path,
                                         AssetType type,
                                         const AssetCreationInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        
        // Create minimal content based on type
        switch (type) {
            case AssetType::Config:
                file << "{\n  \"version\": 1\n}\n";
                break;
            case AssetType::Script:
                file << "// New script\n";
                break;
            default:
                file << "// " << info.description << "\n";
                break;
        }
        
        file.close();
        creation_history_.push_back(info);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create blank asset: " << e.what() << "\n";
        return false;
    }
}

bool AssetCreationTools::CloneAsset(const std::string& source_path,
                                   const std::string& dest_path,
                                   const AssetCreationInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        fs::copy_file(source_path, dest_path, fs::copy_options::overwrite_existing);
        creation_history_.push_back(info);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to clone asset: " << e.what() << "\n";
        return false;
    }
}

void AssetCreationTools::RegisterTemplate(const std::string& name,
                                         AssetType type,
                                         const std::string& template_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    templates_[name] = template_path;
}

std::vector<std::string> AssetCreationTools::GetTemplates(AssetType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    
    for (const auto& [name, path] : templates_) {
        result.push_back(name);
    }
    
    return result;
}

std::vector<AssetCreationInfo> AssetCreationTools::GetCreationHistory(
    const std::string& creator) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AssetCreationInfo> result;
    
    for (const auto& info : creation_history_) {
        if (info.creator == creator) {
            result.push_back(info);
        }
    }
    
    return result;
}

void AssetCreationTools::SetDefaultCreator(const std::string& creator) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_creator_ = creator;
}

// ============================================================================
// Asset Import Pipeline Implementation
// ============================================================================

AssetImportPipeline& AssetImportPipeline::GetInstance() {
    static AssetImportPipeline instance;
    return instance;
}

bool AssetImportPipeline::ImportAsset(const ImportTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Validate if validator exists
        if (task.validator && !task.validator(task.source_path)) {
            import_log_.push_back("Validation failed: " + task.source_path);
            stats_.failed_imports++;
            return false;
        }
        
        // Copy file
        fs::copy_file(task.source_path, task.destination_path,
                     fs::copy_options::overwrite_existing);
        
        // Run post-processor if exists
        if (task.post_process) {
            task.post_process(task.destination_path);
        } else {
            // Check if type has a registered post-processor
            auto it = post_processors_.find(task.type);
            if (it != post_processors_.end()) {
                it->second(task.destination_path);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        stats_.total_imports++;
        stats_.successful_imports++;
        stats_.total_time += duration;
        stats_.average_time = stats_.total_time / stats_.total_imports;
        
        import_log_.push_back("Imported: " + task.destination_path);
        return true;
        
    } catch (const std::exception& e) {
        import_log_.push_back("Import failed: " + std::string(e.what()));
        stats_.failed_imports++;
        return false;
    }
}

bool AssetImportPipeline::ImportBatch(const std::vector<ImportTask>& tasks) {
    bool all_success = true;
    for (const auto& task : tasks) {
        if (!ImportAsset(task)) {
            all_success = false;
        }
    }
    return all_success;
}

bool AssetImportPipeline::AutoImport(const std::string& source_dir,
                                    const std::string& dest_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
            if (entry.is_regular_file()) {
                auto rel_path = fs::relative(entry.path(), source_dir);
                auto dest_path = fs::path(dest_dir) / rel_path;
                
                fs::create_directories(dest_path.parent_path());
                fs::copy_file(entry.path(), dest_path,
                            fs::copy_options::overwrite_existing);
                
                import_log_.push_back("Auto-imported: " + dest_path.string());
                stats_.successful_imports++;
            }
        }
        return true;
    } catch (const std::exception& e) {
        import_log_.push_back("Auto-import failed: " + std::string(e.what()));
        return false;
    }
}

void AssetImportPipeline::RegisterValidator(AssetType type,
                                           std::function<bool(const std::string&)> validator) {
    std::lock_guard<std::mutex> lock(mutex_);
    validators_[type] = validator;
}

void AssetImportPipeline::RegisterPostProcessor(AssetType type,
                                               std::function<void(const std::string&)> processor) {
    std::lock_guard<std::mutex> lock(mutex_);
    post_processors_[type] = processor;
}

AssetImportPipeline::ImportStats AssetImportPipeline::GetImportStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::vector<std::string> AssetImportPipeline::GetImportLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return import_log_;
}

// ============================================================================
// Asset Export Pipeline Implementation
// ============================================================================

AssetExportPipeline& AssetExportPipeline::GetInstance() {
    static AssetExportPipeline instance;
    return instance;
}

bool AssetExportPipeline::ExportAsset(const ExportTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Run pre-export hook if exists
        if (task.pre_export_hook && !task.pre_export_hook(task.asset_path)) {
            stats_.failed_exports++;
            return false;
        }
        
        // Copy file
        fs::create_directories(fs::path(task.export_path).parent_path());
        fs::copy_file(task.asset_path, task.export_path,
                     fs::copy_options::overwrite_existing);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        stats_.total_exports++;
        stats_.successful_exports++;
        stats_.total_time += duration;
        
        return true;
        
    } catch (const std::exception& e) {
        stats_.failed_exports++;
        return false;
    }
}

bool AssetExportPipeline::ExportBatch(const std::vector<ExportTask>& tasks) {
    bool all_success = true;
    for (const auto& task : tasks) {
        if (!ExportAsset(task)) {
            all_success = false;
        }
    }
    return all_success;
}

bool AssetExportPipeline::ExportForPlatform(const std::string& asset_path,
                                           Platform platform,
                                           const std::string& output_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        auto filename = fs::path(asset_path).filename();
        auto output_path = fs::path(output_dir) / filename;
        
        fs::create_directories(output_dir);
        fs::copy_file(asset_path, output_path, fs::copy_options::overwrite_existing);
        
        stats_.successful_exports++;
        return true;
    } catch (const std::exception& e) {
        stats_.failed_exports++;
        return false;
    }
}

bool AssetExportPipeline::ExportAll(const std::string& output_dir, Platform platform) {
    // Would integrate with AssetPipelineManager to get all assets
    return true;
}

void AssetExportPipeline::RegisterExportProcessor(
    Platform platform, AssetType type,
    std::function<bool(const std::string&, const std::string&)> processor) {
    std::lock_guard<std::mutex> lock(mutex_);
    processors_[{platform, type}] = processor;
}

AssetExportPipeline::ExportStats AssetExportPipeline::GetExportStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// ============================================================================
// Asset Review System Implementation
// ============================================================================

AssetReviewSystem& AssetReviewSystem::GetInstance() {
    static AssetReviewSystem instance;
    return instance;
}

bool AssetReviewSystem::SubmitForReview(const std::string& asset_path,
                                        const std::string& reviewer) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetReview review;
    review.asset_path = asset_path;
    review.reviewer = reviewer;
    review.status = ReviewStatus::Pending;
    review.review_time = std::chrono::system_clock::now();
    
    reviews_[asset_path].push_back(review);
    return true;
}

bool AssetReviewSystem::CreateReview(const AssetReview& review) {
    std::lock_guard<std::mutex> lock(mutex_);
    reviews_[review.asset_path].push_back(review);
    return true;
}

bool AssetReviewSystem::UpdateReview(const std::string& asset_path,
                                    const AssetReview& review) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& asset_reviews = reviews_[asset_path];
    for (auto& r : asset_reviews) {
        if (r.reviewer == review.reviewer) {
            r = review;
            return true;
        }
    }
    
    asset_reviews.push_back(review);
    return true;
}

std::vector<AssetReview> AssetReviewSystem::GetReviews(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = reviews_.find(asset_path);
    if (it != reviews_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> AssetReviewSystem::GetPendingReviews(const std::string& reviewer) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    
    for (const auto& [path, asset_reviews] : reviews_) {
        for (const auto& review : asset_reviews) {
            if (review.reviewer == reviewer && review.status == ReviewStatus::Pending) {
                result.push_back(path);
                break;
            }
        }
    }
    
    return result;
}

bool AssetReviewSystem::ApproveAsset(const std::string& asset_path,
                                    const std::string& reviewer,
                                    const std::string& comments) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetReview review;
    review.asset_path = asset_path;
    review.reviewer = reviewer;
    review.status = ReviewStatus::Approved;
    review.comments = comments;
    review.review_time = std::chrono::system_clock::now();
    
    reviews_[asset_path].push_back(review);
    return true;
}

bool AssetReviewSystem::RejectAsset(const std::string& asset_path,
                                   const std::string& reviewer,
                                   const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetReview review;
    review.asset_path = asset_path;
    review.reviewer = reviewer;
    review.status = ReviewStatus::Rejected;
    review.comments = reason;
    review.review_time = std::chrono::system_clock::now();
    
    reviews_[asset_path].push_back(review);
    return true;
}

bool AssetReviewSystem::RequestChanges(const std::string& asset_path,
                                      const std::string& reviewer,
                                      const std::vector<std::string>& changes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetReview review;
    review.asset_path = asset_path;
    review.reviewer = reviewer;
    review.status = ReviewStatus::NeedsChanges;
    review.issues = changes;
    review.review_time = std::chrono::system_clock::now();
    
    reviews_[asset_path].push_back(review);
    return true;
}

AssetReviewSystem::ReviewStats AssetReviewSystem::GetReviewStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ReviewStats stats;
    for (const auto& [path, asset_reviews] : reviews_) {
        for (const auto& review : asset_reviews) {
            stats.total_reviews++;
            switch (review.status) {
                case ReviewStatus::Approved:
                    stats.approved++;
                    break;
                case ReviewStatus::Rejected:
                    stats.rejected++;
                    break;
                case ReviewStatus::Pending:
                    stats.pending++;
                    break;
                default:
                    break;
            }
        }
    }
    
    return stats;
}

// ============================================================================
// Asset Collaboration Manager Implementation
// ============================================================================

AssetCollaborationManager& AssetCollaborationManager::GetInstance() {
    static AssetCollaborationManager instance;
    return instance;
}

bool AssetCollaborationManager::LockAsset(const std::string& asset_path,
                                         const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& collab = collaborations_[asset_path];
    if (collab.locked) {
        return false;
    }
    
    collab.asset_path = asset_path;
    collab.locked = true;
    collab.locked_by = user;
    collab.lock_time = std::chrono::system_clock::now();
    
    return true;
}

bool AssetCollaborationManager::UnlockAsset(const std::string& asset_path,
                                           const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = collaborations_.find(asset_path);
    if (it == collaborations_.end()) {
        return false;
    }
    
    if (it->second.locked_by != user) {
        return false; // Only lock owner can unlock
    }
    
    it->second.locked = false;
    it->second.locked_by = "";
    
    return true;
}

bool AssetCollaborationManager::IsLocked(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = collaborations_.find(asset_path);
    return it != collaborations_.end() && it->second.locked;
}

std::string AssetCollaborationManager::GetLockOwner(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = collaborations_.find(asset_path);
    if (it != collaborations_.end() && it->second.locked) {
        return it->second.locked_by;
    }
    return "";
}

bool AssetCollaborationManager::AddContributor(const std::string& asset_path,
                                              const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& collab = collaborations_[asset_path];
    collab.contributors.push_back(user);
    return true;
}

bool AssetCollaborationManager::RemoveContributor(const std::string& asset_path,
                                                 const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = collaborations_.find(asset_path);
    if (it == collaborations_.end()) {
        return false;
    }
    
    auto& contributors = it->second.contributors;
    contributors.erase(std::remove(contributors.begin(), contributors.end(), user),
                      contributors.end());
    return true;
}

bool AssetCollaborationManager::SetOwner(const std::string& asset_path,
                                        const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& collab = collaborations_[asset_path];
    collab.owner = user;
    return true;
}

AssetCollaboration AssetCollaborationManager::GetCollaborationInfo(
    const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = collaborations_.find(asset_path);
    if (it != collaborations_.end()) {
        return it->second;
    }
    return AssetCollaboration();
}

std::vector<std::string> AssetCollaborationManager::GetUserAssets(
    const std::string& user) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    
    for (const auto& [path, collab] : collaborations_) {
        if (collab.owner == user) {
            result.push_back(path);
        }
    }
    
    return result;
}

bool AssetCollaborationManager::ExportCollaborationReport(
    const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Collaboration Report\n\n";
        
        for (const auto& [path, collab] : collaborations_) {
            file << "## " << path << "\n";
            file << "- Owner: " << collab.owner << "\n";
            file << "- Contributors: " << collab.contributors.size() << "\n";
            file << "- Locked: " << (collab.locked ? "Yes" : "No") << "\n";
            if (collab.locked) {
                file << "- Locked by: " << collab.locked_by << "\n";
            }
            file << "\n";
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// Asset Version Control Implementation
// ============================================================================

AssetVersionControl& AssetVersionControl::GetInstance() {
    static AssetVersionControl instance;
    return instance;
}

bool AssetVersionControl::Initialize(const std::string& repo_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    repo_path_ = repo_path;
    return fs::exists(repo_path);
}

bool AssetVersionControl::CommitAsset(const std::string& asset_path,
                                     const std::string& message,
                                     const std::string& author) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetVersion version;
    version.version_number = static_cast<int>(history_[asset_path].size()) + 1;
    version.author = author;
    version.timestamp = std::chrono::system_clock::now();
    version.description = message;
    
    try {
        version.file_size = fs::file_size(asset_path);
    } catch (...) {
        version.file_size = 0;
    }
    
    history_[asset_path].push_back(version);
    return true;
}

std::vector<AssetVersion> AssetVersionControl::GetHistory(
    const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = history_.find(asset_path);
    if (it != history_.end()) {
        return it->second;
    }
    return {};
}

bool AssetVersionControl::RevertToVersion(const std::string& asset_path,
                                         int version_number) {
    // Would integrate with actual VCS
    return true;
}

std::vector<std::string> AssetVersionControl::CompareVersions(
    const std::string& asset_path, int version1, int version2) const {
    // Would integrate with actual VCS diff
    return {"Comparison not yet implemented"};
}

int AssetVersionControl::GetCurrentVersion(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = history_.find(asset_path);
    if (it != history_.end() && !it->second.empty()) {
        return it->second.back().version_number;
    }
    return 0;
}

bool AssetVersionControl::TagVersion(const std::string& asset_path,
                                    int version,
                                    const std::string& tag) {
    // Would integrate with actual VCS tagging
    return true;
}

bool AssetVersionControl::BranchAsset(const std::string& asset_path,
                                     const std::string& branch_name) {
    // Would integrate with actual VCS branching
    return true;
}

bool AssetVersionControl::MergeAsset(const std::string& asset_path,
                                    const std::string& source_branch) {
    // Would integrate with actual VCS merging
    return true;
}

bool AssetVersionControl::HasConflicts(const std::string& asset_path) const {
    // Would check for merge conflicts
    return false;
}

bool AssetVersionControl::ExportVersionHistory(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Version History\n\n";
        
        for (const auto& [path, versions] : history_) {
            file << "## " << path << "\n\n";
            for (const auto& ver : versions) {
                file << "### Version " << ver.version_number << "\n";
                file << "- Author: " << ver.author << "\n";
                file << "- Description: " << ver.description << "\n";
                file << "- Size: " << ver.file_size << " bytes\n\n";
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// Asset Automation Implementation
// ============================================================================

AssetAutomation& AssetAutomation::GetInstance() {
    static AssetAutomation instance;
    return instance;
}

bool AssetAutomation::RegisterTask(const AutomationTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_[task.name] = task;
    stats_.total_tasks++;
    return true;
}

bool AssetAutomation::UnregisterTask(const std::string& task_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.erase(task_name) > 0;
}

bool AssetAutomation::EnableTask(const std::string& task_name, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(task_name);
    if (it != tasks_.end()) {
        it->second.enabled = enabled;
        return true;
    }
    return false;
}

bool AssetAutomation::RunTask(const std::string& task_name,
                             const std::string& asset_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(task_name);
    if (it == tasks_.end() || !it->second.enabled) {
        return false;
    }
    
    try {
        bool success = it->second.action(asset_path);
        it->second.last_run = std::chrono::system_clock::now();
        stats_.last_run_time = it->second.last_run;
        
        if (success) {
            stats_.successful_runs++;
        } else {
            stats_.failed_runs++;
        }
        
        return success;
    } catch (const std::exception& e) {
        stats_.failed_runs++;
        return false;
    }
}

bool AssetAutomation::RunTriggeredTasks(AutomationRule trigger,
                                       const std::string& asset_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool all_success = true;
    for (auto& [name, task] : tasks_) {
        if (task.enabled && task.trigger == trigger) {
            try {
                bool success = task.action(asset_path);
                if (!success) all_success = false;
                
                task.last_run = std::chrono::system_clock::now();
                stats_.last_run_time = task.last_run;
                
                if (success) {
                    stats_.successful_runs++;
                } else {
                    stats_.failed_runs++;
                }
            } catch (...) {
                all_success = false;
                stats_.failed_runs++;
            }
        }
    }
    
    return all_success;
}

bool AssetAutomation::ScheduleTask(const std::string& task_name, time_point run_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (tasks_.find(task_name) == tasks_.end()) {
        return false;
    }
    
    scheduled_.push({run_time, task_name});
    return true;
}

std::vector<AutomationTask> AssetAutomation::GetScheduledTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AutomationTask> result;
    
    auto temp_queue = scheduled_;
    while (!temp_queue.empty()) {
        auto task_name = temp_queue.top().second;
        auto it = tasks_.find(task_name);
        if (it != tasks_.end()) {
            result.push_back(it->second);
        }
        temp_queue.pop();
    }
    
    return result;
}

void AssetAutomation::Update() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    while (!scheduled_.empty() && scheduled_.top().first <= now) {
        auto task_name = scheduled_.top().second;
        scheduled_.pop();
        
        auto it = tasks_.find(task_name);
        if (it != tasks_.end() && it->second.enabled) {
            // Run task on all affected paths
            for (const auto& path : it->second.affected_paths) {
                try {
                    it->second.action(path);
                    stats_.successful_runs++;
                } catch (...) {
                    stats_.failed_runs++;
                }
            }
            it->second.last_run = now;
        }
    }
}

AssetAutomation::AutomationStats AssetAutomation::GetAutomationStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// ============================================================================
// Asset Quality Assurance Implementation
// ============================================================================

AssetQualityAssurance& AssetQualityAssurance::GetInstance() {
    static AssetQualityAssurance instance;
    return instance;
}

void AssetQualityAssurance::RegisterCheck(const QualityCheck& check) {
    std::lock_guard<std::mutex> lock(mutex_);
    checks_.push_back(check);
}

AssetQualityAssurance::QAResult AssetQualityAssurance::RunQA(
    const std::string& asset_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    QAResult result;
    result.passed = true;
    
    // Get asset metadata from pipeline
    AssetMetadata metadata;
    metadata.path = asset_path;
    metadata.name = fs::path(asset_path).filename().string();
    
    try {
        metadata.size_bytes = fs::file_size(asset_path);
    } catch (...) {
        metadata.size_bytes = 0;
    }
    
    // Run all checks
    for (const auto& check : checks_) {
        try {
            bool check_passed = check.check(metadata);
            stats_.total_checks++;
            
            if (check_passed) {
                result.passed_checks.push_back(check.name);
                stats_.passed++;
            } else {
                if (check.required) {
                    result.failed_checks.push_back(check.name);
                    result.passed = false;
                    stats_.failed++;
                } else {
                    result.warnings.push_back(check.name);
                    stats_.warnings++;
                }
            }
        } catch (...) {
            result.failed_checks.push_back(check.name + " (exception)");
            result.passed = false;
            stats_.failed++;
        }
    }
    
    // Determine quality level based on results
    if (result.passed && result.warnings.empty()) {
        result.quality_level = QualityLevel::Production;
    } else if (result.passed) {
        result.quality_level = QualityLevel::Draft;
    } else {
        result.quality_level = QualityLevel::Placeholder;
    }
    
    return result;
}

std::unordered_map<std::string, AssetQualityAssurance::QAResult>
AssetQualityAssurance::RunQABatch(const std::vector<std::string>& asset_paths) {
    std::unordered_map<std::string, QAResult> results;
    
    for (const auto& path : asset_paths) {
        results[path] = RunQA(path);
    }
    
    return results;
}

bool AssetQualityAssurance::SetQualityLevel(const std::string& asset_path,
                                           QualityLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    quality_levels_[asset_path] = level;
    return true;
}

QualityLevel AssetQualityAssurance::GetQualityLevel(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = quality_levels_.find(asset_path);
    if (it != quality_levels_.end()) {
        return it->second;
    }
    return QualityLevel::Draft;
}

AssetQualityAssurance::QAStats AssetQualityAssurance::GetQAStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

bool AssetQualityAssurance::ExportQAReport(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Quality Assurance Report\n\n";
        file << "## Statistics\n\n";
        file << "- Total Checks: " << stats_.total_checks << "\n";
        file << "- Passed: " << stats_.passed << "\n";
        file << "- Failed: " << stats_.failed << "\n";
        file << "- Warnings: " << stats_.warnings << "\n\n";
        
        file << "## Quality Levels\n\n";
        for (const auto& [path, level] : quality_levels_) {
            file << "- " << path << ": ";
            switch (level) {
                case QualityLevel::Placeholder: file << "Placeholder"; break;
                case QualityLevel::Draft: file << "Draft"; break;
                case QualityLevel::Production: file << "Production"; break;
                case QualityLevel::Final: file << "Final"; break;
                case QualityLevel::Gold: file << "Gold"; break;
            }
            file << "\n";
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// Asset Documentation Generator Implementation
// ============================================================================

AssetDocumentationGenerator& AssetDocumentationGenerator::GetInstance() {
    static AssetDocumentationGenerator instance;
    return instance;
}

bool AssetDocumentationGenerator::GenerateAssetDoc(const std::string& asset_path,
                                                   const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Documentation: " << fs::path(asset_path).filename().string() << "\n\n";
        file << "**Path:** `" << asset_path << "`\n\n";
        
        try {
            file << "**Size:** " << fs::file_size(asset_path) << " bytes\n";
        } catch (...) {
            file << "**Size:** Unknown\n";
        }
        
        for (const auto& [title, content] : custom_sections_) {
            file << "\n## " << title << "\n\n";
            file << content << "\n";
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetDocumentationGenerator::GenerateWorkflowDoc(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Workflow Documentation\n\n";
        file << "## Workflow States\n\n";
        file << "1. **Draft** - Initial creation\n";
        file << "2. **In Progress** - Being worked on\n";
        file << "3. **Pending Review** - Ready for review\n";
        file << "4. **In Review** - Under review\n";
        file << "5. **Changes Requested** - Needs revisions\n";
        file << "6. **Approved** - Approved for use\n";
        file << "7. **Published** - Available in production\n";
        file << "8. **Archived** - No longer in use\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetDocumentationGenerator::GenerateTeamGuide(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Team Collaboration Guide\n\n";
        file << "## Getting Started\n\n";
        file << "1. Lock the asset before editing\n";
        file << "2. Make your changes\n";
        file << "3. Submit for review\n";
        file << "4. Address review comments\n";
        file << "5. Unlock when complete\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetDocumentationGenerator::GenerateAssetCatalog(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Catalog\n\n";
        file << "Complete listing of all game assets.\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void AssetDocumentationGenerator::AddCustomSection(const std::string& title,
                                                   const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_sections_[title] = content;
}

void AssetDocumentationGenerator::SetTemplate(const std::string& template_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    template_path_ = template_path;
}

// ============================================================================
// Asset Training System Implementation
// ============================================================================

AssetTrainingSystem& AssetTrainingSystem::GetInstance() {
    static AssetTrainingSystem instance;
    return instance;
}

bool AssetTrainingSystem::AddTrainingMaterial(const TrainingMaterial& material) {
    std::lock_guard<std::mutex> lock(mutex_);
    materials_.push_back(material);
    return true;
}

std::vector<TrainingMaterial> AssetTrainingSystem::GetMaterialsByTag(
    const std::string& tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrainingMaterial> result;
    
    for (const auto& material : materials_) {
        if (std::find(material.tags.begin(), material.tags.end(), tag) !=
            material.tags.end()) {
            result.push_back(material);
        }
    }
    
    return result;
}

std::vector<TrainingMaterial> AssetTrainingSystem::GetTrainingForAssetType(
    AssetType type) const {
    // Would filter based on asset type
    std::lock_guard<std::mutex> lock(mutex_);
    return materials_;
}

bool AssetTrainingSystem::GenerateOnboardingGuide(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Pipeline Onboarding Guide\n\n";
        file << "## Welcome!\n\n";
        file << "This guide will help you get started with the asset pipeline.\n\n";
        file << "## Quick Start\n\n";
        file << "1. Initialize the workflow system\n";
        file << "2. Create your first asset\n";
        file << "3. Submit for review\n";
        file << "4. Iterate based on feedback\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetTrainingSystem::GenerateBestPractices(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Pipeline Best Practices\n\n";
        file << "## DO\n\n";
        file << "- Always lock assets before editing\n";
        file << "- Submit for review early and often\n";
        file << "- Write clear commit messages\n";
        file << "- Keep assets organized\n\n";
        file << "## DON'T\n\n";
        file << "- Don't bypass the review process\n";
        file << "- Don't keep assets locked unnecessarily\n";
        file << "- Don't ignore QA warnings\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetTrainingSystem::GenerateQuickReference(const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Workflow Quick Reference\n\n";
        file << "## Common Commands\n\n";
        file << "- Lock asset: `LockAsset(path, user)`\n";
        file << "- Submit review: `SubmitForReview(path, reviewer)`\n";
        file << "- Import asset: `ImportAsset(task)`\n";
        file << "- Export asset: `ExportAsset(task)`\n\n";
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool AssetTrainingSystem::ExportAllMaterials(const std::string& output_dir) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        fs::create_directories(output_dir);
        
        for (size_t i = 0; i < materials_.size(); ++i) {
            const auto& material = materials_[i];
            std::string filename = "training_" + std::to_string(i) + ".md";
            std::ofstream file(fs::path(output_dir) / filename);
            
            if (file.is_open()) {
                file << "# " << material.title << "\n\n";
                file << material.description << "\n\n";
                file << material.content << "\n";
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// Asset Workflow Manager Implementation
// ============================================================================

AssetWorkflowManager& AssetWorkflowManager::GetInstance() {
    static AssetWorkflowManager instance;
    return instance;
}

bool AssetWorkflowManager::Initialize(const std::string& assets_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    assets_dir_ = assets_dir;
    initialized_ = true;
    
    // Initialize all subsystems
    AssetVersionControl::GetInstance().Initialize(assets_dir);
    
    std::cout << "Asset Workflow System initialized for: " << assets_dir << "\n";
    
    return true;
}

void AssetWorkflowManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    asset_states_.clear();
    initialized_ = false;
    
    std::cout << "Asset Workflow System shut down.\n";
}

void AssetWorkflowManager::Update() {
    if (!initialized_) return;
    
    // Update automation tasks
    AssetAutomation::GetInstance().Update();
}

WorkflowState AssetWorkflowManager::GetAssetState(const std::string& asset_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = asset_states_.find(asset_path);
    if (it != asset_states_.end()) {
        return it->second;
    }
    return WorkflowState::Draft;
}

bool AssetWorkflowManager::SetAssetState(const std::string& asset_path,
                                        WorkflowState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    asset_states_[asset_path] = state;
    return true;
}

bool AssetWorkflowManager::AdvanceWorkflow(const std::string& asset_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto current_state = asset_states_[asset_path];
    
    switch (current_state) {
        case WorkflowState::Draft:
            asset_states_[asset_path] = WorkflowState::InProgress;
            break;
        case WorkflowState::InProgress:
            asset_states_[asset_path] = WorkflowState::PendingReview;
            break;
        case WorkflowState::PendingReview:
            asset_states_[asset_path] = WorkflowState::InReview;
            break;
        case WorkflowState::InReview:
            asset_states_[asset_path] = WorkflowState::Approved;
            break;
        case WorkflowState::Approved:
            asset_states_[asset_path] = WorkflowState::Published;
            break;
        default:
            return false;
    }
    
    return true;
}

AssetWorkflowManager::WorkflowStats AssetWorkflowManager::GetWorkflowStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    WorkflowStats stats;
    
    for (const auto& [path, state] : asset_states_) {
        switch (state) {
            case WorkflowState::Draft:
                stats.assets_in_draft++;
                break;
            case WorkflowState::InProgress:
                stats.assets_in_progress++;
                break;
            case WorkflowState::PendingReview:
            case WorkflowState::InReview:
                stats.assets_pending_review++;
                break;
            case WorkflowState::Approved:
                stats.assets_approved++;
                break;
            case WorkflowState::Published:
                stats.assets_published++;
                break;
            default:
                break;
        }
    }
    
    return stats;
}

bool AssetWorkflowManager::ExportWorkflowReport(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(output_path);
        if (!file.is_open()) return false;
        
        file << "# Asset Workflow Report\n\n";
        file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
        
        auto stats = GetWorkflowStats();
        file << "## Workflow Statistics\n\n";
        file << "- Assets in Draft: " << stats.assets_in_draft << "\n";
        file << "- Assets in Progress: " << stats.assets_in_progress << "\n";
        file << "- Assets Pending Review: " << stats.assets_pending_review << "\n";
        file << "- Assets Approved: " << stats.assets_approved << "\n";
        file << "- Assets Published: " << stats.assets_published << "\n\n";
        
        auto import_stats = AssetImportPipeline::GetInstance().GetImportStats();
        file << "## Import Statistics\n\n";
        file << "- Total Imports: " << import_stats.total_imports << "\n";
        file << "- Successful: " << import_stats.successful_imports << "\n";
        file << "- Failed: " << import_stats.failed_imports << "\n\n";
        
        auto export_stats = AssetExportPipeline::GetInstance().GetExportStats();
        file << "## Export Statistics\n\n";
        file << "- Total Exports: " << export_stats.total_exports << "\n";
        file << "- Successful: " << export_stats.successful_exports << "\n";
        file << "- Failed: " << export_stats.failed_exports << "\n\n";
        
        auto review_stats = AssetReviewSystem::GetInstance().GetReviewStats();
        file << "## Review Statistics\n\n";
        file << "- Total Reviews: " << review_stats.total_reviews << "\n";
        file << "- Approved: " << review_stats.approved << "\n";
        file << "- Rejected: " << review_stats.rejected << "\n";
        file << "- Pending: " << review_stats.pending << "\n\n";
        
        auto qa_stats = AssetQualityAssurance::GetInstance().GetQAStats();
        file << "## Quality Assurance\n\n";
        file << "- Total Checks: " << qa_stats.total_checks << "\n";
        file << "- Passed: " << qa_stats.passed << "\n";
        file << "- Failed: " << qa_stats.failed << "\n";
        file << "- Warnings: " << qa_stats.warnings << "\n\n";
        
        auto auto_stats = AssetAutomation::GetInstance().GetAutomationStats();
        file << "## Automation\n\n";
        file << "- Total Tasks: " << auto_stats.total_tasks << "\n";
        file << "- Successful Runs: " << auto_stats.successful_runs << "\n";
        file << "- Failed Runs: " << auto_stats.failed_runs << "\n\n";
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to export workflow report: " << e.what() << "\n";
        return false;
    }
}

AssetWorkflowManager::SystemStatus AssetWorkflowManager::GetSystemStatus() const {
    SystemStatus status;
    status.creation_tools_ready = true;
    status.import_pipeline_ready = true;
    status.export_pipeline_ready = true;
    status.review_system_ready = true;
    status.collaboration_ready = true;
    status.version_control_ready = true;
    status.automation_ready = true;
    status.qa_ready = true;
    status.documentation_ready = true;
    status.training_ready = true;
    return status;
}

} // namespace AssetWorkflow
