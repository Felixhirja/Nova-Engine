#include "FrameworkManager.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <queue>

// ============================================================================
// FrameworkManager Implementation
// ============================================================================

FrameworkManager& FrameworkManager::GetInstance() {
    static FrameworkManager instance;
    return instance;
}

void FrameworkManager::RegisterFramework(const std::string& name, FrameworkFactory factory) {
    factories_[name] = factory;
    std::cout << "[FrameworkManager] Registered framework: " << name << std::endl;
}

void FrameworkManager::UnregisterFramework(const std::string& name) {
    factories_.erase(name);
    std::cout << "[FrameworkManager] Unregistered framework: " << name << std::endl;
}

bool FrameworkManager::LoadFramework(const std::string& name, const FrameworkConfig& config) {
    auto factoryIt = factories_.find(name);
    if (factoryIt == factories_.end()) {
        std::cerr << "[FrameworkManager] Framework not registered: " << name << std::endl;
        return false;
    }
    
    return LoadFrameworkInternal(name, config, factoryIt->second);
}

bool FrameworkManager::LoadFrameworkInternal(const std::string& name, const FrameworkConfig& config, FrameworkFactory factory) {
    // Check if already loaded
    if (IsFrameworkLoaded(name)) {
        std::cerr << "[FrameworkManager] Framework already loaded: " << name << std::endl;
        return false;
    }
    
    // Validate dependencies
    auto depResult = ValidateDependencies(name, config);
    if (!depResult.success) {
        std::cerr << "[FrameworkManager] Dependency validation failed for: " << name << std::endl;
        for (const auto& error : depResult.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return false;
    }
    
    // Create framework instance
    std::shared_ptr<IFramework> instance;
    try {
        instance = factory();
        if (!instance) {
            std::cerr << "[FrameworkManager] Failed to create framework: " << name << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[FrameworkManager] Exception creating framework " << name << ": " << e.what() << std::endl;
        return false;
    }
    
    // Initialize framework
    auto startTime = std::chrono::high_resolution_clock::now();
    bool initSuccess = false;
    
    try {
        initSuccess = instance->Initialize(config);
    } catch (const std::exception& e) {
        std::cerr << "[FrameworkManager] Exception initializing framework " << name << ": " << e.what() << std::endl;
        initSuccess = false;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double initTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (!initSuccess) {
        // Try fallback if available
        auto entryIt = frameworks_.find(name);
        if (entryIt != frameworks_.end() && entryIt->second.fallbackFactory) {
            std::cout << "[FrameworkManager] Trying fallback for: " << name << std::endl;
            instance = entryIt->second.fallbackFactory();
            if (instance && instance->Initialize(config)) {
                std::cout << "[FrameworkManager] Fallback successful for: " << name << std::endl;
                initSuccess = true;
            }
        }
        
        if (!initSuccess) {
            std::cerr << "[FrameworkManager] Failed to initialize framework: " << name << std::endl;
            return false;
        }
    }
    
    // Store framework
    FrameworkEntry entry;
    entry.instance = instance;
    entry.config = config;
    entry.factory = factory;
    entry.metrics.initializationTimeMs = initTimeMs;
    
    frameworks_[name] = entry;
    
    std::cout << "[FrameworkManager] Loaded framework: " << name << " (" << initTimeMs << "ms)" << std::endl;
    return true;
}

bool FrameworkManager::UnloadFramework(const std::string& name) {
    return UnloadFrameworkInternal(name, false);
}

bool FrameworkManager::UnloadFrameworkInternal(const std::string& name, bool force) {
    auto it = frameworks_.find(name);
    if (it == frameworks_.end()) {
        std::cerr << "[FrameworkManager] Framework not loaded: " << name << std::endl;
        return false;
    }
    
    // Check for dependents
    auto dependents = GetDependents(name);
    if (!dependents.empty() && !force) {
        std::cerr << "[FrameworkManager] Cannot unload " << name << " - has dependents:" << std::endl;
        for (const auto& dep : dependents) {
            std::cerr << "  - " << dep << std::endl;
        }
        return false;
    }
    
    // Shutdown framework
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        if (it->second.instance) {
            it->second.instance->Shutdown();
        }
    } catch (const std::exception& e) {
        std::cerr << "[FrameworkManager] Exception shutting down framework " << name << ": " << e.what() << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double shutdownTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    frameworks_.erase(it);
    
    std::cout << "[FrameworkManager] Unloaded framework: " << name << " (" << shutdownTimeMs << "ms)" << std::endl;
    return true;
}

bool FrameworkManager::ReloadFramework(const std::string& name) {
    auto it = frameworks_.find(name);
    if (it == frameworks_.end()) {
        std::cerr << "[FrameworkManager] Cannot reload - framework not loaded: " << name << std::endl;
        return false;
    }
    
    FrameworkConfig config = it->second.config;
    FrameworkFactory factory = it->second.factory;
    
    if (!UnloadFramework(name)) {
        return false;
    }
    
    return LoadFrameworkInternal(name, config, factory);
}

ValidationResult FrameworkManager::LoadFrameworks(const std::vector<FrameworkConfig>& configs) {
    ValidationResult result;
    
    // Resolve dependency order
    auto orderedConfigs = configs;
    try {
        auto order = ResolveDependencyOrder(configs);
        orderedConfigs.clear();
        for (const auto& name : order) {
            auto it = std::find_if(configs.begin(), configs.end(),
                [&name](const FrameworkConfig& cfg) { return cfg.name == name; });
            if (it != configs.end()) {
                orderedConfigs.push_back(*it);
            }
        }
    } catch (const std::exception& e) {
        result.AddError(std::string("Dependency resolution failed: ") + e.what());
        return result;
    }
    
    // Load frameworks in dependency order
    for (const auto& config : orderedConfigs) {
        if (!config.enabled) {
            continue;
        }
        
        if (!LoadFramework(config.name, config)) {
            std::string error = "Failed to load framework: " + config.name;
            if (config.required) {
                result.AddError(error);
            } else {
                result.AddWarning(error);
            }
        }
    }
    
    return result;
}

void FrameworkManager::UnloadAllFrameworks() {
    std::cout << "[FrameworkManager] Unloading all frameworks..." << std::endl;
    
    // Unload in reverse dependency order
    std::vector<std::string> names = GetLoadedFrameworks();
    std::reverse(names.begin(), names.end());
    
    for (const auto& name : names) {
        UnloadFrameworkInternal(name, true);  // Force unload
    }
}

bool FrameworkManager::IsFrameworkLoaded(const std::string& name) const {
    return frameworks_.find(name) != frameworks_.end();
}

std::shared_ptr<IFramework> FrameworkManager::GetFramework(const std::string& name) const {
    auto it = frameworks_.find(name);
    return it != frameworks_.end() ? it->second.instance : nullptr;
}

std::vector<std::string> FrameworkManager::GetLoadedFrameworks() const {
    std::vector<std::string> names;
    for (const auto& pair : frameworks_) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<std::string> FrameworkManager::GetAvailableFrameworks() const {
    std::vector<std::string> names;
    for (const auto& pair : factories_) {
        names.push_back(pair.first);
    }
    return names;
}

ValidationResult FrameworkManager::ValidateDependencies(const std::string& name, const FrameworkConfig& config) const {
    ValidationResult result;
    
    for (const auto& dep : config.dependencies) {
        if (!IsFrameworkLoaded(dep)) {
            result.AddError("Missing dependency: " + dep + " (required by " + name + ")");
        }
    }
    
    return result;
}

std::vector<std::string> FrameworkManager::ResolveDependencyOrder(const std::vector<FrameworkConfig>& configs) const {
    std::unordered_map<std::string, std::vector<std::string>> adjList;
    std::unordered_map<std::string, int> inDegree;
    
    // Build adjacency list and in-degree map
    for (const auto& config : configs) {
        inDegree[config.name] = 0;
        adjList[config.name] = {};
    }
    
    for (const auto& config : configs) {
        for (const auto& dep : config.dependencies) {
            adjList[dep].push_back(config.name);
            inDegree[config.name]++;
        }
    }
    
    // Topological sort using Kahn's algorithm
    std::queue<std::string> queue;
    for (const auto& pair : inDegree) {
        if (pair.second == 0) {
            queue.push(pair.first);
        }
    }
    
    std::vector<std::string> order;
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        order.push_back(current);
        
        for (const auto& neighbor : adjList[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }
    
    // Check for cycles
    if (order.size() != configs.size()) {
        throw std::runtime_error("Circular dependency detected");
    }
    
    return order;
}

ValidationResult FrameworkManager::ValidateFramework(const std::string& name) const {
    ValidationResult result;
    
    auto it = frameworks_.find(name);
    if (it == frameworks_.end()) {
        result.AddError("Framework not loaded: " + name);
        return result;
    }
    
    if (!it->second.instance->Validate()) {
        result.AddError("Framework validation failed: " + name);
    }
    
    return result;
}

ValidationResult FrameworkManager::ValidateAllFrameworks() const {
    ValidationResult result;
    
    for (const auto& pair : frameworks_) {
        auto frameworkResult = ValidateFramework(pair.first);
        result.errors.insert(result.errors.end(), 
            frameworkResult.errors.begin(), frameworkResult.errors.end());
        result.warnings.insert(result.warnings.end(),
            frameworkResult.warnings.begin(), frameworkResult.warnings.end());
        result.success = result.success && frameworkResult.success;
    }
    
    return result;
}

ValidationResult FrameworkManager::CheckCompatibility(const std::string& name, const FrameworkConfig& config) const {
    ValidationResult result;
    
    // Check if framework is registered
    if (factories_.find(name) == factories_.end()) {
        result.AddError("Framework not registered: " + name);
        return result;
    }
    
    // Check dependencies
    for (const auto& dep : config.dependencies) {
        if (factories_.find(dep) == factories_.end()) {
            result.AddError("Unknown dependency: " + dep);
        }
    }
    
    return result;
}

FrameworkMetrics FrameworkManager::GetFrameworkMetrics(const std::string& name) const {
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        return it->second.instance->GetMetrics();
    }
    return {};
}

std::unordered_map<std::string, FrameworkMetrics> FrameworkManager::GetAllMetrics() const {
    std::unordered_map<std::string, FrameworkMetrics> metrics;
    for (const auto& pair : frameworks_) {
        metrics[pair.first] = pair.second.instance->GetMetrics();
    }
    return metrics;
}

bool FrameworkManager::IsFrameworkHealthy(const std::string& name) const {
    auto it = frameworks_.find(name);
    return it != frameworks_.end() && it->second.instance->IsHealthy();
}

bool FrameworkManager::SupportsHotSwap(const std::string& name) const {
    auto it = frameworks_.find(name);
    return it != frameworks_.end() && it->second.instance->SupportsHotSwap();
}

bool FrameworkManager::HotSwapFramework(const std::string& name, std::shared_ptr<IFramework> newInstance) {
    auto it = frameworks_.find(name);
    if (it == frameworks_.end()) {
        std::cerr << "[FrameworkManager] Cannot hot swap - framework not loaded: " << name << std::endl;
        return false;
    }
    
    if (!it->second.instance->SupportsHotSwap() || !newInstance->SupportsHotSwap()) {
        std::cerr << "[FrameworkManager] Framework does not support hot swapping: " << name << std::endl;
        return false;
    }
    
    // Prepare old instance for swap
    if (!it->second.instance->PrepareForSwap()) {
        std::cerr << "[FrameworkManager] Failed to prepare for swap: " << name << std::endl;
        return false;
    }
    
    // Initialize new instance
    if (!newInstance->Initialize(it->second.config)) {
        std::cerr << "[FrameworkManager] Failed to initialize new instance: " << name << std::endl;
        return false;
    }
    
    // Complete swap
    if (!newInstance->CompleteSwap()) {
        std::cerr << "[FrameworkManager] Failed to complete swap: " << name << std::endl;
        return false;
    }
    
    // Replace instance
    it->second.instance = newInstance;
    
    std::cout << "[FrameworkManager] Hot swapped framework: " << name << std::endl;
    return true;
}

void FrameworkManager::RegisterFallback(const std::string& name, FrameworkFactory fallbackFactory) {
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        it->second.fallbackFactory = fallbackFactory;
    }
    std::cout << "[FrameworkManager] Registered fallback for: " << name << std::endl;
}

bool FrameworkManager::HasFallback(const std::string& name) const {
    auto it = frameworks_.find(name);
    return it != frameworks_.end() && it->second.fallbackFactory != nullptr;
}

void FrameworkManager::SetFrameworkSetting(const std::string& name, const std::string& key, const std::string& value) {
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        it->second.config.settings[key] = value;
    }
}

std::string FrameworkManager::GetFrameworkSetting(const std::string& name, const std::string& key) const {
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        auto settingIt = it->second.config.settings.find(key);
        if (settingIt != it->second.config.settings.end()) {
            return settingIt->second;
        }
    }
    return "";
}

std::string FrameworkManager::GenerateDocumentation() const {
    std::ostringstream doc;
    doc << "# Framework Manager Documentation\n\n";
    doc << "## Available Frameworks\n\n";
    
    for (const auto& name : GetAvailableFrameworks()) {
        doc << "### " << name << "\n";
        doc << GenerateFrameworkDoc(name) << "\n\n";
    }
    
    return doc.str();
}

std::string FrameworkManager::GenerateFrameworkDoc(const std::string& name) const {
    std::ostringstream doc;
    
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        const auto& config = it->second.config;
        const auto& metrics = it->second.metrics;
        
        doc << "**Version:** " << it->second.instance->GetVersion() << "\n";
        doc << "**Description:** " << config.description << "\n";
        doc << "**Required:** " << (config.required ? "Yes" : "No") << "\n";
        doc << "**Priority:** " << config.priority << "\n";
        
        if (!config.dependencies.empty()) {
            doc << "**Dependencies:** ";
            for (size_t i = 0; i < config.dependencies.size(); ++i) {
                if (i > 0) doc << ", ";
                doc << config.dependencies[i];
            }
            doc << "\n";
        }
        
        doc << "**Initialization Time:** " << metrics.initializationTimeMs << "ms\n";
        doc << "**Health:** " << (metrics.isHealthy ? "Healthy" : "Unhealthy") << "\n";
    } else {
        doc << "*Framework not loaded*\n";
    }
    
    return doc.str();
}

