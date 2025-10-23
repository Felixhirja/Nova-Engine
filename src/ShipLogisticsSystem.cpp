#include "ShipLogisticsSystem.h"

#include <algorithm>
#include <cmath>

// CargoManagementSystem ------------------------------------------------------
CargoManagementSystem::CargoManagementSystem(double maxMassTons, double maxVolumeM3)
    : maxMassTons_(maxMassTons), maxVolumeM3_(maxVolumeM3) {}

bool CargoManagementSystem::AddCargo(const CargoItem& item) {
    if (item.massTons < 0.0 || item.volumeM3 < 0.0) {
        return false;
    }

    double newMass = usedMassTons_ + item.massTons;
    double newVolume = usedVolumeM3_ + item.volumeM3;
    if (newMass > maxMassTons_ || newVolume > maxVolumeM3_) {
        return false;
    }

    manifest_.push_back(item);
    usedMassTons_ = newMass;
    usedVolumeM3_ = newVolume;
    return true;
}

bool CargoManagementSystem::RemoveCargo(const std::string& id) {
    auto it = std::find_if(manifest_.begin(), manifest_.end(), [&](const CargoItem& item) {
        return item.id == id;
    });

    if (it == manifest_.end()) {
        return false;
    }

    usedMassTons_ -= it->massTons;
    usedVolumeM3_ -= it->volumeM3;
    manifest_.erase(it);
    return true;
}

double CargoManagementSystem::GetAvailableMass() const {
    return std::max(0.0, maxMassTons_ - usedMassTons_);
}

double CargoManagementSystem::GetAvailableVolume() const {
    return std::max(0.0, maxVolumeM3_ - usedVolumeM3_);
}

// CrewManagementSystem -------------------------------------------------------
void CrewManagementSystem::AddCrewMember(const CrewMember& member) {
    crew_[member.name] = member;
}

void CrewManagementSystem::AssignStation(const std::string& name, const std::string& stationId) {
    if (crew_.find(name) == crew_.end()) {
        return;
    }
    assignments_[name] = stationId;
}

