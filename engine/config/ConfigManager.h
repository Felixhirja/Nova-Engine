#pragma once

#include "../SimpleJson.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>
#include <functional>
#include <chrono>

namespace NovaEngine {
namespace Config {

// =====================================================
// CONFIGURATION VALIDATION
// =====================================================

struct ValidationError {
    std::string path;
    std::string message;
    std::string schemaRule;
    int severity = 1; // 1=error, 2=warning, 3=info
};

struct ValidationResult {
    bool valid = true;
    std::vector<ValidationError> errors;
    std::vector<ValidationError> warnings;
    
    bool HasErrors() const { return !errors.empty(); }
    bool HasWarnings() const { return !warnings.empty(); }
};

class ConfigValidator {
public:
    ValidationResult Validate(const simplejson::JsonValue& config, const std::string& schemaPath);
    ValidationResult ValidateFile(const std::string& configPath, const std::string& schemaPath);
    
    void RegisterCustomValidator(const std::string& keyword, 
        std::function<bool(const simplejson::JsonValue&)> validator);
    
    static std::string GetDefaultSchemaPath(const std::string& configType);
    
private:
    std::unordered_map<std::string, std::function<bool(const simplejson::JsonValue&)>> customValidators_;
    
    bool ValidateType(const simplejson::JsonValue& value, const std::string& expectedType);
    bool ValidateRequired(const simplejson::JsonObject& obj, const std::vector<std::string>& required);
    bool ValidateRange(const simplejson::JsonValue& value, double min, double max);
};

// =====================================================
// CONFIGURATION VERSIONING
// =====================================================

struct ConfigVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    
    std::string ToString() const;
    bool IsCompatible(const ConfigVersion& other) const;
    
    static ConfigVersion FromString(const std::string& versionStr);
};

class ConfigVersionManager {
public:
    struct Migration {
        ConfigVersion fromVersion;
        ConfigVersion toVersion;
        std::function<simplejson::JsonValue(const simplejson::JsonValue&)> transform;
        std::string description;
    };
    
    void RegisterMigration(const std::string& configType, const Migration& migration);
    
    bool NeedsMigration(const simplejson::JsonValue& config, const std::string& configType) const;
    
    simplejson::JsonValue Migrate(const simplejson::JsonValue& config, 
                                   const std::string& configType,
                                   const ConfigVersion& targetVersion);
    
    ConfigVersion GetCurrentVersion(const std::string& configType) const;
    
private:
    std::unordered_map<std::string, std::vector<Migration>> migrations_;
    std::unordered_map<std::string, ConfigVersion> currentVersions_;
};

// =====================================================
// CONFIGURATION INHERITANCE
// =====================================================

enum class MergeMode {
    Replace,    // Override completely replaces base value
    Merge,      // Objects merged, arrays replaced
    Append,     // Arrays appended, objects merged
    Prepend     // Arrays prepended, objects merged
};

struct InheritanceOptions {
    bool allowMultipleInheritance = false;
    bool deepMerge = true;
    MergeMode defaultMergeMode = MergeMode::Merge;
    int maxInheritanceDepth = 10;
};

class ConfigInheritance {
public:
    simplejson::JsonValue LoadWithInheritance(
        const std::string& configPath,
        const InheritanceOptions& options = InheritanceOptions()
    );
    
    simplejson::JsonValue ResolveInheritance(
        const simplejson::JsonValue& config,
        const InheritanceOptions& options = InheritanceOptions()
    );
    
    std::vector<std::string> GetInheritanceChain(const std::string& configPath);
    
    ValidationResult ValidateInheritance(const std::string& configPath);
    
private:
    simplejson::JsonValue MergeConfigs(
        const simplejson::JsonValue& base,
        const simplejson::JsonValue& override,
        MergeMode mode
    );
    
