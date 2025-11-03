#pragma once

#include <string>
#include <memory>

// Forward declarations
class ActorContext;

/**
 * IActor: Base interface for all game actors
 * Actors represent gameplay objects that integrate with the ECS
 * 
 * TODO: Comprehensive IActor Interface Roadmap
 * 
 * === ACTOR FACTORY SYSTEM ===
 * [✓] Factory Registration: Automatic factory registration for new actor types
 * [✓] Factory Validation: Validate factory configurations and dependencies
 * [✓] Factory Performance: Optimize actor creation and initialization
 * [✓] Factory Caching: Cache frequently created actor configurations
 * [✓] Factory Templates: Template system for actor variations
 * [✓] Factory Analytics: Track actor creation patterns and performance
 * [✓] Factory Documentation: Auto-generate factory documentation
 * [✓] Factory Testing: Automated testing for actor factory system
 * [✓] Factory Debugging: Debug tools for actor creation issues
 * [✓] Factory Monitoring: Monitor actor factory health and performance
 * 
 * === ACTOR LIFECYCLE MANAGEMENT ===
 * [ ] Advanced Lifecycle: Multi-stage actor lifecycle with validation phases
 * [ ] State Transitions: Managed state transitions (creating, active, paused, destroying)
 * [ ] Lifecycle Events: Events for lifecycle state changes and transitions
 * [ ] Lifecycle Validation: Validate actor state consistency during transitions
 * [ ] Lifecycle Debugging: Debug tools for actor lifecycle inspection
 * [ ] Lifecycle Analytics: Track actor lifecycle metrics and performance
 * [ ] Error Recovery: Error recovery during lifecycle transitions
 * [ ] Lifecycle Hooks: Extension hooks for custom lifecycle behavior
 * [ ] Async Lifecycle: Asynchronous lifecycle operations for performance
 * [ ] Lifecycle Documentation: Auto-generate lifecycle documentation
 * 
 * === ACTOR CONTEXT INTEGRATION ===
 * [ ] Context Validation: Validate actor context attachment and integrity
 * [ ] Context Caching: Cache frequently accessed context components
 * [ ] Context Events: Events for context changes and updates
 * [ ] Context Migration: Support for context migration and updates
 * [ ] Context Security: Security validation for actor context access
 * [ ] Context Performance: Optimize context access and operations
 * [ ] Context Debugging: Debug tools for context state inspection
 * [ ] Context Analytics: Track context usage patterns and performance
 * [ ] Context Threading: Thread-safe context access and operations
 * [ ] Context Versioning: Support for different context versions
 * 
 * === ACTOR COMMUNICATION ===
 * [ ] Message System: Actor-to-actor message passing system
 * [ ] Event Broadcasting: Broadcast events to multiple actors
 * [ ] Signal System: Lightweight signal system for actor communication
 * [ ] Communication Queues: Queued communication for asynchronous messaging
 * [ ] Communication Security: Secure communication between actors
 * [ ] Communication Analytics: Track communication patterns and performance
 * [ ] Communication Debugging: Debug tools for actor communication
 * [ ] Communication Validation: Validate message formats and content
 * [ ] Communication Routing: Intelligent message routing and delivery
 * [ ] Communication Performance: Optimize communication performance
 * 
 * === ACTOR BEHAVIOR FRAMEWORK ===
 * [ ] Behavior Trees: Integration with behavior tree systems
 * [ ] State Machines: Built-in state machine support for actors
 * [ ] Script Integration: Script-based behavior definition and execution
 * [ ] Behavior Composition: Compose complex behaviors from simple components
 * [ ] Behavior Validation: Validate behavior configurations and logic
 * [ ] Behavior Analytics: Track behavior execution and performance
 * [ ] Behavior Debugging: Debug tools for behavior inspection and testing
 * [ ] Behavior Events: Events for behavior state changes and execution
 * [ ] Behavior Performance: Optimize behavior execution performance
 * [ ] Behavior Documentation: Auto-generate behavior documentation
 * 
 * === ACTOR SERIALIZATION ===
 * [ ] State Serialization: Serialize and deserialize actor state
 * [ ] Configuration Serialization: Serialize actor configurations
 * [ ] Network Serialization: Efficient network serialization for multiplayer
 * [ ] Save/Load System: Integration with game save/load systems
 * [ ] Serialization Validation: Validate serialized data integrity
 * [ ] Serialization Performance: Optimize serialization performance
 * [ ] Serialization Security: Secure serialization and deserialization
 * [ ] Serialization Versioning: Support for different serialization versions
 * [ ] Serialization Analytics: Track serialization usage and performance
 * [ ] Serialization Debugging: Debug tools for serialization issues
 * 
 * === ACTOR PERFORMANCE ===
 * [ ] Performance Profiling: Built-in performance profiling for actors
 * [ ] Memory Management: Efficient memory management and pooling
 * [ ] Update Optimization: Optimize actor update performance
 * [ ] Batch Processing: Batch processing for multiple actors
 * [ ] LOD System: Level-of-detail system for actor processing
 * [ ] Performance Monitoring: Real-time performance monitoring
 * [ ] Performance Analytics: Track performance metrics and trends
 * [ ] Performance Validation: Validate performance requirements
 * [ ] Performance Debugging: Debug tools for performance issues
 * [ ] Performance Documentation: Document performance characteristics
 * 
 * === ACTOR DEBUGGING ===
 * [ ] Debug Visualization: Visual debugging tools for actor state
 * [ ] Debug Console: In-game debug console for actor manipulation
 * [ ] Debug Logging: Comprehensive logging for actor operations
 * [ ] Debug Validation: Validate actor state and behavior
 * [ ] Debug Analytics: Analytics for debugging patterns and issues
 * [ ] Debug Performance: Performance monitoring for debug operations
 * [ ] Debug Security: Secure debug access and operations
 * [ ] Debug Documentation: Documentation for debug tools and procedures
 * [ ] Debug Testing: Automated testing for debug functionality
 * [ ] Debug Integration: Integration with external debugging tools
 * 
 * === ACTOR EXTENSIBILITY ===
 * [ ] Plugin System: Plugin system for extending actor functionality
 * [ ] Modding Support: Support for modded actor types and behaviors
 * [ ] Extension Points: Extension points for custom actor functionality
 * [ ] Component System: Component-based actor extension system
 * [ ] Script Extensions: Script-based actor extensions and customization
 * [ ] Extension Validation: Validate actor extensions and modifications
 * [ ] Extension Analytics: Track extension usage and performance
 * [ ] Extension Documentation: Documentation for extension development
 * [ ] Extension Testing: Testing framework for actor extensions
 * [ ] Extension Security: Security validation for actor extensions
 */
