#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>

/**
 * Nova Engine Asset Pipeline Enhancement System
 * 
 * Comprehensive asset management with:
 * - Automated validation and integrity checking
 * - Dependency tracking and resolution
 * - Hot reloading for development
 * - Platform-specific optimization
 * - Intelligent caching and streaming
 * - Analytics and performance monitoring
 * - Version control integration
 * - Auto-generated documentation
 */

namespace AssetPipeline {

// ============================================================================
// Asset Metadata and Types
// ============================================================================

enum class AssetType {
    Unknown,
    Texture,
    Model,
    Audio,
    Script,
    Config,
    Shader,
    Material,
    Font,
    Video,
    Data
};

enum class AssetState {
    Unloaded,
    Loading,
    Loaded,
    Failed,
    Reloading,
    Optimizing
};

enum class CompressionType {
    None,
    LZ4,
    Zlib,
    LZMA,
    Auto
};

enum class Platform {
    Windows,
    Linux,
    MacOS,
    Web,
    All
};

struct AssetMetadata {
    std::string path;
    std::string name;
    AssetType type = AssetType::Unknown;
    AssetState state = AssetState::Unloaded;
    
    size_t size_bytes = 0;
    size_t compressed_size = 0;
    uint64_t checksum = 0;
    
    std::chrono::system_clock::time_point last_modified;
    std::chrono::system_clock::time_point last_accessed;
    std::chrono::system_clock::time_point last_validated;
    
    std::vector<std::string> dependencies;
    std::vector<std::string> dependents;
    std::unordered_map<std::string, std::string> tags;
    
    int version = 1;
    int load_priority = 0;
    bool is_streaming = false;
    bool is_compressed = false;
    CompressionType compression = CompressionType::None;
    
    Platform target_platform = Platform::All;
    std::string documentation;
};

struct AssetValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::chrono::milliseconds validation_time{0};
};

struct AssetAnalytics {
    std::string asset_path;
    size_t load_count = 0;
    size_t access_count = 0;
    std::chrono::milliseconds total_load_time{0};
    std::chrono::milliseconds average_load_time{0};
    std::chrono::system_clock::time_point last_load_time;
    size_t memory_usage = 0;
    bool is_hot_asset = false;
};

// ============================================================================
// Asset Validation System
// ============================================================================

class AssetValidator {
public:
    using ValidatorFunc = std::function<AssetValidationResult(const AssetMetadata&)>;
    
    static AssetValidator& GetInstance();
    
    void RegisterValidator(AssetType type, ValidatorFunc validator);
    AssetValidationResult ValidateAsset(const AssetMetadata& metadata);
    bool ValidateAllAssets(std::vector<AssetValidationResult>& results);
    
    void SetStrictMode(bool strict) { strict_mode_ = strict; }
    bool IsStrictMode() const { return strict_mode_; }

private:
    AssetValidator() = default;
    std::unordered_map<AssetType, ValidatorFunc> validators_;
    bool strict_mode_ = false;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Dependency Tracker
// ============================================================================

class DependencyTracker {
public:
    static DependencyTracker& GetInstance();
    
    void RegisterDependency(const std::string& asset, const std::string& dependency);
    void RemoveDependency(const std::string& asset, const std::string& dependency);
    
    std::vector<std::string> GetDependencies(const std::string& asset) const;
    std::vector<std::string> GetDependents(const std::string& asset) const;
    std::vector<std::string> GetDependencyChain(const std::string& asset) const;
    
    bool HasCircularDependency(const std::string& asset) const;
    std::vector<std::string> GetLoadOrder(const std::vector<std::string>& assets) const;
    
    void Clear();
    void ExportDependencyGraph(const std::string& output_path) const;

private:
    DependencyTracker() = default;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependencies_;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependents_;
    mutable std::mutex mutex_;
    
    bool HasCircularDependencyRecursive(const std::string& asset,
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& stack) const;
};

// ============================================================================
// Hot Reloading System
// ============================================================================

class HotReloadManager {
public:
    using ReloadCallback = std::function<void(const std::string&)>;
    
    static HotReloadManager& GetInstance();
    
    void Enable(bool enable) { enabled_ = enable; }
    bool IsEnabled() const { return enabled_; }
    
    void WatchAsset(const std::string& path);
    void UnwatchAsset(const std::string& path);
    void WatchDirectory(const std::string& directory, bool recursive = true);
    
    void RegisterReloadCallback(const std::string& asset, ReloadCallback callback);
    void Update();
    
