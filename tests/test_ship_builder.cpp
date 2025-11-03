#include "engine/ShipBuilder.h"
#include <iostream>
#include <iomanip>

using namespace ShipBuilding;

void PrintShipPerformance(const PerformanceMetrics& metrics) {
    std::cout << "\n=== Performance Metrics ===\n";
    std::cout << std::fixed << std::setprecision(1);
    
    std::cout << "\nPropulsion:\n";
    std::cout << "  Max Speed: " << metrics.maxSpeed << " m/s\n";
    std::cout << "  Acceleration: " << metrics.acceleration << " m/s²\n";
    std::cout << "  Maneuverability: " << metrics.maneuverability << " deg/s\n";
    
    std::cout << "\nCombat:\n";
    std::cout << "  Firepower: " << metrics.totalFirepower << " DPS\n";
    std::cout << "  Shield Strength: " << metrics.shieldStrength << " HP\n";
    std::cout << "  Armor Rating: " << metrics.armorRating << "\n";
    std::cout << "  Sensor Range: " << metrics.sensorRange << " km\n";
    
    std::cout << "\nPower:\n";
    std::cout << "  Generation: " << metrics.powerGeneration << " MW\n";
    std::cout << "  Consumption: " << metrics.powerConsumption << " MW\n";
    std::cout << "  Balance: " << (metrics.powerBalance >= 0 ? "+" : "") 
              << metrics.powerBalance << " MW\n";
    
    std::cout << "\nThermal:\n";
    std::cout << "  Cooling Capacity: " << metrics.coolingCapacity << "\n";
    std::cout << "  Heat Generation: " << metrics.heatGeneration << "\n";
    std::cout << "  Balance: " << (metrics.thermalBalance >= 0 ? "+" : "") 
              << metrics.thermalBalance << "\n";
    
    std::cout << "\nMass & Economics:\n";
    std::cout << "  Total Mass: " << metrics.totalMass << " tons\n";
    std::cout << "  Cargo Capacity: " << metrics.cargoCapacity << " tons\n";
    std::cout << "  Total Cost: $" << metrics.totalCost << "\n";
    std::cout << "  Maintenance: $" << metrics.maintenanceCost << "/cycle\n";
    
    if (!metrics.warnings.empty()) {
        std::cout << "\nWARNINGS:\n";
        for (const auto& warning : metrics.warnings) {
            std::cout << "  ⚠ " << warning << "\n";
        }
    }
    
    if (!metrics.errors.empty()) {
        std::cout << "\nERRORS:\n";
        for (const auto& error : metrics.errors) {
            std::cout << "  ✗ " << error << "\n";
        }
    }
}

