#pragma once

// TODO: ActorContext Enhancement Roadmap
// 
// ═══════════════════════════════════════════════════════════════════════════════════
// ACTOR CONTEXT SYSTEM - COMPREHENSIVE IMPROVEMENT PLAN
// ═══════════════════════════════════════════════════════════════════════════════════
//
// 🏗️  ARCHITECTURE IMPROVEMENTS
// [ ] Dual ECS Support: Add EntityManagerV2 compatibility alongside legacy support
// [ ] Type Safety: Add compile-time checks for component type validity
// [ ] Context Validation: Runtime validation of entity/manager validity
// [ ] Smart Pointers: Consider unique_ptr/shared_ptr for automatic lifecycle management
// [ ] Context Pooling: Object pool for frequently created/destroyed contexts
// [ ] Context Hierarchy: Support parent-child context relationships
// [ ] Multi-Entity Context: Support for contexts managing multiple entities
// [ ] Context Serialization: Save/load context state for persistence
//
// ⚡ PERFORMANCE OPTIMIZATIONS
// [ ] Component Caching: Cache frequently accessed components
// [ ] Lazy Evaluation: Defer expensive operations until actually needed
// [ ] Batch Operations: Support batch component add/remove operations
// [ ] Memory Layout: Optimize memory layout for cache efficiency
// [ ] Context Reuse: Reuse context objects to reduce allocations
// [ ] Fast Path: Optimized paths for common component access patterns
// [ ] SIMD Operations: Vector operations for bulk component updates
// [ ] Lock-Free Access: Concurrent-safe component access without locks
//
// 🔧 DEVELOPER EXPERIENCE
// [ ] Enhanced Debugging: Better error messages with file/line info
// [ ] Component Inspector: Runtime inspection of attached components
// [ ] Type Reflection: Runtime type information for components
// [ ] Context Builder: Fluent API for context construction
// [ ] Auto-Registration: Automatic component registration during context creation
// [ ] Context Templates: Pre-configured contexts for common actor types
// [ ] IDE Integration: Better IntelliSense support with type hints
// [ ] Documentation Generator: Auto-generate docs from component definitions
//
// 🛡️  SAFETY & VALIDATION
// [ ] Null Safety: Comprehensive null pointer checking
// [ ] Entity Lifecycle: Track entity destruction and invalidate contexts
// [ ] Component Dependencies: Validate component dependency requirements
// [ ] Access Control: Read-only vs read-write component access modes
// [ ] Thread Safety: Multi-threaded access safety mechanisms
// [ ] Memory Safety: Bounds checking and memory validation
// [ ] Error Recovery: Graceful handling of invalid operations
// [ ] Assertion System: Development-time checks with detailed failure info
//
// 🔌 INTEGRATION FEATURES
// [ ] Event System: Component change notifications and events
// [ ] Query Integration: Direct QueryBuilder integration for context sets
// [ ] System Callbacks: Hooks for system registration and updates
// [ ] Actor Factories: Integration with actor creation and spawning systems
// [ ] Component Streaming: Network synchronization support
// [ ] Hot Reload: Support for component hot-swapping during development
// [ ] Plugin System: Extensible component system for modding
// [ ] Cross-Engine: Compatibility layer for other ECS implementations
//
// 🎯 SPECIALIZED FEATURES
// [ ] Component Groups: Manage related components as atomic units
// [ ] Proxy Components: Virtual components that redirect to other entities
// [ ] Component Versioning: Track component changes and history
// [ ] Conditional Components: Components that exist only under certain conditions
// [ ] Component Blueprints: Template-based component initialization
// [ ] Dynamic Components: Runtime component type creation and management
// [ ] Component Metadata: Attach metadata and tags to components
// [ ] Resource Management: Automatic cleanup of component-owned resources

