#pragma once

#include "../SimpleJson.h"
#include "ConfigManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NovaEngine {
namespace Config {

// =====================================================
// CONFIGURATION EDITOR - Visual Editor for Actor Configurations
// =====================================================

/**
 * EditorFieldType - Types of fields supported in the visual editor
 */
enum class EditorFieldType {
    Text,
    Number,
    Boolean,
    Color,
    Vector2,
    Vector3,
    Dropdown,
    FileSelect,
    MultiSelect,
    Slider,
    TextArea,
    JsonObject,
    JsonArray
};

/**
 * EditorField - Metadata for an editable configuration field
 */
struct EditorField {
    std::string id;
    std::string label;
    std::string description;
    EditorFieldType type;
    simplejson::JsonValue currentValue;
    simplejson::JsonValue defaultValue;
    
    // Validation
    bool required = false;
    double minValue = 0.0;
    double maxValue = 0.0;
    std::vector<std::string> allowedValues;
    
    // UI hints
    std::string category;
    int displayOrder = 0;
    bool readOnly = false;
    std::string tooltip;
    
    // Dependencies
    std::string dependsOn;
    std::function<bool(const simplejson::JsonValue&)> visibleWhen;
};

/**
 * EditorSection - Groups related fields in the UI
 */
struct EditorSection {
    std::string name;
    std::string description;
    bool collapsed = false;
    std::vector<EditorField> fields;
};

/**
 * EditorLayout - Complete layout for a configuration editor
 */
struct EditorLayout {
    std::string configType;
    std::string title;
    std::string description;
    std::vector<EditorSection> sections;
    
    void AddSection(const EditorSection& section);
    void AddField(const std::string& sectionName, const EditorField& field);
    EditorField* GetField(const std::string& fieldId);
};

/**
 * ConfigEditor - Visual editor for actor configurations
 */
class ConfigEditor {
public:
    ConfigEditor();
    
    // Editor lifecycle
    bool OpenConfig(const std::string& configPath);
    bool SaveConfig(const std::string& configPath = "");
    bool SaveAsConfig(const std::string& newPath);
    void CloseConfig();
    
    // Field operations
    bool SetFieldValue(const std::string& fieldId, const simplejson::JsonValue& value);
    simplejson::JsonValue GetFieldValue(const std::string& fieldId) const;
    
    // Real-time validation
    ValidationResult ValidateField(const std::string& fieldId);
    ValidationResult ValidateAll();
    
    // Layout management
    EditorLayout GenerateLayout(const std::string& configType);
    void SetCustomLayout(const EditorLayout& layout);
    const EditorLayout& GetLayout() const { return layout_; }
    
    // Undo/Redo
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
    // Change tracking
    bool HasUnsavedChanges() const { return isDirty_; }
    std::vector<std::string> GetModifiedFields() const;
    
    // Preview
    simplejson::JsonValue GetPreviewConfig() const;
    
    // Auto-save
    void EnableAutoSave(int intervalSeconds);
    void DisableAutoSave();
    
    // Callbacks
    using ValidationCallback = std::function<void(const ValidationResult&)>;
    using ChangeCallback = std::function<void(const std::string& fieldId, const simplejson::JsonValue& value)>;
    
    void SetValidationCallback(ValidationCallback callback);
    void SetChangeCallback(ChangeCallback callback);
    
private:
    struct EditorState {
        simplejson::JsonValue config;
        std::string configPath;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::string currentPath_;
    simplejson::JsonValue currentConfig_;
    EditorLayout layout_;
    
    bool isDirty_ = false;
    std::vector<EditorState> undoStack_;
    std::vector<EditorState> redoStack_;
    
    ValidationCallback validationCallback_;
    ChangeCallback changeCallback_;
    
    bool autoSaveEnabled_ = false;
    int autoSaveInterval_ = 60;
    std::chrono::steady_clock::time_point lastAutoSave_;
    
    void PushUndoState();
    void MarkDirty();
    void CheckAutoSave();
    EditorSection GenerateSection(const std::string& name, const simplejson::JsonObject& schema);
};

// =====================================================
// CONFIGURATION TESTING - Automated Testing for Configuration Changes
// =====================================================

/**
 * ConfigTest - Single test case for configuration validation
 */
struct ConfigTest {
    std::string name;
    std::string description;
    std::function<bool(const simplejson::JsonValue&)> testFunc;
    std::string expectedResult;
    int priority = 0;
};

/**
 * ConfigTestSuite - Collection of tests for a configuration type
 */
class ConfigTestSuite {
public:
    ConfigTestSuite(const std::string& name);
    
