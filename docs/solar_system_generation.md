# Solar System Generation Design Document

## Overview
This document outlines the design and implementation of the procedural solar system generation system for Nova-Engine. The system will create realistic, varied solar systems with planets, moons, asteroids, and space stations using procedural generation techniques. A high-level gameplay summary of these features lives in `GAME_DESIGN_DOCUMENT.md` under the **Solar System Generation** section to keep the narrative-facing documentation aligned with this technical plan.

## Architecture

### Core Components

#### 1. CelestialBody (Base Structure)
Represents any celestial object in the solar system.

**Properties:**
- Unique ID
- Name (procedurally generated)
- Type (Star, Planet, Moon, Asteroid, Station)
- Physical properties (mass, radius, density)
- Orbital parameters (semi-major axis, eccentricity, inclination, etc.)
- Visual properties (color, texture ID, shader parameters)
- Position and velocity

#### 2. SolarSystem (Container)
Manages all celestial bodies in a system.

**Responsibilities:**
- Generate procedural solar system from seed
- Manage celestial body lifecycle
- Update orbital mechanics
- Provide spatial queries (nearest body, bodies in range)
- Serialize/deserialize system state

#### 3. OrbitMechanics (Physics Engine)
Calculates orbital positions and movements.

**Features:**
- Keplerian orbital elements
- Real-time position calculation
- Multi-body gravitational influence (optional)
- Time acceleration support
- Orbital path visualization

#### 4. CelestialBodyGenerator (Procedural Generation)
Creates celestial bodies with realistic parameters.

**Generation Rules:**
- Star classification (spectral type O, B, A, F, G, K, M)
- Planet distribution based on star type
- Moon generation for gas giants and larger terrestrial planets
- Asteroid belt placement
- Space station placement near habitable zones

### ECS Integration

#### Components

```cpp
struct CelestialBodyComponent : public Component {
    enum class BodyType {
        Star,
        RockyPlanet,
        GasGiant,
        IceGiant,
        Moon,
        Asteroid,
        SpaceStation,
        AsteroidBelt
    };
    
    BodyType type;
    std::string name;
    double mass;           // kg
    double radius;         // km
    double rotationPeriod; // hours
    double temperature;    // Kelvin
    bool hasAtmosphere;
    double atmosphereDensity;
};

struct OrbitalComponent : public Component {
    Entity parentBody;     // What this orbits (0 = star/fixed)
    double semiMajorAxis;  // AU or km
    double eccentricity;   // 0-1
    double inclination;    // degrees
    double longitudeOfAscendingNode; // degrees
    double argumentOfPeriapsis;      // degrees
    double meanAnomalyAtEpoch;       // degrees
    double orbitalPeriod;  // days
    double currentAnomaly; // current position in orbit
};

struct VisualCelestialComponent : public Component {
    int textureHandle;
    float colorR, colorG, colorB;
    float emissive;        // For stars
    float specular;        // For planets with water
    float cloudCoverage;   // 0-1
    int ringTextureHandle; // For planets with rings (-1 if none)
    float ringInnerRadius;
    float ringOuterRadius;
};

struct AtmosphereComponent : public Component {
    float density;
    float height;          // km
    float colorR, colorG, colorB, colorA;
    bool hasWeather;
    float cloudSpeed;
};

struct SpaceStationComponent : public Component {
    enum class StationType {
        Trading,
        Military,
        Research,
        Mining,
        Residential,
        Shipyard
    };
    
    StationType stationType;
    int dockingPorts;
    bool hasShipyard;
    bool hasRepairFacility;
    std::vector<int> availableServices;
};
```

### Implementation Parity Snapshot (October 2025)

The design below mirrors the live generator in `src/SolarSystemGenerator.cpp` and the data surfaces declared in `src/CelestialBody.h`.

