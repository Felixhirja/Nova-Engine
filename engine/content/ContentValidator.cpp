#include "ContentValidator.h"
#include <sstream>
#include <algorithm>

namespace NovaEngine {

// BalanceValidator implementation
bool BalanceValidator::Validate(const ContentDefinition& content, 
                                std::vector<ValidationResult>& results) const {
    bool valid = true;
    SimpleJson json = content.ToJson();
    
    for (const auto& rule : rules_) {
        auto fieldNode = json.Get(rule.fieldName);
        if (!fieldNode || !fieldNode->IsNumber()) {
            continue;
        }
        
        float value = static_cast<float>(fieldNode->AsNumber());
        
        // Check hard limits
        if (value < rule.minValue) {
            results.emplace_back(rule.fieldName,
                "Value " + std::to_string(value) + " is below minimum " + 
                std::to_string(rule.minValue),
                ValidationSeverity::Error);
            valid = false;
        }
        
        if (value > rule.maxValue) {
            results.emplace_back(rule.fieldName,
                "Value " + std::to_string(value) + " is above maximum " + 
                std::to_string(rule.maxValue),
                ValidationSeverity::Error);
            valid = false;
        }
        
        // Check recommended ranges
        if (value < rule.recommendedMin || value > rule.recommendedMax) {
            results.emplace_back(rule.fieldName,
                "Value " + std::to_string(value) + " is outside recommended range [" +
                std::to_string(rule.recommendedMin) + ", " + 
                std::to_string(rule.recommendedMax) + "]",
                ValidationSeverity::Warning);
        }
    }
    
    return valid;
}

// ReferenceValidator implementation
bool ReferenceValidator::Validate(const ContentDefinition& content, 
                                 std::vector<ValidationResult>& results) const {
    bool valid = true;
    
    auto dependencies = content.GetDependencies();
    for (const auto& depId : dependencies) {
        auto* refContent = ContentRegistry::Instance().GetContent(depId);
        if (!refContent) {
            results.emplace_back("dependencies",
                "Referenced content not found: " + depId,
                ValidationSeverity::Error);
            valid = false;
        }
    }
    
    return valid;
}

// ConsistencyValidator implementation
bool ConsistencyValidator::Validate(const ContentDefinition& content, 
                                   std::vector<ValidationResult>& results) const {
    bool valid = true;
    
    for (const auto& pair : checks_) {
        if (!pair.second(content, results)) {
            valid = false;
        }
    }
    
    return valid;
}

// CompletenessValidator implementation
bool CompletenessValidator::Validate(const ContentDefinition& content, 
                                    std::vector<ValidationResult>& results) const {
    bool valid = true;
    SimpleJson json = content.ToJson();
    
    // Check basic required fields
    if (!json.Get("id")) {
        results.emplace_back("id", "Missing required field: id", 
                           ValidationSeverity::Critical);
        valid = false;
    }
    
    if (!json.Get("type")) {
        results.emplace_back("type", "Missing required field: type", 
                           ValidationSeverity::Critical);
        valid = false;
    }
    
    // Check metadata completeness
    auto metadataNode = json.Get("metadata");
    if (!metadataNode) {
        results.emplace_back("metadata", "Missing metadata section", 
                           ValidationSeverity::Warning);
    } else {
        if (!metadataNode->Get("version")) {
            results.emplace_back("metadata.version", 
                               "Missing version in metadata", 
                               ValidationSeverity::Info);
        }
        
        if (!metadataNode->Get("author")) {
            results.emplace_back("metadata.author", 
                               "Missing author in metadata", 
                               ValidationSeverity::Info);
        }
    }
    
    return valid;
}

// FormatValidator implementation
bool FormatValidator::Validate(const ContentDefinition& content, 
                              std::vector<ValidationResult>& results) const {
    bool valid = true;
    
    // Check ID format
    const std::string& id = content.GetId();
    if (id.empty()) {
        results.emplace_back("id", "Content ID cannot be empty", 
                           ValidationSeverity::Critical);
        valid = false;
    }
    
    // Check for invalid characters in ID
    for (char c : id) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') {
            results.emplace_back("id", 
                               "Content ID contains invalid character: " + std::string(1, c),
                               ValidationSeverity::Error);
            valid = false;
            break;
        }
    }
    
    // Check type format
    const std::string& type = content.GetType();
    if (type.empty()) {
        results.emplace_back("type", "Content type cannot be empty", 
                           ValidationSeverity::Critical);
        valid = false;
    }
    
    return valid;
}

