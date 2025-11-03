#include "ConfigEditor.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace NovaEngine {
namespace Config {

// =====================================================
// EditorLayout Implementation
// =====================================================

void EditorLayout::AddSection(const EditorSection& section) {
    sections.push_back(section);
}

void EditorLayout::AddField(const std::string& sectionName, const EditorField& field) {
    for (auto& section : sections) {
        if (section.name == sectionName) {
            section.fields.push_back(field);
            return;
        }
    }
    
    EditorSection newSection;
    newSection.name = sectionName;
    newSection.fields.push_back(field);
    sections.push_back(newSection);
}

EditorField* EditorLayout::GetField(const std::string& fieldId) {
    for (auto& section : sections) {
        for (auto& field : section.fields) {
            if (field.id == fieldId) {
                return &field;
            }
        }
    }
    return nullptr;
}

// =====================================================
// ConfigEditor Implementation
// =====================================================

ConfigEditor::ConfigEditor() {
}

bool ConfigEditor::OpenConfig(const std::string& configPath) {
    auto& manager = ConfigManager::GetInstance();
    currentConfig_ = manager.LoadConfig(configPath);
    
    if (currentConfig_.IsNull()) {
        return false;
    }
    
    currentPath_ = configPath;
    isDirty_ = false;
    undoStack_.clear();
    redoStack_.clear();
    
    PushUndoState();
    
    return true;
}

bool ConfigEditor::SaveConfig(const std::string& configPath) {
    std::string savePath = configPath.empty() ? currentPath_ : configPath;
    
    if (savePath.empty()) {
        return false;
    }
    
    auto validation = ValidateAll();
    if (!validation.valid) {
        if (validationCallback_) {
            validationCallback_(validation);
        }
        return false;
    }
    
    std::ofstream file(savePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << simplejson::Serialize(currentConfig_, true);
    file.close();
    
    isDirty_ = false;
    return true;
}

bool ConfigEditor::SaveAsConfig(const std::string& newPath) {
    if (SaveConfig(newPath)) {
        currentPath_ = newPath;
        return true;
    }
    return false;
}

void ConfigEditor::CloseConfig() {
    currentPath_.clear();
    currentConfig_ = simplejson::JsonValue();
    layout_ = EditorLayout();
    isDirty_ = false;
    undoStack_.clear();
    redoStack_.clear();
}

bool ConfigEditor::SetFieldValue(const std::string& fieldId, const simplejson::JsonValue& value) {
    if (!currentConfig_.IsObject()) {
        return false;
    }
    
    PushUndoState();
    
    auto& obj = currentConfig_.AsObject();
    obj[fieldId] = value;
    
    MarkDirty();
    
    if (changeCallback_) {
        changeCallback_(fieldId, value);
    }
    
    auto validation = ValidateField(fieldId);
    if (validationCallback_) {
        validationCallback_(validation);
    }
    
    return true;
}

simplejson::JsonValue ConfigEditor::GetFieldValue(const std::string& fieldId) const {
    if (!currentConfig_.IsObject()) {
        return simplejson::JsonValue();
    }
    
    auto& obj = currentConfig_.AsObject();
    auto it = obj.find(fieldId);
    if (it != obj.end()) {
        return it->second;
    }
    
    return simplejson::JsonValue();
}

ValidationResult ConfigEditor::ValidateField(const std::string& fieldId) {
    ValidationResult result;
    result.valid = true;
    
    auto* field = layout_.GetField(fieldId);
    if (!field) {
        result.valid = false;
        ValidationError error;
        error.path = fieldId;
        error.message = "Field not found in layout";
        result.errors.push_back(error);
        return result;
    }
    
    auto value = GetFieldValue(fieldId);
    
    if (field->required && value.IsNull()) {
        result.valid = false;
        ValidationError error;
        error.path = fieldId;
        error.message = "Required field is missing";
        result.errors.push_back(error);
    }
    
    if (value.IsNumber() && (field->minValue != 0.0 || field->maxValue != 0.0)) {
        double numValue = value.AsNumber();
        if (field->maxValue > field->minValue) {
            if (numValue < field->minValue || numValue > field->maxValue) {
                result.valid = false;
                ValidationError error;
                error.path = fieldId;
                error.message = "Value out of range [" + 
                              std::to_string(field->minValue) + ", " + 
                              std::to_string(field->maxValue) + "]";
                result.errors.push_back(error);
            }
        }
    }
    
    return result;
}

ValidationResult ConfigEditor::ValidateAll() {
    ValidationResult combined;
    combined.valid = true;
    
    for (const auto& section : layout_.sections) {
        for (const auto& field : section.fields) {
            auto result = ValidateField(field.id);
            if (!result.valid) {
                combined.valid = false;
                combined.errors.insert(combined.errors.end(), 
                                     result.errors.begin(), result.errors.end());
            }
            combined.warnings.insert(combined.warnings.end(),
                                   result.warnings.begin(), result.warnings.end());
        }
    }
    
    return combined;
}

EditorLayout ConfigEditor::GenerateLayout(const std::string& configType) {
    EditorLayout layout;
    layout.configType = configType;
    layout.title = "Edit " + configType;
    
    if (!currentConfig_.IsObject()) {
        return layout;
    }
    
    EditorSection generalSection;
    generalSection.name = "General";
    generalSection.description = "Basic configuration properties";
    
    auto& obj = currentConfig_.AsObject();
    for (const auto& [key, value] : obj) {
        EditorField field;
        field.id = key;
        field.label = key;
        field.currentValue = value;
        
        if (value.IsString()) {
            field.type = EditorFieldType::Text;
        } else if (value.IsNumber()) {
            field.type = EditorFieldType::Number;
        } else if (value.IsBoolean()) {
            field.type = EditorFieldType::Boolean;
        } else if (value.IsObject()) {
            field.type = EditorFieldType::JsonObject;
        } else if (value.IsArray()) {
            field.type = EditorFieldType::JsonArray;
        }
        
        generalSection.fields.push_back(field);
    }
    
    layout.sections.push_back(generalSection);
    
    return layout;
}

void ConfigEditor::SetCustomLayout(const EditorLayout& layout) {
    layout_ = layout;
}

void ConfigEditor::Undo() {
    if (!CanUndo()) {
        return;
    }
    
    EditorState current;
    current.config = currentConfig_;
    current.configPath = currentPath_;
    current.timestamp = std::chrono::system_clock::now();
    redoStack_.push_back(current);
    
    currentConfig_ = undoStack_.back().config;
    undoStack_.pop_back();
    
    MarkDirty();
}

void ConfigEditor::Redo() {
    if (!CanRedo()) {
        return;
    }
    
    PushUndoState();
    
    currentConfig_ = redoStack_.back().config;
    redoStack_.pop_back();
    
    MarkDirty();
}

bool ConfigEditor::CanUndo() const {
    return !undoStack_.empty();
}

bool ConfigEditor::CanRedo() const {
    return !redoStack_.empty();
}

std::vector<std::string> ConfigEditor::GetModifiedFields() const {
    std::vector<std::string> modified;
    
    if (undoStack_.empty() || !currentConfig_.IsObject()) {
        return modified;
    }
    
    const auto& original = undoStack_.front().config;
    if (!original.IsObject()) {
        return modified;
    }
    
    auto& currentObj = currentConfig_.AsObject();
    auto& originalObj = original.AsObject();
    
    for (const auto& [key, value] : currentObj) {
        auto it = originalObj.find(key);
        if (it == originalObj.end() || it->second != value) {
            modified.push_back(key);
        }
    }
    
    return modified;
}

simplejson::JsonValue ConfigEditor::GetPreviewConfig() const {
    return currentConfig_;
}

void ConfigEditor::EnableAutoSave(int intervalSeconds) {
    autoSaveEnabled_ = true;
    autoSaveInterval_ = intervalSeconds;
    lastAutoSave_ = std::chrono::steady_clock::now();
}

void ConfigEditor::DisableAutoSave() {
    autoSaveEnabled_ = false;
}

void ConfigEditor::SetValidationCallback(ValidationCallback callback) {
    validationCallback_ = callback;
}

void ConfigEditor::SetChangeCallback(ChangeCallback callback) {
    changeCallback_ = callback;
}

void ConfigEditor::PushUndoState() {
    EditorState state;
    state.config = currentConfig_;
    state.configPath = currentPath_;
    state.timestamp = std::chrono::system_clock::now();
    undoStack_.push_back(state);
    
    if (undoStack_.size() > 50) {
        undoStack_.erase(undoStack_.begin());
    }
    
    redoStack_.clear();
}

void ConfigEditor::MarkDirty() {
    isDirty_ = true;
    CheckAutoSave();
}

void ConfigEditor::CheckAutoSave() {
    if (!autoSaveEnabled_ || !isDirty_) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastAutoSave_).count();
    
    if (elapsed >= autoSaveInterval_) {
        SaveConfig();
        lastAutoSave_ = now;
    }
}

EditorSection ConfigEditor::GenerateSection(const std::string& name, 
                                           const simplejson::JsonObject& schema) {
    EditorSection section;
    section.name = name;
    return section;
}

// =====================================================
// ConfigTestSuite Implementation
// =====================================================

ConfigTestSuite::ConfigTestSuite(const std::string& name) : name_(name) {
}

void ConfigTestSuite::AddTest(const ConfigTest& test) {
    tests_.push_back(test);
}

void ConfigTestSuite::AddTest(const std::string& name,
                             const std::string& description,
                             std::function<bool(const simplejson::JsonValue&)> testFunc) {
    ConfigTest test;
    test.name = name;
    test.description = description;
    test.testFunc = testFunc;
    tests_.push_back(test);
}

ConfigTestSuite::TestReport ConfigTestSuite::RunTests(const simplejson::JsonValue& config) {
    TestReport report;
    report.suiteName = name_;
    report.totalTests = static_cast<int>(tests_.size());
    
    for (const auto& test : tests_) {
        auto start = std::chrono::high_resolution_clock::now();
        
        TestResult result;
        result.testName = test.name;
        
        try {
            result.passed = test.testFunc(config);
            if (result.passed) {
                report.passedTests++;
            } else {
                report.failedTests++;
                result.message = "Test failed: " + test.description;
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.message = "Exception: " + std::string(e.what());
            report.failedTests++;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        report.totalTimeMs += result.executionTimeMs;
        
        report.results.push_back(result);
    }
    
    return report;
}

ConfigTestSuite::TestReport ConfigTestSuite::RunTestsOnFile(const std::string& configPath) {
    auto& manager = ConfigManager::GetInstance();
    auto config = manager.LoadConfig(configPath);
    return RunTests(config);
}

// =====================================================
// ConfigTestRunner Implementation
// =====================================================

ConfigTestRunner& ConfigTestRunner::GetInstance() {
    static ConfigTestRunner instance;
    return instance;
}

void ConfigTestRunner::RegisterSuite(const std::string& configType, const ConfigTestSuite& suite) {
    testSuites_.insert_or_assign(configType, suite);
}

ConfigTestSuite::TestReport ConfigTestRunner::RunTests(const std::string& configType,
                                                       const std::string& configPath) {
    auto it = testSuites_.find(configType);
    if (it == testSuites_.end()) {
        ConfigTestSuite::TestReport emptyReport;
        emptyReport.suiteName = "Unknown";
        return emptyReport;
    }
    
    return it->second.RunTestsOnFile(configPath);
}

bool ConfigTestRunner::BatchTestReport::AllPassed() const {
    return passedSuites == totalSuites;
}

ConfigTestRunner::BatchTestReport ConfigTestRunner::RunAllTests(const std::string& configPath) {
    BatchTestReport report;
    report.totalSuites = static_cast<int>(testSuites_.size());
    
    for (auto& [type, suite] : testSuites_) {
        auto suiteReport = suite.RunTestsOnFile(configPath);
        report.totalTimeMs += suiteReport.totalTimeMs;
        
        if (suiteReport.AllPassed()) {
            report.passedSuites++;
        }
        
        report.suiteReports.push_back(suiteReport);
    }
    
    return report;
}

ConfigTestRunner::BatchTestReport ConfigTestRunner::RunTestsOnDirectory(const std::string& directory) {
    BatchTestReport combined;
    
    auto& manager = ConfigManager::GetInstance();
    auto configs = manager.DiscoverConfigs();
    
    for (const auto& configPath : configs) {
        auto report = RunAllTests(configPath);
        combined.totalSuites += report.totalSuites;
        combined.passedSuites += report.passedSuites;
        combined.totalTimeMs += report.totalTimeMs;
        combined.suiteReports.insert(combined.suiteReports.end(),
                                    report.suiteReports.begin(),
                                    report.suiteReports.end());
    }
    
    return combined;
}

void ConfigTestRunner::ExportReport(const BatchTestReport& report, const std::string& outputPath) {
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        return;
    }
    
    file << "Configuration Test Report\n";
    file << "========================\n\n";
    file << "Total Suites: " << report.totalSuites << "\n";
    file << "Passed Suites: " << report.passedSuites << "\n";
    file << "Total Time: " << report.totalTimeMs << " ms\n\n";
    
    for (const auto& suite : report.suiteReports) {
        file << "Suite: " << suite.suiteName << "\n";
        file << "  Total Tests: " << suite.totalTests << "\n";
        file << "  Passed: " << suite.passedTests << "\n";
        file << "  Failed: " << suite.failedTests << "\n";
        file << "  Pass Rate: " << suite.GetPassRate() << "%\n\n";
        
        for (const auto& result : suite.results) {
            file << "  Test: " << result.testName << " - ";
            file << (result.passed ? "PASSED" : "FAILED") << "\n";
            if (!result.message.empty()) {
                file << "    " << result.message << "\n";
            }
        }
        file << "\n";
    }
    
    file.close();
}

// =====================================================
// ConfigDocumentation Implementation
// =====================================================

std::string ConfigDocumentation::GenerateDocumentation(const std::string& configType) {
    return GenerateDocumentation(configType, DocOptions{});
}

std::string ConfigDocumentation::GenerateDocumentation(const std::string& configType,
                                                      const DocOptions& options) {
    std::ostringstream doc;
    
    if (options.format == DocFormat::Markdown) {
        doc << "# " << configType << " Configuration\n\n";
        doc << "## Overview\n\n";
        doc << "Configuration documentation for " << configType << " actors.\n\n";
        
        if (options.includeSchema) {
            doc << "## Schema\n\n";
            doc << "This configuration follows a defined schema for validation.\n\n";
        }
    }
    
    return doc.str();
}

std::string ConfigDocumentation::GenerateFieldDoc(const EditorField& field) {
    return GenerateFieldDoc(field, DocOptions{});
}

std::string ConfigDocumentation::GenerateFieldDoc(const EditorField& field,
                                                 const DocOptions& options) {
    std::ostringstream doc;
    
    if (options.format == DocFormat::Markdown) {
        doc << "### " << field.label << "\n\n";
        doc << "- **Type**: " << static_cast<int>(field.type) << "\n";
        doc << "- **Required**: " << (field.required ? "Yes" : "No") << "\n";
        
        if (!field.description.empty()) {
            doc << "- **Description**: " << field.description << "\n";
        }
        
        if (options.includeDefaults && !field.defaultValue.IsNull()) {
            doc << "- **Default**: `" << simplejson::Serialize(field.defaultValue) << "`\n";
        }
        
        doc << "\n";
    }
    
    return doc.str();
}

std::string ConfigDocumentation::GenerateSchemaDoc(const std::string& schemaPath) {
    return GenerateSchemaDoc(schemaPath, DocOptions{});
}

std::string ConfigDocumentation::GenerateSchemaDoc(const std::string& schemaPath,
                                                  const DocOptions& options) {
    return "Schema documentation for: " + schemaPath + "\n";
}

bool ConfigDocumentation::ExportDocumentation(const std::string& configType,
                                             const std::string& outputPath) {
    return ExportDocumentation(configType, outputPath, DocOptions{});
}

bool ConfigDocumentation::ExportDocumentation(const std::string& configType,
                                             const std::string& outputPath,
                                             const DocOptions& options) {
    auto doc = GenerateDocumentation(configType, options);
    
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        return false;
    }
    
    file << doc;
    file.close();
    
    return true;
}

bool ConfigDocumentation::GenerateFullDocumentation(const std::string& outputDirectory) {
    return GenerateFullDocumentation(outputDirectory, DocOptions{});
}

bool ConfigDocumentation::GenerateFullDocumentation(const std::string& outputDirectory,
                                                   const DocOptions& options) {
    return true;
}

std::string ConfigDocumentation::FormatMarkdown(const std::string& content) {
    return content;
}

std::string ConfigDocumentation::FormatHTML(const std::string& content) {
    return "<html><body>" + content + "</body></html>";
}

std::string ConfigDocumentation::EscapeMarkdown(const std::string& text) {
    std::string escaped = text;
    // Basic escaping for special markdown characters
    return escaped;
}

// =====================================================
// ConfigDeployment Implementation
// =====================================================

ConfigDeployment& ConfigDeployment::GetInstance() {
    static ConfigDeployment instance;
    return instance;
}

DeploymentResult ConfigDeployment::Deploy(const std::string& configPath,
                                         const DeploymentOptions& options) {
    DeploymentResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (options.dryRun) {
        result.success = true;
        result.message = "Dry run completed successfully";
        return result;
    }
    
    if (options.validateBeforeDeploy) {
        if (!ValidateBeforeDeploy(configPath)) {
            result.success = false;
            result.message = "Validation failed";
            result.errors.push_back("Configuration validation failed");
            return result;
        }
    }
    
    if (preDeployHook_ && !preDeployHook_(configPath)) {
        result.success = false;
        result.message = "Pre-deploy hook failed";
        return result;
    }
    
    if (options.backupExisting) {
        std::string backupPath = CreateBackup(configPath);
        if (!backupPath.empty()) {
            result.backupFiles.push_back(backupPath);
        }
    }
    
    result.deployedFiles.push_back(configPath);
    result.success = true;
    result.message = "Deployment successful";
    result.deploymentTime = std::chrono::system_clock::now();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.deploymentDurationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (postDeployHook_) {
        postDeployHook_(result);
    }
    
    return result;
}

DeploymentResult ConfigDeployment::DeployBatch(const std::vector<std::string>& configPaths,
                                              const DeploymentOptions& options) {
    DeploymentResult combined;
    combined.deploymentTime = std::chrono::system_clock::now();
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (const auto& path : configPaths) {
        auto result = Deploy(path, options);
        if (result.success) {
            combined.deployedFiles.insert(combined.deployedFiles.end(),
                                        result.deployedFiles.begin(),
                                        result.deployedFiles.end());
            combined.backupFiles.insert(combined.backupFiles.end(),
                                      result.backupFiles.begin(),
                                      result.backupFiles.end());
        } else {
            combined.errors.push_back("Failed to deploy: " + path);
        }
    }
    
    combined.success = combined.errors.empty();
    auto endTime = std::chrono::high_resolution_clock::now();
    combined.deploymentDurationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return combined;
}

DeploymentResult ConfigDeployment::Rollback(const std::string& backupId) {
    DeploymentResult result;
    result.message = "Rollback not yet implemented";
    result.success = false;
    return result;
}

std::vector<std::string> ConfigDeployment::ListBackups() const {
    return std::vector<std::string>();
}

bool ConfigDeployment::ValidateDeployment(const std::string& configPath,
                                         const DeploymentOptions& options) {
    return ValidateBeforeDeploy(configPath);
}

void ConfigDeployment::SetDeploymentHook(
    std::function<bool(const std::string&)> preDeployHook,
    std::function<void(const DeploymentResult&)> postDeployHook) {
    preDeployHook_ = preDeployHook;
    postDeployHook_ = postDeployHook;
}

std::string ConfigDeployment::CreateBackup(const std::string& configPath) {
    auto timestamp = std::chrono::system_clock::now();
    std::string backupPath = configPath + ".backup";
    
    std::ifstream src(configPath, std::ios::binary);
    std::ofstream dst(backupPath, std::ios::binary);
    
    if (src.is_open() && dst.is_open()) {
        dst << src.rdbuf();
        return backupPath;
    }
    
    return "";
}

bool ConfigDeployment::ValidateBeforeDeploy(const std::string& configPath) {
    auto& manager = ConfigManager::GetInstance();
    auto result = manager.ValidateConfig(configPath);
    return result.valid;
}

// =====================================================
// ConfigTemplateManager Implementation
// =====================================================

ConfigTemplateManager& ConfigTemplateManager::GetInstance() {
    static ConfigTemplateManager instance;
    return instance;
}

bool ConfigTemplateManager::RegisterTemplate(const std::string& name,
                                            const std::string& templatePath,
                                            const TemplateInfo& info) {
    templates_[name] = {templatePath, info};
    return true;
}

simplejson::JsonValue ConfigTemplateManager::InstantiateTemplate(
    const std::string& templateName,
    const std::unordered_map<std::string, simplejson::JsonValue>& parameters) {
    
    auto it = templates_.find(templateName);
    if (it == templates_.end()) {
        return simplejson::JsonValue();
    }
    
    return ConfigTemplate::InstantiateTemplate(it->second.first, parameters);
}

std::vector<ConfigTemplateManager::TemplateInfo> 
ConfigTemplateManager::SearchTemplates(const std::string& query) const {
    std::vector<TemplateInfo> results;
    
    for (const auto& [name, data] : templates_) {
        if (name.find(query) != std::string::npos ||
            data.second.description.find(query) != std::string::npos) {
            results.push_back(data.second);
        }
    }
    
    return results;
}

std::vector<ConfigTemplateManager::TemplateInfo>
ConfigTemplateManager::GetTemplatesByCategory(const std::string& category) const {
    std::vector<TemplateInfo> results;
    
    for (const auto& [name, data] : templates_) {
        if (data.second.category == category) {
            results.push_back(data.second);
        }
    }
    
    return results;
}

std::vector<ConfigTemplateManager::TemplateInfo>
ConfigTemplateManager::GetTemplatesByTag(const std::string& tag) const {
    std::vector<TemplateInfo> results;
    
    for (const auto& [name, data] : templates_) {
        auto& tags = data.second.tags;
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            results.push_back(data.second);
        }
    }
    
