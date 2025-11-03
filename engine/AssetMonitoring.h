#pragma once

#include "AssetPipeline.h"
#include <string>
#include <chrono>
#include <vector>
#include <functional>

namespace AssetPipeline {

/**
 * Asset Monitoring System
 * Real-time monitoring of asset system health and performance
 */

enum class HealthStatus {
    Healthy,
    Warning,
    Critical,
    Unknown
};

enum class AlertLevel {
    Info,
    Warning,
    Error,
    Critical
};

struct HealthCheck {
    std::string name;
    HealthStatus status = HealthStatus::Unknown;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    float value = 0.0f;
    float threshold = 0.0f;
};

struct Alert {
    AlertLevel level;
    std::string category;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> affected_assets;
    bool acknowledged = false;
};

struct SystemHealth {
    HealthStatus overall_status = HealthStatus::Unknown;
    
    // Component health
    HealthStatus pipeline_health = HealthStatus::Unknown;
    HealthStatus cache_health = HealthStatus::Unknown;
    HealthStatus streaming_health = HealthStatus::Unknown;
    HealthStatus validation_health = HealthStatus::Unknown;
    
    // Performance indicators
    float average_load_time_ms = 0.0f;
    float cache_hit_rate = 0.0f;
    size_t memory_usage_mb = 0;
    size_t failed_loads = 0;
    
    // Active issues
    std::vector<Alert> active_alerts;
    std::vector<HealthCheck> checks;
    
    std::chrono::system_clock::time_point last_check;
};

class AssetMonitoring {
public:
    static AssetMonitoring& GetInstance();

    // Health monitoring
    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return monitoring_; }
    
    SystemHealth GetSystemHealth();
    std::vector<HealthCheck> RunHealthChecks();
    
    // Health check registration
    using HealthCheckFunc = std::function<HealthCheck()>;
    void RegisterHealthCheck(const std::string& name, HealthCheckFunc check);
    void UnregisterHealthCheck(const std::string& name);
    
    // Alert management
    void RaiseAlert(AlertLevel level, const std::string& category, const std::string& message);
    void RaiseAlert(AlertLevel level, const std::string& category, const std::string& message, const std::vector<std::string>& assets);
    void AcknowledgeAlert(size_t alert_id);
    void ClearAlert(size_t alert_id);
    std::vector<Alert> GetActiveAlerts();
    std::vector<Alert> GetAlertHistory(size_t count = 100);
    
    // Alert callbacks
    using AlertCallback = std::function<void(const Alert&)>;
    void RegisterAlertCallback(AlertLevel min_level, AlertCallback callback);
    
    // Performance monitoring
    struct PerformanceSnapshot {
        std::chrono::system_clock::time_point timestamp;
        size_t total_assets = 0;
        size_t loaded_assets = 0;
        float average_load_time_ms = 0.0f;
        float cache_hit_rate = 0.0f;
        size_t memory_usage_mb = 0;
        size_t active_streams = 0;
    };
    
    PerformanceSnapshot TakeSnapshot();
    std::vector<PerformanceSnapshot> GetPerformanceHistory(const std::chrono::hours& window);
    
    // Threshold configuration
    void SetMemoryThreshold(size_t mb);
    void SetLoadTimeThreshold(float ms);
    void SetCacheHitRateThreshold(float rate);
    void SetFailureRateThreshold(float rate);
    
    // Automated actions
    void EnableAutomatedRecovery(bool enable) { auto_recovery_ = enable; }
    void SetRecoveryAction(const std::string& condition, std::function<void()> action);
    
    // Reports
    std::string GenerateHealthReport();
    std::string GenerateAlertSummary(const std::chrono::hours& window);
    bool ExportMonitoringData(const std::string& file_path);
    
    // Update (call regularly)
    void Update();

private:
    AssetMonitoring() = default;
    
    void CheckThresholds();
    void ExecuteRecoveryActions();
    void TriggerAlertCallbacks(const Alert& alert);
    
    std::atomic<bool> monitoring_{false};
    std::atomic<bool> auto_recovery_{false};
    
    std::vector<Alert> active_alerts_;
    std::vector<Alert> alert_history_;
    std::vector<PerformanceSnapshot> performance_history_;
    
    std::unordered_map<std::string, HealthCheckFunc> health_checks_;
    std::vector<std::pair<AlertLevel, AlertCallback>> alert_callbacks_;
    std::unordered_map<std::string, std::function<void()>> recovery_actions_;
    
    // Thresholds
    size_t memory_threshold_mb_ = 1024;
    float load_time_threshold_ms_ = 100.0f;
    float cache_hit_rate_threshold_ = 0.5f;
    float failure_rate_threshold_ = 0.1f;
    
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
