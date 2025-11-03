// Nova Engine - Asset Pipeline Test
// Demonstrates all features of the Asset Pipeline Enhancement system

#include "engine/AssetPipeline.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace AssetPipeline;

// Custom validator for texture assets
AssetValidationResult ValidateTexture(const AssetMetadata& metadata) {
    AssetValidationResult result;
    
    // Check file exists
    if (!FileExists(metadata.path)) {
        result.is_valid = false;
        result.errors.push_back("Texture file does not exist");
        return result;
    }
    
    // Check file size
    if (metadata.size_bytes == 0) {
        result.is_valid = false;
        result.errors.push_back("Texture file is empty");
        return result;
    }
    
    // Check reasonable size limits
    if (metadata.size_bytes > 100 * 1024 * 1024) { // 100 MB
        result.warnings.push_back("Texture is very large (>100MB)");
    }
    
    std::cout << "  ✓ Texture validated: " << metadata.name << "\n";
    return result;
}

// Custom validator for config assets
AssetValidationResult ValidateConfig(const AssetMetadata& metadata) {
    AssetValidationResult result;
    
    if (!FileExists(metadata.path)) {
        result.is_valid = false;
        result.errors.push_back("Config file does not exist");
        return result;
    }
    
    // Could add JSON/XML parsing validation here
    std::cout << "  ✓ Config validated: " << metadata.name << "\n";
    return result;
}

void TestAssetValidation() {
    std::cout << "\n=== Testing Asset Validation ===\n";
    
    auto& validator = AssetValidator::GetInstance();
    
    // Register validators
    validator.RegisterValidator(AssetType::Texture, ValidateTexture);
    validator.RegisterValidator(AssetType::Config, ValidateConfig);
    
    // Test validation
    AssetMetadata test_asset;
    test_asset.path = "assets/bootstrap.json";
    test_asset.name = "bootstrap.json";
    test_asset.type = AssetType::Config;
    test_asset.size_bytes = 1024;
    
    auto result = validator.ValidateAsset(test_asset);
    std::cout << "Validation result: " << (result.is_valid ? "PASS" : "FAIL") << "\n";
    std::cout << "Validation time: " << result.validation_time.count() << "ms\n";
    
    for (const auto& error : result.errors) {
        std::cout << "  Error: " << error << "\n";
    }
    for (const auto& warning : result.warnings) {
        std::cout << "  Warning: " << warning << "\n";
    }
}

void TestDependencyTracking() {
    std::cout << "\n=== Testing Dependency Tracking ===\n";
    
    auto& deps = DependencyTracker::GetInstance();
    deps.Clear();
    
    // Register dependencies
    deps.RegisterDependency("assets/actors/ships/spaceship.json", "assets/graphics/sprites/ships/fighter.svg");
    deps.RegisterDependency("assets/actors/ships/spaceship.json", "assets/content/ships/classes/fighter.json");
    deps.RegisterDependency("assets/content/ships/classes/fighter.json", "assets/content/ships/modules/hulls/light_hull.json");
    
    std::cout << "Registered asset dependencies\n";
    
    // Get dependencies
    auto dependencies = deps.GetDependencies("assets/actors/ships/spaceship.json");
    std::cout << "\nDependencies for spaceship.json:\n";
    for (const auto& dep : dependencies) {
        std::cout << "  - " << dep << "\n";
    }
    
    // Get dependents
    auto dependents = deps.GetDependents("assets/graphics/sprites/ships/fighter.svg");
    std::cout << "\nAssets depending on fighter.svg:\n";
    for (const auto& dependent : dependents) {
        std::cout << "  - " << dependent << "\n";
    }
    
    // Get full dependency chain
    auto chain = deps.GetDependencyChain("assets/actors/ships/spaceship.json");
    std::cout << "\nFull dependency chain:\n";
    for (const auto& item : chain) {
        std::cout << "  → " << item << "\n";
    }
    
    // Check for circular dependencies
    bool has_circular = deps.HasCircularDependency("assets/actors/ships/spaceship.json");
    std::cout << "\nCircular dependency check: " 
              << (has_circular ? "FOUND (ERROR!)" : "NONE (OK)") << "\n";
    
    // Get optimal load order
    std::vector<std::string> assets = {
        "assets/actors/ships/spaceship.json",
        "assets/graphics/sprites/ships/fighter.svg"
    };
    auto load_order = deps.GetLoadOrder(assets);
    std::cout << "\nOptimal load order:\n";
    for (size_t i = 0; i < load_order.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << load_order[i] << "\n";
    }
    
    // Export dependency graph
    deps.ExportDependencyGraph("dependency_graph.dot");
    std::cout << "\nDependency graph exported to: dependency_graph.dot\n";
    std::cout << "  (Use 'dot -Tpng dependency_graph.dot -o graph.png' to visualize)\n";
}

