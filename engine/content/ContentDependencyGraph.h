#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace NovaEngine {

// Dependency tracking and analysis
class ContentDependencyGraph {
public:
    static ContentDependencyGraph& Instance() {
        static ContentDependencyGraph instance;
        return instance;
    }
    
    // Build dependency graph from current content
    void BuildGraph();
    void Clear();
    
    // Dependency queries
    std::vector<std::string> GetDependencies(const std::string& contentId) const;
    std::vector<std::string> GetDependents(const std::string& contentId) const;
    std::vector<std::string> GetTransitiveDependencies(const std::string& contentId) const;
    std::vector<std::string> GetTransitiveDependents(const std::string& contentId) const;
    
    // Cycle detection
    bool DetectCycles(std::vector<std::vector<std::string>>& cycles) const;
    bool HasCycle(const std::string& contentId) const;
    
    // Topological sorting
    std::vector<std::string> TopologicalSort() const;
    std::vector<std::string> GetLoadOrder() const;  // Optimal load order
    
    // Impact analysis
    struct ImpactAnalysis {
        std::string contentId;
        std::vector<std::string> directDependents;
        std::vector<std::string> transitiveDependents;
        size_t totalImpact;
    };
    
    ImpactAnalysis AnalyzeImpact(const std::string& contentId) const;
    
    // Visualization
    std::string ExportToDot() const;  // GraphViz DOT format
    std::string GenerateVisualReport(const std::string& outputPath) const;
    
    // Statistics
    struct GraphStats {
        size_t nodeCount = 0;
        size_t edgeCount = 0;
        size_t cyclicNodes = 0;
        size_t orphanNodes = 0;  // No dependencies or dependents
        float avgDependencies = 0.0f;
        float avgDependents = 0.0f;
        std::vector<std::pair<std::string, size_t>> mostDepended;  // Top 10
        std::vector<std::pair<std::string, size_t>> mostDependencies;  // Top 10
    };
    
    GraphStats GetStatistics() const;
    
private:
    ContentDependencyGraph() = default;
    
    void DFSCycleDetect(const std::string& node,
                       std::unordered_set<std::string>& visited,
                       std::unordered_set<std::string>& recStack,
                       std::vector<std::vector<std::string>>& cycles) const;
    
    void GetTransitiveDepsRecursive(const std::string& node,
                                   std::unordered_set<std::string>& result) const;
    
    // Adjacency list: contentId -> list of dependencies
    std::unordered_map<std::string, std::vector<std::string>> dependencies_;
    
    // Reverse adjacency list: contentId -> list of dependents
    std::unordered_map<std::string, std::vector<std::string>> dependents_;
};

// Dependency resolver for loading content in correct order
class ContentDependencyResolver {
public:
    static ContentDependencyResolver& Instance() {
        static ContentDependencyResolver instance;
        return instance;
    }
    
    // Resolve load order for a set of content
    std::vector<std::string> ResolveLoadOrder(
        const std::vector<std::string>& contentIds) const;
    
    // Check if dependencies are satisfied
    bool AreDependenciesSatisfied(const std::string& contentId) const;
    
    // Get missing dependencies
    std::vector<std::string> GetMissingDependencies(
        const std::string& contentId) const;
    
    // Batch resolution
    struct BatchResolution {
        std::vector<std::string> resolved;  // In load order
        std::vector<std::string> unresolved;  // Missing dependencies
        std::vector<std::vector<std::string>> cyclicGroups;
    };
    
    BatchResolution ResolveBatch(const std::vector<std::string>& contentIds) const;
    
private:
    ContentDependencyResolver() = default;
};

// Dependency change tracker
class DependencyChangeTracker {
public:
    enum class ChangeType {
        Added,
        Removed,
        Modified
    };
    
    struct DependencyChange {
        std::string contentId;
        ChangeType type;
        std::vector<std::string> affectedDependents;
        std::chrono::system_clock::time_point timestamp;
    };
    
    static DependencyChangeTracker& Instance() {
        static DependencyChangeTracker instance;
        return instance;
    }
    
    void RecordChange(const std::string& contentId, ChangeType type);
    std::vector<DependencyChange> GetRecentChanges(size_t count = 100) const;
    
    // Get all content affected by recent changes
    std::vector<std::string> GetAffectedContent(
        std::chrono::system_clock::time_point since) const;
    
    void Clear();
    
private:
    DependencyChangeTracker() = default;
    std::vector<DependencyChange> changes_;
};

} // namespace NovaEngine
