#include "../engine/AssetCompressionSystem.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <random>

using namespace assets::compression;

// Advanced test for compression format comparison and analysis
int main() {
    std::cout << "Asset Compression System - Advanced Testing" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // Initialize compression system
    auto& system = AssetCompressionSystem::Instance();
    if (!system.Initialize()) {
        std::cerr << "FAILED: System initialization failed" << std::endl;
        return 1;
    }
    
    std::cout << "✓ System initialized successfully" << std::endl;
    
    // Test data generation
    std::vector<uint8_t> textData, binaryData, repetitiveData, randomData;
    
    // Generate different types of test data
    const size_t dataSize = 4096;
    
    // Text-like data (JSON/XML)
    textData.reserve(dataSize);
    std::string textContent = R"({"config":{"compression":{"enabled":true,"level":5},"data":[)";
    for (int i = 0; i < 100; ++i) {
        textContent += std::to_string(i) + ",";
    }
    textContent += "]}";
    while (textData.size() < dataSize) {
        for (char c : textContent) {
            if (textData.size() >= dataSize) break;
            textData.push_back(static_cast<uint8_t>(c));
        }
    }
    
    // Binary data (executable/mesh)
    binaryData.resize(dataSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < dataSize; ++i) {
        binaryData[i] = static_cast<uint8_t>(dis(gen));
    }
    
    // Repetitive data (texture with patterns)
    repetitiveData.resize(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        repetitiveData[i] = static_cast<uint8_t>(i % 16);
    }
    
    // Random data (encrypted/already compressed)
    randomData.resize(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        randomData[i] = static_cast<uint8_t>(dis(gen));
    }
    
    std::cout << "\n=== Testing Format Comparison ===\n" << std::endl;
    
    // Test data types
    struct TestCase {
        std::string name;
        std::vector<uint8_t> data;
        AssetType assetType;
    };
    
    std::vector<TestCase> testCases = {
        {"Text/JSON Data", textData, AssetType::Config_JSON},
        {"Binary Data", binaryData, AssetType::Config_Binary},
        {"Repetitive Data", repetitiveData, AssetType::Texture_Diffuse},
        {"Random Data", randomData, AssetType::Custom}
    };
    
    std::vector<CompressionFormat> formats = {
        CompressionFormat::LZ4,
        CompressionFormat::ZLIB
    };
    
    for (const auto& testCase : testCases) {
        std::cout << "Testing: " << testCase.name << " (" << testCase.data.size() << " bytes)" << std::endl;
        
        // Compare formats for this data type
        std::cout << "  Format comparison results:" << std::endl;
        
        for (auto format : formats) {
            CompressionParams params;
            params.format = format;
            params.assetType = testCase.assetType;
            
            CompressedData compressed;
            auto result = system.Compress(testCase.data, compressed, params);
            
            if (result.success) {
                std::cout << "    " << compression_utils::FormatToString(format) << ": "
                         << "ratio=" << result.compressionRatio << ":1, "
                         << "time=" << result.compressionTime.count() << "ms"
                         << std::endl;
            } else {
                std::cout << "    " << compression_utils::FormatToString(format) << ": FAILED" << std::endl;
            }
        }
        
        // Get optimal format recommendation
        auto optimalFormat = system.SelectOptimalFormat(testCase.assetType, TargetPlatform::PC_Desktop, testCase.data);
        std::cout << "  Recommended format: " << compression_utils::FormatToString(optimalFormat) << std::endl;
        
        std::cout << std::endl;
    }
    
    std::cout << "\n=== Testing Platform-Specific Optimization ===\n" << std::endl;
    
    // Test platform-specific optimization
    std::vector<TargetPlatform> platforms = {
        TargetPlatform::PC_Desktop,
        TargetPlatform::Mobile_Android,
        TargetPlatform::Web_Browser
    };
    
    for (auto platform : platforms) {
        std::cout << "Platform optimization for: ";
        switch (platform) {
            case TargetPlatform::PC_Desktop: std::cout << "PC Desktop"; break;
            case TargetPlatform::Mobile_Android: std::cout << "Android Mobile"; break;
            case TargetPlatform::Web_Browser: std::cout << "Web Browser"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << std::endl;
        
        // Test different asset types
        std::vector<AssetType> assetTypes = {
            AssetType::Texture_Diffuse,
            AssetType::Audio_Music,
            AssetType::Config_JSON,
            AssetType::Mesh_Static
        };
        
        for (auto assetType : assetTypes) {
            auto format = system.SelectOptimalFormat(assetType, platform);
            auto params = system.OptimizeParameters(format, assetType, platform);
            
            std::cout << "  ";
            switch (assetType) {
                case AssetType::Texture_Diffuse: std::cout << "Texture: "; break;
                case AssetType::Audio_Music: std::cout << "Audio: "; break;
                case AssetType::Config_JSON: std::cout << "Config: "; break;
                case AssetType::Mesh_Static: std::cout << "Mesh: "; break;
                default: std::cout << "Other: "; break;
            }
            std::cout << compression_utils::FormatToString(format);
            std::cout << " (quality=" << static_cast<int>(params.quality) << ")";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n=== Performance Benchmarking ===\n" << std::endl;
    
    // Performance test with larger data
    const size_t benchmarkSize = 64 * 1024; // 64KB
    std::vector<uint8_t> benchmarkData(benchmarkSize);
    for (size_t i = 0; i < benchmarkSize; ++i) {
        benchmarkData[i] = static_cast<uint8_t>(i % 256);
    }
    
    std::cout << "Benchmarking compression performance (64KB data):" << std::endl;
    
    for (auto format : {CompressionFormat::LZ4, CompressionFormat::ZLIB}) {
        CompressionParams params;
        params.format = format;
        params.quality = CompressionQuality::Balanced;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        CompressedData compressed;
        auto result = system.Compress(benchmarkData, compressed, params);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        if (result.success) {
            double throughput = (benchmarkSize / 1024.0) / (duration.count() / 1000000.0); // KB/s
            std::cout << "  " << compression_utils::FormatToString(format) << ": ";
            std::cout << "ratio=" << result.compressionRatio << ":1, ";
            std::cout << "time=" << duration.count() << "μs, ";
            std::cout << "throughput=" << std::fixed << std::setprecision(2) << throughput << " KB/s";
            std::cout << std::endl;
        }
    }
    
    // Display final statistics
    std::cout << "\n=== Final Statistics ===\n" << std::endl;
    auto stats = system.GetStats().GetGlobalStats();
    
    std::cout << "Total operations:" << std::endl;
    std::cout << "  Compressions: " << stats.totalCompressions << std::endl;
    std::cout << "  Decompressions: " << stats.totalDecompressions << std::endl;
    std::cout << "  Data processed: " << (stats.totalBytesCompressed / 1024) << " KB compressed, ";
    std::cout << (stats.totalBytesDecompressed / 1024) << " KB decompressed" << std::endl;
    std::cout << "  Average ratio: " << std::fixed << std::setprecision(3) << stats.averageCompressionRatio << ":1" << std::endl;
    
    std::cout << "\nFormat usage:" << std::endl;
    for (const auto& [format, count] : stats.formatUsage) {
        std::cout << "  " << compression_utils::FormatToString(format) << ": " << count << " times" << std::endl;
    }
    
    // Cleanup
    system.Shutdown();
    std::cout << "\n✓ System shut down successfully" << std::endl;
    
    std::cout << "\n=== ADVANCED TESTS COMPLETED SUCCESSFULLY ===\n" << std::endl;
    
    return 0;
}