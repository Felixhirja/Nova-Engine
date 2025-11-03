#pragma once

#include "ActorLifecycleManager.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace lifecycle {

// Forward declarations
class LifecyclePerformanceOptimizer;

/**
 * Performance metrics for lifecycle operations
 */
struct PerformanceMetrics {
    std::chrono::high_resolution_clock::time_point startTime;
    
    // Operation timing
    double totalTransitionTime = 0.0;
    double averageTransitionTime = 0.0;
    size_t transitionCount = 0;
    
    // Batch processing metrics
    double totalBatchTime = 0.0;
    double averageBatchTime = 0.0;
    size_t batchCount = 0;
    size_t totalBatchedActors = 0;
    
    // Memory metrics
    size_t peakMemoryUsage = 0;
    size_t currentMemoryUsage = 0;
    
    // Thread metrics
    size_t threadsUsed = 0;
    double parallelEfficiency = 1.0;
    
    void UpdateTransitionMetrics(double duration) {
        totalTransitionTime += duration;
        transitionCount++;
        averageTransitionTime = totalTransitionTime / transitionCount;
    }
    
    void UpdateBatchMetrics(double duration, size_t actorCount) {
        totalBatchTime += duration;
        batchCount++;
        totalBatchedActors += actorCount;
        averageBatchTime = totalBatchTime / batchCount;
    }
    
    double GetTransitionsPerSecond() const {
        return startTime == std::chrono::high_resolution_clock::time_point{} ? 0.0 :
               transitionCount / std::chrono::duration<double>(
                   std::chrono::high_resolution_clock::now() - startTime).count();
    }
};

/**
 * Object pool for lifecycle contexts to reduce allocations
 */
class LifecycleContextPool {
public:
    static LifecycleContextPool& Instance() {
        static LifecycleContextPool instance;
        return instance;
    }
    
    std::unique_ptr<LifecycleContext> Acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!pool_.empty()) {
            auto context = std::move(pool_.front());
            pool_.pop();
            
            // Reset context for reuse
            context->actor = nullptr;
            context->actorContext = nullptr;
            context->actorName.clear();
            context->actorType.clear();
            context->state = ActorState::Created;
            context->stats = LifecycleStats{};
            context->metadata.clear();
            
            return context;
        }
        
        // Create new context if pool is empty
        return std::make_unique<LifecycleContext>();
    }
    
    void Release(std::unique_ptr<LifecycleContext> context) {
        if (!context) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Limit pool size to prevent excessive memory usage
        if (pool_.size() < maxPoolSize_) {
            pool_.push(std::move(context));
        }
        // If pool is full, context will be automatically destroyed
    }
    
    size_t GetPoolSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }
    
    void SetMaxPoolSize(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxPoolSize_ = size;
        
        // Trim pool if necessary
        while (pool_.size() > maxPoolSize_ && !pool_.empty()) {
            pool_.pop();
        }
    }
    
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    LifecycleContextPool() = default;
    
    mutable std::mutex mutex_;
    std::queue<std::unique_ptr<LifecycleContext>> pool_;
    size_t maxPoolSize_ = 100; // Default max pool size
};

/**
 * Batch operation processor for lifecycle transitions
 */
class BatchProcessor {
public:
    struct BatchOperation {
        std::vector<IActor*> actors;
        ActorState targetState;
        std::chrono::high_resolution_clock::time_point timestamp;
        
        BatchOperation(std::vector<IActor*> a, ActorState state) 
            : actors(std::move(a)), targetState(state), 
              timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    BatchProcessor() : running_(false), workerThread_(&BatchProcessor::ProcessBatches, this) {}
    
    ~BatchProcessor() {
        Stop();
    }
    
    void Start() {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
        condition_.notify_one();
    }
    
    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        condition_.notify_one();
        
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
    
    void QueueBatch(std::vector<IActor*> actors, ActorState targetState) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        batchQueue_.emplace(std::move(actors), targetState);
        condition_.notify_one();
    }
    
    size_t GetQueueSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return batchQueue_.size();
    }
    
    void SetBatchTimeout(std::chrono::milliseconds timeout) {
        batchTimeout_ = timeout;
    }

