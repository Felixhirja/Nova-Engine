#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace NovaEngine {
namespace Config {

// =====================================================
// CONFIG VERSION
// =====================================================

std::string ConfigVersion::ToString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool ConfigVersion::IsCompatible(const ConfigVersion& other) const {
    return major == other.major;
}

ConfigVersion ConfigVersion::FromString(const std::string& versionStr) {
    ConfigVersion version;
    std::istringstream iss(versionStr);
    char dot;
    iss >> version.major >> dot >> version.minor >> dot >> version.patch;
    return version;
}

// =====================================================
// CONFIG VALIDATOR
// =====================================================

ValidationResult ConfigValidator::Validate(const simplejson::JsonValue& config, const std::string& schemaPath) {
    ValidationResult result;
    
    if (!std::filesystem::exists(schemaPath)) {
        result.valid = false;
        result.errors.push_back({"/", "Schema file not found: " + schemaPath, "file_not_found", 1});
        return result;
    }
    
    // Load schema
    std::ifstream schemaFile(schemaPath);
    if (!schemaFile.is_open()) {
        result.valid = false;
        result.errors.push_back({"/", "Could not open schema file", "file_read_error", 1});
        return result;
    }
    
    std::string schemaContent((std::istreambuf_iterator<char>(schemaFile)), std::istreambuf_iterator<char>());
    auto schemaResult = simplejson::Parse(schemaContent);
    
    if (!schemaResult.success) {
        result.valid = false;
        result.errors.push_back({"/", "Invalid schema JSON: " + schemaResult.errorMessage, "schema_parse_error", 1});
        return result;
    }
    
    // Basic validation logic (simplified)
    if (!config.IsObject()) {
        result.valid = false;
        result.errors.push_back({"/", "Config must be an object", "type_mismatch", 1});
        return result;
    }
    
    return result;
}

ValidationResult ConfigValidator::ValidateFile(const std::string& configPath, const std::string& schemaPath) {
    ValidationResult result;
    
    if (!std::filesystem::exists(configPath)) {
        result.valid = false;
        result.errors.push_back({"/", "Config file not found: " + configPath, "file_not_found", 1});
        return result;
    }
    
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        result.valid = false;
        result.errors.push_back({"/", "Could not open config file", "file_read_error", 1});
        return result;
    }
    
