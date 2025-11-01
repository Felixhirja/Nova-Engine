#pragma once
#include "EntityManager.h"
#include <thread>
#include <vector>
#include <algorithm>

namespace ecs {

// Parallel ForEach utilities for multi-threaded entity iteration
class ParallelIterator {
public:
    template<typename T, typename Func>
    static void ForEach(EntityManagerV2& manager, Func&& func, size_t threadCount = 0) {
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
        }
        
        auto archetypes = manager.GetArchetypeManager().GetArchetypesWithComponent<T>();
        
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        
        for (Archetype* archetype : archetypes) {
            const auto& entities = archetype->GetEntities();
            auto* components = archetype->GetComponentVector<T>();
            
            if (!components) continue;
            
            const size_t count = entities.size();
            const size_t chunkSize = (count + threadCount - 1) / threadCount;
            
            for (size_t t = 0; t < threadCount; ++t) {
                size_t start = t * chunkSize;
                size_t end = std::min(start + chunkSize, count);
                
                if (start >= count) break;
                
                threads.emplace_back([&entities, components, start, end, &func]() {
                    for (size_t i = start; i < end; ++i) {
                        func(entities[i], (*components)[i]);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            threads.clear();
        }
    }
    
    template<typename T1, typename T2, typename Func>
    static void ForEach(EntityManagerV2& manager, Func&& func, size_t threadCount = 0) {
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
        }
        
        auto archetypes = manager.GetArchetypeManager().GetArchetypesWithComponents<T1, T2>();
        
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        
        for (Archetype* archetype : archetypes) {
            const auto& entities = archetype->GetEntities();
            auto* comp1 = archetype->GetComponentVector<T1>();
            auto* comp2 = archetype->GetComponentVector<T2>();
            
            if (!comp1 || !comp2) continue;
            
            const size_t count = entities.size();
            const size_t chunkSize = (count + threadCount - 1) / threadCount;
            
            for (size_t t = 0; t < threadCount; ++t) {
                size_t start = t * chunkSize;
                size_t end = std::min(start + chunkSize, count);
                
                if (start >= count) break;
                
                threads.emplace_back([&entities, comp1, comp2, start, end, &func]() {
                    for (size_t i = start; i < end; ++i) {
                        func(entities[i], (*comp1)[i], (*comp2)[i]);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            threads.clear();
        }
    }
};

} // namespace ecs
