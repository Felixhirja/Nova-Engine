#pragma once

#include "../engine/EntityCommon.h"

/**
 * ProjectileActor: Physics-based projectile actor
 * Simplified implementation - loads configuration from JSON
 * 
 * TODO: Comprehensive Projectile System Roadmap
 * 
 * === PROJECTILE PHYSICS ===
 * [ ] Advanced Ballistics: Realistic ballistic physics with gravity and air resistance
 * [ ] Guided Missiles: Advanced guidance systems for seeking projectiles
 * [ ] Energy Weapons: Energy-based weapons with different physics properties
 * [ ] Explosive Projectiles: Area-of-effect damage and explosion mechanics
 * [ ] Penetration System: Armor penetration and ricochet mechanics
 * [ ] Projectile Trails: Visual and physics trails for projectile tracking
 * [ ] Environmental Effects: Environmental effects on projectile behavior
 * [ ] Projectile Clustering: Multiple projectile systems and cluster munitions
 * [ ] Deflection System: Projectile deflection by shields and armor
 * [ ] Physics Validation: Validate projectile physics for realism and gameplay
 * 
 * === PROJECTILE TYPES ===
 * [ ] Kinetic Weapons: Bullets, railgun slugs, and kinetic projectiles
 * [ ] Energy Weapons: Lasers, plasma bolts, and particle beams
 * [ ] Missile Systems: Guided missiles with different guidance methods
 * [ ] Explosive Ordnance: Bombs, grenades, and explosive projectiles
 * [ ] Exotic Weapons: Gravity weapons, temporal weapons, and exotic physics
 * [ ] Smart Munitions: AI-guided smart munitions with target selection
 * [ ] Defensive Projectiles: Countermeasures and defensive projectiles
 * [ ] Mining Tools: Mining charges and industrial projectiles
 * [ ] Utility Projectiles: Sensor probes, communication relays, and utilities
 * [ ] Modular Projectiles: Modular projectile system with interchangeable parts
 * 
 * === PROJECTILE GUIDANCE ===
 * [ ] Target Tracking: Advanced target tracking and prediction systems
 * [ ] Obstacle Avoidance: Intelligent obstacle avoidance for guided projectiles
 * [ ] Formation Flying: Coordinated projectile swarms and formations
 * [ ] Adaptive Guidance: Guidance systems that adapt to target behavior
 * [ ] Electronic Warfare: Jamming resistance and counter-countermeasures
 * [ ] Multi-target: Projectiles that can engage multiple targets
 * [ ] Waypoint Navigation: Projectiles that follow complex flight paths
 * [ ] Collaborative Guidance: Projectiles sharing targeting information
 * [ ] Predictive Guidance: Predictive algorithms for moving targets
 * [ ] Learning Guidance: Guidance systems that learn from previous engagements
 * 
 * === PROJECTILE EFFECTS ===
 * [ ] Visual Effects: Advanced visual effects for different projectile types
 * [ ] Audio Effects: Realistic audio for projectile flight and impact
 * [ ] Particle Systems: Particle effects for trails, impacts, and explosions
 * [ ] Lighting Effects: Dynamic lighting from energy weapons and explosions
 * [ ] Environmental Impact: Environmental damage and effects from impacts
 * [ ] Atmospheric Effects: Atmospheric interactions and visual effects
 * [ ] Material Response: Different materials responding uniquely to impacts
 * [ ] Shader Effects: Advanced shaders for energy weapons and effects
 * [ ] Post-processing: Post-processing effects for weapon discharge and impact
 * [ ] Performance Optimization: Efficient rendering of projectile effects
 * 
 * === PROJECTILE DAMAGE ===
 * [ ] Damage Types: Multiple damage types with different characteristics
 * [ ] Armor Interaction: Complex armor and damage interaction systems
 * [ ] Critical Hits: Critical hit system with location-based damage
 * [ ] Damage Over Time: Damage-over-time effects from special projectiles
 * [ ] Status Effects: Status effects applied by projectile impacts
 * [ ] Damage Falloff: Realistic damage falloff with distance and penetration
 * [ ] Component Damage: Specific component damage and system failures
 * [ ] Structural Damage: Structural integrity and catastrophic damage
 * [ ] Healing Projectiles: Repair nanobots and healing projectiles
 * [ ] Damage Analytics: Track damage statistics and effectiveness
 * 
 * === PROJECTILE AI ===
 * [ ] Target Selection: Intelligent target selection for guided projectiles
 * [ ] Threat Assessment: Assess threats and prioritize targets
 * [ ] Evasion Detection: Detect and counter target evasion maneuvers
 * [ ] Collaborative AI: AI coordination between multiple projectiles
 * [ ] Learning AI: AI that learns from successful and failed engagements
 * [ ] Behavioral Patterns: Different AI behavioral patterns for projectiles
 * [ ] Decision Making: Complex decision-making for guided munitions
 * [ ] Mission Planning: Mission planning for long-range projectiles
 * [ ] Adaptive Behavior: Adaptive behavior based on battlefield conditions
 * [ ] Emergency Protocols: Emergency AI protocols for projectile systems
 * 
 * === PROJECTILE PERFORMANCE ===
 * [ ] LOD System: Level-of-detail system for distant projectiles
 * [ ] Culling Optimization: Frustum and occlusion culling for projectiles
 * [ ] Batch Processing: Batch processing for multiple projectiles
 * [ ] Memory Management: Efficient memory management for projectile pools
 * [ ] Physics Optimization: Optimize physics calculations for projectiles
 * [ ] Collision Optimization: Efficient collision detection for fast projectiles
 * [ ] Network Optimization: Optimize network traffic for projectile data
 * [ ] Threading: Multi-threaded projectile processing
 * [ ] Cache Optimization: Cache-friendly data structures for projectiles
 * [ ] Performance Profiling: Profile projectile system performance
 * 
 * === PROJECTILE NETWORKING ===
 * [ ] Network Synchronization: Efficient projectile state synchronization
 * [ ] Prediction System: Client-side prediction for responsive projectiles
 * [ ] Lag Compensation: Lag compensation for projectile hits and misses
 * [ ] Authority Management: Server authority for projectile damage
 * [ ] Bandwidth Optimization: Optimize bandwidth usage for projectile data
 * [ ] Cheat Prevention: Prevent cheating in projectile systems
 * [ ] Network Diagnostics: Diagnose network issues with projectiles
 * [ ] Interpolation: Smooth interpolation for network-synchronized projectiles
 * [ ] Delta Compression: Compress projectile state changes
 * [ ] Priority System: Prioritize important projectiles for network updates
 */
