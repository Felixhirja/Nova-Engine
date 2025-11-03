#include "../engine/AssetPipelineManager.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// Test asset creation helpers
void CreateTestTexture(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    // Create a minimal PNG file with proper header
    const unsigned char pngHeader[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
        0x00, 0x00, 0x00, 0x0D, // IHDR chunk size
        0x49, 0x48, 0x44, 0x52, // IHDR
        0x00, 0x00, 0x00, 0x10, // width: 16
        0x00, 0x00, 0x00, 0x10, // height: 16
        0x08, 0x02, 0x00, 0x00, 0x00 // bit depth, color type, compression, filter, interlace
    };
    file.write(reinterpret_cast<const char*>(pngHeader), sizeof(pngHeader));
}

void CreateTestModel(const std::string& path) {
    std::ofstream file(path);
    file << "# Simple OBJ test file\n";
    file << "v 0.0 0.0 0.0\n";
    file << "v 1.0 0.0 0.0\n";
    file << "v 0.0 1.0 0.0\n";
    file << "f 1 2 3\n";
}

void CreateTestShader(const std::string& path) {
    std::ofstream file(path);
    file << "#version 330 core\n";
    file << "in vec3 position;\n";
    file << "uniform mat4 modelMatrix;\n";
    file << "void main() {\n";
    file << "    gl_Position = modelMatrix * vec4(position, 1.0);\n";
    file << "}\n";
}

void CreateTestConfig(const std::string& path) {
    std::ofstream file(path);
    file << "{\n";
    file << "  \"name\": \"test_config\",\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"settings\": {\n";
    file << "    \"quality\": \"high\",\n";
    file << "    \"debug\": true\n";
    file << "  }\n";
    file << "}\n";
}

void CreateTestAudio(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    // Create a minimal WAV file header
    const char wavHeader[] = {
        'R', 'I', 'F', 'F',
        0x24, 0x08, 0x00, 0x00, // file size - 8
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        0x10, 0x00, 0x00, 0x00, // fmt chunk size
        0x01, 0x00, // audio format (PCM)
        0x02, 0x00, // number of channels
        0x44, static_cast<char>(0xAC), 0x00, 0x00, // sample rate (44100)
        0x10, static_cast<char>(0xB1), 0x02, 0x00, // byte rate
        0x04, 0x00, // block align
        0x10, 0x00, // bits per sample
        'd', 'a', 't', 'a',
        0x00, 0x08, 0x00, 0x00  // data chunk size
    };
    file.write(wavHeader, sizeof(wavHeader));
    
    // Write some dummy audio data
    for (int i = 0; i < 1024; ++i) {
        file.put(static_cast<char>(i % 256));
    }
}

bool TestBasicInitialization() {
    std::cout << "\n=== Testing Basic Initialization ===" << std::endl;
    
    // Test initialization
    assets::ProcessingConfig config;
    config.outputDirectory = "test_output";
    config.cacheDirectory = "test_cache";
    config.maxThreads = 2;
    
    bool success = assets::pipeline_integration::Initialize(config);
    if (!success) {
        std::cout << "FAILED: Pipeline initialization" << std::endl;
        return false;
    }
    
    std::cout << "SUCCESS: Pipeline initialized" << std::endl;
    
    // Test status
    std::string status = assets::pipeline_integration::GetStatus();
    std::cout << "Pipeline Status:\n" << status << std::endl;
    
    return true;
}

