#include "ActorLifecycleManager.h"
#include "ecs/EntityManager.h"  // Include EntityManager first
#include "IActor.h"
#include "ActorContext.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace lifecycle {

// LifecycleStats implementation
double LifecycleStats::getLifetime() const {
    auto endTime = isAlive() ? std::chrono::high_resolution_clock::now() : destructionTime;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - creationTime);
    return duration.count() / 1000000.0; // Convert to seconds
}

double LifecycleStats::getInitializationDuration() const {
    if (initializationTime == std::chrono::high_resolution_clock::time_point{}) {
        return 0.0;
    }
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(initializationTime - creationTime);
    return duration.count() / 1000000.0; // Convert to seconds
}

double LifecycleStats::getActivationDuration() const {
    if (activationTime == std::chrono::high_resolution_clock::time_point{}) {
        return 0.0;
    }
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(activationTime - initializationTime);
    return duration.count() / 1000000.0; // Convert to seconds
}

// LifecycleContext implementation
bool LifecycleContext::CanTransitionTo(ActorState newState) const {
    // Define valid state transitions
    switch (state) {
        case ActorState::Created:
            return newState == ActorState::Initializing || newState == ActorState::Destroying;
        case ActorState::Initializing:
            return newState == ActorState::Initialized || newState == ActorState::Destroying;
        case ActorState::Initialized:
            return newState == ActorState::Active || newState == ActorState::Destroying;
        case ActorState::Active:
            return newState == ActorState::Pausing || newState == ActorState::Destroying;
        case ActorState::Pausing:
            return newState == ActorState::Paused || newState == ActorState::Destroying;
        case ActorState::Paused:
            return newState == ActorState::Resuming || newState == ActorState::Destroying;
        case ActorState::Resuming:
            return newState == ActorState::Active || newState == ActorState::Destroying;
        case ActorState::Destroying:
            return newState == ActorState::Destroyed;
        case ActorState::Destroyed:
            return false; // No transitions from destroyed state
    }
    return false;
}

std::string LifecycleContext::GetMetadata(const std::string& key, const std::string& defaultValue) const {
    auto it = metadata.find(key);
    return (it != metadata.end()) ? it->second : defaultValue;
}

// ActorLifecycleManager implementation
ActorLifecycleManager& ActorLifecycleManager::Instance() {
    static ActorLifecycleManager instance;
    return instance;
}

void ActorLifecycleManager::RegisterHook(LifecycleEvent event, const std::string& name, LifecycleHook hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    hooks_[event][name] = std::move(hook);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Registered hook '" << name << "' for event " 
                  << utils::EventToString(event) << std::endl;
    }
}

void ActorLifecycleManager::UnregisterHook(LifecycleEvent event, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto eventIt = hooks_.find(event);
    if (eventIt != hooks_.end()) {
        eventIt->second.erase(name);
        
        if (config_.enableDebugLogging) {
            std::cout << "[ActorLifecycle] Unregistered hook '" << name << "' for event " 
                      << utils::EventToString(event) << std::endl;
        }
    }
}

void ActorLifecycleManager::ClearHooks(LifecycleEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    hooks_[event].clear();
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Cleared all hooks for event " 
                  << utils::EventToString(event) << std::endl;
    }
}

void ActorLifecycleManager::ClearAllHooks() {
    std::lock_guard<std::mutex> lock(mutex_);
    hooks_.clear();
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Cleared all hooks" << std::endl;
    }
}

void ActorLifecycleManager::RegisterValidator(const std::string& name, StateValidator validator) {
    std::lock_guard<std::mutex> lock(mutex_);
    validators_[name] = std::move(validator);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Registered validator '" << name << "'" << std::endl;
    }
}

void ActorLifecycleManager::UnregisterValidator(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    validators_.erase(name);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Unregistered validator '" << name << "'" << std::endl;
    }
}

bool ActorLifecycleManager::ValidateTransition(const LifecycleContext& context, ActorState newState) {
    if (!config_.enableValidation) {
        return true;
    }
    
    // Check basic state transition validity
    if (!context.CanTransitionTo(newState)) {
        if (config_.enableDebugLogging) {
            std::cout << "[ActorLifecycle] Invalid transition from " 
                      << utils::StateToString(context.state) << " to " 
                      << utils::StateToString(newState) << " for actor " 
                      << context.actorName << std::endl;
        }
        return false;
    }
    
    // Run custom validators
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, validator] : validators_) {
        if (!validator(context, newState)) {
            if (config_.enableDebugLogging) {
                std::cout << "[ActorLifecycle] Validator '" << name 
                          << "' rejected transition for actor " << context.actorName << std::endl;
            }
            return false;
        }
    }
    
    return true;
}

