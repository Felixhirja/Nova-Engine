#pragma once
#include "EntityManager.h"
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

namespace ecs {

// Memory optimization utilities for ECS
// TODO: Add memory pool management for component storage
// TODO: Implement defragmentation algorithms for better memory layout
// TODO: Add memory usage tracking per component type
// TODO: Implement automatic memory optimization triggers
// TODO: Add memory profiling with allocation/deallocation tracking
// TODO: Support custom allocators for different component types
// TODO: Add memory budget management and overflow detection
// TODO: Implement memory usage predictions based on entity patterns

// Optimization strategy options
enum class OptimizationStrategy {
    Conservative,   // Minimal impact, safe optimizations
    Balanced,      // Good balance of performance and safety
    Aggressive     // Maximum optimization, may impact frame time
};

// Memory pressure levels
enum class MemoryPressureLevel {
    Low,      // Normal operation
    Medium,   // Some optimization recommended
    High,     // Immediate optimization needed
    Critical  // Emergency cleanup required
};

// Component memory information
struct ComponentMemoryInfo {
    std::string componentName;
    size_t totalSize = 0;
    size_t instanceCount = 0;
    size_t averageSize = 0;
    double fragmentationRatio = 0.0;
};
class MemoryOptimizer {
public:
    // TODO: Add more detailed memory statistics
    // TODO: Track memory per component type
    // TODO: Add peak memory usage tracking
    // TODO: Implement memory leak detection
    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalUsed = 0;
        size_t wastedSpace = 0;
        size_t archetypeCount = 0;
        size_t emptyArchetypes = 0;
        double fragmentationRatio = 0.0;
        
        // Enhanced statistics
        size_t peakMemoryUsage = 0;
        size_t memoryGrowthRate = 0;  // bytes per frame/update
        std::unordered_map<std::string, ComponentMemoryInfo> componentStats;
        MemoryPressureLevel pressureLevel = MemoryPressureLevel::Low;
        
        // Cache performance metrics
        double cacheHitRatio = 0.0;
        double cacheMissRatio = 0.0;
        size_t totalCacheAccesses = 0;
        
        // Timing information
        long long lastAnalysisTime = 0;  // timestamp of last analysis
        double analysisTimeMs = 0.0;     // time taken to generate these stats
        
        // TODO: Add memory growth rate tracking
        // TODO: Add cache hit/miss ratios
    };
    
    // TODO: Add real-time memory monitoring
    // TODO: Implement memory usage heatmaps by archetype
    // TODO: Add memory allocation pattern analysis
    // TODO: Support memory usage callbacks for external monitoring
    static MemoryStats AnalyzeMemory(const EntityManagerV2& manager);
    
    // Compact memory by removing empty archetypes and shrinking vectors
    // TODO: Add incremental compaction to avoid frame drops
    // TODO: Implement background compaction in separate thread
    // TODO: Add compaction strategies (aggressive, conservative, balanced)
    // TODO: Support selective compaction by component type
    static void Compact(EntityManagerV2& manager);
    
    // Pre-allocate memory for expected entity counts
    // TODO: Add intelligent pre-allocation based on usage patterns
    // TODO: Implement dynamic growth strategies
    // TODO: Support different allocation strategies per archetype
    // TODO: Add memory warmup for critical component types
    static void Reserve(EntityManagerV2& manager, size_t entityCount);
    
    // Get recommendations for memory optimization
    // TODO: Add AI-driven optimization recommendations
    // TODO: Implement cost-benefit analysis for recommendations
    // TODO: Add performance impact predictions
    // TODO: Support custom recommendation rules
    static std::vector<std::string> GetOptimizationRecommendations(const MemoryStats& stats);
    
    // Defragment archetypes to improve memory layout
    static void DefragmentArchetypes(EntityManagerV2& manager) {
        // TODO: Implement archetype defragmentation algorithm
        // Move entities to reduce fragmentation between archetypes
        auto& archetypeMgr = const_cast<ArchetypeManager&>(manager.GetArchetypeManager());
        (void)archetypeMgr; // Suppress unused warning for now
        
        // Stub: Would reorganize entities to minimize memory gaps
        // 1. Identify fragmented archetypes
        // 2. Plan entity movement to consolidate memory
        // 3. Execute moves while maintaining entity references
    }
    
