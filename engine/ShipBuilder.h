#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "ecs/EntityManager.h"

/**
 * ShipBuilder - Modular ship construction and configuration system
 * 
 * Features:
 * - Component slot management with hardpoints
 * - Validation and compatibility checking
 * - Performance calculations (speed, maneuverability, power)
 * - Preset loadout templates
 * - Ship customization (naming, paint jobs)
 * - Multiple ship hangar management
 */

namespace ShipBuilding {

// Forward declarations
struct ComponentDefinition;
struct ShipHull;
struct ShipLoadout;
struct PerformanceMetrics;

// Component categories
enum class ComponentType {
    Engine,
    Weapon,
    Shield,
    Sensor,
    PowerPlant,
    CargoHold,
    LifeSupport,
    FuelTank,
    Thruster,
    Armor,
    Computer,
    ECM,  // Electronic Countermeasures
    Mining,
    Repair
};

// Component size classes
enum class ComponentSize {
    Small,
    Medium,
    Large,
    XLarge,
    Capital
};

// Hardpoint types
enum class HardpointType {
    Universal,      // Any component type
    Weapon,         // Weapons only
    Utility,        // Non-combat systems
    Engine,         // Propulsion systems
    Internal,       // Internal systems (shields, power, etc.)
    External        // External mounted (cargo pods, etc.)
};

// Component compatibility flags
enum class CompatibilityFlags : uint32_t {
    None = 0,
    RequiresPowerPlant = 1 << 0,
    RequiresCooling = 1 << 1,
    RequiresComputer = 1 << 2,
    ConflictsWithCloaking = 1 << 3,
    RequiresHeavyMount = 1 << 4,
    MilitaryGrade = 1 << 5,
    CivilianOnly = 1 << 6
};

// Component definition
struct ComponentDefinition {
    std::string id;
    std::string name;
    std::string description;
    ComponentType type;
    ComponentSize size;
    
    // Requirements
    double powerDraw = 0.0;         // MW
    double coolingRequired = 0.0;   // Thermal units
    double mass = 0.0;              // Tons
    double volume = 0.0;            // Cubic meters
    uint32_t compatibilityFlags = 0;
    
    // Performance stats (varies by component type)
    std::map<std::string, double> stats;  // "thrust", "damage", "range", etc.
    
    // Economics
    double cost = 0.0;
    int techLevel = 1;
    std::string manufacturer;
    
    // Upgrade paths
    std::vector<std::string> upgradesTo;
    std::string upgradesFrom;
};

// Hardpoint slot definition
struct Hardpoint {
    std::string id;
    HardpointType type;
    ComponentSize maxSize;
    bool occupied = false;
    std::shared_ptr<ComponentDefinition> installedComponent;
    
    // 3D position for visual representation
    double x = 0.0, y = 0.0, z = 0.0;
};

// Ship hull definition
struct ShipHull {
    std::string id;
    std::string name;
    std::string className;  // Fighter, Trader, Explorer, etc.
    
    // Base stats
    double baseMass = 0.0;
    double baseArmor = 0.0;
    double basePower = 0.0;     // Power generation capacity
    double baseCooling = 0.0;   // Cooling capacity
    double cargoCapacity = 0.0;
    double fuelCapacity = 0.0;
    
    // Hardpoints
    std::vector<Hardpoint> hardpoints;
    
    // Visual
    std::string modelPath;
    std::string iconPath;
    
    // Economics
    double cost = 0.0;
    int techLevel = 1;
};

// Complete ship configuration
struct ShipLoadout {
    std::string id;
    std::string name;
    std::string customName;  // Player-assigned name
    std::shared_ptr<ShipHull> hull;
    
    // Installed components (keyed by hardpoint ID)
    std::map<std::string, std::shared_ptr<ComponentDefinition>> components;
    
    // Customization
    struct PaintJob {
        float primaryR = 1.0f, primaryG = 1.0f, primaryB = 1.0f;
        float secondaryR = 0.8f, secondaryG = 0.8f, secondaryB = 0.8f;
        std::string decalId;
    } paintJob;
    
    // Derived stats (calculated)
    PerformanceMetrics* cachedMetrics = nullptr;
    
    // Insurance
    double insuranceValue = 0.0;
    bool insured = false;
};

// Ship performance metrics
struct PerformanceMetrics {
    // Propulsion
    double maxSpeed = 0.0;          // m/s
    double acceleration = 0.0;       // m/s²
    double maneuverability = 0.0;    // deg/s turn rate
    double boostSpeed = 0.0;
    
    // Combat
    double totalFirepower = 0.0;     // DPS
    double shieldStrength = 0.0;     // HP
    double armorRating = 0.0;
    double sensorRange = 0.0;        // km
    
    // Power
    double powerGeneration = 0.0;    // MW
    double powerConsumption = 0.0;   // MW
    double powerBalance = 0.0;       // Generation - Consumption
    
    // Thermal
    double coolingCapacity = 0.0;
    double heatGeneration = 0.0;
    double thermalBalance = 0.0;
    
    // Mass
    double totalMass = 0.0;          // Tons
    double cargoCapacity = 0.0;
    double fuelCapacity = 0.0;
    
    // Economics
    double totalCost = 0.0;
    double maintenanceCost = 0.0;
    
    // Warnings
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

// Preset loadout templates
enum class PresetType {
    Fighter,
    HeavyFighter,
    Interceptor,
    Trader,
    Freighter,
    Explorer,
    Scout,
    Miner,
    Salvager,
    Support,
    Patrol,
    Bomber
};

class ShipBuilder {
public:
    ShipBuilder();
    ~ShipBuilder();
    