- **Seed Derivation:** `SolarSystemGenerator::SetSeed` fans out the base seed into deterministic category seeds (`GenerationSeeds`) for stars, planets, moons, asteroid belts, stations, and naming. Every helper (`CreateRng`, `GetSeed`) resolves back to these derived values, so consuming RNG in one subsystem cannot desynchronise the others. When documenting balancing changes, reference the category salts in code (`'STAR'`, `'PLAN'`, `'MOON'`, `'ASTR'`, `'STAT'`, `'NAME'`) to keep telemetry tooling aligned.
- **Generation Parameters:** Tunables live in `GenerationParameters` (declared in `src/CelestialBody.h`). They gate planet counts, moon likelihoods, belt probability, and station density (`minStations`/`maxStations`, `stationNearHabitableProbability`). Any new rule described here should call out the corresponding field so gameplay engineers know which knob to expose.
- **Entity Surfaces:** Each celestial entity carries the component set shown above plus the shared ECS components (`Faction`, `Name`, `Transform`). Runtime systems (navigation, missions, economy) must consume these rather than introducing parallel metadata.

#### Generation Flow Overview

1. **System Naming** – `GenerateSystem` uses the dedicated name seed to pick a prefix (`"Kepler"`, `"Nova"`, etc.) and a three-digit suffix. Subsequent body names are derived via `SolarSystemGenerator::GenerateName`, which applies Roman numerals for planet order and alphabetical suffixes for moons, ensuring reproducible localisation keys.
2. **Star Creation** – `GenerateStar` samples spectral type, luminosity, and colour from the star RNG. The resulting `StarComponent` feeds habitable zone bounds via `CalculateHabitableZone` and drives later placement heuristics.
3. **Planet Pass** – `GeneratePlanets` iterates between `minPlanets`/`maxPlanets`, spacing orbits using the default inner orbit (0.35 AU) and spacing coefficient (1.65). Planet composition choices (rocky, gas, ice) influence follow-up component flags (`hasAtmosphere`, `PlanetComponent` resource fields) and moon quotas.
4. **Satellite Pass** – When `GenerationParameters::generateMoons` is true, the moon RNG spawns child entities through `GenerateMoons`, using the parent mass and hill sphere checks in `SolarSystemGenerator.cpp` to clamp orbital radii. Resulting moons inherit naming from their parent planet.
5. **Belts and Stations** – Asteroid belts derive their radial placement from gaps in planet orbits and update the owning planet's `SatelliteSystemComponent`. Stations are then seeded with the station RNG, respecting `minStations`/`maxStations` and the habitable zone weighting slider. Each station copies faction ownership, service flags, and `SpaceStationComponent` metadata into the ECS so downstream systems (missions, economy) can query them directly.

#### Component Implementation Reference

To keep the design document aligned with the live implementation (`src/CelestialBody.h`), this section captures the exact data surfaces available to systems that consume solar-system entities.

