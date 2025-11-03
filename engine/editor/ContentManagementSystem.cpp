#include "ContentManagementSystem.h"
#include <iostream>
#include <algorithm>

namespace ContentManagement {

ContentManagementSystem& ContentManagementSystem::GetInstance() {
    static ContentManagementSystem instance;
    return instance;
}

ContentManagementSystem::ContentManagementSystem() 
    : contentDirectory_("assets/content")
    , initialized_(false) {
}

ContentManagementSystem::~ContentManagementSystem() {
    Shutdown();
}

bool ContentManagementSystem::Initialize(const std::string& contentDirectory) {
    if (initialized_) {
        return true;
    }
    
    contentDirectory_ = contentDirectory;
    
    // Create component instances
    editor_ = std::make_unique<ContentEditor>();
    validator_ = std::make_unique<ContentValidator>();
    templateSystem_ = std::make_unique<ContentTemplateSystem>();
    analytics_ = std::make_unique<ContentAnalytics>();
    localization_ = std::make_unique<ContentLocalization>();
    versioning_ = std::make_unique<ContentVersioning>();
    publisher_ = std::make_unique<ContentPublisher>();
    testingFramework_ = std::make_unique<ContentTestingFramework>();
    documentation_ = std::make_unique<ContentDocumentation>();
    integration_ = std::make_unique<ContentIntegration>();
    
    InitializeComponents();
    SetupEventHandlers();
    
    initialized_ = true;
    
    std::cout << "Content Management System initialized at: " << contentDirectory_ << std::endl;
    return true;
}

void ContentManagementSystem::Update(float deltaTime) {
    if (!initialized_) return;
    
    editor_->Update(deltaTime);
    // analytics_->Update(deltaTime);  // Stub - no Update method
    // publisher_->Update(deltaTime);   // Stub - no Update method
    // integration_->Update(deltaTime); // Stub - no Update method
}

void ContentManagementSystem::Shutdown() {
    if (!initialized_) return;
    
    // Save any pending changes
    editor_->SaveAll();
    
    // Cleanup components in reverse order
    integration_.reset();
    documentation_.reset();
    testingFramework_.reset();
    publisher_.reset();
    versioning_.reset();
    localization_.reset();
    analytics_.reset();
    templateSystem_.reset();
    validator_.reset();
    editor_.reset();
    
    initialized_ = false;
}

void ContentManagementSystem::InitializeComponents() {
    // Initialize editor
    editor_->Initialize();
    editor_->LoadContentDirectory(contentDirectory_ + "/templates", ContentEditor::ContentType::Custom);
    
    // Load templates
    templateSystem_->LoadTemplates(contentDirectory_ + "/templates");
    
    // Load schemas for validation
    validator_->LoadSchemasFromDirectory(contentDirectory_ + "/schemas");
    
    // Initialize localization
    localization_->Initialize(contentDirectory_ + "/localization");
    localization_->SetCurrentLocale("en-US");
    
    // Initialize versioning (no specific Initialize method - done in constructor)
    // versioning_ is ready to use
    
    // Load test cases
    testingFramework_->LoadTestsFromDirectory(contentDirectory_ + "/tests");
    
    // Load documentation (GenerateFromSchemas not yet implemented)
    // documentation_->GenerateFromSchemas(validator_->GetAllSchemas());
}

void ContentManagementSystem::SetupEventHandlers() {
    // Content saved -> create version
    editor_->SetSaveCallback([this](const std::string& contentId) {
        auto content = editor_->GetContent(contentId);
        if (content && content->data) {
            versioning_->CommitVersion(
                contentId,
                *content->data,
                "Auto-save from editor",
                "system"
            );
        }
    });
    
    // Content used -> track analytics
    editor_->SetLoadCallback([this](const std::string& contentId) {
        analytics_->TrackContentUsage(contentId, "editor", "system");
    });
    
    // Publishing complete -> notify integration
    // publisher_->SetPublishCallback([this](const std::string& jobId) {
    //     simplejson::JsonObject payload;
    //     payload["job_id"] = simplejson::JsonValue(jobId);
    //     integration_->TriggerWebhook("content.published", payload);
    // });
}

std::string ContentManagementSystem::CreateContent(
    const std::string& templateId, 
    const std::unordered_map<std::string, std::string>& variables) {
    
    // Instantiate template
    auto content = templateSystem_->InstantiateTemplate(templateId, variables);
    if (!content) {
        std::cerr << "Failed to instantiate template: " << templateId << std::endl;
        return "";
    }
    
    // Generate unique ID
    std::string contentId = templateId + "_" + std::to_string(std::time(nullptr));
    
    // Create in editor
    std::string editorId = editor_->CreateContent(ContentEditor::ContentType::Custom, templateId);
    if (editorId.empty()) {
        return "";
    }
    
    // Set content data
    editor_->SelectContent(editorId);
    auto item = editor_->GetSelectedContent();
    if (item) {
        item->data = std::move(content);
        item->id = contentId;
    }
    
    return contentId;
}

bool ContentManagementSystem::EditContent(
    const std::string& contentId, 
    const std::string& fieldPath, 
    const simplejson::JsonValue& value) {
    
    editor_->SelectContent(contentId);
    
    if (!editor_->SetField(fieldPath, value)) {
        return false;
    }
    
    // Validate after edit
    auto content = editor_->GetSelectedContent();
    if (content && content->data) {
        std::vector<ContentValidator::ValidationResult> results;
        validator_->ValidateContent(*content->data, "custom", results);
        
        content->isValid = std::all_of(results.begin(), results.end(),
            [](const auto& r) { return r.severity != ContentValidator::ValidationSeverity::Error; });
    }
    
    return true;
}

bool ContentManagementSystem::SaveContent(
    const std::string& contentId, 
    const std::string& commitMessage, 
    const std::string& author) {
    
    // Save in editor
    if (!editor_->SaveContent(contentId)) {
        return false;
    }
    
    // Create version
    auto content = editor_->GetContent(contentId);
    if (content && content->data) {
        versioning_->CommitVersion(contentId, *content->data, commitMessage, author);
    }
    
    return true;
}

std::string ContentManagementSystem::PublishContent(
    const std::vector<std::string>& contentIds, 
    const std::vector<std::string>& targetIds) {
    
    // Validate all content first
    for (const auto& contentId : contentIds) {
        std::vector<std::string> errors;
        if (!ValidateAndTest(contentId, errors)) {
            std::cerr << "Content validation failed for: " << contentId << std::endl;
            for (const auto& error : errors) {
                std::cerr << "  - " << error << std::endl;
            }
            return "";
        }
    }
    
    // Create bundle
    std::string bundleId = publisher_->CreateBundle(
        "Auto Bundle " + std::to_string(std::time(nullptr)),
        contentIds,
        "system"
    );
    
    // Get publish targets
    std::vector<ContentPublisher::PublishTarget> targets;
    for (const auto& targetId : targetIds) {
        auto target = publisher_->GetPublishTarget(targetId);
        if (target) {
            targets.push_back(*target);
        }
    }
    
    // Publish
    std::string jobId = publisher_->PublishBundle(bundleId, targets);
    
    return jobId;
}

bool ContentManagementSystem::ValidateAndTest(
    const std::string& contentId, 
    std::vector<std::string>& errors) {
    
    auto content = editor_->GetContent(contentId);
    if (!content || !content->data) {
        errors.push_back("Content not found: " + contentId);
        return false;
    }
    
    // Validate
    std::vector<ContentValidator::ValidationResult> validationResults;
    validator_->ValidateContent(*content->data, "custom", validationResults);
    
    for (const auto& result : validationResults) {
        if (result.severity == ContentValidator::ValidationSeverity::Error) {
            errors.push_back("[Validation] " + result.message);
        }
    }
    
    // Test
    auto testReport = testingFramework_->RunTestsForContent(contentId);
    
    if (testReport.failedTests > 0) {
        for (const auto& testResult : testReport.tests) {
            if (!testResult.passed) {
                errors.push_back("[Test] " + testResult.name + ": " + testResult.errorMessage);
            }
        }
    }
    
    return errors.empty();
}

std::string ContentManagementSystem::CompleteContentWorkflow(
    const std::string& templateId,
    const std::unordered_map<std::string, std::string>& variables,
    const std::string& author,
    bool autoPublish) {
    
    // 1. Create from template
    std::string contentId = CreateContent(templateId, variables);
    if (contentId.empty()) {
        std::cerr << "Failed to create content from template" << std::endl;
        return "";
    }
    
    // 2. Validate and test
    std::vector<std::string> errors;
    if (!ValidateAndTest(contentId, errors)) {
        std::cerr << "Content validation failed:" << std::endl;
        for (const auto& error : errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return "";
    }
    
    // 3. Save with version
    if (!SaveContent(contentId, "Initial content creation from " + templateId, author)) {
        std::cerr << "Failed to save content" << std::endl;
        return "";
    }
    
    // 4. Optionally publish
    if (autoPublish) {
        std::string jobId = PublishContent({contentId}, {"production"});
        if (jobId.empty()) {
            std::cerr << "Failed to publish content" << std::endl;
        }
    }
    
    return contentId;
}

void ContentManagementSystem::RenderContentManagementUI() {
    if (!initialized_) return;
    
    // Render main editor UI
    editor_->Render();
    
    // Additional UI panels would go here
    // This is where ImGui rendering would happen
}

void ContentManagementSystem::SetContentDirectory(const std::string& directory) {
    contentDirectory_ = directory;
    if (initialized_) {
        // Reload components with new directory
        InitializeComponents();
    }
}

ContentManagementSystem::SystemStats ContentManagementSystem::GetSystemStats() const {
    SystemStats stats{};
    
    if (!initialized_) return stats;
    
    // Gather stats from components
    stats.totalContent = editor_->GetContentCount();
    
    auto validationStats = validator_->GetValidationStats();
    stats.validatedContent = validationStats.totalValidated;
    stats.validationSuccessRate = validationStats.successRate;
    
    auto testStats = testingFramework_->GetTestStats();
    stats.testsPassed = testStats.passedTests;
    stats.testsTotal = testStats.totalTests;
    stats.testSuccessRate = testStats.totalTests > 0 
        ? static_cast<float>(testStats.passedTests) / testStats.totalTests 
        : 0.0f;
    
    // Publisher stats - simplified for now
    stats.publishedContent = 0;
    
    return stats;
}

} // namespace ContentManagement