// ActorContext depends on EntityManager being fully defined
// This header MUST be included AFTER "ecs/EntityManager.h"
#ifndef ECS_ENTITY_MANAGER_H
#error "ActorContext.h requires ecs/EntityManager.h to be included first"
#endif

#include <memory>

// Forward declare Entity type from global namespace
using Entity = int;

/**
 * ActorContext: Simple ECS integration for actors
 * Beginner-friendly interface for accessing entity components
 * 
 * REQUIREMENT: Must include "ecs/EntityManager.h" before this header
 * 
 * TODO: Core ActorContext improvements
 * [ ] EntityManagerV2 Support: Add modern ECS compatibility layer
 * [ ] Component Validation: Runtime checks for component existence/validity
 * [ ] Type Safety: Compile-time component type verification
 * [ ] Error Handling: Better error messages for invalid operations
 * [ ] Context Lifetime: Automatic invalidation when entity is destroyed
 * [ ] Multi-Manager: Support for multiple EntityManager instances
 * [ ] Component Caching: Cache frequently accessed components for performance
 * [ ] Batch Operations: Support adding/removing multiple components atomically
 * [ ] Context Cloning: Deep copy contexts for entity duplication
 * [ ] Access Patterns: Track usage patterns for optimization opportunities
 */
class ActorContext {
public:
    // TODO: Constructor enhancements
    // [ ] Validation: Check entity validity during construction
    // [ ] Builder Pattern: Fluent API for context setup
    // [ ] Entity Creation: Support creating entity during context construction
    // [ ] Component Preloading: Pre-populate common components during construction
    // [ ] Context Chaining: Support for hierarchical context relationships
    
    ActorContext() = default;
    ActorContext(::EntityManager& em, Entity entity)
        : entityManager_(&em), entity_(entity) {}

    // TODO: Enhanced accessors with safety and performance improvements
    // [ ] Null Safety: Return nullable types or throw exceptions on invalid access
    // [ ] Const Correctness: Better const-safety for read-only operations
    // [ ] Reference Returns: Option to return references instead of pointers
    // [ ] Access Tracking: Log component access patterns for optimization
    // [ ] Lazy Loading: Defer component lookup until actually accessed
    
    // Simple accessors
    ::EntityManager* GetEntityManager() const { return entityManager_; }
    Entity GetEntity() const { return entity_; }

    // Convenience template methods (defined in ActorContextImpl.h)
    // TODO: Advanced component access methods
    // [ ] GetOrAdd: Get component or add with default if missing
    // [ ] TryGet: Safe component access that returns optional/nullable
    // [ ] GetMultiple: Retrieve multiple components in single call
    // [ ] HasComponent: Check component existence without retrieving
    // [ ] RemoveComponent: Safe component removal with validation
    // [ ] UpdateComponent: Atomic component update operations
    // [ ] ComponentCount: Get number of components attached to entity
    // [ ] ListComponents: Get list of all component types on entity
    // [ ] CloneComponents: Copy all components to another entity
    // [ ] ValidateComponents: Check component integrity and relationships
    
    template<typename T>
    T* GetComponent() const;

    template<typename T>
    void AddComponent(T component) const;

private:
    // TODO: Enhanced internal state management
    // [ ] Context ID: Unique identifier for debugging and tracking
    // [ ] Creation Timestamp: Track when context was created for profiling
    // [ ] Access Statistics: Track component access frequency and patterns
    // [ ] Validation State: Cache entity/manager validity to avoid repeated checks
    // [ ] Component Cache: Cache pointers to frequently accessed components
    // [ ] Event Callbacks: Store callbacks for component change notifications
    // [ ] Context Metadata: Store additional context-specific information
    // [ ] Thread Safety: Add synchronization primitives for multi-threaded access
    // [ ] Memory Tracking: Track memory usage for debugging and optimization
    // [ ] Context Flags: Bit flags for various context states and options
    
    ::EntityManager* entityManager_ = nullptr;
    Entity entity_ = 0;
};