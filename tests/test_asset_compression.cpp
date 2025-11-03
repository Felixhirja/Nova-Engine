#include "../engine/AssetCompressionSystem.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fs = std::filesystem;
using namespace assets::compression;

// Helper functions for test data creation
void CreateBinaryFile(const std::string& filename, size_t size) {
    std::ofstream file(filename, std::ios::binary);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte = dis(gen);
        file.write(reinterpret_cast<const char*>(&byte), 1);
    }
}

std::vector<uint8_t> CreateTextureData(int width, int height) {
    std::vector<uint8_t> data(width * height * 4); // RGBA
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = dis(gen);
    }
    
    return data;
}

std::vector<uint8_t> CreateAudioData(int sampleCount, int channels = 2) {
    std::vector<uint8_t> data(sampleCount * channels * sizeof(int16_t));
    int16_t* samples = reinterpret_cast<int16_t*>(data.data());
    
    for (int i = 0; i < sampleCount * channels; ++i) {
        double t = static_cast<double>(i) / (44100.0 * channels);
        samples[i] = static_cast<int16_t>(32767.0 * sin(2.0 * M_PI * 440.0 * t));
    }
    
    return data;
}

class CompressionTestHelper {
public:
    static void CreateTestAssets() {
        // Clean up and create test directory
        if (fs::exists("test_compression_assets")) {
            fs::remove_all("test_compression_assets");
        }
        fs::create_directories("test_compression_assets");
        
        // Create test files for different compression scenarios
        CreateTextFile("test_compression_assets/test.json");
        CreateBinaryFile("test_compression_assets/binary_data.bin", 1024);
        auto textureData = CreateTextureDataVector(256, 256);
        auto audioData = CreateAudioDataVector(44100); // 1 second of audio
        
        // Save texture and audio data to files
        std::ofstream textureFile("test_compression_assets/texture.raw", std::ios::binary);
        textureFile.write(reinterpret_cast<const char*>(textureData.data()), textureData.size());
        textureFile.close();
        
        std::ofstream audioFile("test_compression_assets/audio.raw", std::ios::binary);
        audioFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        audioFile.close();
        
        std::cout << "Created test assets in test_compression_assets/" << std::endl;
    }
    
    static void CleanupTestAssets() {
        if (fs::exists("test_compression_assets")) {
            fs::remove_all("test_compression_assets");
            std::cout << "Cleaned up test assets" << std::endl;
        }
        
        if (fs::exists("test_compressed_output")) {
            fs::remove_all("test_compressed_output");
            std::cout << "Cleaned up compressed output" << std::endl;
        }
    }
    
    static std::vector<uint8_t> GenerateTestData(size_t size, bool repetitive = false) {
        std::vector<uint8_t> data(size);
        
        if (repetitive) {
            // Create repetitive data that compresses well
            for (size_t i = 0; i < size; i++) {
                data[i] = static_cast<uint8_t>(i % 16); // Repeating pattern
            }
        } else {
            // Create random data that doesn't compress well
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            
            for (size_t i = 0; i < size; i++) {
                data[i] = static_cast<uint8_t>(dis(gen));
            }
        }
        
        return data;
    }

private:
    static void CreateTextFile(const std::string& filename) {
        std::ofstream file(filename);
        file << R"({
    "test": true,
    "data": "Unicode test data with various characters",
    "numbers": [1, 2, 3, 4, 5],
    "nested": {
        "key": "value"
    }
})";
    }
    
    static void CreateBinaryFile(const std::string& filename, size_t size) {
        std::ofstream file(filename, std::ios::binary);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = 0; i < size; ++i) {
            uint8_t byte = dis(gen);
            file.write(reinterpret_cast<const char*>(&byte), 1);
        }
    }
    
    static std::vector<uint8_t> CreateTextureDataVector(int width, int height) {
        std::vector<uint8_t> data(width * height * 4); // RGBA
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
        
        return data;
    }
    
    static std::vector<uint8_t> CreateAudioDataVector(int sampleCount, int channels = 2) {
        std::vector<uint8_t> data(sampleCount * channels * sizeof(int16_t));
        int16_t* samples = reinterpret_cast<int16_t*>(data.data());
        
        for (int i = 0; i < sampleCount * channels; ++i) {
            double t = static_cast<double>(i) / (44100.0 * channels);
            samples[i] = static_cast<int16_t>(32767.0 * sin(2.0 * M_PI * 440.0 * t));
        }
        
        return data;
    }
};

