#pragma once

#include "QueryBuilder.h"
#include "EntityManager.h"
#include <optional>
#include <variant>
#include <unordered_map>
#include <memory>
#include <sstream>

namespace ecs {

/**
 * Advanced Query Features
 * Implements optional components, change tracking, and optimization
 */

// Component version tracking for change detection
class ComponentVersionTracker {
public:
    using Version = uint64_t;
    
    // Increment version when component changes
    void IncrementVersion(std::type_index componentType) {
        ++componentVersions_[componentType];
    }
    
    // Get current version
    Version GetVersion(std::type_index componentType) const {
        auto it = componentVersions_.find(componentType);
        return it != componentVersions_.end() ? it->second : 0;
    }
    
    // Check if version changed
    bool HasChanged(std::type_index componentType, Version lastKnownVersion) const {
        return GetVersion(componentType) > lastKnownVersion;
    }
    
private:
    std::unordered_map<std::type_index, Version> componentVersions_;
};

// Query result with change tracking
template<typename ManagerType>
class VersionedQueryResult {
public:
    VersionedQueryResult() = default;
    
    VersionedQueryResult(std::vector<EntityHandle> results, 
                        const ComponentVersionTracker& tracker,
                        const std::vector<std::type_index>& componentTypes)
        : results_(std::move(results))
        , componentTypes_(componentTypes) {
        
        // Capture current versions
        for (const auto& type : componentTypes_) {
            versions_[type] = tracker.GetVersion(type);
        }
    }
    
    // Check if results are still valid
    bool IsValid(const ComponentVersionTracker& tracker) const {
        for (const auto& [type, version] : versions_) {
            if (tracker.GetVersion(type) > version) {
                return false;
            }
        }
        return true;
    }
    
    const std::vector<EntityHandle>& GetResults() const { return results_; }
    
    // Get versions for debugging
    const std::unordered_map<std::type_index, ComponentVersionTracker::Version>& GetVersions() const {
        return versions_;
    }
    
private:
    std::vector<EntityHandle> results_;
    std::vector<std::type_index> componentTypes_;
    std::unordered_map<std::type_index, ComponentVersionTracker::Version> versions_;
};

// Optional component query support (OR logic)
// Note: This provides a framework for OR queries with optional components
// Actual implementation depends on executing multiple queries and merging results
class OptionalComponentQuery {
public:
    OptionalComponentQuery(std::vector<EntityHandle> baseResults)
        : baseResults_(std::move(baseResults)) {}
    
    // Add additional entity results from optional component queries
    void AddOptionalResults(const std::vector<EntityHandle>& optionalResults) {
        // Merge results while maintaining uniqueness
        std::set<EntityHandle> merged(baseResults_.begin(), baseResults_.end());
        merged.insert(optionalResults.begin(), optionalResults.end());
        baseResults_.assign(merged.begin(), merged.end());
    }
    
    // Get merged results
    const std::vector<EntityHandle>& GetResults() const {
        return baseResults_;
    }
    
private:
    std::vector<EntityHandle> baseResults_;
};

// Query statistics for optimization
struct QueryStatistics {
    size_t totalExecutions = 0;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    std::chrono::microseconds totalExecutionTime{0};
    std::chrono::microseconds averageExecutionTime{0};
    size_t averageResultCount = 0;
    
    // Component frequency analysis
    std::unordered_map<std::type_index, size_t> componentFrequency;
    
    // Update statistics
    void RecordExecution(std::chrono::microseconds executionTime, size_t resultCount, bool cacheHit) {
        ++totalExecutions;
        
        if (cacheHit) {
            ++cacheHits;
        } else {
            ++cacheMisses;
        }
        
        totalExecutionTime += executionTime;
        averageExecutionTime = totalExecutionTime / totalExecutions;
        
        // Update rolling average for result count
        averageResultCount = (averageResultCount * (totalExecutions - 1) + resultCount) / totalExecutions;
    }
    