class IActor {
public:
    IActor() = default;
    virtual ~IActor() = default;

    // Delete copy operations
    IActor(const IActor&) = delete;
    IActor& operator=(const IActor&) = delete;

    // Allow move operations
    IActor(IActor&&) = default;
    IActor& operator=(IActor&&) = default;

    /**
     * Attach this actor to an ECS context
     * Called once after construction to establish ECS integration
     * 
     * TODO: Enhanced context attachment
     * [ ] Context Validation: Validate context before attachment
     * [ ] Context Security: Security checks for context access
     * [ ] Context Versioning: Support for different context versions
     * [ ] Context Migration: Migrate between different context types
     * [ ] Context Events: Generate events for context attachment
     * [ ] Context Performance: Optimize context attachment performance
     * [ ] Context Debugging: Debug tools for context attachment
     * [ ] Context Analytics: Track context attachment patterns
     * [ ] Context Error Handling: Handle context attachment errors
     * [ ] Context Documentation: Document context attachment process
     */
    virtual void AttachContext(ActorContext context) {
        context_ = std::move(context);
    }

    /**
     * Initialize the actor with specific configuration
     * Called after AttachContext to set up actor-specific state
     * 
     * TODO: Advanced actor initialization
     * [ ] Multi-phase Init: Multi-stage initialization with dependency resolution
     * [ ] Init Validation: Validate initialization parameters and state
     * [ ] Init Performance: Optimize initialization for fast actor spawning
     * [ ] Init Error Handling: Robust error handling during initialization
     * [ ] Init Events: Generate events for initialization stages
     * [ ] Init Analytics: Track initialization metrics and performance
     * [ ] Init Debugging: Debug tools for initialization process
     * [ ] Init Documentation: Document initialization requirements
     * [ ] Init Testing: Automated testing of initialization scenarios
     * [ ] Init Security: Security validation during initialization
     */
    virtual void Initialize() = 0;