void TestCompressionFormats() {
    std::cout << "\n=== Testing Compression Formats ===" << std::endl;
    
    auto& system = AssetCompressionSystem::Instance();
    
    // Test data
    std::vector<uint8_t> repetitiveData = CompressionTestHelper::GenerateTestData(1024, true);
    std::vector<uint8_t> randomData = CompressionTestHelper::GenerateTestData(1024, false);
    
    std::cout << "Test data created:" << std::endl;
    std::cout << "  Repetitive data: " << repetitiveData.size() << " bytes" << std::endl;
    std::cout << "  Random data: " << randomData.size() << " bytes" << std::endl;
    
    // Test different compression formats
    std::vector<CompressionFormat> formatsToTest = {
        CompressionFormat::LZ4,
        CompressionFormat::ZLIB,
        CompressionFormat::DXT1
    };
    
    for (CompressionFormat format : formatsToTest) {
        std::cout << "\nTesting format: " << compression_utils::FormatToString(format) << std::endl;
        
        // Test with repetitive data
        CompressionParams params(format);
        CompressedData compressed;
        
        CompressionResult result = system.Compress(repetitiveData, compressed, params);
        if (result.success) {
            std::cout << "  Repetitive data compression:" << std::endl;
            std::cout << "    Original: " << result.originalSize << " bytes" << std::endl;
            std::cout << "    Compressed: " << result.compressedSize << " bytes" << std::endl;
            std::cout << "    Ratio: " << result.compressionRatio << ":1" << std::endl;
            std::cout << "    Time: " << result.compressionTime.count() << "ms" << std::endl;
            
            // Test decompression
            std::vector<uint8_t> decompressed;
            CompressionResult decompResult = system.Decompress(compressed, decompressed, params);
            
            if (decompResult.success) {
                std::cout << "    Decompression: SUCCESS" << std::endl;
                std::cout << "    Decompressed size: " << decompressed.size() << " bytes" << std::endl;
                std::cout << "    Decompress time: " << decompResult.decompressionTime.count() << "ms" << std::endl;
                
                // Verify data integrity
                bool dataMatches = (decompressed == repetitiveData);
                std::cout << "    Data integrity: " << (dataMatches ? "PASS" : "FAIL") << std::endl;
                assert(dataMatches);
            } else {
                std::cout << "    Decompression: FAILED - " << decompResult.errorMessage << std::endl;
            }
        } else {
            std::cout << "  Compression FAILED: " << result.errorMessage << std::endl;
        }
        
        // Test with random data
        CompressedData compressedRandom;
        CompressionResult randomResult = system.Compress(randomData, compressedRandom, params);
        if (randomResult.success) {
            std::cout << "  Random data compression:" << std::endl;
            std::cout << "    Original: " << randomResult.originalSize << " bytes" << std::endl;
            std::cout << "    Compressed: " << randomResult.compressedSize << " bytes" << std::endl;
            std::cout << "    Ratio: " << randomResult.compressionRatio << ":1" << std::endl;
        }
    }
    
    std::cout << "SUCCESS: Compression formats tested" << std::endl;
}

void TestFileCompression() {
    std::cout << "\n=== Testing File Compression ===" << std::endl;
    
    auto& system = AssetCompressionSystem::Instance();
    
    // Create output directory
    fs::create_directories("test_compressed_output");
    
    // Test compressing different file types
    struct FileTest {
        std::string inputFile;
        std::string outputFile;
        CompressionFormat format;
        AssetType assetType;
    };
    
    std::vector<FileTest> fileTests = {
        {"test_compression_assets/config.json", "test_compressed_output/config.json.lz4", 
         CompressionFormat::LZ4, AssetType::Config_JSON},
        {"test_compression_assets/binary_data.bin", "test_compressed_output/binary_data.bin.zlib", 
         CompressionFormat::ZLIB, AssetType::Config_Binary},
        {"test_compression_assets/texture_rgba.raw", "test_compressed_output/texture_rgba.raw.dxt1", 
         CompressionFormat::DXT1, AssetType::Texture_Diffuse}
    };
    
    for (const auto& test : fileTests) {
        std::cout << "\nTesting file: " << test.inputFile << std::endl;
        
        if (!fs::exists(test.inputFile)) {
            std::cout << "  File not found, skipping..." << std::endl;
            continue;
        }
        
        // Get optimal compression parameters
        CompressionParams params = system.OptimizeParameters(
            test.format, test.assetType, TargetPlatform::Auto_Detect
        );
        
        // Compress file
        CompressionResult result = system.CompressFile(test.inputFile, test.outputFile, params);
        
        if (result.success) {
            std::cout << "  Compression: SUCCESS" << std::endl;
            std::cout << "    Original size: " << result.originalSize << " bytes" << std::endl;
            std::cout << "    Compressed size: " << result.compressedSize << " bytes" << std::endl;
            std::cout << "    Compression ratio: " << result.compressionRatio << ":1" << std::endl;
            std::cout << "    Time: " << result.compressionTime.count() << "ms" << std::endl;
            
            // Test decompression
            std::string decompressedFile = test.outputFile + ".decompressed";
            CompressionResult decompResult = system.DecompressFile(test.outputFile, decompressedFile, params);
            
            if (decompResult.success) {
                std::cout << "  Decompression: SUCCESS" << std::endl;
                std::cout << "    Decompressed size: " << decompResult.originalSize << " bytes" << std::endl;
                std::cout << "    Time: " << decompResult.decompressionTime.count() << "ms" << std::endl;
                
                // Verify file sizes match
                size_t originalSize = fs::file_size(test.inputFile);
                size_t decompressedSize = fs::file_size(decompressedFile);
                
                bool sizesMatch = (originalSize == decompressedSize);
                std::cout << "    Size verification: " << (sizesMatch ? "PASS" : "FAIL") << std::endl;
                
                if (!sizesMatch) {
                    std::cout << "      Original: " << originalSize << " bytes" << std::endl;
                    std::cout << "      Decompressed: " << decompressedSize << " bytes" << std::endl;
                }
                
                assert(sizesMatch);
            } else {
                std::cout << "  Decompression: FAILED - " << decompResult.errorMessage << std::endl;
            }
        } else {
            std::cout << "  Compression: FAILED - " << result.errorMessage << std::endl;
        }
    }
    
    std::cout << "SUCCESS: File compression tested" << std::endl;
}

