#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace ContentManagement {

/**
 * ContentValidator: Validate content consistency and balance
 * 
 * Features:
 * - Schema validation (type checking, required fields)
 * - Business logic validation (balance, dependencies)
 * - Cross-content validation (references, uniqueness)
 * - Performance validation (resource limits)
 * - Custom validation rules
 */
class ContentValidator {
public:
    enum class ValidationSeverity {
        Info,       // Informational message
        Warning,    // Should be fixed but not blocking
        Error,      // Must be fixed before content can be used
        Critical    // Content is completely broken
    };

    struct ValidationResult {
        ValidationSeverity severity;
        std::string message;
        std::string fieldPath;  // JSON path to the problematic field
        std::string suggestion;  // How to fix it
        int lineNumber;          // For file-based validation
    };

    struct ValidationRule {
        std::string name;
        std::string description;
        std::function<bool(const simplejson::JsonObject&, std::vector<ValidationResult>&)> validator;
        bool enabled;
    };

    struct BalanceRule {
        std::string ruleId;
        std::string name;
        std::string fieldPath;
        float minValue;
        float maxValue;
        float recommendedMin;
        float recommendedMax;
        std::string balanceReason;
    };

    ContentValidator();
    ~ContentValidator();

    // Validation
    bool ValidateContent(const simplejson::JsonObject& content, const std::string& contentType, std::vector<ValidationResult>& results);
    bool ValidateFile(const std::string& filePath, std::vector<ValidationResult>& results);
    bool ValidateDirectory(const std::string& directory, std::unordered_map<std::string, std::vector<ValidationResult>>& results);
    
    // Schema Validation
    bool ValidateSchema(const simplejson::JsonObject& content, const simplejson::JsonObject& schema, std::vector<ValidationResult>& results);
    bool ValidateRequiredFields(const simplejson::JsonObject& content, const std::vector<std::string>& requiredFields, std::vector<ValidationResult>& results);
    bool ValidateFieldType(const simplejson::JsonValue& value, const std::string& expectedType, const std::string& fieldPath, std::vector<ValidationResult>& results);
    
    // Balance Validation
    void RegisterBalanceRule(const BalanceRule& rule);
    bool ValidateBalance(const simplejson::JsonObject& content, std::vector<ValidationResult>& results);
    bool CheckValueInRange(const std::string& fieldPath, float value, float min, float max, ValidationSeverity severity, std::vector<ValidationResult>& results);
    
    // Cross-Content Validation
    bool ValidateReferences(const simplejson::JsonObject& content, const std::vector<std::string>& allContentIds, std::vector<ValidationResult>& results);
    bool ValidateUniqueIds(const std::vector<simplejson::JsonObject>& allContent, std::vector<ValidationResult>& results);
    bool ValidateDependencies(const simplejson::JsonObject& content, const std::unordered_map<std::string, simplejson::JsonObject>& allContent, std::vector<ValidationResult>& results);
    
    // Custom Rules
    void RegisterCustomRule(const ValidationRule& rule);
    void EnableRule(const std::string& ruleName, bool enabled);
    std::vector<std::string> GetRegisteredRules() const;
    
    // Performance Validation
    bool ValidatePerformance(const simplejson::JsonObject& content, std::vector<ValidationResult>& results);
    bool CheckResourceLimits(const simplejson::JsonObject& content, std::vector<ValidationResult>& results);
    
    // Batch Validation
    void ValidateAll(const std::string& contentDirectory, std::unordered_map<std::string, std::vector<ValidationResult>>& results);
    bool HasErrors(const std::vector<ValidationResult>& results) const;
    bool HasWarnings(const std::vector<ValidationResult>& results) const;
    
    // Reporting
    std::string GenerateValidationReport(const std::vector<ValidationResult>& results) const;
    std::string GenerateSummaryReport(const std::unordered_map<std::string, std::vector<ValidationResult>>& allResults) const;
    void ExportValidationReport(const std::string& outputPath, const std::unordered_map<std::string, std::vector<ValidationResult>>& allResults);
    
    // Configuration
    void SetStrictMode(bool strict);  // Treat warnings as errors
    void SetMaxErrors(int maxErrors); // Stop validation after N errors
    
    // Schema management
    struct ValidationSchema {
        std::string name;
        std::string description;
    };
    std::vector<ValidationSchema> GetAllSchemas() const;
    void LoadSchemasFromDirectory(const std::string& directory);
    
    // Statistics
    struct ValidationStats {
        int totalValidated;
        int successfulValidations;
        float successRate;
    };
    ValidationStats GetValidationStats() const;
    
    // Custom validators
    void RegisterCustomValidator(const std::string& validatorId, 
        std::function<bool(const simplejson::JsonObject&, std::vector<std::string>&)> validator);
    
    // Dependency rules
    struct DependencyRule {
        std::string ruleId;
        std::string description;
    };
    void RegisterDependencyRule(const DependencyRule& rule);
    
private:
    bool ValidateInternal(const simplejson::JsonObject& content, const std::string& contentType, std::vector<ValidationResult>& results);
    void AddResult(std::vector<ValidationResult>& results, ValidationSeverity severity, const std::string& message, const std::string& fieldPath = "", const std::string& suggestion = "");
    
    std::string GetFieldValue(const simplejson::JsonObject& obj, const std::string& path) const;
    bool FieldExists(const simplejson::JsonObject& obj, const std::string& path) const;
    
    std::unordered_map<std::string, ValidationRule> customRules_;
    std::unordered_map<std::string, BalanceRule> balanceRules_;
    std::unordered_map<std::string, std::function<bool(const simplejson::JsonObject&, std::vector<std::string>&)>> customValidators_;
    std::unordered_map<std::string, DependencyRule> dependencyRules_;
    bool strictMode_;
    int maxErrors_;
};

} // namespace ContentManagement