private:
    void ProcessBatches() {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait for work or shutdown signal
            condition_.wait(lock, [this] { return !running_ || !batchQueue_.empty(); });
            
            if (!running_ && batchQueue_.empty()) {
                break;
            }
            
            // Process all queued batches
            while (!batchQueue_.empty()) {
                auto batch = std::move(batchQueue_.front());
                batchQueue_.pop();
                lock.unlock();
                
                ProcessSingleBatch(batch);
                
                lock.lock();
            }
        }
    }
    
    void ProcessSingleBatch(const BatchOperation& batch) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Sort actors by current state for more efficient processing
        std::unordered_map<ActorState, std::vector<IActor*>> actorsByState;
        for (auto* actor : batch.actors) {
            ActorState currentState = ActorLifecycleManager::Instance().GetState(actor);
            actorsByState[currentState].push_back(actor);
        }
        
        // Process each state group
        for (auto& [currentState, actors] : actorsByState) {
            ProcessStateGroup(actors, currentState, batch.targetState);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(endTime - startTime).count();
        
        // Update performance metrics
        // Get performance optimizer instance and update metrics
        // (Assuming we can access it here, otherwise we'll remove this)
    }
    
    void ProcessStateGroup(const std::vector<IActor*>& actors, 
                          ActorState currentState, ActorState targetState) {
        // If current state equals target state, skip processing
        if (currentState == targetState) {
            return;
        }
        
        // Determine optimal processing strategy based on state transition
        bool canParallelize = CanParallelizeTransition(currentState, targetState);
        
        if (canParallelize && actors.size() > 4) {
            ProcessActorsParallel(actors, targetState);
        } else {
            ProcessActorsSequential(actors, targetState);
        }
    }
    
    bool CanParallelizeTransition(ActorState from, ActorState to) const {
        // Some transitions can be safely parallelized
        switch (from) {
            case ActorState::Created:
                return to == ActorState::Initializing || to == ActorState::Initialized;
            case ActorState::Initialized:
                return to == ActorState::Active;
            case ActorState::Active:
                return to == ActorState::Pausing;
            case ActorState::Paused:
                return to == ActorState::Resuming;
            case ActorState::Destroying:
                return to == ActorState::Destroyed;
            default:
                return false;
        }
    }
    
    void ProcessActorsParallel(const std::vector<IActor*>& actors, ActorState targetState) {
        const size_t numThreads = std::min(actors.size(), 
                                          static_cast<size_t>(std::thread::hardware_concurrency()));
        const size_t actorsPerThread = actors.size() / numThreads;
        
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        
        for (size_t i = 0; i < numThreads; ++i) {
            size_t start = i * actorsPerThread;
            size_t end = (i == numThreads - 1) ? actors.size() : (i + 1) * actorsPerThread;
            
            threads.emplace_back([this, &actors, targetState, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    ActorLifecycleManager::Instance().TransitionTo(actors[j], targetState);
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    void ProcessActorsSequential(const std::vector<IActor*>& actors, ActorState targetState) {
        for (auto* actor : actors) {
            ActorLifecycleManager::Instance().TransitionTo(actor, targetState);
        }
    }
    
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_;
    std::thread workerThread_;
    std::queue<BatchOperation> batchQueue_;
    std::chrono::milliseconds batchTimeout_{100}; // Default 100ms timeout
};

/**
 * Performance optimizer for lifecycle operations
 */
class LifecyclePerformanceOptimizer {
public:
    static LifecyclePerformanceOptimizer& Instance() {
        static LifecyclePerformanceOptimizer instance;
        return instance;
    }
    
    // Configuration
    struct Config {
        bool enableBatching;
        bool enableParallelProcessing;
        bool enableObjectPooling;
        bool enablePerformanceMonitoring;
        
        size_t batchSize;
        size_t maxBatchSize;
        std::chrono::milliseconds batchTimeout;
        
        size_t maxPoolSize;
        double targetTransitionsPerSecond;
        double memoryUsageWarningThreshold; // MB
        
        Config() : enableBatching(true), enableParallelProcessing(true), 
                   enableObjectPooling(true), enablePerformanceMonitoring(true),
                   batchSize(32), maxBatchSize(128), batchTimeout(100),
                   maxPoolSize(100), targetTransitionsPerSecond(1000.0),
                   memoryUsageWarningThreshold(100.0) {}
    };
    
    void Initialize(const Config& config) {
        config_ = config;
        metrics_.startTime = std::chrono::high_resolution_clock::now();
        
        if (config_.enableBatching) {
            batchProcessor_.Start();
            batchProcessor_.SetBatchTimeout(config_.batchTimeout);
        }
        
        if (config_.enableObjectPooling) {
            LifecycleContextPool::Instance().SetMaxPoolSize(config_.maxPoolSize);
        }
        
        initialized_ = true;
    }
    
    void Initialize() {
        Initialize(Config{});
    }
    
    void Shutdown() {
        if (!initialized_) return;
        
        batchProcessor_.Stop();
        LifecycleContextPool::Instance().Clear();
        
        initialized_ = false;
    }
    
    // Optimized batch operations
    void BatchTransition(const std::vector<IActor*>& actors, ActorState targetState) {
        if (!config_.enableBatching || actors.size() < 2) {
            // Fall back to individual transitions
            for (auto* actor : actors) {
                ActorLifecycleManager::Instance().TransitionTo(actor, targetState);
            }
            return;
        }
        
        // Split into optimal batch sizes
        for (size_t i = 0; i < actors.size(); i += config_.maxBatchSize) {
            size_t end = std::min(i + config_.maxBatchSize, actors.size());
            std::vector<IActor*> batch(actors.begin() + i, actors.begin() + end);
            
            batchProcessor_.QueueBatch(std::move(batch), targetState);
        }
    }
    
    // Memory optimization
    std::unique_ptr<LifecycleContext> AcquireContext() {
        if (config_.enableObjectPooling) {
            return LifecycleContextPool::Instance().Acquire();
        }
        return std::make_unique<LifecycleContext>();
    }
    
    void ReleaseContext(std::unique_ptr<LifecycleContext> context) {
        if (config_.enableObjectPooling) {
            LifecycleContextPool::Instance().Release(std::move(context));
        }
        // Otherwise, context is automatically destroyed
    }
    
    // Performance monitoring
    PerformanceMetrics& GetMetrics() { return metrics_; }
    const PerformanceMetrics& GetMetrics() const { return metrics_; }
    
    void RecordTransition(double duration) {
        if (config_.enablePerformanceMonitoring) {
            metrics_.UpdateTransitionMetrics(duration);
            
            // Check performance warnings
            if (metrics_.averageTransitionTime > 0.01) { // 10ms warning threshold
                std::cout << "[Lifecycle] WARNING: Slow transitions detected (avg: " 
                          << metrics_.averageTransitionTime * 1000 << "ms)" << std::endl;
            }
        }
    }
    
    // Performance analysis
    void PrintPerformanceReport() const {
        std::cout << "\n=== Lifecycle Performance Report ===" << std::endl;
        std::cout << "Total transitions: " << metrics_.transitionCount << std::endl;
        std::cout << "Average transition time: " << (metrics_.averageTransitionTime * 1000) << "ms" << std::endl;
        std::cout << "Transitions per second: " << metrics_.GetTransitionsPerSecond() << std::endl;
        std::cout << "Total batches processed: " << metrics_.batchCount << std::endl;
        std::cout << "Average batch time: " << (metrics_.averageBatchTime * 1000) << "ms" << std::endl;
        std::cout << "Total batched actors: " << metrics_.totalBatchedActors << std::endl;
        std::cout << "Context pool size: " << LifecycleContextPool::Instance().GetPoolSize() << std::endl;
        std::cout << "Batch queue size: " << batchProcessor_.GetQueueSize() << std::endl;
        std::cout << "Memory usage: " << (metrics_.currentMemoryUsage / 1024.0 / 1024.0) << " MB" << std::endl;
        std::cout << "====================================\n" << std::endl;
    }
    
    // Performance optimization recommendations
    void AnalyzePerformance() const {
        std::cout << "\n=== Performance Analysis ===" << std::endl;
        
        // Transition speed analysis
        double tps = metrics_.GetTransitionsPerSecond();
        if (tps < config_.targetTransitionsPerSecond * 0.5) {
            std::cout << "⚠️  Low transition rate: " << tps << " TPS (target: " 
                      << config_.targetTransitionsPerSecond << ")" << std::endl;
            std::cout << "   Recommendations:" << std::endl;
            std::cout << "   - Enable batching if disabled" << std::endl;
            std::cout << "   - Increase batch size" << std::endl;
            std::cout << "   - Enable parallel processing" << std::endl;
        } else {
            std::cout << "✅ Good transition rate: " << tps << " TPS" << std::endl;
        }
        
        // Memory usage analysis
        double memoryMB = metrics_.currentMemoryUsage / 1024.0 / 1024.0;
        if (memoryMB > config_.memoryUsageWarningThreshold) {
            std::cout << "⚠️  High memory usage: " << memoryMB << " MB" << std::endl;
            std::cout << "   Recommendations:" << std::endl;
            std::cout << "   - Enable object pooling if disabled" << std::endl;
            std::cout << "   - Reduce max pool size" << std::endl;
            std::cout << "   - Check for memory leaks in actors" << std::endl;
        } else {
            std::cout << "✅ Memory usage within limits: " << memoryMB << " MB" << std::endl;
        }
        
        // Batch efficiency analysis
        if (metrics_.batchCount > 0) {
            double avgBatchedActors = static_cast<double>(metrics_.totalBatchedActors) / metrics_.batchCount;
            if (avgBatchedActors < config_.batchSize * 0.5) {
                std::cout << "⚠️  Low batch efficiency: " << avgBatchedActors << " actors/batch" << std::endl;
                std::cout << "   Recommendations:" << std::endl;
                std::cout << "   - Reduce batch size" << std::endl;
                std::cout << "   - Increase batch timeout" << std::endl;
            } else {
                std::cout << "✅ Good batch efficiency: " << avgBatchedActors << " actors/batch" << std::endl;
            }
        }
        
        std::cout << "===========================\n" << std::endl;
    }
    
    const Config& GetConfig() const { return config_; }
    
    void UpdateConfig(const Config& config) {
        config_ = config;
        
        if (config_.enableObjectPooling) {
            LifecycleContextPool::Instance().SetMaxPoolSize(config_.maxPoolSize);
        }
        
        if (config_.enableBatching) {
            batchProcessor_.SetBatchTimeout(config_.batchTimeout);
        }
    }

private:
    LifecyclePerformanceOptimizer() = default;
    
    bool initialized_ = false;
    Config config_;
    PerformanceMetrics metrics_;
    BatchProcessor batchProcessor_;
};

/**
 * Smart lifecycle manager wrapper with automatic performance optimization
 */
class OptimizedLifecycleManager {
public:
    static OptimizedLifecycleManager& Instance() {
        static OptimizedLifecycleManager instance;
        return instance;
    }
    
    void Initialize() {
        auto& optimizer = LifecyclePerformanceOptimizer::Instance();
        optimizer.Initialize();
        
        // Register performance optimization hooks with base manager
        auto& baseManager = ActorLifecycleManager::Instance();
        
        // Hook to use optimized context allocation
        baseManager.RegisterHook(LifecycleEvent::PreCreate, "optimization",
            [&optimizer](LifecycleContext& context) {
                // This is called after context creation, so we can't change allocation here
                // But we can track memory usage
                (void)context;
                optimizer.GetMetrics().currentMemoryUsage += sizeof(LifecycleContext);
            });
        
        // Hook to record transition performance
        baseManager.RegisterHook(LifecycleEvent::PostCreate, "perf_monitoring",
            [&optimizer](LifecycleContext& context) {
                auto now = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double>(
                    now - context.stats.creationTime).count();
                optimizer.RecordTransition(duration);
            });
    }
    
    void Shutdown() {
        LifecyclePerformanceOptimizer::Instance().Shutdown();
    }
    
    // Optimized batch operations
    void BatchCreate(const std::vector<IActor*>& actors) {
        LifecyclePerformanceOptimizer::Instance().BatchTransition(actors, ActorState::Created);
    }
    
    void BatchInitialize(const std::vector<IActor*>& actors) {
        LifecyclePerformanceOptimizer::Instance().BatchTransition(actors, ActorState::Initialized);
    }
    
    void BatchActivate(const std::vector<IActor*>& actors) {
        LifecyclePerformanceOptimizer::Instance().BatchTransition(actors, ActorState::Active);
    }
    
    void BatchDestroy(const std::vector<IActor*>& actors) {
        LifecyclePerformanceOptimizer::Instance().BatchTransition(actors, ActorState::Destroying);
    }
    
    // Performance monitoring
    void PrintReport() const {
        LifecyclePerformanceOptimizer::Instance().PrintPerformanceReport();
        LifecyclePerformanceOptimizer::Instance().AnalyzePerformance();
    }
};

} // namespace lifecycle