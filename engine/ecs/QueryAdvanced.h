#pragma once

#include "QueryBuilder.h"
#include "EntityManager.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>

namespace ecs {

/**
 * Advanced Query Features
 * Streaming iterators, pagination, composition, and logical operators
 */

// Streaming iterator for memory-efficient large result sets
template<typename ManagerType, typename... Components>
class QueryStreamIterator {
public:
    using value_type = std::tuple<EntityHandle, Components&...>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;
    using iterator_category = std::forward_iterator_tag;
    
    QueryStreamIterator(ManagerType& manager, const std::vector<EntityHandle>& entities, size_t index)
        : manager_(&manager), entities_(&entities), currentIndex_(index) {}
    
    QueryStreamIterator() : manager_(nullptr), entities_(nullptr), currentIndex_(0) {}
    
    // Dereference operator
    reference operator*() const {
        EntityHandle entity = (*entities_)[currentIndex_];
        return std::tuple_cat(
            std::make_tuple(entity),
            std::forward_as_tuple(*manager_->template GetComponent<Components>(entity)...)
        );
    }
    
    // Pre-increment
    QueryStreamIterator& operator++() {
        ++currentIndex_;
        return *this;
    }
    
    // Post-increment
    QueryStreamIterator operator++(int) {
        QueryStreamIterator temp = *this;
        ++(*this);
        return temp;
    }
    
    // Equality comparison
    bool operator==(const QueryStreamIterator& other) const {
        return currentIndex_ == other.currentIndex_ && entities_ == other.entities_;
    }
    
    bool operator!=(const QueryStreamIterator& other) const {
        return !(*this == other);
    }
    
private:
    ManagerType* manager_;
    const std::vector<EntityHandle>* entities_;
    size_t currentIndex_;
};

// Streaming query result for lazy evaluation
template<typename ManagerType, typename... Components>
class StreamingQueryResult {
public:
    StreamingQueryResult(ManagerType& manager, std::vector<EntityHandle> entities)
        : manager_(&manager), entities_(std::move(entities)) {}
    
    using Iterator = QueryStreamIterator<ManagerType, Components...>;
    
    Iterator begin() {
        return Iterator(*manager_, entities_, 0);
    }
    
    Iterator end() {
        return Iterator(*manager_, entities_, entities_.size());
    }
    
    size_t size() const { return entities_.size(); }
    bool empty() const { return entities_.empty(); }
    
private:
    ManagerType* manager_;
    std::vector<EntityHandle> entities_;
};

// Paginated query result for large datasets
template<typename ManagerType>
class PaginatedQueryResult {
public:
    PaginatedQueryResult(std::vector<EntityHandle> allResults, size_t pageSize)
        : allResults_(std::move(allResults))
        , pageSize_(pageSize)
        , currentPage_(0) {}
    
    // Get current page results
    std::vector<EntityHandle> GetCurrentPage() const {
        size_t start = currentPage_ * pageSize_;
        size_t end = std::min(start + pageSize_, allResults_.size());
        
        if (start >= allResults_.size()) {
            return {};
        }
        
        return std::vector<EntityHandle>(
            allResults_.begin() + start,
            allResults_.begin() + end
        );
    }
    
    // Navigate pages
    bool NextPage() {
        if (HasNextPage()) {
            ++currentPage_;
            return true;
        }
        return false;
    }
    
    bool PrevPage() {
        if (currentPage_ > 0) {
            --currentPage_;
            return true;
        }
        return false;
    }
    
    void SetPage(size_t pageNumber) {
        currentPage_ = std::min(pageNumber, GetTotalPages() - 1);
    }
    
    // Page information
    size_t GetCurrentPageNumber() const { return currentPage_; }
    size_t GetTotalPages() const {
        return (allResults_.size() + pageSize_ - 1) / pageSize_;
    }
    
    bool HasNextPage() const {
        return (currentPage_ + 1) * pageSize_ < allResults_.size();
    }
    
    bool HasPrevPage() const {
        return currentPage_ > 0;
    }
    
    size_t GetTotalResults() const { return allResults_.size(); }
    size_t GetPageSize() const { return pageSize_; }
    
private:
    std::vector<EntityHandle> allResults_;
    size_t pageSize_;
    size_t currentPage_;
};

// Query composition operations
enum class QueryCompositionOp {
    Union,      // Combine results (OR)
    Intersect,  // Common results (AND)
    Except      // Difference (NOT)
};

// Query composition for combining multiple entity result sets
class ComposedQuery {
public:
    ComposedQuery(std::vector<EntityHandle> leftResults,
                  std::vector<EntityHandle> rightResults,
                  QueryCompositionOp operation)
        : leftResults_(std::move(leftResults))
        , rightResults_(std::move(rightResults))
        , operation_(operation) {}
    
