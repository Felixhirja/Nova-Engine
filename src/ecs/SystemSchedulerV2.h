#pragma once
#include "EntityManagerV2.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <typeindex>
#include <unordered_map>
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

// Execution stage for individual system updates
enum class UpdateStage {
    PreUpdate,
    Update,
    PostUpdate
};

// System execution dependency declaration
struct SystemDependency {
    std::type_index type;

    explicit SystemDependency(std::type_index t) : type(t) {}

    template<typename T>
    static SystemDependency Requires() {
        return SystemDependency(std::type_index(typeid(T)));
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

    // Declare system execution dependencies
    virtual std::vector<SystemDependency> GetSystemDependencies() const { return {}; }

    // Declare update phase (override in derived classes)
    virtual UpdatePhase GetUpdatePhase() const { return UpdatePhase::Update; }

    // Declare supported execution stages
    virtual bool SupportsStage(UpdateStage stage) const {
        return stage == UpdateStage::Update;
    }

    // Optional stage-specific hooks
    virtual void PreUpdate(EntityManagerV2&, double) {}
    virtual void PostUpdate(EntityManagerV2&, double) {}

    virtual void RunStage(EntityManagerV2& entityManager, double dt, UpdateStage stage) {
        switch (stage) {
            case UpdateStage::PreUpdate:
                PreUpdate(entityManager, dt);
                break;
            case UpdateStage::Update:
                Update(entityManager, dt);
                break;
            case UpdateStage::PostUpdate:
                PostUpdate(entityManager, dt);
                break;
        }
    }

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
        for (size_t phaseIndex = 0; phaseIndex < phaseSchedules_.size(); ++phaseIndex) {
            ExecutePhase(phaseSchedules_[phaseIndex], entityManager, dt);
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
        for (auto& phase : phaseSchedules_) {
            phase.order.clear();
            phase.dependencies.clear();
            phase.componentAccess.clear();
        }
        needsReschedule_ = true;
    }
    
    size_t GetThreadCount() const {
        return threadPool_.GetThreadCount();
    }
    
private:
    struct PhaseScheduleData {
        std::vector<SystemV2*> order;
        std::unordered_map<SystemV2*, std::vector<SystemV2*>> dependencies;
        std::unordered_map<SystemV2*, std::vector<ComponentDependency>> componentAccess;
    };

    class TaskGraph {
    public:
        using TaskFunc = std::function<void()>;

        size_t AddTask(TaskFunc func) {
            tasks_.push_back(TaskNode{std::move(func), {}, {}});
            return tasks_.size() - 1;
        }

        void AddDependency(size_t before, size_t after) {
            if (before == after) return;
            tasks_[after].prerequisites.push_back(before);
            tasks_[before].dependents.push_back(after);
        }

        void Execute(ThreadPool& threadPool) {
            if (tasks_.empty()) return;

            std::vector<size_t> pendingCounts(tasks_.size(), 0);
            for (size_t i = 0; i < tasks_.size(); ++i) {
                pendingCounts[i] = tasks_[i].prerequisites.size();
            }

            std::mutex mutex;
            std::condition_variable cv;
            std::queue<size_t> ready;
            size_t remaining = tasks_.size();

            for (size_t i = 0; i < tasks_.size(); ++i) {
                if (pendingCounts[i] == 0) {
                    ready.push(i);
                }
            }

            std::vector<size_t> toSubmit;

            auto submitTask = [&](size_t index) {
                threadPool.Enqueue([&, index]() {
                    tasks_[index].func();

                    std::unique_lock<std::mutex> lock(mutex);
                    for (size_t dependent : tasks_[index].dependents) {
                        size_t& pending = pendingCounts[dependent];
                        if (pending == 0) {
                            continue;
                        }
                        if (--pending == 0) {
                            ready.push(dependent);
                        }
                    }

                    if (--remaining == 0) {
                        lock.unlock();
                        cv.notify_all();
                        return;
                    }

                    lock.unlock();
                    cv.notify_all();
                });
            };

            {
                std::unique_lock<std::mutex> lock(mutex);
                while (!ready.empty()) {
                    toSubmit.push_back(ready.front());
                    ready.pop();
                }
            }

            for (size_t index : toSubmit) {
                submitTask(index);
            }
            toSubmit.clear();

            while (true) {
                std::unique_lock<std::mutex> lock(mutex);
                if (remaining == 0) {
                    break;
                }

                if (ready.empty()) {
                    cv.wait(lock, [&] { return remaining == 0 || !ready.empty(); });
                    if (remaining == 0) {
                        break;
                    }
                }

                while (!ready.empty()) {
                    toSubmit.push_back(ready.front());
                    ready.pop();
                }

                lock.unlock();

                for (size_t index : toSubmit) {
                    submitTask(index);
                }
                toSubmit.clear();
            }

            // Ensure all tasks have completed before returning
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&] { return remaining == 0; });
        }