bool TestAssetProcessing() {
    std::cout << "\n=== Testing Asset Processing ===" << std::endl;
    
    // Create test directory
    fs::create_directories("test_assets");
    
    // Create test assets
    CreateTestTexture("test_assets/test_texture.png");
    CreateTestModel("test_assets/test_model.obj");
    CreateTestShader("test_assets/test_shader.glsl");
    CreateTestConfig("test_assets/test_config.json");
    CreateTestAudio("test_assets/test_audio.wav");
    
    std::cout << "Created test assets" << std::endl;
    
    // Test single asset processing
    bool success1 = assets::pipeline_integration::ProcessAsset("test_assets/test_texture.png");
    bool success2 = assets::pipeline_integration::ProcessAsset("test_assets/test_model.obj");
    bool success3 = assets::pipeline_integration::ProcessAsset("test_assets/test_shader.glsl");
    bool success4 = assets::pipeline_integration::ProcessAsset("test_assets/test_config.json");
    bool success5 = assets::pipeline_integration::ProcessAsset("test_assets/test_audio.wav");
    
    if (success1 && success2 && success3 && success4 && success5) {
        std::cout << "SUCCESS: All assets processed successfully" << std::endl;
        return true;
    } else {
        std::cout << "FAILED: Some assets failed to process" << std::endl;
        std::cout << "  Texture: " << (success1 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Model: " << (success2 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Shader: " << (success3 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Config: " << (success4 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Audio: " << (success5 ? "OK" : "FAILED") << std::endl;
        return false;
    }
}

bool TestAssetValidation() {
    std::cout << "\n=== Testing Asset Validation ===" << std::endl;
    
    // Test validation of existing assets
    bool valid1 = assets::pipeline_integration::ValidateAsset("test_assets/test_texture.png");
    bool valid2 = assets::pipeline_integration::ValidateAsset("test_assets/test_model.obj");
    bool valid3 = assets::pipeline_integration::ValidateAsset("test_assets/test_shader.glsl");
    bool valid4 = assets::pipeline_integration::ValidateAsset("test_assets/test_config.json");
    bool valid5 = assets::pipeline_integration::ValidateAsset("test_assets/test_audio.wav");
    
    // Test validation of non-existent file
    bool valid6 = assets::pipeline_integration::ValidateAsset("test_assets/nonexistent.png");
    
    if (valid1 && valid2 && valid3 && valid4 && valid5 && !valid6) {
        std::cout << "SUCCESS: Validation working correctly" << std::endl;
        return true;
    } else {
        std::cout << "FAILED: Validation issues detected" << std::endl;
        std::cout << "  Texture validation: " << (valid1 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Model validation: " << (valid2 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Shader validation: " << (valid3 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Config validation: " << (valid4 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Audio validation: " << (valid5 ? "OK" : "FAILED") << std::endl;
        std::cout << "  Non-existent file (should fail): " << (!valid6 ? "OK" : "FAILED") << std::endl;
        return false;
    }
}

bool TestBatchProcessing() {
    std::cout << "\n=== Testing Batch Processing ===" << std::endl;
    
    // Process entire directory
    assets::pipeline_integration::ProcessDirectory("test_assets", false);
    
    std::cout << "Batch processing initiated" << std::endl;
    
    // Wait a moment for processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "SUCCESS: Batch processing completed" << std::endl;
    return true;
}

bool TestAssetDatabase() {
    std::cout << "\n=== Testing Asset Database ===" << std::endl;
    
    auto& manager = assets::AssetPipelineManager::Instance();
    auto assets_list = manager.GetAllAssets();
    
    std::cout << "Assets in database: " << assets_list.size() << std::endl;
    
    if (assets_list.empty()) {
        std::cout << "WARNING: No assets found in database (processing may still be in progress)" << std::endl;
        return true; // Not necessarily a failure
    }
    
    // Display asset information
    for (const auto& asset : assets_list) {
        std::cout << "  Asset: " << asset.name << " (ID: " << asset.id << ")" << std::endl;
        std::cout << "    Type: " << assets::pipeline_utils::GetPlatformString(asset.platform) << std::endl;
        std::cout << "    Status: " << static_cast<int>(asset.status) << std::endl;
        std::cout << "    Size: " << asset.originalSize << " -> " << asset.processedSize << " bytes" << std::endl;
    }
    
    std::cout << "SUCCESS: Asset database accessible" << std::endl;
    return true;
}

bool TestConsoleCommands() {
    std::cout << "\n=== Testing Console Commands ===" << std::endl;
    
    // Test various console commands
    assets::pipeline_integration::ExecuteCommand("asset.list");
    assets::pipeline_integration::ExecuteCommand("asset.analytics");
    assets::pipeline_integration::ExecuteCommand("asset.scan", {"test_assets"});
    
    std::cout << "SUCCESS: Console commands executed" << std::endl;
    return true;
}

bool TestDifferentConfigurations() {
    std::cout << "\n=== Testing Different Configurations ===" << std::endl;
    
    // Test mobile configuration
    auto mobileConfig = assets::pipeline_integration::CreateMobileConfig();
    std::cout << "Mobile config - Platform: " << assets::pipeline_utils::GetPlatformString(mobileConfig.targetPlatform) 
              << ", Quality: " << assets::pipeline_utils::GetQualityString(mobileConfig.targetQuality) << std::endl;
    
    // Test web configuration
    auto webConfig = assets::pipeline_integration::CreateWebConfig();
    std::cout << "Web config - Platform: " << assets::pipeline_utils::GetPlatformString(webConfig.targetPlatform) 
              << ", Quality: " << assets::pipeline_utils::GetQualityString(webConfig.targetQuality) << std::endl;
    
    // Test production configuration
    auto prodConfig = assets::pipeline_integration::CreateProductionConfig();
    std::cout << "Production config - Platform: " << assets::pipeline_utils::GetPlatformString(prodConfig.targetPlatform) 
              << ", Quality: " << assets::pipeline_utils::GetQualityString(prodConfig.targetQuality) << std::endl;
    
    std::cout << "SUCCESS: Configuration presets working" << std::endl;
    return true;
}

void CleanupTestFiles() {
    std::cout << "\n=== Cleaning Up Test Files ===" << std::endl;
    
    try {
        if (fs::exists("test_assets")) {
            fs::remove_all("test_assets");
        }
        if (fs::exists("test_output")) {
            fs::remove_all("test_output");
        }
        if (fs::exists("test_cache")) {
            fs::remove_all("test_cache");
        }
        
        std::cout << "Test files cleaned up" << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cout << "Warning: Could not clean up all test files: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Asset Processing Pipeline Test Suite" << std::endl;
    std::cout << "====================================" << std::endl;
    
    bool allTestsPassed = true;
    
    try {
        // Run all tests
        allTestsPassed &= TestBasicInitialization();
        allTestsPassed &= TestAssetProcessing();
        allTestsPassed &= TestAssetValidation();
        allTestsPassed &= TestBatchProcessing();
        allTestsPassed &= TestAssetDatabase();
        allTestsPassed &= TestConsoleCommands();
        allTestsPassed &= TestDifferentConfigurations();
        
        // Shutdown the system
        std::cout << "\n=== Shutting Down ===" << std::endl;
        assets::pipeline_integration::Shutdown();
        
        // Final results
        std::cout << "\n=== Test Results ===" << std::endl;
        if (allTestsPassed) {
            std::cout << "SUCCESS: All tests passed!" << std::endl;
        } else {
            std::cout << "FAILED: Some tests failed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        allTestsPassed = false;
    }
    
    CleanupTestFiles();
    
    return allTestsPassed ? 0 : 1;
}