    // Get cache hit ratio
    double GetCacheHitRatio() const {
        return totalExecutions > 0 ? static_cast<double>(cacheHits) / totalExecutions : 0.0;
    }
    
    // Suggest optimization based on statistics
    std::vector<std::string> GetOptimizationSuggestions() const {
        std::vector<std::string> suggestions;
        
        if (GetCacheHitRatio() < 0.5 && totalExecutions > 100) {
            suggestions.push_back("Low cache hit ratio - consider implementing version-based invalidation");
        }
        
        if (averageResultCount > 10000) {
            suggestions.push_back("Large result sets - consider using streaming iterator or pagination");
        }
        
        if (averageExecutionTime.count() > 1000) {
            suggestions.push_back("Slow query execution - consider adding indices or reordering predicates");
        }
        
        return suggestions;
    }
};

// Query profiler for performance analysis
class QueryProfiler {
public:
    static QueryProfiler& GetInstance() {
        static QueryProfiler instance;
        return instance;
    }
    
    // Start profiling a query
    void BeginQuery(const std::string& queryName) {
        activeQueries_[queryName] = std::chrono::high_resolution_clock::now();
    }
    
    // End profiling and record statistics
    void EndQuery(const std::string& queryName, size_t resultCount, bool cacheHit) {
        auto it = activeQueries_.find(queryName);
        if (it == activeQueries_.end()) return;
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - it->second
        );
        
        statistics_[queryName].RecordExecution(duration, resultCount, cacheHit);
        activeQueries_.erase(it);
    }
    
    // Get statistics for a query
    const QueryStatistics* GetStatistics(const std::string& queryName) const {
        auto it = statistics_.find(queryName);
        return it != statistics_.end() ? &it->second : nullptr;
    }
    
    // Get all statistics
    const std::unordered_map<std::string, QueryStatistics>& GetAllStatistics() const {
        return statistics_;
    }
    
    // Generate profiling report
    std::string GenerateReport() const {
        std::ostringstream report;
        report << "=== Query Profiling Report ===\n\n";
        
        for (const auto& [name, stats] : statistics_) {
            report << "Query: " << name << "\n";
            report << "  Executions: " << stats.totalExecutions << "\n";
            report << "  Cache Hit Ratio: " << (stats.GetCacheHitRatio() * 100.0) << "%\n";
            report << "  Avg Execution Time: " << stats.averageExecutionTime.count() << " µs\n";
            report << "  Avg Result Count: " << stats.averageResultCount << "\n";
            
            auto suggestions = stats.GetOptimizationSuggestions();
            if (!suggestions.empty()) {
                report << "  Optimization Suggestions:\n";
                for (const auto& suggestion : suggestions) {
                    report << "    - " << suggestion << "\n";
                }
            }
            report << "\n";
        }
        
        return report.str();
    }
    
    // Clear all statistics
    void Reset() {
        statistics_.clear();
        activeQueries_.clear();
    }
    
private:
    QueryProfiler() = default;
    
    std::unordered_map<std::string, QueryStatistics> statistics_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeQueries_;
};

// RAII query profiler guard
class QueryProfileGuard {
public:
    QueryProfileGuard(const std::string& queryName, size_t resultCount, bool cacheHit)
        : queryName_(queryName), resultCount_(resultCount), cacheHit_(cacheHit) {
        QueryProfiler::GetInstance().BeginQuery(queryName_);
    }
    
    ~QueryProfileGuard() {
        QueryProfiler::GetInstance().EndQuery(queryName_, resultCount_, cacheHit_);
    }
    
private:
    std::string queryName_;
    size_t resultCount_;
    bool cacheHit_;
};

// Macro for automatic query profiling
#define PROFILE_QUERY(name, resultCount, cacheHit) \
    ecs::QueryProfileGuard profileGuard_##__LINE__(name, resultCount, cacheHit)

} // namespace ecs
