#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ContentManagement {

/**
 * ContentDocumentation: Comprehensive content creation guides
 * 
 * Features:
 * - Auto-generated documentation from schemas
 * - Interactive tutorials and wizards
 * - Example content library
 * - Best practices and guidelines
 * - Searchable documentation
 * - Version-specific documentation
 */
class ContentDocumentation {
public:
    enum class DocFormat {
        Markdown,
        HTML,
        PDF,
        Interactive
    };

    struct DocumentationSection {
        std::string id;
        std::string title;
        std::string content;
        std::vector<std::string> subsections;
        std::vector<std::string> tags;
        int order;
        std::string category;
    };

    struct TutorialStep {
        std::string title;
        std::string description;
        std::string action;  // What the user should do
        std::string expectedResult;
        std::vector<std::string> tips;
        std::string contentExample;  // JSON example
        bool interactive;  // Can be executed in the editor
    };

    struct Tutorial {
        std::string id;
        std::string title;
        std::string description;
        std::vector<TutorialStep> steps;
        std::string difficulty;  // "beginner", "intermediate", "advanced"
        int estimatedMinutes;
        std::vector<std::string> prerequisites;
        std::vector<std::string> relatedTutorials;
    };

    struct Example {
        std::string id;
        std::string name;
        std::string description;
        std::string contentType;
        std::unique_ptr<simplejson::JsonObject> exampleContent;
        std::vector<std::string> tags;
        std::string difficulty;
        std::string explanation;  // Why this example is useful
    };

    struct BestPractice {
        std::string id;
        std::string title;
        std::string description;
        std::string category;
        std::vector<std::string> dos;
        std::vector<std::string> donts;
        std::vector<std::string> examples;
        std::string reasoning;
    };

    ContentDocumentation();
    ~ContentDocumentation();

    // Documentation Generation
    bool GenerateDocumentation(const std::string& outputPath, DocFormat format = DocFormat::Markdown);
    std::string GenerateSchemaDocumentation(const std::string& contentType);
    std::string GenerateFieldDocumentation(const std::string& contentType, const std::string& fieldName);
    std::string GenerateAPIDocumentation();
    
    // Schema Documentation
    void DocumentSchema(const std::string& schemaId, const std::string& description);
    void DocumentField(const std::string& schemaId, const std::string& fieldName, const std::string& description, const std::vector<std::string>& examples);
    std::string GetFieldDocumentation(const std::string& schemaId, const std::string& fieldName) const;
    
    // Section Management
    void AddSection(const DocumentationSection& section);
    void UpdateSection(const std::string& sectionId, const DocumentationSection& section);
    void RemoveSection(const std::string& sectionId);
    const DocumentationSection* GetSection(const std::string& sectionId) const;
    std::vector<DocumentationSection> GetSectionsByCategory(const std::string& category) const;
    
    // Tutorial System
    void RegisterTutorial(const Tutorial& tutorial);
    void UpdateTutorial(const std::string& tutorialId, const Tutorial& tutorial);
    const Tutorial* GetTutorial(const std::string& tutorialId) const;
    std::vector<Tutorial> GetTutorialsByDifficulty(const std::string& difficulty) const;
    std::vector<Tutorial> GetRecommendedTutorials(const std::string& userLevel) const;
    
    // Interactive Tutorials
    bool StartTutorial(const std::string& tutorialId);
    bool NextTutorialStep();
    bool PreviousTutorialStep();
    bool CompleteTutorialStep(int stepIndex);
    int GetCurrentTutorialStep() const;
    bool IsTutorialComplete() const;
    
    // Examples Library
    void AddExample(const Example& example);
    void RemoveExample(const std::string& exampleId);
    const Example* GetExample(const std::string& exampleId) const;
    std::vector<Example> GetExamplesByContentType(const std::string& contentType) const;
    std::vector<Example> GetExamplesByTag(const std::string& tag) const;
    std::vector<Example> GetExamplesByDifficulty(const std::string& difficulty) const;
    
    std::unique_ptr<simplejson::JsonObject> InstantiateExample(const std::string& exampleId);
    
    // Best Practices
    void RegisterBestPractice(const BestPractice& practice);
    std::vector<BestPractice> GetBestPracticesByCategory(const std::string& category) const;
    std::vector<BestPractice> GetRelevantBestPractices(const std::string& contentType) const;
    
    // Search
    std::vector<DocumentationSection> SearchDocumentation(const std::string& query) const;
    std::vector<Tutorial> SearchTutorials(const std::string& query) const;
    std::vector<Example> SearchExamples(const std::string& query) const;
    
    // Context-Sensitive Help
    std::string GetContextHelp(const std::string& contentType, const std::string& fieldName) const;
    std::vector<std::string> GetHelpTopics(const std::string& context) const;
    
    // Quick Reference
    std::string GenerateQuickReference(const std::string& contentType) const;
    std::string GenerateCheatSheet() const;
    
    // Validation Documentation
    std::vector<std::string> GetValidationRules(const std::string& contentType) const;
    std::string ExplainValidationError(const std::string& errorCode) const;
    
    // Change Log
    void AddChangeLog(const std::string& version, const std::vector<std::string>& changes);
    std::string GetChangeLog(const std::string& version = "latest") const;
    std::vector<std::string> GetAllVersions() const;
    
    // Export
    bool ExportDocumentation(const std::string& outputPath, DocFormat format = DocFormat::HTML);
    bool ExportTutorials(const std::string& outputPath);
    bool ExportExamples(const std::string& outputPath);
    
    // UI Integration
    void RenderDocumentationBrowser();
    void RenderTutorialViewer(const std::string& tutorialId);
    void RenderExampleBrowser();
    void RenderContextHelp();
    void RenderQuickReference();
    
private:
    std::string RenderMarkdown(const DocumentationSection& section) const;
    std::string RenderHTML(const DocumentationSection& section) const;
    std::string FormatExample(const simplejson::JsonObject& example) const;
    
    std::unordered_map<std::string, DocumentationSection> sections_;
    std::unordered_map<std::string, Tutorial> tutorials_;
    std::unordered_map<std::string, Example> examples_;
    std::unordered_map<std::string, BestPractice> bestPractices_;
    std::unordered_map<std::string, std::string> schemaDocumentation_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> fieldDocumentation_;
    std::unordered_map<std::string, std::vector<std::string>> changeLogs_;
    
    std::string currentTutorialId_;
    int currentTutorialStep_;
};

} // namespace ContentManagement
