#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace ContentManagement {

/**
 * ContentAnalytics: Track content usage and player engagement
 * 
 * Features:
 * - Usage tracking for all content types
 * - Player engagement metrics
 * - Content popularity analysis
 * - Balance data collection
 * - Heat maps and visualization data
 * - A/B testing support
 */
class ContentAnalytics {
public:
    struct UsageMetric {
        std::string contentId;
        std::string contentType;
        int useCount;
        int uniqueUsers;
        std::chrono::system_clock::time_point firstUsed;
        std::chrono::system_clock::time_point lastUsed;
        double averageSessionDuration;
        double totalUsageTime;
    };

    struct EngagementMetric {
        std::string contentId;
        double completionRate;        // % of players who finish/complete
        double retentionRate;          // % who return to use again
        double satisfactionScore;      // Player satisfaction (0-10)
        int abandonmentCount;          // Times players quit while using
        double averageTimeToAbandon;   // How long before they quit
    };

    struct BalanceMetric {
        std::string contentId;
        std::string metricName;
        double minValue;
        double maxValue;
        double averageValue;
        double medianValue;
        double standardDeviation;
        int sampleSize;
    };

    struct PopularityReport {
        std::vector<std::pair<std::string, int>> mostUsed;
        std::vector<std::pair<std::string, int>> leastUsed;
        std::vector<std::pair<std::string, double>> mostEngaging;
        std::vector<std::pair<std::string, double>> trending;  // Recently increasing usage
        std::vector<std::string> unused;  // Never been used
    };

    struct ABTestResult {
        std::string testId;
        std::string variantA;
        std::string variantB;
        int usageA;
        int usageB;
        double engagementA;
        double engagementB;
        double statisticalSignificance;
        std::string recommendation;
    };

    ContentAnalytics();
    ~ContentAnalytics();

    // Tracking
    void TrackContentUsage(const std::string& contentId, const std::string& contentType, const std::string& userId);
    void TrackContentCompletion(const std::string& contentId, const std::string& userId, bool completed);
    void TrackContentAbandonment(const std::string& contentId, const std::string& userId, double timeUsed);
    void TrackBalanceMetric(const std::string& contentId, const std::string& metricName, double value);
    void TrackSessionDuration(const std::string& contentId, const std::string& userId, double duration);
    
    // Queries
    UsageMetric GetUsageMetric(const std::string& contentId) const;
    EngagementMetric GetEngagementMetric(const std::string& contentId) const;
    std::vector<BalanceMetric> GetBalanceMetrics(const std::string& contentId) const;
    
    // Analysis
    PopularityReport GeneratePopularityReport(const std::string& contentType = "") const;
    std::vector<std::string> GetUnderutilizedContent(double thresholdPercentage = 0.1) const;
    std::vector<std::string> GetOverusedContent(double thresholdPercentage = 0.5) const;
    std::vector<std::string> GetBalanceOutliers(const std::string& metricName, double deviationThreshold = 2.0) const;
    
    // A/B Testing
    void StartABTest(const std::string& testId, const std::string& variantA, const std::string& variantB);
    void EndABTest(const std::string& testId);
    ABTestResult GetABTestResult(const std::string& testId) const;
    std::vector<std::string> GetActiveABTests() const;
    
    // Reporting
    std::string GenerateUsageReport(const std::string& contentId) const;
    std::string GenerateEngagementReport(const std::string& contentType = "") const;
    std::string GenerateBalanceReport(const std::string& contentType = "") const;
    std::string GenerateComprehensiveReport() const;
    
    // Export
    void ExportToCSV(const std::string& outputPath) const;
    void ExportToJSON(const std::string& outputPath) const;
    void ExportHeatmapData(const std::string& outputPath, const std::string& metricName) const;
    
    // Time-based Analysis
    std::vector<UsageMetric> GetUsageByTimeRange(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) const;
    std::vector<std::pair<std::string, int>> GetTrendingContent(int days = 7) const;
    std::vector<std::pair<std::string, int>> GetDecliningContent(int days = 7) const;
    
    // Configuration
    void SetTrackingEnabled(bool enabled);
    void SetSamplingRate(double rate);  // 0.0 to 1.0, for high-frequency events
    void ClearOldData(int daysToKeep);
    
    // Persistence
    bool LoadAnalytics(const std::string& filePath);
    bool SaveAnalytics(const std::string& filePath) const;
    
private:
    struct ABTest {
        std::string testId;
        std::string variantA;
        std::string variantB;
        std::chrono::system_clock::time_point startTime;
        bool active;
    };
    
    void UpdateMetric(UsageMetric& metric, const std::string& userId, double duration);
    double CalculateStatisticalSignificance(int sampleA, int sampleB, double metricA, double metricB) const;
    
    std::unordered_map<std::string, UsageMetric> usageMetrics_;
    std::unordered_map<std::string, EngagementMetric> engagementMetrics_;
    std::unordered_map<std::string, std::vector<BalanceMetric>> balanceMetrics_;
    std::unordered_map<std::string, ABTest> abTests_;
    
    bool trackingEnabled_;
    double samplingRate_;
};

} // namespace ContentManagement