void TestHotReloading() {
    std::cout << "\n=== Testing Hot Reloading ===\n";
    
    auto& hot_reload = HotReloadManager::GetInstance();
    hot_reload.Enable(true);
    
    // Watch specific assets
    hot_reload.WatchAsset("assets/bootstrap.json");
    std::cout << "Watching: assets/bootstrap.json\n";
    
    // Watch directory
    hot_reload.WatchDirectory("assets/config", false);
    std::cout << "Watching directory: assets/config\n";
    
    // Register reload callback
    hot_reload.RegisterReloadCallback(
        "assets/bootstrap.json",
        [](const std::string& path) {
            std::cout << "  🔄 Asset reloaded: " << path << "\n";
        }
    );
    
    std::cout << "\nMonitoring for changes (5 seconds)...\n";
    std::cout << "  (Modify watched files to see hot reload in action)\n";
    
    for (int i = 0; i < 5; ++i) {
        hot_reload.Update();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << ".";
        std::cout.flush();
    }
    std::cout << "\n";
    
    size_t pending = hot_reload.GetPendingReloads();
    std::cout << "Pending reloads: " << pending << "\n";
    
    hot_reload.FlushReloads();
}

void TestCompression() {
    std::cout << "\n=== Testing Asset Compression ===\n";
    
    auto& compression = CompressionManager::GetInstance();
    
    // Get optimal compression for different asset types
    auto texture_compression = compression.GetOptimalCompression(AssetType::Texture);
    auto model_compression = compression.GetOptimalCompression(AssetType::Model);
    auto audio_compression = compression.GetOptimalCompression(AssetType::Audio);
    
    std::cout << "Optimal compression types:\n";
    std::cout << "  Texture: " << static_cast<int>(texture_compression) << "\n";
    std::cout << "  Model: " << static_cast<int>(model_compression) << "\n";
    std::cout << "  Audio: " << static_cast<int>(audio_compression) << "\n";
    
    // Test data compression
    std::vector<uint8_t> test_data(1024, 42);
    auto compressed = compression.CompressData(test_data, CompressionType::LZ4);
    
    std::cout << "\nCompression test:\n";
    std::cout << "  Original size: " << test_data.size() << " bytes\n";
    std::cout << "  Compressed size: " << compressed.size() << " bytes\n";
    
    float ratio = static_cast<float>(compressed.size()) / test_data.size();
    std::cout << "  Compression ratio: " << (ratio * 100) << "%\n";
}

void TestVersioning() {
    std::cout << "\n=== Testing Asset Versioning ===\n";
    
    auto& versioning = VersionManager::GetInstance();
    
    // Set versions
    versioning.SetAssetVersion("assets/actors/ships/spaceship.json", 2);
    versioning.SetAssetVersion("assets/graphics/sprites/ships/fighter.svg", 1);
    
    // Add changelog entries
    versioning.AddChangelogEntry(
        "assets/actors/ships/spaceship.json",
        "v2: Added energy management components"
    );
    versioning.AddChangelogEntry(
        "assets/actors/ships/spaceship.json",
        "v2: Updated collision bounds"
    );
    
    // Get version
    int version = versioning.GetAssetVersion("assets/actors/ships/spaceship.json");
    std::cout << "Spaceship asset version: v" << version << "\n";
    
    // Check compatibility
    bool compatible = versioning.IsVersionCompatible("assets/actors/ships/spaceship.json", 2);
    std::cout << "Version compatibility (v2): " << (compatible ? "YES" : "NO") << "\n";
    
    // Get changelog
    auto changelog = versioning.GetChangelog("assets/actors/ships/spaceship.json");
    std::cout << "\nChangelog:\n";
    for (const auto& entry : changelog) {
        std::cout << "  - " << entry << "\n";
    }
    
    // Export manifest
    versioning.ExportVersionManifest("version_manifest.txt");
    std::cout << "\nVersion manifest exported to: version_manifest.txt\n";
}

void TestOptimization() {
    std::cout << "\n=== Testing Asset Optimization ===\n";
    
    auto& optimization = OptimizationManager::GetInstance();
    
    // Set optimization level
    optimization.SetOptimizationLevel(2);
    std::cout << "Optimization level: " << optimization.GetOptimizationLevel() << "\n";
    
    // Check if asset types can be optimized
    std::cout << "\nOptimization support:\n";
    std::cout << "  Texture: " << (optimization.CanOptimize(AssetType::Texture) ? "YES" : "NO") << "\n";
    std::cout << "  Model: " << (optimization.CanOptimize(AssetType::Model) ? "YES" : "NO") << "\n";
    std::cout << "  Script: " << (optimization.CanOptimize(AssetType::Script) ? "YES" : "NO") << "\n";
    
    // Estimate savings
    std::string test_path = "assets/graphics/sprites/ships/fighter.svg";
    if (FileExists(test_path)) {
        size_t savings = optimization.EstimateOptimizationSavings(test_path);
        std::cout << "\nEstimated optimization savings for " << test_path << ":\n";
        std::cout << "  " << (savings / 1024) << " KB\n";
    }
}

