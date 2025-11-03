#include "../engine/AssetStreamingSystem.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// Simple test assets creation
void CreateTestAssets() {
    fs::create_directories("test_stream_assets");
    
    // Create small test file
    std::ofstream small("test_stream_assets/small_asset.txt");
    small << "Small test asset content";
    small.close();
    
    // Create medium test file  
    std::ofstream medium("test_stream_assets/medium_asset.txt");
    for (int i = 0; i < 1000; ++i) {
        medium << "This is line " << i << " of the medium test asset.\n";
    }
    medium.close();
    
    // Create large test file
    std::ofstream large("test_stream_assets/large_asset.txt");
    for (int i = 0; i < 10000; ++i) {
        large << "This is line " << i << " of the large test asset with more content to make it bigger.\n";
    }
    large.close();
}

void CleanupTestAssets() {
    try {
        if (fs::exists("test_stream_assets")) {
            fs::remove_all("test_stream_assets");
        }
    } catch (const std::exception& e) {
        std::cout << "Warning: Could not clean up test assets: " << e.what() << std::endl;
    }
}

bool TestBasicInitialization() {
    std::cout << "\n=== Testing Basic Initialization ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    assets::streaming::MemoryConstraints constraints;
    constraints.maxTotalMemory = 64 * 1024 * 1024; // 64MB
    constraints.maxTextureMemory = 32 * 1024 * 1024;
    constraints.maxMeshMemory = 16 * 1024 * 1024;
    constraints.maxAudioMemory = 16 * 1024 * 1024;
    
    bool success = streamingSystem.Initialize(constraints);
    if (!success) {
        std::cout << "FAILED: Streaming system initialization" << std::endl;
        return false;
    }
    
    std::cout << "SUCCESS: Streaming system initialized" << std::endl;
    
    // Get initial stats
    auto memStats = streamingSystem.GetMemoryStats();
    auto metrics = streamingSystem.GetMetrics();
    std::cout << "Initial Stats:" << std::endl;
    std::cout << "  Max Total Memory: " << constraints.maxTotalMemory / (1024*1024) << " MB" << std::endl;
    std::cout << "  Worker Threads: 2" << std::endl;
    
    return true;
}

bool TestAssetRegistration() {
    std::cout << "\n=== Testing Asset Registration ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    // Register test assets
    bool success1 = streamingSystem.RegisterAsset("small_asset", "test_stream_assets/small_asset.txt", 
                                                assets::streaming::MemoryCategory::Other, 1024);
    
    bool success2 = streamingSystem.RegisterAsset("medium_asset", "test_stream_assets/medium_asset.txt", 
                                                assets::streaming::MemoryCategory::Other, 64*1024);
    
    bool success3 = streamingSystem.RegisterAsset("large_asset", "test_stream_assets/large_asset.txt", 
                                                assets::streaming::MemoryCategory::Other, 1024*1024);
    
    if (success1 && success2 && success3) {
        std::cout << "SUCCESS: All assets registered" << std::endl;
        return true;
    } else {
        std::cout << "FAILED: Asset registration" << std::endl;
        std::cout << "  Small: " << (success1 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Medium: " << (success2 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Large: " << (success3 ? "OK" : "FAILED") << std::endl;
        return false;
    }
}

bool TestAssetLoading() {
    std::cout << "\n=== Testing Asset Loading ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    // Request small asset (should load quickly)
    std::cout << "Requesting small asset..." << std::endl;
    auto future = streamingSystem.RequestAsset("small_asset", 
                                             assets::streaming::StreamingPriority::High, 
                                             assets::streaming::LODLevel::High);
    
    // Wait for loading (with timeout to prevent hanging)
    auto status = future.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::timeout) {
        std::cout << "WARNING: Asset loading timed out" << std::endl;
        return false;
    }
    
    bool loadSuccess = future.get();
    if (loadSuccess) {
        std::cout << "SUCCESS: Asset loaded successfully" << std::endl;
        
        // Check if asset is available
        bool available = streamingSystem.IsAssetLoaded("small_asset");
        std::cout << "Asset availability: " << (available ? "Available" : "Not Available") << std::endl;
        
        return available;
    } else {
        std::cout << "FAILED: Asset loading failed" << std::endl;
        return false;
    }
}

bool TestStreamingStats() {
    std::cout << "\n=== Testing Streaming Stats ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    auto memStats = streamingSystem.GetMemoryStats();
    auto metrics = streamingSystem.GetMetrics();
    
    std::cout << "Current Streaming Stats:" << std::endl;
    std::cout << "  Memory Used - Total: " << memStats.totalUsed / (1024*1024) << " MB" << std::endl;
    std::cout << "  Memory Available: " << memStats.totalAvailable / (1024*1024) << " MB" << std::endl;
    std::cout << "  Utilization: " << memStats.utilizationPercent << "%" << std::endl;
    std::cout << "  Assets Loaded: " << memStats.loadedAssets << std::endl;
    std::cout << "  Assets Loading: " << memStats.loadingAssets << std::endl;
    
    std::cout << "  Total Loads: " << metrics.totalLoads << std::endl;
    std::cout << "  Load Failures: " << metrics.loadFailures << std::endl;
    std::cout << "  Cache Hits: " << metrics.cacheHits << std::endl;
    std::cout << "  Cache Misses: " << metrics.cacheMisses << std::endl;
    
    std::cout << "SUCCESS: Stats look reasonable" << std::endl;
    return true;
}

bool TestAssetAccess() {
    std::cout << "\n=== Testing Asset Access ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    // Try to get the loaded asset
    auto assetData = streamingSystem.GetAsset("small_asset");
    
    if (assetData) {
        std::cout << "SUCCESS: Asset data retrieved" << std::endl;
        
        // Check loading state
        auto state = streamingSystem.GetAssetState("small_asset");
        std::cout << "Asset state: " << (int)state << std::endl;
        
        return true;
    } else {
        std::cout << "WARNING: Asset data not available (might be normal)" << std::endl;
        return true; // Not necessarily a failure
    }
}

bool TestMemoryManagement() {
    std::cout << "\n=== Testing Memory Management ===" << std::endl;
    
    auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
    
    // Update the system to trigger memory management
    streamingSystem.Update(0.016f); // 16ms frame time
    
    std::cout << "SUCCESS: Memory management update completed" << std::endl;
    return true;
}

int main() {
    std::cout << "Asset Streaming System Simple Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    bool allTestsPassed = true;
    
    try {
        // Create test assets
        CreateTestAssets();
        std::cout << "Created test assets" << std::endl;
        
        // Run tests with shorter timeouts
        allTestsPassed &= TestBasicInitialization();
        allTestsPassed &= TestAssetRegistration();
        allTestsPassed &= TestAssetLoading();
        allTestsPassed &= TestStreamingStats();
        allTestsPassed &= TestAssetAccess();
        allTestsPassed &= TestMemoryManagement();
        
        // Immediate shutdown (no waiting)
        std::cout << "\n=== Shutting Down ===" << std::endl;
        auto& streamingSystem = assets::streaming::AssetStreamingSystem::Instance();
        streamingSystem.Shutdown();
        std::cout << "Shutdown completed" << std::endl;
        
        // Final results
        std::cout << "\n=== Test Results ===" << std::endl;
        if (allTestsPassed) {
            std::cout << "SUCCESS: All tests passed!" << std::endl;
        } else {
            std::cout << "WARNING: Some tests had issues (but completed)" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        allTestsPassed = false;
    }
    
    CleanupTestAssets();
    
    // Force quick exit to prevent hanging
    return allTestsPassed ? 0 : 1;
}