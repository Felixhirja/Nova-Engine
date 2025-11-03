#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Nova {

// Planetary atmosphere for landing simulation (different from celestial AtmosphereComponent)
struct PlanetaryAtmosphereComponent : public Component {
    float density = 1.0f;              // kg/m³
    float pressure = 101.325f;          // kPa
    float temperature = 288.15f;        // Kelvin
    float windSpeed = 0.0f;             // m/s
    glm::vec3 windDirection{0, 0, 0};
    float turbulence = 0.0f;            // 0-1 scale
    bool breathable = false;
    float toxicity = 0.0f;              // 0-1 scale
    float radiationLevel = 0.0f;        // Sv/hour
};

// Heat shield for atmospheric entry
struct HeatShieldComponent : public Component {
    float integrity = 100.0f;           // 0-100%
    float maxHeat = 3000.0f;            // Kelvin
    float currentHeat = 300.0f;         // Kelvin
    float coolingRate = 50.0f;          // K/second
    float ablativeThickness = 1.0f;     // meters
    bool deployed = false;
    bool damaged = false;
};

// Landing gear and control
struct LandingGearComponent : public Component {
    bool deployed = false;
    bool locked = false;
    float deployTime = 2.0f;            // seconds
    float currentDeployProgress = 0.0f;
    float maxLandingSpeed = 5.0f;       // m/s
    float groundClearance = 2.0f;       // meters
    bool onGround = false;
    glm::vec3 groundNormal{0, 1, 0};
};

// EVA suit with life support
struct EVASuitComponent : public Component {
    bool equipped = false;
    float oxygenCapacity = 7200.0f;     // seconds (2 hours)
    float oxygenRemaining = 7200.0f;
    float oxygenConsumptionRate = 1.0f; // units/second
    float suitIntegrity = 100.0f;       // 0-100%
    float temperature = 310.15f;        // Kelvin (37°C body temp)
    float radiationShielding = 0.5f;    // 0-1 scale
    bool helmetSealed = true;
    bool lifeSupportActive = true;
    
    // Suit capabilities
    float jetpackFuel = 100.0f;         // 0-100%
    float jetpackThrust = 500.0f;       // Newtons
    bool magneticBootsActive = false;
};

// Surface vehicle component
struct SurfaceVehicleComponent : public Component {
    enum class VehicleType {
        Rover,
        Bike,
        Jetpack,
        Walker
    };
    
    VehicleType type = VehicleType::Rover;
    float fuel = 100.0f;                // 0-100%
    float fuelConsumption = 0.1f;       // per second
    float maxSpeed = 25.0f;             // m/s
    float acceleration = 5.0f;          // m/s²
    float handling = 1.0f;              // 0-1 scale
    int passengerCapacity = 2;
    float cargoCapacity = 500.0f;       // kg
    bool active = false;
};

// Weather system
struct WeatherComponent : public Component {
    enum class WeatherType {
        Clear,
        Cloudy,
        Rain,
        Storm,
        Fog,
        Dust,
        Snow,
        Extreme
    };
    
    WeatherType currentWeather = WeatherType::Clear;
    float intensity = 0.0f;             // 0-1 scale
    float visibility = 10000.0f;        // meters
    float precipitation = 0.0f;         // mm/hour
    float stormSeverity = 0.0f;         // 0-1 scale
    glm::vec3 windVector{0, 0, 0};
    bool hazardous = false;
    
    // Weather effects
    float lightningFrequency = 0.0f;    // strikes/minute
    float temperatureEffect = 0.0f;     // Kelvin modifier
};

// Day/night cycle
struct DayNightCycleComponent : public Component {
    float dayLength = 86400.0f;         // seconds (24 hours)
    float currentTime = 43200.0f;       // seconds (noon)
    float sunAngle = 0.0f;              // radians
    float ambientLight = 1.0f;          // 0-1 scale
    glm::vec3 sunDirection{0, 1, 0};
    bool isDaytime = true;
    float twilightDuration = 3600.0f;   // seconds
};

// Resource deposit for mining
struct ResourceDepositComponent : public Component {
    enum class ResourceType {
        IronOre,
        CopperOre,
        TitaniumOre,
        PreciousMetal,
        RareElement,
        Biomass,
        WaterIce,
        Volatiles
    };
    
    ResourceType type = ResourceType::IronOre;
    float quantity = 1000.0f;           // kg
    float density = 1.0f;               // concentration
    float miningDifficulty = 0.5f;      // 0-1 scale
    bool discovered = false;
    glm::vec3 position{0, 0, 0};
    float radius = 10.0f;               // meters
};

