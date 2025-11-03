#pragma once

#include "ActorLifecycleManager.h"
#include "LifecycleAnalytics.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <deque>

namespace lifecycle {

// Alert levels for lifecycle monitoring
enum class AlertLevel {
    Info,
    Warning,
    Error,
    Critical
};

// Monitoring alert data structure
struct LifecycleAlert {
    AlertLevel level;
    std::string message;
    std::string actorType;
    std::string actorName;
    std::chrono::high_resolution_clock::time_point timestamp;
    double value;  // Associated metric value
    
    LifecycleAlert(AlertLevel lvl, const std::string& msg, const std::string& type = "", 
                   const std::string& name = "", double val = 0.0)
        : level(lvl), message(msg), actorType(type), actorName(name), 
          timestamp(std::chrono::high_resolution_clock::now()), value(val) {}
};

// Real-time lifecycle monitoring system
class LifecycleMonitor {
public:
    struct Config {
        bool enableRealTimeMonitoring;
        bool enablePeriodicReports;
        bool enableAlerting;
        bool enableFileLogging;
        double reportIntervalSeconds;
        double slowInitThresholdSeconds;
        double slowActiveThresholdSeconds;
        size_t highCreationRateThreshold;  // actors per second
        size_t maxActiveActorsThreshold;
        std::string logFilePath;
        
        Config() : enableRealTimeMonitoring(true), enablePeriodicReports(true),
                  enableAlerting(true), enableFileLogging(true),
                  reportIntervalSeconds(30.0), slowInitThresholdSeconds(1.0),
                  slowActiveThresholdSeconds(0.1), highCreationRateThreshold(100),
                  maxActiveActorsThreshold(1000), logFilePath("lifecycle_monitor.log") {}
    };

    static LifecycleMonitor& Instance() {
        static LifecycleMonitor inst;
        return inst;
    }

    void Initialize(const Config& config = Config{}) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (initialized_) return;
        
        config_ = config;
        initialized_ = true;
        running_ = true;
        
        if (config_.enableFileLogging) {
            logFile_.open(config_.logFilePath, std::ios::app);
            if (logFile_) {
                logFile_ << "\n=== Lifecycle Monitor Session Started ===\n";
                logFile_ << "Timestamp: " << GetTimestamp() << "\n";
                logFile_.flush();
            }
        }
        
        // Start monitoring thread
        if (config_.enableRealTimeMonitoring) {
            monitoringThread_ = std::thread(&LifecycleMonitor::MonitoringLoop, this);
        }
        
        // Register lifecycle hooks for real-time monitoring
        RegisterMonitoringHooks();
        
        std::cout << "[LifecycleMonitor] Real-time monitoring initialized" << std::endl;
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (!initialized_) return;
            
            running_ = false;
        }
        condition_.notify_all();
        
        if (monitoringThread_.joinable()) {
            monitoringThread_.join();
        }
        
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (logFile_.is_open()) {
                logFile_ << "=== Lifecycle Monitor Session Ended ===\n";
                logFile_ << "Total alerts generated: " << totalAlerts_ << "\n";
                logFile_.close();
            }
            
