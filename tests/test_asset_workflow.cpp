#include "engine/AssetWorkflow.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace AssetWorkflow;

void TestCreationTools() {
    std::cout << "Testing Asset Creation Tools...\n";
    
    auto& tools = AssetCreationTools::GetInstance();
    
    // Setup
    tools.SetDefaultCreator("test_user");
    
    // Create blank asset
    AssetCreationInfo info;
    info.creator = "test_user";
    info.description = "Test asset";
    info.source = AssetSource::Internal;
    info.creation_time = std::chrono::system_clock::now();
    
    bool created = tools.CreateBlankAsset("test_blank.json", AssetType::Config, info);
    assert(created && "Should create blank asset");
    
    // Clone asset
    bool cloned = tools.CloneAsset("test_blank.json", "test_clone.json", info);
    assert(cloned && "Should clone asset");
    
    // Register template
    tools.RegisterTemplate("test_template", AssetType::Config, "test_blank.json");
    
    // Get templates
    auto templates = tools.GetTemplates(AssetType::Config);
    assert(!templates.empty() && "Should have templates");
    
    // Get creation history
    auto history = tools.GetCreationHistory("test_user");
    assert(history.size() >= 2 && "Should have creation history");
    
    // Cleanup
    fs::remove("test_blank.json");
    fs::remove("test_clone.json");
    
    std::cout << "✓ Asset Creation Tools passed\n\n";
}

void TestImportPipeline() {
    std::cout << "Testing Asset Import Pipeline...\n";
    
    auto& pipeline = AssetImportPipeline::GetInstance();
    
    // Create source file
    std::ofstream source("external_asset.txt");
    source << "Test data";
    source.close();
    
    // Register validator
    pipeline.RegisterValidator(AssetType::Data, [](const std::string& path) {
        return fs::exists(path);
    });
    
    // Register post-processor
    bool processed = false;
    pipeline.RegisterPostProcessor(AssetType::Data, [&processed](const std::string& path) {
        processed = true;
    });
    
    // Import asset
    ImportTask task;
    task.source_path = "external_asset.txt";
    task.destination_path = "imported_asset.txt";
    task.type = AssetType::Data;
    
    bool imported = pipeline.ImportAsset(task);
    assert(imported && "Should import asset");
    assert(processed && "Should run post-processor");
    
    // Check stats
    auto stats = pipeline.GetImportStats();
    assert(stats.total_imports > 0 && "Should have import stats");
    assert(stats.successful_imports > 0 && "Should have successful imports");
    
    // Get log
    auto log = pipeline.GetImportLog();
    assert(!log.empty() && "Should have import log");
    
    // Cleanup
    fs::remove("external_asset.txt");
    fs::remove("imported_asset.txt");
    
    std::cout << "✓ Asset Import Pipeline passed\n\n";
}

void TestExportPipeline() {
    std::cout << "Testing Asset Export Pipeline...\n";
    
    auto& exporter = AssetExportPipeline::GetInstance();
    
    // Create source file
    std::ofstream source("export_source.txt");
    source << "Export data";
    source.close();
    
    fs::create_directories("exports_test");
    
    // Register export processor
    exporter.RegisterExportProcessor(Platform::Windows, AssetType::Data,
        [](const std::string& src, const std::string& dst) {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            return true;
        });
    
    // Export asset
    ExportTask task;
    task.asset_path = "export_source.txt";
    task.export_path = "exports_test/exported.txt";
    task.target_platform = Platform::Windows;
    
    bool exported = exporter.ExportAsset(task);
    assert(exported && "Should export asset");
    assert(fs::exists("exports_test/exported.txt") && "Export file should exist");
    
    // Check stats
    auto stats = exporter.GetExportStats();
    assert(stats.total_exports > 0 && "Should have export stats");
    
    // Cleanup
    fs::remove("export_source.txt");
    fs::remove_all("exports_test");
    
    std::cout << "✓ Asset Export Pipeline passed\n\n";
}

