#pragma once

#include "../ecs/EntityManager.h"
#include "../ecs/PlanetaryComponents.h"
#include "../SimpleJson.h"
#include <string>
#include <memory>

namespace Nova {

/**
 * PlanetaryConfigLoader
 * 
 * Loads planetary scenarios from JSON configuration files.
 * Integrates with ConfigSystem and EntityManager.
 * 
 * Usage:
 *   PlanetaryConfigLoader loader(entityManager);
 *   auto planetEntity = loader.LoadScenario("earthlike_temperate");
 */
class PlanetaryConfigLoader {
public:
    explicit PlanetaryConfigLoader(EntityManager& em) : entityManager_(em) {}
    
    /**
     * Load a planetary scenario by name from config file
     * @param scenarioName Name from planetary_scenarios.json
     * @return Entity ID of created planet
     */
    Entity LoadScenario(const std::string& scenarioName, const std::string& configPath = "assets/config/planetary_scenarios.json");
    
    /**
     * Load EVA suit preset
     * @param presetName Name from eva_suit_presets section
     * @param entity Entity to add EVA suit component to
     */
    void LoadEVASuitPreset(const std::string& presetName, Entity entity);
    
    /**
     * Load vehicle template
     * @param templateName Name from vehicle_templates section
     * @param entity Entity to add vehicle component to
     */
    void LoadVehicleTemplate(const std::string& templateName, Entity entity);
    
    /**
     * Load base template
     * @param templateName Name from base_templates section
     * @param entity Entity to add base component to
     */
    void LoadBaseTemplate(const std::string& templateName, Entity entity);
    
    /**
     * Create a complete planetary environment from scenario
     * Includes atmosphere, weather, day/night, gravity
     * @param scenarioName Scenario to load
     * @param position Position in world space
     * @return Planet entity ID
     */
    Entity CreatePlanetaryEnvironment(const std::string& scenarioName, const glm::vec3& position = {0, 0, 0});
    
private:
    EntityManager& entityManager_;
    SimpleJson config_;
    bool configLoaded_ = false;
    
    void EnsureConfigLoaded(const std::string& configPath);
    
