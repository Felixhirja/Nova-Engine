#include "JsonSchema.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

namespace schema {

// ValidationResult implementation

std::string ValidationResult::GetErrorReport() const {
    if (success) {
        return "Validation successful - no errors found.";
    }
    
    std::stringstream ss;
    ss << "Validation failed with " << errors.size() << " error(s):\n";
    
    for (size_t i = 0; i < errors.size(); ++i) {
        const auto& error = errors[i];
        ss << "  " << (i + 1) << ". Path: " << (error.path.empty() ? "<root>" : error.path) << "\n";
        ss << "     Error: " << error.message << "\n";
        if (!error.schemaRule.empty()) {
            ss << "     Rule: " << error.schemaRule << "\n";
        }
        if (!error.suggestion.empty()) {
            ss << "     💡 Suggestion: " << error.suggestion << "\n";
        }
        if (i < errors.size() - 1) {
            ss << "\n";
        }
    }
    
    return ss.str();
}

// JsonSchema implementation

std::unique_ptr<JsonSchema> JsonSchema::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open schema file: " << filename << std::endl;
        return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto parseResult = simplejson::Parse(content);
    
    if (!parseResult.success) {
        std::cerr << "Failed to parse schema JSON in file: " << filename 
                  << " - " << parseResult.errorMessage << std::endl;
        return nullptr;
    }

    if (!parseResult.value.IsObject()) {
        std::cerr << "Schema root is not an object in file: " << filename << std::endl;
        return nullptr;
    }

    return LoadFromJson(parseResult.value.AsObject());
}

std::unique_ptr<JsonSchema> JsonSchema::LoadFromJson(const simplejson::JsonObject& schemaJson) {
    auto schema = std::make_unique<JsonSchema>();
    
    // Load basic schema metadata
    auto titleIt = schemaJson.find("title");
    if (titleIt != schemaJson.end() && titleIt->second.IsString()) {
        schema->title_ = titleIt->second.AsString();
    }
    
    auto descIt = schemaJson.find("description");
    if (descIt != schemaJson.end() && descIt->second.IsString()) {
        schema->description_ = descIt->second.AsString();
    }
    
    // Load properties
    auto propsIt = schemaJson.find("properties");
    if (propsIt != schemaJson.end() && propsIt->second.IsObject()) {
        const auto& properties = propsIt->second.AsObject();
        for (const auto& [propName, propDef] : properties) {
            if (propDef.IsObject()) {
                SchemaProperty prop = ParseProperty(propDef.AsObject());
                schema->properties_[propName] = prop;
            }
        }
    }
    
    // Load required properties
    auto reqIt = schemaJson.find("required");
    if (reqIt != schemaJson.end() && reqIt->second.IsArray()) {
        const auto& required = reqIt->second.AsArray();
        for (const auto& item : required) {
            if (item.IsString()) {
                schema->requiredProperties_.push_back(item.AsString());
            }
        }
    }
    
    // Load additionalProperties setting
    auto addPropsIt = schemaJson.find("additionalProperties");
    if (addPropsIt != schemaJson.end() && addPropsIt->second.IsBoolean()) {
        schema->additionalProperties_ = addPropsIt->second.AsBoolean();
    }
    
    return schema;
}

ValidationResult JsonSchema::Validate(const simplejson::JsonValue& value, const std::string& rootPath) const {
    ValidationResult result;
    
    if (!value.IsObject()) {
        result.AddError(rootPath, "Expected object, got " + TypeToString(GetValueType(value)));
        return result;
    }
    
    return ValidateObject(value.AsObject(), rootPath);
}

ValidationResult JsonSchema::ValidateObject(const simplejson::JsonObject& obj, const std::string& rootPath) const {
    ValidationResult result;
    
    // Check required properties
    for (const auto& required : requiredProperties_) {
        if (obj.find(required) == obj.end()) {
            std::string suggestion = "Add \"" + required + "\" property to the configuration object";
            result.AddError(JoinPath(rootPath, required), 
                          "Required property '" + required + "' is missing", 
                          "required",
                          suggestion);
        }
    }
    
    // Validate each property in the object
    for (const auto& [propName, propValue] : obj) {
        std::string currentPath = JoinPath(rootPath, propName);
        
        auto schemaIt = properties_.find(propName);
        if (schemaIt != properties_.end()) {
            // Property has schema definition - validate it
            ValidateProperty(propName, propValue, schemaIt->second, currentPath, result);
        } else if (!additionalProperties_) {
            // Additional properties not allowed
            std::string suggestion = "Remove \"" + propName + "\" property or check for typos in property name";
            result.AddError(currentPath, 
                          "Additional property '" + propName + "' is not allowed", 
                          "additionalProperties",
                          suggestion);
        }
    }
    
    return result;
}

