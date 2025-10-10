#pragma once
#include "EntityManagerV2.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace ecs {

// Component access pattern for dependency analysis
enum class ComponentAccess {
    Read,       // System only reads component
    Write,      // System writes to component
    ReadWrite   // System both reads and writes
};

// System component dependency declaration
struct ComponentDependency {
    std::type_index type;
    ComponentAccess access;
    
    ComponentDependency(std::type_index t, ComponentAccess a)
        : type(t), access(a) {}
    
    template<typename T>
    static ComponentDependency Read() {
        return {std::type_index(typeid(T)), ComponentAccess::Read};
    }
    
    template<typename T>
    static ComponentDependency Write() {
        return {std::type_index(typeid(T)), ComponentAccess::Write};
    }
    
    template<typename T>
    static ComponentDependency ReadWrite() {
        return {std::type_index(typeid(T)), ComponentAccess::ReadWrite};
    }
};

// Update phase for system ordering
enum class UpdatePhase {
    Input,          // Input handling
    PreUpdate,      // Before main simulation
    Update,         // Main simulation logic
    PostUpdate,     // After simulation, before rendering
    RenderPrep      // Prepare data for rendering
};

// Base class for systems in V2 architecture
class SystemV2 {
public:
    virtual ~SystemV2() = default;
    
    // Update function with delta time
    virtual void Update(EntityManagerV2& entityManager, double dt) = 0;
    
    // Declare component dependencies (override in derived classes)
    virtual std::vector<ComponentDependency> GetDependencies() const { return {}; }
    
    // Declare update phase (override in derived classes)
    virtual UpdatePhase GetUpdatePhase() const { return UpdatePhase::Update; }
    
    // System name for debugging/profiling
    virtual const char* GetName() const { return "SystemV2"; }
    
    // Profiling data
    struct ProfileData {
        double lastUpdateTime = 0.0;  // Milliseconds
        size_t entitiesProcessed = 0;
        size_t updateCount = 0;
    };
    
    const ProfileData& GetProfileData() const { return profileData_; }
    
protected:
    void RecordUpdateStart() {
        updateStartTime_ = std::chrono::high_resolution_clock::now();
    }
    
    void RecordUpdateEnd(size_t entitiesProcessed) {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = endTime - updateStartTime_;
        
        profileData_.lastUpdateTime = duration.count();
        profileData_.entitiesProcessed = entitiesProcessed;
        profileData_.updateCount++;
    }
    
private:
    ProfileData profileData_;
    std::chrono::high_resolution_clock::time_point updateStartTime_;
};

// Thread pool for parallel system execution
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) {
        workers_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] { WorkerThread(); });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    template<typename Func>
    void Enqueue(Func&& task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            tasks_.emplace(std::forward<Func>(task));
        }
        condition_.notify_one();
    }
    
    void WaitForCompletion() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        completionCondition_.wait(lock, [this] {
            return tasks_.empty() && activeWorkers_ == 0;
        });
    }
    
    size_t GetThreadCount() const { return workers_.size(); }
    
private:
    void WorkerThread() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                
                if (stop_ && tasks_.empty()) return;
                
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    activeWorkers_++;
                }
            }
            
            if (task) {
                task();
                
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    activeWorkers_--;
                    completionCondition_.notify_all();
                }
            }
        }
    }
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::condition_variable completionCondition_;
    std::atomic<size_t> activeWorkers_{0};
    bool stop_ = false;
};

// System scheduler with parallel execution support
class SystemSchedulerV2 {
public:
    explicit SystemSchedulerV2(size_t numThreads = std::thread::hardware_concurrency())
        : threadPool_(numThreads) {}
    
    ~SystemSchedulerV2() = default;
    
