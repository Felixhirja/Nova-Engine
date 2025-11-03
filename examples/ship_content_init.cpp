#include "../engine/content/ShipContentSystem.h"
#include <iostream>

using namespace NovaEngine;

/**
 * Ship Content System - Quick Start Example
 * 
 * This example demonstrates how to initialize and use the Ship Content System
 */

void InitializeShipContentSystem() {
    std::cout << "=== Initializing Ship Content System ===\n\n";
    
    // Get singleton instance
    auto& shipSystem = ShipContentSystem::Instance();
    
    // Initialize the system
    shipSystem.Initialize();
    
    std::cout << "✓ Ship Content System initialized successfully!\n\n";
}

void QuickDesignExample() {
    std::cout << "=== Quick Ship Design Example ===\n\n";
    
    auto& shipSystem = ShipContentSystem::Instance();
    
    // 1. Create a new design session
    std::cout << "1. Creating design session...\n";
    auto session = shipSystem.Designer().CreateSession("fighter_hull_basic");
    std::cout << "   Session ID: " << session.sessionId << "\n\n";
    
    // 2. Add components
    std::cout << "2. Adding components...\n";
    bool success = shipSystem.Designer().AddComponent(
        session, 
        "slot_powerplant_1", 
        "reactor_basic"
    );
    std::cout << "   Power plant added: " << (success ? "✓" : "✗") << "\n";
    
    success = shipSystem.Designer().AddComponent(
        session, 
        "slot_thruster_main_1", 
        "thruster_basic"
    );
    std::cout << "   Main thruster added: " << (success ? "✓" : "✗") << "\n\n";
    
    // 3. Validate the design
    std::cout << "3. Validating design...\n";
    auto validation = shipSystem.Validator().Validate(
        session.currentDesign,
        ShipValidator::ValidationLevel::Standard
    );
    
    std::cout << "   Valid: " << (validation.isValid ? "✓" : "✗") << "\n";
    std::cout << "   Balance Score: " << validation.balanceScore << "\n";
    std::cout << "   Errors: " << validation.errors.size() << "\n";
    std::cout << "   Warnings: " << validation.warnings.size() << "\n";
    std::cout << "   Suggestions: " << validation.suggestions.size() << "\n\n";
    
    // 4. Show suggestions
    if (!validation.suggestions.empty()) {
        std::cout << "   Improvement Suggestions:\n";
        for (const auto& suggestion : validation.suggestions) {
            std::cout << "   - " << suggestion << "\n";
        }
        std::cout << "\n";
    }
    
    // 5. Simulate performance
    std::cout << "4. Simulating performance...\n";
    auto result = ShipAssembler::Assemble(session.currentDesign);
    auto profile = shipSystem.Performance().SimulatePerformance(result);
    
    std::cout << "   Acceleration: " << profile.acceleration << " m/s²\n";
    std::cout << "   Max Speed: " << profile.maxSpeed << " m/s\n";
    std::cout << "   Turn Rate: " << profile.turnRate << " deg/s\n";
    std::cout << "   Combat Rating: " << profile.combatRating << "/100\n";
    std::cout << "   Survival Rating: " << profile.survivalRating << "/100\n";
    std::cout << "   Economic Rating: " << profile.economicRating << "/100\n\n";
    
    // 6. Save design
    std::cout << "5. Saving design...\n";
    success = shipSystem.Designer().SaveDesign(session, "my_first_fighter");
    std::cout << "   Design saved: " << (success ? "✓" : "✗") << "\n\n";
}

void QuickTemplateExample() {
    std::cout << "=== Quick Template Example ===\n\n";
    
    // Get templates by role
    std::cout << "1. Loading fighter templates...\n";
    auto templates = ShipTemplateSystem::GetTemplatesByRole("fighter");
    std::cout << "   Found " << templates.size() << " fighter templates\n\n";
    
    // Instantiate a template (if any exist)
    if (!templates.empty()) {
        std::cout << "2. Instantiating template...\n";
        auto request = ShipTemplateSystem::InstantiateTemplate(templates[0].id);
        std::cout << "   Template instantiated: " << templates[0].name << "\n\n";
        
        // Assemble and validate
        auto result = ShipAssembler::Assemble(request);
        std::cout << "   Assembly valid: " << (result.IsValid() ? "✓" : "✗") << "\n";
        std::cout << "   Total mass: " << result.totalMassTons << " tons\n";
        std::cout << "   Net power: " << result.NetPowerMW() << " MW\n";
        std::cout << "   T/M ratio: " << result.ThrustToMassRatio() << " kN/ton\n\n";
    }
}

void QuickAnalyticsExample() {
    std::cout << "=== Quick Analytics Example ===\n\n";
    
    auto& shipSystem = ShipContentSystem::Instance();
    
    // Record some events
    std::cout << "1. Recording analytics events...\n";
    shipSystem.Analytics().RecordSpawn("fighter_mk1");
    shipSystem.Analytics().RecordFlightTime("fighter_mk1", 120.5);
    shipSystem.Analytics().RecordDestruction("fighter_mk1");
    std::cout << "   Events recorded ✓\n\n";
    
    // Get statistics
    std::cout << "2. Retrieving statistics...\n";
    auto stats = shipSystem.Analytics().GetUsageStats("fighter_mk1");
    std::cout << "   Times spawned: " << stats.timesSpawned << "\n";
    std::cout << "   Times destroyed: " << stats.timesDestroyed << "\n";
    std::cout << "   Total flight time: " << stats.totalFlightTime << "s\n";
    std::cout << "   Average lifetime: " << stats.averageLifetime << "s\n\n";
}

void ShutdownShipContentSystem() {
    std::cout << "=== Shutting Down Ship Content System ===\n\n";
    
    auto& shipSystem = ShipContentSystem::Instance();
    shipSystem.Shutdown();
    
    std::cout << "✓ Ship Content System shutdown complete\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║  Ship Content System - Initialization Demo ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    try {
        // Initialize
        InitializeShipContentSystem();
        
        // Run examples
        QuickDesignExample();
        QuickTemplateExample();
        QuickAnalyticsExample();
        
        // Shutdown
        ShutdownShipContentSystem();
        
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║  ✓ All examples completed successfully ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}