void TestReviewSystem() {
    std::cout << "Testing Asset Review System...\n";
    
    auto& review = AssetReviewSystem::GetInstance();
    
    const std::string asset_path = "test_review_asset.json";
    
    // Submit for review
    bool submitted = review.SubmitForReview(asset_path, "reviewer1");
    assert(submitted && "Should submit for review");
    
    // Create review
    AssetReview r;
    r.asset_path = asset_path;
    r.reviewer = "reviewer1";
    r.status = ReviewStatus::InProgress;
    r.comments = "Looks good overall";
    r.rating = 4;
    r.issues = {"Minor issue 1", "Minor issue 2"};
    r.suggestions = {"Try this", "Consider that"};
    r.review_time = std::chrono::system_clock::now();
    
    bool created = review.CreateReview(r);
    assert(created && "Should create review");
    
    // Get reviews
    auto reviews = review.GetReviews(asset_path);
    assert(!reviews.empty() && "Should have reviews");
    
    // Get pending reviews
    auto pending = review.GetPendingReviews("reviewer1");
    assert(!pending.empty() && "Should have pending reviews");
    
    // Approve asset
    bool approved = review.ApproveAsset(asset_path, "reviewer2", "Approved!");
    assert(approved && "Should approve asset");
    
    // Request changes
    bool requested = review.RequestChanges(asset_path, "reviewer3",
                                          {"Fix bug", "Add docs"});
    assert(requested && "Should request changes");
    
    // Get stats
    auto stats = review.GetReviewStats();
    assert(stats.total_reviews > 0 && "Should have review stats");
    
    std::cout << "✓ Asset Review System passed\n\n";
}

void TestCollaboration() {
    std::cout << "Testing Asset Collaboration...\n";
    
    auto& collab = AssetCollaborationManager::GetInstance();
    
    const std::string asset_path = "test_collab_asset.json";
    
    // Lock asset
    bool locked = collab.LockAsset(asset_path, "user1");
    assert(locked && "Should lock asset");
    
    // Check if locked
    assert(collab.IsLocked(asset_path) && "Asset should be locked");
    
    // Get lock owner
    std::string owner = collab.GetLockOwner(asset_path);
    assert(owner == "user1" && "Should get lock owner");
    
    // Try to lock again (should fail)
    bool locked_again = collab.LockAsset(asset_path, "user2");
    assert(!locked_again && "Should not lock already locked asset");
    
    // Unlock asset
    bool unlocked = collab.UnlockAsset(asset_path, "user1");
    assert(unlocked && "Should unlock asset");
    assert(!collab.IsLocked(asset_path) && "Asset should be unlocked");
    
    // Set owner
    bool owner_set = collab.SetOwner(asset_path, "owner1");
    assert(owner_set && "Should set owner");
    
    // Add contributors
    collab.AddContributor(asset_path, "contributor1");
    collab.AddContributor(asset_path, "contributor2");
    
    // Get collaboration info
    auto info = collab.GetCollaborationInfo(asset_path);
    assert(info.owner == "owner1" && "Should have correct owner");
    assert(info.contributors.size() == 2 && "Should have contributors");
    
    // Get user assets
    auto user_assets = collab.GetUserAssets("owner1");
    assert(!user_assets.empty() && "Should have user assets");
    
    // Export report
    bool exported = collab.ExportCollaborationReport("test_collab_report.md");
    assert(exported && "Should export collaboration report");
    
    // Cleanup
    fs::remove("test_collab_report.md");
    
    std::cout << "✓ Asset Collaboration passed\n\n";
}

void TestVersionControl() {
    std::cout << "Testing Asset Version Control...\n";
    
    auto& vcs = AssetVersionControl::GetInstance();
    
    // Initialize
    bool initialized = vcs.Initialize(".");
    assert(initialized && "Should initialize VCS");
    
    const std::string asset_path = "test_vcs_asset.json";
    
    // Commit asset
    bool committed1 = vcs.CommitAsset(asset_path, "Initial commit", "user1");
    assert(committed1 && "Should commit asset");
    
    bool committed2 = vcs.CommitAsset(asset_path, "Updated config", "user1");
    assert(committed2 && "Should commit second version");
    
    // Get history
    auto history = vcs.GetHistory(asset_path);
    assert(history.size() >= 2 && "Should have version history");
    
    // Get current version
    int version = vcs.GetCurrentVersion(asset_path);
    assert(version > 0 && "Should have current version");
    
    // Tag version
    bool tagged = vcs.TagVersion(asset_path, version, "v1.0");
    assert(tagged && "Should tag version");
    
    // Export version history
    bool exported = vcs.ExportVersionHistory("test_version_history.md");
    assert(exported && "Should export version history");
    
    // Cleanup
    fs::remove("test_version_history.md");
    
    std::cout << "✓ Asset Version Control passed\n\n";
}

