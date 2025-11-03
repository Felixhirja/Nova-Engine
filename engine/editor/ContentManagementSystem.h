#pragma once

#include "ContentEditor.h"
#include "ContentValidator.h"
#include "ContentTemplateSystem.h"
#include "ContentAnalytics.h"
#include "ContentLocalization.h"
#include "ContentVersioning.h"
#include "ContentPublisher.h"
#include "ContentTestingFramework.h"
#include "ContentDocumentation.h"
#include "ContentIntegration.h"
#include <memory>
#include <string>

namespace ContentManagement {

/**
 * ContentManagementSystem: Unified content management system
 * 
 * This class provides a unified interface to all content management subsystems.
 * It coordinates between different components and provides a single entry point
 * for all content management operations.
 */
class ContentManagementSystem {
public:
    static ContentManagementSystem& GetInstance();
    
    // Lifecycle
    bool Initialize(const std::string& contentDirectory);
    void Update(float deltaTime);
    void Shutdown();
    
    // Component Access
    ContentEditor& GetEditor() { return *editor_; }
    ContentValidator& GetValidator() { return *validator_; }
    ContentTemplateSystem& GetTemplateSystem() { return *templateSystem_; }
    ContentAnalytics& GetAnalytics() { return *analytics_; }
    ContentLocalization& GetLocalization() { return *localization_; }
    ContentVersioning& GetVersioning() { return *versioning_; }
    ContentPublisher& GetPublisher() { return *publisher_; }
    ContentTestingFramework& GetTestingFramework() { return *testingFramework_; }
    ContentDocumentation& GetDocumentation() { return *documentation_; }
    ContentIntegration& GetIntegration() { return *integration_; }
    
    // High-Level Operations
    
    // Create new content from template
    std::string CreateContent(const std::string& templateId, const std::unordered_map<std::string, std::string>& variables);
    
    // Edit content with validation
    bool EditContent(const std::string& contentId, const std::string& fieldPath, const simplejson::JsonValue& value);
    
    // Save content with versioning
    bool SaveContent(const std::string& contentId, const std::string& commitMessage, const std::string& author);
    
    // Publish content bundle
    std::string PublishContent(const std::vector<std::string>& contentIds, const std::vector<std::string>& targetIds);
    
    // Validate and test content
    bool ValidateAndTest(const std::string& contentId, std::vector<std::string>& errors);
    
    // Complete workflow: Create -> Edit -> Validate -> Test -> Version -> Publish
    std::string CompleteContentWorkflow(
        const std::string& templateId,
        const std::unordered_map<std::string, std::string>& variables,
        const std::string& author,
        bool autoPublish = false
    );
    
    // UI
    void RenderContentManagementUI();
    
    // Configuration
    void SetContentDirectory(const std::string& directory);
    std::string GetContentDirectory() const { return contentDirectory_; }
    
    // Statistics
    struct SystemStats {
        int totalContent;
        int validatedContent;
        int publishedContent;
        int testsPassed;
        int testsTotal;
        float validationSuccessRate;
        float testSuccessRate;
    };
    
    SystemStats GetSystemStats() const;
    
private:
    ContentManagementSystem();
    ~ContentManagementSystem();
    
    // Prevent copying
    ContentManagementSystem(const ContentManagementSystem&) = delete;
    ContentManagementSystem& operator=(const ContentManagementSystem&) = delete;
    
    void InitializeComponents();
    void SetupEventHandlers();
    
    std::unique_ptr<ContentEditor> editor_;
    std::unique_ptr<ContentValidator> validator_;
    std::unique_ptr<ContentTemplateSystem> templateSystem_;
    std::unique_ptr<ContentAnalytics> analytics_;
    std::unique_ptr<ContentLocalization> localization_;
    std::unique_ptr<ContentVersioning> versioning_;
    std::unique_ptr<ContentPublisher> publisher_;
    std::unique_ptr<ContentTestingFramework> testingFramework_;
    std::unique_ptr<ContentDocumentation> documentation_;
    std::unique_ptr<ContentIntegration> integration_;
    
    std::string contentDirectory_;
    bool initialized_;
};

} // namespace ContentManagement

// Convenience macros
#define CONTENT_SYSTEM ContentManagement::ContentManagementSystem::GetInstance()
#define CONTENT_EDITOR CONTENT_SYSTEM.GetEditor()
#define CONTENT_VALIDATOR CONTENT_SYSTEM.GetValidator()
#define CONTENT_TEMPLATES CONTENT_SYSTEM.GetTemplateSystem()
#define CONTENT_ANALYTICS CONTENT_SYSTEM.GetAnalytics()
#define CONTENT_LOCALIZATION CONTENT_SYSTEM.GetLocalization()
#define CONTENT_VERSIONING CONTENT_SYSTEM.GetVersioning()
#define CONTENT_PUBLISHER CONTENT_SYSTEM.GetPublisher()
#define CONTENT_TESTING CONTENT_SYSTEM.GetTestingFramework()
#define CONTENT_DOCS CONTENT_SYSTEM.GetDocumentation()
#define CONTENT_INTEGRATION CONTENT_SYSTEM.GetIntegration()
