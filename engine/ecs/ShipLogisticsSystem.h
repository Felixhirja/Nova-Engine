#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Cargo management -----------------------------------------------------------
struct CargoItem {
    std::string id;
    std::string description;
    double massTons = 0.0;
    double volumeM3 = 0.0;
};

class CargoManagementSystem {
public:
    CargoManagementSystem(double maxMassTons = 0.0, double maxVolumeM3 = 0.0);

    bool AddCargo(const CargoItem& item);
    bool RemoveCargo(const std::string& id);

    double GetAvailableMass() const;
    double GetAvailableVolume() const;
    const std::vector<CargoItem>& GetManifest() const { return manifest_; }

private:
    double maxMassTons_;
    double maxVolumeM3_;
    double usedMassTons_ = 0.0;
    double usedVolumeM3_ = 0.0;
    std::vector<CargoItem> manifest_;
};

// Crew management ------------------------------------------------------------
struct CrewMember {
    std::string name;
    std::string role;
    std::unordered_map<std::string, int> skills; // 0-100 proficiency
};

class CrewManagementSystem {
public:
    void AddCrewMember(const CrewMember& member);
    void AssignStation(const std::string& name, const std::string& stationId);
    std::optional<std::string> GetAssignment(const std::string& name) const;
    std::vector<std::string> GetCrewAtStation(const std::string& stationId) const;

private:
    std::unordered_map<std::string, CrewMember> crew_;
    std::unordered_map<std::string, std::string> assignments_; // crew name -> station
};

// Ship progression -----------------------------------------------------------
struct ResearchNode {
    std::string id;
    std::string description;
    std::vector<std::string> prerequisites;
    int reputationRequired = 0;
    double researchRequired = 0.0;
    double progress = 0.0;
    bool unlocked = false;
};

class ShipProgressionSystem {
public:
    void AddResearchNode(const ResearchNode& node);
    void AddReputation(int amount);
    bool ContributeResearch(const std::string& nodeId, double amount);
    bool IsUnlocked(const std::string& nodeId) const;

private:
    bool CanUnlock(const ResearchNode& node) const;

    int reputation_ = 0;
    std::unordered_map<std::string, ResearchNode> nodes_;
};

// Flight assist --------------------------------------------------------------
enum class FlightAssistMode {
    Manual,
    StabilityAssist,
    CruiseControl,
    DockingAssist
};

struct FlightAssistState {
    bool autoLevel = false;
    bool inertialDampening = false;
    FlightAssistMode mode = FlightAssistMode::Manual;
    double angularVelocityLimit = 0.0;
    double linearVelocityLimit = 0.0;
};

class FlightAssistController {
public:
    FlightAssistState& GetState() { return state_; }
    const FlightAssistState& GetState() const { return state_; }

    void EnableAutoLevel(bool enabled);
    void EnableDampening(bool enabled);
    void SetMode(FlightAssistMode mode);
    void ConfigureVelocityLimits(double angularDegPerSec, double linearMetersPerSec);

private:
    FlightAssistState state_;
};

// Fuel management ------------------------------------------------------------
struct FuelTank {
    std::string propellantType;
    double capacity = 0.0;
    double amount = 0.0;
    double consumptionRate = 0.0; // units per second at cruise
};

class FuelManagementSystem {
public:
    void AddTank(const std::string& id, const FuelTank& tank);
    void ConsumeFuel(double deltaTimeSeconds);
    void Refuel(const std::string& id, double amount);
    double GetMissionRangeEstimate(double burnEfficiency) const; // in seconds
    std::optional<FuelTank> GetTank(const std::string& id) const;

private:
    std::unordered_map<std::string, FuelTank> tanks_;
};

// Docking --------------------------------------------------------------------
struct DockingPort {
    std::string id;
    bool occupied = false;
    bool airlockPressurized = true;
    double alignmentScore = 0.0; // 0-1
};

class DockingSystem {
public:
    void RegisterPort(const DockingPort& port);
    bool RequestDocking(const std::string& portId);
    void UpdateAlignment(const std::string& portId, double score);
    void SetAirlockState(const std::string& portId, bool pressurized);
    std::optional<DockingPort> GetPort(const std::string& portId) const;

private:
    std::unordered_map<std::string, DockingPort> ports_;
};

// Life support ---------------------------------------------------------------
struct LifeSupportState {
    double oxygenPercent = 21.0;
    double co2Percent = 0.04;
    double temperatureC = 21.0;
    double humidityPercent = 45.0;
    double consumablesHours = 0.0;
    bool emergency = false;
};

class LifeSupportSystem {
public:
    LifeSupportState& GetState() { return state_; }
    const LifeSupportState& GetState() const { return state_; }

    void Update(double deltaTimeHours, double crewCount);
    void AddConsumables(double hours);
    void VentAtmosphere();

private:
    void EvaluateEmergency(double crewCount);

    LifeSupportState state_;
};

