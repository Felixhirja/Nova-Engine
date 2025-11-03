// Example implementation showing how to use the Content Architecture system
// This file demonstrates all major features

#include "ContentFramework.h"
#include "ContentSchema.h"
#include "ContentValidator.h"
#include "ContentDependencyGraph.h"
#include "ContentCompositor.h"
#include <iostream>

using namespace NovaEngine;

// Example 1: Define a custom content type
class WeaponContent : public ContentDefinition {
public:
    WeaponContent(const std::string& id) : ContentDefinition(id, "weapon") {}
    
    // Data
    std::string name;
    float damage = 0.0f;
    float fireRate = 0.0f;
    float range = 0.0f;
    std::string projectileType;
    
    // Serialization
    SimpleJson ToJson() const override {
        SimpleJson json;
        json.Set("id", GetId());
        json.Set("type", GetType());
        json.Set("name", name);
        json.Set("damage", damage);
        json.Set("fireRate", fireRate);
        json.Set("range", range);
        json.Set("projectileType", projectileType);
        return json;
    }
    
    bool FromJson(const SimpleJson& json) override {
        auto nameNode = json.Get("name");
        if (nameNode) name = nameNode->AsString();
        
        auto damageNode = json.Get("damage");
        if (damageNode) damage = static_cast<float>(damageNode->AsNumber());
        
        auto fireRateNode = json.Get("fireRate");
        if (fireRateNode) fireRate = static_cast<float>(fireRateNode->AsNumber());
        
        auto rangeNode = json.Get("range");
        if (rangeNode) range = static_cast<float>(rangeNode->AsNumber());
        
        auto projNode = json.Get("projectileType");
        if (projNode) projectileType = projNode->AsString();
        
        return true;
    }
    
    // Validation
    bool Validate(std::vector<std::string>& errors) const override {
        bool valid = true;
        
        if (damage <= 0) {
            errors.push_back("Damage must be positive");
            valid = false;
        }
        
        if (fireRate <= 0) {
            errors.push_back("Fire rate must be positive");
            valid = false;
        }
        
        if (range <= 0) {
            errors.push_back("Range must be positive");
            valid = false;
        }
        
        // Calculate DPS
        float dps = damage * fireRate;
        if (dps > 100.0f) {
            errors.push_back("DPS too high: " + std::to_string(dps));
            valid = false;
        }
        
        return valid;
    }
    
    // Dependencies
    std::vector<std::string> GetDependencies() const override {
        // This weapon depends on its projectile type
        if (!projectileType.empty()) {
            return {projectileType};
        }
        return {};
    }
    
    // Cloning
    std::unique_ptr<ContentDefinition> Clone() const override {
        auto clone = std::make_unique<WeaponContent>(GetId());
        clone->name = name;
        clone->damage = damage;
        clone->fireRate = fireRate;
        clone->range = range;
        clone->projectileType = projectileType;
        return clone;
    }
};

// Register the weapon type
REGISTER_CONTENT_TYPE(WeaponContent, "weapon");

// Example 2: Create and register schema
void RegisterWeaponSchema() {
    auto schema = BEGIN_CONTENT_SCHEMA("weapon", "Weapon system definition")
        SCHEMA_FIELD("name", String)
            SCHEMA_REQUIRED()
            SCHEMA_DESC("Display name of the weapon")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("damage", Float)
            SCHEMA_REQUIRED()
            SCHEMA_MIN(1.0f)
            SCHEMA_MAX(100.0f)
            SCHEMA_DEFAULT(10.0f)
            SCHEMA_DESC("Damage per shot")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("fireRate", Float)
            SCHEMA_REQUIRED()
            SCHEMA_MIN(0.1f)
            SCHEMA_MAX(10.0f)
            SCHEMA_DEFAULT(1.0f)
            SCHEMA_DESC("Shots per second")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("range", Float)
            SCHEMA_REQUIRED()
            SCHEMA_MIN(10.0f)
            SCHEMA_MAX(1000.0f)
            SCHEMA_DEFAULT(100.0f)
            SCHEMA_DESC("Maximum effective range")
            END_SCHEMA_FIELD()
        
        SCHEMA_FIELD("projectileType", String)
            SCHEMA_REFERENCE("projectile")
            SCHEMA_DESC("Reference to projectile content")
            END_SCHEMA_FIELD()
        
        .Build();
    
    REGISTER_SCHEMA(schema);
}

// Example 3: Create custom validator
void SetupWeaponValidation() {
    // Balance validator
    auto balanceValidator = std::make_shared<BalanceValidator>("weapon");
    
    BalanceValidator::BalanceRule dpsRule;
    dpsRule.fieldName = "damage";
    dpsRule.minValue = 1.0f;
    dpsRule.maxValue = 100.0f;
    dpsRule.recommendedMin = 5.0f;
    dpsRule.recommendedMax = 50.0f;
    balanceValidator->AddRule(dpsRule);
    
    ContentValidatorRegistry::Instance().RegisterValidator(balanceValidator);
    
    // Custom DPS validator
    auto dpsValidator = std::make_shared<CustomValidator>(
        "DPS Validator",
        "Ensures weapon DPS is balanced",
        "weapon",
        [](const ContentDefinition& content, std::vector<ValidationResult>& results) {
            const auto& weapon = static_cast<const WeaponContent&>(content);
            
            float dps = weapon.damage * weapon.fireRate;
            
            if (dps < 5.0f) {
                results.emplace_back("dps", 
                    "DPS too low: " + std::to_string(dps) + " (recommend 5-50)",
                    ValidationSeverity::Warning);
            } else if (dps > 50.0f) {
                results.emplace_back("dps",
                    "DPS too high: " + std::to_string(dps) + " (recommend 5-50)",
                    ValidationSeverity::Error);
                return false;
            }
            
            return true;
        }
    );
    
    ContentValidatorRegistry::Instance().RegisterValidator(dpsValidator);
}