    std::string configContent((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
    auto configResult = simplejson::Parse(configContent);
    
    if (!configResult.success) {
        result.valid = false;
        result.errors.push_back({"/", "Invalid config JSON: " + configResult.errorMessage, "parse_error", 1});
        return result;
    }
    
    return Validate(configResult.value, schemaPath);
}

void ConfigValidator::RegisterCustomValidator(const std::string& keyword, 
    std::function<bool(const simplejson::JsonValue&)> validator) {
    customValidators_[keyword] = validator;
}

std::string ConfigValidator::GetDefaultSchemaPath(const std::string& configType) {
    return "assets/schemas/" + configType + ".schema.json";
}

bool ConfigValidator::ValidateType(const simplejson::JsonValue& value, const std::string& expectedType) {
    if (expectedType == "string") return value.IsString();
    if (expectedType == "number") return value.IsNumber();
    if (expectedType == "boolean") return value.IsBoolean();
    if (expectedType == "object") return value.IsObject();
    if (expectedType == "array") return value.IsArray();
    return false;
}

bool ConfigValidator::ValidateRequired(const simplejson::JsonObject& obj, const std::vector<std::string>& required) {
    for (const auto& key : required) {
        if (obj.find(key) == obj.end()) {
            return false;
        }
    }
    return true;
}

bool ConfigValidator::ValidateRange(const simplejson::JsonValue& value, double min, double max) {
    if (!value.IsNumber()) return false;
    double num = value.AsNumber();
    return num >= min && num <= max;
}

// =====================================================
// CONFIG VERSION MANAGER
// =====================================================

void ConfigVersionManager::RegisterMigration(const std::string& configType, const Migration& migration) {
    migrations_[configType].push_back(migration);
}

bool ConfigVersionManager::NeedsMigration(const simplejson::JsonValue& config, const std::string& configType) const {
    if (!config.IsObject()) return false;
    
    auto& obj = config.AsObject();
    auto versionIt = obj.find("$schema_version");
    if (versionIt == obj.end() || !versionIt->second.IsString()) {
        return true;
    }
    
    auto configVersion = ConfigVersion::FromString(versionIt->second.AsString());
    auto currentVersion = GetCurrentVersion(configType);
    
    return !configVersion.IsCompatible(currentVersion);
}

simplejson::JsonValue ConfigVersionManager::Migrate(const simplejson::JsonValue& config, 
                                                     const std::string& configType,
                                                     const ConfigVersion& targetVersion) {
    auto result = config;
    
    auto migrationsIt = migrations_.find(configType);
    if (migrationsIt == migrations_.end()) {
        return result;
    }
    
    for (const auto& migration : migrationsIt->second) {
        result = migration.transform(result);
    }
    
    return result;
}

ConfigVersion ConfigVersionManager::GetCurrentVersion(const std::string& configType) const {
    auto it = currentVersions_.find(configType);
    if (it != currentVersions_.end()) {
        return it->second;
    }
    return ConfigVersion{1, 0, 0};
}

// =====================================================
// CONFIG INHERITANCE
// =====================================================

simplejson::JsonValue ConfigInheritance::LoadWithInheritance(
    const std::string& configPath,
    const InheritanceOptions& options) {
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return simplejson::JsonValue();
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success) {
        return simplejson::JsonValue();
    }
    
    return ResolveInheritance(result.value, options);
}

simplejson::JsonValue ConfigInheritance::ResolveInheritance(
    const simplejson::JsonValue& config,
    const InheritanceOptions& options) {
    
    if (!config.IsObject()) {
        return config;
    }
    
    auto& obj = config.AsObject();
    auto extendsIt = obj.find("$extends");
    
    if (extendsIt == obj.end() || !extendsIt->second.IsString()) {
        return config;
    }
    
    std::string basePath = extendsIt->second.AsString();
    auto baseConfig = LoadWithInheritance(basePath, options);
    
    if (baseConfig.IsNull()) {
        return config;
    }
    
    return MergeConfigs(baseConfig, config, options.defaultMergeMode);
}

std::vector<std::string> ConfigInheritance::GetInheritanceChain(const std::string& configPath) {
    std::vector<std::string> chain;
    chain.push_back(configPath);
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return chain;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success || !result.value.IsObject()) {
        return chain;
    }
    
    auto& obj = result.value.AsObject();
    auto extendsIt = obj.find("$extends");
    
    if (extendsIt != obj.end() && extendsIt->second.IsString()) {
        std::string basePath = extendsIt->second.AsString();
        auto baseChain = GetInheritanceChain(basePath);
        chain.insert(chain.end(), baseChain.begin(), baseChain.end());
    }
    
    return chain;
}

ValidationResult ConfigInheritance::ValidateInheritance(const std::string& configPath) {
    ValidationResult result;
    
    std::vector<std::string> chain;
    if (DetectCircularReference(configPath, chain)) {
        result.valid = false;
        result.errors.push_back({
            "$extends",
            "Circular inheritance detected: " + configPath,
            "circular_reference",
            1
        });
    }
    
    return result;
}

simplejson::JsonValue ConfigInheritance::MergeConfigs(
    const simplejson::JsonValue& base,
    const simplejson::JsonValue& override,
    MergeMode mode) {
    
    if (!base.IsObject() || !override.IsObject()) {
        return override;
    }
    
    auto result = base;
    auto& resultObj = result.AsObject();
    auto& overrideObj = override.AsObject();
    
    for (const auto& [key, value] : overrideObj) {
        if (key.find("$") == 0) continue; // Skip meta keys
        resultObj[key] = value;
    }
    
    return result;
}

