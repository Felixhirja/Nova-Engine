#pragma once

#include "QueryBuilder.h"
#include "System.h"
#include "EntityManager.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeindex>

namespace ecs {

// Forward declarations
class QueryBasedSystem;
class QueryRegistry;

/**
 * Query-based System Integration
 * Integrates QueryBuilder with System base class for automatic query management
 */

// Query handle for efficient query reuse and caching
struct QueryHandle {
    size_t queryId;
    std::type_index resultType;
    mutable bool isDirty;
    
    QueryHandle(size_t id, std::type_index type) 
        : queryId(id), resultType(type), isDirty(true) {}
    
    void MarkDirty() const { isDirty = true; }
    void MarkClean() const { isDirty = false; }
};

// Query change notification callback
using QueryChangeCallback = std::function<void(const std::vector<EntityHandle>&)>;

// Query-based system base class
class QueryBasedSystem : public System {
public:
    explicit QueryBasedSystem(SystemType type) : System(type) {}
    virtual ~QueryBasedSystem() = default;
    
    // Override Update to use query-based approach
    void Update(EntityManager& entityManager, double dt) override {
        // Refresh queries if needed
        RefreshQueries(entityManager);
        
        // Call derived class update
        UpdateWithQueries(entityManager, dt);
    }
    
    // Virtual method for derived systems to implement
    virtual void UpdateWithQueries(EntityManager& entityManager, double dt) = 0;
    
    // Register a query with automatic tracking
    template<typename... Components>
    QueryHandle RegisterQuery(const std::string& queryName) {
        size_t queryId = queries_.size();
        
        // Store query info (query will be executed on demand)
        QueryInfo info;
        info.name = queryName;
        info.componentTypes = { std::type_index(typeid(Components))... };
        
        queries_.push_back(std::move(info));
        
        return QueryHandle(queryId, std::type_index(typeid(std::tuple<Components...>)));
    }
    
    // Execute a registered query
    template<typename... Components>
    std::vector<EntityHandle> ExecuteQuery(const QueryHandle& handle, EntityManagerV2& manager) {
        if (handle.queryId >= queries_.size()) {
            throw std::runtime_error("Invalid query handle");
        }
        
        auto& info = queries_[handle.queryId];
        
        // Check cache validity
        if (!handle.isDirty && !info.cachedResults.empty()) {
            return info.cachedResults;
        }
        
        // Execute query by collecting entities with specified components
        std::vector<EntityHandle> results;
        manager.ForEach<Components...>([&results](EntityHandle handle, Components&...) {
            results.push_back(handle);
        });
        
        // Update cache
        info.cachedResults = results;
        handle.MarkClean();
        
        return results;
    }
    
    // Subscribe to query change notifications
    void SubscribeToQueryChanges(const QueryHandle& handle, QueryChangeCallback callback) {
        if (handle.queryId >= queries_.size()) return;
        
        auto& info = queries_[handle.queryId];
        info.changeCallbacks.push_back(std::move(callback));
    }
    
    // Invalidate all queries (called when entities are added/removed)
    void InvalidateAllQueries() {
        for (auto& query : queries_) {
            query.isDirty = true;
        }
    }
    
    // Get query statistics
    struct QueryStats {
        std::string name;
        size_t resultCount;
        bool isCached;
        std::chrono::microseconds lastExecutionTime;
    };
    
    std::vector<QueryStats> GetQueryStatistics() const {
        std::vector<QueryStats> stats;
        
        for (const auto& query : queries_) {
            QueryStats s;
            s.name = query.name;
            s.resultCount = query.cachedResults.size();
            s.isCached = !query.isDirty;
            s.lastExecutionTime = query.lastExecutionTime;
            stats.push_back(s);
        }
        
        return stats;
    }
    
protected:
    // Refresh queries if needed
    void RefreshQueries(EntityManager& entityManager) {
        // Check if any entities were modified
        // This would integrate with change tracking system
        
        // For now, mark all queries as dirty if needed
        // Real implementation would use version numbers
    }
    
private:
    struct QueryInfo {
        std::string name;
        std::vector<std::type_index> componentTypes;
        std::vector<EntityHandle> cachedResults;
        std::vector<QueryChangeCallback> changeCallbacks;
        bool isDirty = true;
        std::chrono::microseconds lastExecutionTime{0};
    };
    
    std::vector<QueryInfo> queries_;
};

// Global query registry for cross-system query sharing
class QueryRegistry {
public:
    static QueryRegistry& GetInstance() {
        static QueryRegistry instance;
        return instance;
    }
    
    // Register a named query
    template<typename... Components>
    void RegisterNamedQuery(const std::string& name, EntityManagerV2& manager) {
        QueryMetadata meta;
        meta.componentTypes = { std::type_index(typeid(Components))... };
        namedQueries_[name] = meta;
    }
    
    // Execute a named query
    template<typename... Components>
    std::vector<EntityHandle> ExecuteNamedQuery(const std::string& name, EntityManagerV2& manager) {
        auto it = namedQueries_.find(name);
        if (it == namedQueries_.end()) {
            throw std::runtime_error("Named query not found: " + name);
        }
        
        // Execute query by collecting entities
        std::vector<EntityHandle> results;
        manager.ForEach<Components...>([&results](EntityHandle handle, Components&...) {
            results.push_back(handle);
        });
        
        return results;
    }
    
    // Check if query exists
    bool HasQuery(const std::string& name) const {
        return namedQueries_.find(name) != namedQueries_.end();
    }
    
private:
    QueryRegistry() = default;
    
    struct QueryMetadata {
        std::vector<std::type_index> componentTypes;
    };
    
    std::unordered_map<std::string, QueryMetadata> namedQueries_;
};

// Automatic query registration helper
template<typename... Components>
class AutoQueryRegistration {
public:
    AutoQueryRegistration(const std::string& name, EntityManagerV2& manager) {
        QueryRegistry::GetInstance().RegisterNamedQuery<Components...>(name, manager);
    }
};

// Macro for easy query registration
#define REGISTER_QUERY(name, manager, ...) \
    static ecs::AutoQueryRegistration<__VA_ARGS__> name##_registration(#name, manager)

} // namespace ecs
