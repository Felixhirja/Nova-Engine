#pragma once

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <cstdint>
#include "ecs/Components.h"
#include "ecs/EntityManager.h"

/**
 * @brief Procedurally generates solar systems with planets, moons, asteroids, and stations
 * 
 * This class handles the creation of entire solar systems using seed-based procedural
 * generation. It creates realistic orbital hierarchies and populates systems with
 * diverse celestial bodies and space stations.
 */
struct GenerationSeeds {
    unsigned int baseSeed = 0;
    unsigned int starSeed = 0;
    unsigned int planetSeed = 0;
    unsigned int moonSeed = 0;
    unsigned int asteroidSeed = 0;
    unsigned int stationSeed = 0;
    unsigned int namingSeed = 0;
};

class SolarSystemGenerator {
public:
    SolarSystemGenerator();
    ~SolarSystemGenerator();

    /**
     * @brief Categories for deterministic sub-seeds
     */
    enum class SeedType {
        Star,
        Planet,
        Moon,
        Asteroid,
        Station,
        Name
    };

    /**
     * @brief Configure the generator to use a specific base seed
     *
     * Calling this function will update the derived seeds used for each
     * subsystem (planets, moons, asteroid belts, etc.). The same base seed will
     * always produce the same derived seeds, ensuring reproducible generation
     * regardless of the order in which random numbers are consumed.
     */
    void SetSeed(unsigned int seed);

    /**
     * @brief Retrieve the current set of derived seeds
     * @return Struct containing the base seed and category-specific seeds
     */
    const GenerationSeeds& GetSeeds() const { return seeds_; }

    /**
     * @brief Get a deterministic seed for a specific category
     *
     * @param type Category of seed (star, planet, moon, etc.)
     * @param index Optional index when generating multiple objects in same
     *              category (e.g. planet number). Different indices produce
     *              different seeds while remaining reproducible for the same
     *              base seed.
     * @return Deterministic seed value for the requested category/index
     */
    unsigned int GetSeed(SeedType type, unsigned int index = 0) const;

    /**
     * @brief Convenience helper to create an RNG seeded for a category/index
     */
    std::mt19937 CreateRng(SeedType type, unsigned int index = 0) const;
    
    /**
     * @brief Generate a complete solar system
     * @param entityManager ECS entity manager to create entities in
     * @param seed Random seed for reproducible generation
     * @param params Optional generation parameters
     * @return Entity ID of the central star, or 0 on failure
     */
    Entity GenerateSystem(
        EntityManager* entityManager,
        unsigned int seed,
        const GenerationParameters& params = GenerationParameters()
    );
    
    /**
     * @brief Generate just a star
     * @param entityManager ECS entity manager
     * @param rng Random number generator
     * @return Entity ID of the star
     */
    Entity GenerateStar(EntityManager* entityManager, std::mt19937& rng);
    
    /**
     * @brief Generate planets orbiting a star
     * @param entityManager ECS entity manager
     * @param starEntity Entity ID of the central star
     * @param starComponent Star properties
     * @param rng Random number generator
     * @param params Generation parameters
     * @return Vector of planet entity IDs
     */
    std::vector<Entity> GeneratePlanets(
        EntityManager* entityManager,
        Entity starEntity,
        const StarComponent& starComponent,
        std::mt19937& rng,
        const GenerationParameters& params
    );
    
    /**
     * @brief Generate moons orbiting a planet
     * @param entityManager ECS entity manager
     * @param planetEntity Entity ID of the parent planet
     * @param planetBody Properties of the parent planet
     * @param rng Random number generator
     * @return Vector of moon entity IDs
     */
    std::vector<Entity> GenerateMoons(
        EntityManager* entityManager,
        Entity planetEntity,
        const CelestialBodyComponent& planetBody,
        std::mt19937& rng
    );
    
