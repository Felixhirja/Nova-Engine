#pragma once

#include "../ecs/ShipAssembly.h"
#include "../gameplay/SpaceshipCatalog.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace NovaEngine {

// Ship Designer: Visual ship designer with modular component system
class ShipDesigner {
public:
    struct DesignSession {
        std::string sessionId;
        std::string hullId;
        ShipAssemblyRequest currentDesign;
        ShipAssemblyResult lastValidation;
        std::vector<std::string> undoStack;
        std::vector<std::string> redoStack;
    };
    
    static DesignSession CreateSession(const std::string& hullId);
    static bool AddComponent(DesignSession& session, const std::string& slotId, const std::string& componentId);
    static bool RemoveComponent(DesignSession& session, const std::string& slotId);
    static bool ReplaceComponent(DesignSession& session, const std::string& slotId, const std::string& newComponentId);
    static ShipAssemblyResult ValidateDesign(const DesignSession& session);
    static bool SaveDesign(const DesignSession& session, const std::string& name);
    static DesignSession LoadDesign(const std::string& name);
    static void Undo(DesignSession& session);
    static void Redo(DesignSession& session);
};

// Ship Validation: Validation for ship configurations and balance
class ShipValidator {
public:
    enum class ValidationLevel {
        Basic,      // Structure and requirements only
        Standard,   // + Performance checks
        Strict,     // + Balance checks
        Tournament  // + Meta-game balance
    };
    
    struct ValidationReport {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
        ShipPerformanceMetrics metrics;
        double balanceScore; // 0.0-1.0, higher is better balanced
    };
    
    static ValidationReport Validate(const ShipAssemblyRequest& request, ValidationLevel level = ValidationLevel::Standard);
    static bool CheckBalance(const ShipAssemblyResult& result, std::vector<std::string>& issues);
    static double CalculateBalanceScore(const ShipAssemblyResult& result);
    static std::vector<std::string> SuggestImprovements(const ShipAssemblyResult& result);
};

// Ship Performance: Performance simulation and optimization
class ShipPerformanceSimulator {
public:
    struct PerformanceProfile {
        double acceleration;        // m/s²
        double maxSpeed;           // m/s
        double turnRate;           // deg/s
        double powerEfficiency;    // %
        double heatManagement;     // %
        double combatRating;       // 0-100
        double survivalRating;     // 0-100
        double economicRating;     // 0-100
    };
    
    struct SimulationScenario {
        std::string name;
        double duration;           // seconds
        bool includeCombat;
        bool includeManeuvers;
        bool includeStress;
    };
    
    static PerformanceProfile SimulatePerformance(const ShipAssemblyResult& ship);
    static PerformanceProfile SimulateScenario(const ShipAssemblyResult& ship, const SimulationScenario& scenario);
    static std::vector<std::pair<std::string, double>> CompareShips(const std::vector<ShipAssemblyResult>& ships);
    static ShipAssemblyRequest OptimizeDesign(const ShipAssemblyRequest& request, const std::string& optimizationGoal);
};

// Ship Variants: Advanced variant system for ship customization
class ShipVariantSystem {
public:
    struct Variant {
        std::string baseShipId;
        std::string variantName;
        std::string description;
        std::unordered_map<std::string, std::string> componentOverrides;
        std::vector<PassiveBuff> buffs;
    };
    
    static std::vector<Variant> GetVariants(const std::string& baseShipId);
    static ShipAssemblyRequest ApplyVariant(const ShipAssemblyRequest& base, const Variant& variant);
    static bool RegisterVariant(const Variant& variant);
    static Variant CreateVariant(const std::string& baseShipId, const std::string& name, const ShipAssemblyRequest& modified);
};

// Ship Templates: Template system for rapid ship creation
class ShipTemplateSystem {
public:
    struct Template {
        std::string id;
        std::string name;
        std::string description;
        SpaceshipClassType shipClass;
        std::string role; // "fighter", "trader", "miner", etc.
        ShipAssemblyRequest assemblyTemplate;
        std::vector<std::string> tags;
    };
    
    static std::vector<Template> GetTemplates(SpaceshipClassType classType = SpaceshipClassType::Fighter);
    static std::vector<Template> GetTemplatesByRole(const std::string& role);
    static Template GetTemplate(const std::string& id);
    static ShipAssemblyRequest InstantiateTemplate(const std::string& templateId);
    static bool SaveTemplate(const Template& tmpl);
    static bool DeleteTemplate(const std::string& id);
};

// Ship Catalog: Comprehensive catalog system for ship browsing
class ShipContentCatalog {
public:
    struct CatalogFilter {
        std::vector<SpaceshipClassType> classTypes;
        std::vector<std::string> roles;
        std::vector<std::string> factions;
        double minCost = 0.0;
        double maxCost = 1e9;
        double minCombatRating = 0.0;
        double maxCombatRating = 100.0;
        std::vector<std::string> requiredFeatures;
    };
    
    struct CatalogEntry {
        std::string id;
        std::string displayName;
        SpaceshipClassType classType;
        std::string role;
        std::string faction;
        double cost;
        double combatRating;
        std::string thumbnail;
        std::vector<std::string> features;
        ShipAssemblyRequest assembly;
    };
    
    static std::vector<CatalogEntry> Browse(const CatalogFilter& filter);
    static CatalogEntry GetEntry(const std::string& id);
    static std::vector<CatalogEntry> Search(const std::string& query);
    static std::vector<CatalogEntry> GetFeatured();
    static std::vector<CatalogEntry> GetRecommended(const std::string& playerId);
};

