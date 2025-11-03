#include "ConfigSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nova::config {

// ============================================================================
// ConfigValue Implementation
// ============================================================================

ConfigValue::ConfigValue() : type_(ConfigValueType::Null), valid_(true) {}

ConfigValue::ConfigValue(bool value) 
    : type_(ConfigValueType::Boolean), value_(value), valid_(true) {}

ConfigValue::ConfigValue(int value) 
    : type_(ConfigValueType::Integer), value_(value), valid_(true) {}

ConfigValue::ConfigValue(double value) 
    : type_(ConfigValueType::Float), value_(value), valid_(true) {}

ConfigValue::ConfigValue(const std::string& value) 
    : type_(ConfigValueType::String), value_(value), valid_(true) {}

ConfigValue::ConfigValue(const simplejson::JsonValue& jsonValue) : valid_(true) {
    switch (jsonValue.type()) {
        case simplejson::JsonValue::Type::Null:
            type_ = ConfigValueType::Null;
            break;
        case simplejson::JsonValue::Type::Boolean:
            type_ = ConfigValueType::Boolean;
            value_ = jsonValue.AsBoolean();
            break;
        case simplejson::JsonValue::Type::Number:
            type_ = ConfigValueType::Float;
            value_ = jsonValue.AsNumber();
            break;
        case simplejson::JsonValue::Type::String:
            type_ = ConfigValueType::String;
            value_ = jsonValue.AsString();
            break;
        default:
            type_ = ConfigValueType::Null;
            valid_ = false;
            errorMessage_ = "Unsupported JSON value type";
            break;
    }
}

bool ConfigValue::AsBool(bool defaultValue) const {
    if (type_ == ConfigValueType::Boolean && std::any_cast<bool>(&value_)) {
        return std::any_cast<bool>(value_);
    }
    return defaultValue;
}

int ConfigValue::AsInt(int defaultValue) const {
    if (type_ == ConfigValueType::Integer && std::any_cast<int>(&value_)) {
        return std::any_cast<int>(value_);
    }
    if (type_ == ConfigValueType::Float && std::any_cast<double>(&value_)) {
        return static_cast<int>(std::any_cast<double>(value_));
    }
    return defaultValue;
}

double ConfigValue::AsFloat(double defaultValue) const {
    if (type_ == ConfigValueType::Float && std::any_cast<double>(&value_)) {
        return std::any_cast<double>(value_);
    }
    if (type_ == ConfigValueType::Integer && std::any_cast<int>(&value_)) {
        return static_cast<double>(std::any_cast<int>(value_));
    }
    return defaultValue;
}

std::string ConfigValue::AsString(const std::string& defaultValue) const {
    if (type_ == ConfigValueType::String && std::any_cast<std::string>(&value_)) {
        return std::any_cast<std::string>(value_);
    }
    return defaultValue;
}

// ============================================================================
// ConfigSchema Implementation
// ============================================================================

ConfigSchema::ConfigSchema(const std::string& name, const std::string& version)
    : name_(name), version_(version) {}

ConfigSchema& ConfigSchema::AddField(const Field& field) {
    fieldIndex_[field.name] = fields_.size();
    fields_.push_back(field);
    return *this;
}

ConfigSchema& ConfigSchema::AddField(const std::string& name, ConfigValueType type, bool required) {
    Field field;
    field.name = name;
    field.type = type;
    field.required = required;
    return AddField(field);
}

ConfigSchema& ConfigSchema::AddDescription(const std::string& description) {
    description_ = description;
    return *this;
}

ConfigSchema& ConfigSchema::SetVersion(const std::string& version) {
    version_ = version;
    return *this;
}

const ConfigSchema::Field* ConfigSchema::GetField(const std::string& name) const {
    auto it = fieldIndex_.find(name);
    if (it != fieldIndex_.end()) {
        return &fields_[it->second];
    }
    return nullptr;
}

// ============================================================================
// ConfigSchemaRegistry Implementation
// ============================================================================

ConfigSchemaRegistry& ConfigSchemaRegistry::GetInstance() {
    static ConfigSchemaRegistry instance;
    return instance;
}