    size_t GetPendingReloads() const;
    void FlushReloads();

private:
    HotReloadManager() = default;
    std::atomic<bool> enabled_{true};
    std::unordered_map<std::string, std::chrono::system_clock::time_point> watched_files_;
    std::unordered_map<std::string, ReloadCallback> callbacks_;
    std::vector<std::string> pending_reloads_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Compression System
// ============================================================================

class CompressionManager {
public:
    static CompressionManager& GetInstance();
    
    bool CompressAsset(const std::string& input_path,
                      const std::string& output_path,
                      CompressionType type = CompressionType::Auto);
    
    bool DecompressAsset(const std::string& input_path,
                        const std::string& output_path);
    
    std::vector<uint8_t> CompressData(const std::vector<uint8_t>& data,
                                     CompressionType type = CompressionType::Auto);
    
    std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& data,
                                       CompressionType type);
    
    CompressionType GetOptimalCompression(AssetType type) const;
    float GetCompressionRatio(const std::string& path) const;

private:
    CompressionManager() = default;
    std::unordered_map<std::string, CompressionType> compression_cache_;
};

// ============================================================================
// Asset Versioning System
// ============================================================================

class VersionManager {
public:
    static VersionManager& GetInstance();
    
    void SetAssetVersion(const std::string& path, int version);
    int GetAssetVersion(const std::string& path) const;
    
    bool IsVersionCompatible(const std::string& path, int required_version) const;
    std::vector<std::string> GetChangelog(const std::string& path) const;
    
    void AddChangelogEntry(const std::string& path, const std::string& entry);
    void ExportVersionManifest(const std::string& output_path) const;

private:
    VersionManager() = default;
    std::unordered_map<std::string, int> versions_;
    std::unordered_map<std::string, std::vector<std::string>> changelogs_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Optimization System
// ============================================================================

class OptimizationManager {
public:
    static OptimizationManager& GetInstance();
    
    void OptimizeAsset(const std::string& path, Platform platform);
    void OptimizeDirectory(const std::string& directory, Platform platform);
    
    void SetOptimizationLevel(int level) { optimization_level_ = level; }
    int GetOptimizationLevel() const { return optimization_level_; }
    
    bool CanOptimize(AssetType type) const;
    size_t EstimateOptimizationSavings(const std::string& path) const;

private:
    OptimizationManager() = default;
    int optimization_level_ = 2;
    
    void OptimizeTexture(const std::string& path, Platform platform);
    void OptimizeModel(const std::string& path, Platform platform);
    void OptimizeAudio(const std::string& path, Platform platform);
};

// ============================================================================
// Asset Streaming System
// ============================================================================

class StreamingManager {
public:
    static StreamingManager& GetInstance();
    
    void EnableStreaming(bool enable) { streaming_enabled_ = enable; }
    bool IsStreamingEnabled() const { return streaming_enabled_; }
    
    void MarkStreamable(const std::string& path, int priority = 0);
    void UnmarkStreamable(const std::string& path);
    
    bool IsStreamable(const std::string& path) const;
    void StreamAsset(const std::string& path);
    void UnstreamAsset(const std::string& path);
    
    void SetMemoryBudget(size_t bytes) { memory_budget_ = bytes; }
    size_t GetMemoryUsage() const { return current_memory_usage_; }
    size_t GetMemoryBudget() const { return memory_budget_; }
    
    void Update();

private:
    StreamingManager() = default;
    std::atomic<bool> streaming_enabled_{true};
    size_t memory_budget_ = 512 * 1024 * 1024; // 512 MB
    std::atomic<size_t> current_memory_usage_{0};
    
    struct StreamInfo {
        std::string path;
        int priority;
        bool loaded;
    };
    std::vector<StreamInfo> stream_queue_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Caching System
// ============================================================================

class CacheManager {
public:
    static CacheManager& GetInstance();
    
    void EnableCache(bool enable) { cache_enabled_ = enable; }
    bool IsCacheEnabled() const { return cache_enabled_; }
    
    void CacheAsset(const std::string& path, const void* data, size_t size);
    bool GetCachedAsset(const std::string& path, void** data, size_t* size);
    
    void InvalidateCache(const std::string& path);
    void ClearCache();
    
    void SetCacheSize(size_t bytes) { max_cache_size_ = bytes; }
    size_t GetCacheUsage() const { return current_cache_size_; }
    
