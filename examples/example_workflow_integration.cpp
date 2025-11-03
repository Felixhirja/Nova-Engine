/**
 * Complete Asset Workflow Integration Example
 * 
 * This example demonstrates how to integrate the Asset Workflow System
 * into Nova Engine, showing all integration points and best practices.
 */

#include "engine/AssetWorkflow.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace AssetWorkflow;

// ============================================================================
// Helper Functions
// ============================================================================

std::string GetCurrentUser() {
    // In production, get from login system
    return "developer1";
}

void ShowNotification(const std::string& message) {
    std::cout << "[NOTIFICATION] " << message << "\n";
}

void ShowError(const std::string& message) {
    std::cerr << "[ERROR] " << message << "\n";
}

// ============================================================================
// Workflow Setup and Configuration
// ============================================================================

class WorkflowSetup {
public:
    static void Initialize() {
        std::cout << "=== Initializing Asset Workflow System ===\n\n";
        
        // 1. Initialize main workflow manager
        auto& workflow = AssetWorkflowManager::GetInstance();
        if (!workflow.Initialize("assets/")) {
            ShowError("Failed to initialize workflow system");
            return;
        }
        std::cout << "✓ Workflow Manager initialized\n";
        
        // 2. Setup quality assurance checks
        SetupQAChecks();
        std::cout << "✓ QA checks configured\n";
        
        // 3. Setup automation tasks
        SetupAutomation();
        std::cout << "✓ Automation configured\n";
        
        // 4. Setup version control
        SetupVersionControl();
        std::cout << "✓ Version control configured\n";
        
        // 5. Setup templates
        SetupTemplates();
        std::cout << "✓ Asset templates registered\n";
        
        // 6. Setup training materials
        SetupTraining();
        std::cout << "✓ Training materials loaded\n";
        
        std::cout << "\n✓ Asset Workflow System ready!\n\n";
    }
    
private:
    static void SetupQAChecks() {
        auto& qa = AssetQualityAssurance::GetInstance();
        
        // File size check
        QualityCheck size_check;
        size_check.name = "File Size Check";
        size_check.description = "Ensure assets are under 50MB";
        size_check.required = true;
        size_check.check = [](const AssetMetadata& m) {
            return m.size_bytes < 50 * 1024 * 1024; // < 50MB
        };
        qa.RegisterCheck(size_check);
        
        // Naming convention check
        QualityCheck naming_check;
        naming_check.name = "Naming Convention";
        naming_check.description = "No spaces in asset names";
        naming_check.required = false;
        naming_check.check = [](const AssetMetadata& m) {
            return m.name.find(' ') == std::string::npos;
        };
        qa.RegisterCheck(naming_check);
    }
    
    static void SetupAutomation() {
        auto& automation = AssetAutomation::GetInstance();
        
        // Auto-validate on import
        AutomationTask validate_import;
        validate_import.name = "validate_on_import";
        validate_import.trigger = AutomationRule::OnImport;
        validate_import.action = [](const std::string& path) {
            std::cout << "  → Auto-validating imported asset: " << path << "\n";
            auto& qa = AssetQualityAssurance::GetInstance();
            auto result = qa.RunQA(path);
            
            if (!result.passed) {
                std::cout << "  ✗ Validation failed:\n";
                for (const auto& fail : result.failed_checks) {
                    std::cout << "    - " << fail << "\n";
                }
            } else {
                std::cout << "  ✓ Validation passed\n";
            }
            
            return result.passed;
        };
        validate_import.enabled = true;
        automation.RegisterTask(validate_import);
        
        // Auto-commit on save
        AutomationTask commit_on_modify;
        commit_on_modify.name = "commit_on_modify";
        commit_on_modify.trigger = AutomationRule::OnModify;
        commit_on_modify.action = [](const std::string& path) {
            std::cout << "  → Auto-committing changes: " << path << "\n";
            auto& vcs = AssetVersionControl::GetInstance();
            return vcs.CommitAsset(path, "Auto-commit on save", GetCurrentUser());
        };
        commit_on_modify.enabled = true;
        automation.RegisterTask(commit_on_modify);
        
        // Generate docs on publish
        AutomationTask docs_on_publish;
        docs_on_publish.name = "docs_on_publish";
        docs_on_publish.trigger = AutomationRule::OnPublish;
        docs_on_publish.action = [](const std::string& path) {
            std::cout << "  → Generating documentation: " << path << "\n";
            auto& docs = AssetDocumentationGenerator::GetInstance();
            std::string doc_path = path + ".doc.md";
            return docs.GenerateAssetDoc(path, doc_path);
        };
        docs_on_publish.enabled = true;
        automation.RegisterTask(docs_on_publish);
    }
    