    /**
     * @brief Generate asteroid belts in the system
     * @param entityManager ECS entity manager
     * @param starEntity Entity ID of the central star
     * @param planetOrbits Orbital distances of existing planets
     * @param rng Random number generator
     * @return Vector of asteroid belt entity IDs
     */
    std::vector<Entity> GenerateAsteroidBelts(
        EntityManager* entityManager,
        Entity starEntity,
        const std::vector<double>& planetOrbits,
        std::mt19937& rng
    );
    
    /**
     * @brief Generate space stations in strategic locations
     * @param entityManager ECS entity manager
     * @param starEntity Entity ID of the central star
     * @param planets Vector of planet entities
     * @param rng Random number generator
     * @param params Generation parameters
     * @return Vector of space station entity IDs
     */
    std::vector<Entity> GenerateSpaceStations(
        EntityManager* entityManager,
        Entity starEntity,
        const std::vector<Entity>& planets,
        std::mt19937& rng,
        const GenerationParameters& params
    );
    
    /**
     * @brief Get a descriptive name for a celestial body
     * @param systemName Name of the parent system
     * @param bodyType Type of celestial body
     * @param index Index in sequence (e.g., 3rd planet)
     * @param parentName Name of parent body (for moons)
     * @return Generated name
     */
    static std::string GenerateName(
        const std::string& systemName,
        CelestialBodyComponent::BodyType bodyType,
        int index,
        const std::string& parentName = ""
    );
    
    /**
     * @brief Calculate habitable zone boundaries for a star
     * @param luminosity Star's luminosity relative to Sun
     * @param innerBound Output: inner boundary in AU
     * @param outerBound Output: outer boundary in AU
     */
    static void CalculateHabitableZone(double luminosity, double& innerBound, double& outerBound);
    
    /**
     * @brief Determine if an orbit is in the habitable zone
     * @param semiMajorAxis Orbital distance in AU
     * @param innerBound Inner habitable zone boundary
     * @param outerBound Outer habitable zone boundary
     * @return True if in habitable zone
     */
    static bool IsInHabitableZone(double semiMajorAxis, double innerBound, double outerBound);

private:
    // Generation helper methods
    
    /**
     * @brief Generate spectral type for a star
     * @param rng Random number generator
     * @return Spectral type (weighted towards G and K types)
     */
    StarComponent::SpectralType GenerateSpectralType(std::mt19937& rng);
    
    /**
     * @brief Calculate star properties from spectral type
     * @param type Spectral type
     * @param subclass Spectral subclass (0-9)
     * @param mass Output: star mass in solar masses
     * @param radius Output: star radius in km
     * @param temperature Output: surface temperature in Kelvin
     * @param luminosity Output: luminosity relative to Sun
     */
    void CalculateStarProperties(
        StarComponent::SpectralType type,
        int subclass,
        double& mass,
        double& radius,
        double& temperature,
        double& luminosity
    );
    
    /**
     * @brief Generate orbital distances for planets
     * @param planetCount Number of planets to place
     * @param starMass Mass of central star
     * @param rng Random number generator
     * @return Vector of orbital distances in AU
     */
    std::vector<double> GenerateOrbitalDistances(int planetCount, double starMass, std::mt19937& rng);
    
    /**
     * @brief Determine planet type based on distance from star
     * @param distanceAU Distance from star in AU
     * @param starLuminosity Star's luminosity
     * @param rng Random number generator
     * @return Planet type
     */
    CelestialBodyComponent::BodyType DeterminePlanetType(
        double distanceAU,
        double starLuminosity,
        std::mt19937& rng
    );
    
    /**
     * @brief Generate properties for a rocky planet
     * @param distanceAU Orbital distance in AU
     * @param starLuminosity Star's luminosity
     * @param rng Random number generator
     * @return Celestial body component with properties
     */
    CelestialBodyComponent GenerateRockyPlanetProperties(
        double distanceAU,
        double starLuminosity,
        std::mt19937& rng
    );
    
