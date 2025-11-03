#pragma once

#include "SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <limits>

namespace schema {

/**
 * Validation error information
 */
struct ValidationError {
    std::string path;           // JSON path where error occurred (e.g., "/physics/mass")
    std::string message;        // Human-readable error message
    std::string schemaRule;     // Schema rule that was violated
    std::string suggestion;     // Developer-friendly suggestion to fix the error
    simplejson::JsonValue value; // The invalid value (optional)
    
    ValidationError(const std::string& p, const std::string& msg, const std::string& rule = "", const std::string& sug = "")
        : path(p), message(msg), schemaRule(rule), suggestion(sug) {}
};

/**
 * Validation result containing success status and any errors
 */
struct ValidationResult {
    bool success = true;
    std::vector<ValidationError> errors;
    
    void AddError(const std::string& path, const std::string& message, const std::string& rule = "", const std::string& suggestion = "") {
        errors.emplace_back(path, message, rule, suggestion);
        success = false;
    }
    
    void AddError(const ValidationError& error) {
        errors.push_back(error);
        success = false;
    }
    
    /**
     * Get formatted error report as string
     */
    std::string GetErrorReport() const;
    
    /**
     * Get brief summary for logging
     */
    std::string GetSummary() const {
        if (success) return "✅ Validation passed";
        return "❌ Validation failed (" + std::to_string(errors.size()) + " error" + (errors.size() == 1 ? "" : "s") + ")";
    }
};

/**
 * Schema property definition for validation
 */
struct SchemaProperty {
    enum class Type {
        String, Number, Boolean, Array, Object, Null, Any
    };
    
    Type type = Type::Any;
    bool required = false;
    bool nullable = false;
    
    // Numeric constraints
    double minimum = std::numeric_limits<double>::lowest();
    double maximum = std::numeric_limits<double>::max();
    
    // String constraints
    size_t minLength = 0;
    size_t maxLength = std::numeric_limits<size_t>::max();
    std::vector<std::string> enumValues;
    
    // Array constraints
    size_t minItems = 0;
    size_t maxItems = std::numeric_limits<size_t>::max();
    bool uniqueItems = false;
    
    // Nested schema for objects and arrays
    std::shared_ptr<class JsonSchema> itemSchema;
    std::unordered_map<std::string, SchemaProperty> properties;
    bool additionalProperties = true;
    
    // Default value
    simplejson::JsonValue defaultValue;
    
    SchemaProperty() = default;
    SchemaProperty(Type t) : type(t) {}
};

/**
 * JSON Schema validator for Nova Engine actor configurations
 * 
 * This is a lightweight implementation that covers the most common validation
 * needs for game configuration files. It supports:
 * - Type validation (string, number, boolean, array, object)
 * - Required properties
 * - Numeric ranges (minimum/maximum)
 * - String length constraints
 * - Enum value validation
 * - Array size and uniqueness constraints
 * - Nested object validation
 * 
 * TODO: Future enhancements for full JSON Schema support:
 * [ ] Pattern matching for strings (regex)
 * [ ] Conditional schemas (if/then/else)
 * [ ] Schema composition (allOf, anyOf, oneOf)
 * [ ] Format validation (date, email, etc.)
 * [ ] Schema references ($ref)
 * [ ] Custom validation functions
 */
class JsonSchema {
public:
    JsonSchema() = default;
    
    /**
     * Load schema from JSON file
     */
    static std::unique_ptr<JsonSchema> LoadFromFile(const std::string& filename);
    
    /**
     * Load schema from JSON object
     */
    static std::unique_ptr<JsonSchema> LoadFromJson(const simplejson::JsonObject& schemaJson);
    
    /**
     * Validate a JSON value against this schema
     */
    ValidationResult Validate(const simplejson::JsonValue& value, const std::string& rootPath = "") const;
    
    /**
     * Validate a JSON object against this schema
     */
    ValidationResult ValidateObject(const simplejson::JsonObject& obj, const std::string& rootPath = "") const;
    