bool ConfigSchemaRegistry::RegisterSchema(const std::string& typeName, const ConfigSchema& schema) {
    if (schemas_.find(typeName) != schemas_.end()) {
        std::cerr << "Warning: Schema '" << typeName << "' already registered. Overwriting.\n";
    }
    
    schemas_[typeName] = schema;
    registrationTimes_[typeName] = std::chrono::system_clock::now();
    return true;
}

const ConfigSchema* ConfigSchemaRegistry::GetSchema(const std::string& typeName) const {
    auto it = schemas_.find(typeName);
    return (it != schemas_.end()) ? &it->second : nullptr;
}

std::vector<std::string> ConfigSchemaRegistry::GetRegisteredTypes() const {
    std::vector<std::string> types;
    types.reserve(schemas_.size());
    for (const auto& [type, _] : schemas_) {
        types.push_back(type);
    }
    return types;
}

bool ConfigSchemaRegistry::HasSchema(const std::string& typeName) const {
    return schemas_.find(typeName) != schemas_.end();
}

void ConfigSchemaRegistry::UnregisterSchema(const std::string& typeName) {
    schemas_.erase(typeName);
    registrationTimes_.erase(typeName);
}

void ConfigSchemaRegistry::Clear() {
    schemas_.clear();
    registrationTimes_.clear();
}

std::vector<ConfigSchemaRegistry::SchemaInfo> ConfigSchemaRegistry::GetSchemaInfo() const {
    std::vector<SchemaInfo> info;
    info.reserve(schemas_.size());
    
    for (const auto& [typeName, schema] : schemas_) {
        SchemaInfo si;
        si.typeName = typeName;
        si.version = schema.GetVersion();
        si.fieldCount = schema.GetFields().size();
        
        auto timeIt = registrationTimes_.find(typeName);
        if (timeIt != registrationTimes_.end()) {
            si.registeredAt = timeIt->second;
        }
        
        info.push_back(si);
    }
    
    return info;
}

// ============================================================================
// ValidationResult Implementation
// ============================================================================

void ValidationResult::AddError(const std::string& message) {
    isValid = false;
    errors.push_back(message);
}

void ValidationResult::AddWarning(const std::string& message) {
    warnings.push_back(message);
}

std::string ValidationResult::GetSummary() const {
    std::stringstream ss;
    ss << "Validation " << (isValid ? "PASSED" : "FAILED") << "\n";
    
    if (!errors.empty()) {
        ss << "Errors (" << errors.size() << "):\n";
        for (const auto& error : errors) {
            ss << "  - " << error << "\n";
        }
    }
    
    if (!warnings.empty()) {
        ss << "Warnings (" << warnings.size() << "):\n";
        for (const auto& warning : warnings) {
            ss << "  - " << warning << "\n";
        }
    }
    
    return ss.str();
}

// ============================================================================
// ConfigValidator Implementation
// ============================================================================

ConfigValidator::ConfigValidator(const ConfigSchema& schema) : schema_(schema) {}

ValidationResult ConfigValidator::Validate(const simplejson::JsonObject& config) const {
    ValidationResult result;
    
    // Check required fields
    for (const auto& field : schema_.GetFields()) {
        auto it = config.find(field.name);
        
        if (it == config.end()) {
            if (field.required) {
                result.AddError("Required field '" + field.name + "' is missing");
            }
            continue;
        }
        
        ConfigValue value(it->second);
        if (!value.IsValid()) {
            result.AddError("Field '" + field.name + "' has invalid value: " + value.GetErrorMessage());
            continue;
        }
        
        // Validate type
        if (!ValidateType(value, field.type)) {
            result.AddError("Field '" + field.name + "' has incorrect type");
            continue;
        }
        
        // Validate constraints
        if (!ValidateConstraints(field, value, result)) {
            continue;
        }
    }
    
    // Check for unknown fields in strict mode
    if (strictMode_) {
        for (const auto& [key, _] : config) {
            if (!schema_.GetField(key)) {
                result.AddWarning("Unknown field '" + key + "' (not in schema)");
            }
        }
    }
    
    return result;
}

ValidationResult ConfigValidator::ValidateField(const std::string& fieldName, 
                                               const ConfigValue& value) const {
    ValidationResult result;
    
    const auto* field = schema_.GetField(fieldName);
    if (!field) {
        result.AddError("Field '" + fieldName + "' not found in schema");
        return result;
    }
    
    if (!ValidateType(value, field->type)) {
        result.AddError("Field '" + fieldName + "' has incorrect type");
        return result;
    }
    
    ValidateConstraints(*field, value, result);
    return result;
}