    static void SetupVersionControl() {
        auto& vcs = AssetVersionControl::GetInstance();
        vcs.Initialize("assets/");
    }
    
    static void SetupTemplates() {
        auto& tools = AssetCreationTools::GetInstance();
        
        // Register common templates
        tools.RegisterTemplate("ship_config", AssetType::Config,
                              "templates/ship_template.json");
        tools.RegisterTemplate("weapon_config", AssetType::Config,
                              "templates/weapon_template.json");
        tools.RegisterTemplate("material", AssetType::Material,
                              "templates/material_template.json");
    }
    
    static void SetupTraining() {
        auto& training = AssetTrainingSystem::GetInstance();
        
        // Add basic training material
        TrainingMaterial basics;
        basics.title = "Asset Workflow Basics";
        basics.description = "Learn the basics of the asset workflow";
        basics.content = R"(
# Asset Workflow Basics

## Step 1: Create Asset
Use templates or create blank assets.

## Step 2: Lock for Editing
Always lock before editing to prevent conflicts.

## Step 3: Make Changes
Edit your asset using the appropriate tools.

## Step 4: Run QA
Check quality before submitting.

## Step 5: Submit for Review
Get team lead approval.

## Step 6: Publish
Once approved, publish for production use.
        )";
        basics.tags = {"tutorial", "basics", "beginner"};
        training.AddTrainingMaterial(basics);
    }
};

// ============================================================================
// Asset Creation Workflow
// ============================================================================

class AssetCreationWorkflow {
public:
    static void CreateNewAsset() {
        std::cout << "=== Creating New Asset ===\n\n";
        
        auto& tools = AssetCreationTools::GetInstance();
        auto& workflow = AssetWorkflowManager::GetInstance();
        
        // 1. Create asset from template
        AssetCreationInfo info;
        info.creator = GetCurrentUser();
        info.description = "New interceptor ship configuration";
        info.source = AssetSource::Internal;
        info.creation_time = std::chrono::system_clock::now();
        
        std::string asset_path = "assets/ships/interceptor_mk2.json";
        
        std::cout << "Creating asset from template...\n";
        if (tools.CreateFromTemplate("ship_config", asset_path, info)) {
            std::cout << "✓ Asset created: " << asset_path << "\n";
            
            // 2. Set initial workflow state
            workflow.SetAssetState(asset_path, WorkflowState::Draft);
            std::cout << "✓ Workflow state set to: Draft\n";
            
            ShowNotification("New asset created: " + asset_path);
        } else {
            ShowError("Failed to create asset");
        }
        
        std::cout << "\n";
    }
};

// ============================================================================
// Asset Editing Workflow
// ============================================================================

