#pragma once

#include <string>
#include <vector>

// Telemetry data used to render the energy management HUD overlay.
struct EnergyHUDTelemetry {
    struct Preset {
        std::string name;
        double shields = 0.0;
        double weapons = 0.0;
        double thrusters = 0.0;
    };

    bool valid = false;

    // Shield subsystem metrics
    double shieldPercent = 0.0;
    double shieldCapacityMJ = 0.0;
    double shieldCapacityMaxMJ = 0.0;
    double shieldRechargeRateMJ = 0.0;
    double shieldRechargeDelaySeconds = 0.0;
    double shieldRechargeRemaining = 0.0;
    double shieldAllocation = 0.33;
    double shieldDeliveredMW = 0.0;
    double shieldRequirementMW = 0.0;

    // Weapon subsystem metrics
    double weaponPercent = 0.0;
    double weaponAllocation = 0.33;
    double weaponDeliveredMW = 0.0;
    double weaponRequirementMW = 0.0;
    double weaponCooldownSeconds = 0.0;
    int weaponAmmoCurrent = -1;
    int weaponAmmoMax = -1;

    // Thruster subsystem metrics
    double thrusterPercent = 0.0;
    double thrusterAllocation = 0.34;
    double thrusterDeliveredMW = 0.0;
    double thrusterRequirementMW = 0.0;
    double thrustToMass = 0.0;

    // Aggregate power metrics
    double netPowerMW = 0.0;
    double totalPowerOutputMW = 0.0;
    double drainRateMW = 0.0;
    double efficiencyPercent = 0.0;

    // Warning states
    bool warningPowerDeficit = false;
    bool warningShieldCritical = false;
    bool warningOverloadRisk = false;
    bool warningRechargeDelay = false;

    std::vector<std::string> warnings;

    std::vector<Preset> presets;
    std::string activePreset;
};