// Ship Analytics: Analytics for ship usage and player preferences
class ShipAnalytics {
public:
    struct UsageStats {
        std::string shipId;
        uint64_t timesSpawned;
        uint64_t timesDestroyed;
        double averageLifetime; // seconds
        double totalFlightTime; // seconds
        double killDeathRatio;
        std::unordered_map<std::string, uint64_t> componentUsage;
    };
    
    struct PopularityMetrics {
        std::string shipId;
        double popularityScore;
        double winRate;
        double survivalRate;
        int playerCount;
    };
    
    static void RecordSpawn(const std::string& shipId);
    static void RecordDestruction(const std::string& shipId);
    static void RecordFlightTime(const std::string& shipId, double seconds);
    static UsageStats GetUsageStats(const std::string& shipId);
    static std::vector<PopularityMetrics> GetPopularityRankings();
    static std::vector<std::pair<std::string, uint64_t>> GetMostUsedComponents();
    static void ExportAnalytics(const std::string& filepath);
};

// Ship Documentation: Documentation generation for ship content
class ShipDocumentationGenerator {
public:
    struct DocumentationOptions {
        bool includePerformanceData = true;
        bool includeComponentDetails = true;
        bool includeImages = false;
        bool includeComparisons = false;
        std::string format = "markdown"; // "markdown", "html", "json"
    };
    
    static std::string GenerateShipDocs(const std::string& shipId, const DocumentationOptions& options);
    static std::string GenerateComponentDocs(const std::string& componentId);
    static std::string GenerateCatalogDocs(const DocumentationOptions& options);
    static bool ExportDocumentation(const std::string& outputDir, const DocumentationOptions& options);
};

// Ship Testing: Automated testing for ship configurations
class ShipTestingFramework {
public:
    enum class TestType {
        Validation,     // Structure and requirements
        Performance,    // Performance metrics
        Balance,        // Balance checks
        Integration,    // System integration
        Regression      // Regression testing
    };
    
    struct TestCase {
        std::string name;
        TestType type;
        std::function<bool(const ShipAssemblyResult&)> testFunc;
        std::string expectedResult;
    };
    
    struct TestReport {
        std::string testSuiteName;
        int totalTests;
        int passed;
        int failed;
        std::vector<std::string> failureReasons;
        double executionTime; // seconds
    };
    
    static void RegisterTestCase(const TestCase& test);
    static TestReport RunTests(const std::string& shipId, TestType type = TestType::Validation);
    static TestReport RunAllTests(const std::string& shipId);
    static TestReport RunTestSuite(const std::vector<std::string>& shipIds);
    static void ExportTestResults(const std::string& filepath);
};

// Ship Balancing: AI-powered balancing for ship configurations
class ShipBalancingSystem {
public:
    struct BalanceTarget {
        double targetPowerLevel;
        double targetCost;
        std::string role;
        std::vector<std::string> competitors; // Similar ships to balance against
    };
    
    struct BalanceAdjustment {
        std::string componentId;
        std::string property;
        double currentValue;
        double suggestedValue;
        std::string reasoning;
    };
    
    struct BalanceReport {
        std::string shipId;
        double currentBalanceScore;
        double targetBalanceScore;
        std::vector<BalanceAdjustment> suggestedAdjustments;
        std::string analysis;
    };
    
    static BalanceReport AnalyzeBalance(const std::string& shipId, const BalanceTarget& target);
    static ShipAssemblyRequest ApplyBalanceAdjustments(const ShipAssemblyRequest& request, const std::vector<BalanceAdjustment>& adjustments);
    static std::vector<BalanceReport> AnalyzeFleetBalance(const std::vector<std::string>& shipIds);
    static void SetBalanceRules(const std::string& rulesJson);
    static std::vector<std::string> GetBalanceRecommendations(const std::string& shipId);
};

// Master ship content system integrating all subsystems
class ShipContentSystem {
public:
    static ShipContentSystem& Instance() {
        static ShipContentSystem instance;
        return instance;
    }
    
    void Initialize();
    void Shutdown();
    
    // Quick access to subsystems
    ShipDesigner& Designer() { return designer_; }
    ShipValidator& Validator() { return validator_; }
    ShipPerformanceSimulator& Performance() { return performance_; }
    ShipVariantSystem& Variants() { return variants_; }
    ShipTemplateSystem& Templates() { return templates_; }
    ShipContentCatalog& Catalog() { return catalog_; }
    ShipAnalytics& Analytics() { return analytics_; }
    ShipDocumentationGenerator& Documentation() { return documentation_; }
    ShipTestingFramework& Testing() { return testing_; }
    ShipBalancingSystem& Balancing() { return balancing_; }
    
    // Integrated workflows
    ShipAssemblyResult CreateAndValidateShip(const std::string& hullId, const std::string& templateId = "");
    bool PublishShipDesign(const ShipDesigner::DesignSession& session, const std::string& name);
    void RefreshCatalog();
    
private:
    ShipContentSystem() = default;
    
    ShipDesigner designer_;
    ShipValidator validator_;
    ShipPerformanceSimulator performance_;
    ShipVariantSystem variants_;
    ShipTemplateSystem templates_;
    ShipContentCatalog catalog_;
    ShipAnalytics analytics_;
    ShipDocumentationGenerator documentation_;
    ShipTestingFramework testing_;
    ShipBalancingSystem balancing_;
};

} // namespace NovaEngine