void TestStreaming() {
    std::cout << "\n=== Testing Asset Streaming ===\n";
    
    auto& streaming = StreamingManager::GetInstance();
    streaming.EnableStreaming(true);
    
    // Set memory budget
    streaming.SetMemoryBudget(512 * 1024 * 1024); // 512 MB
    
    std::cout << "Streaming enabled\n";
    std::cout << "Memory budget: " << (streaming.GetMemoryBudget() / 1024 / 1024) << " MB\n";
    
    // Mark assets as streamable with priorities
    streaming.MarkStreamable("assets/distant_planet.svg", 10);
    streaming.MarkStreamable("assets/nearby_ship.svg", 100);
    streaming.MarkStreamable("assets/player_ship.svg", 1000);
    
    std::cout << "\nMarked 3 assets as streamable\n";
    
    // Check if assets are streamable
    bool is_streamable = streaming.IsStreamable("assets/player_ship.svg");
    std::cout << "Player ship streamable: " << (is_streamable ? "YES" : "NO") << "\n";
    
    // Update streaming system
    streaming.Update();
    
    std::cout << "Current memory usage: " 
              << (streaming.GetMemoryUsage() / 1024 / 1024) << " MB\n";
}

void TestCaching() {
    std::cout << "\n=== Testing Asset Caching ===\n";
    
    auto& cache = CacheManager::GetInstance();
    cache.EnableCache(true);
    cache.SetCacheSize(256 * 1024 * 1024); // 256 MB
    
    std::cout << "Cache enabled\n";
    std::cout << "Cache size limit: " << (256) << " MB\n";
    
    // Cache some test data
    uint8_t test_data1[] = {1, 2, 3, 4, 5};
    uint8_t test_data2[] = {6, 7, 8, 9, 10};
    
    cache.CacheAsset("test_asset_1", test_data1, sizeof(test_data1));
    cache.CacheAsset("test_asset_2", test_data2, sizeof(test_data2));
    
    std::cout << "\nCached 2 test assets\n";
    
    // Try to retrieve from cache
    void* cached_data = nullptr;
    size_t cached_size = 0;
    
    bool hit = cache.GetCachedAsset("test_asset_1", &cached_data, &cached_size);
    std::cout << "Cache lookup for test_asset_1: " << (hit ? "HIT" : "MISS") << "\n";
    
    hit = cache.GetCachedAsset("non_existent", &cached_data, &cached_size);
    std::cout << "Cache lookup for non_existent: " << (hit ? "HIT" : "MISS") << "\n";
    
    // Get cache statistics
    auto stats = cache.GetStats();
    std::cout << "\nCache statistics:\n";
    std::cout << "  Hit count: " << stats.hit_count << "\n";
    std::cout << "  Miss count: " << stats.miss_count << "\n";
    std::cout << "  Hit rate: " << (stats.hit_rate * 100) << "%\n";
    std::cout << "  Cache usage: " << stats.cache_size << " bytes\n";
    
    // Invalidate specific cache entry
    cache.InvalidateCache("test_asset_1");
    std::cout << "\nInvalidated test_asset_1\n";
}

void TestAnalytics() {
    std::cout << "\n=== Testing Asset Analytics ===\n";
    
    auto& analytics = AnalyticsManager::GetInstance();
    analytics.ClearAnalytics();
    
    // Simulate asset loads and accesses
    analytics.RecordAssetLoad("assets/ship.svg", std::chrono::milliseconds(150));
    analytics.RecordAssetLoad("assets/ship.svg", std::chrono::milliseconds(120));
    analytics.RecordAssetLoad("assets/station.svg", std::chrono::milliseconds(250));
    
    analytics.RecordAssetAccess("assets/ship.svg");
    analytics.RecordAssetAccess("assets/ship.svg");
    analytics.RecordAssetAccess("assets/ship.svg");
    analytics.RecordAssetAccess("assets/station.svg");
    
    analytics.RecordMemoryUsage("assets/ship.svg", 4096);
    analytics.RecordMemoryUsage("assets/station.svg", 8192);
    
    std::cout << "Recorded analytics for test assets\n";
    
    // Get analytics for specific asset
    auto ship_analytics = analytics.GetAnalytics("assets/ship.svg");
    std::cout << "\nAnalytics for ship.svg:\n";
    std::cout << "  Load count: " << ship_analytics.load_count << "\n";
    std::cout << "  Access count: " << ship_analytics.access_count << "\n";
    std::cout << "  Average load time: " << ship_analytics.average_load_time.count() << "ms\n";
    std::cout << "  Memory usage: " << ship_analytics.memory_usage << " bytes\n";
    
    // Get top assets
    auto top_assets = analytics.GetTopAssets(2);
    std::cout << "\nTop accessed assets:\n";
    for (size_t i = 0; i < top_assets.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << top_assets[i].asset_path 
                  << " (" << top_assets[i].access_count << " accesses)\n";
    }
    
    // Export report
    analytics.ExportReport("analytics_report.md");
    std::cout << "\nAnalytics report exported to: analytics_report.md\n";
}