    void AddTest(const ConfigTest& test);
    void AddTest(const std::string& name, 
                 const std::string& description,
                 std::function<bool(const simplejson::JsonValue&)> testFunc);
    
    struct TestResult {
        std::string testName;
        bool passed = false;
        std::string message;
        double executionTimeMs = 0.0;
    };
    
    struct TestReport {
        std::string suiteName;
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        double totalTimeMs = 0.0;
        std::vector<TestResult> results;
        
        bool AllPassed() const { return failedTests == 0; }
        double GetPassRate() const { 
            return totalTests > 0 ? (double)passedTests / totalTests * 100.0 : 0.0; 
        }
    };
    
    TestReport RunTests(const simplejson::JsonValue& config);
    TestReport RunTestsOnFile(const std::string& configPath);
    
    const std::string& GetName() const { return name_; }
    const std::vector<ConfigTest>& GetTests() const { return tests_; }
    
private:
    std::string name_;
    std::vector<ConfigTest> tests_;
};

/**
 * ConfigTestRunner - Manages and executes configuration tests
 */
class ConfigTestRunner {
public:
    static ConfigTestRunner& GetInstance();
    
    void RegisterSuite(const std::string& configType, const ConfigTestSuite& suite);
    
    ConfigTestSuite::TestReport RunTests(const std::string& configType, 
                                         const std::string& configPath);
    
    struct BatchTestReport {
        int totalSuites = 0;
        int passedSuites = 0;
        std::vector<ConfigTestSuite::TestReport> suiteReports;
        double totalTimeMs = 0.0;
        
        bool AllPassed() const;
    };
    
    BatchTestReport RunAllTests(const std::string& configPath);
    BatchTestReport RunTestsOnDirectory(const std::string& directory);
    
    void ExportReport(const BatchTestReport& report, const std::string& outputPath);
    
private:
    ConfigTestRunner() = default;
    std::unordered_map<std::string, ConfigTestSuite> testSuites_;
};

// =====================================================
// CONFIGURATION DOCUMENTATION - Documentation Generation
// =====================================================

/**
 * ConfigDocumentation - Generates documentation for configurations
 */
class ConfigDocumentation {
public:
    enum class DocFormat {
        Markdown,
        HTML,
        JSON,
        PlainText
    };
    
    struct DocOptions {
        bool includeExamples = true;
        bool includeSchema = true;
        bool includeDefaults = true;
        bool includeValidation = true;
        DocFormat format = DocFormat::Markdown;
    };
    
    static std::string GenerateDocumentation(const std::string& configType);
    static std::string GenerateDocumentation(const std::string& configType,
                                            const DocOptions& options);
    
    static std::string GenerateFieldDoc(const EditorField& field);
    static std::string GenerateFieldDoc(const EditorField& field,
                                       const DocOptions& options);
    
    static std::string GenerateSchemaDoc(const std::string& schemaPath);
    static std::string GenerateSchemaDoc(const std::string& schemaPath,
                                        const DocOptions& options);
    
    static bool ExportDocumentation(const std::string& configType,
                                   const std::string& outputPath);
    static bool ExportDocumentation(const std::string& configType,
                                   const std::string& outputPath,
                                   const DocOptions& options);
    