    // Execute composed query
    std::vector<EntityHandle> Execute() {
        switch (operation_) {
            case QueryCompositionOp::Union:
                return Union(leftResults_, rightResults_);
            
            case QueryCompositionOp::Intersect:
                return Intersect(leftResults_, rightResults_);
            
            case QueryCompositionOp::Except:
                return Except(leftResults_, rightResults_);
        }
        
        return {};
    }
    
private:
    std::vector<EntityHandle> Union(const std::vector<EntityHandle>& left,
                                    const std::vector<EntityHandle>& right) {
        std::set<EntityHandle> resultSet(left.begin(), left.end());
        resultSet.insert(right.begin(), right.end());
        return std::vector<EntityHandle>(resultSet.begin(), resultSet.end());
    }
    
    std::vector<EntityHandle> Intersect(const std::vector<EntityHandle>& left,
                                       const std::vector<EntityHandle>& right) {
        std::set<EntityHandle> leftSet(left.begin(), left.end());
        std::set<EntityHandle> rightSet(right.begin(), right.end());
        
        std::vector<EntityHandle> result;
        std::set_intersection(
            leftSet.begin(), leftSet.end(),
            rightSet.begin(), rightSet.end(),
            std::back_inserter(result)
        );
        
        return result;
    }
    
    std::vector<EntityHandle> Except(const std::vector<EntityHandle>& left,
                                     const std::vector<EntityHandle>& right) {
        std::set<EntityHandle> leftSet(left.begin(), left.end());
        std::set<EntityHandle> rightSet(right.begin(), right.end());
        
        std::vector<EntityHandle> result;
        std::set_difference(
            leftSet.begin(), leftSet.end(),
            rightSet.begin(), rightSet.end(),
            std::back_inserter(result)
        );
        
        return result;
    }
    
    std::vector<EntityHandle> leftResults_;
    std::vector<EntityHandle> rightResults_;
    QueryCompositionOp operation_;
};

// Logical predicate operators
namespace predicates {

template<typename T>
struct GreaterThan {
    T value;
    
    explicit GreaterThan(T val) : value(val) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return GetValue(comp) > value;
    }
    
private:
    template<typename Component>
    auto GetValue(const Component& comp) const -> decltype(comp.value) {
        return comp.value;
    }
};

template<typename T>
struct LessThan {
    T value;
    
    explicit LessThan(T val) : value(val) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return GetValue(comp) < value;
    }
    
private:
    template<typename Component>
    auto GetValue(const Component& comp) const -> decltype(comp.value) {
        return comp.value;
    }
};

template<typename T>
struct EqualTo {
    T value;
    
    explicit EqualTo(T val) : value(val) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return GetValue(comp) == value;
    }
    
private:
    template<typename Component>
    auto GetValue(const Component& comp) const -> decltype(comp.value) {
        return comp.value;
    }
};

template<typename T>
struct InRange {
    T min, max;
    
    InRange(T minVal, T maxVal) : min(minVal), max(maxVal) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        auto val = GetValue(comp);
        return val >= min && val <= max;
    }
    
private:
    template<typename Component>
    auto GetValue(const Component& comp) const -> decltype(comp.value) {
        return comp.value;
    }
};

// Logical AND combinator
template<typename Pred1, typename Pred2>
struct And {
    Pred1 pred1;
    Pred2 pred2;
    
    And(Pred1 p1, Pred2 p2) : pred1(p1), pred2(p2) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return pred1(comp) && pred2(comp);
    }
};

// Logical OR combinator
template<typename Pred1, typename Pred2>
struct Or {
    Pred1 pred1;
    Pred2 pred2;
    
    Or(Pred1 p1, Pred2 p2) : pred1(p1), pred2(p2) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return pred1(comp) || pred2(comp);
    }
};

// Logical NOT combinator
template<typename Pred>
struct Not {
    Pred pred;
    
    explicit Not(Pred p) : pred(p) {}
    
    template<typename Component>
    bool operator()(const Component& comp) const {
        return !pred(comp);
    }
};

} // namespace predicates

// Note: Query composition operators are provided by ComposedQuery class
// Usage example:
// auto query1 = std::make_shared<ComposedQuery<ManagerType>>(...);
// auto query2 = std::make_shared<ComposedQuery<ManagerType>>(...);
// Use ComposedQuery constructor directly for composition

// Helper functions for creating predicates
template<typename T>
auto Gt(T value) { return predicates::GreaterThan<T>(value); }

template<typename T>
auto Lt(T value) { return predicates::LessThan<T>(value); }

template<typename T>
auto Eq(T value) { return predicates::EqualTo<T>(value); }

template<typename T>
auto Range(T min, T max) { return predicates::InRange<T>(min, max); }

template<typename Pred1, typename Pred2>
auto And(Pred1 p1, Pred2 p2) { return predicates::And<Pred1, Pred2>(p1, p2); }

template<typename Pred1, typename Pred2>
auto Or(Pred1 p1, Pred2 p2) { return predicates::Or<Pred1, Pred2>(p1, p2); }

template<typename Pred>
auto Not(Pred p) { return predicates::Not<Pred>(p); }

} // namespace ecs