bool ConfigInheritance::DetectCircularReference(const std::string& configPath, 
                                                 std::vector<std::string>& chain) {
    if (std::find(chain.begin(), chain.end(), configPath) != chain.end()) {
        return true;
    }
    
    chain.push_back(configPath);
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success || !result.value.IsObject()) {
        return false;
    }
    
    auto& obj = result.value.AsObject();
    auto extendsIt = obj.find("$extends");
    
    if (extendsIt != obj.end() && extendsIt->second.IsString()) {
        std::string basePath = extendsIt->second.AsString();
        return DetectCircularReference(basePath, chain);
    }
    
    return false;
}

// =====================================================
// CONFIG TEMPLATE
// =====================================================

simplejson::JsonValue ConfigTemplate::InstantiateTemplate(
    const std::string& templatePath,
    const std::unordered_map<std::string, simplejson::JsonValue>& parameters) {
    
    std::ifstream file(templatePath);
    if (!file.is_open()) {
        return simplejson::JsonValue();
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success) {
        return simplejson::JsonValue();
    }
    
    return ReplaceParameters(result.value, parameters);
}

std::vector<std::string> ConfigTemplate::GetAvailableTemplates(const std::string& category) {
    std::vector<std::string> templates;
    
    std::string searchPath = category.empty() ? "assets/templates/" : "assets/templates/" + category + "/";
    
    if (!std::filesystem::exists(searchPath)) {
        return templates;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
        if (entry.path().extension() == ".json") {
            templates.push_back(entry.path().string());
        }
    }
    
    return templates;
}

std::vector<ConfigTemplate::TemplateParameter> ConfigTemplate::GetTemplateParameters(const std::string& templatePath) {
    std::vector<TemplateParameter> parameters;
    
    std::ifstream file(templatePath);
    if (!file.is_open()) {
        return parameters;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success || !result.value.IsObject()) {
        return parameters;
    }
    
    auto& obj = result.value.AsObject();
    auto templateIt = obj.find("$template");
    
    if (templateIt != obj.end() && templateIt->second.IsObject()) {
        auto& templateObj = templateIt->second.AsObject();
        auto paramsIt = templateObj.find("parameters");
        
        if (paramsIt != templateObj.end() && paramsIt->second.IsObject()) {
            // Extract parameters from template definition
        }
    }
    
    return parameters;
}

simplejson::JsonValue ConfigTemplate::ReplaceParameters(
    const simplejson::JsonValue& template_,
    const std::unordered_map<std::string, simplejson::JsonValue>& parameters) {
    
    // Simplified parameter replacement
    return template_;
}

// =====================================================
// CONFIG OVERRIDE MANAGER
// =====================================================

void ConfigOverrideManager::AddOverride(const ConfigOverride& override) {
    overrides_.push_back(override);
}

void ConfigOverrideManager::RemoveOverride(const std::string& path, OverrideScope scope) {
    overrides_.erase(
        std::remove_if(overrides_.begin(), overrides_.end(),
            [&](const ConfigOverride& o) {
                return o.path == path && o.scope == scope;
            }),
        overrides_.end()
    );
}

void ConfigOverrideManager::ClearOverrides(OverrideScope scope) {
    overrides_.erase(
        std::remove_if(overrides_.begin(), overrides_.end(),
            [&](const ConfigOverride& o) {
                return o.scope == scope;
            }),
        overrides_.end()
    );
}

simplejson::JsonValue ConfigOverrideManager::ApplyOverrides(
    const simplejson::JsonValue& config,
    const std::string& configPath) {
    
    CleanExpiredOverrides();
    
    auto result = config;
    
    std::vector<const ConfigOverride*> applicableOverrides;
    for (const auto& override : overrides_) {
        if (override.path == configPath || override.path == "*") {
            applicableOverrides.push_back(&override);
        }
    }
    
    std::sort(applicableOverrides.begin(), applicableOverrides.end(),
        [](const ConfigOverride* a, const ConfigOverride* b) {
            return a->priority < b->priority;
        });
    
    for (const auto* override : applicableOverrides) {
        result = ApplyOverride(result, *override);
    }
    
    return result;
}

