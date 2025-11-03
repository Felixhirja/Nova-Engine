#pragma once

#include "SimpleJson.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <filesystem>
#include <chrono>
#include <typeindex>
#include <any>

namespace nova::config {

// ============================================================================
// CONFIGURATION ARCHITECTURE - Core Type System
// ============================================================================

/**
 * ConfigValueType - Strongly-typed configuration value types
 */
enum class ConfigValueType {
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Array,
    Object,
    Vector2,
    Vector3,
    Color,
    Path
};

/**
 * ConfigValue - Type-safe configuration value wrapper
 */
class ConfigValue {
public:
    ConfigValue();
    explicit ConfigValue(bool value);
    explicit ConfigValue(int value);
    explicit ConfigValue(double value);
    explicit ConfigValue(const std::string& value);
    explicit ConfigValue(const simplejson::JsonValue& jsonValue);

    ConfigValueType GetType() const { return type_; }
    
    bool AsBool(bool defaultValue = false) const;
    int AsInt(int defaultValue = 0) const;
    double AsFloat(double defaultValue = 0.0) const;
    std::string AsString(const std::string& defaultValue = "") const;
    
    bool IsValid() const { return valid_; }
    std::string GetErrorMessage() const { return errorMessage_; }

private:
    ConfigValueType type_;
    std::any value_;
    bool valid_ = true;
    std::string errorMessage_;
};

// ============================================================================
// SCHEMA SYSTEM - Configuration Schema Definition & Validation
// ============================================================================

/**
 * ConfigSchema - Defines structure and validation rules for configurations
 */
class ConfigSchema {
public:
    struct Field {
        std::string name;
        ConfigValueType type;
        bool required = false;
        std::any defaultValue;
        std::string description;
        
        // Validation constraints
        std::optional<double> minValue;
        std::optional<double> maxValue;
        std::optional<size_t> minLength;
        std::optional<size_t> maxLength;
        std::vector<std::string> allowedValues;
        std::function<bool(const ConfigValue&)> customValidator;
        std::string validationError;
    };

    ConfigSchema() = default;
    ConfigSchema(const std::string& name, const std::string& version = "1.0");

    ConfigSchema& AddField(const Field& field);
    ConfigSchema& AddField(const std::string& name, ConfigValueType type, bool required = false);
    ConfigSchema& AddDescription(const std::string& description);
    ConfigSchema& SetVersion(const std::string& version);

    const std::string& GetName() const { return name_; }
    const std::string& GetVersion() const { return version_; }
    const std::vector<Field>& GetFields() const { return fields_; }
    const Field* GetField(const std::string& name) const;

private:
    std::string name_;
    std::string version_;
    std::string description_;
    std::vector<Field> fields_;
    std::unordered_map<std::string, size_t> fieldIndex_;
};

// ============================================================================
// SCHEMA REGISTRY - Central Schema Management
// ============================================================================

/**
 * ConfigSchemaRegistry - Global registry for all configuration schemas
 * 
 * Provides centralized schema management with:
 * - Automatic schema discovery
 * - Version management
 * - Schema validation
 * - Documentation generation
 */
class ConfigSchemaRegistry {
public:
    static ConfigSchemaRegistry& GetInstance();

    bool RegisterSchema(const std::string& typeName, const ConfigSchema& schema);
    const ConfigSchema* GetSchema(const std::string& typeName) const;
    std::vector<std::string> GetRegisteredTypes() const;
    
    bool HasSchema(const std::string& typeName) const;
    void UnregisterSchema(const std::string& typeName);
    void Clear();

    // Schema metadata
    struct SchemaInfo {
        std::string typeName;
        std::string version;
        std::string description;
        size_t fieldCount;
        std::chrono::system_clock::time_point registeredAt;
    };
    
    std::vector<SchemaInfo> GetSchemaInfo() const;

private:
    ConfigSchemaRegistry() = default;
    std::unordered_map<std::string, ConfigSchema> schemas_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> registrationTimes_;
};

// ============================================================================
// VALIDATION ENGINE - Comprehensive Configuration Validation
// ============================================================================

/**
 * ValidationResult - Result of configuration validation
 */
struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    void AddError(const std::string& message);
    void AddWarning(const std::string& message);
    std::string GetSummary() const;
};