class ProjectileActor : public IActor {
public:
    enum class ProjectileType {
        Bullet,
        Missile,
        Laser,
        Plasma
    };

    ProjectileActor(ProjectileType type = ProjectileType::Bullet,
               uint32_t ownerEntity = 0,
               double damage = 10.0)
        : projectileType_(type), ownerEntity_(ownerEntity), damage_(damage) {}
    ~ProjectileActor() override = default;

    void Initialize() override;
    std::string GetName() const override;
    void Update(double dt) override;

    // Projectile-specific methods
    ProjectileType GetProjectileType() const { return projectileType_; }

    uint32_t GetOwnerEntity() const { return ownerEntity_; }
    void SetOwnerEntity(uint32_t entity) { ownerEntity_ = entity; }

    double GetDamage() const { return damage_; }
    void SetDamage(double damage) { damage_ = damage; }

    double GetLifetime() const { return lifetime_; }
    void SetLifetime(double lifetime) { lifetime_ = lifetime; }

    bool IsExpired() const { return currentLifetime_ >= lifetime_; }
    
    // TODO: Advanced Projectile interface methods
    // [ ] Projectile Commands: High-level command interface for projectile control
    // [ ] Projectile Status: Comprehensive status reporting for projectile state
    // [ ] Projectile Events: Event system for projectile lifecycle and impacts
    // [ ] Projectile Configuration: Runtime configuration of projectile parameters
    // [ ] Projectile Serialization: Save and load projectile state for networking
    // [ ] Projectile Metrics: Performance metrics and impact analytics
    // [ ] Projectile Debugging: Debug interface for projectile behavior inspection
    // [ ] Projectile Integration: Integration with weapon and damage systems
    // [ ] Projectile Extensions: Extension points for modding and custom projectiles
    // [ ] Projectile Validation: Validation of projectile state and physics

private:
    // TODO: Enhanced Projectile data members
    // [ ] Physics State: Advanced physics state including rotation and spin
    // [ ] Guidance Data: Guidance system state and target tracking information
    // [ ] Effect Handles: Handles to visual and audio effects for the projectile
    // [ ] Damage Data: Detailed damage information and penetration state
    // [ ] Network State: Network synchronization state for multiplayer
    // [ ] Performance Counters: Performance monitoring for projectile systems
    // [ ] Event Handlers: Event handlers for collision and impact events
    // [ ] AI State: AI state for guided projectiles and smart munitions
    // [ ] Environmental Data: Environmental interaction state and effects
    // [ ] Debug Data: Debug visualization and diagnostic information
    