class AssetEditingWorkflow {
public:
    static void EditAsset(const std::string& asset_path) {
        std::cout << "=== Editing Asset: " << asset_path << " ===\n\n";
        
        auto& collab = AssetCollaborationManager::GetInstance();
        auto& workflow = AssetWorkflowManager::GetInstance();
        auto& vcs = AssetVersionControl::GetInstance();
        
        std::string user = GetCurrentUser();
        
        // 1. Check if asset is locked
        if (collab.IsLocked(asset_path)) {
            std::string owner = collab.GetLockOwner(asset_path);
            if (owner != user) {
                ShowError("Asset is locked by: " + owner);
                return;
            }
        }
        
        // 2. Lock asset
        std::cout << "Locking asset for editing...\n";
        if (!collab.LockAsset(asset_path, user)) {
            ShowError("Failed to lock asset");
            return;
        }
        std::cout << "✓ Asset locked\n";
        
        // 3. Set workflow state
        workflow.SetAssetState(asset_path, WorkflowState::InProgress);
        std::cout << "✓ Workflow state: InProgress\n";
        
        // 4. Make changes (simulated)
        std::cout << "\nEditing asset...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "✓ Changes made\n";
        
        // 5. Run QA
        std::cout << "\nRunning QA checks...\n";
        auto& qa = AssetQualityAssurance::GetInstance();
        auto qa_result = qa.RunQA(asset_path);
        
        if (qa_result.passed) {
            std::cout << "✓ QA passed\n";
            std::cout << "  Passed checks: " << qa_result.passed_checks.size() << "\n";
            if (!qa_result.warnings.empty()) {
                std::cout << "  Warnings: " << qa_result.warnings.size() << "\n";
            }
        } else {
            std::cout << "✗ QA failed\n";
            for (const auto& fail : qa_result.failed_checks) {
                std::cout << "  - " << fail << "\n";
            }
        }
        
        // 6. Commit version
        std::cout << "\nCommitting changes...\n";
        vcs.CommitAsset(asset_path, "Updated ship configuration", user);
        std::cout << "✓ Version committed\n";
        
        // 7. Unlock asset
        std::cout << "\nUnlocking asset...\n";
        collab.UnlockAsset(asset_path, user);
        std::cout << "✓ Asset unlocked\n";
        
        ShowNotification("Asset saved: " + asset_path);
        
        std::cout << "\n";
    }
};

// ============================================================================
// Review Workflow
// ============================================================================

class ReviewWorkflow {
public:
    static void SubmitForReview(const std::string& asset_path) {
        std::cout << "=== Submitting for Review: " << asset_path << " ===\n\n";
        
        auto& review = AssetReviewSystem::GetInstance();
        auto& workflow = AssetWorkflowManager::GetInstance();
        
        // 1. Check if asset is ready
        auto state = workflow.GetAssetState(asset_path);
        if (state != WorkflowState::InProgress && state != WorkflowState::Draft) {
            ShowError("Asset not ready for review");
            return;
        }
        
        // 2. Run final QA
        std::cout << "Running pre-review QA...\n";
        auto& qa = AssetQualityAssurance::GetInstance();
        auto qa_result = qa.RunQA(asset_path);
        
        if (!qa_result.passed) {
            ShowError("QA must pass before review");
            std::cout << "Fix these issues:\n";
            for (const auto& fail : qa_result.failed_checks) {
                std::cout << "  - " << fail << "\n";
            }
            return;
        }
        std::cout << "✓ QA passed\n";
        
        // 3. Submit for review
        std::cout << "\nSubmitting to lead designer...\n";
        review.SubmitForReview(asset_path, "lead_designer");
        workflow.SetAssetState(asset_path, WorkflowState::PendingReview);
        std::cout << "✓ Submitted for review\n";
        
        ShowNotification("Asset submitted for review: " + asset_path);
        
        std::cout << "\n";
    }
    