| Component | Key Fields | Notes |
|-----------|------------|-------|
| **CelestialBodyComponent** | `type`, `mass`, `radius`, `rotationPeriod`, `axialTilt`, `temperature`, `hasAtmosphere`, `hasRings`, `hasMagneticField`, `isHabitable`, `isLandable`, `isDockable`, `faction` | Base metadata shared by all bodies. `faction` mirrors the ECS `Faction` component for ownership and conflict simulation. |
| **OrbitalComponent** | `parentEntity`, `semiMajorAxis`, `eccentricity`, `inclination`, `longitudeOfAscendingNode`, `argumentOfPeriapsis`, `meanAnomalyAtEpoch`, `orbitalPeriod`, `currentMeanAnomaly`, `cachedPosition`, `cachedVelocity` | Encodes Keplerian elements. Systems cache Cartesian state to avoid recomputation every frame. |
| **VisualCelestialComponent** | `textureHandle`, `normalMapHandle`, `cloudTextureHandle`, `color*`, `emissive`, `specular`, `roughness`, `metallic`, `cloudCoverage`, `ringTextureHandle`, `ringInnerRadius`, `ringOuterRadius`, `ringOpacity`, `lodDistance*`, shader string handles | Rendering pipeline hooks for every LOD tier and shader override slot. |
| **AtmosphereComponent** | `density`, `scaleHeight`, `pressure`, color channels, `oxygenRatio`, `nitrogenRatio`, `carbonDioxideRatio`, `hasWeather`, `cloudSpeed`, `weatherIntensity` | Drives scattering shaders and gameplay constraints (landing, survival). |
| **SpaceStationComponent** | `stationType`, `dockingPorts`, `hasShipyard`, `hasRepairFacility`, `hasRefuelStation`, `hasMarket`, `population`, `maxPopulation`, `availableServices`, `wealthLevel` | Supports trade, mission, and progression systems. Population scales service availability. |
| **SatelliteSystemComponent** | `satelliteEntities`, `moonCount`, `stationCount` | Maintains relationships between parent bodies and satellites for efficient lookups. |
| **StarComponent** | `spectralType`, `spectralSubclass`, `luminosity`, `surfaceTemperature`, `habitableZoneInner`, `habitableZoneOuter`, `coronaSize`, `hasFlares`, `flareIntensity` | Couples generation rules with rendering/lighting. |
| **AsteroidBeltComponent** | `innerRadius`, `outerRadius`, `thickness`, `density`, `composition`, `asteroidCount`, `resourceRichness` | Input for mining, resource spawning, and navigation hazard simulation. |
| **PlanetComponent** | `isTectonicallyActive`, `hasVolcanism`, `hasOceans`, `oceanCoverage`, `hasIceCaps`, `iceCoverage`, `hasLife`, `hasIntelligentLife`, `biodiversityIndex`, `mineralWealth`, `organicResources`, `gravity`, `radiationLevel` | Differentiates terrestrial worlds and feeds mission/crafting hooks. |

All new systems that rely on procedural data should query these components directly instead of introducing parallel metadata structures.

## Procedural Generation Algorithm

### Phase 1: Star Generation

```
1. Generate star from seed
   - Determine spectral type (weighted random: G and K most common)
   - Calculate mass, radius, temperature, luminosity
   - Set visual properties (color, emissive value)
   
2. Calculate habitable zone
   - Inner boundary: sqrt(L/1.1) AU
   - Outer boundary: sqrt(L/0.53) AU
   (where L is luminosity relative to Sun)
```

### Phase 2: Planet Generation

```
1. Determine number of planets (3-12 based on star type)

2. For each planet:
   a. Generate orbital distance (exponential distribution)
      - Inner rocky zone: 0.3 - 2 AU
      - Middle zone: 2 - 5 AU
      - Outer gas giant zone: 5 - 30 AU
      - Far ice zone: 30 - 50 AU
   
   b. Determine planet type based on distance and star type
      - Close to star: Rocky (terrestrial)
      - Mid zone: Mix of rocky and gas giants
      - Far zone: Gas and ice giants
   
   c. Generate physical properties
      - Mass: Based on type and variation
      - Radius: From mass-radius relationship
      - Rotation period: Random with slower rotation for larger planets
      - Temperature: Based on distance from star
   
   d. Generate orbital parameters
      - Eccentricity: 0.0 - 0.2 (most orbits nearly circular)
      - Inclination: 0.0 - 7.0 degrees (most in same plane)
      - Other angles: Random 0-360
   
   e. Determine if planet has rings (20% chance for gas giants)
   
   f. Generate atmosphere if applicable
      - Rocky planets in habitable zone: 70% chance
      - Gas/ice giants: Always have atmosphere
```

### Phase 3: Moon Generation

```
1. For each planet (except small rocky planets):
   a. Determine number of moons
      - Gas giants: 3-20 moons
      - Rocky planets: 0-2 moons (50% chance of having any)
      - Ice giants: 2-10 moons
   
   b. For each moon:
      - Generate orbital distance (Hill sphere calculation)
      - Size: 0.1 - 0.5 of parent planet radius
      - Orbital period: Based on distance and parent mass
      - Tidal locking: Most moons are tidally locked
```

### Phase 4: Asteroid Belt Generation

