# Nova Engine - AI Coding Assistant Guidelines

## Project Overview
Nova Engine is a 3D C++17 game engine focused on space simulation with physically-inspired flight controls, modular spacecraft assembly, and a data-driven ECS architecture. Key features include OpenGL rendering, GLFW windowing, entity-component systems, and actor-based game objects.

## Architecture Fundamentals

### Core Systems
- **Engine Core** (`engine/`): MainLoop, Viewport3D, Input, Simulation - handles game loop, rendering, and input
- **ECS System** (`engine/ecs/`): Dual ECS implementation (legacy EntityManager + modern EntityManagerV2 with archetypes)
  - Modern V2 uses contiguous arrays for 10-50x faster iteration with 70% less memory
  - Legacy system uses map-based storage with shared_ptr components (compatibility layer)
  - **NEW:** Advanced Query System with `QueryBuilder`, parallel execution, caching, and composition
- **Actor System** (`entities/`): Game objects (Spaceship, NPC, Station, Projectile) via `IActor` interface
  - **CRITICAL:** All actors use `ActorContext` to bridge entity and ECS manager
  - Include order matters: `EntityManager.h` MUST come before `ActorContext.h`
  - Actor registration is automatic via `engine/Entities.h` generation from `entities/*.h` files
- **Graphics Pipeline** (`engine/graphics/`): Multiple renderer implementations
  - `EntityRenderer` and `ActorRenderer` both exist (legacy migration in progress)
  - Support for Mesh3D, Billboard, Sprite2D, Particles render modes
- **Physics** (`engine/physics/`): Movement, collision, and physics simulation systems
- **Content & Configuration** (`engine/content/`, `engine/config/`, `assets/`): Data-driven architecture
  - JSON-based actor, ship, and component configurations with schema validation
  - `ConfigSystem` with hot-reloading, type-safe access, and real-time validation
  - `ActorFactorySystem` for automatic registration, creation metrics, and template instantiation
  - Asset pipeline with streaming, hot-reload, versioning, and compression

### Key Design Patterns

#### Actor Registration & Include Order
Actors use automatic registration via build system. Add new entity headers to `entities/` directory - the build system auto-generates `engine/Entities.h`:

```cpp
// In entities/YourNewEntity.h
#include "../engine/EntityCommon.h"  // CRITICAL: Gets correct include order

class YourNewActor : public IActor {
    // IActor interface
    void Initialize() override {
        // Access ECS through context_
        if (auto* em = context_.GetEntityManager()) {
            em->AddComponent<DrawComponent>(context_.GetEntity(), draw);
        }
    }
    void Update(double deltaTime) override { /* ... */ }
    std::string GetName() const override { return "YourActor"; }
};
```

**Include order is critical:** `engine/EntityCommon.h` ensures `EntityManager.h` comes before `ActorContext.h` to avoid incomplete type errors. Always use this pattern for actor headers.

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

#### Advanced Query System
Use QueryBuilder for efficient entity queries with caching and parallel execution:

```cpp
// Basic query
auto query = QueryBuilder(entityManager).With<Position, Velocity>();
query.ForEach([](EntityHandle e, Position& pos, Velocity& vel) {
    pos.x += vel.vx * deltaTime;
});

// Parallel execution for large datasets
query.ForEachParallel([](EntityHandle e, Position& pos, Velocity& vel) {
    // Automatically batched across available CPU cores
});

// Complex filtering with predicates
query.Where([](EntityHandle e, Position& pos) { return pos.x > 0; })
     .Take(100)  // Limit results
     .ForEach([](EntityHandle e, Position& pos) { /* process */ });
```

#### Spaceship Assembly System
Spaceships use modular component assembly with JSON configuration:

```cpp
// Load spaceship from JSON manifest
auto spaceship = SpaceshipSpawnBundles::CreateSpaceshipEntity(
    entityManager, definition, loadout, x, y, z
);
```

## Data-Driven Architecture

### JSON Configuration System
Nova Engine uses a comprehensive JSON-based configuration system with schema validation:

**ConfigSystem** (`engine/ConfigSystem.h/cpp`):
```cpp
// Load config with schema validation
auto& configSystem = ConfigSystem::GetInstance();
auto config = configSystem.LoadConfig("ship", "assets/config/ships/fighter.json");
double speed = config->Get<double>("maxSpeed", 100.0);

// Hot-reload support (watches for file changes)
configSystem.EnableHotReload();
```

**Schema Registry** (`engine/JsonSchema.h/cpp`):
- Define schemas programmatically or load from JSON files
- Type validation (string, number, boolean, array, object)
- Constraints (min/max, required fields, enums, string length)
- Nested object and array validation
- Custom validation functions

**Actor Configuration** (`entities/ActorConfig.h`):
```cpp
// Load actor config with validation
auto result = ActorConfig::LoadFromFileWithValidation(
    "assets/actors/station.json", 
    "station_config"  // schema ID
);
if (result.success) {
    double health = ActorConfig::GetValue(*result.config, "health", 100.0);
}
```

