#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <algorithm>

namespace ContentManagement {

/**
 * ContentTestingFramework: Automated testing for content changes
 * 
 * Features:
 * - Unit tests for individual content items
 * - Integration tests for content relationships
 * - Load testing for performance validation
 * - Balance testing for gameplay metrics
 * - Regression testing for change validation
 * - Test automation and CI/CD integration
 */
class ContentTestingFramework {
public:
    enum class TestType {
        Unit,           // Single content item
        Integration,    // Multiple content items
        Performance,    // Load and stress testing
        Balance,        // Gameplay balance
        Regression,     // Prevent regressions
        Smoke,          // Quick sanity checks
        Acceptance      // User acceptance criteria
    };

    enum class TestResultStatus {
        Passed,
        Failed,
        Skipped,
        Warning,
        Error
    };

    struct TestCase {
        std::string testId;
        std::string name;
        std::string description;
        TestType type;
        std::vector<std::string> contentIds;  // Content to test
        std::function<bool(const std::vector<simplejson::JsonObject>&)> testFunction;
        int timeout;  // milliseconds
        std::vector<std::string> dependencies;  // Other tests that must pass first
        std::vector<std::string> tags;
        bool enabled;
    };

    struct TestExecution {
        std::string executionId;
        std::string testId;
        TestResultStatus result;
        std::string message;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        int duration;  // milliseconds
        std::vector<std::string> logs;
        std::string stackTrace;  // If failed
    };

    struct TestSuite {
        std::string suiteId;
        std::string name;
        std::string description;
        std::vector<std::string> testIds;
        bool runInParallel;
        bool stopOnFailure;
    };

    struct TestResult {
        std::string testId;
        std::string name;
        bool passed;
        std::string errorMessage;
        double executionTime;
    };

    struct TestReport {
        std::string reportId;
        std::chrono::system_clock::time_point timestamp;
        int totalTests;
        int passedTests;
        int failedTests;
        int skippedTests;
        int warningTests;
        float successRate;
        int totalDuration;
        std::vector<TestExecution> executions;
        std::vector<TestResult> tests;  // Simple test results
        std::string summary;
    };

    ContentTestingFramework();
    ~ContentTestingFramework();

    // Test Registration
    void RegisterTest(const TestCase& test);
    void RegisterTestSuite(const TestSuite& suite);
    void UnregisterTest(const std::string& testId);
    bool UpdateTest(const std::string& testId, const TestCase& test);
    
    const TestCase* GetTest(const std::string& testId) const;
    std::vector<std::string> GetAllTestIds() const;
    std::vector<std::string> GetTestsByType(TestType type) const;
    std::vector<std::string> GetTestsByTag(const std::string& tag) const;
    
    // Test Execution
    TestReport RunTest(const std::string& testId);  // Changed to return TestReport for consistency
    TestReport RunTestSuite(const std::string& suiteId);
    TestReport RunAllTests();
    TestReport RunTestsByTag(const std::string& tag);
    TestReport RunTestsForContent(const std::string& contentId);
    
    // Test Management
    void EnableTest(const std::string& testId, bool enabled);
    void SetTestTimeout(const std::string& testId, int timeoutMs);
    void ClearTestHistory();
    
    // Built-in Tests
    void RegisterBuiltInTests();
    
    // Schema validation test
    bool TestSchemaValidation(const simplejson::JsonObject& content, const std::string& schemaId);
    
    // Balance tests
    bool TestValueInRange(const simplejson::JsonObject& content, const std::string& field, float min, float max);
    bool TestRelativeBalance(const std::vector<simplejson::JsonObject>& content, const std::string& field, float maxDeviation);
    
    // Dependency tests
    bool TestDependenciesExist(const simplejson::JsonObject& content, const std::vector<std::string>& requiredIds);
    bool TestNoCyclicDependencies(const simplejson::JsonObject& content);
    
    // Performance tests
    bool TestLoadTime(const std::string& contentId, int maxMilliseconds);
    bool TestMemoryUsage(const std::string& contentId, size_t maxBytes);
    
    // Integration tests
    bool TestContentCompatibility(const std::vector<std::string>& contentIds);
    bool TestSystemIntegration(const std::string& contentId, const std::string& systemName);
    
    // Regression tests
    TestReport RunRegressionTests(const std::string& baselineReportId);
    void SetBaseline(const std::string& reportId);
    
    // Test Reporting
    TestReport GetLastTestReport() const;
    std::vector<TestReport> GetTestReports(int maxReports = 10) const;
    TestExecution GetLastExecution(const std::string& testId) const;
    std::vector<TestExecution> GetExecutionHistory(const std::string& testId, int maxExecutions = 10) const;
    
    std::string GenerateTestReport(const TestReport& report) const;
    std::string GenerateCoverageReport() const;
    void ExportTestResults(const std::string& outputPath, const std::string& format = "junit") const;
    
    // Test Discovery
    void ScanForTests(const std::string& directory);
    void AutoGenerateTests(const std::string& contentId);
    
    // CI/CD Integration
    int RunTestsAndExit();  // Returns exit code for CI systems
    void SetContinuousIntegrationMode(bool enabled);
    void SetFailFast(bool enabled);
    
    // Monitoring
    void SetOnTestStarted(std::function<void(const std::string&)> callback);
    void SetOnTestCompleted(std::function<void(const std::string&, TestResult)> callback);
    
    // UI Integration
    void RenderTestRunner();
    void RenderTestResults();
    void RenderTestCoverage();
    
    // Additional utilities
    void LoadTestsFromDirectory(const std::string& directory);
    
    struct TestStats {
        int totalTests;
        int passedTests;
        int failedTests;
    };
    TestStats GetTestStats() const;
    
private:
    bool ExecuteTest(const TestCase& test, TestExecution& execution);
    bool CheckDependencies(const std::string& testId, std::vector<std::string>& missing) const;
    void LogTestExecution(const TestExecution& execution);
    
    std::vector<simplejson::JsonObject> LoadTestContent(const std::vector<std::string>& contentIds);
    
    std::unordered_map<std::string, TestCase> tests_;
    std::unordered_map<std::string, TestSuite> testSuites_;
    std::vector<TestReport> testReports_;
    std::unordered_map<std::string, std::vector<TestExecution>> executionHistory_;
    
    std::string baselineReportId_;
    bool ciMode_;
    bool failFast_;
    
    std::function<void(const std::string&)> onTestStarted_;
    std::function<void(const std::string&, TestResult)> onTestCompleted_;
};

} // namespace ContentManagement
