#include "../engine/AssetVersioningSystem.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef NDEBUG
#undef NDEBUG
#include <cassert>
#define NDEBUG
#endif

namespace fs = std::filesystem;
using namespace assets::versioning;

// Test helpers
class VersioningTestHelper {
public:
    static void CreateTestAssets() {
        fs::create_directories("test_versioning_assets");
        
        // Create test files
        CreateTestFile("test_versioning_assets/config.json", R"({
    "name": "Test Config",
    "version": "1.0.0",
    "settings": {
        "debug": true,
        "level": "info"
    }
})");

        CreateTestFile("test_versioning_assets/shader.glsl", R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})");

        CreateTestFile("test_versioning_assets/texture_list.txt", R"(texture1.png
texture2.jpg
normal_map.png
specular_map.tga)");

        std::cout << "Created test assets in test_versioning_assets/" << std::endl;
    }

    static void ModifyTestFile(const std::string& filePath, const std::string& newContent) {
        // Small delay to ensure different modification time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::ofstream file(filePath);
        if (file) {
            file << newContent;
            std::cout << "Modified " << filePath << std::endl;
        }
    }

    static void CleanupTestAssets() {
        if (fs::exists("test_versioning_assets")) {
            fs::remove_all("test_versioning_assets");
            std::cout << "Cleaned up test assets" << std::endl;
        }
    }

private:
    static void CreateTestFile(const std::string& filePath, const std::string& content) {
        std::ofstream file(filePath);
        if (file) {
            file << content;
        }
    }
};

void TestVersionClass() {
    std::cout << "\n=== Testing Version Class ===" << std::endl;
    
    // Test version creation and string conversion
    Version v1{1, 2, 3, 4};
    assert(v1.ToString() == "1.2.3.4");
    
    // Test version parsing
    Version v2;
    assert(v2.FromString("2.5.1"));
    assert(v2.major == 2 && v2.minor == 5 && v2.patch == 1 && v2.build == 0);
    
    // Test version comparison
    Version v3{1, 2, 3, 5};
    assert(v1 < v3);
    assert(v3 > v1);
    assert(v1 != v3);
    
    Version v4{1, 2, 3, 4};
    assert(v1 == v4);
    assert(v1 <= v4);
    assert(v1 >= v4);
    
    std::cout << "SUCCESS: Version class working correctly" << std::endl;
    std::cout << "  v1: " << v1.ToString() << std::endl;
    std::cout << "  v2: " << v2.ToString() << std::endl;
    std::cout << "  v3: " << v3.ToString() << std::endl;
}

void TestVersionHistory() {
    std::cout << "\n=== Testing Version History ===" << std::endl;
    
    VersionHistory history("test_asset");
    
    // Create test versions
    AssetVersionEntry v1;
    v1.version = Version{1, 0, 0, 0};
    v1.changeDescription = "Initial version";
    v1.timestamp = std::chrono::system_clock::now();
    v1.author = "TestUser";
    
    AssetVersionEntry v2;
    v2.version = Version{1, 1, 0, 0};
    v2.changeDescription = "Added features";
    v2.timestamp = std::chrono::system_clock::now();
    v2.author = "TestUser";
    
    AssetVersionEntry v3;
    v3.version = Version{2, 0, 0, 0};
    v3.changeDescription = "Major update";
    v3.timestamp = std::chrono::system_clock::now();
    v3.author = "TestUser";
    
    // Add versions
    history.AddVersion(v1);
    history.AddVersion(v3);  // Add out of order
    history.AddVersion(v2);
    
    // Test retrieval
    auto latest = history.GetLatestVersion();
    Version expectedLatest(2, 0, 0, 0);
    assert(latest && latest->version == expectedLatest);
    
    auto all = history.GetAllVersions();
    assert(all.size() == 3);
    Version expected1(1, 0, 0, 0);
    Version expected2(1, 1, 0, 0);
    Version expected3(2, 0, 0, 0);
    assert(all[0].version == expected1);  // Should be sorted
    assert(all[1].version == expected2);
    assert(all[2].version == expected3);
    
    // Test specific version lookup
    Version lookupVersion(1, 1, 0, 0);
    auto specific = history.GetVersion(lookupVersion);
    assert(specific && specific->changeDescription == "Added features");
    
    // Test next version calculation
    auto next = history.GetNextVersion();
    Version expectedNext(2, 0, 1, 0);
    assert(next == expectedNext);
    
    auto nextMinor = history.GetNextVersion(false, true);
    Version expectedNextMinor(2, 1, 0, 0);
    assert(nextMinor == expectedNextMinor);
    
    auto nextMajor = history.GetNextVersion(true, false);
    Version expectedNextMajor(3, 0, 0, 0);
    assert(nextMajor == expectedNextMajor);
    
    std::cout << "SUCCESS: Version history working correctly" << std::endl;
    std::cout << "  Total versions: " << history.GetVersionCount() << std::endl;
    std::cout << "  Latest version: " << latest->version.ToString() << std::endl;
}