    struct CacheStats {
        size_t hit_count = 0;
        size_t miss_count = 0;
        float hit_rate = 0.0f;
        size_t cache_size = 0;
    };
    CacheStats GetStats() const;

private:
    CacheManager() = default;
    std::atomic<bool> cache_enabled_{true};
    size_t max_cache_size_ = 256 * 1024 * 1024; // 256 MB
    std::atomic<size_t> current_cache_size_{0};
    
    struct CacheEntry {
        std::vector<uint8_t> data;
        std::chrono::system_clock::time_point last_access;
    };
    std::unordered_map<std::string, CacheEntry> cache_;
    std::atomic<size_t> hit_count_{0};
    std::atomic<size_t> miss_count_{0};
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Analytics System
// ============================================================================

class AnalyticsManager {
public:
    static AnalyticsManager& GetInstance();
    
    void RecordAssetLoad(const std::string& path, std::chrono::milliseconds duration);
    void RecordAssetAccess(const std::string& path);
    void RecordMemoryUsage(const std::string& path, size_t bytes);
    
    AssetAnalytics GetAnalytics(const std::string& path) const;
    std::vector<AssetAnalytics> GetTopAssets(size_t count) const;
    std::vector<std::string> GetHotAssets() const;
    
    void ExportReport(const std::string& output_path) const;
    void ClearAnalytics();

private:
    AnalyticsManager() = default;
    std::unordered_map<std::string, AssetAnalytics> analytics_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Asset Documentation Generator
// ============================================================================

class DocumentationGenerator {
public:
    static DocumentationGenerator& GetInstance();
    
    void GenerateDocumentation(const std::string& output_path);
    void GenerateAssetDoc(const std::string& asset_path, const std::string& output_path);
    
    void SetDocFormat(const std::string& format) { doc_format_ = format; }
    std::string GetDocFormat() const { return doc_format_; }
    
    void AddCustomSection(const std::string& title, const std::string& content);

private:
    DocumentationGenerator() = default;
    std::string doc_format_ = "markdown";
    std::unordered_map<std::string, std::string> custom_sections_;
};

// ============================================================================
// Main Asset Pipeline Manager
// ============================================================================

class AssetPipelineManager {
public:
    static AssetPipelineManager& GetInstance();
    
    // Initialization
    bool Initialize(const std::string& asset_root);
    void Shutdown();
    
    // Asset registration and discovery
    void RegisterAsset(const std::string& path, AssetType type);
    void DiscoverAssets(const std::string& directory);
    AssetMetadata* GetAssetMetadata(const std::string& path);
    
    // Pipeline operations
    bool ValidateAllAssets();
    void OptimizeAllAssets(Platform platform);
    void GenerateAllDocumentation();
    
    // System access
    AssetValidator& GetValidator() { return AssetValidator::GetInstance(); }
    DependencyTracker& GetDependencies() { return DependencyTracker::GetInstance(); }
    HotReloadManager& GetHotReload() { return HotReloadManager::GetInstance(); }
    CompressionManager& GetCompression() { return CompressionManager::GetInstance(); }
    VersionManager& GetVersioning() { return VersionManager::GetInstance(); }
    OptimizationManager& GetOptimization() { return OptimizationManager::GetInstance(); }
    StreamingManager& GetStreaming() { return StreamingManager::GetInstance(); }
    CacheManager& GetCache() const { return CacheManager::GetInstance(); }
    AnalyticsManager& GetAnalytics() const { return AnalyticsManager::GetInstance(); }
    DocumentationGenerator& GetDocumentation() const { return DocumentationGenerator::GetInstance(); }
    StreamingManager& GetStreaming() const { return StreamingManager::GetInstance(); }
    
    // Update and maintenance
    void Update();
    void FlushAll();
    
    // Status and reporting
    struct PipelineStatus {
        size_t total_assets = 0;
        size_t loaded_assets = 0;
        size_t failed_assets = 0;
        size_t cached_assets = 0;
        size_t streamed_assets = 0;
        size_t memory_usage = 0;
        size_t cache_usage = 0;
    };
    PipelineStatus GetStatus() const;
    void ExportStatusReport(const std::string& output_path) const;

private:
    AssetPipelineManager() = default;
    std::string asset_root_;
    std::unordered_map<std::string, AssetMetadata> assets_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Utility Functions
// ============================================================================

AssetType GetAssetTypeFromExtension(const std::string& path);
std::string GetAssetTypeName(AssetType type);
uint64_t CalculateChecksum(const std::string& path);
bool FileExists(const std::string& path);
size_t GetFileSize(const std::string& path);
std::chrono::system_clock::time_point GetFileModificationTime(const std::string& path);

} // namespace AssetPipeline