void ActorLifecycleManager::RegisterOptimizer(const std::string& name, PerformanceOptimizer optimizer) {
    std::lock_guard<std::mutex> lock(mutex_);
    optimizers_[name] = std::move(optimizer);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Registered optimizer '" << name << "'" << std::endl;
    }
}

void ActorLifecycleManager::UnregisterOptimizer(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    optimizers_.erase(name);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Unregistered optimizer '" << name << "'" << std::endl;
    }
}

void ActorLifecycleManager::OptimizeBatch(std::vector<LifecycleContext*>& contexts) {
    if (!config_.enablePerformanceOptimization || contexts.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, optimizer] : optimizers_) {
        optimizer(contexts);
    }
}

void ActorLifecycleManager::RegisterActor(IActor* actor, ActorContext* context) {
    if (!actor) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto lifecycleContext = std::make_unique<LifecycleContext>();
    lifecycleContext->actor = actor;
    lifecycleContext->actorContext = context;
    lifecycleContext->actorName = actor->GetName();
    lifecycleContext->actorType = actor->GetName(); // Could be enhanced with type info
    lifecycleContext->state = ActorState::Created;
    lifecycleContext->stats.creationTime = std::chrono::high_resolution_clock::now();
    
    actorContexts_[actor] = std::move(lifecycleContext);
    
    // Execute post-creation hooks
    if (config_.enableHooks) {
        ExecuteHooks(LifecycleEvent::PostCreate, *actorContexts_[actor]);
    }
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Registered actor '" << actor->GetName() 
                  << "' (total actors: " << actorContexts_.size() << ")" << std::endl;
    }
}

void ActorLifecycleManager::UnregisterActor(IActor* actor) {
    if (!actor) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = actorContexts_.find(actor);
    if (it != actorContexts_.end()) {
        // Execute pre-destroy hooks
        if (config_.enableHooks && it->second->state != ActorState::Destroyed) {
            it->second->state = ActorState::Destroying;
            ExecuteHooks(LifecycleEvent::PreDestroy, *it->second);
        }
        
        // Update stats
        it->second->stats.destructionTime = std::chrono::high_resolution_clock::now();
        it->second->state = ActorState::Destroyed;
        
        // Execute post-destroy hooks
        if (config_.enableHooks) {
            ExecuteHooks(LifecycleEvent::PostDestroy, *it->second);
        }
        
        if (config_.enableDebugLogging) {
            std::cout << "[ActorLifecycle] Unregistered actor '" << actor->GetName() 
                      << "' (remaining actors: " << (actorContexts_.size() - 1) << ")" << std::endl;
        }
        
        actorContexts_.erase(it);
    }
}

LifecycleContext* ActorLifecycleManager::GetContext(IActor* actor) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = actorContexts_.find(actor);
    return (it != actorContexts_.end()) ? it->second.get() : nullptr;
}

bool ActorLifecycleManager::TransitionTo(IActor* actor, ActorState newState) {
    if (!actor) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = actorContexts_.find(actor);
    if (it == actorContexts_.end()) {
        return false;
    }
    
    auto& context = *it->second;
    ActorState oldState = context.state;
    
    // Validate transition
    if (!ValidateTransition(context, newState)) {
        return false;
    }
    
    // Execute pre-transition hooks
    if (config_.enableHooks) {
        LifecycleEvent preEvent = GetPreEvent(newState);
        if (preEvent != LifecycleEvent::PostDestroy) { // Invalid placeholder
            ExecuteHooks(preEvent, context);
        }
    }
    
    // Update state and stats
    context.state = newState;
    UpdateStats(context, newState);
    
    // Execute post-transition hooks
    if (config_.enableHooks) {
        LifecycleEvent postEvent = GetPostEvent(newState);
        if (postEvent != LifecycleEvent::PostDestroy) { // Invalid placeholder
            ExecuteHooks(postEvent, context);
        }
    }
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Actor '" << actor->GetName() 
                  << "' transitioned from " << utils::StateToString(oldState) 
                  << " to " << utils::StateToString(newState) << std::endl;
    }
    
    return true;
}

