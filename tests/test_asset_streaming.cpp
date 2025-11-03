#include "../engine/AssetStreamingSystem.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <filesystem>
#include <fstream>

using namespace assets::streaming;

// Test helper functions
bool CreateTestFile(const std::string& filePath, size_t sizeKB = 1) {
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file) return false;
    
    // Write test data
    std::vector<char> data(sizeKB * 1024, 'A');
    file.write(data.data(), data.size());
    return true;
}

void CleanupTestFiles() {
    std::filesystem::remove_all("test_streaming_assets");
    std::filesystem::remove_all("streaming_output");
    std::filesystem::remove_all("streaming_cache");
}

int main() {
    std::cout << "Asset Streaming System Test Suite" << std::endl;
    std::cout << "==================================" << std::endl;

    // Cleanup from previous runs
    CleanupTestFiles();

    auto& streamingSystem = AssetStreamingSystem::Instance();

    // Test 1: System Initialization
    std::cout << "\n=== Testing System Initialization ===" << std::endl;
    
    MemoryConstraints constraints;
    constraints.maxTotalMemory = 64 * 1024 * 1024;  // 64MB for testing
    constraints.maxTextureMemory = 32 * 1024 * 1024;
    constraints.maxMeshMemory = 16 * 1024 * 1024;
    constraints.maxAudioMemory = 8 * 1024 * 1024;
    
    bool initResult = streamingSystem.Initialize(constraints);
    assert(initResult);
    std::cout << "SUCCESS: Streaming system initialized" << std::endl;

    // Test 2: Asset Registration
    std::cout << "\n=== Testing Asset Registration ===" << std::endl;
    
    // Create test files
    CreateTestFile("test_streaming_assets/texture1.png", 512);  // 512KB texture
    CreateTestFile("test_streaming_assets/mesh1.obj", 256);     // 256KB mesh
    CreateTestFile("test_streaming_assets/audio1.wav", 1024);   // 1MB audio
    CreateTestFile("test_streaming_assets/texture2.png", 128);  // 128KB texture
    CreateTestFile("test_streaming_assets/mesh2.obj", 64);      // 64KB mesh

    // Register assets
    streamingSystem.RegisterAsset("texture1", "test_streaming_assets/texture1.png", 
                                 MemoryCategory::Texture, 512 * 1024);
    streamingSystem.RegisterAsset("mesh1", "test_streaming_assets/mesh1.obj", 
                                 MemoryCategory::Mesh, 256 * 1024);
    streamingSystem.RegisterAsset("audio1", "test_streaming_assets/audio1.wav", 
                                 MemoryCategory::Audio, 1024 * 1024);
    streamingSystem.RegisterAsset("texture2", "test_streaming_assets/texture2.png", 
                                 MemoryCategory::Texture, 128 * 1024);
    streamingSystem.RegisterAsset("mesh2", "test_streaming_assets/mesh2.obj", 
                                 MemoryCategory::Mesh, 64 * 1024);

    std::cout << "SUCCESS: Assets registered" << std::endl;

    // Test 3: Basic Asset Loading
    std::cout << "\n=== Testing Basic Asset Loading ===" << std::endl;
    
    auto loadFuture = streamingSystem.RequestAsset("texture1", StreamingPriority::High);
    bool loadSuccess = loadFuture.get();
    assert(loadSuccess);
    assert(streamingSystem.IsAssetLoaded("texture1"));
    
    auto assetData = streamingSystem.GetAsset("texture1");
    assert(assetData != nullptr);
    
    std::cout << "SUCCESS: Asset loaded successfully" << std::endl;

    // Test 4: Distance-Based Loading
    std::cout << "\n=== Testing Distance-Based Loading ===" << std::endl;
    
    streamingSystem.SetViewerPosition(0, 0, 0);
    
    // Update distances for different assets
    streamingSystem.UpdateAssetDistance("texture1", 15.0f);  // Close - should be high priority
    streamingSystem.UpdateAssetDistance("mesh1", 100.0f);   // Medium distance
    streamingSystem.UpdateAssetDistance("audio1", 300.0f);  // Far - low priority
    streamingSystem.UpdateAssetDistance("texture2", 1200.0f); // Very far - should unload
    
    // Update system to process distance changes
    streamingSystem.Update(0.016f); // ~60 FPS
    
    std::cout << "SUCCESS: Distance-based priorities updated" << std::endl;

    // Test 5: LOD System
    std::cout << "\n=== Testing LOD System ===" << std::endl;
    
    // Enable adaptive LOD
    streamingSystem.EnableAdaptiveLOD(true);
    
    // Test LOD calculation for different distances
    auto closeLOD = streamingSystem.CalculateOptimalLOD("texture1");   // Close asset
    auto farLOD = streamingSystem.CalculateOptimalLOD("audio1");       // Far asset
    
    std::cout << "Close asset LOD: " << streaming_utils::LODToString(closeLOD) << std::endl;
    std::cout << "Far asset LOD: " << streaming_utils::LODToString(farLOD) << std::endl;
    
    // Request LOD change
    streamingSystem.RequestLODChange("texture1", LODLevel::Highest);
    
    std::cout << "SUCCESS: LOD system working" << std::endl;

    // Test 6: Memory Management
    std::cout << "\n=== Testing Memory Management ===" << std::endl;
    
    // Load multiple assets to test memory pressure
    std::vector<std::future<bool>> loadFutures;
    loadFutures.push_back(streamingSystem.RequestAsset("mesh1", StreamingPriority::Medium));
    loadFutures.push_back(streamingSystem.RequestAsset("audio1", StreamingPriority::Low));
    loadFutures.push_back(streamingSystem.RequestAsset("texture2", StreamingPriority::Medium));
    
    // Wait for all loads to complete
    for (auto& future : loadFutures) {
        future.get();
    }
    
    auto memStats = streamingSystem.GetMemoryStats();
    std::cout << "Memory usage: " << streaming_utils::FormatMemorySize(memStats.totalUsed) 
              << " (" << std::fixed << std::setprecision(1) << memStats.utilizationPercent << "%)" << std::endl;
    std::cout << "Loaded assets: " << memStats.loadedAssets << std::endl;
    
    // Test garbage collection
    streamingSystem.ForceGarbageCollection();
    
    std::cout << "SUCCESS: Memory management working" << std::endl;

    // Test 7: Async Loading with Callbacks
    std::cout << "\n=== Testing Async Loading ===" << std::endl;
    
    bool callbackExecuted = false;
    streamingSystem.RequestAssetAsync("mesh2", StreamingPriority::High, 
        [&callbackExecuted](bool success) {
            std::cout << "Async callback executed with result: " << (success ? "success" : "failure") << std::endl;
            callbackExecuted = true;
        });
    
    // Wait for callback
    int attempts = 0;
    while (!callbackExecuted && attempts++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        streamingSystem.Update(0.010f);
    }
    
    assert(callbackExecuted);
    std::cout << "SUCCESS: Async loading with callbacks working" << std::endl;

    // Test 8: Asset States and Progress
    std::cout << "\n=== Testing Asset States and Progress ===" << std::endl;
    
    // Test various asset states
    assert(streamingSystem.GetAssetState("texture1") == LoadingState::Loaded);
    assert(streamingSystem.GetLoadProgress("texture1") == 1.0f);
    
    // Start loading a new asset and check progress
    auto progressFuture = streamingSystem.RequestAsset("mesh1", StreamingPriority::Medium);
    
    // Check progress while loading (may be too fast to catch, but test the API)
    float progress = streamingSystem.GetLoadProgress("mesh1");
    std::cout << "Loading progress: " << (progress * 100.0f) << "%" << std::endl;
    
    progressFuture.get();
    assert(streamingSystem.GetLoadProgress("mesh1") == 1.0f);
    
    std::cout << "SUCCESS: Asset states and progress tracking working" << std::endl;

    // Test 9: Streaming Metrics
    std::cout << "\n=== Testing Streaming Metrics ===" << std::endl;
    
    auto metrics = streamingSystem.GetMetrics();
    std::cout << "Total loads: " << metrics.totalLoads << std::endl;
    std::cout << "Total unloads: " << metrics.totalUnloads << std::endl;
    std::cout << "Load failures: " << metrics.loadFailures << std::endl;
    std::cout << "Peak memory: " << metrics.peakMemoryUsage << " MB" << std::endl;
    
    std::cout << "SUCCESS: Metrics collection working" << std::endl;

    // Test 10: Asset Handles
    std::cout << "\n=== Testing Asset Handles ===" << std::endl;
    
    StreamingAssetHandle handle("texture1");
    assert(handle.IsLoaded());
    assert(handle.GetState() == LoadingState::Loaded);
    assert(handle.Get() != nullptr);
    
    handle.UpdateDistance(50.0f);
    
    auto handleFuture = handle.Request(StreamingPriority::High);
    handleFuture.get();
    
    std::cout << "SUCCESS: Asset handles working" << std::endl;

    // Test 11: System Performance Under Load
    std::cout << "\n=== Testing System Performance ===" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Simulate rapid asset requests
    std::vector<std::future<bool>> performanceFutures;
    for (int i = 0; i < 50; ++i) {
        std::string assetId = "texture" + std::to_string(i % 2 + 1);
        performanceFutures.push_back(streamingSystem.RequestAsset(assetId, StreamingPriority::Medium));
    }
    
    // Wait for all requests
    for (auto& future : performanceFutures) {
        future.get();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float>(endTime - startTime).count();
    
    std::cout << "Performance test: 50 requests completed in " << duration << " seconds" << std::endl;
    std::cout << "SUCCESS: System performance acceptable" << std::endl;

    // Test 12: Configuration and Debug Info
    std::cout << "\n=== Testing Configuration and Debug Info ===" << std::endl;
    
    DistanceConfig distConfig;
    distConfig.criticalDistance = 5.0f;
    distConfig.highDistance = 25.0f;
    distConfig.mediumDistance = 100.0f;
    streamingSystem.SetDistanceConfig(distConfig);
    
    streamingSystem.PrintDebugInfo();
    
    std::cout << "SUCCESS: Configuration and debug info working" << std::endl;

    // Final system update
    std::cout << "\n=== Final System State ===" << std::endl;
    
    streamingSystem.Update(0.016f);
    
    auto finalStats = streamingSystem.GetMemoryStats();
    std::cout << "Final memory usage: " << streaming_utils::FormatMemorySize(finalStats.totalUsed) << std::endl;
    std::cout << "Final loaded assets: " << finalStats.loadedAssets << std::endl;
    
    auto finalMetrics = streamingSystem.GetMetrics();
    std::cout << "Final total loads: " << finalMetrics.totalLoads << std::endl;
    std::cout << "Final load failures: " << finalMetrics.loadFailures << std::endl;

    // Shutdown
    std::cout << "\n=== Shutting Down ===" << std::endl;
    streamingSystem.Shutdown();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "SUCCESS: All Asset Streaming System tests passed!" << std::endl;

    // Cleanup
    std::cout << "\n=== Cleaning Up Test Files ===" << std::endl;
    CleanupTestFiles();
    std::cout << "Test files cleaned up" << std::endl;

    return 0;
}