    /**
     * Add a property to the schema
     */
    void AddProperty(const std::string& name, const SchemaProperty& property);
    
    /**
     * Set whether additional properties are allowed
     */
    void SetAdditionalProperties(bool allowed) { additionalProperties_ = allowed; }
    
    /**
     * Add required property
     */
    void AddRequired(const std::string& name) { requiredProperties_.push_back(name); }
    
    /**
     * Get schema title and description
     */
    const std::string& GetTitle() const { return title_; }
    const std::string& GetDescription() const { return description_; }
    void SetTitle(const std::string& title) { title_ = title; }
    void SetDescription(const std::string& description) { description_ = description; }

private:
    std::string title_;
    std::string description_;
    std::unordered_map<std::string, SchemaProperty> properties_;
    std::vector<std::string> requiredProperties_;
    bool additionalProperties_ = true;
    
    /**
     * Validate a specific property
     */
    void ValidateProperty(const std::string& propName, 
                         const simplejson::JsonValue& value,
                         const SchemaProperty& schema,
                         const std::string& currentPath,
                         ValidationResult& result) const;
    
    /**
     * Type conversion helpers
     */
    static SchemaProperty::Type ParseType(const std::string& typeStr);
    static std::string TypeToString(SchemaProperty::Type type);
    
    /**
     * Path manipulation helpers
     */
    static std::string JoinPath(const std::string& base, const std::string& property);
    
    /**
     * Helper methods for validation
     */
    void ValidateString(const std::string& str, 
                       const SchemaProperty& schema,
                       const std::string& currentPath,
                       ValidationResult& result) const;
    
    void ValidateNumber(double num, 
                       const SchemaProperty& schema,
                       const std::string& currentPath,
                       ValidationResult& result) const;
    
    void ValidateArray(const simplejson::JsonArray& arr, 
                      const SchemaProperty& schema,
                      const std::string& currentPath,
                      ValidationResult& result) const;
    
    void ValidateNestedObject(const simplejson::JsonObject& obj, 
                             const SchemaProperty& schema,
                             const std::string& currentPath,
                             ValidationResult& result) const;
    
    /**
     * Schema parsing helpers
     */
    static SchemaProperty ParseProperty(const simplejson::JsonObject& propDef);
    static SchemaProperty::Type GetValueType(const simplejson::JsonValue& value);
};

/**
 * Schema registry for managing multiple schemas
 */
class SchemaRegistry {
public:
    static SchemaRegistry& Instance();
    
    /**
     * Register a schema with a name/ID
     */
    void RegisterSchema(const std::string& id, std::unique_ptr<JsonSchema> schema);
    
    /**
     * Get schema by ID
     */
    JsonSchema* GetSchema(const std::string& id) const;
    
    /**
     * Load and register schema from file
     */
    bool LoadSchemaFromFile(const std::string& id, const std::string& filename);
    
    /**
     * Validate a configuration file against a specific schema
     */
    ValidationResult ValidateFile(const std::string& schemaId, const std::string& configFile) const;
    
    /**
     * Get all registered schema IDs
     */
    std::vector<std::string> GetSchemaIds() const;

private:
    std::unordered_map<std::string, std::unique_ptr<JsonSchema>> schemas_;
};

/**
 * Factory functions for common schema patterns
 */
namespace SchemaFactory {
    /**
     * Create basic actor config schema
     */
    std::unique_ptr<JsonSchema> CreateActorConfigSchema();
    
    /**
     * Create ship-specific schema
     */
    std::unique_ptr<JsonSchema> CreateShipConfigSchema();
    
    /**
     * Create station-specific schema
     */
    std::unique_ptr<JsonSchema> CreateStationConfigSchema();
    
    /**
     * Create projectile-specific schema
     */
    std::unique_ptr<JsonSchema> CreateProjectileConfigSchema();
}

} // namespace schema