// Mining equipment
struct MiningEquipmentComponent : public Component {
    enum class EquipmentType {
        HandDrill,
        MiningLaser,
        ExtractionRig,
        SurveyScanner
    };
    
    EquipmentType type = EquipmentType::HandDrill;
    float power = 100.0f;               // 0-100%
    float efficiency = 1.0f;            // mining rate multiplier
    float miningRate = 10.0f;           // kg/second
    float heatGeneration = 0.0f;        // current heat
    float maxHeat = 100.0f;
    bool active = false;
    float range = 5.0f;                 // meters
};

// Surface scanner
struct SurfaceScannerComponent : public Component {
    float scanRange = 100.0f;           // meters
    float scanResolution = 1.0f;        // 0-1 scale
    float scanProgress = 0.0f;          // 0-1 for current scan
    bool scanning = false;
    float energyConsumption = 10.0f;    // units/second
    
    // Detected features
    std::vector<int> detectedResources;
    std::vector<int> detectedHazards;
    std::vector<int> detectedStructures;
};

// Cave system entry
struct CaveSystemComponent : public Component {
    int entranceCount = 0;
    float depth = 0.0f;                 // meters
    float exploredPercent = 0.0f;       // 0-100%
    bool hasHostiles = false;
    bool hasResources = true;
    float ambientLight = 0.1f;          // 0-1 scale
    float oxygenLevel = 0.0f;           // 0-1 (0 = none)
    std::vector<glm::vec3> entrancePoints;
};

// Flora/Fauna encounter
struct BiologicalEntityComponent : public Component {
    enum class EntityType {
        Flora,
        Fauna,
        Fungal,
        Bacterial,
        Unknown
    };
    
    EntityType type = EntityType::Flora;
    std::string species = "Unknown";
    bool hostile = false;
    bool harvestable = false;
    float dangerLevel = 0.0f;           // 0-1 scale
    float biomassValue = 10.0f;         // kg
    bool scanned = false;
    bool rare = false;
};

// Surface base/outpost
struct SurfaceBaseComponent : public Component {
    enum class BaseType {
        Outpost,
        MiningStation,
        ResearchLab,
        Habitat,
        Spaceport,
        Military
    };
    
    BaseType type = BaseType::Outpost;
    std::string name = "Unnamed Base";
    float integrity = 100.0f;           // 0-100%
    int population = 0;
    bool powered = true;
    bool lifeSupportOnline = true;
    float oxygenLevel = 100.0f;
    float powerReserve = 100.0f;
    
    // Construction
    bool underConstruction = false;
    float constructionProgress = 0.0f;  // 0-1
    
    // Services
    bool hasRefueling = false;
    bool hasRepair = false;
    bool hasMedical = false;
    bool hasMarket = false;
};

// Landing zone
struct LandingZoneComponent : public Component {
    enum class ZoneType {
        Spaceport,
        OutpostPad,
        Emergency,
        ProceduralSite
    };
    
    ZoneType type = ZoneType::ProceduralSite;
    glm::vec3 position{0, 0, 0};
    float radius = 50.0f;               // meters
    bool occupied = false;
    bool cleared = true;                // safe to land
    float terrainRoughness = 0.0f;      // 0-1 scale
    int maxShipSize = 3;                // 1=small, 5=capital
    bool hasBeacon = true;
    bool controlled = false;            // ATC present
};

// Planetary gravity well
struct GravityWellComponent : public Component {
    float surfaceGravity = 9.81f;       // m/s²
    float altitude = 0.0f;              // meters above surface
    float escapeVelocity = 11200.0f;    // m/s
    bool inAtmosphere = false;
    float atmosphericDrag = 0.0f;       // N (force)
};

// Environmental hazard
struct EnvironmentalHazardComponent : public Component {
    enum class HazardType {
        Radiation,
        ExtremeHeat,
        ExtremeCold,
        ToxicAtmosphere,
        AcidRain,
        SeismicActivity,
        VolcanicActivity,
        LavaFlow
    };
    
    HazardType type = HazardType::Radiation;
    float intensity = 0.5f;             // 0-1 scale
    float damageRate = 1.0f;            // damage/second
    float radius = 100.0f;              // meters
    glm::vec3 position{0, 0, 0};
    bool active = true;
    float duration = -1.0f;             // -1 = permanent
};

} // namespace Nova