    static bool GenerateFullDocumentation(const std::string& outputDirectory);
    static bool GenerateFullDocumentation(const std::string& outputDirectory,
                                         const DocOptions& options);
    
private:
    static std::string FormatMarkdown(const std::string& content);
    static std::string FormatHTML(const std::string& content);
    static std::string EscapeMarkdown(const std::string& text);
};

// =====================================================
// CONFIGURATION DEPLOYMENT - Deployment Pipeline
// =====================================================

/**
 * DeploymentTarget - Target environment for configuration deployment
 */
enum class DeploymentTarget {
    Development,
    Testing,
    Staging,
    Production,
    Custom
};

/**
 * DeploymentOptions - Options for configuration deployment
 */
struct DeploymentOptions {
    DeploymentTarget target;
    bool validateBeforeDeploy = true;
    bool backupExisting = true;
    bool runTests = true;
    bool dryRun = false;
    std::string customTargetPath;
};

/**
 * DeploymentResult - Result of a deployment operation
 */
struct DeploymentResult {
    bool success = false;
    std::string message;
    std::vector<std::string> deployedFiles;
    std::vector<std::string> backupFiles;
    std::vector<std::string> errors;
    std::chrono::system_clock::time_point deploymentTime;
    double deploymentDurationMs = 0.0;
};

/**
 * ConfigDeployment - Handles configuration deployment pipeline
 */
class ConfigDeployment {
public:
    static ConfigDeployment& GetInstance();
    
    DeploymentResult Deploy(const std::string& configPath,
                           const DeploymentOptions& options);
    
    DeploymentResult DeployBatch(const std::vector<std::string>& configPaths,
                                const DeploymentOptions& options);
    
    DeploymentResult Rollback(const std::string& backupId);
    
    std::vector<std::string> ListBackups() const;
    
    bool ValidateDeployment(const std::string& configPath,
                           const DeploymentOptions& options);
    
    void SetDeploymentHook(std::function<bool(const std::string&)> preDeployHook,
                          std::function<void(const DeploymentResult&)> postDeployHook);
    
private:
    ConfigDeployment() = default;
    
    std::function<bool(const std::string&)> preDeployHook_;
    std::function<void(const DeploymentResult&)> postDeployHook_;
    
    std::string CreateBackup(const std::string& configPath);
    bool ValidateBeforeDeploy(const std::string& configPath);
};

// =====================================================
// CONFIGURATION TEMPLATES - Template System
// =====================================================

/**
 * ConfigTemplateManager - Advanced template system for configurations
 */
class ConfigTemplateManager {
public:
    static ConfigTemplateManager& GetInstance();
    
    struct TemplateInfo {
        std::string name;
        std::string category;
        std::string description;
        std::vector<std::string> tags;
        std::string author;
        std::string version;
        std::vector<ConfigTemplate::TemplateParameter> parameters;
    };
    
    bool RegisterTemplate(const std::string& name,
                         const std::string& templatePath,
                         const TemplateInfo& info);
    
    simplejson::JsonValue InstantiateTemplate(
        const std::string& templateName,
        const std::unordered_map<std::string, simplejson::JsonValue>& parameters
    );
    
    std::vector<TemplateInfo> SearchTemplates(const std::string& query) const;
    std::vector<TemplateInfo> GetTemplatesByCategory(const std::string& category) const;
    std::vector<TemplateInfo> GetTemplatesByTag(const std::string& tag) const;
    
    TemplateInfo GetTemplateInfo(const std::string& templateName) const;
    
    bool ValidateTemplate(const std::string& templateName,
                         std::vector<std::string>& errors) const;
    
private:
    ConfigTemplateManager() = default;
    
    std::unordered_map<std::string, std::pair<std::string, TemplateInfo>> templates_;
};

// =====================================================
// REAL-TIME VALIDATION - Live Validation During Editing
// =====================================================

/**
 * RealTimeValidator - Provides real-time validation as configs are edited
 */
class RealTimeValidator {
public:
    RealTimeValidator();
    
    void StartValidation(const std::string& configPath);
    void StopValidation();
    
    ValidationResult ValidateIncremental(const std::string& fieldPath,
                                        const simplejson::JsonValue& value);
    
    using ValidationListener = std::function<void(const ValidationResult&)>;
    void AddListener(ValidationListener listener);
    void RemoveAllListeners();
    
    void SetValidationDelay(int milliseconds);
    
    struct ValidationCache {
        std::unordered_map<std::string, ValidationResult> cachedResults;
        std::chrono::steady_clock::time_point lastValidation;
    };
    
    const ValidationCache& GetCache() const { return cache_; }
    void ClearCache();
    
private:
    std::string currentPath_;
    bool validating_ = false;
    std::vector<ValidationListener> listeners_;
    ValidationCache cache_;
    int validationDelay_ = 500; // ms
    
    void NotifyListeners(const ValidationResult& result);
};

} // namespace Config
} // namespace NovaEngine
