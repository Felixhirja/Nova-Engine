# Nova Engine - AI Coding Assistant Guidelines

## Project Overview
Nova Engine is a 3D C++17 game engine focused on space simulation with physically-inspired flight controls, modular spacecraft assembly, and a data-driven ECS architecture. Key features include OpenGL rendering, GLFW windowing, entity-component systems, and actor-based game objects.

## Architecture Fundamentals

### Core Systems
- **Engine Core** (`engine/`): MainLoop, Viewport3D, Input, Simulation - handles game loop, rendering, and input
- **ECS System** (`engine/ecs/`): Dual ECS implementation (legacy EntityManager + modern EntityManagerV2 with archetypes)
- **Entity System** (`entities/`): Game objects (Spaceship, NPC, Station, Projectile) with automatic registration
- **Graphics Pipeline** (`engine/graphics/`): OpenGL rendering with multiple modes (Mesh3D, Billboard, Sprite2D, Particles)
- **Physics** (`engine/physics/`): Movement, collision, and physics simulation systems

### Key Design Patterns

#### Actor Registration
Entities use automatic registration via build system. Add new entity headers to `entities/` directory - the build system auto-generates `engine/Entities.h`:

```cpp
// In entities/YourNewEntity.h
class YourNewActor : public IActor {
    // ... implementation
};
```

#### Component-Based Architecture
Use ECS components for data, systems for logic. Prefer modern EntityManagerV2 for new code:

```cpp
// Modern ECS V2 (preferred)
EntityHandle entity = em.CreateEntity();
em.AddComponent<Position>(entity, {x, y, z});
em.AddComponent<Velocity>(entity, {vx, vy, vz});

// Legacy ECS (still supported)
Entity entity = em.CreateEntity();
em.AddComponent<Position>(entity, std::make_shared<Position>(x, y, z));
```

#### Spaceship Assembly System
Spaceships use modular component assembly with JSON configuration:

```cpp
// Load spaceship from JSON manifest
auto spaceship = SpaceshipSpawnBundles::CreateSpaceshipEntity(
    entityManager, definition, loadout, x, y, z
);
```

## Development Workflow

### Building
```bash
make                    # Build main executable
make test              # Build all test executables
make clean             # Remove all build artifacts
```

### Testing
Tests are comprehensive and cover all major systems. Run specific tests:
```bash
./tests/test_simulation          # Core game loop
./tests/test_camera_comprehensive # Camera systems
./tests/test_ecs_v2             # Modern ECS
./tests/test_physics            # Physics simulation
./tests/test_ship_assembly      # Spaceship creation
```

### Windows Development
Use PowerShell scripts for DLL management:
```powershell
.\scripts\check_dlls.ps1        # Copy required DLLs from MSYS2
```

### Cross-Platform Considerations
- **Linux**: Uses pkg-config for GLFW/FreeType detection
- **Windows**: MSYS2/MinGW with bundled DLLs in `lib/`
- **Build System**: Makefile auto-detects platform and dependencies

## Build System Details

### Automatic Actor Registration
The build system automatically generates `engine/Entities.h` from all headers in `entities/` directory using pure Makefile commands:
- Add new entity `.h` files to `entities/` 
- Run `make` - the Makefile generates `engine/Actors.h` automatically with:
  - Timestamp of generation
  - Total actor count
  - Proper include statements
  - Completion marker
- No external scripts needed - uses built-in shell commands
- Cross-platform compatible (Windows/Unix)

### Dependency Detection
- GLFW/FreeType detection via pkg-config (Linux/macOS) or fallback paths (Windows)
- OpenGL context requirements: 4.3+ Core Profile (4.6 recommended)
- Windows DLLs bundled in `lib/` for runtime distribution

### Test Compilation
Each test compiles independently against engine objects (excluding main.o):
```makefile
tests/test_simulation: tests/test_simulation.cpp $(filter-out engine/main.o,$(OBJ)) $(GLAD_OBJ)
```

## Code Conventions

### File Organization
- `engine/`: Core engine systems
- `entities/`: Game object implementations
- `assets/`: Game assets (models, textures, configs)
- `docs/`: Design documents and guides
- `tests/`: Unit and integration tests
- `lib/`: Windows runtime dependencies

### Naming Conventions
- **Classes**: PascalCase (e.g., `Spaceship`, `EntityManager`)
- **Methods**: PascalCase (e.g., `UpdateAI()`, `GetPosition()`)
- **Variables**: camelCase with descriptive names
- **Files**: Match class names (e.g., `Spaceship.h`, `Camera.cpp`)

### Memory Management
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use RAII patterns extensively
- ECS components use shared ownership for flexibility

### Error Handling
- Use exceptions sparingly, prefer return codes for game logic
- Assert liberally in debug builds
- Log errors to console/file for debugging

## Common Implementation Patterns

### Actor Creation
```cpp
// 1. Create actor instance
auto spaceship = std::make_unique<Spaceship>();

// 2. Attach to ECS context
ActorContext context{entityManager, entity};
spaceship->AttachContext(context);

// 3. Initialize with specific configuration
spaceship->Initialize(SpaceshipClassType::Fighter, 0);

// 4. Add rendering component
if (auto* draw = spaceship->Context().GetComponent<DrawComponent>()) {
    draw->renderMode = RenderMode::Mesh3D;
    draw->meshHandle = spaceshipMeshId;
}
```