void JsonSchema::ValidateProperty(const std::string& propName, 
                                const simplejson::JsonValue& value,
                                const SchemaProperty& schema,
                                const std::string& currentPath,
                                ValidationResult& result) const {
    
    // Check if null value is allowed
    if (value.IsNull()) {
        if (!schema.nullable) {
            result.AddError(currentPath, "Property cannot be null", "nullable");
        }
        return;
    }
    
    // Type validation
    auto actualType = GetValueType(value);
    if (schema.type != SchemaProperty::Type::Any && 
        schema.type != actualType) {
        std::string suggestion = "Change value to " + TypeToString(schema.type) + " type";
        if (schema.type == SchemaProperty::Type::String) {
            suggestion += " (wrap in quotes if it's text)";
        } else if (schema.type == SchemaProperty::Type::Number) {
            suggestion += " (use a numeric value like 42 or 3.14)";
        } else if (schema.type == SchemaProperty::Type::Boolean) {
            suggestion += " (use true or false)";
        } else if (schema.type == SchemaProperty::Type::Array) {
            suggestion += " (use square brackets: [item1, item2])";
        } else if (schema.type == SchemaProperty::Type::Object) {
            suggestion += " (use curly braces: {\"key\": \"value\"})";
        }
        
        result.AddError(currentPath, 
                      "Expected " + TypeToString(schema.type) + 
                      ", got " + TypeToString(actualType), 
                      "type",
                      suggestion);
        return; // Skip further validation if type is wrong
    }
    
    // Type-specific validation
    switch (schema.type) {
        case SchemaProperty::Type::String:
            ValidateString(value.AsString(), schema, currentPath, result);
            break;
            
        case SchemaProperty::Type::Number:
            ValidateNumber(value.AsNumber(), schema, currentPath, result);
            break;
            
        case SchemaProperty::Type::Array:
            ValidateArray(value.AsArray(), schema, currentPath, result);
            break;
            
        case SchemaProperty::Type::Object:
            ValidateNestedObject(value.AsObject(), schema, currentPath, result);
            break;
            
        default:
            // Boolean and Null don't need additional validation
            break;
    }
}

void JsonSchema::ValidateString(const std::string& str, 
                              const SchemaProperty& schema,
                              const std::string& currentPath,
                              ValidationResult& result) const {
    
    // Length validation
    if (str.length() < schema.minLength) {
        result.AddError(currentPath, 
                      "String length " + std::to_string(str.length()) + 
                      " is less than minimum " + std::to_string(schema.minLength), 
                      "minLength");
    }
    
    if (str.length() > schema.maxLength) {
        result.AddError(currentPath, 
                      "String length " + std::to_string(str.length()) + 
                      " exceeds maximum " + std::to_string(schema.maxLength), 
                      "maxLength");
    }
    
    // Enum validation
    if (!schema.enumValues.empty()) {
        bool found = std::find(schema.enumValues.begin(), schema.enumValues.end(), str) 
                    != schema.enumValues.end();
        if (!found) {
            std::stringstream ss;
            ss << "Value '" << str << "' is not one of the allowed values: [";
            for (size_t i = 0; i < schema.enumValues.size(); ++i) {
                ss << "'" << schema.enumValues[i] << "'";
                if (i < schema.enumValues.size() - 1) ss << ", ";
            }
            ss << "]";
            result.AddError(currentPath, ss.str(), "enum");
        }
    }
}

void JsonSchema::ValidateNumber(double num, 
                              const SchemaProperty& schema,
                              const std::string& currentPath,
                              ValidationResult& result) const {
    
    if (num < schema.minimum) {
        result.AddError(currentPath, 
                      "Value " + std::to_string(num) + 
                      " is less than minimum " + std::to_string(schema.minimum), 
                      "minimum");
    }
    
    if (num > schema.maximum) {
        result.AddError(currentPath, 
                      "Value " + std::to_string(num) + 
                      " exceeds maximum " + std::to_string(schema.maximum), 
                      "maximum");
    }
}

