#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace AssetPipeline {

/**
 * Asset Metrics System
 * Comprehensive metrics and analytics for asset management
 */

struct AssetMetrics {
    // Performance metrics
    size_t load_count = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    std::chrono::milliseconds total_load_time{0};
    std::chrono::milliseconds average_load_time{0};
    std::chrono::milliseconds min_load_time{INT_MAX};
    std::chrono::milliseconds max_load_time{0};
    
    // Memory metrics
    size_t current_memory_usage = 0;
    size_t peak_memory_usage = 0;
    size_t total_bytes_loaded = 0;
    
    // Access patterns
    std::chrono::system_clock::time_point first_access;
    std::chrono::system_clock::time_point last_access;
    std::chrono::system_clock::time_point most_recent_load;
    size_t access_frequency = 0; // Accesses per hour
    
    // Lifecycle metrics
    size_t reload_count = 0;
    size_t validation_count = 0;
    size_t optimization_count = 0;
    size_t compression_count = 0;
    
    // Error metrics
    size_t load_failures = 0;
    size_t validation_failures = 0;
    std::vector<std::string> recent_errors;
    
    // Dependency metrics
    size_t dependency_depth = 0;
    size_t dependent_count = 0;
    size_t circular_dependency_count = 0;
};

struct SystemMetrics {
    // Overall statistics
    size_t total_assets = 0;
    size_t loaded_assets = 0;
    size_t failed_assets = 0;
    
    // Memory statistics
    size_t total_memory_used = 0;
    size_t cache_memory_used = 0;
    size_t streaming_memory_used = 0;
    
    // Performance statistics
    float average_load_time_ms = 0.0f;
    float cache_hit_rate = 0.0f;
    size_t total_loads = 0;
    
    // Type breakdown
    std::unordered_map<AssetType, size_t> assets_by_type;
    std::unordered_map<AssetType, size_t> memory_by_type;
    
    // Platform breakdown
    std::unordered_map<Platform, size_t> assets_by_platform;
    
    // Hot assets (top 10)
    std::vector<std::string> most_accessed_assets;
    std::vector<std::string> largest_assets;
    std::vector<std::string> slowest_loading_assets;
};

class AssetMetricsCollector {
public:
    static AssetMetricsCollector& GetInstance();

    // Metric recording
    void RecordLoad(const std::string& asset_path, std::chrono::milliseconds load_time, bool success);
    void RecordCacheHit(const std::string& asset_path);
    void RecordCacheMiss(const std::string& asset_path);
    void RecordMemoryUsage(const std::string& asset_path, size_t bytes);
    void RecordAccess(const std::string& asset_path);
    void RecordReload(const std::string& asset_path);
    void RecordValidation(const std::string& asset_path, bool success);
    void RecordOptimization(const std::string& asset_path);
    void RecordError(const std::string& asset_path, const std::string& error);

    // Metric retrieval
    AssetMetrics GetMetrics(const std::string& asset_path);
    SystemMetrics GetSystemMetrics();
    
    // Analysis
    std::vector<std::string> GetTopAssets(size_t count, const std::string& metric = "access_count");
    std::vector<std::string> GetProblematicAssets(); // High failure rate or slow
    std::vector<std::string> GetUnusedAssets(const std::chrono::hours& threshold);
    std::vector<std::string> GetMemoryHogs(size_t min_size_mb);
    
    // Trends
    std::vector<float> GetLoadTimesTrend(const std::string& asset_path, size_t samples = 10);
    std::vector<size_t> GetAccessCountTrend(const std::chrono::hours& window);
    
    // Reports
    std::string GenerateMetricsReport();
    std::string GenerateAssetReport(const std::string& asset_path);
    std::string GeneratePerformanceReport();
    std::string GenerateMemoryReport();
    
    // Export
    bool ExportMetricsCSV(const std::string& file_path);
    bool ExportMetricsJSON(const std::string& file_path);
    
    // Reset
    void ResetMetrics(const std::string& asset_path);
    void ResetAllMetrics();

private:
    AssetMetricsCollector() = default;
    
    std::unordered_map<std::string, AssetMetrics> metrics_;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