std::vector<ConfigOverride> ConfigOverrideManager::GetActiveOverrides() const {
    return overrides_;
}

void ConfigOverrideManager::CleanExpiredOverrides() {
    auto now = std::chrono::system_clock::now();
    overrides_.erase(
        std::remove_if(overrides_.begin(), overrides_.end(),
            [&](const ConfigOverride& o) {
                return o.expires != std::chrono::system_clock::time_point() && o.expires < now;
            }),
        overrides_.end()
    );
}

simplejson::JsonValue ConfigOverrideManager::ApplyOverride(
    const simplejson::JsonValue& config,
    const ConfigOverride& override) {
    
    // Simplified override application
    return config;
}

// =====================================================
// CONFIG SECURITY
// =====================================================

ValidationResult ConfigSecurity::ValidateSecurity(
    const simplejson::JsonValue& config,
    const SecurityOptions& options) {
    
    ValidationResult result;
    
    if (options.sanitizeInput) {
        // Check for potentially dangerous values
    }
    
    return result;
}

simplejson::JsonValue ConfigSecurity::EncryptSensitiveFields(
    const simplejson::JsonValue& config,
    const std::vector<std::string>& sensitiveFields,
    const std::string& key) {
    
    // Placeholder for encryption logic
    return config;
}

simplejson::JsonValue ConfigSecurity::DecryptSensitiveFields(
    const simplejson::JsonValue& config,
    const std::string& key) {
    
    // Placeholder for decryption logic
    return config;
}

bool ConfigSecurity::ValidateConfigSignature(
    const std::string& configPath,
    const std::string& signaturePath) {
    
    // Placeholder for signature validation
    return true;
}

// =====================================================
// CONFIG CACHE
// =====================================================

void ConfigCache::SetCachePolicy(CachePolicy policy, size_t maxSizeMB) {
    policy_ = policy;
    maxSizeMB_ = maxSizeMB;
}

const simplejson::JsonValue* ConfigCache::Get(const std::string& configPath) {
    auto it = cache_.find(configPath);
    
    if (it != cache_.end()) {
        hits_++;
        it->second.accessCount++;
        it->second.lastAccess = std::chrono::steady_clock::now();
        return &it->second.config;
    }
    
    misses_++;
    return nullptr;
}

void ConfigCache::Put(const std::string& configPath, const simplejson::JsonValue& config) {
    EvictIfNeeded();
    
    CacheEntry entry;
    entry.config = config;
    entry.lastAccess = std::chrono::steady_clock::now();
    entry.accessCount = 1;
    
    if (std::filesystem::exists(configPath)) {
        entry.lastModified = std::filesystem::last_write_time(configPath);
    }
    
    cache_[configPath] = entry;
}

void ConfigCache::Preload(const std::vector<std::string>& configPaths) {
    for (const auto& path : configPaths) {
        if (cache_.find(path) == cache_.end()) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                auto result = simplejson::Parse(content);
                if (result.success) {
                    Put(path, result.value);
                }
            }
        }
    }
}

void ConfigCache::Clear() {
    cache_.clear();
    hits_ = 0;
    misses_ = 0;
}

void ConfigCache::Remove(const std::string& configPath) {
    cache_.erase(configPath);
}

ConfigCache::CacheStats ConfigCache::GetStats() const {
    CacheStats stats;
    stats.totalEntries = cache_.size();
    stats.hits = hits_;
    stats.misses = misses_;
    
    if (hits_ + misses_ > 0) {
        stats.hitRate = static_cast<double>(hits_) / (hits_ + misses_);
    }
    
    size_t totalMemory = 0;
    for (const auto& [path, entry] : cache_) {
        totalMemory += entry.memoryUsage;
    }
    stats.memoryUsageMB = totalMemory / (1024 * 1024);
    
    return stats;
}

