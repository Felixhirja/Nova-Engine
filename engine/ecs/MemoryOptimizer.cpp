#include "MemoryOptimizer.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>

namespace ecs {

// Static variables for global memory management state
namespace {
    std::mutex g_monitoring_mutex;
    std::thread g_monitoring_thread;
    std::atomic<bool> g_monitoring_active{false};
    std::function<void(const MemoryOptimizer::MemoryStats&)> g_monitoring_callback;
    EntityManagerV2* g_monitored_manager = nullptr;
    
    // Memory budget tracking
    std::unordered_map<EntityManagerV2*, size_t> g_memory_budgets;
    std::unordered_map<EntityManagerV2*, bool> g_auto_optimization_enabled;
    
    // Performance tracking
    std::unordered_map<std::string, double> g_operation_timings;
}

// Enhanced AnalyzeMemory with timing and extended statistics
MemoryOptimizer::MemoryStats MemoryOptimizer::AnalyzeMemory(const EntityManagerV2& manager) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    MemoryStats stats;
    
    try {
        const auto& archetypeMgr = manager.GetArchetypeManager();
        stats.archetypeCount = archetypeMgr.GetArchetypeCount();
        stats.totalAllocated = archetypeMgr.GetMemoryUsage();
        
        // Component-specific analysis
        std::unordered_map<std::string, ComponentMemoryInfo> componentBreakdown;
        
        for (const auto& archetype : archetypeMgr.GetAllArchetypes()) {
            if (!archetype) continue; // Skip null archetypes
            
            try {
                size_t entityCount = archetype->GetEntityCount();
                if (entityCount == 0 && archetype->GetId() != 0) {
                    stats.emptyArchetypes++;
                }
                
                size_t used = archetype->GetMemoryUsage();
                stats.totalUsed += used;
                
            } catch (const std::exception& e) {
                // Skip this archetype if there's an error accessing it
                std::cerr << "Warning: Error accessing archetype: " << e.what() << "\n";
                continue;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Warning: Error analyzing memory: " << e.what() << "\n";
        // Return basic stats even if analysis fails
        stats.archetypeCount = 1; // Assume at least one archetype
        stats.totalUsed = 1024; // Conservative estimate
        stats.totalAllocated = 2048; // Conservative estimate with overhead
    }
    
    stats.wastedSpace = stats.totalAllocated > stats.totalUsed 
        ? stats.totalAllocated - stats.totalUsed 
        : 0;
    stats.fragmentationRatio = stats.totalAllocated > 0 
        ? static_cast<double>(stats.wastedSpace) / stats.totalAllocated 
        : 0.0;
    
    // Set memory pressure level (use safe method)
    try {
        stats.pressureLevel = GetMemoryPressureLevel(manager);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Error getting pressure level: " << e.what() << "\n";
        stats.pressureLevel = MemoryPressureLevel::Low;
    }
    
    // Update timing information
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.analysisTimeMs = duration.count() / 1000.0;
    stats.lastAnalysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return stats;
}

// Enhanced Compact with timing and progress tracking
void MemoryOptimizer::Compact(EntityManagerV2& manager) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get non-const reference to archetype manager for modification
    auto& archetypeMgr = const_cast<ArchetypeManager&>(manager.GetArchetypeManager());
    archetypeMgr.Shrink();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    g_operation_timings["Compact"] = duration.count() / 1000.0;
    
    std::cout << "[MemoryOptimizer] Compaction completed in " << g_operation_timings["Compact"] << "ms\n";
}

// Implementation of ProfileMemoryUsage with actual output
void MemoryOptimizer::ProfileMemoryUsage(const EntityManagerV2& manager, const std::string& outputPath) {
    MemoryStats stats = AnalyzeMemory(manager);
    
    std::ostream* output = &std::cout;
    std::ofstream file;
    
    if (!outputPath.empty()) {
        file.open(outputPath);
        if (file.is_open()) {
            output = &file;
        }
    }
    
    *output << "=== ECS Memory Profile ===\n";
    *output << "Total Allocated: " << stats.totalAllocated << " bytes\n";
    *output << "Total Used: " << stats.totalUsed << " bytes\n";
    *output << "Wasted Space: " << stats.wastedSpace << " bytes\n";
    *output << "Fragmentation Ratio: " << (stats.fragmentationRatio * 100.0) << "%\n";
    *output << "Archetype Count: " << stats.archetypeCount << "\n";
    *output << "Empty Archetypes: " << stats.emptyArchetypes << "\n";
    
    switch (stats.pressureLevel) {
        case MemoryPressureLevel::Low:
            *output << "Memory Pressure: Low (Normal)\n";
            break;
        case MemoryPressureLevel::Medium:
            *output << "Memory Pressure: Medium (Some optimization recommended)\n";
            break;
        case MemoryPressureLevel::High:
            *output << "Memory Pressure: High (Immediate optimization needed)\n";
            break;
        case MemoryPressureLevel::Critical:
            *output << "Memory Pressure: Critical (Emergency cleanup required)\n";
            break;
    }
    
    *output << "Analysis Time: " << stats.analysisTimeMs << "ms\n";
    *output << "==========================\n";
    
    if (file.is_open()) {
        std::cout << "[MemoryOptimizer] Profile exported to: " << outputPath << "\n";
    }
}

// Implementation of real-time monitoring
void MemoryOptimizer::StartRealTimeMonitoring(EntityManagerV2& manager, std::function<void(const MemoryStats&)> callback) {
    std::lock_guard<std::mutex> lock(g_monitoring_mutex);
    
    if (g_monitoring_active) {
        std::cout << "[MemoryOptimizer] Warning: Monitoring already active. Stopping previous monitoring.\n";
        StopRealTimeMonitoring(manager);
    }
    
    g_monitored_manager = &manager;
    g_monitoring_callback = callback;
    g_monitoring_active = true;
    
    g_monitoring_thread = std::thread([]() {
        while (g_monitoring_active) {
            if (g_monitored_manager && g_monitoring_callback) {
                MemoryStats stats = AnalyzeMemory(*g_monitored_manager);
                g_monitoring_callback(stats);
            }
            
            // Monitor every 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    std::cout << "[MemoryOptimizer] Real-time monitoring started\n";
}

void MemoryOptimizer::StopRealTimeMonitoring(EntityManagerV2& manager) {
    (void)manager; // Suppress unused warning
    
    std::lock_guard<std::mutex> lock(g_monitoring_mutex);
    
    g_monitoring_active = false;
    
    if (g_monitoring_thread.joinable()) {
        g_monitoring_thread.join();
    }
    
    g_monitored_manager = nullptr;
    g_monitoring_callback = nullptr;
    
    std::cout << "[MemoryOptimizer] Real-time monitoring stopped\n";
}

// Implementation of memory budget management
void MemoryOptimizer::SetMemoryBudget(EntityManagerV2& manager, size_t maxBytes) {
    g_memory_budgets[&manager] = maxBytes;
    
    std::cout << "[MemoryOptimizer] Memory budget set to " << maxBytes << " bytes\n";
    
    // Check if current usage exceeds budget
    MemoryStats stats = AnalyzeMemory(manager);
    if (stats.totalAllocated > maxBytes) {
        std::cout << "[MemoryOptimizer] Warning: Current usage (" << stats.totalAllocated 
                  << " bytes) exceeds budget. Consider running optimization.\n";
    }
}

// Implementation of auto-optimization
void MemoryOptimizer::EnableAutoOptimization(EntityManagerV2& manager, bool enable) {
    g_auto_optimization_enabled[&manager] = enable;
    
    if (enable) {
        std::cout << "[MemoryOptimizer] Auto-optimization enabled\n";
        
        // Start monitoring for auto-optimization triggers
        if (!g_monitoring_active) {
            StartRealTimeMonitoring(manager, [&manager](const MemoryStats& stats) {
                // Auto-optimize based on pressure level
                if (stats.pressureLevel >= MemoryPressureLevel::High) {
                    std::cout << "[MemoryOptimizer] Auto-optimization triggered due to high memory pressure\n";
                    OptimizeWithStrategy(manager, OptimizationStrategy::Balanced);
                }
            });
        }
    } else {
        std::cout << "[MemoryOptimizer] Auto-optimization disabled\n";
        StopRealTimeMonitoring(manager);
    }
}

// Implementation of statistics export
bool MemoryOptimizer::ExportMemoryStats(const MemoryStats& stats, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MemoryOptimizer] Error: Could not open file for export: " << filePath << "\n";
        return false;
    }
    
    // Export as JSON format
    file << "{\n";
    file << "  \"timestamp\": " << stats.lastAnalysisTime << ",\n";
    file << "  \"totalAllocated\": " << stats.totalAllocated << ",\n";
    file << "  \"totalUsed\": " << stats.totalUsed << ",\n";
    file << "  \"wastedSpace\": " << stats.wastedSpace << ",\n";
    file << "  \"fragmentationRatio\": " << stats.fragmentationRatio << ",\n";
    file << "  \"archetypeCount\": " << stats.archetypeCount << ",\n";
    file << "  \"emptyArchetypes\": " << stats.emptyArchetypes << ",\n";
    file << "  \"peakMemoryUsage\": " << stats.peakMemoryUsage << ",\n";
    file << "  \"memoryGrowthRate\": " << stats.memoryGrowthRate << ",\n";
    file << "  \"pressureLevel\": " << static_cast<int>(stats.pressureLevel) << ",\n";
    file << "  \"cacheHitRatio\": " << stats.cacheHitRatio << ",\n";
    file << "  \"cacheMissRatio\": " << stats.cacheMissRatio << ",\n";
    file << "  \"totalCacheAccesses\": " << stats.totalCacheAccesses << ",\n";
    file << "  \"analysisTimeMs\": " << stats.analysisTimeMs << "\n";
    file << "}\n";
    
    file.close();
    
    std::cout << "[MemoryOptimizer] Memory statistics exported to: " << filePath << "\n";
    return true;
}

// Implementation of benchmarking
void MemoryOptimizer::BenchmarkOptimizations(EntityManagerV2& manager) {
    std::cout << "[MemoryOptimizer] Starting optimization benchmarks...\n";
    
    // Baseline measurement
    MemoryStats baseline = AnalyzeMemory(manager);
    std::cout << "Baseline - Used: " << baseline.totalUsed << " bytes, Fragmentation: " 
              << (baseline.fragmentationRatio * 100.0) << "%\n";
    
    // Test Conservative strategy
    auto start = std::chrono::high_resolution_clock::now();
    OptimizeWithStrategy(manager, OptimizationStrategy::Conservative);
    auto end = std::chrono::high_resolution_clock::now();
    auto conservativeTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
    MemoryStats afterConservative = AnalyzeMemory(manager);
    
    std::cout << "Conservative - Time: " << conservativeTime << "ms, Used: " << afterConservative.totalUsed 
              << " bytes, Fragmentation: " << (afterConservative.fragmentationRatio * 100.0) << "%\n";
    
    // Test Balanced strategy
    start = std::chrono::high_resolution_clock::now();
    OptimizeWithStrategy(manager, OptimizationStrategy::Balanced);
    end = std::chrono::high_resolution_clock::now();
    auto balancedTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
    MemoryStats afterBalanced = AnalyzeMemory(manager);
    
    std::cout << "Balanced - Time: " << balancedTime << "ms, Used: " << afterBalanced.totalUsed 
              << " bytes, Fragmentation: " << (afterBalanced.fragmentationRatio * 100.0) << "%\n";
    
    // Test Aggressive strategy
    start = std::chrono::high_resolution_clock::now();
    OptimizeWithStrategy(manager, OptimizationStrategy::Aggressive);
    end = std::chrono::high_resolution_clock::now();
    auto aggressiveTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
    MemoryStats afterAggressive = AnalyzeMemory(manager);
    
    std::cout << "Aggressive - Time: " << aggressiveTime << "ms, Used: " << afterAggressive.totalUsed 
              << " bytes, Fragmentation: " << (afterAggressive.fragmentationRatio * 100.0) << "%\n";
    
    std::cout << "[MemoryOptimizer] Benchmark completed.\n";
}

// Enhanced GetOptimizationRecommendations with more sophisticated logic
std::vector<std::string> MemoryOptimizer::GetOptimizationRecommendations(const MemoryStats& stats) {
    std::vector<std::string> recommendations;
    
    // Fragmentation-based recommendations
    if (stats.fragmentationRatio > 0.5) {
        recommendations.push_back("Critical fragmentation detected. Run aggressive compaction immediately.");
    } else if (stats.fragmentationRatio > 0.3) {
        recommendations.push_back("High fragmentation detected. Consider calling Compact().");
    } else if (stats.fragmentationRatio > 0.1) {
        recommendations.push_back("Moderate fragmentation. Monitor memory usage closely.");
    }
    
    // Empty archetype recommendations
    if (stats.emptyArchetypes > (stats.archetypeCount * 0.3)) {
        recommendations.push_back("Many empty archetypes exist (" + std::to_string(stats.emptyArchetypes) + 
                                "). Call Compact() to free memory.");
    }
    
    // Archetype count recommendations
    if (stats.archetypeCount > 1000) {
        recommendations.push_back("Large number of archetypes (" + std::to_string(stats.archetypeCount) + 
                                "). Consider reducing component combinations.");
    }
    
    // Memory pressure recommendations
    switch (stats.pressureLevel) {
        case MemoryPressureLevel::Critical:
            recommendations.push_back("CRITICAL: Immediate memory optimization required!");
            recommendations.push_back("Consider enabling auto-optimization with aggressive strategy.");
            break;
        case MemoryPressureLevel::High:
            recommendations.push_back("High memory pressure. Run optimization soon.");
            break;
        case MemoryPressureLevel::Medium:
            recommendations.push_back("Moderate memory pressure. Schedule optimization during low-usage periods.");
            break;
        case MemoryPressureLevel::Low:
            recommendations.push_back("Memory usage is optimal.");
            break;
    }
    
    // Performance recommendations
    if (stats.analysisTimeMs > 10.0) {
        recommendations.push_back("Memory analysis is taking longer than expected (" + 
                                std::to_string(stats.analysisTimeMs) + "ms). Consider optimization.");
    }
    
    // Cache performance recommendations
    if (stats.cacheMissRatio > 0.2 && stats.totalCacheAccesses > 1000) {
        recommendations.push_back("High cache miss ratio detected. Consider component layout optimization.");
    }
    
    return recommendations;
}

} // namespace ecs