void JsonSchema::ValidateArray(const simplejson::JsonArray& arr, 
                             const SchemaProperty& schema,
                             const std::string& currentPath,
                             ValidationResult& result) const {
    
    // Size validation
    if (arr.size() < schema.minItems) {
        result.AddError(currentPath, 
                      "Array size " + std::to_string(arr.size()) + 
                      " is less than minimum " + std::to_string(schema.minItems), 
                      "minItems");
    }
    
    if (arr.size() > schema.maxItems) {
        result.AddError(currentPath, 
                      "Array size " + std::to_string(arr.size()) + 
                      " exceeds maximum " + std::to_string(schema.maxItems), 
                      "maxItems");
    }
    
    // Unique items validation
    if (schema.uniqueItems) {
        // This is a simplified uniqueness check - in a full implementation
        // we'd need to properly compare JsonValues
        // For now, we'll skip this validation
    }
    
    // Validate each item if item schema is provided
    if (schema.itemSchema) {
        for (size_t i = 0; i < arr.size(); ++i) {
            std::string itemPath = currentPath + "[" + std::to_string(i) + "]";
            auto itemResult = schema.itemSchema->Validate(arr[i], itemPath);
            if (!itemResult.success) {
                for (const auto& error : itemResult.errors) {
                    result.AddError(error);
                }
            }
        }
    }
}

void JsonSchema::ValidateNestedObject(const simplejson::JsonObject& obj, 
                                    const SchemaProperty& schema,
                                    const std::string& currentPath,
                                    ValidationResult& result) const {
    
    // Validate against nested schema if provided
    if (schema.itemSchema) {
        auto nestedResult = schema.itemSchema->ValidateObject(obj, currentPath);
        if (!nestedResult.success) {
            for (const auto& error : nestedResult.errors) {
                result.AddError(error);
            }
        }
    }
    
    // Validate individual properties if defined
    for (const auto& [propName, propValue] : obj) {
        auto propSchemaIt = schema.properties.find(propName);
        if (propSchemaIt != schema.properties.end()) {
            std::string propPath = JoinPath(currentPath, propName);
            ValidateProperty(propName, propValue, propSchemaIt->second, propPath, result);
        }
    }
}

// Helper methods

SchemaProperty JsonSchema::ParseProperty(const simplejson::JsonObject& propDef) {
    SchemaProperty prop;
    
    // Parse type
    auto typeIt = propDef.find("type");
    if (typeIt != propDef.end() && typeIt->second.IsString()) {
        prop.type = ParseType(typeIt->second.AsString());
    }
    
    // Parse constraints based on type
    if (prop.type == SchemaProperty::Type::String) {
        auto minLenIt = propDef.find("minLength");
        if (minLenIt != propDef.end() && minLenIt->second.IsNumber()) {
            prop.minLength = static_cast<size_t>(minLenIt->second.AsNumber());
        }
        
        auto maxLenIt = propDef.find("maxLength");
        if (maxLenIt != propDef.end() && maxLenIt->second.IsNumber()) {
            prop.maxLength = static_cast<size_t>(maxLenIt->second.AsNumber());
        }
        
        auto enumIt = propDef.find("enum");
        if (enumIt != propDef.end() && enumIt->second.IsArray()) {
            const auto& enumArray = enumIt->second.AsArray();
            for (const auto& item : enumArray) {
                if (item.IsString()) {
                    prop.enumValues.push_back(item.AsString());
                }
            }
        }
    } else if (prop.type == SchemaProperty::Type::Number) {
        auto minIt = propDef.find("minimum");
        if (minIt != propDef.end() && minIt->second.IsNumber()) {
            prop.minimum = minIt->second.AsNumber();
        }
        
        auto maxIt = propDef.find("maximum");
        if (maxIt != propDef.end() && maxIt->second.IsNumber()) {
            prop.maximum = maxIt->second.AsNumber();
        }
    } else if (prop.type == SchemaProperty::Type::Array) {
        auto minItemsIt = propDef.find("minItems");
        if (minItemsIt != propDef.end() && minItemsIt->second.IsNumber()) {
            prop.minItems = static_cast<size_t>(minItemsIt->second.AsNumber());
        }
        
        auto maxItemsIt = propDef.find("maxItems");
        if (maxItemsIt != propDef.end() && maxItemsIt->second.IsNumber()) {
            prop.maxItems = static_cast<size_t>(maxItemsIt->second.AsNumber());
        }
        
        auto uniqueIt = propDef.find("uniqueItems");
        if (uniqueIt != propDef.end() && uniqueIt->second.IsBoolean()) {
            prop.uniqueItems = uniqueIt->second.AsBoolean();
        }
    }
    
    // Parse default value
    auto defaultIt = propDef.find("default");
    if (defaultIt != propDef.end()) {
        prop.defaultValue = defaultIt->second;
    }
    
    return prop;
}

