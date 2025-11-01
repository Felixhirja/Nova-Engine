#pragma once
#include "EntityManager.h"
#include <vector>
#include <functional>

namespace ecs {

// Fluent query builder for entity queries
template<typename... Components>
class Query {
public:
    explicit Query(EntityManagerV2& manager) : manager_(manager) {}
    
    template<typename Func>
    void ForEach(Func&& func) {
        manager_.ForEach<Components...>(std::forward<Func>(func));
    }
    
    size_t Count() const {
        size_t count = 0;
        manager_.ForEach<Components...>([&count](EntityHandle, Components&...) {
            count++;
        });
        return count;
    }
    
    std::vector<EntityHandle> ToVector() const {
        std::vector<EntityHandle> result;
        manager_.ForEach<Components...>([&result](EntityHandle handle, Components&...) {
            result.push_back(handle);
        });
        return result;
    }
    
    template<typename Predicate>
    Query<Components...>& Where(Predicate&& pred) {
        predicates_.push_back(std::forward<Predicate>(pred));
        return *this;
    }
    
private:
    EntityManagerV2& manager_;
    std::vector<std::function<bool(EntityHandle, Components&...)>> predicates_;
};

// Query builder entry point
class QueryBuilder {
public:
    explicit QueryBuilder(EntityManagerV2& manager) : manager_(manager) {}
    
    template<typename... Components>
    Query<Components...> With() {
        return Query<Components...>(manager_);
    }
    
private:
    EntityManagerV2& manager_;
};

} // namespace ecs
