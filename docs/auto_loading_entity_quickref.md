# Auto-Loading Entity Quick Reference

## Creating a New Auto-Loading Entity

### 1. Create Entity Header (`entities/YourEntity.h`)

```cpp
#pragma once
#include "../engine/EntityCommon.h"

class YourEntity : public IActor {
public:
    YourEntity() { LoadConfiguration(); }
    
    void Initialize() override { SetupComponents(); }
    void Update(double deltaTime) override { /* game logic */ }
    std::string GetName() const override { return "YourEntity"; }

private:
    void LoadConfiguration() {
        config_.LoadFromFile("assets/actors/your_entity.json");
    }
    
    void SetupComponents() {
        if (auto* em = context_.GetEntityManager()) {
            // Add Position component
            auto pos = std::make_shared<Position>();
            pos->x = config_.GetNumber("position.x", 0.0);
            pos->y = config_.GetNumber("position.y", 0.0);
            pos->z = config_.GetNumber("position.z", 0.0);
            em->AddComponent<Position>(context_.GetEntity(), pos);

            // Add DrawComponent
            auto draw = std::make_shared<DrawComponent>();
            draw->mode = DrawComponent::RenderMode::Mesh3D;
            draw->visible = config_.GetBoolean("visible", true);
            draw->SetTint(
                config_.GetNumber("visual.color[0]", 1.0),
                config_.GetNumber("visual.color[1]", 1.0),
                config_.GetNumber("visual.color[2]", 1.0)
            );
            em->AddComponent<DrawComponent>(context_.GetEntity(), draw);

            // Add Physics component
            auto physics = std::make_shared<PhysicsBody>();
            physics->mass = config_.GetNumber("mass", 1.0);
            physics->isStatic = config_.GetBoolean("static", false);
            em->AddComponent<PhysicsBody>(context_.GetEntity(), physics);

            // Add ViewportID
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(context_.GetEntity(), vp);
        }
    }
    
    ActorConfig config_;
};
```

### 2. Create JSON Config (`assets/actors/your_entity.json`)

```json
{
    "name": "Your Entity",
    "description": "Entity description",
    "health": 100.0,
    "mass": 500.0,
    "visible": true,
    "static": false,
    "visual": {
        "scale": 1.0,
        "color": [1.0, 1.0, 1.0]
    },
    "position": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
    }
}
```

### 3. Update EntityFactory (`engine/EntityFactory.h`)

```cpp
// Add to header
static Entity CreateYourEntity(EntityManager& em, double x, double y, double z);
```

### 4. Implement Factory Method (`engine/EntityFactory.cpp`)

```cpp
Entity EntityFactory::CreateYourEntity(EntityManager& em, double x, double y, double z) {
    Entity entity = em.CreateEntity();
    
    auto actor = std::make_unique<YourEntity>();
    ActorContext context{&em, entity};
    actor->AttachContext(context);
    actor->Initialize();
    
    // Set position
    if (auto* pos = em.GetComponent<Position>(entity)) {
        pos->x = x; pos->y = y; pos->z = z;
    }
    
    em.AttachActor(entity, std::move(actor));
    return entity;
}
```

### 5. Add to CreateFromConfig (`engine/EntityFactory.cpp`)

```cpp
// In CreateFromConfig method, add:
if (configName == "your_entity") {
    return CreateYourEntity(em, x, y, z);
}
```

### 6. Build and Test

```bash
make                                    # Auto-includes your entity
make test                              # Build tests
./tests/test_your_entity               # Run your tests
```

## Usage Examples

```cpp
// Type-safe creation
Entity entity = EntityFactory::CreateYourEntity(entityManager, 10.0, 20.0, 30.0);

// Generic creation
Entity entity = EntityFactory::CreateFromConfig("your_entity", entityManager, 10.0, 20.0, 30.0);
```

## Key Components

- **Position**: Entity world position (x, y, z)
- **DrawComponent**: Rendering (mode, visibility, color)
- **PhysicsBody**: Physics properties (mass, static flag)
- **ViewportID**: Viewport assignment for rendering
- **Velocity**: Movement (vx, vy, vz)
- **PlayerPhysics**: Player-specific physics
- **CameraComponent**: Camera properties

## Common ActorConfig Methods

```cpp
// Numbers
double value = config_.GetNumber("path.to.value", defaultValue);

// Strings
std::string text = config_.GetString("name", "default");

// Booleans
bool flag = config_.GetBoolean("visible", true);

// Arrays (use index)
double r = config_.GetNumber("visual.color[0]", 1.0);
double g = config_.GetNumber("visual.color[1]", 1.0);
double b = config_.GetNumber("visual.color[2]", 1.0);
```

## Testing Template (`tests/test_your_entity.cpp`)

```cpp
#include "../engine/EntityManager.h"
#include "../engine/EntityFactory.h"
#include "../entities/YourEntity.h"
#include <cassert>
#include <iostream>

void TestDirectCreation() {
    EntityManager em;
    Entity entity = em.CreateEntity();
    
    auto actor = std::make_unique<YourEntity>();
    ActorContext context{&em, entity};
    actor->AttachContext(context);
    actor->Initialize();
    
    assert(em.GetComponent<Position>(entity) != nullptr);
    assert(em.GetComponent<DrawComponent>(entity) != nullptr);
    std::cout << "✓ Direct creation test passed" << std::endl;
}

void TestFactoryCreation() {
    EntityManager em;
    Entity entity = EntityFactory::CreateYourEntity(em, 10.0, 20.0, 30.0);
    
    auto* pos = em.GetComponent<Position>(entity);
    assert(pos != nullptr);
    assert(pos->x == 10.0 && pos->y == 20.0 && pos->z == 30.0);
    std::cout << "✓ Factory creation test passed" << std::endl;
}

int main() {
    TestDirectCreation();
    TestFactoryCreation();
    std::cout << "All YourEntity tests passed!" << std::endl;
    return 0;
}
```

## Build System Notes

- The Makefile automatically scans `entities/*.h`
- Generates `engine/Entities.h` with all includes
- No manual registration required
- Clean build if you get include errors: `make clean && make`

## Troubleshooting

**Include errors**: Make sure first include is `#include "../engine/EntityCommon.h"`

**Component not found**: Verify component type matches what's in EntityCommon.h

**Config not loading**: Check JSON syntax and file path

**Factory errors**: Make sure you added the method to both .h and .cpp files

**Build errors**: Try `make clean && make` for a fresh build