ValidationResult FrameworkManager::RunFrameworkTests(const std::string& name) {
    ValidationResult result;
    
    auto it = frameworks_.find(name);
    if (it == frameworks_.end()) {
        result.AddError("Framework not loaded: " + name);
        return result;
    }
    
    // Basic validation test
    if (!it->second.instance->Validate()) {
        result.AddError("Framework validation test failed");
    }
    
    // Health check test
    if (!it->second.instance->IsHealthy()) {
        result.AddWarning("Framework health check indicates issues");
    }
    
    return result;
}

std::unordered_map<std::string, ValidationResult> FrameworkManager::RunAllTests() {
    std::unordered_map<std::string, ValidationResult> results;
    
    for (const auto& pair : frameworks_) {
        results[pair.first] = RunFrameworkTests(pair.first);
    }
    
    return results;
}

void FrameworkManager::UpdateMetrics(const std::string& name, const FrameworkMetrics& metrics) {
    auto it = frameworks_.find(name);
    if (it != frameworks_.end()) {
        it->second.metrics = metrics;
    }
}

std::vector<std::string> FrameworkManager::GetDependents(const std::string& name) const {
    std::vector<std::string> dependents;
    
    for (const auto& pair : frameworks_) {
        const auto& deps = pair.second.config.dependencies;
        if (std::find(deps.begin(), deps.end(), name) != deps.end()) {
            dependents.push_back(pair.first);
        }
    }
    
    return dependents;
}