ActorState ActorLifecycleManager::GetState(const IActor* actor) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = actorContexts_.find(const_cast<IActor*>(actor));
    return (it != actorContexts_.end()) ? it->second->state : ActorState::Destroyed;
}

void ActorLifecycleManager::BatchTransition(const std::vector<IActor*>& actors, ActorState newState) {
    if (actors.empty()) return;
    
    std::vector<LifecycleContext*> contexts;
    contexts.reserve(actors.size());
    
    // Collect contexts
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (IActor* actor : actors) {
            auto it = actorContexts_.find(actor);
            if (it != actorContexts_.end()) {
                contexts.push_back(it->second.get());
            }
        }
    }
    
    // Optimize batch if needed
    if (config_.enablePerformanceOptimization) {
        OptimizeBatch(contexts);
    }
    
    // Perform transitions
    for (LifecycleContext* context : contexts) {
        if (context && ValidateTransition(*context, newState)) {
            TransitionTo(context->actor, newState);
        }
    }
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Batch transitioned " << contexts.size() 
                  << " actors to " << utils::StateToString(newState) << std::endl;
    }
}

void ActorLifecycleManager::BatchUpdate(double deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [actor, context] : actorContexts_) {
        if (context->state == ActorState::Active && actor) {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            actor->Update(deltaTime);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            double updateTime = duration.count() / 1000000.0;
            
            // Update statistics
            context->stats.updateCallCount++;
            context->stats.totalUpdateTime += updateTime;
            context->stats.averageUpdateTime = context->stats.totalUpdateTime / context->stats.updateCallCount;
        }
    }
}

std::vector<LifecycleStats> ActorLifecycleManager::GetAllStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LifecycleStats> stats;
    stats.reserve(actorContexts_.size());
    
    for (const auto& [actor, context] : actorContexts_) {
        stats.push_back(context->stats);
    }
    
    return stats;
}

LifecycleStats ActorLifecycleManager::GetStats(const IActor* actor) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = actorContexts_.find(const_cast<IActor*>(actor));
    if (it != actorContexts_.end()) {
        return it->second->stats;
    }
    return LifecycleStats{}; // Return default stats if not found
}

size_t ActorLifecycleManager::GetActorCountInState(ActorState state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(actorContexts_.begin(), actorContexts_.end(),
        [state](const auto& pair) {
            return pair.second->state == state;
        });
}

void ActorLifecycleManager::PrintDebugInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "\n=== Actor Lifecycle Manager Debug Info ===" << std::endl;
    std::cout << "Total actors: " << actorContexts_.size() << std::endl;
    
    // Count by state
    std::unordered_map<ActorState, size_t> stateCounts;
    for (const auto& [actor, context] : actorContexts_) {
        stateCounts[context->state]++;
    }
    
    std::cout << "Actor counts by state:" << std::endl;
    for (const auto& [state, count] : stateCounts) {
        std::cout << "  " << utils::StateToString(state) << ": " << count << std::endl;
    }
    
    std::cout << "Registered hooks: " << hooks_.size() << " events" << std::endl;
    std::cout << "Registered validators: " << validators_.size() << std::endl;
    std::cout << "Registered optimizers: " << optimizers_.size() << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Validation enabled: " << (config_.enableValidation ? "yes" : "no") << std::endl;
    std::cout << "  Hooks enabled: " << (config_.enableHooks ? "yes" : "no") << std::endl;
    std::cout << "  Performance optimization: " << (config_.enablePerformanceOptimization ? "yes" : "no") << std::endl;
    std::cout << "  Analytics enabled: " << (config_.enableAnalytics ? "yes" : "no") << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

void ActorLifecycleManager::PrintActorState(IActor* actor) const {
    if (!actor) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = actorContexts_.find(actor);
    if (it != actorContexts_.end()) {
        const auto& context = *it->second;
        std::cout << "Actor '" << context.actorName << "':" << std::endl;
        std::cout << "  State: " << utils::StateToString(context.state) << std::endl;
        std::cout << "  Type: " << context.actorType << std::endl;
        std::cout << "  Lifetime: " << std::fixed << std::setprecision(3) 
                  << context.stats.getLifetime() << "s" << std::endl;
        std::cout << "  Updates: " << context.stats.updateCallCount 
                  << " (avg: " << std::fixed << std::setprecision(6) 
                  << context.stats.averageUpdateTime << "s)" << std::endl;
    } else {
        std::cout << "Actor not found in lifecycle manager" << std::endl;
    }
}