### System Implementation
```cpp
class PhysicsSystem : public System {
public:
    void Update(EntityManager& em, double dt) override {
        em.ForEach<Position, Velocity>(
            [&](Entity e, Position& pos, Velocity& vel) {
                pos.x += vel.vx * dt;
                pos.y += vel.vy * dt;
            }
        );
    }
};
```

### Component Definition
```cpp
struct Position : Component {
    double x = 0.0, y = 0.0, z = 0.0;
};

struct Velocity : Component {
    double vx = 0.0, vy = 0.0, vz = 0.0;
};
```

### Behavior Tree AI
```cpp
// Create behavior tree nodes
auto sequence = std::make_shared<SequenceNode>();
sequence->AddChild(std::make_shared<TargetingNode>());
sequence->AddChild(std::make_shared<ManeuverNode>([](NavigationState& nav) {
    // Custom maneuver logic
}));

// Register with library
BehaviorTreeLibrary::GetInstance().RegisterTree("fighter_ai", sequence);
```

## Rendering Integration

### DrawComponent Setup
Actors become visible by attaching DrawComponent:

```cpp
DrawComponent draw;
draw.renderMode = RenderMode::Mesh3D;    // Mesh3D, Billboard, Sprite2D, Particles
draw.visible = true;
draw.renderLayer = 1;                    // Drawing order
draw.meshHandle = modelId;
draw.materialHandle = materialId;
entityManager->AddComponent<DrawComponent>(entity, draw);
```

### ActorRenderer Usage
```cpp
#include "graphics/ActorRenderer.h"
ActorRenderer renderer;
renderer.Initialize();
// In render loop:
renderer.Render(entityManager, camera);
```

## Testing Guidelines

### Unit Test Structure
Tests follow naming convention: `test_<system>.cpp`
- Place in `tests/` directory
- Link against engine objects but exclude main.o
- Test both success and failure cases

### Integration Testing
- Test actor spawning and ECS integration
- Verify rendering pipeline with headless tests
- Stress test with many entities

## Performance Considerations

### ECS Optimization
- Use EntityManagerV2 for new systems (10-50x faster iteration)
- Prefer archetype-based storage for frequently accessed components
- Minimize component additions/removals during gameplay

### Rendering Performance
- Use billboards for simple objects (projectiles, particles)
- Implement level-of-detail (LOD) for distant objects
- Batch draw calls by render layer

### Memory Management
- Pre-allocate containers when possible
- Use object pools for frequently created/destroyed objects
- Profile memory usage in debug builds

## Debugging Tools

### ECS Inspector
Press `I` in-game to open ECS inspector. Use `[`/`]` to cycle filters, `0` to clear.

### Visual Debugging
- `F8`: Toggle world coordinate axes
- `F9`: Toggle mini axes gizmo
- `B`: Toggle bloom post-processing
- `L`: Toggle HUD overlay

### Logging
- Use `std::cout` for debug output
- Check `build.log` and `test.log` for build/test output
- Capture diagnostics in `artifacts/` directory

## External Dependencies

### Required Libraries
- **GLFW 3.x**: Windowing and input
- **OpenGL 4.3+**: Rendering (4.6 recommended)
- **GLAD**: OpenGL function loading
- **FreeType**: Font rendering (optional)
- **GLU**: OpenGL utilities

### Asset Pipeline
- **SVG Assets**: Converted to textures via `scripts/package_svg_fonts.py`
- **Ship Models**: JSON-based configuration in `assets/ships/`
- **Component Definitions**: Modular ship parts in `assets/components/`

## Common Pitfalls

1. **Entity Registration**: Always add new entity headers to `entities/` directory - build system auto-generates includes
2. **ECS Component Access**: Check `IsAlive(entity)` before accessing components
3. **OpenGL Context**: Ensure valid context before rendering operations
4. **Cross-Platform Paths**: Use forward slashes in code, let build system handle conversion
5. **Memory Ownership**: Understand shared ownership semantics in ECS

## Getting Started

1. **Clone and Setup**: Follow README.md for platform-specific setup
2. **Build**: `make` to compile, `make test` for test suite
3. **Run**: `./nova-engine` (Linux) or `scripts\run_engine.bat` (Windows)
4. **Controls**: WASD + mouse for movement, `Q` to quit, `I` for ECS inspector
5. **Documentation**: Check `docs/` for detailed system guides

## Architecture Evolution

The engine is migrating from legacy ECS to modern archetype-based system:
- **Legacy**: `EntityManager` with map-based storage
- **Modern**: `EntityManagerV2` with contiguous arrays (10-50x faster)
- **Migration**: New systems use V2, legacy code remains compatible

**File Organization**: Related actor functionality is consolidated:
- `SpaceshipSpawn.cpp` merged into `Spaceship.cpp` for better cohesion
- Actor spawning logic centralized with spaceship catalog functionality