    // === Core Build Functions ===
    
    /**
     * Create a new ship from a hull template
     */
    std::shared_ptr<ShipLoadout> CreateShip(const std::string& hullId);
    
    /**
     * Install a component in a specific hardpoint
     * Returns true if successful
     */
    bool InstallComponent(ShipLoadout& ship, 
                         const std::string& hardpointId,
                         const std::string& componentId);
    
    /**
     * Remove component from hardpoint
     */
    bool RemoveComponent(ShipLoadout& ship, const std::string& hardpointId);
    
    /**
     * Validate ship configuration
     * Returns true if valid, populates errors/warnings
     */
    bool ValidateShip(const ShipLoadout& ship, 
                     std::vector<std::string>& errors,
                     std::vector<std::string>& warnings);
    
    /**
     * Calculate ship performance metrics
     */
    PerformanceMetrics CalculatePerformance(const ShipLoadout& ship);
    
    // === Preset System ===
    
    /**
     * Load a preset loadout template
     */
    std::shared_ptr<ShipLoadout> LoadPreset(PresetType preset);
    
    /**
     * Save current configuration as custom preset
     */
    bool SaveAsPreset(const ShipLoadout& ship, const std::string& presetName);
    
    /**
     * Get all available preset templates
     */
    std::vector<std::pair<PresetType, std::string>> GetAvailablePresets() const;
    
    // === Component Catalog ===
    
    /**
     * Get all available components of a type
     */
    std::vector<std::shared_ptr<ComponentDefinition>> 
        GetComponentsByType(ComponentType type);
    
    /**
     * Get compatible components for a hardpoint
     */
    std::vector<std::shared_ptr<ComponentDefinition>> 
        GetCompatibleComponents(const ShipLoadout& ship, 
                               const std::string& hardpointId);
    
    /**
     * Get component upgrade options
     */
    std::vector<std::shared_ptr<ComponentDefinition>> 
        GetUpgradeOptions(const std::string& componentId);
    
    // === Hull Catalog ===
    
    /**
     * Get all available ship hulls
     */
    std::vector<std::shared_ptr<ShipHull>> GetAvailableHulls() const;
    
    /**
     * Get hulls by class name
     */
    std::vector<std::shared_ptr<ShipHull>> 
        GetHullsByClass(const std::string& className);
    
    // === Hangar Management ===
    
    /**
     * Add ship to player's hangar
     */
    bool AddToHangar(std::shared_ptr<ShipLoadout> ship, const std::string& playerId);
    
    /**
     * Remove ship from hangar
     */
    bool RemoveFromHangar(const std::string& shipId, const std::string& playerId);
    
    /**
     * Get all ships in player's hangar
     */
    std::vector<std::shared_ptr<ShipLoadout>> 
        GetHangarShips(const std::string& playerId);
    
    /**
     * Set active ship
     */
    bool SetActiveShip(const std::string& shipId, const std::string& playerId);
    
    // === Customization ===
    
    /**
     * Set custom ship name
     */
    void SetShipName(ShipLoadout& ship, const std::string& name);
    
    /**
     * Apply paint job
     */
    void SetPaintJob(ShipLoadout& ship, 
                    float pr, float pg, float pb,
                    float sr, float sg, float sb);
    
    /**
     * Apply decal
     */
    void SetDecal(ShipLoadout& ship, const std::string& decalId);
    
    // === Insurance ===
    
    /**
     * Calculate insurance cost for ship
     */
    double CalculateInsuranceCost(const ShipLoadout& ship);
    
    /**
     * Purchase insurance for ship
     */
    bool PurchaseInsurance(ShipLoadout& ship);
    
    /**
     * File insurance claim (ship lost)
     */
    bool FileInsuranceClaim(const std::string& shipId, const std::string& playerId);
    
    // === Data Loading ===
    
    /**
     * Load component definitions from JSON
     */
    bool LoadComponentCatalog(const std::string& jsonPath);
    
    /**
     * Load hull definitions from JSON
     */
    bool LoadHullCatalog(const std::string& jsonPath);
    
    /**
     * Load preset loadouts from JSON
     */
    bool LoadPresets(const std::string& jsonPath);
    
    // === Serialization ===
    
    /**
     * Save ship configuration to JSON
     */
    bool SaveShip(const ShipLoadout& ship, const std::string& filepath);
    
    /**
     * Load ship configuration from JSON
     */
    std::shared_ptr<ShipLoadout> LoadShip(const std::string& filepath);

private:
    // Component catalog
    std::map<std::string, std::shared_ptr<ComponentDefinition>> componentCatalog_;
    
    // Hull catalog
    std::map<std::string, std::shared_ptr<ShipHull>> hullCatalog_;
    
    // Preset loadouts
    std::map<PresetType, std::shared_ptr<ShipLoadout>> presets_;
    std::map<std::string, std::shared_ptr<ShipLoadout>> customPresets_;
    
    // Player hangars (playerId -> ships)
    std::map<std::string, std::vector<std::shared_ptr<ShipLoadout>>> hangars_;
    std::map<std::string, std::string> activeShips_;  // playerId -> shipId
    
    // Helper functions
    bool CheckHardpointCompatibility(const Hardpoint& hardpoint,
                                     const ComponentDefinition& component);
    bool CheckComponentRequirements(const ShipLoadout& ship,
                                   const ComponentDefinition& component);
    double CalculatePowerConsumption(const ShipLoadout& ship);
    double CalculateHeatGeneration(const ShipLoadout& ship);
    double CalculateTotalMass(const ShipLoadout& ship);
};

} // namespace ShipBuilding