std::string ActorLifecycleManager::GetStateReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "Actor Lifecycle State Report\n";
    oss << "===========================\n";
    oss << "Total actors: " << actorContexts_.size() << "\n\n";
    
    // Group by state
    std::unordered_map<ActorState, std::vector<std::string>> stateGroups;
    for (const auto& [actor, context] : actorContexts_) {
        stateGroups[context->state].push_back(context->actorName);
    }
    
    for (const auto& [state, actors] : stateGroups) {
        oss << utils::StateToString(state) << " (" << actors.size() << "):\n";
        for (const auto& name : actors) {
            oss << "  - " << name << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

void ActorLifecycleManager::DestroyAllActors() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (config_.enableDebugLogging) {
        std::cout << "[ActorLifecycle] Destroying all " << actorContexts_.size() << " actors" << std::endl;
    }
    
    for (auto& [actor, context] : actorContexts_) {
        if (context->state != ActorState::Destroyed) {
            context->state = ActorState::Destroying;
            if (config_.enableHooks) {
                ExecuteHooks(LifecycleEvent::PreDestroy, *context);
            }
            
            context->stats.destructionTime = std::chrono::high_resolution_clock::now();
            context->state = ActorState::Destroyed;
            
            if (config_.enableHooks) {
                ExecuteHooks(LifecycleEvent::PostDestroy, *context);
            }
        }
    }
    
    actorContexts_.clear();
}

void ActorLifecycleManager::GarbageCollect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = actorContexts_.begin();
    while (it != actorContexts_.end()) {
        if (it->second->state == ActorState::Destroyed) {
            if (config_.enableDebugLogging) {
                std::cout << "[ActorLifecycle] Garbage collecting destroyed actor '"
                          << it->second->actorName << "'" << std::endl;
            }
            it = actorContexts_.erase(it);
        } else {
            ++it;
        }
    }
}

// Private helper methods
void ActorLifecycleManager::ExecuteHooks(LifecycleEvent event, LifecycleContext& context) {
    auto eventIt = hooks_.find(event);
    if (eventIt != hooks_.end()) {
        for (const auto& [name, hook] : eventIt->second) {
            try {
                hook(context);
            } catch (const std::exception& e) {
                std::cerr << "[ActorLifecycle] Hook '" << name << "' threw exception: " 
                          << e.what() << std::endl;
            }
        }
    }
}

bool ActorLifecycleManager::IsValidTransition(ActorState current, ActorState target) const {
    // This is handled by LifecycleContext::CanTransitionTo
    // Could be moved here for centralized logic
    return true;
}

void ActorLifecycleManager::UpdateStats(LifecycleContext& context, ActorState newState) {
    auto now = std::chrono::high_resolution_clock::now();
    
    switch (newState) {
        case ActorState::Initialized:
            context.stats.initializationTime = now;
            break;
        case ActorState::Active:
            context.stats.activationTime = now;
            break;
        case ActorState::Destroyed:
            context.stats.destructionTime = now;
            break;
        default:
            break;
    }
}

LifecycleEvent ActorLifecycleManager::GetPreEvent(ActorState state) const {
    switch (state) {
        case ActorState::Initializing: return LifecycleEvent::PreInitialize;
        case ActorState::Active: return LifecycleEvent::PreActivate;
        case ActorState::Pausing: return LifecycleEvent::PrePause;
        case ActorState::Resuming: return LifecycleEvent::PreResume;
        case ActorState::Destroying: return LifecycleEvent::PreDestroy;
        default: return LifecycleEvent::PostDestroy; // Invalid placeholder
    }
}

LifecycleEvent ActorLifecycleManager::GetPostEvent(ActorState state) const {
    switch (state) {
        case ActorState::Created: return LifecycleEvent::PostCreate;
        case ActorState::Initialized: return LifecycleEvent::PostInitialize;
        case ActorState::Active: return LifecycleEvent::PostActivate;
        case ActorState::Paused: return LifecycleEvent::PostPause;
        case ActorState::Destroyed: return LifecycleEvent::PostDestroy;
        default: return LifecycleEvent::PostDestroy; // Invalid placeholder
    }
}