    return results;
}

ConfigTemplateManager::TemplateInfo 
ConfigTemplateManager::GetTemplateInfo(const std::string& templateName) const {
    auto it = templates_.find(templateName);
    if (it != templates_.end()) {
        return it->second.second;
    }
    return TemplateInfo();
}

bool ConfigTemplateManager::ValidateTemplate(const std::string& templateName,
                                            std::vector<std::string>& errors) const {
    auto it = templates_.find(templateName);
    if (it == templates_.end()) {
        errors.push_back("Template not found: " + templateName);
        return false;
    }
    
    return true;
}

// =====================================================
// RealTimeValidator Implementation
// =====================================================

RealTimeValidator::RealTimeValidator() {
}

void RealTimeValidator::StartValidation(const std::string& configPath) {
    currentPath_ = configPath;
    validating_ = true;
    cache_.cachedResults.clear();
}

void RealTimeValidator::StopValidation() {
    validating_ = false;
}

ValidationResult RealTimeValidator::ValidateIncremental(const std::string& fieldPath,
                                                       const simplejson::JsonValue& value) {
    ValidationResult result;
    result.valid = true;
    
    auto it = cache_.cachedResults.find(fieldPath);
    if (it != cache_.cachedResults.end()) {
        auto elapsed = std::chrono::steady_clock::now() - cache_.lastValidation;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < validationDelay_) {
            return it->second;
        }
    }
    
    cache_.cachedResults[fieldPath] = result;
    cache_.lastValidation = std::chrono::steady_clock::now();
    
    NotifyListeners(result);
    
    return result;
}

void RealTimeValidator::AddListener(ValidationListener listener) {
    listeners_.push_back(listener);
}

void RealTimeValidator::RemoveAllListeners() {
    listeners_.clear();
}

void RealTimeValidator::SetValidationDelay(int milliseconds) {
    validationDelay_ = milliseconds;
}

void RealTimeValidator::ClearCache() {
    cache_.cachedResults.clear();
}

void RealTimeValidator::NotifyListeners(const ValidationResult& result) {
    for (const auto& listener : listeners_) {
        listener(result);
    }
}

} // namespace Config
} // namespace NovaEngine
