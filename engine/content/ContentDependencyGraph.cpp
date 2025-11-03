#include "ContentDependencyGraph.h"
#include "ContentFramework.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>

namespace NovaEngine {

// ContentDependencyGraph implementation
void ContentDependencyGraph::BuildGraph() {
    Clear();
    
    auto& registry = ContentRegistry::Instance();
    auto allContent = registry.QueryContent([](const ContentDefinition*) { return true; });
    
    // Build forward and reverse adjacency lists
    for (const auto* content : allContent) {
        const std::string& id = content->GetId();
        auto deps = content->GetDependencies();
        
        dependencies_[id] = deps;
        
        for (const auto& dep : deps) {
            dependents_[dep].push_back(id);
        }
    }
}

void ContentDependencyGraph::Clear() {
    dependencies_.clear();
    dependents_.clear();
}

std::vector<std::string> ContentDependencyGraph::GetDependencies(
    const std::string& contentId) const {
    auto it = dependencies_.find(contentId);
    return it != dependencies_.end() ? it->second : std::vector<std::string>();
}

std::vector<std::string> ContentDependencyGraph::GetDependents(
    const std::string& contentId) const {
    auto it = dependents_.find(contentId);
    return it != dependents_.end() ? it->second : std::vector<std::string>();
}

std::vector<std::string> ContentDependencyGraph::GetTransitiveDependencies(
    const std::string& contentId) const {
    std::unordered_set<std::string> result;
    GetTransitiveDepsRecursive(contentId, result);
    return std::vector<std::string>(result.begin(), result.end());
}

void ContentDependencyGraph::GetTransitiveDepsRecursive(
    const std::string& node, std::unordered_set<std::string>& result) const {
    auto deps = GetDependencies(node);
    
    for (const auto& dep : deps) {
        if (result.find(dep) == result.end()) {
            result.insert(dep);
            GetTransitiveDepsRecursive(dep, result);
        }
    }
}

std::vector<std::string> ContentDependencyGraph::GetTransitiveDependents(
    const std::string& contentId) const {
    std::unordered_set<std::string> result;
    std::queue<std::string> queue;
    queue.push(contentId);
    
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        
        auto deps = GetDependents(current);
        for (const auto& dep : deps) {
            if (result.find(dep) == result.end()) {
                result.insert(dep);
                queue.push(dep);
            }
        }
    }
    
    return std::vector<std::string>(result.begin(), result.end());
}

bool ContentDependencyGraph::DetectCycles(
    std::vector<std::vector<std::string>>& cycles) const {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recStack;
    
    for (const auto& pair : dependencies_) {
        if (visited.find(pair.first) == visited.end()) {
            DFSCycleDetect(pair.first, visited, recStack, cycles);
        }
    }
    
    return !cycles.empty();
}

void ContentDependencyGraph::DFSCycleDetect(
    const std::string& node,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recStack,
    std::vector<std::vector<std::string>>& cycles) const {
    
    visited.insert(node);
    recStack.insert(node);
    
    auto deps = GetDependencies(node);
    for (const auto& dep : deps) {
        if (visited.find(dep) == visited.end()) {
            DFSCycleDetect(dep, visited, recStack, cycles);
        } else if (recStack.find(dep) != recStack.end()) {
            // Found cycle
            std::vector<std::string> cycle;
            cycle.push_back(dep);
            cycle.push_back(node);
            cycles.push_back(cycle);
        }
    }
    
    recStack.erase(node);
}

bool ContentDependencyGraph::HasCycle(const std::string& contentId) const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recStack;
    
    DFSCycleDetect(contentId, visited, recStack, cycles);
    
    return !cycles.empty();
}

std::vector<std::string> ContentDependencyGraph::TopologicalSort() const {
    std::vector<std::string> result;
    std::unordered_map<std::string, int> inDegree;
    
    // Calculate in-degrees
    for (const auto& pair : dependencies_) {
        if (inDegree.find(pair.first) == inDegree.end()) {
            inDegree[pair.first] = 0;
        }
        
        for (const auto& dep : pair.second) {
            inDegree[dep]++;
        }
    }
    
    // Find nodes with in-degree 0
    std::queue<std::string> queue;
    for (const auto& pair : dependencies_) {
        if (inDegree[pair.first] == 0) {
            queue.push(pair.first);
        }
    }
    
    // Process nodes
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        result.push_back(current);
        
        auto deps = GetDependents(current);
        for (const auto& dep : deps) {
            inDegree[dep]--;
            if (inDegree[dep] == 0) {
                queue.push(dep);
            }
        }
    }
    
    return result;
}

std::vector<std::string> ContentDependencyGraph::GetLoadOrder() const {
    return TopologicalSort();
}

ContentDependencyGraph::ImpactAnalysis ContentDependencyGraph::AnalyzeImpact(
    const std::string& contentId) const {
    ImpactAnalysis analysis;
    analysis.contentId = contentId;
    analysis.directDependents = GetDependents(contentId);
    analysis.transitiveDependents = GetTransitiveDependents(contentId);
    analysis.totalImpact = analysis.transitiveDependents.size();
    
    return analysis;
}

std::string ContentDependencyGraph::ExportToDot() const {
    std::ostringstream dot;
    
    dot << "digraph ContentDependencies {\n";
    dot << "  rankdir=LR;\n";
    dot << "  node [shape=box];\n\n";
    
    for (const auto& pair : dependencies_) {
        for (const auto& dep : pair.second) {
            dot << "  \"" << pair.first << "\" -> \"" << dep << "\";\n";
        }
    }
    
    dot << "}\n";
    
    return dot.str();
}