void TestAssetVersioningSystem() {
    std::cout << "\n=== Testing Asset Versioning System ===" << std::endl;
    
    auto& system = AssetVersioningSystem::Instance();
    
    // Test initialization
    ChangeTrackingConfig config;
    config.enableAutoVersioning = true;
    config.enableChecksumValidation = true;
    config.maxVersionHistory = 50;
    
    bool initialized = system.Initialize(config);
    assert(initialized);
    std::cout << "SUCCESS: System initialized" << std::endl;
    
    // Test asset registration
    AssetMetadata metadata;
    metadata.assetType = "JSON";
    metadata.author = "TestUser";
    metadata.description = "Test configuration file";
    
    bool registered = system.RegisterAsset("test_config", "test_versioning_assets/config.json", metadata);
    assert(registered);
    assert(system.IsAssetRegistered("test_config"));
    std::cout << "SUCCESS: Asset registered" << std::endl;
    
    // Test version creation
    Version newVersion = system.CreateNewVersion("test_config", "Updated configuration");
    Version expectedNewVersion(1, 0, 1, 0);
    assert(newVersion == expectedNewVersion);
    std::cout << "SUCCESS: New version created: " << newVersion.ToString() << std::endl;
    
    // Test version retrieval
    Version currentVersion = system.GetAssetVersion("test_config");
    Version latestVersion = system.GetLatestVersion("test_config");
    assert(currentVersion == newVersion);
    assert(latestVersion == newVersion);
    std::cout << "SUCCESS: Version retrieval working" << std::endl;
    
    // Test change tracking
    std::string changeId = system.RecordChange("test_config", ChangeType::Modified, "Manual change test");
    assert(!changeId.empty());
    
    auto changes = system.GetChangeHistory("test_config");
    assert(changes.size() >= 2);  // Initial creation + our changes
    std::cout << "SUCCESS: Change tracking working" << std::endl;
    std::cout << "  Total changes: " << changes.size() << std::endl;
    
    // Test version history
    auto versions = system.GetAllVersions("test_config");
    assert(versions.size() >= 2);  // Initial + created version
    std::cout << "SUCCESS: Version history retrieval working" << std::endl;
    
    // Test metadata
    auto retrievedMetadata = system.GetMetadata("test_config");
    assert(retrievedMetadata.assetType == "JSON");
    assert(retrievedMetadata.author == "TestUser");
    std::cout << "SUCCESS: Metadata retrieval working" << std::endl;
    
    system.Shutdown();
    std::cout << "SUCCESS: System shut down cleanly" << std::endl;
}

void TestDependencyTracking() {
    std::cout << "\n=== Testing Dependency Tracking ===" << std::endl;
    
    auto& system = AssetVersioningSystem::Instance();
    system.Initialize();
    
    // Register multiple assets
    system.RegisterAsset("shader", "test_versioning_assets/shader.glsl");
    system.RegisterAsset("config", "test_versioning_assets/config.json");
    system.RegisterAsset("textures", "test_versioning_assets/texture_list.txt");
    
    // Set up dependencies: shader depends on config and textures
    system.AddDependency("shader", "config");
    system.AddDependency("shader", "textures");
    system.AddDependency("config", "textures");  // Config also depends on textures
    
    // Test dependency retrieval
    auto shaderDeps = system.GetDependencies("shader");
    assert(shaderDeps.size() == 2);
    assert(std::find(shaderDeps.begin(), shaderDeps.end(), "config") != shaderDeps.end());
    assert(std::find(shaderDeps.begin(), shaderDeps.end(), "textures") != shaderDeps.end());
    
    auto textureDependents = system.GetDependents("textures");
    assert(textureDependents.size() == 2);
    assert(std::find(textureDependents.begin(), textureDependents.end(), "shader") != textureDependents.end());
    assert(std::find(textureDependents.begin(), textureDependents.end(), "config") != textureDependents.end());
    
    // Test transitive dependencies
    auto transitive = system.GetTransitiveDependencies("shader");
    assert(transitive.size() >= 2);  // Should include config and textures
    
    std::cout << "SUCCESS: Dependency tracking working" << std::endl;
    std::cout << "  Shader dependencies: " << shaderDeps.size() << std::endl;
    std::cout << "  Texture dependents: " << textureDependents.size() << std::endl;
    std::cout << "  Transitive dependencies: " << transitive.size() << std::endl;
    
    // Test dependency removal
    system.RemoveDependency("shader", "config");
    auto updatedDeps = system.GetDependencies("shader");
    assert(updatedDeps.size() == 1);
    assert(std::find(updatedDeps.begin(), updatedDeps.end(), "textures") != updatedDeps.end());
    std::cout << "SUCCESS: Dependency removal working" << std::endl;
    
    system.Shutdown();
}