// ContentValidatorRegistry implementation
void ContentValidatorRegistry::RegisterValidator(std::shared_ptr<IContentValidator> validator) {
    if (validator) {
        validators_.push_back(validator);
    }
}

void ContentValidatorRegistry::UnregisterValidator(const std::string& name) {
    validators_.erase(
        std::remove_if(validators_.begin(), validators_.end(),
            [&name](const auto& v) { return v->GetName() == name; }),
        validators_.end());
}

std::vector<std::shared_ptr<IContentValidator>> 
ContentValidatorRegistry::GetValidatorsForType(const std::string& typeName) const {
    std::vector<std::shared_ptr<IContentValidator>> result;
    
    for (const auto& validator : validators_) {
        if (validator->SupportsType(typeName)) {
            result.push_back(validator);
        }
    }
    
    return result;
}

bool ContentValidatorRegistry::ValidateContent(const ContentDefinition& content, 
                                              std::vector<ValidationResult>& results) const {
    bool valid = true;
    
    auto validators = GetValidatorsForType(content.GetType());
    for (const auto& validator : validators) {
        if (!validator->Validate(content, results)) {
            valid = false;
        }
    }
    
    return valid;
}

bool ContentValidatorRegistry::ValidateAllContent(
    std::unordered_map<std::string, std::vector<ValidationResult>>& resultMap) const {
    bool allValid = true;
    
    auto& registry = ContentRegistry::Instance();
    auto allContent = registry.QueryContent([](const ContentDefinition*) { return true; });
    
    for (auto* content : allContent) {
        std::vector<ValidationResult> results;
        if (!ValidateContent(*content, results)) {
            resultMap[content->GetId()] = results;
            allValid = false;
        } else if (!results.empty()) {
            // Include warnings/info even for valid content
            resultMap[content->GetId()] = results;
        }
    }
    
    return allValid;
}

std::string ContentValidatorRegistry::GenerateReport(
    const std::unordered_map<std::string, std::vector<ValidationResult>>& resultMap) const {
    std::ostringstream report;
    
    report << "# Content Validation Report\n\n";
    
    size_t totalErrors = 0;
    size_t totalWarnings = 0;
    size_t totalInfo = 0;
    
    for (const auto& pair : resultMap) {
        for (const auto& result : pair.second) {
            switch (result.severity) {
                case ValidationSeverity::Critical:
                case ValidationSeverity::Error:
                    totalErrors++;
                    break;
                case ValidationSeverity::Warning:
                    totalWarnings++;
                    break;
                case ValidationSeverity::Info:
                    totalInfo++;
                    break;
            }
        }
    }
    
    report << "## Summary\n\n";
    report << "- **Total Issues:** " << (totalErrors + totalWarnings + totalInfo) << "\n";
    report << "- **Errors:** " << totalErrors << "\n";
    report << "- **Warnings:** " << totalWarnings << "\n";
    report << "- **Info:** " << totalInfo << "\n\n";
    
    if (!resultMap.empty()) {
        report << "## Details\n\n";
        
        for (const auto& pair : resultMap) {
            report << "### " << pair.first << "\n\n";
            
            for (const auto& result : pair.second) {
                std::string severityStr;
                switch (result.severity) {
                    case ValidationSeverity::Critical:
                        severityStr = "🔴 CRITICAL";
                        break;
                    case ValidationSeverity::Error:
                        severityStr = "❌ ERROR";
                        break;
                    case ValidationSeverity::Warning:
                        severityStr = "⚠️ WARNING";
                        break;
                    case ValidationSeverity::Info:
                        severityStr = "ℹ️ INFO";
                        break;
                }
                
                report << "- **" << severityStr << "** ";
                if (!result.field.empty()) {
                    report << "`" << result.field << "`: ";
                }
                report << result.message << "\n";
                
                if (!result.suggestionText.empty()) {
                    report << "  - *Suggestion:* " << result.suggestionText << "\n";
                }
            }
            
            report << "\n";
        }
    } else {
        report << "✅ All content is valid!\n";
    }
    
    return report.str();
}

// ValidationBuilder implementation
std::vector<ValidationResult> ValidationBuilder::Execute() const {
    std::vector<ValidationResult> results;
    
    auto* content = ContentRegistry::Instance().GetContent(contentId_);
    if (!content) {
        results.emplace_back("", "Content not found: " + contentId_,
                           ValidationSeverity::Critical);
        return results;
    }
    
    for (const auto& validator : validators_) {
        validator->Validate(*content, results);
    }
    
    return results;
}

} // namespace NovaEngine