    /**
     * @brief Generate properties for a gas giant
     * @param distanceAU Orbital distance in AU
     * @param rng Random number generator
     * @return Celestial body component with properties
     */
    CelestialBodyComponent GenerateGasGiantProperties(double distanceAU, std::mt19937& rng);
    
    /**
     * @brief Generate properties for an ice giant
     * @param distanceAU Orbital distance in AU
     * @param rng Random number generator
     * @return Celestial body component with properties
     */
    CelestialBodyComponent GenerateIceGiantProperties(double distanceAU, std::mt19937& rng);
    
    /**
     * @brief Generate orbital parameters
     * @param semiMajorAxis Orbital distance
     * @param parentMass Mass of parent body
     * @param isInner True if inner system (tighter orbits)
     * @param rng Random number generator
     * @return Orbital component with parameters
     */
    OrbitalComponent GenerateOrbitalParameters(
        double semiMajorAxis,
        double parentMass,
        bool isInner,
        std::mt19937& rng
    );
    
    /**
     * @brief Generate visual properties for a celestial body
     * @param bodyType Type of body
     * @param rng Random number generator
     * @return Visual component with properties
     */
    VisualCelestialComponent GenerateVisualProperties(
        CelestialBodyComponent::BodyType bodyType,
        std::mt19937& rng
    );
    
    /**
     * @brief Determine number of moons for a planet
     * @param planetType Type of planet
     * @param planetMass Mass of planet
     * @param rng Random number generator
     * @return Number of moons to generate
     */
    int DetermineMoonCount(
        CelestialBodyComponent::BodyType planetType,
        double planetMass,
        std::mt19937& rng
    );
    
    /**
     * @brief Calculate Hill sphere for moon placement
     * @param planetMass Mass of planet
     * @param semiMajorAxis Planet's distance from star
     * @param starMass Mass of star
     * @return Hill sphere radius in km
     */
    double CalculateHillSphere(double planetMass, double semiMajorAxis, double starMass);
    
    /**
     * @brief Find suitable location for an asteroid belt
     * @param planetOrbits Existing planet orbital distances
     * @param rng Random number generator
     * @param innerRadius Output: inner radius
     * @param outerRadius Output: outer radius
     * @return True if suitable location found
     */
    bool FindAsteroidBeltLocation(
        const std::vector<double>& planetOrbits,
        std::mt19937& rng,
        double& innerRadius,
        double& outerRadius
    );
    
    /**
     * @brief Choose appropriate station type for location
     * @param nearHabitableZone True if near habitable planets
     * @param nearAsteroidBelt True if near asteroid belt
     * @param nearGasGiant True if near gas giant
     * @param rng Random number generator
     * @return Station type
     */
    SpaceStationComponent::StationType ChooseStationType(
        bool nearHabitableZone,
        bool nearAsteroidBelt,
        bool nearGasGiant,
        std::mt19937& rng
    );
    
    // Utility methods
    
    /**
     * @brief Random float in range [min, max]
     */
    float RandomFloat(std::mt19937& rng, float min, float max);
    
    /**
     * @brief Random double in range [min, max]
     */
    double RandomDouble(std::mt19937& rng, double min, double max);
    
    /**
     * @brief Random integer in range [min, max] inclusive
     */
    int RandomInt(std::mt19937& rng, int min, int max);
    
    /**
     * @brief Random boolean with given probability (0-1)
     */
    bool RandomBool(std::mt19937& rng, float probability = 0.5f);
    
    /**
     * @brief Generate random color with constraints
     */
    void GenerateRandomColor(std::mt19937& rng, float& r, float& g, float& b, 
                            float minBrightness = 0.3f, float maxBrightness = 1.0f);

private:
    // Seed management helpers
    void InitializeSeedState(unsigned int seed);
    static unsigned int CombineSeed(unsigned int seed, std::uint32_t salt);
    static unsigned int CombineSeedWithIndex(unsigned int seed, unsigned int index);

    // System generation state
    std::string currentSystemName_;
    unsigned int currentSeed_;
    GenerationSeeds seeds_;
};
