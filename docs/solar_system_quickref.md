# Solar System Generation - Quick Reference

## 📁 File Structure

```
src/
  ├── CelestialBody.h           - Component definitions for all celestial bodies
  ├── CelestialBody.cpp         - Vector3 and basic math implementations
  ├── SolarSystem.h             - Solar system manager and orbital mechanics
  ├── SolarSystem.cpp           - [TO BE CREATED]
  ├── SolarSystemGenerator.h    - Procedural generation engine
  └── SolarSystemGenerator.cpp  - [TO BE CREATED]

docs/
  ├── solar_system_generation.md - Complete design document
  ├── solar_system_tasks.md       - Detailed task breakdown with estimates
  └── solar_system_seed_catalog.md - Curated seeds for reproducible setups
```

## 🎯 Quick Start

### 1. Basic Usage (Once Implemented)

```cpp
#include "SolarSystem.h"
#include "SolarSystemGenerator.h"

// Initialize
SolarSystem solarSystem;
solarSystem.Init(entityManager, "Kepler System");

// Generate procedural system
SolarSystemGenerator generator;
Entity starEntity = generator.GenerateSystem(
    entityManager,
    12345,  // seed
    GenerationParameters()
);
solarSystem.SetStarEntity(starEntity);

// Game loop
void Update(double dt) {
    // Update orbits (with optional time acceleration)
    solarSystem.Update(dt, timeAcceleration);
}
```

### 2. Query Celestial Bodies

```cpp
// Find nearest body
Entity nearest = solarSystem.FindNearestBody(playerPosition);

// Find bodies in radius
auto nearby = solarSystem.FindBodiesInRadius(position, 1000.0);

// Get all planets
const auto& planets = solarSystem.GetPlanets();

// Get moons of a planet
auto moons = solarSystem.GetMoons(planetEntity);
```

### 3. Access Body Information

```cpp
// Get celestial body component
auto* body = entityManager->GetComponent<CelestialBodyComponent>(entity);
if (body) {
    std::cout << "Body: " << body->name << std::endl;
    std::cout << "Type: " << (int)body->type << std::endl;
    std::cout << "Mass: " << body->mass << " kg" << std::endl;
    std::cout << "Radius: " << body->radius << " km" << std::endl;
}

// Get orbital information
auto* orbit = entityManager->GetComponent<OrbitalComponent>(entity);
if (orbit) {
    std::cout << "Semi-major axis: " << orbit->semiMajorAxis << " AU" << std::endl;
    std::cout << "Period: " << orbit->orbitalPeriod << " days" << std::endl;
}

// Get visual properties
auto* visual = entityManager->GetComponent<VisualCelestialComponent>(entity);
if (visual) {
    // Use for rendering
}
```

## 📊 Component Reference

### CelestialBodyComponent
Core properties of any celestial object.

**Key Fields:**
- `type` - Star, RockyPlanet, GasGiant, Moon, Asteroid, SpaceStation
- `name` - Procedurally generated name
- `mass` - Mass in kg
- `radius` - Radius in km
- `temperature` - Surface temperature in Kelvin
- `isLandable`, `isDockable` - Gameplay flags

### OrbitalComponent
Keplerian orbital elements.

**Key Fields:**
- `parentEntity` - What this orbits (0 = star)
- `semiMajorAxis` - Average orbital distance (AU for planets, km for moons)
- `eccentricity` - Orbit shape (0 = circle, < 1 = ellipse)
- `inclination` - Tilt from reference plane (degrees)
- `orbitalPeriod` - Time for one orbit (days)
- `cachedPosition` - Current 3D position (updated by system)

### VisualCelestialComponent
Rendering properties.

**Key Fields:**
- `textureHandle`, `normalMapHandle` - Texture IDs
- `colorR/G/B` - Base color
- `emissive` - Self-illumination (stars)
- `ringTextureHandle`, `ringInnerRadius`, `ringOuterRadius` - Planet rings

### StarComponent
Star-specific data.

**Key Fields:**
- `spectralType` - O, B, A, F, G, K, M
- `luminosity` - Relative to Sun
- `surfaceTemperature` - Kelvin
- `habitableZoneInner/Outer` - AU boundaries

### SpaceStationComponent
Station properties.

**Key Fields:**
- `stationType` - Trading, Military, Research, Mining, etc.
- `dockingPorts` - Number of docking bays
- `hasShipyard`, `hasRepairFacility`, `hasRefuelStation` - Services

## 🔢 Constants & Conversions

```cpp
// Astronomical Unit (Earth-Sun distance)
const double AU = 149597870.7;  // km

// Solar mass
const double SOLAR_MASS = 1.989e30;  // kg

// Earth properties (reference)
const double EARTH_MASS = 5.972e24;    // kg
const double EARTH_RADIUS = 6371.0;    // km
const double EARTH_YEAR = 365.25;      // days

// Convert AU to km
double km = au * AU;

// Convert days to seconds
double seconds = days * 86400.0;
```

## 🎨 Spectral Types