    /**
     * Get the actor's ECS context
     * 
     * TODO: Enhanced context access
     * [ ] Context Caching: Cache context access for performance
     * [ ] Context Validation: Validate context state before access
     * [ ] Context Security: Security checks for context access
     * [ ] Context Performance: Optimize context access operations
     * [ ] Context Events: Generate events for context access
     * [ ] Context Analytics: Track context access patterns
     * [ ] Context Debugging: Debug tools for context access
     * [ ] Context Error Handling: Handle context access errors
     * [ ] Context Documentation: Document context access patterns
     * [ ] Context Threading: Thread-safe context access
     */
    const ActorContext& Context() const { return context_; }

    /**
     * Get the actor's name/type for debugging
     * 
     * TODO: Enhanced actor identification
     * [ ] Unique IDs: Support for unique actor identifiers
     * [ ] Type Hierarchy: Support for actor type hierarchies
     * [ ] Localization: Localized actor names for different languages
     * [ ] Name Validation: Validate actor names and identifiers
     * [ ] Name Analytics: Track actor name usage and patterns
     * [ ] Name Performance: Optimize name retrieval performance
     * [ ] Name Documentation: Document actor naming conventions
     * [ ] Name Security: Security validation for actor names
     * [ ] Name Debugging: Debug tools for actor identification
     * [ ] Name Integration: Integration with external identification systems
     */
    virtual std::string GetName() const = 0;

    /**
     * Update the actor (called each frame)
     * 
     * TODO: Advanced actor update system
     * [ ] Update Scheduling: Intelligent update scheduling based on priority
     * [ ] Update Batching: Batch updates for similar actors
     * [ ] Update Performance: Optimize update performance and profiling
     * [ ] Update Validation: Validate actor state after updates
     * [ ] Update Events: Generate events for update cycles
     * [ ] Update Analytics: Track update performance and patterns
     * [ ] Update Debugging: Debug tools for update process
     * [ ] Update Error Handling: Handle errors during update cycles
     * [ ] Update Documentation: Document update requirements and behavior
     * [ ] Update Threading: Multi-threaded update processing
     */
    virtual void Update(double dt) {
        (void)dt; // Default: no-op
    }

protected:
    // TODO: Enhanced actor data members
    // [ ] Performance Counters: Performance monitoring and metrics collection
    // [ ] Event Handlers: Event handling system for actor interactions
    // [ ] State Variables: Additional state variables for complex actor behavior
    // [ ] Cache Data: Cache frequently accessed data for performance
    // [ ] Debug Data: Debug information and diagnostic data
    // [ ] Security Data: Security-related data and access controls
    // [ ] Analytics Data: Analytics and telemetry data collection
    // [ ] Extension Data: Data for actor extensions and plugins
    // [ ] Network Data: Network synchronization and communication data
    // [ ] Configuration Data: Runtime configuration and settings data
    
    ActorContext context_;
};

// Actor factory function type
using ActorFactory = std::unique_ptr<IActor>(*)();

// TODO: Enhanced actor factory system
// [ ] Factory Registry: Centralized registry for actor factories
// [ ] Factory Validation: Validate factory functions and signatures
// [ ] Factory Performance: Optimize factory function performance
// [ ] Factory Analytics: Track factory usage and performance
// [ ] Factory Security: Security validation for factory functions
// [ ] Factory Documentation: Document factory function requirements
// [ ] Factory Testing: Automated testing for factory functions
// [ ] Factory Error Handling: Handle factory function errors
// [ ] Factory Debugging: Debug tools for factory function inspection
// [ ] Factory Integration: Integration with dependency injection systems