// ============================================================================
// Built-in Framework Implementations
// ============================================================================

bool GraphicsFramework::Initialize(const FrameworkConfig& config) {
    std::cout << "[GraphicsFramework] Initializing..." << std::endl;
    state_ = FrameworkState::Initializing;
    
    // Simulate initialization
    state_ = FrameworkState::Running;
    metrics_.isHealthy = true;
    
    std::cout << "[GraphicsFramework] Initialized successfully" << std::endl;
    return true;
}

void GraphicsFramework::Shutdown() {
    std::cout << "[GraphicsFramework] Shutting down..." << std::endl;
    state_ = FrameworkState::Unloading;
    state_ = FrameworkState::Unloaded;
}

bool GraphicsFramework::Validate() const {
    return state_ == FrameworkState::Running;
}

bool GraphicsFramework::IsHealthy() const {
    return metrics_.isHealthy && state_ == FrameworkState::Running;
}

bool AudioFramework::Initialize(const FrameworkConfig& config) {
    std::cout << "[AudioFramework] Initializing..." << std::endl;
    state_ = FrameworkState::Initializing;
    
    state_ = FrameworkState::Running;
    metrics_.isHealthy = true;
    
    std::cout << "[AudioFramework] Initialized successfully" << std::endl;
    return true;
}

