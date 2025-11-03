# Nova Engine Content Architecture

A comprehensive content management system for game content with schema validation, dependency tracking, composition, versioning, testing, and analytics.

## Overview

The Content Architecture provides a unified framework for managing all types of game content including:
- Spacecraft definitions
- Weapon systems
- NPCs and AI behaviors
- Missions and quests
- UI configurations
- Audio and visual effects
- Game balance parameters

## Core Components

### 1. Content Framework (`ContentFramework.h`)
- **ContentDefinition**: Base class for all content types
- **ContentRegistry**: Central registry for all content
- **ContentFactory**: Factory for creating content instances
- **ContentLoader**: Load/save content from files
- **ContentFramework**: Main interface for content management

### 2. Content Schema (`ContentSchema.h`)
- **SchemaField**: Define field types and constraints
- **ContentSchema**: Schema definition for content types
- **SchemaBuilder**: Fluent API for building schemas
- **ContentSchemaRegistry**: Registry for all schemas

### 3. Content Validation (`ContentValidator.h`)
- **BalanceValidator**: Ensures numerical balance
- **ReferenceValidator**: Validates content references
- **ConsistencyValidator**: Checks internal consistency
- **CompletenessValidator**: Ensures required data exists
- **FormatValidator**: Validates data format
- **ContentValidatorRegistry**: Manages all validators

### 4. Dependency Management (`ContentDependencyGraph.h`)
- **ContentDependencyGraph**: Tracks content dependencies
- **ContentDependencyResolver**: Resolves load order
- **DependencyChangeTracker**: Tracks dependency changes
- Cycle detection and topological sorting

### 5. Content Composition (`ContentCompositor.h`)
- **ContentCompositor**: Compose content from multiple bases
- **ContentInheritance**: Hierarchical inheritance system
- **ContentTemplate**: Template-based content creation
- **CompositionBuilder**: Fluent API for composition

### 6. Version Control (`ContentVersioning.h`)
- **ContentVersionControl**: Full version history
- **ContentMigration**: Automated version migration
- **ContentChangelog**: Generate changelogs
- **SemanticVersion**: Version comparison utilities

### 7. Content Testing (`ContentTesting.h`)
- **BalanceTest**: Balance testing
- **IntegrationTest**: Integration testing
- **RegressionTest**: Regression detection
- **PerformanceTest**: Performance testing
- **ContentTestRegistry**: Test management
- **ContentTestRunner**: Automated test execution

### 8. Content Analytics (`ContentAnalytics.h`)
- **ContentAnalytics**: Usage tracking and statistics
- **PlayerEngagementAnalytics**: Player engagement metrics
- **ContentHealthMonitor**: Content health monitoring
- **ContentABTesting**: A/B testing framework

## Quick Start

### 1. Define a Content Type

```cpp
#include "engine/content/ContentFramework.h"

class SpaceshipContent : public NovaEngine::ContentDefinition {
public:
    SpaceshipContent(const std::string& id)
        : ContentDefinition(id, "spaceship") {}
    
    // Data members
    std::string name;
    float health;
    float speed;
    std::vector<std::string> weapons;
    
    SimpleJson ToJson() const override {
        SimpleJson json;
        json.Set("id", GetId());
        json.Set("type", GetType());
        json.Set("name", name);
        json.Set("health", health);
        json.Set("speed", speed);
        // ... more fields
        return json;
    }
    
    bool FromJson(const SimpleJson& json) override {
        auto nameNode = json.Get("name");
        if (nameNode) name = nameNode->AsString();
        
        auto healthNode = json.Get("health");
        if (healthNode) health = healthNode->AsNumber();
        
        // ... more fields
        return true;
    }
    
    bool Validate(std::vector<std::string>& errors) const override {
        if (health <= 0) {
            errors.push_back("Health must be positive");
            return false;
        }
        return true;
    }
    
    std::vector<std::string> GetDependencies() const override {
        return weapons;  // Weapons are dependencies
    }
    
    std::unique_ptr<ContentDefinition> Clone() const override {
        auto clone = std::make_unique<SpaceshipContent>(GetId());
        clone->name = name;
        clone->health = health;
        clone->speed = speed;
        clone->weapons = weapons;
        return clone;
    }
};

// Register the type
REGISTER_CONTENT_TYPE(SpaceshipContent, "spaceship");
```