    // Optimize component layout for better cache performance
    static void OptimizeComponentLayout(EntityManagerV2& manager) {
        // TODO: Implement component layout optimization
        // Reorder components within archetypes for better cache locality
        auto& archetypeMgr = const_cast<ArchetypeManager&>(manager.GetArchetypeManager());
        (void)archetypeMgr; // Suppress unused warning for now
        
        // Stub: Would reorder components based on access patterns
        // 1. Analyze component access frequency
        // 2. Group frequently accessed components together
        // 3. Align components to cache line boundaries
    }
    
    // Balance entity distribution across archetypes
    static void BalanceArchetypeDistribution(EntityManagerV2& manager) {
        // TODO: Implement archetype load balancing
        auto& archetypeMgr = const_cast<ArchetypeManager&>(manager.GetArchetypeManager());
        (void)archetypeMgr; // Suppress unused warning for now
        
        // Stub: Would balance entity counts across similar archetypes
        // 1. Identify overloaded archetypes
        // 2. Find or create alternative archetypes
        // 3. Migrate entities to balance load
    }
    
    // Predict future memory usage based on current trends
    static MemoryStats PredictMemoryUsage(const EntityManagerV2& manager, size_t futureEntityCount) {
        // TODO: Implement memory usage prediction algorithm
        MemoryStats currentStats = AnalyzeMemory(manager);
        MemoryStats prediction = currentStats;
        
        // Stub: Simple linear prediction based on current usage
        if (currentStats.archetypeCount > 0) {
            double entitiesPerArchetype = static_cast<double>(futureEntityCount) / currentStats.archetypeCount;
            double growthFactor = entitiesPerArchetype / std::max(1.0, static_cast<double>(currentStats.totalUsed) / currentStats.archetypeCount);
            
            prediction.totalAllocated = static_cast<size_t>(currentStats.totalAllocated * growthFactor);
            prediction.totalUsed = static_cast<size_t>(currentStats.totalUsed * growthFactor);
            prediction.wastedSpace = prediction.totalAllocated - prediction.totalUsed;
            prediction.fragmentationRatio = prediction.totalAllocated > 0 
                ? static_cast<double>(prediction.wastedSpace) / prediction.totalAllocated 
                : 0.0;
        }
        
        return prediction;
    }
    
    // Set memory budget limit for the ECS system
    static void SetMemoryBudget(EntityManagerV2& manager, size_t maxBytes);
    
    // Check if memory pressure is currently high
    static bool IsMemoryPressureHigh(const EntityManagerV2& manager) {
        // TODO: Implement sophisticated memory pressure detection
        MemoryStats stats = AnalyzeMemory(manager);
        
        // Stub: Simple heuristic-based pressure detection
        return stats.fragmentationRatio > 0.4 || 
               stats.emptyArchetypes > (stats.archetypeCount * 0.2) ||
               stats.wastedSpace > (1024 * 1024 * 100); // 100MB waste threshold
    }
    
    // Enable or disable automatic memory optimization
    static void EnableAutoOptimization(EntityManagerV2& manager, bool enable);
    
    // Advanced memory profiling with detailed statistics
    static void ProfileMemoryUsage(const EntityManagerV2& manager, const std::string& outputPath = "");
    
    // Detect memory leaks in component storage
    static std::vector<std::string> DetectMemoryLeaks(const EntityManagerV2& manager) {
        // TODO: Implement memory leak detection
        (void)manager; // Suppress unused warning for now
        
        std::vector<std::string> leaks;
        
        // Stub: Would analyze for potential memory leaks
        // 1. Track allocation/deallocation patterns
        // 2. Identify unreferenced memory blocks
        // 3. Check for circular references in component data
        // 4. Report suspicious memory growth patterns
        
        return leaks;
    }
    