    ProjectileType projectileType_;
    uint32_t ownerEntity_;
    double damage_;
    double lifetime_ = 10.0; // seconds
    double currentLifetime_ = 0.0;

    std::unique_ptr<simplejson::JsonObject> config_;

    // Projectile-specific properties loaded from JSON
    std::string name_;
    double speed_ = 500.0;
    std::string model_;
    std::vector<std::string> effects_;
};

// Inline implementations

inline void ProjectileActor::Initialize() {
    // TODO: Advanced Projectile initialization
    // [ ] Multi-stage Init: Multi-stage initialization with physics setup
    // [ ] Effect Loading: Load and initialize visual and audio effects
    // [ ] Physics Integration: Initialize physics body and collision detection
    // [ ] Guidance Setup: Setup guidance systems for guided projectiles
    // [ ] Network Setup: Initialize network synchronization for multiplayer
    // [ ] Performance Setup: Setup performance monitoring and optimization
    // [ ] Event Registration: Register event handlers for lifecycle events
    // [ ] Validation: Validate projectile configuration and parameters
    // [ ] Error Handling: Robust error handling during initialization
    // [ ] Optimization: Optimize initialization for rapid projectile spawning
    
    // Load configuration from JSON
    config_ = ActorConfig::LoadFromFile("assets/actors/projectiles/projectile.json");
    if (config_) {
        name_ = ActorConfig::GetString(*config_, "name", "Plasma Bolt");
        speed_ = ActorConfig::GetNumber(*config_, "speed", 500.0);
        damage_ = ActorConfig::GetNumber(*config_, "damage", 150.0);
        lifetime_ = ActorConfig::GetNumber(*config_, "lifetime", 10.0);
        model_ = ActorConfig::GetString(*config_, "model", "plasma_bolt");
    }

    // Set up basic ECS components
    if (auto* em = context_.GetEntityManager()) {
        // Add position and velocity components
        em->AddComponent<Position>(context_.GetEntity(), std::make_shared<Position>(0.0, 0.0, 0.0));
        em->AddComponent<Velocity>(context_.GetEntity(), std::make_shared<Velocity>(0.0, 0.0, speed_));
        // Auto-add ViewportID component for rendering
        em->AddComponent<ViewportID>(context_.GetEntity(), std::make_shared<ViewportID>(0));
    }
}

inline std::string ProjectileActor::GetName() const {
    if (!name_.empty()) return name_;
    
    switch (projectileType_) {
        case ProjectileType::Bullet: return "Bullet";
        case ProjectileType::Missile: return "Missile";
        case ProjectileType::Laser: return "Laser";
        case ProjectileType::Plasma: return "Plasma";
        default: return "Projectile";
    }
}

inline void ProjectileActor::Update(double dt) {
    // TODO: Comprehensive Projectile update system
    // [ ] Physics Update: Update projectile physics including velocity and rotation
    // [ ] Guidance Update: Update guidance systems and target tracking
    // [ ] Collision Detection: Perform collision detection and impact resolution
    // [ ] Effect Update: Update visual and audio effects for the projectile
    // [ ] Lifetime Management: Manage projectile lifetime and cleanup
    // [ ] Network Update: Update network synchronization state
    // [ ] Performance Monitoring: Monitor projectile performance and metrics
    // [ ] Event Processing: Process collision and impact events
    // [ ] State Validation: Validate projectile state consistency
    // [ ] Cleanup: Clean up expired or impacted projectiles
    
    currentLifetime_ += dt;
    // TODO: Implement projectile behavior
}