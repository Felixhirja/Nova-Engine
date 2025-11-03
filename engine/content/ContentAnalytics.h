#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include "ContentFramework.h"

namespace NovaEngine {

// Usage event tracking
struct ContentUsageEvent {
    std::string contentId;
    std::string eventType;  // "loaded", "accessed", "modified", "saved"
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> metadata;
    double durationMs = 0.0;
};

// Analytics collector
class ContentAnalytics {
public:
    static ContentAnalytics& Instance() {
        static ContentAnalytics instance;
        return instance;
    }
    
    // Event tracking
    void TrackEvent(const ContentUsageEvent& event);
    void TrackLoad(const std::string& contentId, double loadTimeMs);
    void TrackAccess(const std::string& contentId);
    void TrackModification(const std::string& contentId);
    
    // Query events
    std::vector<ContentUsageEvent> GetEvents(const std::string& contentId,
        std::chrono::system_clock::time_point since = {}) const;
    
    std::vector<ContentUsageEvent> GetEventsByType(const std::string& eventType,
        std::chrono::system_clock::time_point since = {}) const;
    
    // Statistics
    struct ContentStats {
        std::string contentId;
        size_t loadCount = 0;
        size_t accessCount = 0;
        size_t modificationCount = 0;
        double avgLoadTimeMs = 0.0;
        double totalLoadTimeMs = 0.0;
        std::chrono::system_clock::time_point firstAccess;
        std::chrono::system_clock::time_point lastAccess;
    };
    
    ContentStats GetContentStats(const std::string& contentId) const;
    std::vector<ContentStats> GetAllStats() const;
    
    // Aggregated analytics
    struct AggregatedStats {
        size_t totalContent = 0;
        size_t totalLoads = 0;
        size_t totalAccesses = 0;
        double avgLoadTimeMs = 0.0;
        
        std::vector<std::pair<std::string, size_t>> topLoaded;
        std::vector<std::pair<std::string, size_t>> topAccessed;
        std::vector<std::pair<std::string, double>> slowestToLoad;
        std::vector<std::pair<std::string, size_t>> mostModified;
        std::vector<std::pair<std::string, size_t>> unused;  // Never accessed
        
        std::unordered_map<std::string, size_t> usageByType;
        std::unordered_map<std::string, double> loadTimeByType;
    };
    
    AggregatedStats GetAggregatedStats(
        std::chrono::system_clock::time_point since = {}) const;
    
    // Time series data
    struct TimeSeriesData {
        std::vector<std::chrono::system_clock::time_point> timestamps;
        std::vector<size_t> values;
    };
    
    TimeSeriesData GetLoadTimeSeries(const std::string& contentId,
        std::chrono::hours interval = std::chrono::hours(1)) const;
    
    TimeSeriesData GetAccessTimeSeries(const std::string& contentId,
        std::chrono::hours interval = std::chrono::hours(1)) const;
    
    // Heat map data
    struct HeatMapData {
        std::vector<std::string> contentIds;
        std::vector<std::vector<size_t>> accessMatrix;  // [hour][contentId]
    };
    
    HeatMapData GetAccessHeatMap(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;
    
    // Export
    bool ExportToJson(const std::string& filepath) const;
    bool ExportToCsv(const std::string& filepath) const;
    
    // Visualization
    std::string GenerateReport(const std::string& outputDir) const;
    
    // Cleanup
    void ClearOldEvents(std::chrono::system_clock::time_point olderThan);
    void Clear();
    
private:
    ContentAnalytics() = default;
    
    std::vector<ContentUsageEvent> events_;
    mutable std::unordered_map<std::string, ContentStats> cachedStats_;
    mutable bool statsCacheDirty_ = true;
};

// Player engagement analytics
class PlayerEngagementAnalytics {
public:
    static PlayerEngagementAnalytics& Instance() {
        static PlayerEngagementAnalytics instance;
        return instance;
    }
    
    struct EngagementMetrics {
        std::string contentId;
        
        // Engagement scores
        double popularityScore = 0.0;  // Based on access frequency
        double utilityScore = 0.0;     // How often used vs loaded
        double retentionScore = 0.0;   // Repeat usage
        
        // Player behavior
        size_t uniquePlayers = 0;
        double avgSessionsPerPlayer = 0.0;
        double avgDurationPerSession = 0.0;
        
        // Temporal patterns
        std::vector<int> hourlyUsage;  // Usage by hour of day
        std::vector<int> dailyUsage;   // Usage by day of week
    };
    
    void TrackPlayerInteraction(const std::string& playerId, 
                               const std::string& contentId,
                               double durationSec);
    
    EngagementMetrics GetEngagementMetrics(const std::string& contentId) const;
    
    std::vector<std::pair<std::string, double>> GetTopEngagingContent(size_t count = 10) const;
    std::vector<std::pair<std::string, double>> GetLowEngagementContent(size_t count = 10) const;
    
    // Recommendations
    std::vector<std::string> GetRecommendationsFor(const std::string& playerId, 
                                                   size_t count = 5) const;
    
    std::vector<std::string> GetSimilarContent(const std::string& contentId,
                                               size_t count = 5) const;
    
private:
    PlayerEngagementAnalytics() = default;
    
    struct PlayerInteraction {
        std::string playerId;
        std::string contentId;
        std::chrono::system_clock::time_point timestamp;
        double durationSec;
    };
    
    std::vector<PlayerInteraction> interactions_;
};

// Content health monitoring
class ContentHealthMonitor {
public:
    static ContentHealthMonitor& Instance() {
        static ContentHealthMonitor instance;
        return instance;
    }
    
    struct HealthScore {
        std::string contentId;
        double overallScore = 100.0;  // 0-100
        
        double validationScore = 100.0;
        double performanceScore = 100.0;
        double usageScore = 100.0;
        double qualityScore = 100.0;
        
        std::vector<std::string> issues;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
    };
    
    HealthScore EvaluateHealth(const std::string& contentId) const;
    
    std::vector<std::pair<std::string, HealthScore>> GetUnhealthyContent(
        double threshold = 70.0) const;
    
    // Monitoring
    void StartMonitoring(std::chrono::seconds interval = std::chrono::seconds(60));
    void StopMonitoring();
    
    // Alerts
    using AlertCallback = std::function<void(const std::string&, const HealthScore&)>;
    void RegisterAlertCallback(AlertCallback callback);
    
private:
    ContentHealthMonitor() = default;
    
    bool monitoring_ = false;
    std::vector<AlertCallback> callbacks_;
};

// A/B testing framework
class ContentABTesting {
public:
    static ContentABTesting& Instance() {
        static ContentABTesting instance;
        return instance;
    }
    
    struct ABTest {
        std::string testId;
        std::string name;
        std::string description;
        std::vector<std::string> variants;  // Content IDs for variants
        std::unordered_map<std::string, double> variantWeights;
        
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        
        bool active = false;
    };
    
    struct ABTestResult {
        std::string testId;
        std::unordered_map<std::string, size_t> variantUsage;
        std::unordered_map<std::string, double> variantPerformance;
        std::string winner;
        double confidence = 0.0;
    };
    
    void CreateTest(const ABTest& test);
    std::string SelectVariant(const std::string& testId, const std::string& userId);
    void RecordResult(const std::string& testId, const std::string& variantId,
                     double performanceMetric);
    
    ABTestResult GetTestResults(const std::string& testId) const;
    void EndTest(const std::string& testId);
    
private:
    ContentABTesting() = default;
    
    std::unordered_map<std::string, ABTest> tests_;
    std::unordered_map<std::string, ABTestResult> results_;
};

} // namespace NovaEngine