    private:
        struct TaskNode {
            TaskFunc func;
            std::vector<size_t> dependents;
            std::vector<size_t> prerequisites;
        };

        std::vector<TaskNode> tasks_;
    };

    void RebuildSchedule() {
        static constexpr size_t kPhaseCount = static_cast<size_t>(UpdatePhase::RenderPrep) + 1;

        for (auto& phase : phaseSchedules_) {
            phase.order.clear();
            phase.dependencies.clear();
            phase.componentAccess.clear();
        }

        if (systems_.empty()) {
            return;
        }

        std::unordered_map<std::type_index, SystemV2*> typeToSystem;
        typeToSystem.reserve(systems_.size());

        for (auto& systemPtr : systems_) {
            SystemV2* system = systemPtr.get();
            std::type_index type(typeid(*system));
            if (!typeToSystem.emplace(type, system).second) {
                throw std::runtime_error(std::string("Duplicate system registration detected for type: ") + system->GetName());
            }
        }

        std::array<std::vector<SystemV2*>, kPhaseCount> systemsByPhase;
        std::unordered_map<SystemV2*, std::vector<SystemV2*>> samePhaseDependencies;
        std::unordered_map<SystemV2*, std::vector<SystemV2*>> adjacency;

        for (auto& systemPtr : systems_) {
            SystemV2* system = systemPtr.get();
            size_t phaseIndex = static_cast<size_t>(system->GetUpdatePhase());
            systemsByPhase[phaseIndex].push_back(system);

            auto deps = system->GetSystemDependencies();
            for (const auto& dep : deps) {
                auto found = typeToSystem.find(dep.type);
                if (found == typeToSystem.end()) {
                    throw std::runtime_error(std::string("System dependency for ") + system->GetName() + " not registered");
                }

                SystemV2* dependencySystem = found->second;
                size_t depPhaseIndex = static_cast<size_t>(dependencySystem->GetUpdatePhase());

                if (depPhaseIndex > phaseIndex) {
                    throw std::runtime_error(std::string("Invalid dependency: ") + system->GetName() + " depends on future phase system " + dependencySystem->GetName());
                }

                if (depPhaseIndex == phaseIndex) {
                    samePhaseDependencies[system].push_back(dependencySystem);
                    adjacency[dependencySystem].push_back(system);
                }
            }
        }

        for (size_t phaseIndex = 0; phaseIndex < kPhaseCount; ++phaseIndex) {
            auto& systems = systemsByPhase[phaseIndex];
            PhaseScheduleData data;

            if (systems.empty()) {
                phaseSchedules_[phaseIndex] = std::move(data);
                continue;
            }

            std::unordered_map<SystemV2*, size_t> indegree;
            for (SystemV2* system : systems) {
                size_t degree = 0;
                auto depIt = samePhaseDependencies.find(system);
                if (depIt != samePhaseDependencies.end()) {
                    degree = depIt->second.size();
                }
                indegree[system] = degree;
            }

            std::queue<SystemV2*> ready;
            for (SystemV2* system : systems) {
                if (indegree[system] == 0) {
                    ready.push(system);
                }
            }

            while (!ready.empty()) {
                SystemV2* current = ready.front();
                ready.pop();
                data.order.push_back(current);

                auto dependentsIt = adjacency.find(current);
                if (dependentsIt == adjacency.end()) {
                    continue;
                }

                for (SystemV2* dependent : dependentsIt->second) {
                    auto inIt = indegree.find(dependent);
                    if (inIt == indegree.end()) {
                        continue;
                    }

                    if (inIt->second == 0) {
                        continue;
                    }

                    inIt->second--;
                    if (inIt->second == 0) {
                        ready.push(dependent);
                    }
                }
            }

            if (data.order.size() != systems.size()) {
                throw std::runtime_error("Cycle detected in system dependencies for phase scheduling");
            }

            for (SystemV2* system : systems) {
                auto depIt = samePhaseDependencies.find(system);
                if (depIt != samePhaseDependencies.end()) {
                    data.dependencies.emplace(system, depIt->second);
                }
                data.componentAccess.emplace(system, system->GetDependencies());
            }

            phaseSchedules_[phaseIndex] = std::move(data);
        }
    }

    bool HasComponentConflict(const std::vector<ComponentDependency>& a,
                              const std::vector<ComponentDependency>& b) const {
        for (const auto& depA : a) {
            for (const auto& depB : b) {
                if (depA.type != depB.type) {
                    continue;
                }

                const bool aWrites = depA.access == ComponentAccess::Write ||
                                     depA.access == ComponentAccess::ReadWrite;
                const bool bWrites = depB.access == ComponentAccess::Write ||
                                     depB.access == ComponentAccess::ReadWrite;

                if (aWrites || bWrites) {
                    return true;
                }
            }
        }

        return false;
    }

    void ExecutePhase(const PhaseScheduleData& phase, EntityManagerV2& entityManager, double dt) {
        ExecuteStage(phase, UpdateStage::PreUpdate, entityManager, dt);
        ExecuteStage(phase, UpdateStage::Update, entityManager, dt);
        ExecuteStage(phase, UpdateStage::PostUpdate, entityManager, dt);
    }

    void ExecuteStage(const PhaseScheduleData& phase,
                      UpdateStage stage,
                      EntityManagerV2& entityManager,
                      double dt) {
        if (phase.order.empty()) {
            return;
        }

        TaskGraph graph;
        std::unordered_map<SystemV2*, size_t> taskIds;

        for (SystemV2* system : phase.order) {
            if (!system->SupportsStage(stage)) {
                continue;
            }

            size_t id = graph.AddTask([system, &entityManager, dt, stage]() {
                system->RunStage(entityManager, dt, stage);
            });
            taskIds.emplace(system, id);
        }

        if (taskIds.empty()) {
            return;
        }

        for (SystemV2* system : phase.order) {
            auto taskIt = taskIds.find(system);
            if (taskIt == taskIds.end()) {
                continue;
            }

            auto depIt = phase.dependencies.find(system);
            if (depIt == phase.dependencies.end()) {
                continue;
            }

            for (SystemV2* dependency : depIt->second) {
                auto depTaskIt = taskIds.find(dependency);
                if (depTaskIt != taskIds.end()) {
                    graph.AddDependency(depTaskIt->second, taskIt->second);
                }
            }
        }

        for (size_t i = 0; i < phase.order.size(); ++i) {
            SystemV2* a = phase.order[i];
            auto taskAIt = taskIds.find(a);
            if (taskAIt == taskIds.end()) {
                continue;
            }

            const auto& accessA = phase.componentAccess.at(a);

            for (size_t j = i + 1; j < phase.order.size(); ++j) {
                SystemV2* b = phase.order[j];
                auto taskBIt = taskIds.find(b);
                if (taskBIt == taskIds.end()) {
                    continue;
                }

                const auto& accessB = phase.componentAccess.at(b);

                if (HasComponentConflict(accessA, accessB)) {
                    graph.AddDependency(taskAIt->second, taskBIt->second);
                }
            }
        }

        graph.Execute(threadPool_);
    }

    std::vector<std::unique_ptr<SystemV2>> systems_;
    std::array<PhaseScheduleData, static_cast<size_t>(UpdatePhase::RenderPrep) + 1> phaseSchedules_{};
    ThreadPool threadPool_;
    bool needsReschedule_ = true;
};

} // namespace ecs