SchemaProperty::Type JsonSchema::GetValueType(const simplejson::JsonValue& value) {
    if (value.IsNull()) return SchemaProperty::Type::Null;
    if (value.IsBoolean()) return SchemaProperty::Type::Boolean;
    if (value.IsNumber()) return SchemaProperty::Type::Number;
    if (value.IsString()) return SchemaProperty::Type::String;
    if (value.IsArray()) return SchemaProperty::Type::Array;
    if (value.IsObject()) return SchemaProperty::Type::Object;
    return SchemaProperty::Type::Any;
}

SchemaProperty::Type JsonSchema::ParseType(const std::string& typeStr) {
    if (typeStr == "string") return SchemaProperty::Type::String;
    if (typeStr == "number") return SchemaProperty::Type::Number;
    if (typeStr == "boolean") return SchemaProperty::Type::Boolean;
    if (typeStr == "array") return SchemaProperty::Type::Array;
    if (typeStr == "object") return SchemaProperty::Type::Object;
    if (typeStr == "null") return SchemaProperty::Type::Null;
    return SchemaProperty::Type::Any;
}

std::string JsonSchema::TypeToString(SchemaProperty::Type type) {
    switch (type) {
        case SchemaProperty::Type::String: return "string";
        case SchemaProperty::Type::Number: return "number";
        case SchemaProperty::Type::Boolean: return "boolean";
        case SchemaProperty::Type::Array: return "array";
        case SchemaProperty::Type::Object: return "object";
        case SchemaProperty::Type::Null: return "null";
        case SchemaProperty::Type::Any: return "any";
    }
    return "unknown";
}

std::string JsonSchema::JoinPath(const std::string& base, const std::string& property) {
    if (base.empty()) return "/" + property;
    return base + "/" + property;
}

void JsonSchema::AddProperty(const std::string& name, const SchemaProperty& property) {
    properties_[name] = property;
}

// SchemaRegistry implementation

SchemaRegistry& SchemaRegistry::Instance() {
    static SchemaRegistry instance;
    return instance;
}

void SchemaRegistry::RegisterSchema(const std::string& id, std::unique_ptr<JsonSchema> schema) {
    schemas_[id] = std::move(schema);
}

JsonSchema* SchemaRegistry::GetSchema(const std::string& id) const {
    auto it = schemas_.find(id);
    return (it != schemas_.end()) ? it->second.get() : nullptr;
}

bool SchemaRegistry::LoadSchemaFromFile(const std::string& id, const std::string& filename) {
    auto schema = JsonSchema::LoadFromFile(filename);
    if (schema) {
        RegisterSchema(id, std::move(schema));
        return true;
    }
    return false;
}

ValidationResult SchemaRegistry::ValidateFile(const std::string& schemaId, const std::string& configFile) const {
    ValidationResult result;
    
    auto* schema = GetSchema(schemaId);
    if (!schema) {
        result.AddError("", "Schema '" + schemaId + "' not found in registry");
        return result;
    }
    
    // Load the config file
    std::ifstream file(configFile);
    if (!file.is_open()) {
        result.AddError("", "Failed to open config file: " + configFile);
        return result;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto parseResult = simplejson::Parse(content);
    
    if (!parseResult.success) {
        result.AddError("", "Failed to parse JSON: " + parseResult.errorMessage);
        return result;
    }
    
    return schema->Validate(parseResult.value);
}

std::vector<std::string> SchemaRegistry::GetSchemaIds() const {
    std::vector<std::string> ids;
    ids.reserve(schemas_.size());
    for (const auto& [id, schema] : schemas_) {
        ids.push_back(id);
    }
    return ids;
}

// SchemaFactory implementation

namespace SchemaFactory {

std::unique_ptr<JsonSchema> CreateActorConfigSchema() {
    // This will be implemented to create a basic actor config schema programmatically
    // For now, we'll load from file
    return JsonSchema::LoadFromFile("assets/schemas/actor_config.schema.json");
}

std::unique_ptr<JsonSchema> CreateShipConfigSchema() {
    return JsonSchema::LoadFromFile("assets/schemas/ship_config.schema.json");
}

std::unique_ptr<JsonSchema> CreateStationConfigSchema() {
    return JsonSchema::LoadFromFile("assets/schemas/station_config.schema.json");
}

std::unique_ptr<JsonSchema> CreateProjectileConfigSchema() {
    // TODO: Create projectile schema file
    return CreateActorConfigSchema();
}

} // namespace SchemaFactory

} // namespace schema