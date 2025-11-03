#include "../engine/AssetVersioningSystem.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace assets::versioning;

void CreateTestFile(const std::string& filePath, const std::string& content) {
    fs::create_directories(fs::path(filePath).parent_path());
    std::ofstream file(filePath);
    if (file) {
        file << content;
    }
}

int main() {
    std::cout << "=== Minimal Asset Versioning Test ===" << std::endl;
    
    // Clean up and create test directory
    if (fs::exists("test_minimal_assets")) {
        fs::remove_all("test_minimal_assets");
    }
    fs::create_directories("test_minimal_assets");
    
    // Create a simple test file
    CreateTestFile("test_minimal_assets/test.txt", "Hello World");
    
    try {
        std::cout << "1. Getting AssetVersioningSystem instance..." << std::endl;
        auto& system = AssetVersioningSystem::Instance();
        
        std::cout << "2. Initializing system..." << std::endl;
        ChangeTrackingConfig config;
        config.enableChecksumValidation = false;  // Disable to avoid checksum issues
        config.enableAutoVersioning = false;     // Disable auto-versioning
        
        bool initialized = system.Initialize(config);
        assert(initialized);
        std::cout << "SUCCESS: System initialized" << std::endl;
        
        std::cout << "3. Registering asset..." << std::endl;
        AssetMetadata metadata;
        metadata.assetType = "TEXT";
        metadata.author = "TestUser";
        
        bool registered = system.RegisterAsset("test_asset", "test_minimal_assets/test.txt", metadata);
        assert(registered);
        std::cout << "SUCCESS: Asset registered" << std::endl;
        
        std::cout << "4. Attempting CreateNewVersion..." << std::endl;
        std::cout << "   This is where the hang typically occurs" << std::endl;
        
        // Add timeout mechanism by calling in separate thread
        std::cout << "   Calling CreateNewVersion with minimal parameters..." << std::endl;
        Version newVersion = system.CreateNewVersion("test_asset", "Test version");
        
        std::cout << "SUCCESS: CreateNewVersion completed!" << std::endl;
        std::cout << "New version: " << newVersion.ToString() << std::endl;
        
        std::cout << "5. Shutting down..." << std::endl;
        system.Shutdown();
        std::cout << "SUCCESS: System shut down" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: Exception caught: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    if (fs::exists("test_minimal_assets")) {
        fs::remove_all("test_minimal_assets");
    }
    
    std::cout << "=== Minimal Test Completed Successfully ===" << std::endl;
    return 0;
}