void ConfigCache::EvictIfNeeded() {
    if (policy_ == CachePolicy::NoCache) {
        cache_.clear();
        return;
    }
    
    if (policy_ == CachePolicy::Unlimited) {
        return;
    }
    
    // Simplified eviction
    while (cache_.size() > 100) {
        std::string toEvict = SelectEvictionCandidate();
        cache_.erase(toEvict);
    }
}

std::string ConfigCache::SelectEvictionCandidate() {
    if (cache_.empty()) return "";
    
    std::string candidate = cache_.begin()->first;
    
    if (policy_ == CachePolicy::LRU) {
        auto oldest = std::chrono::steady_clock::now();
        for (const auto& [path, entry] : cache_) {
            if (entry.lastAccess < oldest) {
                oldest = entry.lastAccess;
                candidate = path;
            }
        }
    }
    
    return candidate;
}

// =====================================================
// CONFIG ANALYTICS
// =====================================================

void ConfigAnalytics::TrackLoad(const std::string& configPath, double loadTimeMs) {
    auto& stats = stats_[configPath];
    stats.configPath = configPath;
    stats.loadCount++;
    stats.totalLoadTimeMs += loadTimeMs;
    stats.avgLoadTimeMs = stats.totalLoadTimeMs / stats.loadCount;
    stats.lastUsed = std::chrono::system_clock::now();
    
    if (stats.loadCount == 1) {
        stats.firstUsed = stats.lastUsed;
    }
}

void ConfigAnalytics::TrackUsage(const std::string& configPath, const std::string& usedBy) {
    auto& stats = stats_[configPath];
    stats.configPath = configPath;
    
    if (std::find(stats.usedBy.begin(), stats.usedBy.end(), usedBy) == stats.usedBy.end()) {
        stats.usedBy.push_back(usedBy);
    }
}

ConfigUsageStats ConfigAnalytics::GetStats(const std::string& configPath) const {
    auto it = stats_.find(configPath);
    if (it != stats_.end()) {
        return it->second;
    }
    return ConfigUsageStats{};
}

std::vector<std::string> ConfigAnalytics::FindUnusedConfigs(int daysSinceLastUse) const {
    std::vector<std::string> unused;
    auto now = std::chrono::system_clock::now();
    auto threshold = std::chrono::hours(daysSinceLastUse * 24);
    
    for (const auto& [path, stats] : stats_) {
        if (stats.loadCount == 0 || (now - stats.lastUsed) > threshold) {
            unused.push_back(path);
        }
    }
    
    return unused;
}

std::vector<ConfigUsageStats> ConfigAnalytics::GetMostUsed(int limit) const {
    std::vector<ConfigUsageStats> result;
    
    for (const auto& [path, stats] : stats_) {
        result.push_back(stats);
    }
    
    std::sort(result.begin(), result.end(),
        [](const ConfigUsageStats& a, const ConfigUsageStats& b) {
            return a.loadCount > b.loadCount;
        });
    
    if (result.size() > static_cast<size_t>(limit)) {
        result.resize(limit);
    }
    
    return result;
}

std::vector<ConfigUsageStats> ConfigAnalytics::GetSlowestLoading(int limit) const {
    std::vector<ConfigUsageStats> result;
    
    for (const auto& [path, stats] : stats_) {
        result.push_back(stats);
    }
    
    std::sort(result.begin(), result.end(),
        [](const ConfigUsageStats& a, const ConfigUsageStats& b) {
            return a.avgLoadTimeMs > b.avgLoadTimeMs;
        });
    
    if (result.size() > static_cast<size_t>(limit)) {
        result.resize(limit);
    }
    
    return result;
}

void ConfigAnalytics::ExportReport(const std::string& outputPath) const {
    std::ofstream report(outputPath);
    
    if (!report.is_open()) {
        std::cerr << "Failed to open report file: " << outputPath << std::endl;
        return;
    }
    
    report << "Configuration Analytics Report\n";
    report << "==============================\n\n";
    
    report << "Total Configurations: " << stats_.size() << "\n\n";
    
    report << "Most Used Configurations:\n";
    auto mostUsed = GetMostUsed(10);
    for (const auto& stats : mostUsed) {
        report << "  " << stats.configPath << ": " << stats.loadCount << " loads\n";
    }
    
    report << "\nSlowest Loading Configurations:\n";
    auto slowest = GetSlowestLoading(10);
    for (const auto& stats : slowest) {
        report << "  " << stats.configPath << ": " << stats.avgLoadTimeMs << " ms avg\n";
    }
    
    report.close();
}

