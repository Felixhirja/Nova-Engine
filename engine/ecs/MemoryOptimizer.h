#pragma once
#include "EntityManager.h"
#include <algorithm>
#include <vector>

namespace ecs {

// Memory optimization utilities for ECS
class MemoryOptimizer {
public:
    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalUsed = 0;
        size_t wastedSpace = 0;
        size_t archetypeCount = 0;
        size_t emptyArchetypes = 0;
        double fragmentationRatio = 0.0;
    };
    
    static MemoryStats AnalyzeMemory(const EntityManagerV2& manager) {
        MemoryStats stats;
        
        const auto& archetypeMgr = manager.GetArchetypeManager();
        stats.archetypeCount = archetypeMgr.GetArchetypeCount();
        stats.totalAllocated = archetypeMgr.GetMemoryUsage();
        
        for (const auto& archetype : archetypeMgr.GetAllArchetypes()) {
            size_t entityCount = archetype->GetEntityCount();
            if (entityCount == 0 && archetype->GetId() != 0) {
                stats.emptyArchetypes++;
            }
            
            size_t used = archetype->GetMemoryUsage();
            stats.totalUsed += used;
        }
        
        stats.wastedSpace = stats.totalAllocated > stats.totalUsed 
            ? stats.totalAllocated - stats.totalUsed 
            : 0;
        stats.fragmentationRatio = stats.totalAllocated > 0 
            ? static_cast<double>(stats.wastedSpace) / stats.totalAllocated 
            : 0.0;
        
        return stats;
    }
    
    // Compact memory by removing empty archetypes and shrinking vectors
    static void Compact(EntityManagerV2& manager) {
        manager.GetArchetypeManager().Shrink();
    }
    
    // Pre-allocate memory for expected entity counts
    static void Reserve(EntityManagerV2& manager, size_t entityCount) {
        // This would require extending EntityManagerV2 with a Reserve method
        (void)manager;
        (void)entityCount;
    }
    
    // Get recommendations for memory optimization
    static std::vector<std::string> GetOptimizationRecommendations(const MemoryStats& stats) {
        std::vector<std::string> recommendations;
        
        if (stats.fragmentationRatio > 0.3) {
            recommendations.push_back("High fragmentation detected. Consider calling Compact().");
        }
        
        if (stats.emptyArchetypes > 10) {
            recommendations.push_back("Many empty archetypes exist. Call Compact() to free memory.");
        }
        
        if (stats.archetypeCount > 1000) {
            recommendations.push_back("Large number of archetypes. Consider reducing component combinations.");
        }
        
        return recommendations;
    }
};

} // namespace ecs