            initialized_ = false;
        }
        
        std::cout << "[LifecycleMonitor] Monitoring shutdown complete" << std::endl;
    }

    // Generate comprehensive real-time status report
    std::string GetRealtimeStatus() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::ostringstream ss;
        
        // Get current analytics data
        auto analytics = LifecycleAnalytics::Instance().GenerateReport();
        
        ss << "=== Real-time Lifecycle Status ===\n";
        ss << "Monitor uptime: " << GetUptimeSeconds() << "s\n";
        ss << "Total alerts: " << totalAlerts_ << "\n";
        ss << "Recent alerts (last 10):\n";
        
        // Show recent alerts (up to 10)
        auto recentAlerts = GetRecentAlerts(10);
        for (const auto& alert : recentAlerts) {
            ss << "  [" << AlertLevelToString(alert.level) << "] " 
               << alert.message;
            if (!alert.actorType.empty()) {
                ss << " (type=" << alert.actorType << ")";
            }
            ss << "\n";
        }
        
        ss << "\nCurrent analytics:\n";
        ss << analytics;
        
        return ss.str();
    }

    // Export monitoring data to JSON format
    std::string ExportMonitoringData() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::ostringstream ss;
        
        ss << "{\n";
        ss << "  \"monitorUptime\": " << GetUptimeSeconds() << ",\n";
        ss << "  \"totalAlerts\": " << totalAlerts_ << ",\n";
        ss << "  \"config\": {\n";
        ss << "    \"reportInterval\": " << config_.reportIntervalSeconds << ",\n";
        ss << "    \"slowInitThreshold\": " << config_.slowInitThresholdSeconds << ",\n";
        ss << "    \"slowActiveThreshold\": " << config_.slowActiveThresholdSeconds << ",\n";
        ss << "    \"highCreationRateThreshold\": " << config_.highCreationRateThreshold << ",\n";
        ss << "    \"maxActiveActorsThreshold\": " << config_.maxActiveActorsThreshold << "\n";
        ss << "  },\n";
        
        // Export recent alerts
        ss << "  \"recentAlerts\": [\n";
        auto recentAlerts = GetRecentAlerts(50);
        for (size_t i = 0; i < recentAlerts.size(); ++i) {
            if (i > 0) ss << ",\n";
            const auto& alert = recentAlerts[i];
            ss << "    {\n";
            ss << "      \"level\": \"" << AlertLevelToString(alert.level) << "\",\n";
            ss << "      \"message\": \"" << EscapeJson(alert.message) << "\",\n";
            ss << "      \"actorType\": \"" << EscapeJson(alert.actorType) << "\",\n";
            ss << "      \"actorName\": \"" << EscapeJson(alert.actorName) << "\",\n";
            ss << "      \"value\": " << alert.value << "\n";
            ss << "    }";
        }
        ss << "\n  ],\n";
        
        // Include analytics data
        ss << "  \"analytics\": " << LifecycleAnalytics::Instance().ExportJson();
        ss << "\n}\n";
        
        return ss.str();
    }

    // Print real-time dashboard to console
    void PrintDashboard() const {
        std::cout << "\n" << GetRealtimeStatus() << std::endl;
    }

    // Get recent alerts for UI display
    std::vector<LifecycleAlert> GetRecentAlerts(size_t maxCount) const {
        std::vector<LifecycleAlert> result;
        auto it = alerts_.end();
        size_t count = 0;
        while (it != alerts_.begin() && count < maxCount) {
            --it;
            result.push_back(*it);
            ++count;
        }
        return result;
    }

    // Manual alert generation for external systems
    void AddAlert(AlertLevel level, const std::string& message, 
                  const std::string& actorType = "", const std::string& actorName = "", 
                  double value = 0.0) {
        std::lock_guard<std::mutex> lk(mutex_);
        alerts_.emplace_back(level, message, actorType, actorName, value);
        totalAlerts_++;
        
        // Limit alert history size
        if (alerts_.size() > 1000) {
            alerts_.pop_front();
        }
        
        // Log to file
        if (logFile_.is_open()) {
            logFile_ << "[" << GetTimestamp() << "] [" << AlertLevelToString(level) << "] " 
                     << message;
            if (!actorType.empty()) {
                logFile_ << " (type=" << actorType << ", name=" << actorName << ")";
            }
            if (value != 0.0) {
                logFile_ << " value=" << value;
            }
            logFile_ << "\n";
            logFile_.flush();
        }
        
        // Console output for critical alerts
        if (level == AlertLevel::Critical || level == AlertLevel::Error) {
            std::cout << "[LifecycleMonitor] [" << AlertLevelToString(level) << "] " 
                      << message << std::endl;
        }
    }