    static void PerformReview(const std::string& asset_path) {
        std::cout << "=== Reviewing Asset: " << asset_path << " ===\n\n";
        
        auto& review = AssetReviewSystem::GetInstance();
        auto& workflow = AssetWorkflowManager::GetInstance();
        
        std::string reviewer = "lead_designer";
        
        // 1. Get pending reviews
        auto pending = review.GetPendingReviews(reviewer);
        std::cout << "Pending reviews: " << pending.size() << "\n\n";
        
        // 2. Create detailed review
        AssetReview detailed_review;
        detailed_review.asset_path = asset_path;
        detailed_review.reviewer = reviewer;
        detailed_review.status = ReviewStatus::InProgress;
        detailed_review.comments = "Reviewed asset thoroughly";
        detailed_review.rating = 4; // 4/5 stars
        detailed_review.review_time = std::chrono::system_clock::now();
        
        // Simulate review (in production, would be interactive)
        bool approve = true; // Simulate approval
        
        if (approve) {
            detailed_review.status = ReviewStatus::Approved;
            detailed_review.comments = "Excellent work! Ready for production.";
            detailed_review.suggestions = {
                "Consider adding particle effects",
                "Could use more detail in textures"
            };
            
            std::cout << "✓ Asset approved\n";
            std::cout << "  Rating: " << detailed_review.rating << "/5\n";
            std::cout << "  Comments: " << detailed_review.comments << "\n";
            
            review.CreateReview(detailed_review);
            review.ApproveAsset(asset_path, reviewer, "Approved");
            workflow.SetAssetState(asset_path, WorkflowState::Approved);
            
            ShowNotification("Asset approved: " + asset_path);
            
        } else {
            detailed_review.status = ReviewStatus::NeedsChanges;
            detailed_review.issues = {
                "Balance issues with weapon power",
                "Missing some metadata"
            };
            
            std::cout << "✗ Changes requested\n";
            std::cout << "  Issues:\n";
            for (const auto& issue : detailed_review.issues) {
                std::cout << "    - " << issue << "\n";
            }
            
            review.CreateReview(detailed_review);
            review.RequestChanges(asset_path, reviewer, detailed_review.issues);
            workflow.SetAssetState(asset_path, WorkflowState::ChangesRequested);
        }
        
        std::cout << "\n";
    }
};

// ============================================================================
// Publishing Workflow
// ============================================================================

class PublishingWorkflow {
public:
    static void PublishAsset(const std::string& asset_path) {
        std::cout << "=== Publishing Asset: " << asset_path << " ===\n\n";
        
        auto& workflow = AssetWorkflowManager::GetInstance();
        auto& exporter = AssetExportPipeline::GetInstance();
        auto& docs = AssetDocumentationGenerator::GetInstance();
        auto& automation = AssetAutomation::GetInstance();
        
        // 1. Check if approved
        auto state = workflow.GetAssetState(asset_path);
        if (state != WorkflowState::Approved) {
            ShowError("Asset must be approved before publishing");
            return;
        }
        
        // 2. Run automation tasks
        std::cout << "Running pre-publish automation...\n";
        automation.RunTriggeredTasks(AutomationRule::OnPublish, asset_path);
        std::cout << "✓ Automation complete\n";
        
        // 3. Export for platforms
        std::cout << "\nExporting for platforms...\n";
        exporter.ExportForPlatform(asset_path, Platform::Windows,
                                  "exports/windows/");
        std::cout << "  ✓ Windows export\n";
        
        exporter.ExportForPlatform(asset_path, Platform::Linux,
                                  "exports/linux/");
        std::cout << "  ✓ Linux export\n";
        
        exporter.ExportForPlatform(asset_path, Platform::Web,
                                  "exports/web/");
        std::cout << "  ✓ Web export\n";
        
        // 4. Generate documentation
        std::cout << "\nGenerating documentation...\n";
        docs.GenerateAssetDoc(asset_path, asset_path + ".doc.md");
        std::cout << "✓ Documentation generated\n";
        
        // 5. Set to published
        workflow.SetAssetState(asset_path, WorkflowState::Published);
        std::cout << "\n✓ Asset published successfully!\n";
        
        ShowNotification("Asset published: " + asset_path);
        
        std::cout << "\n";
    }
};

// ============================================================================
// Reporting and Analytics
// ============================================================================

