#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ContentFramework.h"

namespace NovaEngine {

// Validator severity levels
enum class ValidationSeverity {
    Info,
    Warning,
    Error,
    Critical
};

// Validation result
struct ValidationResult {
    std::string field;
    std::string message;
    ValidationSeverity severity;
    std::string suggestionText;
    
    ValidationResult(const std::string& f, const std::string& msg, 
                    ValidationSeverity sev = ValidationSeverity::Error)
        : field(f), message(msg), severity(sev) {}
};

// Base validator interface
class IContentValidator {
public:
    virtual ~IContentValidator() = default;
    
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    
    virtual bool Validate(const ContentDefinition& content, 
                         std::vector<ValidationResult>& results) const = 0;
    
    virtual bool SupportsType(const std::string& typeName) const = 0;
};

// Balance validator - ensures numerical balance
class BalanceValidator : public IContentValidator {
public:
    struct BalanceRule {
        std::string fieldName;
        float minValue = 0.0f;
        float maxValue = 100.0f;
        float recommendedMin = 10.0f;
        float recommendedMax = 90.0f;
        std::string balanceGroup;  // For cross-field balance checks
    };
    
    BalanceValidator(const std::string& typeName) : typeName_(typeName) {}
    
    void AddRule(const BalanceRule& rule) { rules_.push_back(rule); }
    
    std::string GetName() const override { return "Balance Validator"; }
    std::string GetDescription() const override { 
        return "Validates numerical balance for gameplay";
    }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override;
    
    bool SupportsType(const std::string& typeName) const override {
        return typeName == typeName_;
    }
    
private:
    std::string typeName_;
    std::vector<BalanceRule> rules_;
};

// Reference validator - ensures references are valid
class ReferenceValidator : public IContentValidator {
public:
    ReferenceValidator() = default;
    
    std::string GetName() const override { return "Reference Validator"; }
    std::string GetDescription() const override { 
        return "Validates content references and dependencies";
    }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override;
    
    bool SupportsType(const std::string& typeName) const override { 
        return true;  // Works for all types
    }
};

// Consistency validator - checks internal consistency
class ConsistencyValidator : public IContentValidator {
public:
    using ConsistencyCheckFunc = std::function<bool(const ContentDefinition&, 
                                                    std::vector<ValidationResult>&)>;
    
    ConsistencyValidator(const std::string& typeName) : typeName_(typeName) {}
    
    void AddCheck(const std::string& name, ConsistencyCheckFunc check) {
        checks_[name] = check;
    }
    
    std::string GetName() const override { return "Consistency Validator"; }
    std::string GetDescription() const override { 
        return "Validates internal data consistency";
    }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override;
    
    bool SupportsType(const std::string& typeName) const override {
        return typeName == typeName_;
    }
    
private:
    std::string typeName_;
    std::unordered_map<std::string, ConsistencyCheckFunc> checks_;
};

// Completeness validator - checks for missing data
class CompletenessValidator : public IContentValidator {
public:
    CompletenessValidator() = default;
    
    std::string GetName() const override { return "Completeness Validator"; }
    std::string GetDescription() const override { 
        return "Validates content completeness and required data";
    }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override;
    
    bool SupportsType(const std::string& typeName) const override { 
        return true;
    }
};

// Format validator - checks data format and structure
class FormatValidator : public IContentValidator {
public:
    FormatValidator() = default;
    
    std::string GetName() const override { return "Format Validator"; }
    std::string GetDescription() const override { 
        return "Validates data format and structure";
    }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override;
    
    bool SupportsType(const std::string& typeName) const override { 
        return true;
    }
};

// Custom validator support
class CustomValidator : public IContentValidator {
public:
    using ValidatorFunc = std::function<bool(const ContentDefinition&, 
                                            std::vector<ValidationResult>&)>;
    
    CustomValidator(const std::string& name, const std::string& description,
                   const std::string& typeName, ValidatorFunc func)
        : name_(name), description_(description), typeName_(typeName), 
          validatorFunc_(func) {}
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override { return description_; }
    
    bool Validate(const ContentDefinition& content, 
                 std::vector<ValidationResult>& results) const override {
        return validatorFunc_(content, results);
    }
    
    bool SupportsType(const std::string& typeName) const override {
        return typeName_ == "*" || typeName == typeName_;
    }
    
private:
    std::string name_;
    std::string description_;
    std::string typeName_;
    ValidatorFunc validatorFunc_;
};

// Validator registry
class ContentValidatorRegistry {
public:
    static ContentValidatorRegistry& Instance() {
        static ContentValidatorRegistry instance;
        return instance;
    }
    
    void RegisterValidator(std::shared_ptr<IContentValidator> validator);
    void UnregisterValidator(const std::string& name);
    
    std::vector<std::shared_ptr<IContentValidator>> GetValidatorsForType(
        const std::string& typeName) const;
    
    bool ValidateContent(const ContentDefinition& content, 
                        std::vector<ValidationResult>& results) const;
    
    bool ValidateAllContent(
        std::unordered_map<std::string, std::vector<ValidationResult>>& resultMap) const;
    
    // Generate validation report
    std::string GenerateReport(
        const std::unordered_map<std::string, std::vector<ValidationResult>>& resultMap) const;
    
private:
    ContentValidatorRegistry() = default;
    std::vector<std::shared_ptr<IContentValidator>> validators_;
};

// Validation builder for fluent API
class ValidationBuilder {
public:
    ValidationBuilder(const std::string& contentId) : contentId_(contentId) {}
    
    ValidationBuilder& WithValidator(std::shared_ptr<IContentValidator> validator) {
        validators_.push_back(validator);
        return *this;
    }
    
    ValidationBuilder& BalanceCheck(const std::string& field, 
                                    float min, float max,
                                    float recMin, float recMax) {
        // Add balance validator
        return *this;
    }
    
    std::vector<ValidationResult> Execute() const;
    
private:
    std::string contentId_;
    std::vector<std::shared_ptr<IContentValidator>> validators_;
};

} // namespace NovaEngine