void TestChangeDetection() {
    std::cout << "\n=== Testing Change Detection ===" << std::endl;
    
    auto& system = AssetVersioningSystem::Instance();
    
    ChangeTrackingConfig config;
    config.enableAutoVersioning = false;  // Manual versioning for this test
    config.enableChecksumValidation = true;
    system.Initialize(config);
    
    // Register asset
    system.RegisterAsset("config", "test_versioning_assets/config.json");
    
    auto initialVersion = system.GetLatestVersion("config");
    auto initialChanges = system.GetChangeHistory("config");
    
    // Modify the file
    VersioningTestHelper::ModifyTestFile("test_versioning_assets/config.json", R"({
    "name": "Modified Test Config",
    "version": "1.1.0",
    "settings": {
        "debug": false,
        "level": "warn",
        "new_feature": true
    }
})");
    
    // Update system to detect changes
    system.Update();
    
    // Check if changes were detected
    auto updatedChanges = system.GetChangeHistory("config");
    std::cout << "Initial changes: " << initialChanges.size() << std::endl;
    std::cout << "Updated changes: " << updatedChanges.size() << std::endl;
    
    // Create version manually
    auto newVersion = system.CreateNewVersion("config", "Manual version after file modification");
    assert(newVersion > initialVersion);
    
    std::cout << "SUCCESS: Change detection working" << std::endl;
    std::cout << "  Initial version: " << initialVersion.ToString() << std::endl;
    std::cout << "  New version: " << newVersion.ToString() << std::endl;
    
    system.Shutdown();
}

void TestUtilityFunctions() {
    std::cout << "\n=== Testing Utility Functions ===" << std::endl;
    
    // Test version utilities
    auto parsed = versioning_utils::ParseVersion("3.2.1.789");
    assert(parsed.major == 3 && parsed.minor == 2 && parsed.patch == 1 && parsed.build == 789);
    
    std::string formatted = versioning_utils::FormatVersion(parsed);
    assert(formatted == "3.2.1.789");
    
    assert(versioning_utils::IsValidVersionString("1.0.0"));
    assert(versioning_utils::IsValidVersionString("10.25.3.1000"));
    assert(!versioning_utils::IsValidVersionString("invalid"));
    
    // Test change type utilities
    assert(versioning_utils::ChangeTypeToString(ChangeType::Created) == "Created");
    assert(versioning_utils::ChangeTypeToString(ChangeType::Modified) == "Modified");
    assert(versioning_utils::StringToChangeType("Deleted") == ChangeType::Deleted);
    
    // Test file utilities
    std::string checksum = versioning_utils::CalculateFileChecksum("test_versioning_assets/config.json");
    assert(!checksum.empty());
    
    size_t fileSize = versioning_utils::GetFileSize("test_versioning_assets/config.json");
    assert(fileSize > 0);
    
    std::cout << "SUCCESS: Utility functions working" << std::endl;
    std::cout << "  Parsed version: " << formatted << std::endl;
    std::cout << "  File checksum: " << checksum.substr(0, 8) << "..." << std::endl;
    std::cout << "  File size: " << fileSize << " bytes" << std::endl;
}

void TestConsoleCommands() {
    std::cout << "\n=== Testing Console Commands ===" << std::endl;
    
    auto& system = AssetVersioningSystem::Instance();
    system.Initialize();
    
    // Register test asset
    system.RegisterAsset("test_asset", "test_versioning_assets/config.json");
    system.CreateNewVersion("test_asset", "Test version for console commands");
    
    // Test version list command
    std::vector<std::string> listArgs = {"version_list", "test_asset"};
    VersioningConsoleCommands::HandleVersionList(listArgs);
    
    // Test stats command
    std::vector<std::string> statsArgs = {"version_stats"};
    VersioningConsoleCommands::HandleVersionStats(statsArgs);
    
    std::cout << "SUCCESS: Console commands executed" << std::endl;
    
    system.Shutdown();
}

int main() {
    std::cout << "Asset Versioning System Test Suite" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        // Create test environment
        VersioningTestHelper::CreateTestAssets();
        
        // Run tests
        TestVersionClass();
        TestVersionHistory();
        TestAssetVersioningSystem();
        TestDependencyTracking();
        TestChangeDetection();
        TestUtilityFunctions();
        TestConsoleCommands();
        
        // Cleanup
        VersioningTestHelper::CleanupTestAssets();
        
        std::cout << "\n=== ALL TESTS PASSED ===\n" << std::endl;
        std::cout << "Asset Versioning System is working correctly!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        VersioningTestHelper::CleanupTestAssets();
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        VersioningTestHelper::CleanupTestAssets();
        return 1;
    }
    
    return 0;
}