```
1. Determine asteroid belt locations (0-2 belts)
   - Typically between rocky planets and gas giants
   - Or in outer system

2. For each belt:
   - Inner radius and outer radius
   - Approximate number of trackable asteroids
   - Density distribution (sparse/moderate/dense)
   
3. Generate individual asteroids (for gameplay)
   - Position within belt (random)
   - Size: Power-law distribution (many small, few large)
   - Composition type (metallic, rocky, icy)
```

### Phase 5: Space Station Generation

```
1. Identify strategic locations
   - Planets in habitable zone
   - Resource-rich asteroid belts
   - Gas giants (for fuel)
   - Lagrange points

2. Generate stations (2-8 per system)
   - Type based on location:
     * Trading hubs near habitable planets
     * Mining stations near asteroid belts
     * Research stations in unique locations
     * Military stations at system periphery
   
   - Orbital parameters similar to moons
   - Services and facilities based on type
```

## Socio-Economic Simulation Layers

### Faction Framework

Every generated system integrates with the core ECS `Faction` component (`src/ecs/Components.h`) and the `CelestialBodyComponent::faction` field to model territorial control, docking permissions, and conflict triggers. The baseline campaign ships with the following primary factions:

| Faction | Identity | Typical Presence | Gameplay Hooks |
|---------|----------|------------------|----------------|
| **Auroran Combine** | Technocratic trade league prioritizing infrastructure and neutral markets. | Habitable-zone trade hubs and high-traffic jump points. | Controls most **Trading** stations; offers escort and supply contracts; low aggression thresholds. |
| **Helios Dominion** | Militaristic successor state defending old core worlds. | Inner-system military bastions and fortified moons. | Provides bounty missions, navy-grade ship variants, and enforces strict docking clearance. |
| **Orion Prospectors Guild** | Decentralized mining consortium with strong union presence. | Dense asteroid belts and outer-system refineries. | Unlocks mining licenses, bulk ore contracts, and specialized extraction modules. |
| **Voidbound Syndicate** | Smuggler coalition thriving in unregulated sectors. | Shadow ports near Lagrange points and remote stations. | Offers black-market modules, sabotage missions, and dynamic contraband pricing. |
| **Lumen Research Collective** | Academic alliance seeking stellar anomalies. | Research stations near exotic phenomena and flare-prone stars. | Grants access to experimental tech trees and exploration quests; reputation gates scientific modules. |
| **Frontier Wardens** | Volunteer defense militia protecting emerging colonies. | Sparse outer colonies and ice-giant moons. | Supplies defensive turrets, patrol missions, and civilian evacuation events. |

Factions dictate procedural naming templates, encounter tables, and spawn weighting for fleets. During generation, each station or habitable body is assigned a controlling faction based on location heuristics (e.g., mining belts skew toward the Prospectors Guild). Reputation changes are stored per entity, enabling mission scripting to respond to player actions without additional lookups.

### Resource and Crafting Loops

The procedural data also seeds the economy layer, aligning with `PlanetComponent`, `AsteroidBeltComponent`, and station metadata.

1. **Resource Categories**
   - **Metallic Ores:** Derived from asteroid belts with `composition = Metallic` or planets with `mineralWealth > 0.6`. Feeds hull fabrication and weapon crafting.
   - **Volatiles & Fuels:** Generated by icy bodies (`composition = Icy`, `organicResources > 0.4`) and gas giants with high atmospheric density. Used for propulsion consumables and reactor maintenance.
   - **Organics & Biomass:** Available on habitable or ocean worlds (`hasLife`, `organicResources > 0.5`). Enables medical supplies, foodstuffs, and bio-upgrade crafting.
   - **Exotics:** Tagged on anomaly-rich locations (flare-heavy stars, `radiationLevel > 0.6`). Required for advanced shield matrices and research unlocks.

2. **Extraction Gameplay**
   - Asteroid belts expose discrete mining nodes scaled by `resourceRichness`. Stations owned by the Prospectors Guild provide refining bonuses and unique blueprints.
   - Planetary landing sites check `isLandable`, atmospheric density, and gravity to determine viable extraction rigs and required gear.