### 2. Define a Schema

```cpp
#include "engine/content/ContentSchema.h"

void RegisterSpaceshipSchema() {
    auto schema = BEGIN_CONTENT_SCHEMA("spaceship", "Spaceship definition")
        SCHEMA_FIELD("name", String)
            SCHEMA_REQUIRED()
            SCHEMA_DESC("Display name of the spaceship")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("health", Float)
            SCHEMA_REQUIRED()
            SCHEMA_MIN(1.0f)
            SCHEMA_MAX(10000.0f)
            SCHEMA_DEFAULT(100.0f)
            SCHEMA_DESC("Maximum health points")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("speed", Float)
            SCHEMA_REQUIRED()
            SCHEMA_MIN(0.0f)
            SCHEMA_MAX(1000.0f)
            SCHEMA_DESC("Maximum speed in units/second")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("weapons", Array)
            SCHEMA_DESC("List of weapon IDs")
            END_SCHEMA_FIELD()
        
        .Build();
    
    REGISTER_SCHEMA(schema);
}
```

### 3. Load and Use Content

```cpp
#include "engine/content/ContentFramework.h"

void InitializeContent() {
    auto& framework = NovaEngine::ContentFramework::Instance();
    framework.Initialize();
    
    // Load all content from directory
    framework.LoadContentDirectory("assets/content/spaceships");
    
    // Get specific content
    auto* ship = framework.GetContentAs<SpaceshipContent>("fighter_mk1");
    if (ship) {
        std::cout << "Loaded ship: " << ship->name << std::endl;
        std::cout << "Health: " << ship->health << std::endl;
    }
    
    // Validate all content
    std::unordered_map<std::string, std::vector<std::string>> errors;
    framework.ValidateAllContent(errors);
}
```

### 4. Composition and Inheritance

```cpp
// Create a variant by composition
auto heavyFighter = NovaEngine::CompositionBuilder("heavy_fighter")
    .AddBase("fighter_mk1")
    .AddBase("armor_upgrade")
    .WithStrategy("health", NovaEngine::CompositionStrategy::Add)
    .WithStrategy("speed", NovaEngine::CompositionStrategy::Multiply)
    .Build();

// Or use inheritance
NovaEngine::ContentInheritance::Instance().SetBaseContent(
    "elite_fighter", "fighter_mk1");

auto resolved = NovaEngine::ContentInheritance::Instance()
    .ResolveInheritance("elite_fighter");
```

### 5. Validation

```cpp
// Custom validator
auto validator = std::make_shared<NovaEngine::CustomValidator>(
    "Spaceship Balance",
    "Ensures spaceships are balanced",
    "spaceship",
    [](const NovaEngine::ContentDefinition& content, 
       std::vector<NovaEngine::ValidationResult>& results) {
        auto& ship = static_cast<const SpaceshipContent&>(content);
        
        // Check balance ratio
        float ratio = ship.health / ship.speed;
        if (ratio < 0.5f || ratio > 2.0f) {
            results.emplace_back("balance", 
                "Health/Speed ratio out of balance",
                NovaEngine::ValidationSeverity::Warning);
        }
        
        return true;
    }
);

NovaEngine::ContentValidatorRegistry::Instance().RegisterValidator(validator);
```

### 6. Testing

