#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "ContentFramework.h"

namespace NovaEngine {

// Test result
struct ContentTestResult {
    std::string testName;
    bool passed;
    std::string message;
    double executionTimeMs;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// Base test interface
class IContentTest {
public:
    virtual ~IContentTest() = default;
    
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual bool SupportsType(const std::string& typeName) const = 0;
    
    virtual ContentTestResult Execute(const ContentDefinition& content) = 0;
};

// Balance test - ensures content is balanced
class BalanceTest : public IContentTest {
public:
    struct BalanceConstraint {
        std::string field;
        double minValue;
        double maxValue;
        double weight = 1.0;  // For composite scoring
    };
    
    BalanceTest(const std::string& typeName, const std::string& name)
        : typeName_(typeName), name_(name) {}
    
    void AddConstraint(const BalanceConstraint& constraint) {
        constraints_.push_back(constraint);
    }
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override {
        return "Balance test for " + typeName_;
    }
    bool SupportsType(const std::string& typeName) const override {
        return typeName == typeName_;
    }
    
    ContentTestResult Execute(const ContentDefinition& content) override;
    
private:
    std::string typeName_;
    std::string name_;
    std::vector<BalanceConstraint> constraints_;
};

// Integration test - tests content with dependencies
class IntegrationTest : public IContentTest {
public:
    IntegrationTest(const std::string& name, const std::string& typeName)
        : name_(name), typeName_(typeName) {}
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override {
        return "Integration test for " + typeName_;
    }
    bool SupportsType(const std::string& typeName) const override {
        return typeName == typeName_;
    }
    
    ContentTestResult Execute(const ContentDefinition& content) override;
    
private:
    std::string name_;
    std::string typeName_;
};

// Regression test - ensures content hasn't regressed
class RegressionTest : public IContentTest {
public:
    RegressionTest(const std::string& name, const std::string& contentId)
        : name_(name), contentId_(contentId) {}
    
    void SetBaseline(const SimpleJson& baseline) {
        baseline_ = baseline;
    }
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override {
        return "Regression test for " + contentId_;
    }
    bool SupportsType(const std::string& typeName) const override {
        return true;  // Works for all types
    }
    
    ContentTestResult Execute(const ContentDefinition& content) override;
    
private:
    std::string name_;
    std::string contentId_;
    SimpleJson baseline_;
};

// Performance test - tests content load/processing performance
class PerformanceTest : public IContentTest {
public:
    PerformanceTest(const std::string& name, double maxTimeMs)
        : name_(name), maxTimeMs_(maxTimeMs) {}
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override {
        return "Performance test (max " + std::to_string(maxTimeMs_) + "ms)";
    }
    bool SupportsType(const std::string& typeName) const override {
        return true;
    }
    
    ContentTestResult Execute(const ContentDefinition& content) override;
    
private:
    std::string name_;
    double maxTimeMs_;
};

// Custom test
class CustomContentTest : public IContentTest {
public:
    using TestFunc = std::function<ContentTestResult(const ContentDefinition&)>;
    
    CustomContentTest(const std::string& name, const std::string& description,
                     const std::string& typeName, TestFunc func)
        : name_(name), description_(description), typeName_(typeName), 
          testFunc_(func) {}
    
    std::string GetName() const override { return name_; }
    std::string GetDescription() const override { return description_; }
    bool SupportsType(const std::string& typeName) const override {
        return typeName_ == "*" || typeName == typeName_;
    }
    
    ContentTestResult Execute(const ContentDefinition& content) override {
        return testFunc_(content);
    }
    
private:
    std::string name_;
    std::string description_;
    std::string typeName_;
    TestFunc testFunc_;
};

// Content test suite
class ContentTestSuite {
public:
    ContentTestSuite(const std::string& name) : name_(name) {}
    
    void AddTest(std::shared_ptr<IContentTest> test) {
        tests_.push_back(test);
    }
    
    std::vector<ContentTestResult> Run(const ContentDefinition& content);
    std::vector<ContentTestResult> RunAll();  // Run on all content
    
    const std::string& GetName() const { return name_; }
    size_t GetTestCount() const { return tests_.size(); }
    
private:
    std::string name_;
    std::vector<std::shared_ptr<IContentTest>> tests_;
};

// Content test registry
class ContentTestRegistry {
public:
    static ContentTestRegistry& Instance() {
        static ContentTestRegistry instance;
        return instance;
    }
    
    void RegisterTest(std::shared_ptr<IContentTest> test);
    void RegisterSuite(std::shared_ptr<ContentTestSuite> suite);
    
    std::vector<std::shared_ptr<IContentTest>> GetTestsForType(
        const std::string& typeName) const;
    
    std::shared_ptr<ContentTestSuite> GetSuite(const std::string& name) const;
    
    // Run tests
    std::vector<ContentTestResult> RunTests(const std::string& contentId);
    std::vector<ContentTestResult> RunAllTests();
    std::vector<ContentTestResult> RunSuite(const std::string& suiteName);
    
    // Generate report
    struct TestReport {
        size_t totalTests = 0;
        size_t passed = 0;
        size_t failed = 0;
        double totalTimeMs = 0.0;
        std::vector<ContentTestResult> results;
    };
    
    TestReport GenerateReport(const std::vector<ContentTestResult>& results) const;
    std::string ExportReport(const TestReport& report, const std::string& format = "markdown") const;
    
private:
    ContentTestRegistry() = default;
    
    std::vector<std::shared_ptr<IContentTest>> tests_;
    std::unordered_map<std::string, std::shared_ptr<ContentTestSuite>> suites_;
};

// Automated test runner
class ContentTestRunner {
public:
    static ContentTestRunner& Instance() {
        static ContentTestRunner instance;
        return instance;
    }
    
    struct TestConfig {
        bool stopOnFailure = false;
        bool verbose = false;
        std::vector<std::string> includeTags;
        std::vector<std::string> excludeTags;
        int parallelJobs = 1;
    };
    
    void SetConfig(const TestConfig& config) { config_ = config; }
    
    // Run tests
    ContentTestRegistry::TestReport RunAllTests();
    ContentTestRegistry::TestReport RunTestsForContent(const std::string& contentId);
    ContentTestRegistry::TestReport RunTestsForType(const std::string& typeName);
    
    // Watch mode - run tests on content changes
    void StartWatchMode();
    void StopWatchMode();
    
private:
    ContentTestRunner() = default;
    
    TestConfig config_;
    bool watchMode_ = false;
};

} // namespace NovaEngine