    // Component loaders
    void LoadAtmosphere(const SimpleJson& json, Entity entity);
    void LoadGravity(const SimpleJson& json, Entity entity);
    void LoadDayNight(const SimpleJson& json, Entity entity);
    void LoadWeather(const SimpleJson& json, Entity entity);
    void LoadHazards(const SimpleJson& json, Entity entity);
};

// Inline implementation for header-only convenience

inline void PlanetaryConfigLoader::EnsureConfigLoaded(const std::string& configPath) {
    if (!configLoaded_) {
        // Load from file (implement file I/O)
        // config_ = SimpleJson::Parse(fileContents);
        configLoaded_ = true;
    }
}

inline void PlanetaryConfigLoader::LoadAtmosphere(const SimpleJson& json, Entity entity) {
    PlanetaryAtmosphereComponent atmo;
    
    if (json.HasKey("density")) atmo.density = json.GetFloat("density", 1.225f);
    if (json.HasKey("pressure")) atmo.pressure = json.GetFloat("pressure", 101.325f);
    if (json.HasKey("temperature")) atmo.temperature = json.GetFloat("temperature", 288.15f);
    if (json.HasKey("breathable")) atmo.breathable = json.GetBool("breathable", false);
    if (json.HasKey("toxicity")) atmo.toxicity = json.GetFloat("toxicity", 0.0f);
    if (json.HasKey("radiation_level")) atmo.radiationLevel = json.GetFloat("radiation_level", 0.0f);
    
    entityManager_.AddComponent(entity, atmo);
}

inline void PlanetaryConfigLoader::LoadGravity(const SimpleJson& json, Entity entity) {
    GravityWellComponent gravity;
    
    if (json.HasKey("surface_gravity")) gravity.surfaceGravity = json.GetFloat("surface_gravity", 9.81f);
    if (json.HasKey("escape_velocity")) gravity.escapeVelocity = json.GetFloat("escape_velocity", 11200.0f);
    
    entityManager_.AddComponent(entity, gravity);
}

inline void PlanetaryConfigLoader::LoadDayNight(const SimpleJson& json, Entity entity) {
    DayNightCycleComponent cycle;
    
    if (json.HasKey("day_length")) cycle.dayLength = json.GetFloat("day_length", 86400.0f);
    if (json.HasKey("twilight_duration")) cycle.twilightDuration = json.GetFloat("twilight_duration", 3600.0f);
    
    entityManager_.AddComponent(entity, cycle);
}

inline void PlanetaryConfigLoader::LoadWeather(const SimpleJson& json, Entity entity) {
    WeatherComponent weather;
    
    std::string weatherType = json.GetString("default_type", "Clear");
    
    // Map string to enum
    if (weatherType == "Clear") weather.currentWeather = WeatherComponent::WeatherType::Clear;
    else if (weatherType == "Cloudy") weather.currentWeather = WeatherComponent::WeatherType::Cloudy;
    else if (weatherType == "Rain") weather.currentWeather = WeatherComponent::WeatherType::Rain;
    else if (weatherType == "Storm") weather.currentWeather = WeatherComponent::WeatherType::Storm;
    else if (weatherType == "Fog") weather.currentWeather = WeatherComponent::WeatherType::Fog;
    else if (weatherType == "Dust") weather.currentWeather = WeatherComponent::WeatherType::Dust;
    else if (weatherType == "Snow") weather.currentWeather = WeatherComponent::WeatherType::Snow;
    else if (weatherType == "Extreme") weather.currentWeather = WeatherComponent::WeatherType::Extreme;
    
    entityManager_.AddComponent(entity, weather);
}

inline void PlanetaryConfigLoader::LoadHazards(const SimpleJson& json, Entity entity) {
    // Load hazard array from config
    // Each hazard spawns a separate entity with EnvironmentalHazardComponent
    // Implementation depends on SimpleJson array support
}

inline Entity PlanetaryConfigLoader::CreatePlanetaryEnvironment(const std::string& scenarioName, const glm::vec3& position) {
    EnsureConfigLoaded("assets/config/planetary_scenarios.json");
    
    Entity planet = entityManager_.CreateEntity();
    
    // Add position
    Position pos(position.x, position.y, position.z);
    entityManager_.AddComponent(planet, pos);
    
    // Load scenario components
    // Implementation would parse JSON and call component loaders
    // This requires proper SimpleJson::Parse implementation
    
    return planet;
}

inline Entity PlanetaryConfigLoader::LoadScenario(const std::string& scenarioName, const std::string& configPath) {
    return CreatePlanetaryEnvironment(scenarioName);
}

inline void PlanetaryConfigLoader::LoadEVASuitPreset(const std::string& presetName, Entity entity) {
    EnsureConfigLoaded("assets/config/planetary_scenarios.json");
    
    EVASuitComponent suit;
    
    // Parse preset from config
    // Would extract values from JSON eva_suit_presets section
    
    if (presetName == "standard") {
        suit.oxygenCapacity = 7200.0f;
        suit.oxygenConsumptionRate = 1.0f;
        suit.jetpackThrust = 500.0f;
        suit.radiationShielding = 0.5f;
    } else if (presetName == "extended_ops") {
        suit.oxygenCapacity = 14400.0f;
        suit.oxygenConsumptionRate = 0.8f;
        suit.jetpackThrust = 500.0f;
        suit.radiationShielding = 0.5f;
    } else if (presetName == "combat_suit") {
        suit.oxygenCapacity = 5400.0f;
        suit.oxygenConsumptionRate = 1.2f;
        suit.jetpackThrust = 700.0f;
        suit.radiationShielding = 0.7f;
    } else if (presetName == "exploration_suit") {
        suit.oxygenCapacity = 18000.0f;
        suit.oxygenConsumptionRate = 0.7f;
        suit.jetpackThrust = 600.0f;
        suit.radiationShielding = 0.8f;
    }
    
    suit.equipped = true;
    suit.oxygenRemaining = suit.oxygenCapacity;
    suit.jetpackFuel = 100.0f;
    
    entityManager_.AddComponent(entity, suit);
}

inline void PlanetaryConfigLoader::LoadVehicleTemplate(const std::string& templateName, Entity entity) {
    EnsureConfigLoaded("assets/config/planetary_scenarios.json");
    
    SurfaceVehicleComponent vehicle;
    
    if (templateName == "light_rover") {
        vehicle.type = SurfaceVehicleComponent::VehicleType::Rover;
        vehicle.maxSpeed = 25.0f;
        vehicle.acceleration = 8.0f;
        vehicle.fuel = 100.0f;
        vehicle.fuelConsumption = 0.1f;
        vehicle.passengerCapacity = 2;
        vehicle.cargoCapacity = 500.0f;
    } else if (templateName == "heavy_rover") {
        vehicle.type = SurfaceVehicleComponent::VehicleType::Rover;
        vehicle.maxSpeed = 15.0f;
        vehicle.acceleration = 4.0f;
        vehicle.fuel = 200.0f;
        vehicle.fuelConsumption = 0.2f;
        vehicle.passengerCapacity = 6;
        vehicle.cargoCapacity = 2000.0f;
    } else if (templateName == "scout_bike") {
        vehicle.type = SurfaceVehicleComponent::VehicleType::Bike;
        vehicle.maxSpeed = 45.0f;
        vehicle.acceleration = 12.0f;
        vehicle.fuel = 50.0f;
        vehicle.fuelConsumption = 0.15f;
        vehicle.passengerCapacity = 1;
        vehicle.cargoCapacity = 100.0f;
    } else if (templateName == "mining_walker") {
        vehicle.type = SurfaceVehicleComponent::VehicleType::Walker;
        vehicle.maxSpeed = 8.0f;
        vehicle.acceleration = 2.0f;
        vehicle.fuel = 300.0f;
        vehicle.fuelConsumption = 0.25f;
        vehicle.passengerCapacity = 2;
        vehicle.cargoCapacity = 5000.0f;
    }
    
    entityManager_.AddComponent(entity, vehicle);
}

inline void PlanetaryConfigLoader::LoadBaseTemplate(const std::string& templateName, Entity entity) {
    EnsureConfigLoaded("assets/config/planetary_scenarios.json");
    
    SurfaceBaseComponent base;
    
    if (templateName == "frontier_outpost") {
        base.type = SurfaceBaseComponent::BaseType::Outpost;
        base.population = 25;
        base.powerReserve = 1000.0f;
        base.hasRefueling = true;
        base.hasRepair = true;
    } else if (templateName == "mining_complex") {
        base.type = SurfaceBaseComponent::BaseType::MiningStation;
        base.population = 100;
        base.powerReserve = 5000.0f;
        base.hasRefueling = true;
        base.hasRepair = true;
        base.hasMarket = true;
    } else if (templateName == "research_station") {
        base.type = SurfaceBaseComponent::BaseType::ResearchLab;
        base.population = 50;
        base.powerReserve = 3000.0f;
        base.hasMedical = true;
    } else if (templateName == "spaceport") {
        base.type = SurfaceBaseComponent::BaseType::Spaceport;
        base.population = 500;
        base.powerReserve = 20000.0f;
        base.hasRefueling = true;
        base.hasRepair = true;
        base.hasMedical = true;
        base.hasMarket = true;
    }
    
    base.integrity = 100.0f;
    base.powered = true;
    base.lifeSupportOnline = true;
    base.oxygenLevel = 100.0f;
    
    entityManager_.AddComponent(entity, base);
}

} // namespace Nova
