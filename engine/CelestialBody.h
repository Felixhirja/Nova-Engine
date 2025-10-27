#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ecs/Component.h"

// Forward declarations
struct Vector3 {
    double x, y, z;
    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    double Length() const;
    Vector3 Normalized() const;
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(double scalar) const;
    double Dot(const Vector3& other) const;
    Vector3 Cross(const Vector3& other) const;
    double Distance(const Vector3& other) const;
};

// ============================================================================
// ECS Components for Celestial Bodies
// ============================================================================

/**
 * @brief Core properties of any celestial body
 */
struct CelestialBodyComponent : public Component {
    enum class BodyType {
        Star,
        RockyPlanet,      // Mercury, Venus, Earth, Mars type
        GasGiant,         // Jupiter, Saturn type
        IceGiant,         // Uranus, Neptune type
        Moon,
        Asteroid,
        SpaceStation,
        AsteroidBelt
    };
    
    BodyType type = BodyType::RockyPlanet;
    std::string name = "Unnamed";
    
    // Physical properties
    double mass = 5.972e24;           // kg (Earth default)
    double radius = 6371.0;           // km (Earth default)
    double rotationPeriod = 24.0;     // hours
    double axialTilt = 0.0;           // degrees
    double temperature = 288.0;       // Kelvin (Earth default: ~15°C)
    
    // Composition and features
    bool hasAtmosphere = false;
    double atmosphereDensity = 0.0;   // kg/m³
    bool hasRings = false;
    bool hasMagneticField = false;
    bool isHabitable = false;
    
    // Gameplay properties
    bool isLandable = false;
    bool isDockable = false;          // For stations
    int faction = 0;                  // Faction ownership (0 = neutral)
};

/**
 * @brief Orbital mechanics component using Keplerian elements
 */
struct OrbitalComponent : public Component {
    unsigned int parentEntity = 0;    // Entity ID of parent body (0 = orbits star/barycenter)
    
    // Classical orbital elements
    double semiMajorAxis = 1.0;       // AU for planets, km for moons
    double eccentricity = 0.0;        // 0 = circular, 0-1 = ellipse
    double inclination = 0.0;         // degrees from reference plane
    double longitudeOfAscendingNode = 0.0;  // Ω (degrees)
    double argumentOfPeriapsis = 0.0; // ω (degrees)
    double meanAnomalyAtEpoch = 0.0;  // M₀ (degrees)
    
    // Derived properties
    double orbitalPeriod = 365.25;    // days
    double currentMeanAnomaly = 0.0;  // Current M (degrees)
    
    // Cached position (updated by orbital system)
    Vector3 cachedPosition = Vector3(0, 0, 0);
    Vector3 cachedVelocity = Vector3(0, 0, 0);
    double lastUpdateTime = 0.0;
};

/**
 * @brief Visual representation properties for celestial bodies
 */
struct VisualCelestialComponent : public Component {
    int textureHandle = -1;
    int normalMapHandle = -1;
    int cloudTextureHandle = -1;
    
    // Color (used if no texture or for tinting)
    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;
    
    // Material properties
    float emissive = 0.0f;            // For stars (0-1)
    float specular = 0.0f;            // For water/ice reflection
    float roughness = 0.5f;           // Surface roughness
    float metallic = 0.0f;            // Metallic property
    
    // Clouds (for applicable planets)
    float cloudCoverage = 0.0f;       // 0-1
    float cloudSpeed = 0.0f;          // Rotation speed relative to surface
    
    // Rings (for gas giants)
    int ringTextureHandle = -1;
    float ringInnerRadius = 0.0f;     // km
    float ringOuterRadius = 0.0f;     // km
    float ringOpacity = 1.0f;
    
    // LOD settings
    int currentLOD = 0;               // 0 = highest detail
    float lodDistance0 = 100.0f;      // Distance thresholds for LOD
    float lodDistance1 = 500.0f;
    float lodDistance2 = 2000.0f;

    // Shader configuration (populated by SolarSystem when available)
    std::string surfaceVertexShader;      // Vertex shader for body surface
    std::string surfaceFragmentShader;    // Fragment shader for body surface
    std::string orbitVertexShader;        // Vertex shader for orbit visualization
    std::string orbitFragmentShader;      // Fragment shader for orbit visualization
};

/**
 * @brief Atmospheric properties for planets with atmospheres
 */
struct AtmosphereComponent : public Component {
    float density = 1.225f;           // kg/m³ at surface (Earth = 1.225)
    float scaleHeight = 8.5f;         // km (thickness)
    float pressure = 101.325f;        // kPa at surface
    
    // Visual properties
    float colorR = 0.5f;
    float colorG = 0.7f;
    float colorB = 1.0f;
    float colorA = 0.3f;              // Atmosphere glow intensity
    
    // Composition (simplified)
    float oxygenRatio = 0.21f;        // For habitability
    float nitrogenRatio = 0.78f;
    float carbonDioxideRatio = 0.0004f;
    
