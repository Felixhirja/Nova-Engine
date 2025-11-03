#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace Nova {

// Resource types in the game
enum class ResourceType {
    // Common Ores
    IronOre,
    CopperOre,
    NickelOre,
    SilicateRock,
    CarbonCompounds,
    
    // Rare Elements
    TitaniumOre,
    PlatinumOre,
    GoldOre,
    RareEarthElements,
    Uranium,
    
    // Exotic Materials
    AntiMatter,
    ExoticCrystals,
    AlienArtifacts,
    QuantumMatter,
    DarkMatterResidue,
    
    // Volatiles
    WaterIce,
    Methane,
    Ammonia,
    Helium3,
    Hydrogen,
    
    // Refined Materials
    Steel,
    Electronics,
    AdvancedAlloys,
    FusionFuel,
    Nanomaterials
};

// Mining claim component - represents ownership of an asteroid belt region
struct MiningClaimComponent {
    std::string claimantID;              // Player/faction who owns claim
    float claimRadius = 5000.0f;         // meters
    glm::vec3 claimCenter{0, 0, 0};
    float timeRemaining = 86400.0f;      // seconds (24 hours default)
    bool contested = false;
    std::vector<std::string> challengers; // Other entities disputing claim
    float claimValue = 0.0f;             // Estimated resource value
    bool registered = false;             // Officially registered with authorities
};

// Enhanced resource deposit with more detail
struct EnhancedResourceDepositComponent {
    ResourceType primaryResource = ResourceType::IronOre;
    ResourceType secondaryResource = ResourceType::CopperOre;
    
    float primaryQuantity = 10000.0f;    // kg
    float secondaryQuantity = 2000.0f;   // kg bonus resource
    
    float density = 1.0f;                // 0-1 concentration
    float miningDifficulty = 0.5f;       // 0-1 scale (affects mining rate)
    float hardness = 0.5f;               // Rock hardness (tool wear)
    
    // Discovery state
    bool discovered = false;
    bool surveyed = false;               // Has been prospected
    float surveyAccuracy = 0.0f;         // 0-1 how accurate survey is
    
    // Physical properties
    glm::vec3 position{0, 0, 0};
    float radius = 10.0f;                // meters
    float rotationSpeed = 0.0f;          // rad/s for asteroids
    
    // Environmental
    float temperature = 273.15f;         // Kelvin
    float radiation = 0.0f;              // Sv/hour
    bool unstable = false;               // Could fracture/explode
    float instabilityTimer = 0.0f;
};

// Laser drill mining tool
struct LaserDrillComponent {
    float power = 100.0f;                // 0-100% current power
    float maxPower = 100.0f;
    float powerConsumption = 20.0f;      // units/second
    
    float miningRate = 15.0f;            // kg/second base rate
    float efficiency = 1.0f;             // 0-2 multiplier
    float beamIntensity = 1.0f;          // 0-1 scale
    
    // Thermal management
    float heatGeneration = 10.0f;        // per second when active
    float currentHeat = 0.0f;
    float maxHeat = 200.0f;
    float coolingRate = 5.0f;            // per second
    bool overheated = false;
    
    // Operational
    bool active = false;
    float range = 50.0f;                 // meters
    glm::vec3 targetPosition{0, 0, 0};
    int targetEntityID = -1;
    
    // Upgrades
    int drillLevel = 1;                  // 1-5 tech level
    bool hasAutoTargeting = false;
    bool hasCoolingBoost = false;
};

// Mechanical extractor - alternative mining method
struct ExtractorComponent {
    enum class ExtractorType {
        BasicDrill,
        ImpactHammer,
        ChemicalDissolver,
        NaniteSwarm
    };
    
    ExtractorType type = ExtractorType::BasicDrill;
    float durability = 100.0f;           // 0-100%
    float wearRate = 0.1f;               // per kg mined
    float miningRate = 8.0f;             // kg/second (slower than laser but more efficient)
    float powerConsumption = 10.0f;
    bool active = false;
    float range = 10.0f;                 // Close-range mining
    int targetEntityID = -1;
};

// Resource prospecting/survey tool
struct ProspectorComponent {
    float scanRange = 500.0f;            // meters
    float scanResolution = 0.5f;         // 0-1 detail level
    float scanProgress = 0.0f;           // 0-1 current scan
    bool scanning = false;
    float energyConsumption = 5.0f;      // units/second
    float scanTime = 10.0f;              // seconds for full scan
    
    // Results
    std::vector<int> detectedDeposits;   // Entity IDs
    std::unordered_map<int, float> depositValues; // Estimated value
    
    // Upgrades
    bool canDetectRareElements = false;
    bool canAnalyzeComposition = true;
    float accuracyBonus = 0.0f;          // 0-1 added to survey accuracy
};

// Refinery component - processes raw ore into usable materials
struct RefineryComponent {
    enum class RefineryType {
        BasicSmelter,
        ChemicalProcessor,
        AdvancedRefinery,
        MolecularFabricator
    };
    
    RefineryType type = RefineryType::BasicSmelter;
    bool active = false;
    float processingRate = 5.0f;         // kg/second input
    float efficiency = 0.8f;             // 0-1 (80% yield default)
    float powerConsumption = 30.0f;
    