```cpp
// Create test suite
auto testSuite = std::make_shared<NovaEngine::ContentTestSuite>("Spaceship Tests");

// Add balance test
auto balanceTest = std::make_shared<NovaEngine::BalanceTest>("spaceship", "Balance Test");
balanceTest->AddConstraint({"health", 10.0, 1000.0, 50.0, 500.0});
balanceTest->AddConstraint({"speed", 1.0, 500.0, 10.0, 200.0});
testSuite->AddTest(balanceTest);

// Register and run
NovaEngine::ContentTestRegistry::Instance().RegisterSuite(testSuite);
auto results = testSuite->RunAll();

// Generate report
auto report = NovaEngine::ContentTestRegistry::Instance().GenerateReport(results);
```

### 7. Analytics

```cpp
// Track usage
NovaEngine::ContentAnalytics::Instance().TrackLoad("fighter_mk1", 15.5);
NovaEngine::ContentAnalytics::Instance().TrackAccess("fighter_mk1");

// Get statistics
auto stats = NovaEngine::ContentAnalytics::Instance().GetContentStats("fighter_mk1");
std::cout << "Load count: " << stats.loadCount << std::endl;
std::cout << "Avg load time: " << stats.avgLoadTimeMs << "ms" << std::endl;

// Generate analytics report
auto aggStats = NovaEngine::ContentAnalytics::Instance().GetAggregatedStats();
std::cout << "Most loaded content: " << std::endl;
for (const auto& [id, count] : aggStats.topLoaded) {
    std::cout << "  " << id << ": " << count << " loads" << std::endl;
}
```

## Content File Format

Content files use JSON format:

```json
{
  "id": "fighter_mk1",
  "type": "spaceship",
  "name": "Fighter Mk I",
  "metadata": {
    "version": "1.0.0",
    "author": "Designer",
    "tags": ["fighter", "light", "starter"],
    "description": "Basic light fighter"
  },
  "health": 100.0,
  "speed": 150.0,
  "weapons": [
    "laser_cannon_mk1",
    "missile_pod_basic"
  ],
  "stats": {
    "armor": 10,
    "shields": 50,
    "cargo": 20
  }
}
```

## Best Practices

1. **Always Define Schemas**: Create schemas for all content types to enable validation
2. **Use Composition**: Build complex content from simpler base components
3. **Version Control**: Commit content changes regularly
4. **Validate Early**: Run validation during development, not just at runtime
5. **Test Content**: Create automated tests for balance and integration
6. **Monitor Analytics**: Use analytics to identify unused or problematic content
7. **Document Changes**: Use the changelog system to track content evolution
8. **Dependency Management**: Keep dependency chains shallow to avoid complexity

## Performance Considerations

- Content is cached after first load
- Validation can be run asynchronously
- Dependency resolution uses topological sorting for optimal load order
- Analytics use event batching to minimize overhead
- Schema validation is fast (microseconds per content item)

## Integration with Game Systems

### Entity System Integration

```cpp
// In your entity factory
auto* shipContent = ContentFramework::Instance().GetContentAs<SpaceshipContent>(shipId);
if (shipContent) {
    Entity entity = entityManager.CreateEntity();
    entityManager.AddComponent(entity, 
        std::make_shared<Health>(shipContent->health));
    entityManager.AddComponent(entity, 
        std::make_shared<Velocity>(shipContent->speed, 0, 0));
    // ... more components
}
```

### Hot Reloading

```cpp
// Watch for content changes
ContentFramework::Instance().ReloadContent("fighter_mk1");

// Content is automatically revalidated and dependencies updated
```

## Future Enhancements

- [ ] Visual content editor
- [ ] Real-time collaborative editing
- [ ] Machine learning for balance optimization
- [ ] Procedural content generation templates
- [ ] Cloud-based content synchronization
- [ ] Advanced diff/merge tools
- [ ] Content localization support
- [ ] Performance profiling integration

## API Reference

See individual header files for detailed API documentation:
- `ContentFramework.h` - Core content management
- `ContentSchema.h` - Schema system
- `ContentValidator.h` - Validation framework
- `ContentDependencyGraph.h` - Dependency management
- `ContentCompositor.h` - Composition and inheritance
- `ContentVersioning.h` - Version control
- `ContentTesting.h` - Testing framework
- `ContentAnalytics.h` - Analytics and monitoring