int main() {
    std::cout << "=== Nova Engine Ship Building System Test ===\n\n";
    
    // Create ship builder
    ShipBuilder builder;
    
    // Note: In a full implementation, you would load data from JSON files here:
    // builder.LoadHullCatalog("assets/config/ship_hulls.json");
    // builder.LoadComponentCatalog("assets/config/ship_components.json");
    // builder.LoadPresets("assets/config/ship_presets.json");
    
    // For this test, we'll create test data programmatically
    
    // Create a test hull
    auto testHull = std::make_shared<ShipHull>();
    testHull->id = "hull_test_fighter";
    testHull->name = "Test Fighter";
    testHull->className = "Fighter";
    testHull->baseMass = 25.0;
    testHull->baseArmor = 150.0;
    testHull->basePower = 20.0;
    testHull->baseCooling = 15.0;
    testHull->cargoCapacity = 10.0;
    testHull->fuelCapacity = 100.0;
    testHull->cost = 50000.0;
    testHull->techLevel = 1;
    
    // Add hardpoints
    Hardpoint engine;
    engine.id = "engine_slot";
    engine.type = HardpointType::Engine;
    engine.maxSize = ComponentSize::Medium;
    testHull->hardpoints.push_back(engine);
    
    Hardpoint weapon1;
    weapon1.id = "weapon_slot_1";
    weapon1.type = HardpointType::Weapon;
    weapon1.maxSize = ComponentSize::Small;
    testHull->hardpoints.push_back(weapon1);
    
    Hardpoint weapon2;
    weapon2.id = "weapon_slot_2";
    weapon2.type = HardpointType::Weapon;
    weapon2.maxSize = ComponentSize::Small;
    testHull->hardpoints.push_back(weapon2);
    
    Hardpoint shield;
    shield.id = "shield_slot";
    shield.type = HardpointType::Internal;
    shield.maxSize = ComponentSize::Small;
    testHull->hardpoints.push_back(shield);
    
    std::cout << "Created test hull: " << testHull->name << "\n";
    std::cout << "  Class: " << testHull->className << "\n";
    std::cout << "  Hardpoints: " << testHull->hardpoints.size() << "\n";
    std::cout << "  Base Power: " << testHull->basePower << " MW\n";
    std::cout << "  Base Cooling: " << testHull->baseCooling << " units\n";
    std::cout << "  Cost: $" << testHull->cost << "\n";
    
    // Create test components
    auto testEngine = std::make_shared<ComponentDefinition>();
    testEngine->id = "engine_test_ion";
    testEngine->name = "Test Ion Engine";
    testEngine->description = "Basic ion propulsion for testing";
    testEngine->type = ComponentType::Engine;
    testEngine->size = ComponentSize::Medium;
    testEngine->powerDraw = 5.0;
    testEngine->coolingRequired = 3.0;
    testEngine->mass = 10.0;
    testEngine->cost = 15000.0;
    testEngine->stats["thrust"] = 50000.0;
    testEngine->stats["efficiency"] = 0.85;
    
    auto testWeapon = std::make_shared<ComponentDefinition>();
    testWeapon->id = "weapon_test_laser";
    testWeapon->name = "Test Pulse Laser";
    testWeapon->description = "Basic energy weapon for testing";
    testWeapon->type = ComponentType::Weapon;
    testWeapon->size = ComponentSize::Small;
    testWeapon->powerDraw = 2.5;
    testWeapon->coolingRequired = 2.0;
    testWeapon->mass = 3.0;
    testWeapon->cost = 8000.0;
    testWeapon->stats["dps"] = 25.0;
    testWeapon->stats["range"] = 2000.0;
    
    auto testShield = std::make_shared<ComponentDefinition>();
    testShield->id = "shield_test_basic";
    testShield->name = "Test Shield Generator";
    testShield->description = "Basic energy shield for testing";
    testShield->type = ComponentType::Shield;
    testShield->size = ComponentSize::Small;
    testShield->powerDraw = 4.0;
    testShield->coolingRequired = 2.5;
    testShield->mass = 5.0;
    testShield->cost = 12000.0;
    testShield->stats["strength"] = 500.0;
    testShield->stats["regen_rate"] = 10.0;
    
    std::cout << "\nCreated test components:\n";
    std::cout << "  - " << testEngine->name << " ($" << testEngine->cost << ")\n";
    std::cout << "  - " << testWeapon->name << " ($" << testWeapon->cost << ")\n";
    std::cout << "  - " << testShield->name << " ($" << testShield->cost << ")\n";
    
    // Create a ship
    std::cout << "\n=== Creating Ship ===\n";
    auto ship = std::make_shared<ShipLoadout>();
    ship->id = "ship_test_001";
    ship->name = "Test Fighter";
    ship->customName = "Star Blazer";
    ship->hull = testHull;
    
    std::cout << "Ship created: " << ship->customName << "\n";
    std::cout << "  Hull: " << ship->hull->name << "\n";
    
    // Install components manually (since JSON loading isn't implemented yet)
    std::cout << "\n=== Installing Components ===\n";
    
    // Install engine
    for (auto& hp : ship->hull->hardpoints) {
        if (hp.id == "engine_slot") {
            hp.installedComponent = testEngine;
            hp.occupied = true;
            ship->components[hp.id] = testEngine;
            std::cout << "Installed " << testEngine->name << " in " << hp.id << "\n";
        }
    }
    
    // Install weapons
    for (auto& hp : ship->hull->hardpoints) {
        if (hp.id == "weapon_slot_1" || hp.id == "weapon_slot_2") {
            hp.installedComponent = testWeapon;
            hp.occupied = true;
            ship->components[hp.id] = testWeapon;
            std::cout << "Installed " << testWeapon->name << " in " << hp.id << "\n";
        }
    }
    
    // Install shield
    for (auto& hp : ship->hull->hardpoints) {
        if (hp.id == "shield_slot") {
            hp.installedComponent = testShield;
            hp.occupied = true;
            ship->components[hp.id] = testShield;
            std::cout << "Installed " << testShield->name << " in " << hp.id << "\n";
        }
    }
    
    // Calculate performance
    std::cout << "\n=== Calculating Performance ===\n";
    auto metrics = builder.CalculatePerformance(*ship);
    PrintShipPerformance(metrics);
    
    // Validate ship
    std::cout << "\n=== Validation ===\n";
    std::vector<std::string> errors, warnings;
    bool valid = builder.ValidateShip(*ship, errors, warnings);
    
    if (valid) {
        std::cout << "✓ Ship configuration is VALID\n";
    } else {
        std::cout << "✗ Ship configuration has ERRORS\n";
    }
    
    // Test customization
    std::cout << "\n=== Customization ===\n";
    builder.SetShipName(*ship, "Crimson Thunder");
    std::cout << "Renamed ship to: " << ship->customName << "\n";
    
    builder.SetPaintJob(*ship, 0.8f, 0.2f, 0.2f, 0.3f, 0.3f, 0.3f);
    std::cout << "Applied paint job - Primary: RGB(" 
              << ship->paintJob.primaryR << ", "
              << ship->paintJob.primaryG << ", "
              << ship->paintJob.primaryB << ")\n";
    
    // Test insurance
    std::cout << "\n=== Insurance ===\n";
    double insuranceCost = builder.CalculateInsuranceCost(*ship);
    std::cout << "Insurance cost: $" << insuranceCost << "\n";
    
    builder.PurchaseInsurance(*ship);
    std::cout << "Insurance purchased: " << (ship->insured ? "YES" : "NO") << "\n";
    std::cout << "Insurance value: $" << ship->insuranceValue << "\n";
    
    // Test hangar system
    std::cout << "\n=== Hangar System ===\n";
    std::string playerId = "player_001";
    builder.AddToHangar(ship, playerId);
    
    auto hangarShips = builder.GetHangarShips(playerId);
    std::cout << "Ships in hangar: " << hangarShips.size() << "\n";
    for (const auto& s : hangarShips) {
        std::cout << "  - " << s->customName << " (" << s->hull->className << ")\n";
    }
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "Ship building system is operational!\n";
    std::cout << "\nNext steps:\n";
    std::cout << "  1. Implement JSON loading for hulls, components, and presets\n";
    std::cout << "  2. Integrate ShipEditorUI with ImGui rendering\n";
    std::cout << "  3. Add 3D ship visualization in editor\n";
    std::cout << "  4. Implement save/load ship configurations\n";
    std::cout << "  5. Connect to game economy and progression systems\n";
    
    return 0;
}
