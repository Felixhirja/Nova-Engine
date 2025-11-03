#pragma once

#include "AssetPipeline.h"
#include <string>
#include <functional>
#include <vector>
#include <optional>

namespace AssetPipeline {

/**
 * Asset API System
 * Unified programmatic interface for asset management
 */

// API Result types
template<typename T>
struct APIResult {
    bool success = false;
    std::string error_message;
    T data;
    
    explicit operator bool() const { return success; }
};

// Event types
enum class AssetEvent {
    Created,
    Modified,
    Deleted,
    Loaded,
    Unloaded,
    Validated,
    Optimized,
    Compressed,
    Streamed
};

using AssetEventCallback = std::function<void(const std::string& asset_path, AssetEvent event)>;

class AssetAPI {
public:
    static AssetAPI& GetInstance();

    // Asset CRUD operations
    APIResult<AssetMetadata> CreateAsset(const std::string& path, AssetType type);
    APIResult<AssetMetadata> GetAsset(const std::string& path);
    APIResult<bool> UpdateAsset(const std::string& path, const AssetMetadata& metadata);
    APIResult<bool> DeleteAsset(const std::string& path);
    
    // Bulk operations
    APIResult<std::vector<AssetMetadata>> ListAssets(const std::string& directory = "");
    APIResult<bool> ImportAssets(const std::vector<std::string>& paths);
    APIResult<bool> ExportAssets(const std::vector<std::string>& paths, const std::string& destination);
    
    // Loading operations
    APIResult<bool> LoadAsset(const std::string& path);
    APIResult<bool> UnloadAsset(const std::string& path);
    APIResult<bool> ReloadAsset(const std::string& path);
    APIResult<bool> PreloadAssets(const std::vector<std::string>& paths);
    
    // Validation operations
    APIResult<AssetValidationResult> ValidateAsset(const std::string& path);
    APIResult<std::vector<AssetValidationResult>> ValidateAssets(const std::vector<std::string>& paths);
    
    // Optimization operations
    APIResult<bool> OptimizeAsset(const std::string& path, Platform platform = Platform::All);
    APIResult<bool> OptimizeAssets(const std::vector<std::string>& paths, Platform platform = Platform::All);
    
    // Compression operations
    APIResult<bool> CompressAsset(const std::string& path, CompressionType type = CompressionType::Auto);
    APIResult<bool> DecompressAsset(const std::string& path);
    
    // Dependency operations
    APIResult<std::vector<std::string>> GetDependencies(const std::string& path);
    APIResult<std::vector<std::string>> GetDependents(const std::string& path);
    APIResult<bool> AddDependency(const std::string& asset, const std::string& dependency);
    APIResult<bool> RemoveDependency(const std::string& asset, const std::string& dependency);
    
    // Tag operations
    APIResult<bool> AddTag(const std::string& path, const std::string& key, const std::string& value);
    APIResult<bool> RemoveTag(const std::string& path, const std::string& key);
    APIResult<std::unordered_map<std::string, std::string>> GetTags(const std::string& path);
    
    // Search operations
    APIResult<std::vector<AssetMetadata>> SearchAssets(const std::string& query);
    APIResult<std::vector<AssetMetadata>> FilterAssets(AssetType type);
    
    // Metrics operations
    APIResult<AssetMetrics> GetMetrics(const std::string& path);
    APIResult<SystemMetrics> GetSystemMetrics();
    
    // Event system
    void RegisterEventCallback(AssetEvent event, AssetEventCallback callback);
    void UnregisterEventCallback(AssetEvent event);
    void TriggerEvent(const std::string& asset_path, AssetEvent event);
    
    // Batch operations
    struct BatchOperation {
        enum class Type {
            Load,
            Unload,
            Validate,
            Optimize,
            Compress
        };
        
        Type type;
        std::vector<std::string> assets;
        std::unordered_map<std::string, std::string> parameters;
    };
    
    APIResult<bool> ExecuteBatch(const std::vector<BatchOperation>& operations);
    
    // Query builder
    class QueryBuilder {
    public:
        QueryBuilder& OfType(AssetType type);
        QueryBuilder& InDirectory(const std::string& directory);
        QueryBuilder& WithTag(const std::string& key, const std::string& value);
        QueryBuilder& LargerThan(size_t bytes);
        QueryBuilder& SmallerThan(size_t bytes);
        QueryBuilder& ModifiedAfter(std::chrono::system_clock::time_point time);
        QueryBuilder& ModifiedBefore(std::chrono::system_clock::time_point time);
        QueryBuilder& WithState(AssetState state);
        QueryBuilder& Limit(size_t count);
        
        APIResult<std::vector<AssetMetadata>> Execute();
        
    private:
        friend class AssetAPI;
        std::unordered_map<std::string, std::string> criteria_;
    };
    
    QueryBuilder Query();
    
    // Statistics
    struct APIStats {
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        std::chrono::milliseconds average_response_time{0};
    };
    
    APIStats GetAPIStats();
    void ResetAPIStats();

private:
    AssetAPI() = default;
    
    void UpdateStats(bool success, std::chrono::milliseconds response_time);
    
    std::unordered_map<AssetEvent, std::vector<AssetEventCallback>> event_callbacks_;
    APIStats stats_;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
