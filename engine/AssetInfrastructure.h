#pragma once

/**
 * Nova Engine Asset Infrastructure System
 * 
 * Complete technical infrastructure layer for the Asset Pipeline
 * Includes: Database, Search, Tags, Metrics, API, Integration,
 *           Monitoring, Debugging, Testing, and Documentation
 * 
 * This header provides unified access to all infrastructure components.
 */

#include "AssetPipeline.h"
#include "AssetDatabase.h"
#include "AssetSearch.h"
#include "AssetTags.h"
#include "AssetMetrics.h"
#include "AssetAPI.h"
#include "AssetIntegration.h"
#include "AssetMonitoring.h"
#include "AssetDebugging.h"
#include "AssetTesting.h"
#include "AssetDocumentation.h"

namespace AssetPipeline {

/**
 * Unified Asset Infrastructure Manager
 * Central coordinator for all infrastructure components
 */
class AssetInfrastructure {
public:
    static AssetInfrastructure& GetInstance();

    // Initialization
    bool Initialize(const std::string& asset_root, const std::string& db_path = "");
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Component access
    AssetDatabase& GetDatabase() { return AssetDatabase::GetInstance(); }
    AssetSearch& GetSearch() { return AssetSearch::GetInstance(); }
    AssetTags& GetTags() { return AssetTags::GetInstance(); }
    AssetMetricsCollector& GetMetrics() { return AssetMetricsCollector::GetInstance(); }
    AssetAPI& GetAPI() { return AssetAPI::GetInstance(); }
    AssetIntegration& GetIntegration() { return AssetIntegration::GetInstance(); }
    AssetMonitoring& GetMonitoring() { return AssetMonitoring::GetInstance(); }
    AssetDebugger& GetDebugger() { return AssetDebugger::GetInstance(); }
    AssetTestFramework& GetTesting() { return AssetTestFramework::GetInstance(); }
    AssetDocumentationGenerator& GetDocumentation() { return AssetDocumentationGenerator::GetInstance(); }

    // Quick status
    struct InfrastructureStatus {
        bool database_initialized = false;
        bool monitoring_active = false;
        bool debugging_enabled = false;
        bool testing_enabled = false;
        size_t total_assets = 0;
        size_t documented_assets = 0;
        size_t active_integrations = 0;
        size_t active_alerts = 0;
        std::string health_status;
    };
    
    InfrastructureStatus GetStatus();
    
    // Quick operations
    void EnableDevelopmentMode();
    void EnableProductionMode();
    void EnableTestingMode();
    
    // Update (call regularly)
    void Update();

private:
    AssetInfrastructure() = default;
    
    std::atomic<bool> initialized_{false};
    std::string asset_root_;
};

} // namespace AssetPipeline