std::string ContentDependencyGraph::GenerateVisualReport(const std::string& outputPath) const {
    // Generate DOT file
    std::string dotContent = ExportToDot();
    
    // Could integrate with GraphViz here to generate actual images
    return dotContent;
}

ContentDependencyGraph::GraphStats ContentDependencyGraph::GetStatistics() const {
    GraphStats stats;
    
    stats.nodeCount = dependencies_.size();
    
    size_t totalDeps = 0;
    size_t totalDependents = 0;
    
    std::vector<std::pair<std::string, size_t>> depCounts;
    std::vector<std::pair<std::string, size_t>> dependentCounts;
    
    for (const auto& pair : dependencies_) {
        size_t depCount = pair.second.size();
        totalDeps += depCount;
        stats.edgeCount += depCount;
        
        depCounts.push_back({pair.first, depCount});
        
        auto deps = GetDependents(pair.first);
        size_t dependentCount = deps.size();
        totalDependents += dependentCount;
        
        dependentCounts.push_back({pair.first, dependentCount});
        
        // Check for orphans
        if (depCount == 0 && dependentCount == 0) {
            stats.orphanNodes++;
        }
    }
    
    if (stats.nodeCount > 0) {
        stats.avgDependencies = static_cast<float>(totalDeps) / stats.nodeCount;
        stats.avgDependents = static_cast<float>(totalDependents) / stats.nodeCount;
    }
    
    // Sort and get top 10
    auto sortFunc = [](const auto& a, const auto& b) { return a.second > b.second; };
    
    std::sort(depCounts.begin(), depCounts.end(), sortFunc);
    std::sort(dependentCounts.begin(), dependentCounts.end(), sortFunc);
    
    stats.mostDependencies = std::vector<std::pair<std::string, size_t>>(
        depCounts.begin(), depCounts.begin() + std::min<size_t>(10, depCounts.size()));
    
    stats.mostDepended = std::vector<std::pair<std::string, size_t>>(
        dependentCounts.begin(), dependentCounts.begin() + std::min<size_t>(10, dependentCounts.size()));
    
    return stats;
}

// ContentDependencyResolver implementation
std::vector<std::string> ContentDependencyResolver::ResolveLoadOrder(
    const std::vector<std::string>& contentIds) const {
    
    auto& graph = ContentDependencyGraph::Instance();
    std::unordered_set<std::string> required(contentIds.begin(), contentIds.end());
    std::unordered_set<std::string> allDeps;
    
    // Collect all dependencies
    for (const auto& id : contentIds) {
        auto deps = graph.GetTransitiveDependencies(id);
        allDeps.insert(deps.begin(), deps.end());
    }
    
    // Combine with required content
    allDeps.insert(contentIds.begin(), contentIds.end());
    
    // Get topological order
    auto fullOrder = graph.GetLoadOrder();
    
    // Filter to only include required content and dependencies
    std::vector<std::string> result;
    for (const auto& id : fullOrder) {
        if (allDeps.find(id) != allDeps.end()) {
            result.push_back(id);
        }
    }
    
    return result;
}

bool ContentDependencyResolver::AreDependenciesSatisfied(
    const std::string& contentId) const {
    
    auto missing = GetMissingDependencies(contentId);
    return missing.empty();
}

std::vector<std::string> ContentDependencyResolver::GetMissingDependencies(
    const std::string& contentId) const {
    
    std::vector<std::string> missing;
    auto& registry = ContentRegistry::Instance();
    auto& graph = ContentDependencyGraph::Instance();
    
    auto deps = graph.GetDependencies(contentId);
    for (const auto& dep : deps) {
        if (!registry.GetContent(dep)) {
            missing.push_back(dep);
        }
    }
    
    return missing;
}

ContentDependencyResolver::BatchResolution ContentDependencyResolver::ResolveBatch(
    const std::vector<std::string>& contentIds) const {
    
    BatchResolution result;
    auto& graph = ContentDependencyGraph::Instance();
    
    // Detect cycles
    graph.DetectCycles(result.cyclicGroups);
    
    // Get load order
    result.resolved = ResolveLoadOrder(contentIds);
    
    // Check for unresolved
    for (const auto& id : contentIds) {
        auto missing = GetMissingDependencies(id);
        result.unresolved.insert(result.unresolved.end(), missing.begin(), missing.end());
    }
    
    return result;
}

// DependencyChangeTracker implementation
void DependencyChangeTracker::RecordChange(const std::string& contentId, ChangeType type) {
    auto& graph = ContentDependencyGraph::Instance();
    
    DependencyChange change;
    change.contentId = contentId;
    change.type = type;
    change.affectedDependents = graph.GetTransitiveDependents(contentId);
    change.timestamp = std::chrono::system_clock::now();
    
    changes_.push_back(change);
}

std::vector<DependencyChangeTracker::DependencyChange> 
DependencyChangeTracker::GetRecentChanges(size_t count) const {
    size_t start = changes_.size() > count ? changes_.size() - count : 0;
    return std::vector<DependencyChange>(changes_.begin() + start, changes_.end());
}

std::vector<std::string> DependencyChangeTracker::GetAffectedContent(
    std::chrono::system_clock::time_point since) const {
    
    std::unordered_set<std::string> affected;
    
    for (const auto& change : changes_) {
        if (change.timestamp >= since) {
            affected.insert(change.contentId);
            affected.insert(change.affectedDependents.begin(), 
                          change.affectedDependents.end());
        }
    }
    
    return std::vector<std::string>(affected.begin(), affected.end());
}

void DependencyChangeTracker::Clear() {
    changes_.clear();
}

} // namespace NovaEngine