    // Current job
    ResourceType inputResource = ResourceType::IronOre;
    ResourceType outputResource = ResourceType::Steel;
    float inputAmount = 0.0f;
    float outputAmount = 0.0f;
    float processingProgress = 0.0f;     // 0-1
    
    // Capacity
    float inputStorageMax = 10000.0f;    // kg
    float outputStorageMax = 5000.0f;
    
    // Recipe management
    std::unordered_map<ResourceType, ResourceType> availableRecipes;
};

// Cargo hold for storing mined resources
struct ResourceCargoComponent {
    float capacity = 1000.0f;            // kg total
    float currentMass = 0.0f;
    std::unordered_map<ResourceType, float> resources; // Resource type -> amount
    
    bool autoSort = true;
    bool compressed = false;             // Compression tech
    float compressionRatio = 1.0f;       // 1.0 = no compression, 2.0 = double capacity
    
    // Transfer
    float transferRate = 10.0f;          // kg/second
    bool transferring = false;
    int transferTargetID = -1;
};

// Mining vessel designation
struct MiningVesselComponent {
    enum class VesselClass {
        SoloMiner,          // Small single-pilot mining ship
        IndustrialMiner,    // Medium multi-crew mining ship
        MiningBarge,        // Large dedicated mining vessel
        Mothership          // Massive mining operation base ship
    };
    
    VesselClass vesselClass = VesselClass::SoloMiner;
    int crewCapacity = 1;
    int currentCrew = 1;
    
    // Equipment slots
    int laserDrillSlots = 1;
    int extractorSlots = 0;
    int refinerySlots = 0;
    int cargoHolds = 1;
    
    // Operational
    float fuelConsumption = 1.0f;        // per second when mining
    float maintenanceCost = 100.0f;      // credits/hour
    bool certified = true;               // Legal mining license
};

// Automated mining drone
struct MiningDroneComponent {
    enum class DroneMode {
        Idle,
        Prospecting,
        Mining,
        Returning,
        Recharging
    };
    
    DroneMode mode = DroneMode::Idle;
    int mothershipID = -1;               // Entity it returns to
    float autonomy = 3600.0f;            // seconds of operation
    float remainingPower = 3600.0f;
    
    float miningRate = 3.0f;             // kg/second (slower but automatic)
    float cargoCapacity = 200.0f;        // kg
    float currentCargo = 0.0f;
    ResourceType targetResource = ResourceType::IronOre;
    int targetDepositID = -1;
    
    // AI behavior
    float searchRadius = 1000.0f;        // How far to look for deposits
    bool returnWhenFull = true;
    bool avoidHazards = true;
    float riskTolerance = 0.3f;          // 0-1 how much danger it accepts
};

// Environmental hazards specific to mining
struct MiningHazardComponent {
    enum class HazardType {
        Radiation,
        UnstableAsteroid,
        VolcanicActivity,
        GasVent,
        MicrometeoRoids,
        ElectricalStorm,
        GravityAnomaly
    };
    
    HazardType type = HazardType::Radiation;
    float intensity = 0.5f;              // 0-1 scale
    float damageRate = 2.0f;             // damage/second to ship/equipment
    float radius = 200.0f;               // meters
    bool active = true;
    
    // Dynamic behavior
    float cycleTime = 60.0f;             // seconds for hazard cycle
    float currentCycleTime = 0.0f;
    bool intermittent = false;           // Turns on/off
    
    // Warnings
    bool detected = false;
    float warningRange = 500.0f;         // When to alert miners
};

// Resource market pricing (for stations/trading)
struct ResourceMarketComponent {
    std::unordered_map<ResourceType, float> buyPrices;  // Credits per kg
    std::unordered_map<ResourceType, float> sellPrices;
    std::unordered_map<ResourceType, float> demand;     // 0-1 scale
    std::unordered_map<ResourceType, float> supply;     // Available quantity
    
    float marketVolatility = 0.1f;       // Price fluctuation rate
    float reputationBonus = 0.0f;        // Price bonus for high reputation
    bool blackMarket = false;            // Accepts illegal goods
};

// Mining statistics tracking
struct MiningStatsComponent {
    float totalMinedMass = 0.0f;         // kg lifetime
    float sessionMinedMass = 0.0f;       // kg this session
    int depositsExhausted = 0;
    int depositsDiscovered = 0;
    
    std::unordered_map<ResourceType, float> resourcesMinedByType;
    
    float totalEarnings = 0.0f;          // Credits
    float totalExpenses = 0.0f;          // Fuel, repairs, etc.
    
    // Time tracking
    float miningTimeActive = 0.0f;       // seconds actively mining
    float totalMiningTime = 0.0f;        // Total time in mining operations
    
    // Records
    float largestSingleHaul = 0.0f;      // kg
    float highestValueResource = 0.0f;   // Credits
    ResourceType mostMinedResource = ResourceType::IronOre;
};

// Tool durability/maintenance
struct ToolDurabilityComponent {
    float condition = 100.0f;            // 0-100%
    float degradationRate = 0.01f;       // Per second of use
    float repairCost = 100.0f;           // Credits to full repair
    
    bool needsMaintenance = false;
    bool broken = false;
    float efficiencyPenalty = 0.0f;      // 0-1 how much performance loss
    
    // Maintenance schedule
    float lastMaintenanceTime = 0.0f;
    float maintenanceInterval = 3600.0f; // Recommended time between maintenance
};

} // namespace Nova