### Content Management
**ActorFactorySystem** (`engine/ActorFactorySystem.h/cpp`):
- Automatic actor registration via `REGISTER_ACTOR` macro
- Factory validation and dependency tracking
- Performance metrics (creation time, usage statistics)
- Template instantiation with parameters
- Category organization (combat, world, default)

**Asset Pipeline** (`engine/AssetPipeline.h`, `engine/AssetStreamer.h`):
- Asset streaming with priority queues
- Hot-reload during development
- Versioning and compatibility checks
- Compression (LZ4, Zstandard)
- Asset database with metadata tracking (`assets/asset_database.json`)

**Bootstrap Configuration** (`assets/bootstrap.json`):
- Framework loading (input, audio, rendering)
- Deployment mode configuration
- Runtime feature flags

### Schema Validation Workflow
```cpp
// 1. Register schema (do once at startup)
schema::SchemaRegistry::Instance().LoadSchemaFromFile(
    "actor_config", 
    "assets/schemas/actor_config.schema.json"
);

// 2. Validate configuration
auto validation = schema::SchemaRegistry::Instance().ValidateFile(
    "actor_config",
    "assets/actors/my_actor.json"
);

// 3. Check validation results
if (!validation.success) {
    for (const auto& error : validation.errors) {
        std::cerr << error.path << ": " << error.message << "\n";
    }
}
```

**Schema directories:**
- `assets/schemas/` - JSON schema definitions
- `assets/actors/` - Actor configurations
- `assets/config/` - System configurations
- `assets/content/` - Game content definitions

## Development Workflow

### Building
**Primary build modes** (use PowerShell scripts for fastest results on Windows):
```powershell
.\build_fast_dev.ps1     # Fast dev builds (O1 optimization, 4-6x faster than release)
.\build_unity.ps1        # Unity builds (combines sources, 10x faster for clean builds)
.\build_debug.ps1        # Debug builds with symbols (g, O0)
mingw32-make             # Standard release build (O3 optimization)
```

**Direct Make commands:**
```bash
mingw32-make -j8 BUILD_MODE=fast    # Fast mode (-O1)
mingw32-make -j8 UNITY_BUILD=1      # Unity build
mingw32-make -j8 BUILD_MODE=debug   # Debug mode
mingw32-make test                   # Build all test executables
mingw32-make clean                  # Remove all build artifacts
```

**VS Code tasks available:**
- `Make: fast` - Fast incremental builds
- `Make: fast-run` - Fast build + launch
- `Make: run` - Standard build + launch

**Incremental rebuilds:** To force recompilation of specific files:
```powershell
Remove-Item engine/MainLoop.o -ErrorAction SilentlyContinue; mingw32-make nova-engine
```

**Common build issues on Windows:**
- Use `mingw32-make` instead of `make` 
- Ensure MSYS2 bin directory is in PATH: `C:\msys64\mingw64\bin`
- Run `scripts\check_dlls.ps1` to copy required DLLs before building
- If actor registration fails, delete `engine/Entities.h` and rebuild

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

**Required DLLs for runtime** (auto-copied by script):
- `glfw3.dll` - GLFW windowing
- `libfreeglut.dll` - OpenGL utilities
- `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll` - MinGW runtime
- `xinput1_4.dll` - Optional gamepad support

**If build fails with missing DLLs:** Set `$env:MSYS2_MINGW64_BIN` to your MSYS2 bin path (default: `C:\msys64\mingw64\bin`).

### Cross-Platform Considerations
- **Linux**: Uses pkg-config for GLFW/FreeType detection
- **Windows**: MSYS2/MinGW with bundled DLLs in `lib/`
- **Build System**: Makefile auto-detects platform and dependencies

## Build System Details

### Automatic Actor Registration
The build system automatically generates `engine/Entities.h` from all headers in `entities/` directory using pure Makefile commands:
- Add new entity `.h` files to `entities/` 
- Run `make` - the Makefile generates `engine/Entities.h` automatically with:
  - Timestamp of generation
  - Total actor count
  - Proper include statements
  - Completion marker
- No external scripts needed - uses built-in shell commands
- Cross-platform compatible (Windows/Unix)

**How it works (Windows PowerShell):**
```makefile
engine/Entities.h: $(ACTOR_HEADERS)
    @echo #pragma once > $@
    @echo // Auto-generated - includes all entity headers for registration >> $@
    @for %%f in (entities\*.h) do @echo #include "../entities/%%~nxf" >> $@
```