void AudioFramework::Shutdown() {
    std::cout << "[AudioFramework] Shutting down..." << std::endl;
    state_ = FrameworkState::Unloading;
    state_ = FrameworkState::Unloaded;
}

bool AudioFramework::Validate() const {
    return state_ == FrameworkState::Running;
}

bool AudioFramework::IsHealthy() const {
    return metrics_.isHealthy && state_ == FrameworkState::Running;
}

bool InputFramework::Initialize(const FrameworkConfig& config) {
    std::cout << "[InputFramework] Initializing..." << std::endl;
    state_ = FrameworkState::Initializing;
    
    state_ = FrameworkState::Running;
    metrics_.isHealthy = true;
    
    std::cout << "[InputFramework] Initialized successfully" << std::endl;
    return true;
}

void InputFramework::Shutdown() {
    std::cout << "[InputFramework] Shutting down..." << std::endl;
    state_ = FrameworkState::Unloading;
    state_ = FrameworkState::Unloaded;
}

bool InputFramework::Validate() const {
    return state_ == FrameworkState::Running;
}

bool InputFramework::IsHealthy() const {
    return metrics_.isHealthy && state_ == FrameworkState::Running;
}

bool PhysicsFramework::Initialize(const FrameworkConfig& config) {
    std::cout << "[PhysicsFramework] Initializing..." << std::endl;
    state_ = FrameworkState::Initializing;
    
    state_ = FrameworkState::Running;
    metrics_.isHealthy = true;
    
    std::cout << "[PhysicsFramework] Initialized successfully" << std::endl;
    return true;
}

void PhysicsFramework::Shutdown() {
    std::cout << "[PhysicsFramework] Shutting down..." << std::endl;
    state_ = FrameworkState::Unloading;
    state_ = FrameworkState::Unloaded;
}

bool PhysicsFramework::Validate() const {
    return state_ == FrameworkState::Running;
}

bool PhysicsFramework::IsHealthy() const {
    return metrics_.isHealthy && state_ == FrameworkState::Running;
}

bool PhysicsFramework::PrepareForSwap() {
    std::cout << "[PhysicsFramework] Preparing for hot swap..." << std::endl;
    // Save state, pause simulation, etc.
    return true;
}

bool PhysicsFramework::CompleteSwap() {
    std::cout << "[PhysicsFramework] Completing hot swap..." << std::endl;
    // Restore state, resume simulation, etc.
    return true;
}