void TestAutomation() {
    std::cout << "Testing Asset Automation...\n";
    
    auto& automation = AssetAutomation::GetInstance();
    
    // Register task
    bool task_ran = false;
    AutomationTask task;
    task.name = "test_task";
    task.trigger = AutomationRule::OnImport;
    task.action = [&task_ran](const std::string& path) {
        task_ran = true;
        return true;
    };
    task.enabled = true;
    
    bool registered = automation.RegisterTask(task);
    assert(registered && "Should register task");
    
    // Run task manually
    bool ran = automation.RunTask("test_task", "test_asset.json");
    assert(ran && "Should run task");
    assert(task_ran && "Task should have executed");
    
    // Reset flag
    task_ran = false;
    
    // Run triggered tasks
    bool triggered = automation.RunTriggeredTasks(AutomationRule::OnImport,
                                                  "test_asset.json");
    assert(triggered && "Should run triggered tasks");
    assert(task_ran && "Task should have executed via trigger");
    
    // Enable/disable task
    bool disabled = automation.EnableTask("test_task", false);
    assert(disabled && "Should disable task");
    
    bool enabled = automation.EnableTask("test_task", true);
    assert(enabled && "Should enable task");
    
    // Schedule task
    auto future = std::chrono::system_clock::now() + std::chrono::seconds(1);
    bool scheduled = automation.ScheduleTask("test_task", future);
    assert(scheduled && "Should schedule task");
    
    // Update (would run scheduled tasks)
    automation.Update();
    
    // Get stats
    auto stats = automation.GetAutomationStats();
    assert(stats.total_tasks > 0 && "Should have automation stats");
    
    // Unregister task
    bool unregistered = automation.UnregisterTask("test_task");
    assert(unregistered && "Should unregister task");
    
    std::cout << "✓ Asset Automation passed\n\n";
}

void TestQualityAssurance() {
    std::cout << "Testing Asset Quality Assurance...\n";
    
    auto& qa = AssetQualityAssurance::GetInstance();
    
    // Create test file
    std::ofstream test_file("test_qa_asset.json");
    test_file << "{\"test\": \"data\"}";
    test_file.close();
    
    // Register quality checks
    QualityCheck size_check;
    size_check.name = "Size Check";
    size_check.description = "Check file size";
    size_check.required = true;
    size_check.check = [](const AssetMetadata& metadata) {
        return metadata.size_bytes < 1024 * 1024; // < 1MB
    };
    
    qa.RegisterCheck(size_check);
    
    QualityCheck naming_check;
    naming_check.name = "Naming Check";
    naming_check.description = "Check naming convention";
    naming_check.required = false;
    naming_check.check = [](const AssetMetadata& metadata) {
        return metadata.name.find(' ') == std::string::npos; // No spaces
    };
    
    qa.RegisterCheck(naming_check);
    
    // Run QA
    auto result = qa.RunQA("test_qa_asset.json");
    assert(result.passed && "QA should pass");
    assert(!result.passed_checks.empty() && "Should have passed checks");
    
    // Set quality level
    bool level_set = qa.SetQualityLevel("test_qa_asset.json", 
                                        QualityLevel::Production);
    assert(level_set && "Should set quality level");
    
    // Get quality level
    auto level = qa.GetQualityLevel("test_qa_asset.json");
    assert(level == QualityLevel::Production && "Should get correct quality level");
    
    // Run QA batch
    std::vector<std::string> assets = {"test_qa_asset.json"};
    auto batch_results = qa.RunQABatch(assets);
    assert(!batch_results.empty() && "Should have batch results");
    
    // Export QA report
    bool exported = qa.ExportQAReport("test_qa_report.md");
    assert(exported && "Should export QA report");
    
    // Get stats
    auto stats = qa.GetQAStats();
    assert(stats.total_checks > 0 && "Should have QA stats");
    
    // Cleanup
    fs::remove("test_qa_asset.json");
    fs::remove("test_qa_report.md");
    
    std::cout << "✓ Asset Quality Assurance passed\n\n";
}