/**
 * ConfigValidator - Validates configurations against schemas
 */
class ConfigValidator {
public:
    explicit ConfigValidator(const ConfigSchema& schema);

    ValidationResult Validate(const simplejson::JsonObject& config) const;
    ValidationResult ValidateField(const std::string& fieldName, const ConfigValue& value) const;
    
    void SetStrictMode(bool strict) { strictMode_ = strict; }
    bool IsStrictMode() const { return strictMode_; }

private:
    const ConfigSchema& schema_;
    bool strictMode_ = false;

    bool ValidateType(const ConfigValue& value, ConfigValueType expectedType) const;
    bool ValidateConstraints(const ConfigSchema::Field& field, const ConfigValue& value,
                           ValidationResult& result) const;
};

// ============================================================================
// HOT RELOAD SYSTEM - Real-time Configuration Reloading
// ============================================================================

/**
 * ConfigWatcher - Monitors configuration files for changes
 */
class ConfigWatcher {
public:
    using ReloadCallback = std::function<void(const std::string& path)>;

    ConfigWatcher();
    
    void Watch(const std::string& path, ReloadCallback callback);
    void Unwatch(const std::string& path);
    void CheckForChanges();
    
    void SetPollInterval(std::chrono::milliseconds interval) { pollInterval_ = interval; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    struct WatchEntry {
        std::filesystem::path path;
        std::filesystem::file_time_type lastModified;
        ReloadCallback callback;
    };

    std::unordered_map<std::string, WatchEntry> watchedFiles_;
    std::chrono::milliseconds pollInterval_{1000};
    std::chrono::steady_clock::time_point lastCheck_;
    bool enabled_ = true;
};

// ============================================================================
// CONFIGURATION CONTAINER - Type-safe Configuration Storage
// ============================================================================

/**
 * Configuration - Type-safe configuration container
 */
class Configuration {
public:
    Configuration(const std::string& name, const ConfigSchema* schema = nullptr);

    bool Load(const std::string& filePath);
    bool LoadFromJson(const simplejson::JsonObject& json);
    bool Save(const std::string& filePath) const;

    template<typename T>
    T Get(const std::string& key, const T& defaultValue = T()) const;
    
    bool Has(const std::string& key) const;
    void Set(const std::string& key, const ConfigValue& value);
    
    ValidationResult Validate() const;
    const ConfigSchema* GetSchema() const { return schema_; }
    const std::string& GetName() const { return name_; }
    
    const simplejson::JsonObject& GetData() const { return data_; }

private:
    std::string name_;
    const ConfigSchema* schema_;
    simplejson::JsonObject data_;
    std::string filePath_;
};

// ============================================================================
// MIGRATION SYSTEM - Configuration Version Migration
// ============================================================================

/**
 * ConfigMigration - Handles migration between configuration versions
 */
class ConfigMigration {
public:
    using MigrationFunction = std::function<bool(simplejson::JsonObject&)>;

    ConfigMigration(const std::string& fromVersion, const std::string& toVersion);

    void AddStep(const std::string& description, MigrationFunction function);
    bool Migrate(simplejson::JsonObject& config) const;
    
    const std::string& GetFromVersion() const { return fromVersion_; }
    const std::string& GetToVersion() const { return toVersion_; }

private:
    struct MigrationStep {
        std::string description;
        MigrationFunction function;
    };

    std::string fromVersion_;
    std::string toVersion_;
    std::vector<MigrationStep> steps_;
};

/**
 * ConfigMigrationManager - Manages configuration migrations
 */
class ConfigMigrationManager {
public:
    static ConfigMigrationManager& GetInstance();

    void RegisterMigration(const std::string& typeName, const ConfigMigration& migration);
    bool Migrate(const std::string& typeName, simplejson::JsonObject& config,
                const std::string& fromVersion, const std::string& toVersion);
    