class WorkflowReporting {
public:
    static void GenerateReports() {
        std::cout << "=== Generating Workflow Reports ===\n\n";
        
        auto& workflow = AssetWorkflowManager::GetInstance();
        auto& qa = AssetQualityAssurance::GetInstance();
        auto& collab = AssetCollaborationManager::GetInstance();
        auto& vcs = AssetVersionControl::GetInstance();
        
        // 1. Workflow statistics
        std::cout << "Workflow Statistics:\n";
        auto workflow_stats = workflow.GetWorkflowStats();
        std::cout << "  Assets in draft: " << workflow_stats.assets_in_draft << "\n";
        std::cout << "  Assets in progress: " << workflow_stats.assets_in_progress << "\n";
        std::cout << "  Pending review: " << workflow_stats.assets_pending_review << "\n";
        std::cout << "  Approved: " << workflow_stats.assets_approved << "\n";
        std::cout << "  Published: " << workflow_stats.assets_published << "\n\n";
        
        // 2. QA statistics
        std::cout << "QA Statistics:\n";
        auto qa_stats = qa.GetQAStats();
        std::cout << "  Total checks: " << qa_stats.total_checks << "\n";
        std::cout << "  Passed: " << qa_stats.passed << "\n";
        std::cout << "  Failed: " << qa_stats.failed << "\n";
        std::cout << "  Warnings: " << qa_stats.warnings << "\n\n";
        
        // 3. Review statistics
        std::cout << "Review Statistics:\n";
        auto& review = AssetReviewSystem::GetInstance();
        auto review_stats = review.GetReviewStats();
        std::cout << "  Total reviews: " << review_stats.total_reviews << "\n";
        std::cout << "  Approved: " << review_stats.approved << "\n";
        std::cout << "  Rejected: " << review_stats.rejected << "\n";
        std::cout << "  Pending: " << review_stats.pending << "\n\n";
        
        // 4. Export comprehensive reports
        std::cout << "Exporting reports...\n";
        workflow.ExportWorkflowReport("reports/workflow_report.md");
        qa.ExportQAReport("reports/qa_report.md");
        collab.ExportCollaborationReport("reports/collaboration_report.md");
        vcs.ExportVersionHistory("reports/version_history.md");
        std::cout << "✓ Reports exported to reports/\n\n";
    }
    
    static void ShowSystemStatus() {
        std::cout << "=== Asset Workflow System Status ===\n\n";
        
        auto& workflow = AssetWorkflowManager::GetInstance();
        auto status = workflow.GetSystemStatus();
        
        std::cout << "Subsystem Status:\n";
        std::cout << "  Creation Tools:  " << (status.creation_tools_ready ? "✓" : "✗") << "\n";
        std::cout << "  Import Pipeline: " << (status.import_pipeline_ready ? "✓" : "✗") << "\n";
        std::cout << "  Export Pipeline: " << (status.export_pipeline_ready ? "✓" : "✗") << "\n";
        std::cout << "  Review System:   " << (status.review_system_ready ? "✓" : "✗") << "\n";
        std::cout << "  Collaboration:   " << (status.collaboration_ready ? "✓" : "✗") << "\n";
        std::cout << "  Version Control: " << (status.version_control_ready ? "✓" : "✗") << "\n";
        std::cout << "  Automation:      " << (status.automation_ready ? "✓" : "✗") << "\n";
        std::cout << "  QA:              " << (status.qa_ready ? "✓" : "✗") << "\n";
        std::cout << "  Documentation:   " << (status.documentation_ready ? "✓" : "✗") << "\n";
        std::cout << "  Training:        " << (status.training_ready ? "✓" : "✗") << "\n\n";
    }
};

// ============================================================================
// Main Integration Example
// ============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║      Nova Engine - Asset Workflow Integration Example     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    // 1. Initialize workflow system
    WorkflowSetup::Initialize();
    
    // 2. Show system status
    WorkflowReporting::ShowSystemStatus();
    
    // 3. Complete workflow example
    std::string asset_path = "assets/ships/interceptor_mk2.json";
    
    // Create new asset
    AssetCreationWorkflow::CreateNewAsset();
    
    // Edit asset
    AssetEditingWorkflow::EditAsset(asset_path);
    
    // Submit for review
    ReviewWorkflow::SubmitForReview(asset_path);
    
    // Perform review
    ReviewWorkflow::PerformReview(asset_path);
    
    // Publish asset
    PublishingWorkflow::PublishAsset(asset_path);
    
    // 4. Generate reports
    WorkflowReporting::GenerateReports();
    
    // 5. Cleanup
    std::cout << "=== Shutting Down ===\n\n";
    auto& workflow = AssetWorkflowManager::GetInstance();
    workflow.Shutdown();
    std::cout << "✓ Asset Workflow System shut down\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              Integration Example Complete!                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