void TestDocumentation() {
    std::cout << "Testing Asset Documentation...\n";
    
    auto& docs = AssetDocumentationGenerator::GetInstance();
    
    // Create test asset
    std::ofstream test_file("test_doc_asset.json");
    test_file << "{\"test\": \"data\"}";
    test_file.close();
    
    // Add custom sections
    docs.AddCustomSection("Usage", "How to use this asset");
    docs.AddCustomSection("Notes", "Important notes");
    
    // Generate asset doc
    bool asset_doc = docs.GenerateAssetDoc("test_doc_asset.json",
                                          "test_asset_doc.md");
    assert(asset_doc && "Should generate asset doc");
    assert(fs::exists("test_asset_doc.md") && "Asset doc should exist");
    
    // Generate workflow doc
    bool workflow_doc = docs.GenerateWorkflowDoc("test_workflow_doc.md");
    assert(workflow_doc && "Should generate workflow doc");
    assert(fs::exists("test_workflow_doc.md") && "Workflow doc should exist");
    
    // Generate team guide
    bool team_guide = docs.GenerateTeamGuide("test_team_guide.md");
    assert(team_guide && "Should generate team guide");
    assert(fs::exists("test_team_guide.md") && "Team guide should exist");
    
    // Generate asset catalog
    bool catalog = docs.GenerateAssetCatalog("test_catalog.md");
    assert(catalog && "Should generate catalog");
    assert(fs::exists("test_catalog.md") && "Catalog should exist");
    
    // Cleanup
    fs::remove("test_doc_asset.json");
    fs::remove("test_asset_doc.md");
    fs::remove("test_workflow_doc.md");
    fs::remove("test_team_guide.md");
    fs::remove("test_catalog.md");
    
    std::cout << "✓ Asset Documentation passed\n\n";
}

void TestTraining() {
    std::cout << "Testing Asset Training System...\n";
    
    auto& training = AssetTrainingSystem::GetInstance();
    
    // Add training material
    TrainingMaterial material;
    material.title = "Creating Assets 101";
    material.description = "Learn the basics of asset creation";
    material.content = "Step 1: Create asset\nStep 2: Test asset\nStep 3: Submit";
    material.tags = {"tutorial", "beginner", "assets"};
    material.related_assets = {"example_asset.json"};
    
    bool added = training.AddTrainingMaterial(material);
    assert(added && "Should add training material");
    
    // Get materials by tag
    auto beginner_materials = training.GetMaterialsByTag("beginner");
    assert(!beginner_materials.empty() && "Should have beginner materials");
    
    // Get training for asset type
    auto type_training = training.GetTrainingForAssetType(AssetType::Config);
    assert(!type_training.empty() && "Should have type training");
    
    // Generate onboarding guide
    bool onboarding = training.GenerateOnboardingGuide("test_onboarding.md");
    assert(onboarding && "Should generate onboarding guide");
    assert(fs::exists("test_onboarding.md") && "Onboarding guide should exist");
    
    // Generate best practices
    bool best_practices = training.GenerateBestPractices("test_best_practices.md");
    assert(best_practices && "Should generate best practices");
    assert(fs::exists("test_best_practices.md") && "Best practices should exist");
    
    // Generate quick reference
    bool quick_ref = training.GenerateQuickReference("test_quick_ref.md");
    assert(quick_ref && "Should generate quick reference");
    assert(fs::exists("test_quick_ref.md") && "Quick reference should exist");
    
    // Export all materials
    fs::create_directories("test_training_export");
    bool exported = training.ExportAllMaterials("test_training_export");
    assert(exported && "Should export all materials");
    
    // Cleanup
    fs::remove("test_onboarding.md");
    fs::remove("test_best_practices.md");
    fs::remove("test_quick_ref.md");
    fs::remove_all("test_training_export");
    
    std::cout << "✓ Asset Training System passed\n\n";
}

void TestWorkflowManager() {
    std::cout << "Testing Asset Workflow Manager...\n";
    
    auto& workflow = AssetWorkflowManager::GetInstance();
    
    // Initialize
    bool initialized = workflow.Initialize(".");
    assert(initialized && "Should initialize workflow");
    
    const std::string asset_path = "test_workflow_asset.json";
    
    // Set asset state
    bool state_set = workflow.SetAssetState(asset_path, WorkflowState::Draft);
    assert(state_set && "Should set asset state");
    
    // Get asset state
    auto state = workflow.GetAssetState(asset_path);
    assert(state == WorkflowState::Draft && "Should get correct state");
    
    // Advance workflow
    bool advanced = workflow.AdvanceWorkflow(asset_path);
    assert(advanced && "Should advance workflow");
    
    state = workflow.GetAssetState(asset_path);
    assert(state == WorkflowState::InProgress && "Should be in progress");
    
    // Advance through all states
    workflow.AdvanceWorkflow(asset_path); // -> PendingReview
    workflow.AdvanceWorkflow(asset_path); // -> InReview
    workflow.AdvanceWorkflow(asset_path); // -> Approved
    workflow.AdvanceWorkflow(asset_path); // -> Published
    
    state = workflow.GetAssetState(asset_path);
    assert(state == WorkflowState::Published && "Should be published");
    
    // Update
    workflow.Update();
    
    // Get workflow stats
    auto stats = workflow.GetWorkflowStats();
    assert(stats.assets_published > 0 && "Should have published assets");
    
    // Export workflow report
    bool exported = workflow.ExportWorkflowReport("test_workflow_report.md");
    assert(exported && "Should export workflow report");
    assert(fs::exists("test_workflow_report.md") && "Report should exist");
    
    // Get system status
    auto status = workflow.GetSystemStatus();
    assert(status.creation_tools_ready && "Creation tools should be ready");
    assert(status.import_pipeline_ready && "Import pipeline should be ready");
    assert(status.export_pipeline_ready && "Export pipeline should be ready");
    assert(status.review_system_ready && "Review system should be ready");
    assert(status.collaboration_ready && "Collaboration should be ready");
    assert(status.version_control_ready && "Version control should be ready");
    assert(status.automation_ready && "Automation should be ready");
    assert(status.qa_ready && "QA should be ready");
    assert(status.documentation_ready && "Documentation should be ready");
    assert(status.training_ready && "Training should be ready");
    
    // Cleanup
    fs::remove("test_workflow_report.md");
    
    // Shutdown
    workflow.Shutdown();
    
    std::cout << "✓ Asset Workflow Manager passed\n\n";
}