void TestDocumentation() {
    std::cout << "\n=== Testing Documentation Generation ===\n";
    
    auto& doc_gen = DocumentationGenerator::GetInstance();
    
    // Add custom sections
    doc_gen.AddCustomSection(
        "Asset Naming Conventions",
        "All assets should follow lowercase_with_underscores naming"
    );
    doc_gen.AddCustomSection(
        "Supported Formats",
        "Textures: PNG, SVG, BMP\nModels: OBJ, FBX\nAudio: WAV, OGG"
    );
    
    // Set format
    doc_gen.SetDocFormat("markdown");
    std::cout << "Documentation format: " << doc_gen.GetDocFormat() << "\n";
    
    // Generate documentation
    doc_gen.GenerateDocumentation("asset_pipeline_documentation.md");
    std::cout << "Generated documentation: asset_pipeline_documentation.md\n";
}

void TestPipelineManager() {
    std::cout << "\n=== Testing Asset Pipeline Manager ===\n";
    
    auto& pipeline = AssetPipelineManager::GetInstance();
    
    // Initialize
    bool init_success = pipeline.Initialize("assets/");
    std::cout << "Pipeline initialization: " << (init_success ? "SUCCESS" : "FAILED") << "\n";
    
    // Register some test assets
    pipeline.RegisterAsset("assets/bootstrap.json", AssetType::Config);
    pipeline.RegisterAsset("assets/graphics/sprites/ships/fighter.svg", AssetType::Texture);
    
    std::cout << "Registered test assets\n";
    
    // Discover all assets
    pipeline.DiscoverAssets("assets/");
    
    // Get asset metadata
    auto* metadata = pipeline.GetAssetMetadata("assets/bootstrap.json");
    if (metadata) {
        std::cout << "\nAsset metadata for bootstrap.json:\n";
        std::cout << "  Name: " << metadata->name << "\n";
        std::cout << "  Type: " << GetAssetTypeName(metadata->type) << "\n";
        std::cout << "  Size: " << metadata->size_bytes << " bytes\n";
        std::cout << "  Checksum: " << metadata->checksum << "\n";
    }
    
    // Update pipeline
    pipeline.Update();
    
    // Get status
    auto status = pipeline.GetStatus();
    std::cout << "\nPipeline Status:\n";
    std::cout << "  Total assets: " << status.total_assets << "\n";
    std::cout << "  Loaded assets: " << status.loaded_assets << "\n";
    std::cout << "  Failed assets: " << status.failed_assets << "\n";
    std::cout << "  Cached assets: " << status.cached_assets << "\n";
    std::cout << "  Streamed assets: " << status.streamed_assets << "\n";
    std::cout << "  Memory usage: " << (status.memory_usage / 1024) << " KB\n";
    std::cout << "  Cache usage: " << (status.cache_usage / 1024) << " KB\n";
    
    // Export status report
    pipeline.ExportStatusReport("pipeline_status.md");
    std::cout << "\nPipeline status report exported to: pipeline_status.md\n";
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Nova Engine - Asset Pipeline Enhancement Test Suite   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    try {
        TestAssetValidation();
        TestDependencyTracking();
        TestHotReloading();
        TestCompression();
        TestVersioning();
        TestOptimization();
        TestStreaming();
        TestCaching();
        TestAnalytics();
        TestDocumentation();
        TestPipelineManager();
        
        std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              All Tests Completed Successfully!           ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        
        std::cout << "\nGenerated Files:\n";
        std::cout << "  - dependency_graph.dot\n";
        std::cout << "  - version_manifest.txt\n";
        std::cout << "  - analytics_report.md\n";
        std::cout << "  - asset_pipeline_documentation.md\n";
        std::cout << "  - pipeline_status.md\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