3. **Crafting Pipelines**
   - Crafting recipes consume categorized resources and station services. Shipyards with `hasShipyard = true` fabricate hull frames; research stations unlock prototype components when the player delivers exotics plus faction-specific requisitions.
   - Wealthier stations (`wealthLevel >= 3`) maintain broader blueprint inventories and reduce crafting time. Population metrics gate production queues and restocking rates.

4. **Dynamic Economy Hooks**
   - Faction conflicts adjust supply/demand curves system-wide. Military blockades by the Helios Dominion increase fuel prices; Syndicate smuggling bypasses shortages at the risk of patrol encounters.
   - Procedural events (e.g., flare activity from `StarComponent::hasFlares`) trigger temporary modifiers such as research surges or evacuation missions, encouraging players to harvest resources under time pressure.

These layers ensure each generated system feeds mission design, ship progression, and player-driven crafting without diverging from the existing ECS surface area.

## Orbital Mechanics Implementation

### Keplerian Orbital Elements

The system uses classical orbital elements for efficient calculation:

```cpp
// Calculate position from orbital elements at time t
Vector3 CalculateOrbitalPosition(OrbitalComponent& orbit, double time) {
    // 1. Calculate mean anomaly
    double M = orbit.meanAnomalyAtEpoch + 
               (2.0 * PI / orbit.orbitalPeriod) * time;
    
    // 2. Solve Kepler's equation for eccentric anomaly E
    double E = SolveKeplersEquation(M, orbit.eccentricity);
    
    // 3. Calculate true anomaly
    double nu = 2.0 * atan2(
        sqrt(1.0 + orbit.eccentricity) * sin(E / 2.0),
        sqrt(1.0 - orbit.eccentricity) * cos(E / 2.0)
    );
    
    // 4. Calculate distance
    double r = orbit.semiMajorAxis * (1.0 - orbit.eccentricity * cos(E));
    
    // 5. Transform to 3D space
    return TransformToCartesian(r, nu, orbit);
}
```

### Performance Optimizations

1. **Update frequency tiering:**
   - Stars: Static (no updates)
   - Planets: Update every frame (visible objects)
   - Moons: Update every 2-5 frames
   - Distant asteroids: Update every 10-30 frames
   - Off-screen objects: Update every 60 frames

2. **Level of Detail (LOD):**
   - Near camera (< 100 units): Full detail, accurate orbits
   - Medium distance (100-1000 units): Simplified rendering
   - Far distance (> 1000 units): Point lights/icons only

3. **Spatial partitioning:**
   - Octree or grid-based system for quick spatial queries
   - Only update objects in active sectors

## Visual Representation

### Rendering Strategy

1. **Stars:** 
   - Emissive sphere with corona shader
   - Lens flare effects
   - Light source for entire system

2. **Planets:**
   - Textured sphere with normal maps
   - Atmosphere shader (edge glow)
   - Cloud layer for applicable planets
   - Rings rendered as transparent textured quads

3. **Moons:**
   - Simpler sphere rendering
   - Basic texture and lighting

4. **Asteroids:**
   - Low-poly models for near objects
   - Instanced rendering for distant asteroids
   - Imposters for very distant objects

5. **Space Stations:**
   - Detailed 3D models (LOD system)
   - Animated docking lights
   - Marker/icon when distant

6. **Orbital Paths:**
   - Optional visualization (toggle)
   - Dotted line for predicted path
   - Color-coded by body type

## Data Management

### Serialization Format

```json
{
  "seed": 1234567890,
  "version": 1,
  "star": {
    "spectralType": "G2V",
    "mass": 1.0,
    "radius": 696340,
    "temperature": 5778,
    "luminosity": 1.0
  },
  "planets": [
    {
      "id": 1,
      "name": "Kepler-Alpha-I",
      "type": "RockyPlanet",
      "mass": 5.972e24,
      "radius": 6371,
      "orbit": {
        "semiMajorAxis": 1.0,
        "eccentricity": 0.017,
        "inclination": 0.0,
        "period": 365.25
      },
      "moons": [...]
    }
  ],
  "asteroidBelts": [...],
  "spaceStations": [...]
}
```

