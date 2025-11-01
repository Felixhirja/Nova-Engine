#pragma once
#include <chrono>
#include <string>
#include <unordered_map>

namespace ecs {

// Performance profiling utilities
class PerformanceMetrics {
public:
    struct Timer {
        std::chrono::high_resolution_clock::time_point start;
        std::chrono::high_resolution_clock::time_point end;
        bool running = false;
        
        void Start() {
            start = std::chrono::high_resolution_clock::now();
            running = true;
        }
        
        double Stop() {
            end = std::chrono::high_resolution_clock::now();
            running = false;
            return ElapsedMs();
        }
        
        double ElapsedMs() const {
            auto endTime = running ? std::chrono::high_resolution_clock::now() : end;
            return std::chrono::duration<double, std::milli>(endTime - start).count();
        }
    };
    
    struct SystemMetrics {
        double totalTime = 0.0;
        double minTime = 1e9;
        double maxTime = 0.0;
        size_t callCount = 0;
        size_t entitiesProcessed = 0;
        
        void Record(double timeMs, size_t entities) {
            totalTime += timeMs;
            minTime = std::min(minTime, timeMs);
            maxTime = std::max(maxTime, timeMs);
            callCount++;
            entitiesProcessed += entities;
        }
        
        double GetAverageTime() const {
            return callCount > 0 ? totalTime / callCount : 0.0;
        }
        
        double GetAverageEntitiesPerCall() const {
            return callCount > 0 ? static_cast<double>(entitiesProcessed) / callCount : 0.0;
        }
    };
    
    void RecordSystem(const std::string& name, double timeMs, size_t entities) {
        metrics_[name].Record(timeMs, entities);
    }
    
    const SystemMetrics* GetMetrics(const std::string& name) const {
        auto it = metrics_.find(name);
        return it != metrics_.end() ? &it->second : nullptr;
    }
    
    void Clear() {
        metrics_.clear();
    }
    
    const std::unordered_map<std::string, SystemMetrics>& GetAllMetrics() const {
        return metrics_;
    }
    
private:
    std::unordered_map<std::string, SystemMetrics> metrics_;
};

// RAII scope timer
class ScopedTimer {
public:
    explicit ScopedTimer(PerformanceMetrics& metrics, const std::string& name, size_t entities = 0)
        : metrics_(metrics), name_(name), entities_(entities) {
        timer_.Start();
    }
    
    ~ScopedTimer() {
        double elapsed = timer_.Stop();
        metrics_.RecordSystem(name_, elapsed, entities_);
    }
    
private:
    PerformanceMetrics& metrics_;
    std::string name_;
    size_t entities_;
    PerformanceMetrics::Timer timer_;
};

} // namespace ecs
