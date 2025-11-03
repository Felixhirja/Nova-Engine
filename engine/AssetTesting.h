#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace AssetPipeline {

/**
 * Asset Testing System
 * Automated testing framework for asset systems
 */

enum class TestStatus {
    NotRun,
    Running,
    Passed,
    Failed,
    Skipped,
    Error
};

struct TestResult {
    std::string test_name;
    TestStatus status = TestStatus::NotRun;
    std::string message;
    std::chrono::milliseconds duration{0};
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> warnings;
};

struct TestSuite {
    std::string name;
    std::vector<std::string> test_names;
    size_t total_tests = 0;
    size_t passed = 0;
    size_t failed = 0;
    size_t skipped = 0;
    std::chrono::milliseconds total_duration{0};
    std::vector<TestResult> results;
};

class AssetTestFramework {
public:
    static AssetTestFramework& GetInstance();

    // Test registration
    using TestFunction = std::function<bool()>;
    void RegisterTest(const std::string& suite_name, const std::string& test_name, TestFunction test);
    void RegisterSetup(const std::string& suite_name, std::function<void()> setup);
    void RegisterTeardown(const std::string& suite_name, std::function<void()> teardown);
    
    // Test execution
    TestResult RunTest(const std::string& suite_name, const std::string& test_name);
    TestSuite RunTestSuite(const std::string& suite_name);
    std::vector<TestSuite> RunAllTests();
    
    // Selective testing
    std::vector<TestSuite> RunTestsMatching(const std::string& pattern);
    TestSuite RunTestsForAsset(const std::string& asset_path);
    
    // Built-in test suites
    TestSuite RunValidationTests();
    TestSuite RunDependencyTests();
    TestSuite RunLoadingTests();
    TestSuite RunCachingTests();
    TestSuite RunStreamingTests();
    TestSuite RunIntegrationTests();
    
    // Assertions
    class Assert {
    public:
        static void IsTrue(bool condition, const std::string& message = "");
        static void IsFalse(bool condition, const std::string& message = "");
        static void AreEqual(const std::string& a, const std::string& b, const std::string& message = "");
        static void AreNotEqual(const std::string& a, const std::string& b, const std::string& message = "");
        static void IsNull(void* ptr, const std::string& message = "");
        static void IsNotNull(void* ptr, const std::string& message = "");
        static void Throws(std::function<void()> func, const std::string& message = "");
    };
    
    // Asset validation tests
    bool TestAssetExists(const std::string& path);
    bool TestAssetLoads(const std::string& path);
    bool TestAssetValidates(const std::string& path);
    bool TestAssetHasValidDependencies(const std::string& path);
    bool TestAssetOptimizes(const std::string& path);
    bool TestAssetCompresses(const std::string& path);
    
    // Performance tests
    bool TestLoadTimeUnder(const std::string& path, std::chrono::milliseconds max_time);
    bool TestMemoryUsageUnder(const std::string& path, size_t max_bytes);
    bool TestCacheHitRate(float min_rate);
    
    // Stress tests
    bool StressTestConcurrentLoads(size_t asset_count, size_t thread_count);
    bool StressTestMemoryPressure(size_t target_mb);
    bool StressTestRapidReloads(const std::string& path, size_t iterations);
    
    // Mock assets for testing
    std::string CreateMockAsset(AssetType type, size_t size_kb = 1);
    void RemoveMockAsset(const std::string& path);
    void CleanupMockAssets();
    
    // Test reports
    std::string GenerateTestReport(const TestSuite& suite);
    std::string GenerateTestReport(const std::vector<TestSuite>& suites);
    bool ExportTestResults(const std::string& file_path, const std::vector<TestSuite>& suites);
    
    // CI/CD integration
    int GetExitCode(const std::vector<TestSuite>& suites); // 0 = all passed, 1 = failures
    bool ExportJUnitXML(const std::string& file_path, const std::vector<TestSuite>& suites);
    
    // Coverage tracking
    struct CoverageData {
        size_t total_assets = 0;
        size_t tested_assets = 0;
        float coverage_percentage = 0.0f;
        std::vector<std::string> untested_assets;
    };
    
    CoverageData GetCoverageData();
    
    // Continuous testing
    void EnableContinuousTesting(bool enable);
    void SetTestInterval(std::chrono::seconds interval);
    
private:
    AssetTestFramework() = default;
    
    struct Test {
        std::string suite_name;
        std::string test_name;
        TestFunction function;
    };
    
    std::vector<Test> tests_;
    std::unordered_map<std::string, std::function<void()>> setup_functions_;
    std::unordered_map<std::string, std::function<void()>> teardown_functions_;
    std::vector<std::string> mock_assets_;
    
    std::atomic<bool> continuous_testing_{false};
    std::chrono::seconds test_interval_{60};
    
    mutable std::mutex mutex_;
};

// Test helper macros
#define ASSET_TEST(suite, name) \
    struct Test_##suite##_##name { \
        Test_##suite##_##name() { \
            AssetPipeline::AssetTestFramework::GetInstance().RegisterTest( \
                #suite, #name, []() -> bool

#define ASSET_TEST_END \
            ); \
        } \
    } test_instance_##__LINE__;

} // namespace AssetPipeline