    // Weather effects
    bool hasWeather = false;
    float cloudSpeed = 10.0f;         // m/s
    float weatherIntensity = 0.5f;    // For visual effects
};

/**
 * @brief Properties specific to space stations
 */
struct SpaceStationComponent : public Component {
    enum class StationType {
        Trading,      // Commodity markets
        Military,     // Defense and security
        Research,     // Scientific facilities
        Mining,       // Ore processing
        Residential,  // Habitation
        Shipyard      // Ship construction and repair
    };
    
    StationType stationType = StationType::Trading;
    
    // Facilities
    int dockingPorts = 4;
    bool hasShipyard = false;
    bool hasRepairFacility = false;
    bool hasRefuelStation = true;
    bool hasMarket = false;
    
    // Population and resources
    int population = 1000;
    int maxPopulation = 5000;
    
    // Services (bit flags could be used for more services)
    std::vector<int> availableServices; // Service IDs
    
    // Economy
    int wealthLevel = 1;              // 1-5, affects prices and available goods
};

/**
 * @brief Component for tracking a body's moons/satellites
 */
struct SatelliteSystemComponent : public Component {
    std::vector<unsigned int> satelliteEntities; // Entity IDs of moons/stations
    int moonCount = 0;
    int stationCount = 0;
};

/**
 * @brief Star-specific properties
 */
struct StarComponent : public Component {
    enum class SpectralType {
        O,  // Blue, very hot, massive
        B,  // Blue-white, hot
        A,  // White, hot
        F,  // Yellow-white, medium
        G,  // Yellow, Sun-like
        K,  // Orange, cool
        M   // Red, cool, small
    };
    
    SpectralType spectralType = SpectralType::G;
    int spectralSubclass = 2;         // 0-9 (e.g., G2 for Sun)
    
    double luminosity = 1.0;          // Relative to Sun
    double surfaceTemperature = 5778.0; // Kelvin
    
    // Habitable zone boundaries
    double habitableZoneInner = 0.95; // AU
    double habitableZoneOuter = 1.37; // AU
    
    // Visual effects
    float coronaSize = 1.5f;          // Multiplier for corona render
    bool hasFlares = true;            // Solar flares
    float flareIntensity = 0.5f;
};

/**
 * @brief Asteroid belt region (not individual asteroids)
 */
struct AsteroidBeltComponent : public Component {
    double innerRadius = 2.2;         // AU
    double outerRadius = 3.2;         // AU
    double thickness = 0.5;           // AU (vertical extent)
    
    enum class DensityLevel {
        Sparse,
        Moderate,
        Dense,
        VeryDense
    };
    DensityLevel density = DensityLevel::Moderate;
    
    enum class CompositionType {
        Metallic,   // Iron, nickel
        Rocky,      // Silicates
        Icy,        // Water ice, frozen volatiles
        Mixed       // Combination
    };
    CompositionType composition = CompositionType::Rocky;
    
    // Approximate count of significant asteroids
    int asteroidCount = 1000;
    
    // Resource richness (for mining gameplay)
    float resourceRichness = 0.5f;    // 0-1
};

/**
 * @brief Planet-specific additional data
 */
struct PlanetComponent : public Component {
    // Geological activity
    bool isTectonicallyActive = false;
    bool hasVolcanism = false;
    
    // Surface features
    bool hasOceans = false;
    float oceanCoverage = 0.0f;       // 0-1 (Earth = 0.71)
    bool hasIceCaps = false;
    float iceCoverage = 0.0f;
    
    // Biosphere
    bool hasLife = false;
    bool hasIntelligentLife = false;
    float biodiversityIndex = 0.0f;   // 0-1
    
    // Resources
    float mineralWealth = 0.5f;       // 0-1, mining value
    float organicResources = 0.0f;    // 0-1, biological resources
    
    // Surface conditions
    float gravity = 9.81f;            // m/s² (Earth = 9.81)
    float radiationLevel = 0.0f;      // Sieverts/hour (hazard level)
};

// ============================================================================
// Utility structures
// ============================================================================

/**
 * @brief Result of orbital position calculation
 */
struct OrbitalPosition {
    Vector3 position;
    Vector3 velocity;
    double trueAnomaly;               // Current angle in orbit
    double distance;                  // Distance from parent
    bool isValid;
};

/**
 * @brief Parameters for procedural generation
 */
struct GenerationParameters {
    unsigned int seed = 0;
    
    // System-wide parameters
    int minPlanets = 3;
    int maxPlanets = 10;
    float gasGiantProbability = 0.4f;
    float asteroidBeltProbability = 0.7f;
    float moonProbability = 0.6f;     // For rocky planets
    
    // Station generation
    int minStations = 2;
    int maxStations = 8;
    float stationNearHabitableProbability = 0.8f;
    
    // Visual variety
    bool generateRings = true;
    bool generateAtmospheres = true;
    bool generateMoons = true;
};
