# Actor Configurations

This directory contains JSON configuration files for all auto-loading entities in Nova Engine.

**TODO: Comprehensive Actor Configuration System Roadmap**

### === CONFIGURATION SCHEMA ENHANCEMENT ===
[ ] Schema Validation: Implement JSON schema validation for all actor configs
[ ] Schema Evolution: Support for backward-compatible schema versioning
[ ] Schema Documentation: Auto-generate documentation from schema definitions
[ ] Schema Inheritance: Hierarchical configuration inheritance system
[ ] Schema Composition: Compose complex configs from reusable components
[ ] Schema Validation UI: Visual validation tools for designers
[ ] Schema Testing: Automated testing for configuration changes
[ ] Schema Migration: Migration tools for schema format changes
[ ] Schema Performance: Optimize configuration loading and parsing
[ ] Schema Analytics: Analytics for configuration usage patterns

#

### === CONFIGURATION MANAGEMENT ===
[ ] Config Editor: Visual editor for actor configurations
[ ] Config Validation: Real-time validation during configuration editing
[ ] Config Templates: Template system for common actor configurations
[ ] Config Version Control: Version control integration for configurations
[ ] Config Deployment: Deployment pipeline for configuration updates
[ ] Config Testing: Automated testing for configuration changes
[ ] Config Documentation: Documentation generation for configurations
[ ] Config Analytics: Analytics for configuration usage and performance
[ ] Config Security: Security validation for actor configurations
[ ] Config Performance: Performance optimization for configuration loading

### === MODDING INFRASTRUCTURE ===
[ ] Mod Config Support: Support for modded actor configurations
[ ] Mod Validation: Validate mod configurations for compatibility
[ ] Mod Distribution: Distribution system for mod configurations
[ ] Mod Documentation: Documentation and guides for mod configuration
[ ] Mod Testing: Testing framework for mod configurations
[ ] Mod Security: Security validation for mod configurations
[ ] Mod Performance: Performance monitoring for mod configurations
[ ] Mod Integration: Seamless integration of mod configurations
[ ] Mod Analytics: Analytics for mod configuration usage
[ ] Mod Marketplace: Marketplace for mod configuration sharing

### === ACTOR LIFECYCLE MANAGEMENT ===
[ ] Lifecycle Hooks: Hooks for actor creation, initialization, and destruction
[ ] Lifecycle Validation: Validate actor lifecycle state transitions
[ ] Lifecycle Performance: Optimize actor lifecycle operations
[ ] Lifecycle Analytics: Track actor lifecycle patterns and performance
[ ] Lifecycle Documentation: Documentation for actor lifecycle management
[ ] Lifecycle Testing: Automated testing for actor lifecycle operations
[ ] Lifecycle Debugging: Debug tools for actor lifecycle issues
[ ] Lifecycle Monitoring: Monitor actor lifecycle health and performance
[ ] Lifecycle Optimization: Optimize actor lifecycle for different scenarios
[ ] Lifecycle Integration: Integration with external lifecycle management tools

### === ADVANCED FEATURES ===
[ ] Dynamic Configuration: Runtime configuration updates and hot reloading
[ ] Configuration Profiles: Different configuration profiles for different scenarios
[ ] Configuration Optimization: AI-powered configuration optimization
[ ] Configuration A/B Testing: A/B testing framework for configurations
[ ] Configuration Machine Learning: ML-based configuration recommendations
[ ] Configuration Automation: Automated configuration generation and updates
[ ] Configuration Integration: Integration with external configuration tools
[ ] Configuration Innovation: Innovation in configuration technologies
[ ] Configuration Ecosystem: Ecosystem of configuration tools and services
[ ] Configuration Future: Future-proofing configuration system architecture

## Directory Structure

- **ships/**: Ship-based actors (Player, Spaceship, NPCs)
- **world/**: World objects (Stations, Cargo Containers, Asteroids)
- **projectiles/**: Projectile-based actors (Bullets, Missiles, Lasers)
- **effects/**: Effect actors (Explosions, Particle Fields)

## Configuration Format

All actor configs follow this JSON schema:

```json
{
    "name": "Display Name",
    "description": "Designer description",
    "category": "entity_type",
    
    "gameplay": {
        "health": 100.0,
        "capacity": 1000.0,
        "speed": 50.0
    },
    
    "physics": {
        "mass": 500.0,
        "static": false,
        "collision": true
    },
    
    "visual": {
        "model": "model_name",
        "scale": 1.0,
        "color": [1.0, 1.0, 1.0],
        "material": "default"
    },
    
    "position": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
    },
    
    "metadata": {
        "version": "1.0",
        "author": "Designer Name",
        "tags": ["tag1", "tag2"]
    }
}
```

## Usage

These configs are automatically loaded by the auto-loading entity system:

```cpp
// Automatic loading in entity constructor
config_.LoadFromFile("assets/actors/category/entity_name.json");

// Factory creation
Entity entity = EntityFactory::CreateFromConfig("entity_name", em, x, y, z);
```

## Adding New Actors

1. Create `category/new_actor.json` with configuration
2. Create `entities/NewActor.h` header file
3. Update `EntityFactory` with creation method
4. Build system automatically includes the new entity

See `docs/auto_loading_entity_quickref.md` for detailed instructions.