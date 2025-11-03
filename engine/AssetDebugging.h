#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <chrono>
#include <sstream>

namespace AssetPipeline {

/**
 * Asset Debugging System
 * Debug tools for asset system troubleshooting
 */

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

struct DebugLogEntry {
    LogLevel level;
    std::chrono::system_clock::time_point timestamp;
    std::string category;
    std::string message;
    std::string asset_path;
    std::string thread_id;
};

struct AssetTrace {
    std::string asset_path;
    std::vector<DebugLogEntry> events;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::milliseconds total_time{0};
};

struct Breakpoint {
    std::string asset_path;
    std::string condition; // e.g., "on_load", "on_error", "on_validate"
    bool enabled = true;
    std::function<void(const AssetMetadata&)> callback;
};

class AssetDebugger {
public:
    static AssetDebugger& GetInstance();

    // Logging
    void SetLogLevel(LogLevel level);
    LogLevel GetLogLevel() const { return log_level_; }
    
    void Log(LogLevel level, const std::string& category, const std::string& message);
    void Log(LogLevel level, const std::string& category, const std::string& asset_path, const std::string& message);
    
    std::vector<DebugLogEntry> GetLogs(size_t count = 100);
    std::vector<DebugLogEntry> GetLogsForAsset(const std::string& asset_path);
    std::vector<DebugLogEntry> GetLogsByLevel(LogLevel level);
    void ClearLogs();
    
    // Log filtering
    void EnableCategory(const std::string& category);
    void DisableCategory(const std::string& category);
    bool IsCategoryEnabled(const std::string& category);
    
    // Asset tracing
    void StartTrace(const std::string& asset_path);
    void StopTrace(const std::string& asset_path);
    AssetTrace GetTrace(const std::string& asset_path);
    std::vector<std::string> GetActiveTraces();
    
    // Breakpoints
    size_t AddBreakpoint(const std::string& asset_path, const std::string& condition);
    void RemoveBreakpoint(size_t id);
    void EnableBreakpoint(size_t id, bool enable);
    std::vector<Breakpoint> GetBreakpoints();
    
    void SetBreakpointCallback(size_t id, std::function<void(const AssetMetadata&)> callback);
    void TriggerBreakpoint(const std::string& asset_path, const std::string& condition, const AssetMetadata& metadata);
    
    // Asset inspection
    struct AssetDiagnostics {
        AssetMetadata metadata;
        std::vector<std::string> issues;
        std::vector<std::string> warnings;
        std::vector<std::string> dependencies_status;
        std::vector<std::string> dependents_status;
        size_t memory_footprint = 0;
        std::chrono::milliseconds last_load_time{0};
        size_t load_failure_count = 0;
    };
    
    AssetDiagnostics InspectAsset(const std::string& asset_path);
    std::string GenerateDiagnosticReport(const std::string& asset_path);
    
    // Performance profiling
    struct ProfileEntry {
        std::string operation;
        std::chrono::microseconds duration;
        std::chrono::system_clock::time_point timestamp;
    };
    
    void BeginProfile(const std::string& operation);
    void EndProfile(const std::string& operation);
    std::vector<ProfileEntry> GetProfileData(const std::string& operation);
    void ClearProfileData();
    
    // RAII profiler helper
    class ScopedProfile {
    public:
        ScopedProfile(const std::string& op) : operation(op) {
            AssetDebugger::GetInstance().BeginProfile(operation);
        }
        ~ScopedProfile() {
            AssetDebugger::GetInstance().EndProfile(operation);
        }
    private:
        std::string operation;
    };
    
    // Memory debugging
    struct MemorySnapshot {
        std::chrono::system_clock::time_point timestamp;
        size_t total_allocated = 0;
        std::unordered_map<std::string, size_t> allocation_by_asset;
        std::unordered_map<AssetType, size_t> allocation_by_type;
    };
    
    MemorySnapshot TakeMemorySnapshot();
    std::vector<MemorySnapshot> GetMemoryHistory();
    std::vector<std::string> DetectMemoryLeaks();
    
    // Validation debugging
    void EnableValidationDebug(bool enable) { validation_debug_ = enable; }
    std::vector<std::string> GetValidationTrace(const std::string& asset_path);
    
    // Dependency debugging
    std::string VisualizeDependencyChain(const std::string& asset_path);
    std::vector<std::string> FindCircularDependencies();
    std::string ExportDependencyTree(const std::string& asset_path, int max_depth = 5);
    
    // Export debug data
    bool ExportDebugLog(const std::string& file_path);
    bool ExportTraceData(const std::string& file_path);
    bool ExportProfileData(const std::string& file_path);
    
    // Debug UI helper
    std::string GetDebugString(const std::string& asset_path);
    
private:
    AssetDebugger() = default;
    
    LogLevel log_level_ = LogLevel::Info;
    std::vector<DebugLogEntry> log_entries_;
    std::unordered_set<std::string> enabled_categories_;
    
    std::unordered_map<std::string, AssetTrace> active_traces_;
    std::unordered_map<size_t, Breakpoint> breakpoints_;
    size_t next_breakpoint_id_ = 0;
    
    std::unordered_map<std::string, std::vector<ProfileEntry>> profile_data_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> active_profiles_;
    
    std::vector<MemorySnapshot> memory_history_;
    std::atomic<bool> validation_debug_{false};
    
    mutable std::mutex mutex_;
};

// Convenience macros
#define ASSET_DEBUG_LOG(category, message) \
    AssetPipeline::AssetDebugger::GetInstance().Log(AssetPipeline::LogLevel::Debug, category, message)

#define ASSET_ERROR_LOG(category, message) \
    AssetPipeline::AssetDebugger::GetInstance().Log(AssetPipeline::LogLevel::Error, category, message)

#define ASSET_PROFILE(operation) \
    AssetPipeline::AssetDebugger::ScopedProfile _profile_##__LINE__(operation)

} // namespace AssetPipeline