    // Get memory usage by component type
    static std::unordered_map<std::string, size_t> GetMemoryUsageByComponent(const EntityManagerV2& manager) {
        // TODO: Implement per-component memory tracking
        (void)manager; // Suppress unused warning for now
        
        std::unordered_map<std::string, size_t> usage;
        
        // Stub: Would analyze memory usage per component type
        // 1. Iterate through all archetypes
        // 2. Calculate memory usage per component type
        // 3. Aggregate across all instances
        // 4. Include overhead calculations
        
        return usage;
    }
    
    // Monitor memory usage in real-time
    static void StartRealTimeMonitoring(EntityManagerV2& manager, std::function<void(const MemoryStats&)> callback);
    
    // Stop real-time monitoring
    static void StopRealTimeMonitoring(EntityManagerV2& manager);
    
    // Create memory usage heatmap
    static std::vector<std::vector<double>> CreateMemoryHeatmap(const EntityManagerV2& manager) {
        // TODO: Implement memory heatmap generation
        (void)manager; // Suppress unused warning for now
        
        std::vector<std::vector<double>> heatmap;
        
        // Stub: Would generate 2D heatmap of memory usage
        // 1. Map memory regions to 2D grid
        // 2. Calculate usage intensity per grid cell
        // 3. Normalize values for visualization
        // 4. Include archetype boundaries
        
        return heatmap;
    }
    
    // Benchmark memory optimization operations
    static void BenchmarkOptimizations(EntityManagerV2& manager);
    
    // Get current memory pressure level
    static MemoryPressureLevel GetMemoryPressureLevel(const EntityManagerV2& manager) {
        MemoryStats stats = AnalyzeMemory(manager);
        
        // Simple heuristic to determine pressure level
        if (stats.fragmentationRatio > 0.6 || stats.wastedSpace > (1024 * 1024 * 200)) {
            return MemoryPressureLevel::Critical;
        } else if (stats.fragmentationRatio > 0.4 || stats.wastedSpace > (1024 * 1024 * 100)) {
            return MemoryPressureLevel::High;
        } else if (stats.fragmentationRatio > 0.2 || stats.emptyArchetypes > 10) {
            return MemoryPressureLevel::Medium;
        } else {
            return MemoryPressureLevel::Low;
        }
    }
    
    // Perform optimization based on strategy
    static void OptimizeWithStrategy(EntityManagerV2& manager, OptimizationStrategy strategy) {
        switch (strategy) {
            case OptimizationStrategy::Conservative:
                // Only perform safe, minimal optimizations
                if (GetMemoryPressureLevel(manager) >= MemoryPressureLevel::High) {
                    Compact(manager);
                }
                break;
                
            case OptimizationStrategy::Balanced:
                // Perform moderate optimizations
                if (GetMemoryPressureLevel(manager) >= MemoryPressureLevel::Medium) {
                    Compact(manager);
                    DefragmentArchetypes(manager);
                }
                break;
                
            case OptimizationStrategy::Aggressive:
                // Perform all available optimizations
                Compact(manager);
                DefragmentArchetypes(manager);
                OptimizeComponentLayout(manager);
                BalanceArchetypeDistribution(manager);
                break;
        }
    }
    
    // Export memory statistics to file
    static bool ExportMemoryStats(const MemoryStats& stats, const std::string& filePath);
    
    // Import memory optimization settings
    static bool ImportOptimizationSettings(EntityManagerV2& manager, const std::string& filePath) {
        // TODO: Implement settings import
        (void)manager; (void)filePath; // Suppress unused warnings for now
        
        // Stub: Would load optimization configuration from file
        // 1. Parse configuration file
        // 2. Apply optimization settings
        // 3. Configure automatic optimization triggers
        // 4. Set memory budgets and thresholds
        
        return true; // Placeholder success
    }
};

} // namespace ecs

// TODO: ECS Memory Management System TODOs:
// TODO: Integrate with garbage collection systems
// TODO: Add memory debugging tools and leak detection
// TODO: Implement memory usage profiler with flame graphs
// TODO: Support for different memory architectures (NUMA, etc.)
// TODO: Add memory usage metrics export for external monitoring
// TODO: Implement memory usage alerts and notifications
// TODO: Support for memory-mapped file backing for large datasets
// TODO: Add compression for rarely accessed component data
// TODO: Implement memory usage quotas per system/subsystem
// TODO: Add memory optimization benchmarking suite