    bool DetectCircularReference(const std::string& configPath, 
                                  std::vector<std::string>& chain);
};

// =====================================================
// CONFIGURATION TEMPLATES
// =====================================================

class ConfigTemplate {
public:
    struct TemplateParameter {
        std::string name;
        std::string type;
        simplejson::JsonValue defaultValue;
        bool required = false;
        std::string description;
    };
    
    static simplejson::JsonValue InstantiateTemplate(
        const std::string& templatePath,
        const std::unordered_map<std::string, simplejson::JsonValue>& parameters
    );
    
    static std::vector<std::string> GetAvailableTemplates(const std::string& category = "");
    
    static std::vector<TemplateParameter> GetTemplateParameters(const std::string& templatePath);
    
private:
    static simplejson::JsonValue ReplaceParameters(
        const simplejson::JsonValue& template_,
        const std::unordered_map<std::string, simplejson::JsonValue>& parameters
    );
};

// =====================================================
// CONFIGURATION OVERRIDES
// =====================================================

enum class OverrideScope {
    Global,     // System-wide override
    Session,    // Current session only
    Debug,      // Debug/development only
    User        // User-specific override
};

struct ConfigOverride {
    std::string path;
    simplejson::JsonValue value;
    OverrideScope scope;
    int priority = 0;
    std::chrono::system_clock::time_point expires;
    std::string reason;
};

class ConfigOverrideManager {
public:
    void AddOverride(const ConfigOverride& override);
    void RemoveOverride(const std::string& path, OverrideScope scope);
    void ClearOverrides(OverrideScope scope);
    
    simplejson::JsonValue ApplyOverrides(
        const simplejson::JsonValue& config,
        const std::string& configPath
    );
    
    std::vector<ConfigOverride> GetActiveOverrides() const;
    
private:
    std::vector<ConfigOverride> overrides_;
    
    void CleanExpiredOverrides();
    simplejson::JsonValue ApplyOverride(
        const simplejson::JsonValue& config,
        const ConfigOverride& override
    );
};

// =====================================================
// CONFIGURATION SECURITY
// =====================================================

class ConfigSecurity {
public:
    struct SecurityOptions {
        bool validateSignatures = false;
        bool encryptSensitive = false;
        bool sanitizeInput = true;
        std::string encryptionKey;
    };
    
    static ValidationResult ValidateSecurity(
        const simplejson::JsonValue& config,
        const SecurityOptions& options
    );
    
    static simplejson::JsonValue EncryptSensitiveFields(
        const simplejson::JsonValue& config,
        const std::vector<std::string>& sensitiveFields,
        const std::string& key
    );
    
    static simplejson::JsonValue DecryptSensitiveFields(
        const simplejson::JsonValue& config,
        const std::string& key
    );
    
    static bool ValidateConfigSignature(
        const std::string& configPath,
        const std::string& signaturePath
    );
};

// =====================================================
// CONFIGURATION PERFORMANCE
// =====================================================

struct ConfigLoadStats {
    double loadTimeMs = 0.0;
    size_t memoryUsageBytes = 0;
    size_t diskReadBytes = 0;
    bool fromCache = false;
    std::chrono::system_clock::time_point loadTime;
};

enum class CachePolicy {
    NoCache,
    LRU,        // Least Recently Used
    MRU,        // Most Recently Used
    LFU,        // Least Frequently Used
    Unlimited
};

class ConfigCache {
public:
    struct CacheEntry {
        simplejson::JsonValue config;
        std::filesystem::file_time_type lastModified;
        size_t memoryUsage = 0;
        int accessCount = 0;
        std::chrono::steady_clock::time_point lastAccess;
    };
    
    void SetCachePolicy(CachePolicy policy, size_t maxSizeMB = 100);
    
    const simplejson::JsonValue* Get(const std::string& configPath);
    
    void Put(const std::string& configPath, const simplejson::JsonValue& config);
    
    void Preload(const std::vector<std::string>& configPaths);
    
    void Clear();
    
    void Remove(const std::string& configPath);
    