bool ConfigValidator::ValidateType(const ConfigValue& value, ConfigValueType expectedType) const {
    // Allow Float to match Integer (JSON doesn't distinguish)
    if (value.GetType() == ConfigValueType::Float && expectedType == ConfigValueType::Integer) {
        return true;
    }
    if (value.GetType() == ConfigValueType::Integer && expectedType == ConfigValueType::Float) {
        return true;
    }
    return value.GetType() == expectedType;
}

bool ConfigValidator::ValidateConstraints(const ConfigSchema::Field& field, 
                                         const ConfigValue& value,
                                         ValidationResult& result) const {
    // Min/max value validation
    if (field.minValue.has_value() && value.GetType() == ConfigValueType::Float) {
        if (value.AsFloat() < field.minValue.value()) {
            result.AddError("Field '" + field.name + "' value is below minimum");
            return false;
        }
    }
    
    if (field.maxValue.has_value() && value.GetType() == ConfigValueType::Float) {
        if (value.AsFloat() > field.maxValue.value()) {
            result.AddError("Field '" + field.name + "' value exceeds maximum");
            return false;
        }
    }
    
    // String length validation
    if (field.minLength.has_value() && value.GetType() == ConfigValueType::String) {
        if (value.AsString().length() < field.minLength.value()) {
            result.AddError("Field '" + field.name + "' string is too short");
            return false;
        }
    }
    
    if (field.maxLength.has_value() && value.GetType() == ConfigValueType::String) {
        if (value.AsString().length() > field.maxLength.value()) {
            result.AddError("Field '" + field.name + "' string is too long");
            return false;
        }
    }
    
    // Allowed values validation
    if (!field.allowedValues.empty() && value.GetType() == ConfigValueType::String) {
        const auto& str = value.AsString();
        if (std::find(field.allowedValues.begin(), field.allowedValues.end(), str) 
            == field.allowedValues.end()) {
            result.AddError("Field '" + field.name + "' has invalid value");
            return false;
        }
    }
    
    // Custom validator
    if (field.customValidator && !field.customValidator(value)) {
        result.AddError("Field '" + field.name + "' failed custom validation: " 
                       + field.validationError);
        return false;
    }
    
    return true;
}

// ============================================================================
// ConfigWatcher Implementation
// ============================================================================

ConfigWatcher::ConfigWatcher() : lastCheck_(std::chrono::steady_clock::now()) {}

void ConfigWatcher::Watch(const std::string& path, ReloadCallback callback) {
    std::filesystem::path fsPath(path);
    
    if (!std::filesystem::exists(fsPath)) {
        std::cerr << "Warning: Config file does not exist: " << path << "\n";
        return;
    }
    
    WatchEntry entry;
    entry.path = fsPath;
    entry.lastModified = std::filesystem::last_write_time(fsPath);
    entry.callback = callback;
    
    watchedFiles_[path] = entry;
}

void ConfigWatcher::Unwatch(const std::string& path) {
    watchedFiles_.erase(path);
}

void ConfigWatcher::CheckForChanges() {
    if (!enabled_) return;
    
    auto now = std::chrono::steady_clock::now();
    if (now - lastCheck_ < pollInterval_) {
        return;
    }
    
    lastCheck_ = now;
    
    for (auto& [path, entry] : watchedFiles_) {
        if (!std::filesystem::exists(entry.path)) {
            continue;
        }
        
        auto currentModTime = std::filesystem::last_write_time(entry.path);
        if (currentModTime != entry.lastModified) {
            entry.lastModified = currentModTime;
            if (entry.callback) {
                entry.callback(path);
            }
        }
    }
}

// ============================================================================
// Configuration Implementation
// ============================================================================

Configuration::Configuration(const std::string& name, const ConfigSchema* schema)
    : name_(name), schema_(schema) {}