| Type | Color | Temp (K) | Mass (☉) | Examples |
|------|-------|----------|----------|----------|
| O | Blue | 30000+ | 16+ | Rare, very hot |
| B | Blue-white | 10000-30000 | 2.1-16 | Rigel, Spica |
| A | White | 7500-10000 | 1.4-2.1 | Sirius, Vega |
| F | Yellow-white | 6000-7500 | 1.04-1.4 | Procyon |
| G | Yellow | 5200-6000 | 0.8-1.04 | Sun, Alpha Centauri A |
| K | Orange | 3700-5200 | 0.45-0.8 | Epsilon Eridani |
| M | Red | 2400-3700 | 0.08-0.45 | Proxima Centauri |

*Most common: G and K types (like our Sun)*

## 🌍 Planet Types

### Rocky Planets (Terrestrial)
- **Location:** Inner system (0.3-2 AU)
- **Mass:** 0.05 - 2 Earth masses
- **Examples:** Mercury, Venus, Earth, Mars
- **Features:** Solid surface, potential atmospheres, landable

### Gas Giants
- **Location:** Mid-outer system (2-15 AU)
- **Mass:** 50 - 500 Earth masses
- **Examples:** Jupiter, Saturn
- **Features:** No solid surface, many moons, often have rings

### Ice Giants
- **Location:** Outer system (15-30 AU)
- **Mass:** 10 - 20 Earth masses
- **Examples:** Uranus, Neptune
- **Features:** Ice-rich composition, cold temperatures

## 🌙 Moon Generation Rules

| Planet Type | Moon Probability | Count Range |
|-------------|------------------|-------------|
| Small Rocky | 20% | 0-1 |
| Large Rocky | 60% | 0-2 |
| Gas Giant | 95% | 3-20 |
| Ice Giant | 90% | 2-10 |

## ⚙️ Generation Parameters

```cpp
GenerationParameters params;
params.seed = 12345;              // Reproducible generation
params.minPlanets = 3;            // Minimum planets
params.maxPlanets = 10;           // Maximum planets
params.gasGiantProbability = 0.4; // 40% chance per planet
params.asteroidBeltProbability = 0.7; // 70% chance of belt
params.moonProbability = 0.6;     // 60% for rocky planets
params.minStations = 2;           // Minimum space stations
params.maxStations = 8;           // Maximum space stations
```

## 🧮 Orbital Mechanics Formulas

### Kepler's Equation
```
M = E - e * sin(E)
```
Where:
- M = Mean anomaly (radians)
- E = Eccentric anomaly (radians)
- e = Eccentricity

### Orbital Period (Kepler's Third Law)
```
T² = (4π² / GM) * a³
```
Where:
- T = Orbital period
- G = Gravitational constant
- M = Mass of central body
- a = Semi-major axis

### Distance from Focus
```
r = a(1 - e * cos(E))
```

## 🎮 Gameplay Integration Points

### Navigation System
- Use `FindNearestBody()` for auto-pilot
- Calculate ETAs using orbital positions and velocities
- Display distances in appropriate units (km, AU, light-years)

### Landing System
- Check `isLandable` flag
- Use planet radius for approach distance triggers
- Consider atmosphere for entry effects

### Station Interaction
- Check `isDockable` flag
- Query `SpaceStationComponent` for available services
- Use `dockingPorts` for simultaneous ship limit

### Resource System
- Check `PlanetComponent.mineralWealth` for mining value
- Use `AsteroidBeltComponent.resourceRichness`
- Consider `temperature` and `atmosphereDensity` for habitability

## 🐛 Debugging Tips

### Visualize Orbits
```cpp
solarSystem.SetOrbitalVisualizationEnabled(true);
```

### Inspect Generated System
```cpp
std::cout << "System: " << solarSystem.GetSystemName() << std::endl;
std::cout << "Star: " << solarSystem.GetStarEntity() << std::endl;
std::cout << "Planets: " << solarSystem.GetPlanets().size() << std::endl;
std::cout << "Stations: " << solarSystem.GetSpaceStations().size() << std::endl;
```

### Test Reproducibility
```cpp
// Same seed should produce identical systems
Entity star1 = generator.GenerateSystem(em, 12345, params);
Entity star2 = generator.GenerateSystem(em, 12345, params);
// Should have identical planet configurations
```

### Performance Monitoring
```cpp
auto start = std::chrono::high_resolution_clock::now();
solarSystem.Update(dt, timeAcceleration);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Update time: " << duration.count() << " μs" << std::endl;
```

## 📚 Related Documentation

- **Design Document:** `docs/solar_system_generation.md`
- **Task Breakdown:** `docs/solar_system_tasks.md`
- **Game Design:** `GAME_DESIGN_DOCUMENT.md`
- **TODO List:** `TODO_LIST.txt`

## 🚀 Next Steps

1. **Implement Phase 1**: Core framework (SolarSystem.cpp)
2. **Test Orbits**: Verify circular orbit calculations
3. **Implement Phase 2**: Procedural generation (SolarSystemGenerator.cpp)
4. **Generate First System**: Test with various seeds
5. **Polish & Optimize**: Phases 3-6
6. **Integrate**: Connect with player, camera, UI (Phase 7)

---

*For detailed implementation steps, see `docs/solar_system_tasks.md`*
*For design rationale and algorithms, see `docs/solar_system_generation.md`*