// Utility functions implementation
namespace utils {

std::string StateToString(ActorState state) {
    switch (state) {
        case ActorState::Created: return "Created";
        case ActorState::Initializing: return "Initializing";
        case ActorState::Initialized: return "Initialized";
        case ActorState::Active: return "Active";
        case ActorState::Pausing: return "Pausing";
        case ActorState::Paused: return "Paused";
        case ActorState::Resuming: return "Resuming";
        case ActorState::Destroying: return "Destroying";
        case ActorState::Destroyed: return "Destroyed";
        default: return "Unknown";
    }
}

std::string EventToString(LifecycleEvent event) {
    switch (event) {
        case LifecycleEvent::PreCreate: return "PreCreate";
        case LifecycleEvent::PostCreate: return "PostCreate";
        case LifecycleEvent::PreInitialize: return "PreInitialize";
        case LifecycleEvent::PostInitialize: return "PostInitialize";
        case LifecycleEvent::PreActivate: return "PreActivate";
        case LifecycleEvent::PostActivate: return "PostActivate";
        case LifecycleEvent::PrePause: return "PrePause";
        case LifecycleEvent::PostPause: return "PostPause";
        case LifecycleEvent::PreResume: return "PreResume";
        case LifecycleEvent::PostResume: return "PostResume";
        case LifecycleEvent::PreDestroy: return "PreDestroy";
        case LifecycleEvent::PostDestroy: return "PostDestroy";
        default: return "Unknown";
    }
}

ActorState StringToState(const std::string& str) {
    if (str == "Created") return ActorState::Created;
    if (str == "Initializing") return ActorState::Initializing;
    if (str == "Initialized") return ActorState::Initialized;
    if (str == "Active") return ActorState::Active;
    if (str == "Pausing") return ActorState::Pausing;
    if (str == "Paused") return ActorState::Paused;
    if (str == "Resuming") return ActorState::Resuming;
    if (str == "Destroying") return ActorState::Destroying;
    if (str == "Destroyed") return ActorState::Destroyed;
    return ActorState::Created; // Default
}

LifecycleEvent StringToEvent(const std::string& str) {
    if (str == "PreCreate") return LifecycleEvent::PreCreate;
    if (str == "PostCreate") return LifecycleEvent::PostCreate;
    if (str == "PreInitialize") return LifecycleEvent::PreInitialize;
    if (str == "PostInitialize") return LifecycleEvent::PostInitialize;
    if (str == "PreActivate") return LifecycleEvent::PreActivate;
    if (str == "PostActivate") return LifecycleEvent::PostActivate;
    if (str == "PrePause") return LifecycleEvent::PrePause;
    if (str == "PostPause") return LifecycleEvent::PostPause;
    if (str == "PreResume") return LifecycleEvent::PreResume;
    if (str == "PostResume") return LifecycleEvent::PostResume;
    if (str == "PreDestroy") return LifecycleEvent::PreDestroy;
    if (str == "PostDestroy") return LifecycleEvent::PostDestroy;
    return LifecycleEvent::PostCreate; // Default
}

double GetAverageLifetime(const std::vector<LifecycleStats>& stats) {
    if (stats.empty()) return 0.0;
    
    double total = 0.0;
    for (const auto& stat : stats) {
        total += stat.getLifetime();
    }
    
    return total / stats.size();
}

double GetAverageInitTime(const std::vector<LifecycleStats>& stats) {
    if (stats.empty()) return 0.0;
    
    double total = 0.0;
    size_t count = 0;
    
    for (const auto& stat : stats) {
        double initTime = stat.getInitializationDuration();
        if (initTime > 0.0) {
            total += initTime;
            count++;
        }
    }
    
    return count > 0 ? total / count : 0.0;
}

size_t GetActiveActorCount(const std::vector<LifecycleStats>& stats) {
    return std::count_if(stats.begin(), stats.end(),
        [](const LifecycleStats& stat) {
            return stat.isAlive();
        });
}

bool IsCreationState(ActorState state) {
    return state == ActorState::Created || state == ActorState::Initializing;
}

bool IsActiveState(ActorState state) {
    return state == ActorState::Active;
}

bool IsDestructionState(ActorState state) {
    return state == ActorState::Destroying || state == ActorState::Destroyed;
}

bool IsTransientState(ActorState state) {
    return state == ActorState::Initializing || 
           state == ActorState::Pausing || 
           state == ActorState::Resuming || 
           state == ActorState::Destroying;
}

} // namespace utils

} // namespace lifecycle