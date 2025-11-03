#include "../engine/LifecyclePerformanceOptimizer.h"
#include "../engine/IActor.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <random>
#include <cassert>

using namespace lifecycle;

// Test actor for performance testing
class PerfTestActor : public IActor {
private:
    std::string name_;
    static std::atomic<int> counter_;
    
public:
    PerfTestActor() : name_("PerfActor" + std::to_string(counter_++)) {}
    
    void Initialize() override {
        // Simulate some initialization work
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    void Update(double) override {
        // Simulate update work
        volatile int work = 0;
        for (int i = 0; i < 100; ++i) {
            work += i;
        }
    }
    
    std::string GetName() const override {
        return name_;
    }
};

std::atomic<int> PerfTestActor::counter_{0};

// Performance test suite
class LifecyclePerformanceTests {
public:
    void RunAllTests() {
        std::cout << "=== Lifecycle Performance Tests ===" << std::endl;
        
        TestBasicPerformance();
        TestBatchPerformance();
        TestObjectPooling();
        TestParallelProcessing();
        TestMemoryUsage();
        
        std::cout << "=== All Performance Tests Complete ===" << std::endl;
    }

private:
    void TestBasicPerformance() {
        std::cout << "\n--- Test 1: Basic Performance ---" << std::endl;
        
        auto& optimizer = LifecyclePerformanceOptimizer::Instance();
        optimizer.Initialize();
        
        // Create test actors
        std::vector<std::unique_ptr<PerfTestActor>> actors;
        const size_t numActors = 100;
        actors.reserve(numActors);
        
        EntityManager entityManager;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Create and register actors
        for (size_t i = 0; i < numActors; ++i) {
            auto actor = std::make_unique<PerfTestActor>();
            Entity entity = entityManager.CreateEntity();
            ActorContext context{entityManager, entity};
            actor->AttachContext(context);
            
            ActorLifecycleManager::Instance().RegisterActor(actor.get(), &context);
            actors.push_back(std::move(actor));
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Created " << numActors << " actors in " << (duration * 1000) << "ms" << std::endl;
        std::cout << "Average creation time: " << (duration / numActors * 1000000) << "μs per actor" << std::endl;
        
        // Cleanup
        for (auto& actor : actors) {
            ActorLifecycleManager::Instance().UnregisterActor(actor.get());
        }
        
        optimizer.PrintPerformanceReport();
        optimizer.Shutdown();
    }
    
    void TestBatchPerformance() {
        std::cout << "\n--- Test 2: Batch Performance ---" << std::endl;
        
        auto& optimizer = LifecyclePerformanceOptimizer::Instance();
        LifecyclePerformanceOptimizer::Config config;
        config.enableBatching = true;
        config.batchSize = 32;
        config.enableParallelProcessing = true;
        optimizer.Initialize(config);
        
        auto& optimizedManager = OptimizedLifecycleManager::Instance();
        optimizedManager.Initialize();
        
        // Create test actors
        std::vector<std::unique_ptr<PerfTestActor>> actors;
        std::vector<IActor*> actorPtrs;
        const size_t numActors = 200;
        actors.reserve(numActors);
        actorPtrs.reserve(numActors);
        
        EntityManager entityManager;
        
        // Setup actors
        for (size_t i = 0; i < numActors; ++i) {
            auto actor = std::make_unique<PerfTestActor>();
            Entity entity = entityManager.CreateEntity();
            ActorContext context{entityManager, entity};
            actor->AttachContext(context);
            
            ActorLifecycleManager::Instance().RegisterActor(actor.get(), &context);
            actorPtrs.push_back(actor.get());
            actors.push_back(std::move(actor));
        }
        
        // Test batch initialization
        auto startTime = std::chrono::high_resolution_clock::now();
        optimizedManager.BatchInitialize(actorPtrs);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        double batchDuration = std::chrono::duration<double>(endTime - startTime).count();
        std::cout << "Batch initialized " << numActors << " actors in " << (batchDuration * 1000) << "ms" << std::endl;
        
        // Test batch activation
        startTime = std::chrono::high_resolution_clock::now();
        optimizedManager.BatchActivate(actorPtrs);
        endTime = std::chrono::high_resolution_clock::now();
        
        batchDuration = std::chrono::duration<double>(endTime - startTime).count();
        std::cout << "Batch activated " << numActors << " actors in " << (batchDuration * 1000) << "ms" << std::endl;
        
        // Wait for batch processing to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test batch destruction
        startTime = std::chrono::high_resolution_clock::now();
        optimizedManager.BatchDestroy(actorPtrs);
        endTime = std::chrono::high_resolution_clock::now();
        
        batchDuration = std::chrono::duration<double>(endTime - startTime).count();
        std::cout << "Batch destroyed " << numActors << " actors in " << (batchDuration * 1000) << "ms" << std::endl;
        
        // Cleanup
        for (auto& actor : actors) {
            ActorLifecycleManager::Instance().UnregisterActor(actor.get());
        }
        
        optimizedManager.PrintReport();
        optimizedManager.Shutdown();
        optimizer.Shutdown();
    }
    
    void TestObjectPooling() {
        std::cout << "\n--- Test 3: Object Pooling ---" << std::endl;
        
        auto& pool = LifecycleContextPool::Instance();
        pool.SetMaxPoolSize(50);
        
        // Test pool allocation and release
        std::vector<std::unique_ptr<LifecycleContext>> contexts;
        const size_t numContexts = 30;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Acquire contexts
        for (size_t i = 0; i < numContexts; ++i) {
            contexts.push_back(pool.Acquire());
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double acquireTime = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Acquired " << numContexts << " contexts in " << (acquireTime * 1000000) << "μs" << std::endl;
        std::cout << "Pool size after acquire: " << pool.GetPoolSize() << std::endl;
        
        // Release contexts
        startTime = std::chrono::high_resolution_clock::now();
        
        for (auto& context : contexts) {
            pool.Release(std::move(context));
        }
        contexts.clear();
        
        endTime = std::chrono::high_resolution_clock::now();
        double releaseTime = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Released " << numContexts << " contexts in " << (releaseTime * 1000000) << "μs" << std::endl;
        std::cout << "Pool size after release: " << pool.GetPoolSize() << std::endl;
        
        // Test reuse efficiency
        startTime = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < numContexts; ++i) {
            auto context = pool.Acquire();
            pool.Release(std::move(context));
        }
        
        endTime = std::chrono::high_resolution_clock::now();
        double reuseTime = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Acquire/Release cycle for " << numContexts << " contexts: " 
                  << (reuseTime * 1000000) << "μs" << std::endl;
        std::cout << "Average time per cycle: " << (reuseTime / numContexts * 1000000) << "μs" << std::endl;
        
        pool.Clear();
    }
    
    void TestParallelProcessing() {
        std::cout << "\n--- Test 4: Parallel Processing ---" << std::endl;
        
        // Test with parallel processing enabled
        auto& optimizer = LifecyclePerformanceOptimizer::Instance();
        LifecyclePerformanceOptimizer::Config parallelConfig;
        parallelConfig.enableParallelProcessing = true;
        parallelConfig.enableBatching = true;
        parallelConfig.batchSize = 64;
        optimizer.Initialize(parallelConfig);
        
        auto& optimizedManager = OptimizedLifecycleManager::Instance();
        optimizedManager.Initialize();
        
        const size_t numActors = 500;
        std::vector<std::unique_ptr<PerfTestActor>> actors;
        std::vector<IActor*> actorPtrs;
        actors.reserve(numActors);
        actorPtrs.reserve(numActors);
        
        EntityManager entityManager;
        
        // Setup actors
        for (size_t i = 0; i < numActors; ++i) {
            auto actor = std::make_unique<PerfTestActor>();
            Entity entity = entityManager.CreateEntity();
            ActorContext context{entityManager, entity};
            actor->AttachContext(context);
            
            ActorLifecycleManager::Instance().RegisterActor(actor.get(), &context);
            actorPtrs.push_back(actor.get());
            actors.push_back(std::move(actor));
        }
        
        // Test parallel batch processing
        auto startTime = std::chrono::high_resolution_clock::now();
        optimizedManager.BatchInitialize(actorPtrs);
        optimizedManager.BatchActivate(actorPtrs);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        double parallelTime = std::chrono::duration<double>(endTime - startTime).count();
        std::cout << "Parallel batch processing for " << numActors << " actors: " 
                  << (parallelTime * 1000) << "ms" << std::endl;
        
        // Wait for async processing
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Cleanup
        for (auto& actor : actors) {
            ActorLifecycleManager::Instance().UnregisterActor(actor.get());
        }
        
        optimizedManager.PrintReport();
        optimizedManager.Shutdown();
        optimizer.Shutdown();
    }
    
    void TestMemoryUsage() {
        std::cout << "\n--- Test 5: Memory Usage ---" << std::endl;
        
        auto& optimizer = LifecyclePerformanceOptimizer::Instance();
        LifecyclePerformanceOptimizer::Config memConfig;
        memConfig.enableObjectPooling = true;
        memConfig.maxPoolSize = 200;
        memConfig.enablePerformanceMonitoring = true;
        optimizer.Initialize(memConfig);
        
        const size_t numActors = 1000;
        std::vector<std::unique_ptr<PerfTestActor>> actors;
        actors.reserve(numActors);
        
        EntityManager entityManager;
        
        // Create actors and measure memory growth
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < numActors; ++i) {
            auto actor = std::make_unique<PerfTestActor>();
            Entity entity = entityManager.CreateEntity();
            ActorContext context{entityManager, entity};
            actor->AttachContext(context);
            
            ActorLifecycleManager::Instance().RegisterActor(actor.get(), &context);
            actors.push_back(std::move(actor));
            
            // Sample memory usage every 100 actors
            if ((i + 1) % 100 == 0) {
                auto& metrics = optimizer.GetMetrics();
                std::cout << "After " << (i + 1) << " actors - Pool size: " 
                          << LifecycleContextPool::Instance().GetPoolSize() 
                          << ", Memory: " << (metrics.currentMemoryUsage / 1024.0) << " KB" << std::endl;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Created " << numActors << " actors in " << (totalTime * 1000) << "ms" << std::endl;
        std::cout << "Final pool size: " << LifecycleContextPool::Instance().GetPoolSize() << std::endl;
        
        // Cleanup and test memory reclamation
        startTime = std::chrono::high_resolution_clock::now();
        
        for (auto& actor : actors) {
            ActorLifecycleManager::Instance().UnregisterActor(actor.get());
        }
        actors.clear();
        
        endTime = std::chrono::high_resolution_clock::now();
        double cleanupTime = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "Cleanup took " << (cleanupTime * 1000) << "ms" << std::endl;
        std::cout << "Pool size after cleanup: " << LifecycleContextPool::Instance().GetPoolSize() << std::endl;
        
        optimizer.PrintPerformanceReport();
        optimizer.AnalyzePerformance();
        optimizer.Shutdown();
    }
};

int main() {
    std::cout << "=== Actor Lifecycle Performance Test Suite ===" << std::endl;
    
    try {
        LifecyclePerformanceTests tests;
        tests.RunAllTests();
        
        std::cout << "\n✅ All performance tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}