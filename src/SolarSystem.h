#pragma once

#include <vector>
#include "CelestialBody.h"
#include "ecs/EntityManager.h"

/**
 * @brief Manages a complete solar system and handles orbital mechanics
 * 
 * This class serves as the central manager for a solar system, tracking all
 * celestial bodies and updating their orbital positions over time.
 */
class SolarSystem {
public:
    SolarSystem();
    ~SolarSystem();
    
    /**
     * @brief Initialize the solar system
     * @param entityManager Pointer to ECS entity manager
     * @param systemName Name of this solar system
     */
    void Init(EntityManager* entityManager, const std::string& systemName);
    
    /**
     * @brief Update orbital positions for all bodies
     * @param dt Delta time in seconds
     * @param timeAcceleration Time acceleration factor (1.0 = normal, 10.0 = 10x speed)
     */
    void Update(double dt, double timeAcceleration = 1.0);
    
    /**
     * @brief Get the central star entity
     * @return Entity ID of the star, or 0 if none
     */
    Entity GetStarEntity() const { return starEntity_; }
    
    /**
     * @brief Set the central star
     * @param starEntity Entity ID of the star
     */
    void SetStarEntity(Entity starEntity) { starEntity_ = starEntity; }
    
    /**
     * @brief Add a planet to the system
     * @param planetEntity Entity ID of the planet
     */
    void AddPlanet(Entity planetEntity);
    
    /**
     * @brief Get all planets in the system
     * @return Vector of planet entity IDs
     */
    const std::vector<Entity>& GetPlanets() const { return planets_; }
    
    /**
     * @brief Add a moon to a planet
     * @param planetEntity Parent planet entity ID
     * @param moonEntity Moon entity ID
     */
    void AddMoon(Entity planetEntity, Entity moonEntity);
    
    /**
     * @brief Get all moons for a specific planet
     * @param planetEntity Planet entity ID
     * @return Vector of moon entity IDs
     */
    std::vector<Entity> GetMoons(Entity planetEntity) const;
    
    /**
     * @brief Add an asteroid belt
     * @param beltEntity Entity ID of the belt
     */
    void AddAsteroidBelt(Entity beltEntity);
    
    /**
     * @brief Add a space station
     * @param stationEntity Entity ID of the station
     */
    void AddSpaceStation(Entity stationEntity);
    
    /**
     * @brief Get all space stations
     * @return Vector of station entity IDs
     */
    const std::vector<Entity>& GetSpaceStations() const { return spaceStations_; }
    
    /**
     * @brief Find the nearest celestial body to a position
     * @param position Query position
     * @param maxDistance Maximum search distance (negative = unlimited)
     * @return Entity ID of nearest body, or 0 if none found
     */
    Entity FindNearestBody(const Vector3& position, double maxDistance = -1.0) const;
    
    /**
     * @brief Find all bodies within a radius
     * @param position Center position
     * @param radius Search radius
     * @return Vector of entity IDs within radius
     */
    std::vector<Entity> FindBodiesInRadius(const Vector3& position, double radius) const;
    
    /**
     * @brief Get the current simulation time
     * @return Simulation time in seconds since epoch
     */
    double GetSimulationTime() const { return simulationTime_; }
    
    /**
     * @brief Set the simulation time (for time jumps)
     * @param time New simulation time in seconds
     */
    void SetSimulationTime(double time) { simulationTime_ = time; }
    
    /**
     * @brief Get the system name
     * @return Name of the solar system
     */
    const std::string& GetSystemName() const { return systemName_; }
    
    /**
     * @brief Clear all entities from the system
     */
    void Clear();
    
    /**
     * @brief Calculate orbital position for a body at current time
     * @param orbit Orbital parameters
     * @param parentPosition Position of parent body
     * @return Orbital position result
     */
    OrbitalPosition CalculateOrbitalPosition(
        const OrbitalComponent& orbit,
        const Vector3& parentPosition
    ) const;
    
    /**
     * @brief Solve Kepler's equation for eccentric anomaly
     * @param meanAnomaly Mean anomaly in radians
     * @param eccentricity Orbital eccentricity
     * @param tolerance Convergence tolerance
     * @param maxIterations Maximum Newton-Raphson iterations
     * @return Eccentric anomaly in radians
     */
    static double SolveKeplersEquation(
        double meanAnomaly,
        double eccentricity,
        double tolerance = 1e-8,
        int maxIterations = 20
    );
    
    /**
     * @brief Convert orbital elements to Cartesian position
     * @param trueAnomaly True anomaly in radians
     * @param distance Distance from parent in km
     * @param orbit Orbital parameters
     * @return Position vector in 3D space
     */
    static Vector3 OrbitalToCartesian(
        double trueAnomaly,
        double distance,
        const OrbitalComponent& orbit
    );
    
    /**
     * @brief Enable or disable orbital visualization
     * @param enabled True to show orbital paths
     */
    void SetOrbitalVisualizationEnabled(bool enabled) { orbitalVisualizationEnabled_ = enabled; }
    
    /**
     * @brief Check if orbital visualization is enabled
     * @return True if enabled
     */
    bool IsOrbitalVisualizationEnabled() const { return orbitalVisualizationEnabled_; }

private:
    /**
     * @brief Update a single orbital component
     * @param entity Entity ID
     * @param orbit Orbital component to update
     * @param parentPosition Position of parent body
     */
    void UpdateOrbit(Entity entity, OrbitalComponent* orbit, const Vector3& parentPosition);
    
    /**
     * @brief Recursively update a body and its satellites
     * @param entity Entity ID to update
     * @param parentPosition Position of parent body
     */
    void UpdateBodyHierarchy(Entity entity, const Vector3& parentPosition);
    
    /**
     * @brief Get position of an entity
     * @param entity Entity ID
     * @return Position vector (0,0,0 if not found)
     */
    Vector3 GetEntityPosition(Entity entity) const;

private:
    EntityManager* entityManager_;
    std::string systemName_;
    
    // Hierarchical structure
    Entity starEntity_;
    std::vector<Entity> planets_;
    std::vector<Entity> asteroidBelts_;
    std::vector<Entity> spaceStations_;
    
    // Moon tracking (planet entity -> list of moon entities)
    std::vector<std::pair<Entity, std::vector<Entity>>> planetMoons_;
    
    // Simulation time
    double simulationTime_;         // In seconds since arbitrary epoch
    
    // Rendering options
    bool orbitalVisualizationEnabled_;
    
    // Update optimization
    int updateCounter_;             // For staggered updates
    static const int UPDATE_FREQUENCY = 1; // Update every N frames
};