private:
    LifecycleMonitor() = default;
    
    void RegisterMonitoringHooks() {
        auto& manager = ActorLifecycleManager::Instance();
        
        // Monitor slow initializations
        manager.RegisterHook(LifecycleEvent::PostInitialize, "monitor_slow_init",
            [this](LifecycleContext& ctx) {
                double initTime = ctx.stats.getInitializationDuration();
                if (initTime > config_.slowInitThresholdSeconds) {
                    AddAlert(AlertLevel::Warning, 
                             "Slow actor initialization detected", 
                             ctx.actorType, ctx.actorName, initTime);
                }
            });
        
        // Monitor actor creation rates
        manager.RegisterHook(LifecycleEvent::PostCreate, "monitor_creation_rate",
            [this](LifecycleContext& ctx) {
                (void)ctx;  // Suppress unused parameter warning
                auto now = std::chrono::high_resolution_clock::now();
                std::lock_guard<std::mutex> lk(mutex_);
                creationTimes_.push_back(now);
                
                // Clean old creation times (keep last 60 seconds)
                auto cutoff = now - std::chrono::seconds(60);
                while (!creationTimes_.empty() && creationTimes_.front() < cutoff) {
                    creationTimes_.pop_front();
                }
                
                // Check if creation rate is too high
                if (creationTimes_.size() > config_.highCreationRateThreshold) {
                    AddAlert(AlertLevel::Warning, 
                             "High actor creation rate detected", 
                             "", "", static_cast<double>(creationTimes_.size()));
                }
            });
        
        // Monitor active actor count
        manager.RegisterHook(LifecycleEvent::PostActivate, "monitor_active_count",
            [this](LifecycleContext& ctx) {
                (void)ctx;  // Suppress unused parameter warning
                auto stats = ActorLifecycleManager::Instance().GetAllStats();
                size_t activeCount = 0;
                for (const auto& stat : stats) {
                    // GetAllStats returns vector, iterate through it
                    (void)stat;  // Each stat entry
                    activeCount++;
                }
                
                if (activeCount > config_.maxActiveActorsThreshold) {
                    AddAlert(AlertLevel::Error, 
                             "Maximum active actor threshold exceeded", 
                             "", "", static_cast<double>(activeCount));
                }
            });
    }
    
    void MonitoringLoop() {
        auto lastReport = std::chrono::high_resolution_clock::now();
        
        while (running_) {
            std::unique_lock<std::mutex> lk(mutex_);
            condition_.wait_for(lk, std::chrono::seconds(1), [this] { return !running_; });
            
            if (!running_) break;
            
            auto now = std::chrono::high_resolution_clock::now();
            
            // Generate periodic reports
            if (config_.enablePeriodicReports) {
                auto elapsed = std::chrono::duration<double>(now - lastReport).count();
                if (elapsed >= config_.reportIntervalSeconds) {
                    GeneratePeriodicReport();
                    lastReport = now;
                }
            }
            
            // Check for system health issues
            CheckSystemHealth();
        }
    }
    
    void GeneratePeriodicReport() {
        if (logFile_.is_open()) {
            logFile_ << "\n--- Periodic Report (" << GetTimestamp() << ") ---\n";
            logFile_ << LifecycleAnalytics::Instance().GenerateReport();
            logFile_ << "Monitor uptime: " << GetUptimeSeconds() << "s\n";
            logFile_ << "Total alerts: " << totalAlerts_ << "\n";
            logFile_.flush();
        }
        
        std::cout << "[LifecycleMonitor] Periodic report generated" << std::endl;
    }
    
    void CheckSystemHealth() {
        // Check for memory issues, stalled actors, etc.
        auto stats = ActorLifecycleManager::Instance().GetAllStats();
        
        // Check for very long-lived actors that might indicate leaks
        for (const auto& stat : stats) {
            double lifetime = stat.getLifetime();
            if (lifetime > 300.0) {  // 5 minutes
                AddAlert(AlertLevel::Info, 
                         "Long-lived actor detected (possible leak?)", 
                         "", "", lifetime);
            }
        }
    }
    
    std::string GetTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    double GetUptimeSeconds() const {
        if (!initialized_) return 0.0;
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - startTime_).count();
    }
    
    static std::string AlertLevelToString(AlertLevel level) {
        switch (level) {
            case AlertLevel::Info: return "INFO";
            case AlertLevel::Warning: return "WARN";
            case AlertLevel::Error: return "ERROR";
            case AlertLevel::Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    static std::string EscapeJson(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else result += c;
        }
        return result;
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool initialized_ = false;
    std::atomic<bool> running_{false};
    
    Config config_;
    std::thread monitoringThread_;
    std::ofstream logFile_;
    std::chrono::high_resolution_clock::time_point startTime_ = std::chrono::high_resolution_clock::now();
    
    // Alert management
    std::deque<LifecycleAlert> alerts_;
    size_t totalAlerts_ = 0;
    
    // Creation rate tracking
    std::deque<std::chrono::high_resolution_clock::time_point> creationTimes_;
};

// Console command integration for runtime monitoring access
class LifecycleConsoleCommands {
public:
    static void RegisterCommands() {
        // Register console commands if console system exists
        // This would integrate with the engine's console command system
        std::cout << "[LifecycleConsole] Monitoring commands available:" << std::endl;
        std::cout << "  lifecycle.status - Show real-time status" << std::endl;
        std::cout << "  lifecycle.dashboard - Print monitoring dashboard" << std::endl;
        std::cout << "  lifecycle.export - Export monitoring data to JSON" << std::endl;
        std::cout << "  lifecycle.analytics - Show analytics report" << std::endl;
    }
    
    static void ExecuteCommand(const std::string& command) {
        if (command == "lifecycle.status") {
            std::cout << LifecycleMonitor::Instance().GetRealtimeStatus() << std::endl;
        } else if (command == "lifecycle.dashboard") {
            LifecycleMonitor::Instance().PrintDashboard();
        } else if (command == "lifecycle.export") {
            std::cout << LifecycleMonitor::Instance().ExportMonitoringData() << std::endl;
        } else if (command == "lifecycle.analytics") {
            LifecycleAnalytics::Instance().PrintReport();
        } else {
            std::cout << "Unknown lifecycle command: " << command << std::endl;
        }
    }
};

// Utility functions for monitoring integration
namespace monitoring_utils {
    // Initialize complete monitoring system
    inline void InitializeMonitoringSystem() {
        LifecycleMonitor::Config config;
        config.enableRealTimeMonitoring = true;
        config.enablePeriodicReports = false;  // Disable for tests to avoid long waits
        config.enableAlerting = true;
        config.enableFileLogging = true;
        config.reportIntervalSeconds = 5.0;   // Shorter interval for testing
        config.slowInitThresholdSeconds = 0.5;  // 500ms threshold
        config.highCreationRateThreshold = 50;  // 50 actors per minute
        config.maxActiveActorsThreshold = 500;
        
        LifecycleMonitor::Instance().Initialize(config);
        LifecycleConsoleCommands::RegisterCommands();
        
        std::cout << "[LifecycleMonitoring] Complete monitoring system initialized" << std::endl;
    }
    
    // Shutdown monitoring system
    inline void ShutdownMonitoringSystem() {
        LifecycleMonitor::Instance().Shutdown();
        std::cout << "[LifecycleMonitoring] Monitoring system shutdown complete" << std::endl;
    }
    
    // Quick health check function
    inline void PrintQuickHealthCheck() {
        std::cout << "\n=== Quick Lifecycle Health Check ===" << std::endl;
        auto recentAlerts = LifecycleMonitor::Instance().GetRecentAlerts(5);
        if (recentAlerts.empty()) {
            std::cout << "No recent alerts - system healthy" << std::endl;
        } else {
            std::cout << "Recent alerts:" << std::endl;
            for (const auto& alert : recentAlerts) {
                std::cout << "  [" << (alert.level == AlertLevel::Critical ? "CRITICAL" :
                                       alert.level == AlertLevel::Error ? "ERROR" :
                                       alert.level == AlertLevel::Warning ? "WARN" : "INFO")
                          << "] " << alert.message << std::endl;
            }
        }
        std::cout << "===================================\n" << std::endl;
    }
}

} // namespace lifecycle