void TestFormatSelection() {
    std::cout << "\n=== Testing Format Selection ===" << std::endl;
    
    auto& system = AssetCompressionSystem::Instance();
    
    struct SelectionTest {
        AssetType assetType;
        TargetPlatform platform;
        std::string description;
    };
    
    std::vector<SelectionTest> tests = {
        {AssetType::Texture_Diffuse, TargetPlatform::PC_Desktop, "Desktop texture"},
        {AssetType::Texture_Diffuse, TargetPlatform::Mobile_Android, "Android texture"},
        {AssetType::Audio_Music, TargetPlatform::PC_Desktop, "Desktop audio"},
        {AssetType::Config_JSON, TargetPlatform::Web_Browser, "Web config"},
        {AssetType::Animation_Data, TargetPlatform::Console_Xbox, "Console animation"}
    };
    
    for (const auto& test : tests) {
        std::cout << "\nTesting: " << test.description << std::endl;
        
        CompressionFormat selectedFormat = system.SelectOptimalFormat(
            test.assetType, test.platform
        );
        
        std::cout << "  Selected format: " << compression_utils::FormatToString(selectedFormat) << std::endl;
        
        // Get optimized parameters
        CompressionParams params = system.OptimizeParameters(
            selectedFormat, test.assetType, test.platform
        );
        
        std::cout << "  Quality level: ";
        switch (params.quality) {
            case CompressionQuality::Fastest: std::cout << "Fastest"; break;
            case CompressionQuality::Balanced: std::cout << "Balanced"; break;
            case CompressionQuality::Best: std::cout << "Best"; break;
            case CompressionQuality::Custom: std::cout << "Custom"; break;
        }
        std::cout << std::endl;
        
        std::cout << "  Multithreading: " << (params.enableMultithreading ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  Hardware accel: " << (params.enableHardwareAccel ? "Enabled" : "Disabled") << std::endl;
        
        if (params.maxMemoryUsage > 0) {
            std::cout << "  Memory limit: " << (params.maxMemoryUsage / (1024 * 1024)) << " MB" << std::endl;
        }
    }
    
    std::cout << "SUCCESS: Format selection tested" << std::endl;
}

void TestCompressionStatistics() {
    std::cout << "\n=== Testing Compression Statistics ===" << std::endl;
    
    auto& system = AssetCompressionSystem::Instance();
    
    // Get initial statistics
    auto initialStats = system.GetStats().GetGlobalStats();
    std::cout << "Initial statistics:" << std::endl;
    std::cout << "  Total compressions: " << initialStats.totalCompressions << std::endl;
    std::cout << "  Total decompressions: " << initialStats.totalDecompressions << std::endl;
    
    // Perform some compression operations
    std::vector<uint8_t> testData = CompressionTestHelper::GenerateTestData(2048, true);
    
    std::vector<CompressionFormat> formats = {
        CompressionFormat::LZ4,
        CompressionFormat::ZLIB
    };
    
    for (CompressionFormat format : formats) {
        CompressionParams params(format);
        CompressedData compressed;
        
        system.Compress(testData, compressed, params);
        
        std::vector<uint8_t> decompressed;
        system.Decompress(compressed, decompressed, params);
    }
    
    // Get updated statistics
    auto finalStats = system.GetStats().GetGlobalStats();
    std::cout << "\nFinal statistics:" << std::endl;
    std::cout << "  Total compressions: " << finalStats.totalCompressions << std::endl;
    std::cout << "  Total decompressions: " << finalStats.totalDecompressions << std::endl;
    std::cout << "  Total bytes compressed: " << finalStats.totalBytesCompressed << std::endl;
    std::cout << "  Total bytes decompressed: " << finalStats.totalBytesDecompressed << std::endl;
    std::cout << "  Average compression ratio: " << finalStats.averageCompressionRatio << std::endl;
    
    // Test format-specific statistics
    for (CompressionFormat format : formats) {
        auto formatStats = system.GetStats().GetFormatStats(format);
        std::cout << "\n" << compression_utils::FormatToString(format) << " statistics:" << std::endl;
        std::cout << "  Compressions: " << formatStats.compressions << std::endl;
        std::cout << "  Decompressions: " << formatStats.decompressions << std::endl;
        std::cout << "  Average ratio: " << formatStats.averageRatio << std::endl;
    }
    
    std::cout << "SUCCESS: Statistics tested" << std::endl;
}

void TestUtilityFunctions() {
    std::cout << "\n=== Testing Utility Functions ===" << std::endl;
    
    // Test format string conversion
    std::vector<CompressionFormat> formats = {
        CompressionFormat::LZ4,
        CompressionFormat::ZLIB,
        CompressionFormat::DXT1,
        CompressionFormat::OGG_VORBIS
    };
    
    std::cout << "Format string conversion:" << std::endl;
    for (CompressionFormat format : formats) {
        std::string formatStr = compression_utils::FormatToString(format);
        CompressionFormat parsedFormat = compression_utils::StringToFormat(formatStr);
        
        std::cout << "  " << formatStr << " -> ";
        std::cout << (format == parsedFormat ? "PASS" : "FAIL") << std::endl;
        assert(format == parsedFormat);
    }
    
    // Test format type detection
    std::cout << "\nFormat type detection:" << std::endl;
    
    struct FormatTypeTest {
        CompressionFormat format;
        std::string type;
        std::function<bool(CompressionFormat)> checker;
    };
    
    std::vector<FormatTypeTest> typeTests = {
        {CompressionFormat::DXT1, "texture", compression_utils::IsTextureFormat},
        {CompressionFormat::OGG_VORBIS, "audio", compression_utils::IsAudioFormat},
        {CompressionFormat::DXT5, "lossy", compression_utils::IsLossyFormat},
        {CompressionFormat::LZ4, "lossless", [](CompressionFormat f) { return !compression_utils::IsLossyFormat(f); }}
    };
    
    for (const auto& test : typeTests) {
        std::string formatName = compression_utils::FormatToString(test.format);
        bool result = test.checker(test.format);
        std::cout << "  " << formatName << " is " << test.type << ": " 
                 << (result ? "PASS" : "FAIL") << std::endl;
    }
    
    // Test compression ratio calculation
    float ratio = compression_utils::CalculateCompressionRatio(1000, 250);
    std::cout << "\nCompression ratio calculation:" << std::endl;
    std::cout << "  1000 -> 250 bytes = " << ratio << ":1" << std::endl;
    assert(std::abs(ratio - 4.0f) < 0.001f);
    
    std::cout << "SUCCESS: Utility functions tested" << std::endl;
}

int main() {
    std::cout << "Asset Compression System Test Suite" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Create test assets
    CompressionTestHelper::CreateTestAssets();
    
    try {
        // Initialize compression system
        auto& system = AssetCompressionSystem::Instance();
        
        bool initialized = system.Initialize();
        if (!initialized) {
            std::cerr << "FAILED: System initialization failed" << std::endl;
            return 1;
        }
        std::cout << "SUCCESS: System initialized" << std::endl;
        
        // Get available formats
        auto formats = system.GetAvailableFormats();
        std::cout << "Available compression formats: " << formats.size() << std::endl;
        for (CompressionFormat format : formats) {
            std::cout << "  - " << compression_utils::FormatToString(format) << std::endl;
        }
        
        // Run tests
        TestCompressionFormats();
        TestFileCompression();
        TestFormatSelection();
        TestCompressionStatistics();
        TestUtilityFunctions();
        
        // Shutdown system
        system.Shutdown();
        std::cout << "SUCCESS: System shut down cleanly" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: Exception caught: " << e.what() << std::endl;
        CompressionTestHelper::CleanupTestAssets();
        return 1;
    }
    
    // Cleanup
    CompressionTestHelper::CleanupTestAssets();
    
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    std::cout << "\nAsset Compression System is working correctly!" << std::endl;
    
    return 0;
}