**Important:** Delete `engine/Entities.h` and rebuild if actor registration errors occur.

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
draw.mode = DrawComponent::RenderMode::Mesh3D;  // Mesh3D, Billboard, Sprite2D, Particles
draw.visible = true;
draw.renderLayer = 1;                           // Drawing order
draw.meshHandle = modelId;
draw.materialHandle = materialId;
draw.SetTint(r, g, b);                          // Optional color tinting
entityManager->AddComponent<DrawComponent>(entity, draw);
```

**RenderMode Options:**
- `Mesh3D` - Full 3D mesh rendering (currently fallback: colored cube)
- `Billboard` - Always faces camera (for particles, projectiles)
- `Sprite2D` - 2D sprite in screen space
- `Particles` - Particle system rendering
- `Wireframe` - Debug wireframe view
- `Custom` - User-defined `customRenderCallback`

### EntityRenderer vs ActorRenderer
**Current state:** Both exist during migration
- `EntityRenderer` - New implementation (header-only, inline methods)
- `ActorRenderer` - Legacy implementation (similar API)
- Both use `ForEach<DrawComponent, Position>` pattern for rendering

```cpp
// Modern approach (V2 ECS)
EntityRenderer renderer;
renderer.Initialize();
renderer.Render(entityManagerV2, camera);

// Legacy approach
ActorRenderer renderer;
renderer.Initialize();
renderer.RenderLegacy(entityManager, camera);
```

**Implementation note:** Both renderers currently use OpenGL immediate mode (legacy) as a fallback when no mesh is loaded. Modern shader-based rendering is TODO.

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
2. **Include Order for Actors**: Use `#include "../engine/EntityCommon.h"` as the first include in actor headers to avoid incomplete type errors with `ActorContext`
3. **ECS Component Access**: Check `IsAlive(entity)` before accessing components
4. **OpenGL Context**: Ensure valid context before rendering operations
5. **Cross-Platform Paths**: Use forward slashes in code, let build system handle conversion
6. **Memory Ownership**: Understand shared ownership semantics in legacy ECS (shared_ptr), value semantics in V2
7. **Incremental Compilation**: When rebuild fails, delete specific `.o` files to force recompilation (especially `MainLoop.o`, `Viewport3D.o`, `Simulation.o`)
8. **Renderer Selection**: Use `EntityRenderer` for V2 ECS, `ActorRenderer` for legacy. Both have `Render()` and `RenderLegacy()` methods
9. **Windows Build Environment**: Always use `mingw32-make` instead of `make` on Windows; ensure MSYS2 bin is in PATH
10. **DLL Dependencies**: Run `scripts\check_dlls.ps1` before building to copy required runtime DLLs
11. **Query Performance**: Use `QueryBuilder` for complex entity queries instead of nested `ForEach` loops
12. **JSON Configuration**: Always validate configs against schemas before loading; check `validation.success`
13. **Asset Loading**: Use `AssetDatabase` for tracking; avoid direct file I/O in gameplay code
14. **Schema Registration**: Register schemas at startup before loading any configs that use them
15. **Hot-Reload**: Enable `ConfigSystem::EnableHotReload()` only in development builds - has performance cost

## Getting Started

1. **Clone and Setup**: Follow README.md for platform-specific setup
2. **Build**: `mingw32-make` (Windows) or `make` (Linux/macOS) to compile, `make test` for test suite
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

## Key Files Reference

### Build System
- `Makefile`: Cross-platform build system with auto-detection for GLFW/FreeType
- `engine/Entities.h`: Auto-generated from `entities/*.h` files - DO NOT edit manually
- `scripts/check_dlls.ps1`: Windows DLL dependency checker and copier
- `build_fast_dev.ps1`: Fast development builds (O1 optimization)
- `build_unity.ps1`: Unity builds for faster clean compilation
- `build_debug.ps1`: Debug builds with full symbols

### ECS Core Files
- `engine/ecs/EntityManager.h`: Legacy and V2 ECS implementations
- `engine/ecs/QueryBuilder.h`: Advanced query system with parallel execution
- `engine/ecs/ComponentTraits.h`: Type system for components
- `engine/ecs/SystemSchedulerV2.h`: Modern parallel system scheduler

### Actor System
- `entities/*.h`: Actor implementations (auto-registered)
- `engine/EntityCommon.h`: Required first include for actors
- `engine/ActorContext.h`: Bridge between actors and ECS
- `engine/ActorFactorySystem.h`: Factory registration and metrics

### Configuration & Content
- `engine/ConfigSystem.h`: Core configuration loading with hot-reload
- `engine/JsonSchema.h`: JSON schema validation system
- `engine/content/ContentFramework.h`: Unified content management
- `entities/ActorConfig.h`: Actor configuration with validation
- `assets/bootstrap.json`: Framework loading configuration
- `assets/schemas/`: JSON schema definitions
- `assets/actors/`: Actor configuration files

### Asset Pipeline
- `engine/AssetPipeline.h`: Asset processing and versioning
- `engine/AssetStreamer.h`: Asynchronous asset loading
- `engine/AssetDatabase.h`: Asset metadata tracking
- `assets/asset_database.json`: Asset registry and metadata

### Documentation
- `docs/ECS_QUERY_SYSTEM.md`: Complete query system guide
- `docs/engine_overview.md`: Framework architecture overview
- `docs/CONFIG_ARCHITECTURE.md`: Configuration system design
- `docs/CONTENT_ARCHITECTURE.md`: Content management system
- `docs/actor_factory_system.md`: Actor factory documentation
- `BUILD_SPEED_GUIDE.md`: Build optimization strategies