### File Structure

```
data/
  systems/
    system_<seed>.json        - Complete system data
    system_<seed>.bin         - Binary format for fast loading
  
  assets/
    textures/
      celestial/
        star_*.png
        planet_*.png
        moon_*.png
        asteroid_*.png
        station_*.png
      
    models/
      station_*.obj
      asteroid_*.obj
```

## Integration with Existing Systems

### Camera System
- Follow celestial bodies
- Smooth camera transitions between bodies
- Support for cinematic planet flybys

### Player System
- Navigate between celestial bodies
- Landing mechanics for planets/moons/stations
- Gravity wells affecting ship physics

### UI/HUD
- System map display
- Selected body information panel
- Distance and ETA calculations
- Navigation markers

## Development Phases

### Phase 1: Core Framework (Week 1-2)
- [ ] Create CelestialBody base structures
- [ ] Implement SolarSystem container class
- [ ] Basic orbital mechanics (circular orbits)
- [ ] Simple visualization (colored spheres)

### Phase 2: Procedural Generation (Week 3-4)
- [ ] Star generation algorithm
- [ ] Planet generation with types
- [ ] Moon generation
- [ ] Asteroid belt creation
- [ ] Seed-based reproducibility

### Phase 3: Advanced Orbits (Week 5)
- [ ] Keplerian orbital elements
- [ ] Elliptical orbit support
- [ ] Orbital plane inclination
- [ ] Time acceleration support

### Phase 4: Visual Polish (Week 6-7)
- [ ] Textured planets
- [ ] Atmosphere rendering
- [ ] Planetary rings
- [ ] Improved star rendering
- [ ] Orbital path visualization

### Phase 5: Space Stations (Week 8)
- [ ] Station generation algorithm
- [ ] Station models/textures
- [ ] Strategic placement logic
- [ ] Service/facility system

### Phase 6: Performance & Optimization (Week 9-10)
- [ ] LOD system implementation
- [ ] Update frequency optimization
- [ ] Spatial partitioning
- [ ] Culling systems

### Phase 7: Integration (Week 11-12)
- [ ] Player navigation between bodies
- [ ] Landing mechanics
- [ ] Gravity well physics
- [ ] UI integration (system map)
- [ ] Save/load functionality

## Testing Strategy

### Unit Tests
- Orbital mechanics calculations
- Procedural generation reproducibility
- Serialization/deserialization

### Integration Tests
- System generation with various seeds
- Performance with large systems (10+ planets, 50+ moons)
- Camera transitions between bodies

### Visual Tests
- Screenshot comparison for consistency
- Orbital path accuracy
- Rendering at different distances

## Future Enhancements

1. **Advanced Features:**
   - Binary star systems
   - Rogue planets (no orbit)
   - Comets with elliptical orbits
   - Black holes and neutron stars
   - Nebulae and gas clouds

2. **Gameplay Features:**
   - Resource scanning and mining
   - Planetary colonization
   - Terraforming mechanics
   - Dynamic events (asteroid impacts, solar flares)

3. **Visual Improvements:**
   - Volumetric atmospheres
   - Realistic planetary surfaces with height maps
   - Weather systems on planets
   - Advanced star shaders (sunspots, flares)

4. **Multiplayer:**
   - Synchronized solar system state
   - Player-created structures (stations, outposts)
   - Territory control mechanics

## References

- Kepler's Laws of Planetary Motion
- NASA Exoplanet Archive (for realistic parameters)
- Elite Dangerous stellar forge system
- No Man's Sky procedural generation
- Space Engine astronomical accuracy

## Conclusion

This solar system generation system provides a solid foundation for creating varied, realistic space environments. The modular design allows for incremental development while maintaining compatibility with the existing Nova-Engine architecture. The procedural approach ensures infinite variety while maintaining scientific plausibility.