std::optional<std::string> CrewManagementSystem::GetAssignment(const std::string& name) const {
    auto it = assignments_.find(name);
    if (it != assignments_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> CrewManagementSystem::GetCrewAtStation(const std::string& stationId) const {
    std::vector<std::string> result;
    for (const auto& [name, assignedStation] : assignments_) {
        if (assignedStation == stationId) {
            result.push_back(name);
        }
    }
    return result;
}

// ShipProgressionSystem ------------------------------------------------------
void ShipProgressionSystem::AddResearchNode(const ResearchNode& node) {
    nodes_[node.id] = node;
}

void ShipProgressionSystem::AddReputation(int amount) {
    reputation_ = std::max(0, reputation_ + amount);
}

bool ShipProgressionSystem::ContributeResearch(const std::string& nodeId, double amount) {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    ResearchNode& node = it->second;
    if (node.unlocked || amount <= 0.0) {
        return node.unlocked;
    }

    node.progress += amount;
    if (node.progress >= node.researchRequired && CanUnlock(node)) {
        node.unlocked = true;
        node.progress = node.researchRequired;
        return true;
    }

    return false;
}

bool ShipProgressionSystem::IsUnlocked(const std::string& nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    return it->second.unlocked;
}

bool ShipProgressionSystem::CanUnlock(const ResearchNode& node) const {
    if (reputation_ < node.reputationRequired) {
        return false;
    }

    for (const std::string& prerequisiteId : node.prerequisites) {
        auto it = nodes_.find(prerequisiteId);
        if (it == nodes_.end() || !it->second.unlocked) {
            return false;
        }
    }

    return true;
}

// FlightAssistController -----------------------------------------------------
void FlightAssistController::EnableAutoLevel(bool enabled) {
    state_.autoLevel = enabled;
}

void FlightAssistController::EnableDampening(bool enabled) {
    state_.inertialDampening = enabled;
}

void FlightAssistController::SetMode(FlightAssistMode mode) {
    state_.mode = mode;

    switch (mode) {
        case FlightAssistMode::Manual:
            state_.autoLevel = false;
            state_.inertialDampening = false;
            break;
        case FlightAssistMode::StabilityAssist:
            state_.autoLevel = true;
            state_.inertialDampening = true;
            break;
        case FlightAssistMode::CruiseControl:
            state_.autoLevel = true;
            state_.inertialDampening = true;
            break;
        case FlightAssistMode::DockingAssist:
            state_.autoLevel = true;
            state_.inertialDampening = true;
            state_.angularVelocityLimit = std::min(state_.angularVelocityLimit, 1.0);
            state_.linearVelocityLimit = std::min(state_.linearVelocityLimit, 5.0);
            break;
    }
}

void FlightAssistController::ConfigureVelocityLimits(double angularDegPerSec, double linearMetersPerSec) {
    state_.angularVelocityLimit = std::max(0.0, angularDegPerSec);
    state_.linearVelocityLimit = std::max(0.0, linearMetersPerSec);
}

// FuelManagementSystem -------------------------------------------------------
void FuelManagementSystem::AddTank(const std::string& id, const FuelTank& tank) {
    tanks_[id] = tank;
}

void FuelManagementSystem::ConsumeFuel(double deltaTimeSeconds) {
    if (deltaTimeSeconds <= 0.0) {
        return;
    }

    for (auto& [id, tank] : tanks_) {
        double consumption = tank.consumptionRate * deltaTimeSeconds;
        tank.amount = std::max(0.0, tank.amount - consumption);
    }
}

void FuelManagementSystem::Refuel(const std::string& id, double amount) {
    auto it = tanks_.find(id);
    if (it == tanks_.end() || amount <= 0.0) {
        return;
    }

    FuelTank& tank = it->second;
    tank.amount = std::min(tank.capacity, tank.amount + amount);
}

double FuelManagementSystem::GetMissionRangeEstimate(double burnEfficiency) const {
    if (burnEfficiency <= 0.0) {
        return 0.0;
    }

    double totalSeconds = 0.0;
    for (const auto& [id, tank] : tanks_) {
        if (tank.consumptionRate > 0.0) {
            double effectiveRate = tank.consumptionRate / burnEfficiency;
            totalSeconds += tank.amount / effectiveRate;
        }
    }
    return totalSeconds;
}

std::optional<FuelTank> FuelManagementSystem::GetTank(const std::string& id) const {
    auto it = tanks_.find(id);
    if (it == tanks_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// DockingSystem --------------------------------------------------------------
void DockingSystem::RegisterPort(const DockingPort& port) {
    ports_[port.id] = port;
}

bool DockingSystem::RequestDocking(const std::string& portId) {
    auto it = ports_.find(portId);
    if (it == ports_.end()) {
        return false;
    }

    DockingPort& port = it->second;
    if (port.occupied) {
        return false;
    }

    port.occupied = true;
    port.alignmentScore = 0.0;
    return true;
}

void DockingSystem::UpdateAlignment(const std::string& portId, double score) {
    auto it = ports_.find(portId);
    if (it == ports_.end()) {
        return;
    }

    DockingPort& port = it->second;
    port.alignmentScore = std::clamp(score, 0.0, 1.0);
}

void DockingSystem::SetAirlockState(const std::string& portId, bool pressurized) {
    auto it = ports_.find(portId);
    if (it == ports_.end()) {
        return;
    }

    DockingPort& port = it->second;
    port.airlockPressurized = pressurized;
}

std::optional<DockingPort> DockingSystem::GetPort(const std::string& portId) const {
    auto it = ports_.find(portId);
    if (it == ports_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// LifeSupportSystem ----------------------------------------------------------
void LifeSupportSystem::Update(double deltaTimeHours, double crewCount) {
    if (deltaTimeHours <= 0.0) {
        return;
    }

    // Consumables depletion scales with crew count.
    double consumption = crewCount * deltaTimeHours;
    state_.consumablesHours = std::max(0.0, state_.consumablesHours - consumption);

    // Air quality drift when consumables are low.
    if (state_.consumablesHours <= 0.0) {
        state_.oxygenPercent = std::max(0.0, state_.oxygenPercent - 0.5 * deltaTimeHours);
        state_.co2Percent += 0.1 * deltaTimeHours;
    } else {
        state_.oxygenPercent = std::min(21.0, state_.oxygenPercent + 0.1 * deltaTimeHours);
        state_.co2Percent = std::max(0.04, state_.co2Percent - 0.02 * deltaTimeHours);
    }

    EvaluateEmergency(crewCount);
}

void LifeSupportSystem::AddConsumables(double hours) {
    if (hours <= 0.0) {
        return;
    }
    state_.consumablesHours += hours;
    EvaluateEmergency(1.0);
}

void LifeSupportSystem::VentAtmosphere() {
    state_.oxygenPercent = 0.0;
    state_.co2Percent = 0.0;
    state_.emergency = true;
}

void LifeSupportSystem::EvaluateEmergency(double crewCount) {
    bool oxygenLow = state_.oxygenPercent < 18.0;
    bool co2High = state_.co2Percent > 1.0;
    bool consumablesDepleted = state_.consumablesHours < crewCount;

    state_.emergency = oxygenLow || co2High || consumablesDepleted;
}