void TestIntegration() {
    std::cout << "Testing Complete Workflow Integration...\n";
    
    const std::string asset_path = "integration_test_asset.json";
    
    // 1. Initialize workflow
    auto& workflow = AssetWorkflowManager::GetInstance();
    workflow.Initialize(".");
    
    // 2. Create asset
    auto& tools = AssetCreationTools::GetInstance();
    AssetCreationInfo info;
    info.creator = "integration_user";
    info.description = "Integration test asset";
    info.source = AssetSource::Internal;
    info.creation_time = std::chrono::system_clock::now();
    
    tools.CreateBlankAsset(asset_path, AssetType::Config, info);
    
    // 3. Lock asset
    auto& collab = AssetCollaborationManager::GetInstance();
    collab.LockAsset(asset_path, "integration_user");
    
    // 4. Set state
    workflow.SetAssetState(asset_path, WorkflowState::InProgress);
    
    // 5. Run QA
    auto& qa = AssetQualityAssurance::GetInstance();
    auto qa_result = qa.RunQA(asset_path);
    assert(qa_result.passed && "QA should pass");
    
    // 6. Commit version
    auto& vcs = AssetVersionControl::GetInstance();
    vcs.CommitAsset(asset_path, "Initial version", "integration_user");
    
    // 7. Submit for review
    collab.UnlockAsset(asset_path, "integration_user");
    auto& review = AssetReviewSystem::GetInstance();
    review.SubmitForReview(asset_path, "reviewer");
    workflow.SetAssetState(asset_path, WorkflowState::PendingReview);
    
    // 8. Approve
    review.ApproveAsset(asset_path, "reviewer", "Looks good!");
    workflow.SetAssetState(asset_path, WorkflowState::Approved);
    
    // 9. Export
    auto& exporter = AssetExportPipeline::GetInstance();
    fs::create_directories("integration_exports");
    ExportTask export_task;
    export_task.asset_path = asset_path;
    export_task.export_path = "integration_exports/" + 
                              fs::path(asset_path).filename().string();
    export_task.target_platform = Platform::All;
    exporter.ExportAsset(export_task);
    
    // 10. Publish
    workflow.SetAssetState(asset_path, WorkflowState::Published);
    
    // 11. Generate documentation
    auto& docs = AssetDocumentationGenerator::GetInstance();
    docs.GenerateAssetDoc(asset_path, "integration_asset_doc.md");
    
    // Verify final state
    auto final_state = workflow.GetAssetState(asset_path);
    assert(final_state == WorkflowState::Published && "Should be published");
    
    // Cleanup
    fs::remove(asset_path);
    fs::remove_all("integration_exports");
    fs::remove("integration_asset_doc.md");
    
    workflow.Shutdown();
    
    std::cout << "✓ Complete Workflow Integration passed\n\n";
}

int main() {
    std::cout << "=== Asset Workflow System Test Suite ===\n\n";
    
    try {
        TestCreationTools();
        TestImportPipeline();
        TestExportPipeline();
        TestReviewSystem();
        TestCollaboration();
        TestVersionControl();
        TestAutomation();
        TestQualityAssurance();
        TestDocumentation();
        TestTraining();
        TestWorkflowManager();
        TestIntegration();
        
        std::cout << "=== All Tests Passed! ===\n";
        std::cout << "\n✅ Asset Workflow System is fully operational\n";
        std::cout << "✅ All 10 subsystems tested successfully\n";
        std::cout << "✅ Integration test completed\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
