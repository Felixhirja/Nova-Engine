#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include "../SimpleJson.h"

namespace NovaEngine {

// Schema field types
enum class SchemaFieldType {
    String,
    Integer,
    Float,
    Boolean,
    Object,
    Array,
    Reference,  // Reference to another content
    Enum
};

// Schema field definition
struct SchemaField {
    std::string name;
    SchemaFieldType type;
    bool required = false;
    bool nullable = false;
    
    // Constraints
    std::variant<std::monostate, int, float, std::string> defaultValue;
    std::variant<std::monostate, int, float> minValue;
    std::variant<std::monostate, int, float> maxValue;
    std::vector<std::string> enumValues;  // For enum types
    std::string referenceType;  // For reference types
    std::string description;
    
    // Nested schema for Object type
    std::shared_ptr<class ContentSchema> nestedSchema;
    
    // Array element schema
    std::shared_ptr<SchemaField> arrayElementType;
    
    bool ValidateValue(const SimpleJson& value, std::vector<std::string>& errors) const;
};

// Content schema definition
class ContentSchema {
public:
    ContentSchema(const std::string& typeName) : typeName_(typeName) {}
    
    const std::string& GetTypeName() const { return typeName_; }
    void SetDescription(const std::string& desc) { description_ = desc; }
    const std::string& GetDescription() const { return description_; }
    
    // Field management
    void AddField(const SchemaField& field);
    const SchemaField* GetField(const std::string& name) const;
    const std::vector<SchemaField>& GetFields() const { return fields_; }
    
    // Inheritance
    void SetBaseSchema(const std::string& baseTypeName);
    const std::string& GetBaseSchema() const { return baseSchema_; }
    bool HasBaseSchema() const { return !baseSchema_.empty(); }
    
    // Validation
    bool Validate(const SimpleJson& data, std::vector<std::string>& errors) const;
    
    // Documentation
    std::string GenerateDocumentation() const;
    SimpleJson ToSchemaJson() const;
    
private:
    std::string typeName_;
    std::string description_;
    std::string baseSchema_;
    std::vector<SchemaField> fields_;
    std::unordered_map<std::string, size_t> fieldIndex_;
};

// Schema builder for fluent API
class SchemaBuilder {
public:
    explicit SchemaBuilder(const std::string& typeName) 
        : schema_(std::make_shared<ContentSchema>(typeName)) {}
    
    SchemaBuilder& Description(const std::string& desc) {
        schema_->SetDescription(desc);
        return *this;
    }
    
    SchemaBuilder& Inherits(const std::string& baseType) {
        schema_->SetBaseSchema(baseType);
        return *this;
    }
    
    SchemaBuilder& Field(const std::string& name, SchemaFieldType type) {
        currentField_ = SchemaField{};
        currentField_.name = name;
        currentField_.type = type;
        return *this;
    }
    
    SchemaBuilder& Required(bool req = true) {
        currentField_.required = req;
        return *this;
    }
    
    SchemaBuilder& Nullable(bool nullable = true) {
        currentField_.nullable = nullable;
        return *this;
    }
    
    SchemaBuilder& Default(const std::variant<std::monostate, int, float, std::string>& value) {
        currentField_.defaultValue = value;
        return *this;
    }
    
    SchemaBuilder& Min(float minVal) {
        currentField_.minValue = minVal;
        return *this;
    }
    
    SchemaBuilder& Max(float maxVal) {
        currentField_.maxValue = maxVal;
        return *this;
    }
    
    SchemaBuilder& Enum(const std::vector<std::string>& values) {
        currentField_.enumValues = values;
        return *this;
    }
    
    SchemaBuilder& Reference(const std::string& refType) {
        currentField_.referenceType = refType;
        return *this;
    }
    
    SchemaBuilder& Description(const std::string& desc) {
        currentField_.description = desc;
        return *this;
    }
    
    SchemaBuilder& EndField() {
        schema_->AddField(currentField_);
        currentField_ = SchemaField{};
        return *this;
    }
    
    std::shared_ptr<ContentSchema> Build() {
        return schema_;
    }
    
private:
    std::shared_ptr<ContentSchema> schema_;
    SchemaField currentField_;
};

// Schema registry
class ContentSchemaRegistry {
public:
    static ContentSchemaRegistry& Instance() {
        static ContentSchemaRegistry instance;
        return instance;
    }
    
    void RegisterSchema(std::shared_ptr<ContentSchema> schema);
    std::shared_ptr<ContentSchema> GetSchema(const std::string& typeName) const;
    bool HasSchema(const std::string& typeName) const;
    
    std::vector<std::string> GetAllSchemaTypes() const;
    
    // Validation
    bool ValidateContent(const std::string& typeName, const SimpleJson& data,
                        std::vector<std::string>& errors) const;
    
    // Documentation
    void GenerateAllDocumentation(const std::string& outputDir) const;
    
private:
    ContentSchemaRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<ContentSchema>> schemas_;
};

// Helper macros for schema registration
#define BEGIN_CONTENT_SCHEMA(TypeName, Description) \
    NovaEngine::SchemaBuilder(TypeName).Description(Description)

#define SCHEMA_FIELD(Name, Type) \
    .Field(Name, NovaEngine::SchemaFieldType::Type)

#define SCHEMA_REQUIRED() \
    .Required(true)

#define SCHEMA_NULLABLE() \
    .Nullable(true)

#define SCHEMA_DEFAULT(Value) \
    .Default(Value)

#define SCHEMA_MIN(Value) \
    .Min(Value)

#define SCHEMA_MAX(Value) \
    .Max(Value)

#define SCHEMA_ENUM(...) \
    .Enum({__VA_ARGS__})

#define SCHEMA_REFERENCE(RefType) \
    .Reference(RefType)

#define SCHEMA_DESC(Desc) \
    .Description(Desc)

#define END_SCHEMA_FIELD() \
    .EndField()

#define SCHEMA_INHERITS(BaseType) \
    .Inherits(BaseType)

#define REGISTER_SCHEMA(Builder) \
    NovaEngine::ContentSchemaRegistry::Instance().RegisterSchema(Builder.Build())

} // namespace NovaEngine
