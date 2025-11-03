#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
class ResourceManager;
namespace ecs { class SystemSchedulerV2; }

// Framework lifecycle states
enum class FrameworkState {
    Unloaded,
    Loading,
    Loaded,
    Initializing,
    Running,
    Failed,
    Unloading
};

// Framework metadata and configuration
struct FrameworkConfig {
    std::string name;
    std::string version;
    std::string description;
    bool required = false;
    bool enabled = true;
    int priority = 0;  // Higher priority loads first
    std::vector<std::string> dependencies;  // Framework names this depends on
    std::unordered_map<std::string, std::string> settings;  // Framework-specific settings
};

// Framework performance metrics
struct FrameworkMetrics {
    double initializationTimeMs = 0.0;
    double shutdownTimeMs = 0.0;
    size_t memoryUsageBytes = 0;
    int failureCount = 0;
    bool isHealthy = true;
    std::string lastError;
};

// Framework interface - all frameworks must implement this
class IFramework {
public:
    virtual ~IFramework() = default;
    
    // Lifecycle methods
    virtual bool Initialize(const FrameworkConfig& config) = 0;
    virtual void Shutdown() = 0;
    virtual bool Validate() const = 0;
    
    // Information methods
    virtual std::string GetName() const = 0;
    virtual std::string GetVersion() const = 0;
    virtual FrameworkState GetState() const = 0;
    
    // Health and monitoring
    virtual bool IsHealthy() const { return true; }
    virtual FrameworkMetrics GetMetrics() const { return {}; }
    
    // Hot swapping support
    virtual bool SupportsHotSwap() const { return false; }
    virtual bool PrepareForSwap() { return false; }
    virtual bool CompleteSwap() { return false; }
};

// Framework factory for dynamic creation
using FrameworkFactory = std::function<std::shared_ptr<IFramework>()>;

// Framework validation result
struct ValidationResult {
    bool success = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    void AddError(const std::string& error) {
        success = false;
        errors.push_back(error);
    }
    
    void AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// Framework management system
class FrameworkManager {
public:
    static FrameworkManager& GetInstance();
    
    // Framework registration
    void RegisterFramework(const std::string& name, FrameworkFactory factory);
    void UnregisterFramework(const std::string& name);
    
    // Framework lifecycle management
    bool LoadFramework(const std::string& name, const FrameworkConfig& config);
    bool UnloadFramework(const std::string& name);
    bool ReloadFramework(const std::string& name);
    
    // Batch operations
    ValidationResult LoadFrameworks(const std::vector<FrameworkConfig>& configs);
    void UnloadAllFrameworks();
    
    // Framework queries
    bool IsFrameworkLoaded(const std::string& name) const;
    std::shared_ptr<IFramework> GetFramework(const std::string& name) const;
    std::vector<std::string> GetLoadedFrameworks() const;
    std::vector<std::string> GetAvailableFrameworks() const;
    
    // Dependency management
    ValidationResult ValidateDependencies(const std::string& name, const FrameworkConfig& config) const;
    std::vector<std::string> ResolveDependencyOrder(const std::vector<FrameworkConfig>& configs) const;
    
    // Validation
    ValidationResult ValidateFramework(const std::string& name) const;
    ValidationResult ValidateAllFrameworks() const;
    ValidationResult CheckCompatibility(const std::string& name, const FrameworkConfig& config) const;
    
    // Monitoring and profiling
    FrameworkMetrics GetFrameworkMetrics(const std::string& name) const;
    std::unordered_map<std::string, FrameworkMetrics> GetAllMetrics() const;
    bool IsFrameworkHealthy(const std::string& name) const;
    
    // Hot swapping
    bool SupportsHotSwap(const std::string& name) const;
    bool HotSwapFramework(const std::string& name, std::shared_ptr<IFramework> newInstance);
    
    // Fallback mechanisms
    void RegisterFallback(const std::string& name, FrameworkFactory fallbackFactory);
    bool HasFallback(const std::string& name) const;
    
    // Configuration
    void SetFrameworkSetting(const std::string& name, const std::string& key, const std::string& value);
    std::string GetFrameworkSetting(const std::string& name, const std::string& key) const;
    
    // Documentation generation
    std::string GenerateDocumentation() const;
    std::string GenerateFrameworkDoc(const std::string& name) const;
    
    // Testing support
    ValidationResult RunFrameworkTests(const std::string& name);
    std::unordered_map<std::string, ValidationResult> RunAllTests();
    
private:
    FrameworkManager() = default;
    FrameworkManager(const FrameworkManager&) = delete;
    FrameworkManager& operator=(const FrameworkManager&) = delete;
    
    struct FrameworkEntry {
        std::shared_ptr<IFramework> instance;
        FrameworkConfig config;
        FrameworkMetrics metrics;
        FrameworkFactory factory;
        FrameworkFactory fallbackFactory;
    };
    
    std::unordered_map<std::string, FrameworkEntry> frameworks_;
    std::unordered_map<std::string, FrameworkFactory> factories_;
    
    bool LoadFrameworkInternal(const std::string& name, const FrameworkConfig& config, FrameworkFactory factory);
    bool UnloadFrameworkInternal(const std::string& name, bool force = false);
    void UpdateMetrics(const std::string& name, const FrameworkMetrics& metrics);
    std::vector<std::string> GetDependents(const std::string& name) const;
};

// Built-in framework implementations
class GraphicsFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override;
    void Shutdown() override;
    bool Validate() const override;
    
    std::string GetName() const override { return "Graphics"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override;
    FrameworkMetrics GetMetrics() const override { return metrics_; }
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
};

class AudioFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override;
    void Shutdown() override;
    bool Validate() const override;
    
    std::string GetName() const override { return "Audio"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override;
    FrameworkMetrics GetMetrics() const override { return metrics_; }
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
};

class InputFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override;
    void Shutdown() override;
    bool Validate() const override;
    
    std::string GetName() const override { return "Input"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override;
    FrameworkMetrics GetMetrics() const override { return metrics_; }
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
};

class PhysicsFramework : public IFramework {
public:
    bool Initialize(const FrameworkConfig& config) override;
    void Shutdown() override;
    bool Validate() const override;
    
    std::string GetName() const override { return "Physics"; }
    std::string GetVersion() const override { return "1.0.0"; }
    FrameworkState GetState() const override { return state_; }
    
    bool IsHealthy() const override;
    FrameworkMetrics GetMetrics() const override { return metrics_; }
    
    bool SupportsHotSwap() const override { return true; }
    bool PrepareForSwap() override;
    bool CompleteSwap() override;
    
private:
    FrameworkState state_ = FrameworkState::Unloaded;
    FrameworkMetrics metrics_;
};
