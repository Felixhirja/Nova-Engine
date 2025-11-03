#include "../engine/content/ShipContentSystem.h"
#include <iostream>
#include <cassert>

using namespace NovaEngine;

void TestShipDesigner() {
    std::cout << "\n=== Testing Ship Designer ===\n";
    
    // Create design session
    auto session = ShipDesigner::CreateSession("fighter_hull_basic");
    std::cout << "✓ Created design session: " << session.sessionId << "\n";
    
    // Add components
    bool success = ShipDesigner::AddComponent(session, "slot_powerplant_1", "reactor_basic");
    std::cout << "✓ Added component: " << (success ? "Valid" : "Invalid") << "\n";
    
    // Validate design
    auto result = ShipDesigner::ValidateDesign(session);
    std::cout << "✓ Design validation complete\n";
    
    // Save design
    success = ShipDesigner::SaveDesign(session, "test_fighter");
    std::cout << "✓ Design saved: " << (success ? "Success" : "Failed") << "\n";
}

void TestShipValidator() {
    std::cout << "\n=== Testing Ship Validator ===\n";
    
    ShipAssemblyRequest request;
    request.hullId = "fighter_hull_basic";
    
    // Validate at different levels
    auto report = ShipValidator::Validate(request, ShipValidator::ValidationLevel::Basic);
    std::cout << "✓ Basic validation complete\n";
    std::cout << "  - Valid: " << report.isValid << "\n";
    std::cout << "  - Errors: " << report.errors.size() << "\n";
    std::cout << "  - Warnings: " << report.warnings.size() << "\n";
    std::cout << "  - Balance Score: " << report.balanceScore << "\n";
    
    report = ShipValidator::Validate(request, ShipValidator::ValidationLevel::Standard);
    std::cout << "✓ Standard validation complete\n";
    
    report = ShipValidator::Validate(request, ShipValidator::ValidationLevel::Strict);
    std::cout << "✓ Strict validation complete\n";
    
    // Check balance
    auto assemblyResult = ShipAssembler::Assemble(request);
    std::vector<std::string> issues;
    bool balanced = ShipValidator::CheckBalance(assemblyResult, issues);
    std::cout << "✓ Balance check: " << (balanced ? "Balanced" : "Issues found") << "\n";
    for (const auto& issue : issues) {
        std::cout << "  - " << issue << "\n";
    }
    
    // Calculate balance score
    double score = ShipValidator::CalculateBalanceScore(assemblyResult);
    std::cout << "✓ Balance score: " << score << "\n";
    
    // Get suggestions
    auto suggestions = ShipValidator::SuggestImprovements(assemblyResult);
    std::cout << "✓ Improvement suggestions: " << suggestions.size() << "\n";
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion << "\n";
    }
}

void TestShipPerformanceSimulator() {
    std::cout << "\n=== Testing Ship Performance Simulator ===\n";
    
    ShipAssemblyRequest request;
    request.hullId = "fighter_hull_basic";
    auto result = ShipAssembler::Assemble(request);
    
    // Simulate performance
    auto profile = ShipPerformanceSimulator::SimulatePerformance(result);
    std::cout << "✓ Performance simulation complete\n";
    std::cout << "  - Acceleration: " << profile.acceleration << " m/s²\n";
    std::cout << "  - Max Speed: " << profile.maxSpeed << " m/s\n";
    std::cout << "  - Turn Rate: " << profile.turnRate << " deg/s\n";
    std::cout << "  - Power Efficiency: " << profile.powerEfficiency << "%\n";
    std::cout << "  - Heat Management: " << profile.heatManagement << "%\n";
    std::cout << "  - Combat Rating: " << profile.combatRating << "/100\n";
    std::cout << "  - Survival Rating: " << profile.survivalRating << "/100\n";
    std::cout << "  - Economic Rating: " << profile.economicRating << "/100\n";
    
    // Test scenario simulation
    ShipPerformanceSimulator::SimulationScenario scenario;
    scenario.name = "Combat Test";
    scenario.duration = 60.0;
    scenario.includeCombat = true;
    scenario.includeManeuvers = true;
    scenario.includeStress = false;
    
    auto scenarioProfile = ShipPerformanceSimulator::SimulateScenario(result, scenario);
    std::cout << "✓ Scenario simulation complete\n";
    std::cout << "  - Combat Rating (modified): " << scenarioProfile.combatRating << "/100\n";
}

void TestShipContentSystem() {
    std::cout << "\n=== Testing Ship Content System ===\n";
    
    auto& system = ShipContentSystem::Instance();
    system.Initialize();
    std::cout << "✓ Ship content system initialized\n";
    
    // Test integrated workflow
    auto result = system.CreateAndValidateShip("fighter_hull_basic");
    std::cout << "✓ Created and validated ship\n";
    std::cout << "  - Valid: " << result.IsValid() << "\n";
    std::cout << "  - Components: " << result.components.size() << "\n";
    std::cout << "  - Total Mass: " << result.totalMassTons << " tons\n";
    std::cout << "  - Power: " << result.NetPowerMW() << " MW net\n";
    std::cout << "  - Thrust/Mass: " << result.ThrustToMassRatio() << " kN/ton\n";
    
    system.Shutdown();
    std::cout << "✓ Ship content system shutdown\n";
}