    std::vector<std::string> GetAvailableMigrations(const std::string& typeName) const;

private:
    ConfigMigrationManager() = default;
    std::unordered_map<std::string, std::vector<ConfigMigration>> migrations_;
};

// ============================================================================
// ANALYTICS SYSTEM - Configuration Usage Tracking
// ============================================================================

/**
 * ConfigAnalytics - Tracks configuration usage and performance
 */
class ConfigAnalytics {
public:
    struct UsageStats {
        size_t loadCount = 0;
        size_t accessCount = 0;
        size_t validationFailures = 0;
        std::chrono::milliseconds totalLoadTime{0};
        std::chrono::milliseconds averageLoadTime{0};
        std::chrono::system_clock::time_point lastAccessed;
        std::unordered_map<std::string, size_t> fieldAccessCounts;
    };

    static ConfigAnalytics& GetInstance();

    void RecordLoad(const std::string& configName, std::chrono::milliseconds duration);
    void RecordAccess(const std::string& configName, const std::string& fieldName = "");
    void RecordValidationFailure(const std::string& configName);
    
    UsageStats GetStats(const std::string& configName) const;
    std::vector<std::pair<std::string, UsageStats>> GetAllStats() const;
    
    void Reset();
    void EnableTracking(bool enable) { trackingEnabled_ = enable; }
    bool IsTrackingEnabled() const { return trackingEnabled_; }

private:
    ConfigAnalytics() = default;
    std::unordered_map<std::string, UsageStats> stats_;
    bool trackingEnabled_ = true;
};

// ============================================================================
// UNIFIED CONFIG SYSTEM - High-Level Configuration Management
// ============================================================================

/**
 * ConfigSystem - Unified configuration system
 * 
 * Central hub integrating all configuration subsystems:
 * - Schema registry
 * - Validation engine
 * - Hot reload system
 * - Migration support
 * - Analytics tracking
 */
class ConfigSystem {
public:
    static ConfigSystem& GetInstance();

    bool Initialize();
    void Shutdown();

    // Configuration loading with full pipeline
    std::shared_ptr<Configuration> LoadConfig(const std::string& typeName, 
                                             const std::string& filePath);
    bool ReloadConfig(const std::string& typeName);
    void ReloadAll();

    // Schema management
    bool RegisterSchema(const std::string& typeName, const ConfigSchema& schema);
    const ConfigSchema* GetSchema(const std::string& typeName) const;

    // Hot reload support
    void EnableHotReload(bool enable);
    void CheckHotReload();

    // Migration support
    void RegisterMigration(const std::string& typeName, const ConfigMigration& migration);

    // Analytics
    ConfigAnalytics::UsageStats GetStats(const std::string& typeName) const;
    void ResetAnalytics();

    // Configuration access
    std::shared_ptr<Configuration> GetConfig(const std::string& typeName) const;
    bool HasConfig(const std::string& typeName) const;

private:
    ConfigSystem() = default;
    
    std::unordered_map<std::string, std::shared_ptr<Configuration>> configs_;
    std::unordered_map<std::string, std::string> configPaths_;
    std::unique_ptr<ConfigWatcher> watcher_;
    bool initialized_ = false;
    bool hotReloadEnabled_ = false;
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

template<typename T>
T Configuration::Get(const std::string& key, const T& defaultValue) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return defaultValue;
    }

    // Record analytics
    ConfigAnalytics::GetInstance().RecordAccess(name_, key);

    // Type conversion based on T
    if constexpr (std::is_same_v<T, bool>) {
        return it->second.AsBoolean(defaultValue);
    } else if constexpr (std::is_same_v<T, int>) {
        return static_cast<int>(it->second.AsNumber(defaultValue));
    } else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
        return static_cast<T>(it->second.AsNumber(defaultValue));
    } else if constexpr (std::is_same_v<T, std::string>) {
        return it->second.AsString(defaultValue);
    }
    
    return defaultValue;
}

} // namespace nova::config
