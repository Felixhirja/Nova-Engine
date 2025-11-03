#pragma once

#include "ConfigSystem.h"
#include "PlayerConfig.h"
#include "BootstrapConfiguration.h"
#include "../entities/ActorConfig.h"

namespace nova::config {

/**
 * ConfigIntegration - Integration layer between new unified config system
 * and existing legacy configuration structures
 * 
 * Provides backward compatibility while enabling gradual migration
 */
class ConfigIntegration {
public:
    // ========================================================================
    // SCHEMA DEFINITIONS - Pre-built schemas for existing config types
    // ========================================================================

    /**
     * Register all built-in configuration schemas
     */
    static void RegisterBuiltInSchemas();

    /**
     * Create schema for PlayerConfig
     */
    static ConfigSchema CreatePlayerConfigSchema();

    /**
     * Create schema for BootstrapConfiguration
     */
    static ConfigSchema CreateBootstrapSchema();

    /**
     * Create schema for ActorConfig (base)
     */
    static ConfigSchema CreateActorConfigSchema();

    /**
     * Create schema for StationConfig
     */
    static ConfigSchema CreateStationConfigSchema();

    // ========================================================================
    // LEGACY ADAPTERS - Convert between old and new config systems
    // ========================================================================

    /**
     * Convert legacy PlayerConfig to unified Configuration
     */
    static std::shared_ptr<Configuration> FromPlayerConfig(const PlayerConfig& legacy);

    /**
     * Convert unified Configuration to legacy PlayerConfig
     */
    static PlayerConfig ToPlayerConfig(const Configuration& config);

    /**
     * Convert legacy BootstrapConfiguration to unified Configuration
     */
    static std::shared_ptr<Configuration> FromBootstrapConfig(
        const BootstrapConfiguration& legacy);

    /**
     * Convert unified Configuration to legacy BootstrapConfiguration
     */
    static BootstrapConfiguration ToBootstrapConfig(const Configuration& config);

    // ========================================================================
    // MIGRATION HELPERS - Version upgrade paths
    // ========================================================================

    /**
     * Register all built-in migrations
     */
    static void RegisterBuiltInMigrations();

    /**
     * Create migration from PlayerConfig v1.0 to v2.0
     * Example: adds new physics fields
     */
    static ConfigMigration CreatePlayerConfigMigration_1_0_to_2_0();

    // ========================================================================
    // VALIDATION HELPERS - Common validation patterns
    // ========================================================================

    /**
     * Common validators for configuration fields
     */
    struct Validators {
        static bool PositiveNumber(const ConfigValue& value);
        static bool NonEmptyString(const ConfigValue& value);
        static bool ValidColor(const ConfigValue& value);
        static bool ValidPath(const ConfigValue& value);
        static bool InRange(double min, double max, const ConfigValue& value);
    };

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * Load configuration with automatic type detection
     */
    static std::shared_ptr<Configuration> LoadConfigAuto(const std::string& filePath);

    /**
     * Generate documentation for all registered schemas
     */
    static std::string GenerateDocumentation();

    /**
     * Export schema as JSON Schema format (for IDE integration)
     */
    static std::string ExportSchemaAsJsonSchema(const std::string& typeName);
};

// ============================================================================
// HELPER MACROS - Simplify schema definition
// ============================================================================

#define NOVA_CONFIG_SCHEMA(TypeName) \
    ConfigSchema schema(#TypeName, "1.0"); \
    schema.AddDescription("Auto-generated schema for " #TypeName)

#define NOVA_FIELD(name, type, required) \
    schema.AddField(#name, ConfigValueType::type, required)

#define NOVA_FIELD_WITH_DEFAULT(name, type, defaultVal) \
    { \
        ConfigSchema::Field field; \
        field.name = #name; \
        field.type = ConfigValueType::type; \
        field.required = false; \
        field.defaultValue = defaultVal; \
        schema.AddField(field); \
    }

#define NOVA_FIELD_RANGE(name, type, minVal, maxVal) \
    { \
        ConfigSchema::Field field; \
        field.name = #name; \
        field.type = ConfigValueType::type; \
        field.minValue = minVal; \
        field.maxValue = maxVal; \
        schema.AddField(field); \
    }

// ============================================================================
// TEMPLATE HELPERS - Type-safe configuration loading
// ============================================================================

/**
 * ConfigLoader - Template-based configuration loader
 * 
 * Usage:
 *   auto config = ConfigLoader<PlayerConfig>::Load("player.json");
 */
template<typename T>
class ConfigLoader {
public:
    static T Load(const std::string& filePath);
    static bool Save(const T& config, const std::string& filePath);
};

// Specializations for known types
template<>
class ConfigLoader<PlayerConfig> {
public:
    static PlayerConfig Load(const std::string& filePath) {
        auto& system = ConfigSystem::GetInstance();
        auto config = system.LoadConfig("PlayerConfig", filePath);
        
        if (!config) {
            return PlayerConfig::GetDefault();
        }
        
        return ConfigIntegration::ToPlayerConfig(*config);
    }
    
    static bool Save(const PlayerConfig& playerConfig, const std::string& filePath) {
        auto config = ConfigIntegration::FromPlayerConfig(playerConfig);
        return config->Save(filePath);
    }
};

template<>
class ConfigLoader<BootstrapConfiguration> {
public:
    static BootstrapConfiguration Load(const std::string& filePath) {
        auto& system = ConfigSystem::GetInstance();
        auto config = system.LoadConfig("BootstrapConfiguration", filePath);
        
        if (!config) {
            return BootstrapConfiguration();
        }
        
        return ConfigIntegration::ToBootstrapConfig(*config);
    }
    
    static bool Save(const BootstrapConfiguration& bootstrap, const std::string& filePath) {
        auto config = ConfigIntegration::FromBootstrapConfig(bootstrap);
        return config->Save(filePath);
    }
};

// ============================================================================
// CONFIG PREPROCESSOR - Runtime config manipulation
// ============================================================================

/**
 * ConfigPreprocessor - Apply transformations to configurations before loading
 * 
 * Useful for:
 * - Environment-specific overrides (dev/staging/prod)
 * - User preferences overlay
 * - Feature flags
 */
class ConfigPreprocessor {
public:
    using TransformFunction = std::function<void(simplejson::JsonObject&)>;

    void RegisterTransform(const std::string& name, TransformFunction transform);
    void ApplyTransforms(simplejson::JsonObject& config) const;
    void SetEnabled(const std::string& name, bool enabled);

private:
    struct Transform {
        std::string name;
        TransformFunction function;
        bool enabled = true;
    };

    std::vector<Transform> transforms_;
};

// ============================================================================
// CONFIG TESTING FRAMEWORK - Automated config validation
// ============================================================================

/**
 * ConfigTestSuite - Testing framework for configurations
 */
class ConfigTestSuite {
public:
    struct TestCase {
        std::string name;
        std::string configJson;
        bool shouldPass;
        std::vector<std::string> expectedErrors;
    };

    ConfigTestSuite(const std::string& typeName);

    void AddTestCase(const TestCase& testCase);
    bool RunTests(bool verbose = false);
    std::string GetReport() const;

private:
    std::string typeName_;
    std::vector<TestCase> testCases_;
    std::vector<std::string> results_;
};

} // namespace nova::config