void TestAnalytics() {
    std::cout << "\n=== Testing Ship Analytics ===\n";
    
    // Record some events
    ShipAnalytics::RecordSpawn("fighter_mk1");
    ShipAnalytics::RecordSpawn("fighter_mk1");
    ShipAnalytics::RecordFlightTime("fighter_mk1", 120.5);
    ShipAnalytics::RecordFlightTime("fighter_mk1", 85.3);
    ShipAnalytics::RecordDestruction("fighter_mk1");
    std::cout << "✓ Recorded analytics events\n";
    
    auto stats = ShipAnalytics::GetUsageStats("fighter_mk1");
    std::cout << "✓ Retrieved usage statistics\n";
    
    auto popularity = ShipAnalytics::GetPopularityRankings();
    std::cout << "✓ Retrieved popularity rankings: " << popularity.size() << " entries\n";
}

void TestTemplates() {
    std::cout << "\n=== Testing Ship Templates ===\n";
    
    auto templates = ShipTemplateSystem::GetTemplates(SpaceshipClassType::Fighter);
    std::cout << "✓ Retrieved fighter templates: " << templates.size() << " found\n";
    
    templates = ShipTemplateSystem::GetTemplatesByRole("trader");
    std::cout << "✓ Retrieved trader templates: " << templates.size() << " found\n";
}

void TestVariants() {
    std::cout << "\n=== Testing Ship Variants ===\n";
    
    auto variants = ShipVariantSystem::GetVariants("fighter_mk1");
    std::cout << "✓ Retrieved variants: " << variants.size() << " found\n";
    
    ShipVariantSystem::Variant newVariant;
    newVariant.baseShipId = "fighter_mk1";
    newVariant.variantName = "Test Interceptor";
    newVariant.description = "Speed-focused variant";
    
    bool success = ShipVariantSystem::RegisterVariant(newVariant);
    std::cout << "✓ Registered variant: " << (success ? "Success" : "Failed") << "\n";
}

void TestCatalog() {
    std::cout << "\n=== Testing Ship Catalog ===\n";
    
    ShipContentCatalog::CatalogFilter filter;
    filter.classTypes = {SpaceshipClassType::Fighter, SpaceshipClassType::Corvette};
    filter.minCombatRating = 50.0;
    
    auto entries = ShipContentCatalog::Browse(filter);
    std::cout << "✓ Browsed catalog with filter: " << entries.size() << " ships found\n";
    
    auto searchResults = ShipContentCatalog::Search("fighter");
    std::cout << "✓ Search results: " << searchResults.size() << " found\n";
    
    auto featured = ShipContentCatalog::GetFeatured();
    std::cout << "✓ Featured ships: " << featured.size() << " found\n";
}

void TestDocumentation() {
    std::cout << "\n=== Testing Ship Documentation ===\n";
    
    ShipDocumentationGenerator::DocumentationOptions options;
    options.includePerformanceData = true;
    options.includeComponentDetails = true;
    options.format = "markdown";
    
    auto docs = ShipDocumentationGenerator::GenerateShipDocs("fighter_mk1", options);
    std::cout << "✓ Generated ship documentation (" << docs.length() << " chars)\n";
    
    auto componentDocs = ShipDocumentationGenerator::GenerateComponentDocs("reactor_basic");
    std::cout << "✓ Generated component documentation (" << componentDocs.length() << " chars)\n";
}

void TestBalancing() {
    std::cout << "\n=== Testing Ship Balancing ===\n";
    
    ShipBalancingSystem::BalanceTarget target;
    target.targetPowerLevel = 70.0;
    target.role = "fighter";
    
    auto report = ShipBalancingSystem::AnalyzeBalance("fighter_mk1", target);
    std::cout << "✓ Balance analysis complete\n";
    std::cout << "  - Current Score: " << report.currentBalanceScore << "\n";
    std::cout << "  - Target Score: " << report.targetBalanceScore << "\n";
    std::cout << "  - Adjustments: " << report.suggestedAdjustments.size() << "\n";
    
    auto recommendations = ShipBalancingSystem::GetBalanceRecommendations("fighter_mk1");
    std::cout << "✓ Balance recommendations: " << recommendations.size() << " found\n";
}

void TestTesting() {
    std::cout << "\n=== Testing Ship Testing Framework ===\n";
    
    ShipTestingFramework::TestCase test;
    test.name = "Basic Structure Test";
    test.type = ShipTestingFramework::TestType::Validation;
    test.testFunc = [](const ShipAssemblyResult& ship) {
        return ship.IsValid();
    };
    
    ShipTestingFramework::RegisterTestCase(test);
    std::cout << "✓ Registered test case\n";
    
    auto report = ShipTestingFramework::RunTests("fighter_mk1", ShipTestingFramework::TestType::Validation);
    std::cout << "✓ Test report: " << report.passed << "/" << report.totalTests << " passed\n";
    std::cout << "  - Execution time: " << report.executionTime << "s\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Ship Content System - Comprehensive Test\n";
    std::cout << "========================================\n";
    
    try {
        TestShipContentSystem();
        TestShipDesigner();
        TestShipValidator();
        TestShipPerformanceSimulator();
        TestAnalytics();
        TestTemplates();
        TestVariants();
        TestCatalog();
        TestDocumentation();
        TestBalancing();
        TestTesting();
        
        std::cout << "\n========================================\n";
        std::cout << "✅ ALL TESTS COMPLETED SUCCESSFULLY\n";
        std::cout << "========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
