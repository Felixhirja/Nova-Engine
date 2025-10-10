#include "../src/ShipAssembly.h"

#include <iostream>
#include <cmath>

int main() {
    // Ensure catalogs are initialized
    const ShipHullBlueprint* fighterHull = ShipHullCatalog::Find("fighter_mk1");
    if (!fighterHull) {
        std::cerr << "Failed to locate fighter hull blueprint" << std::endl;
        return 1;
    }

    ShipAssemblyRequest goodRequest;
    goodRequest.hullId = fighterHull->id;

    auto assign = [&](const HullSlot& slot) {
        switch (slot.category) {
            case ComponentSlotCategory::PowerPlant:
                return std::string("fusion_core_mk1");
            case ComponentSlotCategory::MainThruster:
                return std::string("main_thruster_viper");
            case ComponentSlotCategory::ManeuverThruster:
                return std::string("rcs_cluster_micro");
            case ComponentSlotCategory::Shield:
                return std::string("shield_array_light");
            case ComponentSlotCategory::Weapon:
                return std::string("weapon_twin_cannon");
            case ComponentSlotCategory::Sensor:
                return std::string("sensor_targeting_mk1");
            case ComponentSlotCategory::Support:
                return std::string("support_life_pod");
            default:
                return std::string();
        }
    };

    for (const auto& slot : fighterHull->slots) {
        std::string componentId = assign(slot);
        if (componentId.empty()) {
            std::cerr << "No default component mapping for slot category " << ToString(slot.category) << std::endl;
            return 2;
        }
        goodRequest.slotAssignments[slot.slotId] = componentId;
    }

    ShipAssemblyResult goodResult = ShipAssembler::Assemble(goodRequest);
    if (!goodResult.IsValid()) {
        std::cerr << "Expected successful assembly, errors found: " << goodResult.diagnostics.errors.size() << std::endl;
        return 3;
    }

    if (goodResult.components.size() != fighterHull->slots.size()) {
        std::cerr << "Component count mismatch: got " << goodResult.components.size()
                  << " expected " << fighterHull->slots.size() << std::endl;
        return 4;
    }

    if (goodResult.totalMassTons <= fighterHull->baseMassTons) {
        std::cerr << "Total mass should exceed base hull mass" << std::endl;
        return 5;
    }

    if (goodResult.mainThrustKN <= 0.0 || goodResult.maneuverThrustKN <= 0.0) {
        std::cerr << "Derived thrust metrics not populated" << std::endl;
        return 6;
    }

    if (goodResult.NetPowerMW() >= 0.0) {
        std::cerr << "Expected negative net power due to overdraw" << std::endl;
        return 7;
    }

    const SubsystemSummary* powerPlant = goodResult.GetSubsystem(ComponentSlotCategory::PowerPlant);
    if (!powerPlant || powerPlant->components.size() != 1) {
        std::cerr << "Power plant subsystem missing or incorrect" << std::endl;
        return 8;
    }

    const SubsystemSummary* thrusters = goodResult.GetSubsystem(ComponentSlotCategory::MainThruster);
    if (!thrusters || thrusters->components.size() != 1) {
        std::cerr << "Main thruster subsystem missing" << std::endl;
        return 9;
    }

    const SubsystemSummary* avionics = goodResult.GetSubsystem(ComponentSlotCategory::Sensor);
    if (!avionics || avionics->components.empty() || goodResult.avionicsModuleCount <= 0) {
        std::cerr << "Avionics subsystem missing" << std::endl;
        return 10;
    }

    if (goodResult.diagnostics.warnings.empty()) {
        std::cerr << "Expected power warning due to load" << std::endl;
        return 11;
    }

    bool hasPowerWarning = false;
    for (const auto& warning : goodResult.diagnostics.warnings) {
        if (warning.find("Net power deficit") != std::string::npos) {
            hasPowerWarning = true;
            break;
        }
    }
    if (!hasPowerWarning) {
        std::cerr << "Missing expected power deficit warning" << std::endl;
        return 12;
    }

    const ShipPerformanceMetrics& perf = goodResult.performance;
    auto approxEqual = [](double a, double b, double eps = 1e-6) {
        return std::fabs(a - b) <= eps;
    };

    if (!approxEqual(perf.heatGenerationMW, 18.3)) {
        std::cerr << "Unexpected heat generation total: " << perf.heatGenerationMW << std::endl;
        return 15;
    }
    if (!approxEqual(perf.heatDissipationMW, 19.2)) {
        std::cerr << "Unexpected heat dissipation total: " << perf.heatDissipationMW << std::endl;
        return 16;
    }
    if (!approxEqual(goodResult.NetHeatMW(), perf.NetHeatMW())) {
        std::cerr << "Net heat mismatch" << std::endl;
        return 17;
    }
    if (goodResult.crewRequired != 2 || goodResult.crewCapacity != 4) {
        std::cerr << "Crew totals incorrect: required=" << goodResult.crewRequired
                  << " capacity=" << goodResult.crewCapacity << std::endl;
        return 18;
    }
    if (!approxEqual(goodResult.CrewUtilization(), 0.5)) {
        std::cerr << "Crew utilization mismatch: " << goodResult.CrewUtilization() << std::endl;
        return 19;
    }

    ShipAssemblyRequest badRequest = goodRequest;
    // Intentionally assign incompatible component to first slot
    if (!fighterHull->slots.empty()) {
        badRequest.slotAssignments[fighterHull->slots.front().slotId] = "weapon_cooling_cannon";
    }

    ShipAssemblyResult badResult = ShipAssembler::Assemble(badRequest);
    if (badResult.IsValid()) {
        std::cerr << "Bad assembly unexpectedly succeeded" << std::endl;
        return 13;
    }
    if (badResult.diagnostics.errors.empty()) {
        std::cerr << "Expected errors for bad assembly" << std::endl;
        return 14;
    }

    std::cout << "Ship assembly tests passed." << std::endl;

    // Test weapon stats
    const ShipComponentBlueprint* twinCannon = ShipComponentCatalog::Find("weapon_twin_cannon");
    if (!twinCannon) {
        std::cerr << "Twin cannon blueprint not found" << std::endl;
        return 20;
    }
    if (twinCannon->weaponDamagePerShot != 15.0 || twinCannon->weaponRangeKm != 5.0 ||
        twinCannon->weaponFireRatePerSecond != 10.0 || twinCannon->weaponAmmoCapacity != 200 ||
        twinCannon->weaponAmmoType != "projectile" || twinCannon->weaponIsTurret != false) {
        std::cerr << "Weapon stats incorrect for twin cannon" << std::endl;
        return 21;
    }

    const ShipComponentBlueprint* defensiveTurret = ShipComponentCatalog::Find("weapon_defensive_turret");
    if (!defensiveTurret) {
        std::cerr << "Defensive turret blueprint not found" << std::endl;
        return 22;
    }
    if (!defensiveTurret->weaponIsTurret || defensiveTurret->weaponTrackingSpeedDegPerSec != 60.0) {
        std::cerr << "Turret stats incorrect" << std::endl;
        return 23;
    }

    std::cout << "Weapon stats tests passed." << std::endl;

    // Test shield stats
    const ShipComponentBlueprint* lightShield = ShipComponentCatalog::Find("shield_array_light");
    if (!lightShield) {
        std::cerr << "Light shield blueprint not found" << std::endl;
        return 24;
    }
    if (lightShield->shieldCapacityMJ != 150.0 || lightShield->shieldRechargeRateMJPerSec != 5.0 ||
        lightShield->shieldRechargeDelaySeconds != 3.0 || lightShield->shieldDamageAbsorption != 0.8) {
        std::cerr << "Shield stats incorrect for light shield" << std::endl;
        return 25;
    }

    const ShipComponentBlueprint* heavyShield = ShipComponentCatalog::Find("shield_array_heavy");
    if (!heavyShield) {
        std::cerr << "Heavy shield blueprint not found" << std::endl;
        return 26;
    }
    if (heavyShield->shieldCapacityMJ != 600.0 || heavyShield->shieldDamageAbsorption != 0.9) {
        std::cerr << "Shield stats incorrect for heavy shield" << std::endl;
        return 27;
    }

    std::cout << "Shield stats tests passed." << std::endl;
    return 0;
}
