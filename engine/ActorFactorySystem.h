#pragma once

#include "ecs/EntityManager.h"
#include "ActorContext.h"
#include "IActor.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>

/**
 * ActorFactorySystem: Advanced actor factory system with registration,
 * validation, caching, templating, analytics, and debugging.
 * 
 * Features:
 * - Automatic factory registration for new actor types
 * - Factory validation and dependency checking
 * - Performance optimization and caching
 * - Template system for actor variations
 * - Analytics tracking and monitoring
 * - Comprehensive debugging tools
 */
class ActorFactorySystem {
public:
    // Factory function signature
    using FactoryFunction = std::function<std::unique_ptr<IActor>()>;
    
    // Factory metadata for validation and analytics
    struct FactoryMetadata {
        std::string actorType;
        std::string category;
        std::vector<std::string> dependencies;
        size_t creationCount = 0;
        double totalCreationTime = 0.0;
        double avgCreationTime = 0.0;
        std::chrono::steady_clock::time_point lastUsed;
        bool isValid = true;
        std::string validationErrors;
    };
    
    // Factory template for actor variations
    struct ActorTemplate {
        std::string templateName;
        std::string baseType;
        std::unordered_map<std::string, std::string> parameters;
        std::chrono::steady_clock::time_point createdAt;
        size_t usageCount = 0;
    };
    
    // Factory creation result
    struct FactoryResult {
        std::unique_ptr<IActor> actor;
        bool success = false;
        std::string errorMessage;
        double creationTimeMs = 0.0;
    };
    
    // Performance metrics
    struct PerformanceMetrics {
        size_t totalCreations = 0;
        double totalTimeMs = 0.0;
        double avgTimeMs = 0.0;
        double minTimeMs = std::numeric_limits<double>::max();
        double maxTimeMs = 0.0;
        std::unordered_map<std::string, size_t> creationsByType;
    };

    /**
     * Get singleton instance
     */
    static ActorFactorySystem& GetInstance();

    /**
     * Register a factory function for an actor type
     */
    void RegisterFactory(const std::string& actorType, FactoryFunction factory, 
                        const std::string& category = "default",
                        const std::vector<std::string>& dependencies = {});

    /**
     * Validate factory configurations and dependencies
     */
    bool ValidateFactory(const std::string& actorType, std::string& errorMsg);
    bool ValidateAllFactories();

    /**
     * Create actor with automatic caching and optimization
     */
    FactoryResult CreateActor(const std::string& actorType, 
                             EntityManager& entityManager,
                             Entity entity);

    /**
     * Create actor from template
     */
    FactoryResult CreateFromTemplate(const std::string& templateName,
                                    EntityManager& entityManager,
                                    Entity entity);

    /**
     * Register actor template for variations
     */
    void RegisterTemplate(const std::string& templateName,
                         const std::string& baseType,
                         const std::unordered_map<std::string, std::string>& parameters);

    /**
     * Cache management
     */
    void EnableCaching(bool enable);
    void ClearCache();
    size_t GetCacheSize() const;

    /**
     * Analytics and monitoring
     */
    const FactoryMetadata& GetFactoryMetadata(const std::string& actorType) const;
    PerformanceMetrics GetPerformanceMetrics() const;
    std::vector<std::string> GetMostUsedActorTypes(size_t count = 10) const;
    
    /**
     * Debugging tools
     */
    void EnableDebugMode(bool enable);
    void LogFactoryState(const std::string& actorType) const;
    void LogAllFactories() const;
    std::string GetFactoryHealthReport() const;
    
    /**
     * Auto-generate factory documentation
     */
    std::string GenerateDocumentation() const;
    void ExportDocumentation(const std::string& filepath) const;

    /**
     * Factory testing
     */
    bool TestFactory(const std::string& actorType, std::string& result);
    std::vector<std::string> TestAllFactories();

    /**
     * Query available factories
     */
    std::vector<std::string> GetRegisteredTypes() const;
    std::vector<std::string> GetFactoriesByCategory(const std::string& category) const;
    bool HasFactory(const std::string& actorType) const;

private:
    ActorFactorySystem() = default;
    ~ActorFactorySystem() = default;
    ActorFactorySystem(const ActorFactorySystem&) = delete;
    ActorFactorySystem& operator=(const ActorFactorySystem&) = delete;

    // Factory registry
    std::unordered_map<std::string, FactoryFunction> factories_;
    std::unordered_map<std::string, FactoryMetadata> metadata_;
    std::unordered_map<std::string, ActorTemplate> templates_;
    
    // Performance and caching
    bool cachingEnabled_ = true;
    bool debugMode_ = false;
    PerformanceMetrics metrics_;
    mutable std::mutex mutex_;
    
    // Helper methods
    void UpdateMetrics(const std::string& actorType, double creationTimeMs);
    bool CheckDependencies(const std::string& actorType, std::string& errorMsg);
    void LogCreation(const std::string& actorType, bool success, double timeMs);
};

/**
 * Macro for automatic factory registration
 * Usage: REGISTER_ACTOR_FACTORY(ActorClass, "category", {"dependency1", "dependency2"})
 */
#define REGISTER_ACTOR_FACTORY(ActorClass, Category, Dependencies) \
    namespace { \
        struct ActorClass##FactoryRegistrar { \
            ActorClass##FactoryRegistrar() { \
                ActorFactorySystem::GetInstance().RegisterFactory( \
                    #ActorClass, \
                    []() -> std::unique_ptr<IActor> { \
                        return std::make_unique<ActorClass>(); \
                    }, \
                    Category, \
                    Dependencies \
                ); \
            } \
        }; \
        static ActorClass##FactoryRegistrar actorClass##_registrar; \
    }

/**
 * Simplified macro for basic registration
 */
#define REGISTER_ACTOR(ActorClass) \
    REGISTER_ACTOR_FACTORY(ActorClass, "default", {})
