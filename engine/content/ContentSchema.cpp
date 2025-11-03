#include "ContentSchema.h"
#include <iostream>
#include <sstream>
#include <fstream>

namespace NovaEngine {

// SchemaField validation
bool SchemaField::ValidateValue(const SimpleJson& value, std::vector<std::string>& errors) const {
    // Check null
    if (value.IsNull()) {
        if (!nullable) {
            errors.push_back("Field '" + name + "' cannot be null");
            return false;
        }
        return true;
    }
    
    // Type checking
    switch (type) {
        case SchemaFieldType::String:
            if (!value.IsString()) {
                errors.push_back("Field '" + name + "' must be a string");
                return false;
            }
            if (!enumValues.empty()) {
                std::string val = value.AsString();
                bool found = false;
                for (const auto& enumVal : enumValues) {
                    if (val == enumVal) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    errors.push_back("Field '" + name + "' has invalid enum value: " + val);
                    return false;
                }
            }
            break;
            
        case SchemaFieldType::Integer:
            if (!value.IsNumber()) {
                errors.push_back("Field '" + name + "' must be an integer");
                return false;
            }
            if (std::holds_alternative<int>(minValue)) {
                int val = static_cast<int>(value.AsNumber());
                if (val < std::get<int>(minValue)) {
                    errors.push_back("Field '" + name + "' is below minimum value");
                    return false;
                }
            }
            if (std::holds_alternative<int>(maxValue)) {
                int val = static_cast<int>(value.AsNumber());
                if (val > std::get<int>(maxValue)) {
                    errors.push_back("Field '" + name + "' is above maximum value");
                    return false;
                }
            }
            break;
            
        case SchemaFieldType::Float:
            if (!value.IsNumber()) {
                errors.push_back("Field '" + name + "' must be a number");
                return false;
            }
            if (std::holds_alternative<float>(minValue)) {
                float val = static_cast<float>(value.AsNumber());
                if (val < std::get<float>(minValue)) {
                    errors.push_back("Field '" + name + "' is below minimum value");
                    return false;
                }
            }
            if (std::holds_alternative<float>(maxValue)) {
                float val = static_cast<float>(value.AsNumber());
                if (val > std::get<float>(maxValue)) {
                    errors.push_back("Field '" + name + "' is above maximum value");
                    return false;
                }
            }
            break;
            
        case SchemaFieldType::Boolean:
            if (!value.IsBool()) {
                errors.push_back("Field '" + name + "' must be a boolean");
                return false;
            }
            break;
            
        case SchemaFieldType::Object:
            if (!value.IsObject()) {
                errors.push_back("Field '" + name + "' must be an object");
                return false;
            }
            if (nestedSchema) {
                return nestedSchema->Validate(value, errors);
            }
            break;
            
        case SchemaFieldType::Array:
            if (!value.IsArray()) {
                errors.push_back("Field '" + name + "' must be an array");
                return false;
            }
            break;
            
        case SchemaFieldType::Reference:
            if (!value.IsString()) {
                errors.push_back("Field '" + name + "' (reference) must be a string");
                return false;
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

// ContentSchema implementation
void ContentSchema::AddField(const SchemaField& field) {
    fieldIndex_[field.name] = fields_.size();
    fields_.push_back(field);
}

const SchemaField* ContentSchema::GetField(const std::string& name) const {
    auto it = fieldIndex_.find(name);
    if (it != fieldIndex_.end()) {
        return &fields_[it->second];
    }
    return nullptr;
}

void ContentSchema::SetBaseSchema(const std::string& baseTypeName) {
    baseSchema_ = baseTypeName;
}

bool ContentSchema::Validate(const SimpleJson& data, std::vector<std::string>& errors) const {
    if (!data.IsObject()) {
        errors.push_back("Content data must be an object");
        return false;
    }
    
    bool valid = true;
    
    // Validate base schema first if exists
    if (!baseSchema_.empty()) {
        auto baseSchema = ContentSchemaRegistry::Instance().GetSchema(baseSchema_);
        if (baseSchema) {
            if (!baseSchema->Validate(data, errors)) {
                valid = false;
            }
        }
    }
    
    // Check required fields
    for (const auto& field : fields_) {
        auto fieldNode = data.Get(field.name);
        
        if (!fieldNode) {
            if (field.required) {
                errors.push_back("Required field missing: " + field.name);
                valid = false;
            }
            continue;
        }
        
        if (!field.ValidateValue(*fieldNode, errors)) {
            valid = false;
        }
    }
    
    return valid;
}

std::string ContentSchema::GenerateDocumentation() const {
    std::ostringstream doc;
    
    doc << "# " << typeName_ << "\n\n";
    
    if (!description_.empty()) {
        doc << description_ << "\n\n";
    }
    
    if (!baseSchema_.empty()) {
        doc << "**Inherits from:** " << baseSchema_ << "\n\n";
    }
    
    doc << "## Fields\n\n";
    
    for (const auto& field : fields_) {
        doc << "### " << field.name;
        if (field.required) {
            doc << " (Required)";
        }
        doc << "\n\n";
        
        doc << "**Type:** ";
        switch (field.type) {
            case SchemaFieldType::String: doc << "String"; break;
            case SchemaFieldType::Integer: doc << "Integer"; break;
            case SchemaFieldType::Float: doc << "Float"; break;
            case SchemaFieldType::Boolean: doc << "Boolean"; break;
            case SchemaFieldType::Object: doc << "Object"; break;
            case SchemaFieldType::Array: doc << "Array"; break;
            case SchemaFieldType::Reference: doc << "Reference"; break;
            case SchemaFieldType::Enum: doc << "Enum"; break;
        }
        doc << "\n\n";
        
        if (!field.description.empty()) {
            doc << field.description << "\n\n";
        }
        
        if (!field.enumValues.empty()) {
            doc << "**Valid values:** ";
            for (size_t i = 0; i < field.enumValues.size(); ++i) {
                if (i > 0) doc << ", ";
                doc << field.enumValues[i];
            }
            doc << "\n\n";
        }
        
        if (!field.referenceType.empty()) {
            doc << "**References:** " << field.referenceType << "\n\n";
        }
    }
    
    return doc.str();
}

SimpleJson ContentSchema::ToSchemaJson() const {
    SimpleJson schema;
    schema.Set("type", typeName_);
    schema.Set("description", description_);
    
    if (!baseSchema_.empty()) {
        schema.Set("inherits", baseSchema_);
    }
    
    // Add fields
    SimpleJson fieldsArray;
    for (const auto& field : fields_) {
        SimpleJson fieldJson;
        fieldJson.Set("name", field.name);
        fieldJson.Set("required", field.required);
        fieldJson.Set("nullable", field.nullable);
        
        // Add more field details as needed
        
        fieldsArray.Append(fieldJson);
    }
    schema.Set("fields", fieldsArray);
    
    return schema;
}

// ContentSchemaRegistry implementation
void ContentSchemaRegistry::RegisterSchema(std::shared_ptr<ContentSchema> schema) {
    if (schema) {
        schemas_[schema->GetTypeName()] = schema;
    }
}

std::shared_ptr<ContentSchema> ContentSchemaRegistry::GetSchema(const std::string& typeName) const {
    auto it = schemas_.find(typeName);
    return it != schemas_.end() ? it->second : nullptr;
}

bool ContentSchemaRegistry::HasSchema(const std::string& typeName) const {
    return schemas_.find(typeName) != schemas_.end();
}

std::vector<std::string> ContentSchemaRegistry::GetAllSchemaTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : schemas_) {
        types.push_back(pair.first);
    }
    return types;
}

bool ContentSchemaRegistry::ValidateContent(const std::string& typeName, const SimpleJson& data,
                                            std::vector<std::string>& errors) const {
    auto schema = GetSchema(typeName);
    if (!schema) {
        errors.push_back("Schema not found for type: " + typeName);
        return false;
    }
    
    return schema->Validate(data, errors);
}

void ContentSchemaRegistry::GenerateAllDocumentation(const std::string& outputDir) const {
    for (const auto& pair : schemas_) {
        std::string filename = outputDir + "/" + pair.first + "_schema.md";
        std::ofstream file(filename);
        if (file.is_open()) {
            file << pair.second->GenerateDocumentation();
            file.close();
        }
    }
}

} // namespace NovaEngine