bool Configuration::Load(const std::string& filePath) {
    auto startTime = std::chrono::steady_clock::now();
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filePath << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto parseResult = simplejson::Parse(content);
    if (!parseResult.success) {
        std::cerr << "Failed to parse config JSON: " << parseResult.errorMessage << "\n";
        return false;
    }
    
    if (!parseResult.value.IsObject()) {
        std::cerr << "Config root must be a JSON object\n";
        return false;
    }
    
    data_ = parseResult.value.AsObject();
    filePath_ = filePath;
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);
    ConfigAnalytics::GetInstance().RecordLoad(name_, duration);
    
    return true;
}

bool Configuration::LoadFromJson(const simplejson::JsonObject& json) {
    data_ = json;
    return true;
}

bool Configuration::Save(const std::string& filePath) const {
    // TODO: Implement JSON serialization
    std::cerr << "Configuration::Save not yet implemented\n";
    return false;
}

bool Configuration::Has(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void Configuration::Set(const std::string& key, const ConfigValue& value) {
    // TODO: Convert ConfigValue to JsonValue and store
}

ValidationResult Configuration::Validate() const {
    if (!schema_) {
        ValidationResult result;
        result.AddWarning("No schema available for validation");
        return result;
    }
    
    ConfigValidator validator(*schema_);
    auto result = validator.Validate(data_);
    
    if (!result.isValid) {
        ConfigAnalytics::GetInstance().RecordValidationFailure(name_);
    }
    
    return result;
}

// ============================================================================
// ConfigMigration Implementation
// ============================================================================

ConfigMigration::ConfigMigration(const std::string& fromVersion, const std::string& toVersion)
    : fromVersion_(fromVersion), toVersion_(toVersion) {}

void ConfigMigration::AddStep(const std::string& description, MigrationFunction function) {
    steps_.push_back({description, function});
}

bool ConfigMigration::Migrate(simplejson::JsonObject& config) const {
    for (const auto& step : steps_) {
        if (!step.function(config)) {
            std::cerr << "Migration step failed: " << step.description << "\n";
            return false;
        }
    }
    return true;
}

// ============================================================================
// ConfigMigrationManager Implementation
// ============================================================================

ConfigMigrationManager& ConfigMigrationManager::GetInstance() {
    static ConfigMigrationManager instance;
    return instance;
}

void ConfigMigrationManager::RegisterMigration(const std::string& typeName, 
                                              const ConfigMigration& migration) {
    migrations_[typeName].push_back(migration);
}

bool ConfigMigrationManager::Migrate(const std::string& typeName, 
                                    simplejson::JsonObject& config,
                                    const std::string& fromVersion, 
                                    const std::string& toVersion) {
    auto it = migrations_.find(typeName);
    if (it == migrations_.end()) {
        return false;
    }
    
    for (const auto& migration : it->second) {
        if (migration.GetFromVersion() == fromVersion && 
            migration.GetToVersion() == toVersion) {
            return migration.Migrate(config);
        }
    }
    
    return false;
}

std::vector<std::string> ConfigMigrationManager::GetAvailableMigrations(
    const std::string& typeName) const {
    std::vector<std::string> migrations;
    
    auto it = migrations_.find(typeName);
    if (it != migrations_.end()) {
        for (const auto& migration : it->second) {
            migrations.push_back(migration.GetFromVersion() + " -> " + migration.GetToVersion());
        }
    }
    
    return migrations;
}

// ============================================================================
// ConfigAnalytics Implementation
// ============================================================================

ConfigAnalytics& ConfigAnalytics::GetInstance() {
    static ConfigAnalytics instance;
    return instance;
}

void ConfigAnalytics::RecordLoad(const std::string& configName, 
                                std::chrono::milliseconds duration) {
    if (!trackingEnabled_) return;
    
    auto& stats = stats_[configName];
    stats.loadCount++;
    stats.totalLoadTime += duration;
    stats.averageLoadTime = stats.totalLoadTime / stats.loadCount;
    stats.lastAccessed = std::chrono::system_clock::now();
}

void ConfigAnalytics::RecordAccess(const std::string& configName, const std::string& fieldName) {
    if (!trackingEnabled_) return;
    
    auto& stats = stats_[configName];
    stats.accessCount++;
    stats.lastAccessed = std::chrono::system_clock::now();
    
    if (!fieldName.empty()) {
        stats.fieldAccessCounts[fieldName]++;
    }
}

void ConfigAnalytics::RecordValidationFailure(const std::string& configName) {
    if (!trackingEnabled_) return;
    
    auto& stats = stats_[configName];
    stats.validationFailures++;
}

ConfigAnalytics::UsageStats ConfigAnalytics::GetStats(const std::string& configName) const {
    auto it = stats_.find(configName);
    return (it != stats_.end()) ? it->second : UsageStats();
}

std::vector<std::pair<std::string, ConfigAnalytics::UsageStats>> 
ConfigAnalytics::GetAllStats() const {
    return std::vector<std::pair<std::string, UsageStats>>(stats_.begin(), stats_.end());
}

void ConfigAnalytics::Reset() {
    stats_.clear();
}

// ============================================================================
// ConfigSystem Implementation
// ============================================================================

ConfigSystem& ConfigSystem::GetInstance() {
    static ConfigSystem instance;
    return instance;
}

bool ConfigSystem::Initialize() {
    if (initialized_) {
        return true;
    }
    
    watcher_ = std::make_unique<ConfigWatcher>();
    initialized_ = true;
    
    return true;
}

void ConfigSystem::Shutdown() {
    configs_.clear();
    configPaths_.clear();
    watcher_.reset();
    initialized_ = false;
}

std::shared_ptr<Configuration> ConfigSystem::LoadConfig(const std::string& typeName, 
                                                       const std::string& filePath) {
    auto& registry = ConfigSchemaRegistry::GetInstance();
    const ConfigSchema* schema = registry.GetSchema(typeName);
    
    auto config = std::make_shared<Configuration>(typeName, schema);
    
    if (!config->Load(filePath)) {
        return nullptr;
    }
    
    // Validate configuration
    if (schema) {
        auto validation = config->Validate();
        if (!validation.isValid) {
            std::cerr << "Configuration validation failed for " << typeName << ":\n";
            std::cerr << validation.GetSummary();
        }
    }
    
    configs_[typeName] = config;
    configPaths_[typeName] = filePath;
    
    // Setup hot reload if enabled
    if (hotReloadEnabled_) {
        watcher_->Watch(filePath, [this, typeName](const std::string&) {
            ReloadConfig(typeName);
        });
    }
    
    return config;
}

bool ConfigSystem::ReloadConfig(const std::string& typeName) {
    auto pathIt = configPaths_.find(typeName);
    if (pathIt == configPaths_.end()) {
        return false;
    }
    
    std::cout << "Reloading config: " << typeName << "\n";
    return LoadConfig(typeName, pathIt->second) != nullptr;
}

void ConfigSystem::ReloadAll() {
    std::vector<std::string> types;
    for (const auto& [type, _] : configPaths_) {
        types.push_back(type);
    }
    
    for (const auto& type : types) {
        ReloadConfig(type);
    }
}

bool ConfigSystem::RegisterSchema(const std::string& typeName, const ConfigSchema& schema) {
    return ConfigSchemaRegistry::GetInstance().RegisterSchema(typeName, schema);
}

const ConfigSchema* ConfigSystem::GetSchema(const std::string& typeName) const {
    return ConfigSchemaRegistry::GetInstance().GetSchema(typeName);
}

void ConfigSystem::EnableHotReload(bool enable) {
    hotReloadEnabled_ = enable;
    if (watcher_) {
        watcher_->SetEnabled(enable);
    }
}

void ConfigSystem::CheckHotReload() {
    if (watcher_ && hotReloadEnabled_) {
        watcher_->CheckForChanges();
    }
}

void ConfigSystem::RegisterMigration(const std::string& typeName, 
                                    const ConfigMigration& migration) {
    ConfigMigrationManager::GetInstance().RegisterMigration(typeName, migration);
}

ConfigAnalytics::UsageStats ConfigSystem::GetStats(const std::string& typeName) const {
    return ConfigAnalytics::GetInstance().GetStats(typeName);
}

void ConfigSystem::ResetAnalytics() {
    ConfigAnalytics::GetInstance().Reset();
}

std::shared_ptr<Configuration> ConfigSystem::GetConfig(const std::string& typeName) const {
    auto it = configs_.find(typeName);
    return (it != configs_.end()) ? it->second : nullptr;
}

bool ConfigSystem::HasConfig(const std::string& typeName) const {
    return configs_.find(typeName) != configs_.end();
}

} // namespace nova::config
