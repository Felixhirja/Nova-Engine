#include "ConfigIntegration.h"
#include <sstream>
#include <iostream>

namespace nova::config {

// ============================================================================
// Schema Registration
// ============================================================================

void ConfigIntegration::RegisterBuiltInSchemas() {
    auto& registry = ConfigSchemaRegistry::GetInstance();
    
    registry.RegisterSchema("PlayerConfig", CreatePlayerConfigSchema());
    registry.RegisterSchema("BootstrapConfiguration", CreateBootstrapSchema());
    registry.RegisterSchema("ActorConfig", CreateActorConfigSchema());
    registry.RegisterSchema("StationConfig", CreateStationConfigSchema());
}

ConfigSchema ConfigIntegration::CreatePlayerConfigSchema() {
    ConfigSchema schema("PlayerConfig", "1.0");
    schema.AddDescription("Player entity configuration including spawn, movement, physics, and visuals");
    
    // Spawn position
    ConfigSchema::Field spawnX;
    spawnX.name = "spawnPosition.x";
    spawnX.type = ConfigValueType::Float;
    spawnX.defaultValue = 0.0;
    schema.AddField(spawnX);
    
    ConfigSchema::Field spawnY;
    spawnY.name = "spawnPosition.y";
    spawnY.type = ConfigValueType::Float;
    spawnY.defaultValue = 0.0;
    schema.AddField(spawnY);
    
    ConfigSchema::Field spawnZ;
    spawnZ.name = "spawnPosition.z";
    spawnZ.type = ConfigValueType::Float;
    spawnZ.defaultValue = 0.0;
    schema.AddField(spawnZ);
    
    // Movement
    ConfigSchema::Field forwardSpeed;
    forwardSpeed.name = "movement.forwardSpeed";
    forwardSpeed.type = ConfigValueType::Float;
    forwardSpeed.minValue = 0.0;
    forwardSpeed.maxValue = 100.0;
    forwardSpeed.defaultValue = 5.0;
    schema.AddField(forwardSpeed);
    
    ConfigSchema::Field acceleration;
    acceleration.name = "movement.acceleration";
    acceleration.type = ConfigValueType::Float;
    acceleration.minValue = 0.0;
    acceleration.defaultValue = 4.0;
    schema.AddField(acceleration);
    
    // Physics
    ConfigSchema::Field enableGravity;
    enableGravity.name = "physics.enableGravity";
    enableGravity.type = ConfigValueType::Boolean;
    enableGravity.defaultValue = false;
    schema.AddField(enableGravity);
    
    // Visual
    ConfigSchema::Field colorR;
    colorR.name = "visual.r";
    colorR.type = ConfigValueType::Float;
    colorR.minValue = 0.0;
    colorR.maxValue = 1.0;
    colorR.defaultValue = 0.2;
    schema.AddField(colorR);
    
    ConfigSchema::Field scale;
    scale.name = "visual.scale";
    scale.type = ConfigValueType::Float;
    scale.minValue = 0.1;
    scale.maxValue = 10.0;
    scale.defaultValue = 0.5;
    schema.AddField(scale);
    
    return schema;
}

ConfigSchema ConfigIntegration::CreateBootstrapSchema() {
    ConfigSchema schema("BootstrapConfiguration", "1.0");
    schema.AddDescription("Engine bootstrap settings for framework initialization");
    
    ConfigSchema::Field loadInput;
    loadInput.name = "loadInput";
    loadInput.type = ConfigValueType::Boolean;
    loadInput.required = true;
    loadInput.defaultValue = true;
    schema.AddField(loadInput);
    
    ConfigSchema::Field loadAudio;
    loadAudio.name = "loadAudio";
    loadAudio.type = ConfigValueType::Boolean;
    loadAudio.required = true;
    loadAudio.defaultValue = true;
    schema.AddField(loadAudio);
    
    ConfigSchema::Field loadRendering;
    loadRendering.name = "loadRendering";
    loadRendering.type = ConfigValueType::Boolean;
    loadRendering.required = true;
    loadRendering.defaultValue = true;
    schema.AddField(loadRendering);
    
    return schema;
}

ConfigSchema ConfigIntegration::CreateActorConfigSchema() {
    ConfigSchema schema("ActorConfig", "1.0");
    schema.AddDescription("Base actor configuration");
    
    ConfigSchema::Field actorType;
    actorType.name = "actorType";
    actorType.type = ConfigValueType::String;
    actorType.required = true;
    schema.AddField(actorType);
    
    ConfigSchema::Field initialHealth;
    initialHealth.name = "initialHealth";
    initialHealth.type = ConfigValueType::Float;
    initialHealth.minValue = 0.0;
    initialHealth.defaultValue = 100.0;
    schema.AddField(initialHealth);
    
    return schema;
}

ConfigSchema ConfigIntegration::CreateStationConfigSchema() {
    ConfigSchema schema("StationConfig", "1.0");
    schema.AddDescription("Space station configuration");
    
    // Inherit from ActorConfig
    auto baseSchema = CreateActorConfigSchema();
    for (const auto& field : baseSchema.GetFields()) {
        schema.AddField(field);
    }
    
    ConfigSchema::Field stationType;
    stationType.name = "stationType";
    stationType.type = ConfigValueType::String;
    stationType.allowedValues = {"research", "trading", "military", "mining"};
    stationType.defaultValue = std::string("research");
    schema.AddField(stationType);
    
    ConfigSchema::Field dockingPorts;
    dockingPorts.name = "dockingPorts";
    dockingPorts.type = ConfigValueType::Integer;
    dockingPorts.minValue = 1.0;
    dockingPorts.maxValue = 20.0;
    dockingPorts.defaultValue = 4;
    schema.AddField(dockingPorts);
    
    return schema;
}

// ============================================================================
// Legacy Adapters
// ============================================================================

std::shared_ptr<Configuration> ConfigIntegration::FromPlayerConfig(const PlayerConfig& legacy) {
    auto config = std::make_shared<Configuration>("PlayerConfig");
    
    simplejson::JsonObject json;
    
    // Spawn position
    simplejson::JsonObject spawnPos;
    spawnPos["x"] = simplejson::JsonValue(legacy.spawnPosition.x);
    spawnPos["y"] = simplejson::JsonValue(legacy.spawnPosition.y);
    spawnPos["z"] = simplejson::JsonValue(legacy.spawnPosition.z);
    json["spawnPosition"] = simplejson::JsonValue(spawnPos);
    
    // Movement
    simplejson::JsonObject movement;
    movement["forwardSpeed"] = simplejson::JsonValue(legacy.movement.forwardSpeed);
    movement["backwardSpeed"] = simplejson::JsonValue(legacy.movement.backwardSpeed);
    movement["strafeSpeed"] = simplejson::JsonValue(legacy.movement.strafeSpeed);
    movement["acceleration"] = simplejson::JsonValue(legacy.movement.acceleration);
    json["movement"] = simplejson::JsonValue(movement);
    
    // Physics
    simplejson::JsonObject physics;
    physics["enableGravity"] = simplejson::JsonValue(legacy.physics.enableGravity);
    physics["gravityStrength"] = simplejson::JsonValue(legacy.physics.gravityStrength);
    json["physics"] = simplejson::JsonValue(physics);
    
    // Visual
    simplejson::JsonObject visual;
    visual["r"] = simplejson::JsonValue(static_cast<double>(legacy.visual.r));
    visual["g"] = simplejson::JsonValue(static_cast<double>(legacy.visual.g));
    visual["b"] = simplejson::JsonValue(static_cast<double>(legacy.visual.b));
    visual["scale"] = simplejson::JsonValue(static_cast<double>(legacy.visual.scale));
    json["visual"] = simplejson::JsonValue(visual);
    
    config->LoadFromJson(json);
    return config;
}

PlayerConfig ConfigIntegration::ToPlayerConfig(const Configuration& config) {
    PlayerConfig result;
    
    // Spawn position
    result.spawnPosition.x = config.Get<double>("spawnPosition.x", 0.0);
    result.spawnPosition.y = config.Get<double>("spawnPosition.y", 0.0);
    result.spawnPosition.z = config.Get<double>("spawnPosition.z", 0.0);
    
    // Movement
    result.movement.forwardSpeed = config.Get<double>("movement.forwardSpeed", 5.0);
    result.movement.backwardSpeed = config.Get<double>("movement.backwardSpeed", 5.0);
    result.movement.strafeSpeed = config.Get<double>("movement.strafeSpeed", 5.0);
    result.movement.acceleration = config.Get<double>("movement.acceleration", 4.0);
    
    // Physics
    result.physics.enableGravity = config.Get<bool>("physics.enableGravity", false);
    result.physics.gravityStrength = config.Get<double>("physics.gravityStrength", -9.8);
    
    // Visual
    result.visual.r = static_cast<float>(config.Get<double>("visual.r", 0.2));
    result.visual.g = static_cast<float>(config.Get<double>("visual.g", 0.8));
    result.visual.b = static_cast<float>(config.Get<double>("visual.b", 1.0));
    result.visual.scale = static_cast<float>(config.Get<double>("visual.scale", 0.5));
    
    return result;
}

std::shared_ptr<Configuration> ConfigIntegration::FromBootstrapConfig(
    const BootstrapConfiguration& legacy) {
    auto config = std::make_shared<Configuration>("BootstrapConfiguration");
    
    simplejson::JsonObject json;
    json["loadInput"] = simplejson::JsonValue(legacy.loadInput);
    json["loadAudio"] = simplejson::JsonValue(legacy.loadAudio);
    json["loadRendering"] = simplejson::JsonValue(legacy.loadRendering);
    
    config->LoadFromJson(json);
    return config;
}

BootstrapConfiguration ConfigIntegration::ToBootstrapConfig(const Configuration& config) {
    BootstrapConfiguration result;
    
    result.loadInput = config.Get<bool>("loadInput", true);
    result.loadAudio = config.Get<bool>("loadAudio", true);
    result.loadRendering = config.Get<bool>("loadRendering", true);
    
    return result;
}

// ============================================================================
// Migrations
// ============================================================================

void ConfigIntegration::RegisterBuiltInMigrations() {
    auto& manager = ConfigMigrationManager::GetInstance();
    
    manager.RegisterMigration("PlayerConfig", CreatePlayerConfigMigration_1_0_to_2_0());
}

ConfigMigration ConfigIntegration::CreatePlayerConfigMigration_1_0_to_2_0() {
    ConfigMigration migration("1.0", "2.0");
    
    migration.AddStep("Add boost mechanics", [](simplejson::JsonObject& config) {
        // Add boost configuration if not present
        auto it = config.find("movement");
        if (it != config.end() && it->second.IsObject()) {
            auto& movement = it->second.AsObject();
            
            if (movement.find("boostMultiplier") == movement.end()) {
                movement["boostMultiplier"] = simplejson::JsonValue(2.0);
            }
            if (movement.find("boostDuration") == movement.end()) {
                movement["boostDuration"] = simplejson::JsonValue(3.0);
            }
        }
        return true;
    });
    
    return migration;
}

// ============================================================================
// Validators
// ============================================================================

bool ConfigIntegration::Validators::PositiveNumber(const ConfigValue& value) {
    return value.AsFloat() > 0.0;
}

bool ConfigIntegration::Validators::NonEmptyString(const ConfigValue& value) {
    return !value.AsString().empty();
}

bool ConfigIntegration::Validators::ValidColor(const ConfigValue& value) {
    double val = value.AsFloat();
    return val >= 0.0 && val <= 1.0;
}

bool ConfigIntegration::Validators::ValidPath(const ConfigValue& value) {
    // Simple path validation
    std::string path = value.AsString();
    return !path.empty() && path.find("..") == std::string::npos;
}

bool ConfigIntegration::Validators::InRange(double min, double max, const ConfigValue& value) {
    double val = value.AsFloat();
    return val >= min && val <= max;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::shared_ptr<Configuration> ConfigIntegration::LoadConfigAuto(const std::string& filePath) {
    // Auto-detect config type from filename or content
    std::string fileName = std::filesystem::path(filePath).filename().string();
    std::string typeName;
    
    if (fileName.find("player") != std::string::npos) {
        typeName = "PlayerConfig";
    } else if (fileName.find("bootstrap") != std::string::npos) {
        typeName = "BootstrapConfiguration";
    } else if (fileName.find("station") != std::string::npos) {
        typeName = "StationConfig";
    } else {
        typeName = "ActorConfig";  // Default
    }
    
    auto& system = ConfigSystem::GetInstance();
    return system.LoadConfig(typeName, filePath);
}

std::string ConfigIntegration::GenerateDocumentation() {
    std::stringstream ss;
    auto& registry = ConfigSchemaRegistry::GetInstance();
    
    ss << "# Nova Engine Configuration Documentation\n\n";
    ss << "Auto-generated from configuration schemas.\n\n";
    
    for (const auto& info : registry.GetSchemaInfo()) {
        const auto* schema = registry.GetSchema(info.typeName);
        if (!schema) continue;
        
        ss << "## " << info.typeName << "\n\n";
        ss << "**Version:** " << info.version << "\n";
        ss << "**Fields:** " << info.fieldCount << "\n\n";
        
        ss << "### Fields\n\n";
        ss << "| Name | Type | Required | Description |\n";
        ss << "|------|------|----------|-------------|\n";
        
        for (const auto& field : schema->GetFields()) {
            ss << "| " << field.name << " | ";
            
            // Type
            switch (field.type) {
                case ConfigValueType::Boolean: ss << "Boolean"; break;
                case ConfigValueType::Integer: ss << "Integer"; break;
                case ConfigValueType::Float: ss << "Float"; break;
                case ConfigValueType::String: ss << "String"; break;
                default: ss << "Unknown"; break;
            }
            
            ss << " | " << (field.required ? "Yes" : "No") << " | ";
            ss << field.description << " |\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

std::string ConfigIntegration::ExportSchemaAsJsonSchema(const std::string& typeName) {
    auto& registry = ConfigSchemaRegistry::GetInstance();
    const auto* schema = registry.GetSchema(typeName);
    
    if (!schema) {
        return "{}";
    }
    
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"$schema\": \"http://json-schema.org/draft-07/schema#\",\n";
    ss << "  \"type\": \"object\",\n";
    ss << "  \"title\": \"" << schema->GetName() << "\",\n";
    ss << "  \"properties\": {\n";
    
    bool first = true;
    for (const auto& field : schema->GetFields()) {
        if (!first) ss << ",\n";
        first = false;
        
        ss << "    \"" << field.name << "\": {\n";
        
        // Type mapping
        switch (field.type) {
            case ConfigValueType::Boolean:
                ss << "      \"type\": \"boolean\"";
                break;
            case ConfigValueType::Integer:
                ss << "      \"type\": \"integer\"";
                break;
            case ConfigValueType::Float:
                ss << "      \"type\": \"number\"";
                break;
            case ConfigValueType::String:
                ss << "      \"type\": \"string\"";
                break;
            default:
                ss << "      \"type\": \"string\"";
                break;
        }
        
        if (!field.description.empty()) {
            ss << ",\n      \"description\": \"" << field.description << "\"";
        }
        
        ss << "\n    }";
    }
    
    ss << "\n  }\n";
    ss << "}\n";
    
    return ss.str();
}

// ============================================================================
// ConfigPreprocessor Implementation
// ============================================================================

void ConfigPreprocessor::RegisterTransform(const std::string& name, 
                                          TransformFunction transform) {
    transforms_.push_back({name, transform, true});
}

void ConfigPreprocessor::ApplyTransforms(simplejson::JsonObject& config) const {
    for (const auto& transform : transforms_) {
        if (transform.enabled && transform.function) {
            transform.function(config);
        }
    }
}

void ConfigPreprocessor::SetEnabled(const std::string& name, bool enabled) {
    for (auto& transform : transforms_) {
        if (transform.name == name) {
            transform.enabled = enabled;
            break;
        }
    }
}

// ============================================================================
// ConfigTestSuite Implementation
// ============================================================================

ConfigTestSuite::ConfigTestSuite(const std::string& typeName) : typeName_(typeName) {}

void ConfigTestSuite::AddTestCase(const TestCase& testCase) {
    testCases_.push_back(testCase);
}

bool ConfigTestSuite::RunTests(bool verbose) {
    results_.clear();
    bool allPassed = true;
    
    auto& registry = ConfigSchemaRegistry::GetInstance();
    const auto* schema = registry.GetSchema(typeName_);
    
    if (!schema) {
        results_.push_back("ERROR: Schema not found for " + typeName_);
        return false;
    }
    
    ConfigValidator validator(*schema);
    
    for (const auto& testCase : testCases_) {
        auto parseResult = simplejson::Parse(testCase.configJson);
        
        if (!parseResult.success) {
            results_.push_back("FAIL: " + testCase.name + " - Parse error");
            allPassed = false;
            continue;
        }
        
        auto validation = validator.Validate(parseResult.value.AsObject());
        bool passed = (validation.isValid == testCase.shouldPass);
        
        if (passed) {
            results_.push_back("PASS: " + testCase.name);
        } else {
            results_.push_back("FAIL: " + testCase.name);
            allPassed = false;
            
            if (verbose) {
                results_.push_back("  Expected: " + 
                    std::string(testCase.shouldPass ? "valid" : "invalid"));
                results_.push_back("  Got: " + 
                    std::string(validation.isValid ? "valid" : "invalid"));
            }
        }
    }
    
    return allPassed;
}

std::string ConfigTestSuite::GetReport() const {
    std::stringstream ss;
    ss << "Configuration Test Report: " << typeName_ << "\n";
    ss << "Total tests: " << testCases_.size() << "\n\n";
    
    for (const auto& result : results_) {
        ss << result << "\n";
    }
    
    return ss.str();
}

} // namespace nova::config