    struct CacheStats {
        size_t totalEntries = 0;
        size_t memoryUsageMB = 0;
        int hits = 0;
        int misses = 0;
        double hitRate = 0.0;
    };
    
    CacheStats GetStats() const;
    
private:
    std::unordered_map<std::string, CacheEntry> cache_;
    CachePolicy policy_ = CachePolicy::LRU;
    size_t maxSizeMB_ = 100;
    int hits_ = 0;
    int misses_ = 0;
    
    void EvictIfNeeded();
    std::string SelectEvictionCandidate();
};

// =====================================================
// CONFIGURATION ANALYTICS
// =====================================================

struct ConfigUsageStats {
    std::string configPath;
    int loadCount = 0;
    double totalLoadTimeMs = 0.0;
    double avgLoadTimeMs = 0.0;
    std::chrono::system_clock::time_point lastUsed;
    std::chrono::system_clock::time_point firstUsed;
    std::vector<std::string> usedBy;
};

class ConfigAnalytics {
public:
    void TrackLoad(const std::string& configPath, double loadTimeMs);
    
    void TrackUsage(const std::string& configPath, const std::string& usedBy);
    
    ConfigUsageStats GetStats(const std::string& configPath) const;
    
    std::vector<std::string> FindUnusedConfigs(int daysSinceLastUse = 30) const;
    
    std::vector<ConfigUsageStats> GetMostUsed(int limit = 10) const;
    
    std::vector<ConfigUsageStats> GetSlowestLoading(int limit = 10) const;
    
    void ExportReport(const std::string& outputPath) const;
    
    void Clear();
    
private:
    std::unordered_map<std::string, ConfigUsageStats> stats_;
};

// =====================================================
// MAIN CONFIGURATION MANAGER
// =====================================================

class ConfigManager {
public:
    static ConfigManager& GetInstance();
    
    struct LoadOptions {
        bool validateSchema = true;
        bool useCache = true;
        bool resolveInheritance = true;
        bool applyOverrides = true;
        bool trackAnalytics = true;
        InheritanceOptions inheritanceOptions;
    };
    
    // Initialize configuration system
    bool Initialize(const std::string& configRoot = "assets/config/");
    
    // Load configuration
    simplejson::JsonValue LoadConfig(
        const std::string& configPath
    );
    
    simplejson::JsonValue LoadConfig(
        const std::string& configPath,
        const LoadOptions& options
    );
    
    // Reload configuration
    void ReloadConfig(const std::string& configPath);
    
    // Validate configuration
    ValidationResult ValidateConfig(const std::string& configPath);
    
    // Get configuration with caching
    const simplejson::JsonValue* GetCachedConfig(const std::string& configPath);
    
    // Access subsystems
    ConfigValidator& GetValidator() { return validator_; }
    ConfigVersionManager& GetVersionManager() { return versionManager_; }
    ConfigInheritance& GetInheritanceSystem() { return inheritance_; }
    ConfigOverrideManager& GetOverrideManager() { return overrideManager_; }
    ConfigCache& GetCache() { return cache_; }
    ConfigAnalytics& GetAnalytics() { return analytics_; }
    
    // Statistics
    ConfigLoadStats GetLoadStats(const std::string& configPath) const;
    
    // Configuration discovery
    std::vector<std::string> DiscoverConfigs(const std::string& pattern = "*.json") const;
    
    // Bulk operations
    void ReloadAll();
    ValidationResult ValidateAll();
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    std::string configRoot_;
    bool initialized_ = false;
    
    ConfigValidator validator_;
    ConfigVersionManager versionManager_;
    ConfigInheritance inheritance_;
    ConfigOverrideManager overrideManager_;
    ConfigCache cache_;
    ConfigAnalytics analytics_;
    
    std::unordered_map<std::string, ConfigLoadStats> loadStats_;
};

} // namespace Config
} // namespace NovaEngine