void ConfigAnalytics::Clear() {
    stats_.clear();
}

// =====================================================
// CONFIG MANAGER
// =====================================================

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::Initialize(const std::string& configRoot) {
    configRoot_ = configRoot;
    
    cache_.SetCachePolicy(CachePolicy::LRU, 100);
    
    initialized_ = true;
    return true;
}

simplejson::JsonValue ConfigManager::LoadConfig(const std::string& configPath) {
    return LoadConfig(configPath, LoadOptions{});
}

simplejson::JsonValue ConfigManager::LoadConfig(
    const std::string& configPath,
    const LoadOptions& options) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (options.useCache) {
        auto* cached = cache_.Get(configPath);
        if (cached) {
            auto endTime = std::chrono::high_resolution_clock::now();
            double loadTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            
            if (options.trackAnalytics) {
                analytics_.TrackLoad(configPath, loadTimeMs);
            }
            
            return *cached;
        }
    }
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return simplejson::JsonValue();
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto result = simplejson::Parse(content);
    
    if (!result.success) {
        return simplejson::JsonValue();
    }
    
    auto config = result.value;
    
    if (options.resolveInheritance) {
        config = inheritance_.ResolveInheritance(config, options.inheritanceOptions);
    }
    
    if (options.applyOverrides) {
        config = overrideManager_.ApplyOverrides(config, configPath);
    }
    
    if (options.useCache) {
        cache_.Put(configPath, config);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double loadTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (options.trackAnalytics) {
        analytics_.TrackLoad(configPath, loadTimeMs);
    }
    
    ConfigLoadStats stats;
    stats.loadTimeMs = loadTimeMs;
    stats.fromCache = false;
    stats.loadTime = std::chrono::system_clock::now();
    loadStats_[configPath] = stats;
    
    return config;
}

void ConfigManager::ReloadConfig(const std::string& configPath) {
    cache_.Remove(configPath);
    LoadConfig(configPath);
}

ValidationResult ConfigManager::ValidateConfig(const std::string& configPath) {
    std::string schemaPath = ConfigValidator::GetDefaultSchemaPath("actor");
    return validator_.ValidateFile(configPath, schemaPath);
}

const simplejson::JsonValue* ConfigManager::GetCachedConfig(const std::string& configPath) {
    return cache_.Get(configPath);
}

ConfigLoadStats ConfigManager::GetLoadStats(const std::string& configPath) const {
    auto it = loadStats_.find(configPath);
    if (it != loadStats_.end()) {
        return it->second;
    }
    return ConfigLoadStats{};
}

std::vector<std::string> ConfigManager::DiscoverConfigs(const std::string& pattern) const {
    std::vector<std::string> configs;
    
    if (!std::filesystem::exists(configRoot_)) {
        return configs;
    }
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(configRoot_)) {
        if (entry.path().extension() == ".json") {
            configs.push_back(entry.path().string());
        }
    }
    
    return configs;
}

void ConfigManager::ReloadAll() {
    cache_.Clear();
    
    auto configs = DiscoverConfigs();
    for (const auto& config : configs) {
        LoadConfig(config);
    }
}

ValidationResult ConfigManager::ValidateAll() {
    ValidationResult result;
    
    auto configs = DiscoverConfigs();
    for (const auto& config : configs) {
        auto configResult = ValidateConfig(config);
        result.errors.insert(result.errors.end(), configResult.errors.begin(), configResult.errors.end());
        result.warnings.insert(result.warnings.end(), configResult.warnings.begin(), configResult.warnings.end());
    }
    
    result.valid = result.errors.empty();
    return result;
}

} // namespace Config
} // namespace NovaEngine