    // Register a system
    template<typename T, typename... Args>
    T& RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of<SystemV2, T>::value, 
                     "System must derive from SystemV2");
        
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        systems_.emplace_back(std::move(system));
        
        // Rebuild schedule after adding system
        needsReschedule_ = true;
        
        return ref;
    }
    
    // Update all systems (with parallel execution)
    void UpdateAll(EntityManagerV2& entityManager, double dt) {
        if (needsReschedule_) {
            RebuildSchedule();
            needsReschedule_ = false;
        }
        
        // Execute systems phase by phase
        for (const auto& phase : schedule_) {
            ExecutePhase(phase, entityManager, dt);
        }
    }
    
    // Get profiling data for all systems
    struct SystemProfile {
        const char* name;
        UpdatePhase phase;
        double updateTime;
        size_t entitiesProcessed;
        size_t updateCount;
    };
    
    std::vector<SystemProfile> GetSystemProfiles() const {
        std::vector<SystemProfile> profiles;
        profiles.reserve(systems_.size());
        
        for (const auto& system : systems_) {
            const auto& data = system->GetProfileData();
            profiles.push_back({
                system->GetName(),
                system->GetUpdatePhase(),
                data.lastUpdateTime,
                data.entitiesProcessed,
                data.updateCount
            });
        }
        
        return profiles;
    }
    
    double GetTotalUpdateTime() const {
        double total = 0.0;
        for (const auto& system : systems_) {
            total += system->GetProfileData().lastUpdateTime;
        }
        return total;
    }
    
    void Clear() {
        systems_.clear();
        schedule_.clear();
        needsReschedule_ = true;
    }
    
    size_t GetThreadCount() const {
        return threadPool_.GetThreadCount();
    }
    
private:
    // Schedule: list of system batches (parallel groups) for each phase
    using SystemBatch = std::vector<SystemV2*>;
    using PhaseSchedule = std::vector<SystemBatch>;
    std::vector<PhaseSchedule> schedule_;  // One schedule per UpdatePhase
    
    void RebuildSchedule() {
        // Group systems by phase
        std::unordered_map<UpdatePhase, std::vector<SystemV2*>> systemsByPhase;
        for (auto& system : systems_) {
            systemsByPhase[system->GetUpdatePhase()].push_back(system.get());
        }
        
        // Build schedule for each phase
        schedule_.clear();
        schedule_.resize(static_cast<size_t>(UpdatePhase::RenderPrep) + 1);
        
        for (auto& [phase, systemList] : systemsByPhase) {
            size_t phaseIndex = static_cast<size_t>(phase);
            schedule_[phaseIndex] = BuildParallelSchedule(systemList);
        }
    }
    
    // Build parallel schedule using dependency analysis
    PhaseSchedule BuildParallelSchedule(const std::vector<SystemV2*>& systems) {
        PhaseSchedule schedule;
        std::unordered_set<SystemV2*> scheduled;
        
        while (scheduled.size() < systems.size()) {
            SystemBatch batch;
            std::unordered_set<std::type_index> writtenTypes;
            std::unordered_set<std::type_index> readTypes;
            
            // Find systems that can run in parallel
            for (SystemV2* system : systems) {
                if (scheduled.count(system)) continue;
                
                // Check if this system conflicts with current batch
                bool conflicts = false;
                auto deps = system->GetDependencies();
                
                for (const auto& dep : deps) {
                    if (dep.access == ComponentAccess::Write || 
                        dep.access == ComponentAccess::ReadWrite) {
                        // Writing - conflicts with any existing read or write
                        if (readTypes.count(dep.type) || writtenTypes.count(dep.type)) {
                            conflicts = true;
                            break;
                        }
                    } else {
                        // Reading - conflicts with writes only
                        if (writtenTypes.count(dep.type)) {
                            conflicts = true;
                            break;
                        }
                    }
                }
                
                if (!conflicts) {
                    batch.push_back(system);
                    scheduled.insert(system);
                    
                    // Record access patterns
                    for (const auto& dep : deps) {
                        if (dep.access == ComponentAccess::Read) {
                            readTypes.insert(dep.type);
                        } else {
                            writtenTypes.insert(dep.type);
                        }
                    }
                }
            }
            
            if (!batch.empty()) {
                schedule.push_back(batch);
            } else {
                // Safety: if no progress, schedule remaining systems sequentially
                for (SystemV2* system : systems) {
                    if (!scheduled.count(system)) {
                        schedule.push_back({system});
                        scheduled.insert(system);
                    }
                }
                break;
            }
        }
        
        return schedule;
    }
    
    void ExecutePhase(const PhaseSchedule& phase, EntityManagerV2& entityManager, double dt) {
        for (const auto& batch : phase) {
            if (batch.size() == 1) {
                // Single system - execute directly
                batch[0]->Update(entityManager, dt);
            } else {
                // Multiple systems - execute in parallel
                for (SystemV2* system : batch) {
                    threadPool_.Enqueue([system, &entityManager, dt] {
                        system->Update(entityManager, dt);
                    });
                }
                threadPool_.WaitForCompletion();
            }
        }
    }
    
    std::vector<std::unique_ptr<SystemV2>> systems_;
    ThreadPool threadPool_;
    bool needsReschedule_ = true;
};

} // namespace ecs