// Example 4: Complete workflow demonstration
void ContentArchitectureExample() {
    std::cout << "=== Nova Engine Content Architecture Example ===" << std::endl;
    
    // 1. Initialize framework
    std::cout << "\n1. Initializing Content Framework..." << std::endl;
    auto& framework = ContentFramework::Instance();
    framework.Initialize();
    
    // 2. Register schema
    std::cout << "2. Registering weapon schema..." << std::endl;
    RegisterWeaponSchema();
    
    // 3. Setup validation
    std::cout << "3. Setting up validation..." << std::endl;
    SetupWeaponValidation();
    
    // 4. Create some content programmatically
    std::cout << "4. Creating weapon content..." << std::endl;
    
    auto laserBase = std::make_unique<WeaponContent>("laser_base");
    laserBase->name = "Basic Laser";
    laserBase->damage = 10.0f;
    laserBase->fireRate = 2.0f;
    laserBase->range = 100.0f;
    laserBase->projectileType = "laser_bolt";
    
    auto laserUpgrade = std::make_unique<WeaponContent>("laser_upgrade");
    laserUpgrade->name = "Laser Damage Upgrade";
    laserUpgrade->damage = 15.0f;  // Additional damage
    
    ContentRegistry::Instance().RegisterContent(std::move(laserBase));
    ContentRegistry::Instance().RegisterContent(std::move(laserUpgrade));
    
    // 5. Validate content
    std::cout << "5. Validating content..." << std::endl;
    std::vector<ValidationResult> validationResults;
    auto* weapon = ContentRegistry::Instance().GetContent("laser_base");
    if (weapon) {
        ContentValidatorRegistry::Instance().ValidateContent(*weapon, validationResults);
        
        std::cout << "   Validation results:" << std::endl;
        for (const auto& result : validationResults) {
            std::cout << "   - " << result.field << ": " << result.message << std::endl;
        }
    }
    
    // 6. Create composition
    std::cout << "\n6. Creating weapon variant through composition..." << std::endl;
    auto heavyLaser = CompositionBuilder("heavy_laser")
        .AddBase("laser_base")
        .AddBase("laser_upgrade")
        .WithStrategy("damage", CompositionStrategy::Add)
        .Build();
    
    if (heavyLaser) {
        std::cout << "   Created heavy laser variant" << std::endl;
        ContentRegistry::Instance().RegisterContent(std::move(heavyLaser));
    }
    
    // 7. Build dependency graph
    std::cout << "\n7. Building dependency graph..." << std::endl;
    auto& depGraph = ContentDependencyGraph::Instance();
    depGraph.BuildGraph();
    
    auto stats = depGraph.GetStatistics();
    std::cout << "   Nodes: " << stats.nodeCount << std::endl;
    std::cout << "   Edges: " << stats.edgeCount << std::endl;
    
    // 8. Track analytics
    std::cout << "\n8. Tracking analytics..." << std::endl;
    auto& analytics = ContentAnalytics::Instance();
    analytics.TrackLoad("laser_base", 10.5);
    analytics.TrackAccess("laser_base");
    analytics.TrackAccess("laser_base");
    
    auto contentStats = analytics.GetContentStats("laser_base");
    std::cout << "   Laser Base Stats:" << std::endl;
    std::cout << "   - Loads: " << contentStats.loadCount << std::endl;
    std::cout << "   - Accesses: " << contentStats.accessCount << std::endl;
    
    // 9. Generate reports
    std::cout << "\n9. Generating reports..." << std::endl;
    
    std::unordered_map<std::string, std::vector<ValidationResult>> allValidation;
    ContentValidatorRegistry::Instance().ValidateAllContent(allValidation);
    
    auto report = ContentValidatorRegistry::Instance().GenerateReport(allValidation);
    std::cout << "\n" << report << std::endl;
    
    // 10. Content statistics
    std::cout << "\n10. Content Statistics:" << std::endl;
    auto frameworkStats = framework.GetContentStats();
    std::cout << "    Total content: " << frameworkStats.totalContent << std::endl;
    std::cout << "    Total loads: " << frameworkStats.totalLoads << std::endl;
    std::cout << "    Total usage: " << frameworkStats.totalUsage << std::endl;
    
    std::cout << "\n=== Example Complete ===" << std::endl;
}

// Main function if running as standalone
#ifdef CONTENT_EXAMPLE_STANDALONE
int main() {
    try {
        ContentArchitectureExample();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
#endif
