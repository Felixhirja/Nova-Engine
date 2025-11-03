#include "AssetCompressionSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <future>
#include <regex>
#include <random>
#include <chrono>
#include <cmath>
#include <filesystem>

// Include compression libraries (simplified for this implementation)
// In a real implementation, these would be actual compression library headers
#ifdef USE_ZLIB
// #include <zlib.h>
#endif

#ifdef USE_LZ4
// #include <lz4.h>
#endif

#ifdef USE_ZSTD
// #include <zstd.h>
#endif

namespace fs = std::filesystem;

namespace assets {
    namespace compression {

        // Built-in codec implementations
        namespace builtin_codecs {
            
            // LZ4 Codec Implementation
            class LZ4Codec : public ICompressionCodec {
            public:
                CompressionFormat GetFormat() const override { return CompressionFormat::LZ4; }
                std::string GetName() const override { return "LZ4 Fast Compression"; }
                
                std::vector<AssetType> GetSupportedAssetTypes() const override {
                    return {
                        AssetType::Config_JSON, AssetType::Config_Binary,
                        AssetType::Shader_Source, AssetType::Font_Data,
                        AssetType::Animation_Data, AssetType::Custom
                    };
                }
                
                std::vector<TargetPlatform> GetSupportedPlatforms() const override {
                    return {
                        TargetPlatform::PC_Desktop, TargetPlatform::PC_Mobile,
                        TargetPlatform::Console_PlayStation, TargetPlatform::Console_Xbox,
                        TargetPlatform::Mobile_Android, TargetPlatform::Mobile_iOS,
                        TargetPlatform::Web_Browser
                    };
                }
                
                bool SupportsStreaming() const override { return true; }
                bool SupportsHardwareAcceleration() const override { return false; }
                bool SupportsMultithreading() const override { return true; }
                bool SupportsQualityLevels() const override { return false; }
                
                CompressionResult Compress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.originalSize = input.size();
                    result.format = CompressionFormat::LZ4;
                    
                    try {
                        // Simulate LZ4 compression
                        // In real implementation, use LZ4_compress_default()
                        output.clear();
                        
                        // Simple run-length encoding simulation for demonstration
                        size_t i = 0;
                        while (i < input.size()) {
                            uint8_t current = input[i];
                            size_t count = 1;
                            
                            // Count consecutive identical bytes
                            while (i + count < input.size() && input[i + count] == current && count < 255) {
                                count++;
                            }
                            
                            if (count > 3) {
                                // Use RLE for runs of 4 or more
                                output.push_back(0xFF); // Escape byte
                                output.push_back(static_cast<uint8_t>(count));
                                output.push_back(current);
                            } else {
                                // Store literal bytes
                                for (size_t j = 0; j < count; j++) {
                                    output.push_back(current);
                                }
                            }
                            
                            i += count;
                        }
                        
                        result.compressedSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "LZ4 compression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.compressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                CompressionResult Decompress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.compressedSize = input.size();
                    result.format = CompressionFormat::LZ4;
                    
                    try {
                        // Simulate LZ4 decompression
                        output.clear();
                        
                        size_t i = 0;
                        while (i < input.size()) {
                            if (input[i] == 0xFF && i + 2 < input.size()) {
                                // RLE decompression
                                uint8_t count = input[i + 1];
                                uint8_t value = input[i + 2];
                                
                                for (uint8_t j = 0; j < count; j++) {
                                    output.push_back(value);
                                }
                                
                                i += 3;
                            } else {
                                // Literal byte
                                output.push_back(input[i]);
                                i++;
                            }
                        }
                        
                        result.originalSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "LZ4 decompression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.decompressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                bool ValidateParams(const CompressionParams& params) const override {
                    return params.format == CompressionFormat::LZ4;
                }
            };
            
            // ZLIB Codec Implementation
            class ZLIBCodec : public ICompressionCodec {
            public:
                CompressionFormat GetFormat() const override { return CompressionFormat::ZLIB; }
                std::string GetName() const override { return "ZLIB Compression"; }
                
                std::vector<AssetType> GetSupportedAssetTypes() const override {
                    return {
                        AssetType::Config_JSON, AssetType::Config_Binary,
                        AssetType::Shader_Source, AssetType::Font_Data,
                        AssetType::Animation_Data, AssetType::Texture_UI,
                        AssetType::Custom
                    };
                }
                
                std::vector<TargetPlatform> GetSupportedPlatforms() const override {
                    return {
                        TargetPlatform::PC_Desktop, TargetPlatform::PC_Mobile,
                        TargetPlatform::Console_PlayStation, TargetPlatform::Console_Xbox,
                        TargetPlatform::Console_Nintendo, TargetPlatform::Mobile_Android,
                        TargetPlatform::Mobile_iOS, TargetPlatform::Web_Browser
                    };
                }
                
                bool SupportsStreaming() const override { return true; }
                bool SupportsHardwareAcceleration() const override { return false; }
                bool SupportsMultithreading() const override { return true; }
                bool SupportsQualityLevels() const override { return true; }
                
                CompressionResult Compress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.originalSize = input.size();
                    result.format = CompressionFormat::ZLIB;
                    
                    try {
                        // Simulate ZLIB compression with better ratio than LZ4
                        output.clear();
                        
                        // Huffman-style compression simulation
                        std::unordered_map<uint8_t, size_t> frequency;
                        for (uint8_t byte : input) {
                            frequency[byte]++;
                        }
                        
                        // Create simple dictionary
                        std::vector<std::pair<uint8_t, size_t>> sortedFreq(frequency.begin(), frequency.end());
                        std::sort(sortedFreq.begin(), sortedFreq.end(),
                            [](const auto& a, const auto& b) { return a.second > b.second; });
                        
                        // Store dictionary (simplified)
                        output.push_back(static_cast<uint8_t>(sortedFreq.size()));
                        for (const auto& pair : sortedFreq) {
                            output.push_back(pair.first);
                        }
                        
                        // Compress data using dictionary indices
                        for (uint8_t byte : input) {
                            auto it = std::find_if(sortedFreq.begin(), sortedFreq.end(),
                                [byte](const auto& pair) { return pair.first == byte; });
                            if (it != sortedFreq.end()) {
                                output.push_back(static_cast<uint8_t>(std::distance(sortedFreq.begin(), it)));
                            }
                        }
                        
                        result.compressedSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "ZLIB compression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.compressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                CompressionResult Decompress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.compressedSize = input.size();
                    result.format = CompressionFormat::ZLIB;
                    
                    try {
                        output.clear();
                        
                        if (input.empty()) {
                            result.success = false;
                            result.errorMessage = "Empty input data";
                            return result;
                        }
                        
                        // Read dictionary
                        size_t dictSize = input[0];
                        if (input.size() < 1 + dictSize) {
                            result.success = false;
                            result.errorMessage = "Invalid compressed data format";
                            return result;
                        }
                        
                        std::vector<uint8_t> dictionary;
                        for (size_t i = 0; i < dictSize; i++) {
                            dictionary.push_back(input[1 + i]);
                        }
                        
                        // Decompress data
                        for (size_t i = 1 + dictSize; i < input.size(); i++) {
                            uint8_t index = input[i];
                            if (index < dictionary.size()) {
                                output.push_back(dictionary[index]);
                            }
                        }
                        
                        result.originalSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "ZLIB decompression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.decompressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                bool ValidateParams(const CompressionParams& params) const override {
                    return params.format == CompressionFormat::ZLIB;
                }
            };
            
            // DXT1 Texture Codec (simplified implementation)
            class DXT1Codec : public ICompressionCodec {
            public:
                CompressionFormat GetFormat() const override { return CompressionFormat::DXT1; }
                std::string GetName() const override { return "DXT1 Texture Compression"; }
                
                std::vector<AssetType> GetSupportedAssetTypes() const override {
                    return {
                        AssetType::Texture_Diffuse, AssetType::Texture_Specular,
                        AssetType::Texture_UI
                    };
                }
                
                std::vector<TargetPlatform> GetSupportedPlatforms() const override {
                    return {
                        TargetPlatform::PC_Desktop, TargetPlatform::Console_Xbox,
                        TargetPlatform::Console_PlayStation
                    };
                }
                
                bool SupportsStreaming() const override { return false; }
                bool SupportsHardwareAcceleration() const override { return true; }
                bool SupportsMultithreading() const override { return true; }
                bool SupportsQualityLevels() const override { return true; }
                
                CompressionResult Compress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.originalSize = input.size();
                    result.format = CompressionFormat::DXT1;
                    
                    try {
                        // Simulate DXT1 compression (4:1 ratio for RGB textures)
                        // DXT1 compresses 4x4 pixel blocks into 8 bytes
                        
                        // Assume input is RGBA format (4 bytes per pixel)
                        if (input.size() % 4 != 0) {
                            result.success = false;
                            result.errorMessage = "Input data must be in RGBA format";
                            return result;
                        }
                        
                        size_t pixelCount = input.size() / 4;
                        size_t blockCount = (pixelCount + 15) / 16; // Round up to nearest 4x4 block
                        
                        output.resize(blockCount * 8); // 8 bytes per DXT1 block
                        
                        // Simulate compression by downsampling and quantizing
                        for (size_t block = 0; block < blockCount; block++) {
                            size_t baseIndex = block * 8;
                            
                            // Simulate DXT1 block format
                            // 2 bytes for color0, 2 bytes for color1, 4 bytes for indices
                            
                            // Color endpoints (simplified)
                            output[baseIndex + 0] = static_cast<uint8_t>(block & 0xFF);
                            output[baseIndex + 1] = static_cast<uint8_t>((block >> 8) & 0xFF);
                            output[baseIndex + 2] = static_cast<uint8_t>((block >> 4) & 0xFF);
                            output[baseIndex + 3] = static_cast<uint8_t>((block >> 12) & 0xFF);
                            
                            // Indices for 4x4 block (2 bits per pixel)
                            output[baseIndex + 4] = static_cast<uint8_t>((block * 73) & 0xFF);
                            output[baseIndex + 5] = static_cast<uint8_t>((block * 137) & 0xFF);
                            output[baseIndex + 6] = static_cast<uint8_t>((block * 211) & 0xFF);
                            output[baseIndex + 7] = static_cast<uint8_t>((block * 251) & 0xFF);
                        }
                        
                        result.compressedSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.qualityScore = 0.85f; // Simulate quality loss
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "DXT1 compression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.compressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                CompressionResult Decompress(
                    const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    const CompressionParams& params = {}
                ) override {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    CompressionResult result;
                    result.compressedSize = input.size();
                    result.format = CompressionFormat::DXT1;
                    
                    try {
                        // Simulate DXT1 decompression
                        if (input.size() % 8 != 0) {
                            result.success = false;
                            result.errorMessage = "Invalid DXT1 compressed data size";
                            return result;
                        }
                        
                        size_t blockCount = input.size() / 8;
                        output.resize(blockCount * 16 * 4); // 16 pixels per block, 4 bytes per pixel
                        
                        for (size_t block = 0; block < blockCount; block++) {
                            size_t blockOffset = block * 8;
                            size_t outputOffset = block * 16 * 4;
                            
                            // Extract color endpoints
                            uint16_t color0 = (input[blockOffset + 1] << 8) | input[blockOffset + 0];
                            uint16_t color1 = (input[blockOffset + 3] << 8) | input[blockOffset + 2];
                            
                            // Convert to RGB
                            uint8_t r0 = (color0 >> 11) << 3;
                            uint8_t g0 = ((color0 >> 5) & 0x3F) << 2;
                            uint8_t b0 = (color0 & 0x1F) << 3;
                            
                            uint8_t r1 = (color1 >> 11) << 3;
                            uint8_t g1 = ((color1 >> 5) & 0x3F) << 2;
                            uint8_t b1 = (color1 & 0x1F) << 3;
                            
                            // Fill 4x4 block with interpolated colors
                            for (int pixel = 0; pixel < 16; pixel++) {
                                size_t pixelOffset = outputOffset + pixel * 4;
                                
                                // Simplified interpolation
                                float t = static_cast<float>(pixel) / 15.0f;
                                output[pixelOffset + 0] = static_cast<uint8_t>(r0 * (1.0f - t) + r1 * t); // R
                                output[pixelOffset + 1] = static_cast<uint8_t>(g0 * (1.0f - t) + g1 * t); // G
                                output[pixelOffset + 2] = static_cast<uint8_t>(b0 * (1.0f - t) + b1 * t); // B
                                output[pixelOffset + 3] = 255; // A
                            }
                        }
                        
                        result.originalSize = output.size();
                        result.compressionRatio = static_cast<float>(result.originalSize) / result.compressedSize;
                        result.success = true;
                        
                    } catch (const std::exception& e) {
                        result.success = false;
                        result.errorMessage = "DXT1 decompression failed: " + std::string(e.what());
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    result.decompressionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    return result;
                }
                
                bool ValidateParams(const CompressionParams& params) const override {
                    return params.format == CompressionFormat::DXT1;
                }
                
                float EstimateQuality(
                    const std::vector<uint8_t>& original,
                    const std::vector<uint8_t>& compressed,
                    const CompressionParams& params = {}
                ) const override {
                    // Simulate quality estimation for lossy texture compression
                    return 0.85f; // DXT1 typically achieves ~85% visual quality
                }
            };
        }

        // CompressedData implementation
        CompressedData::CompressedData(std::vector<uint8_t> data, CompressionFormat format, CompressionParams params)
            : data_(std::move(data)), format_(format), params_(std::move(params)) {
        }

        void CompressedData::SetMetadata(const std::string& key, const std::string& value) {
            metadata_[key] = value;
        }

        std::string CompressedData::GetMetadata(const std::string& key) const {
            auto it = metadata_.find(key);
            return (it != metadata_.end()) ? it->second : "";
        }

        bool CompressedData::SaveToFile(const std::string& filePath) const {
            try {
                std::ofstream file(filePath, std::ios::binary);
                if (!file) return false;
                
                // Write format identifier
                uint32_t formatId = static_cast<uint32_t>(format_);
                file.write(reinterpret_cast<const char*>(&formatId), sizeof(formatId));
                
                // Write data size
                uint64_t dataSize = data_.size();
                file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                
                // Write data
                file.write(reinterpret_cast<const char*>(data_.data()), data_.size());
                
                // Write metadata
                uint32_t metadataCount = static_cast<uint32_t>(metadata_.size());
                file.write(reinterpret_cast<const char*>(&metadataCount), sizeof(metadataCount));
                
                for (const auto& pair : metadata_) {
                    uint32_t keySize = static_cast<uint32_t>(pair.first.size());
                    uint32_t valueSize = static_cast<uint32_t>(pair.second.size());
                    
                    file.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
                    file.write(pair.first.c_str(), keySize);
                    file.write(reinterpret_cast<const char*>(&valueSize), sizeof(valueSize));
                    file.write(pair.second.c_str(), valueSize);
                }
                
                return true;
            } catch (...) {
                return false;
            }
        }

        bool CompressedData::LoadFromFile(const std::string& filePath) {
            try {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) return false;
                
                // Read format identifier
                uint32_t formatId;
                file.read(reinterpret_cast<char*>(&formatId), sizeof(formatId));
                format_ = static_cast<CompressionFormat>(formatId);
                
                // Read data size
                uint64_t dataSize;
                file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
                
                // Read data
                data_.resize(dataSize);
                file.read(reinterpret_cast<char*>(data_.data()), dataSize);
                
                // Read metadata
                uint32_t metadataCount;
                file.read(reinterpret_cast<char*>(&metadataCount), sizeof(metadataCount));
                
                metadata_.clear();
                for (uint32_t i = 0; i < metadataCount; i++) {
                    uint32_t keySize, valueSize;
                    
                    file.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
                    std::string key(keySize, '\0');
                    file.read(&key[0], keySize);
                    
                    file.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
                    std::string value(valueSize, '\0');
                    file.read(&value[0], valueSize);
                    
                    metadata_[key] = value;
                }
                
                return true;
            } catch (...) {
                return false;
            }
        }

        // CompressedData::StreamReader implementation
        CompressedData::StreamReader::StreamReader(const CompressedData& data, size_t chunkSize)
            : data_(data), position_(0), chunkSize_(chunkSize) {
        }

        bool CompressedData::StreamReader::ReadChunk(std::vector<uint8_t>& chunk) {
            if (IsAtEnd()) return false;
            
            size_t remainingBytes = data_.GetSize() - position_;
            size_t bytesToRead = std::min(chunkSize_, remainingBytes);
            
            chunk.resize(bytesToRead);
            std::copy(data_.GetData().begin() + position_,
                     data_.GetData().begin() + position_ + bytesToRead,
                     chunk.begin());
            
            position_ += bytesToRead;
            return true;
        }

        bool CompressedData::StreamReader::IsAtEnd() const {
            return position_ >= data_.GetSize();
        }

        void CompressedData::StreamReader::Reset() {
            position_ = 0;
        }

        CompressedData::StreamReader CompressedData::CreateStreamReader(size_t chunkSize) const {
            return StreamReader(*this, chunkSize);
        }

        // ICompressionCodec base implementation
        CompressionParams ICompressionCodec::OptimizeParams(
            const CompressionParams& params,
            const std::vector<uint8_t>& sampleData
        ) const {
            // Default implementation - just return the input params
            CompressionParams optimized = params;
            
            // Basic optimization based on data size
            if (!sampleData.empty()) {
                if (sampleData.size() < 1024) {
                    // Small data - prioritize speed
                    optimized.quality = CompressionQuality::Fastest;
                } else if (sampleData.size() > 1024 * 1024) {
                    // Large data - prioritize compression ratio
                    optimized.quality = CompressionQuality::Best;
                }
            }
            
            return optimized;
        }

        // CompressionStats implementation
        void CompressionStats::RecordCompression(CompressionFormat format, const CompressionResult& result) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            
            globalStats_.totalCompressions++;
            globalStats_.totalBytesCompressed += result.originalSize;
            globalStats_.totalCompressionTime += result.compressionTime.count();
            globalStats_.formatUsage[format]++;
            
            if (result.compressionRatio > 0) {
                globalStats_.averageCompressionRatio = 
                    (globalStats_.averageCompressionRatio * (globalStats_.totalCompressions - 1) + result.compressionRatio) 
                    / globalStats_.totalCompressions;
            }
            
            // Update format-specific stats
            auto& formatStat = formatStats_[format];
            formatStat.format = format;
            formatStat.compressions++;
            formatStat.totalInputBytes += result.originalSize;
            formatStat.totalOutputBytes += result.compressedSize;
            formatStat.totalTime += result.compressionTime.count();
            
            if (result.originalSize > 0) {
                formatStat.averageRatio = static_cast<double>(formatStat.totalInputBytes) / formatStat.totalOutputBytes;
            }
            
            if (result.compressionTime.count() > 0) {
                formatStat.averageSpeed = static_cast<double>(formatStat.totalInputBytes) / 
                                        (1024.0 * 1024.0 * formatStat.totalTime / 1000.0); // MB/s
            }
        }

        void CompressionStats::RecordDecompression(CompressionFormat format, const CompressionResult& result) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            
            globalStats_.totalDecompressions++;
            globalStats_.totalBytesDecompressed += result.originalSize;
            globalStats_.totalDecompressionTime += result.decompressionTime.count();
            
            if (result.decompressionTime.count() > 0 && result.originalSize > 0) {
                globalStats_.averageDecompressionSpeed = 
                    static_cast<double>(globalStats_.totalBytesDecompressed) / 
                    (1024.0 * 1024.0 * globalStats_.totalDecompressionTime / 1000.0); // MB/s
            }
            
            auto& formatStat = formatStats_[format];
            formatStat.decompressions++;
        }

        CompressionStats::GlobalStats CompressionStats::GetGlobalStats() const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            return globalStats_;
        }

        CompressionStats::FormatStats CompressionStats::GetFormatStats(CompressionFormat format) const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            auto it = formatStats_.find(format);
            return (it != formatStats_.end()) ? it->second : FormatStats{};
        }

        std::vector<CompressionStats::FormatStats> CompressionStats::GetAllFormatStats() const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            std::vector<FormatStats> stats;
            for (const auto& pair : formatStats_) {
                stats.push_back(pair.second);
            }
            return stats;
        }

        void CompressionStats::Reset() {
            std::lock_guard<std::mutex> lock(statsMutex_);
            globalStats_ = GlobalStats{};
            formatStats_.clear();
        }

        void CompressionStats::ResetFormat(CompressionFormat format) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            formatStats_.erase(format);
        }

        // AssetCompressionSystem implementation
        AssetCompressionSystem& AssetCompressionSystem::Instance() {
            static AssetCompressionSystem instance;
            return instance;
        }

        bool AssetCompressionSystem::Initialize() {
            if (initialized_.load()) {
                std::cout << "[Compression] System already initialized" << std::endl;
                return true;
            }

            std::cout << "[Compression] Initializing Asset Compression System..." << std::endl;
            
            try {
                // Register built-in codecs
                InitializeBuiltinCodecs();
                
                // Initialize thread pool
                shutdownThreads_.store(false);
                
                initialized_.store(true);
                std::cout << "[Compression] Asset Compression System initialized successfully" << std::endl;
                std::cout << "[Compression] Available formats: " << GetAvailableFormats().size() << std::endl;
                
                return true;
            } catch (const std::exception& e) {
                std::cout << "[Compression] Initialization failed: " << e.what() << std::endl;
                return false;
            }
        }

        void AssetCompressionSystem::Shutdown() {
            if (!initialized_.load()) {
                return;
            }

            std::cout << "[Compression] Shutting down Asset Compression System..." << std::endl;
            
            // Shutdown thread pool
            shutdownThreads_.store(true);
            for (auto& thread : threadPool_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            threadPool_.clear();
            
            // Shutdown codecs
            ShutdownCodecs();
            
            initialized_.store(false);
            std::cout << "[Compression] Asset Compression System shut down" << std::endl;
        }

        void AssetCompressionSystem::RegisterCodec(std::unique_ptr<ICompressionCodec> codec) {
            if (!codec) return;
            
            std::lock_guard<std::mutex> lock(codecMutex_);
            CompressionFormat format = codec->GetFormat();
            codecs_[format] = std::move(codec);
            
            std::cout << "[Compression] Registered codec: " << codecs_[format]->GetName() << std::endl;
        }

        void AssetCompressionSystem::UnregisterCodec(CompressionFormat format) {
            std::lock_guard<std::mutex> lock(codecMutex_);
            auto it = codecs_.find(format);
            if (it != codecs_.end()) {
                std::cout << "[Compression] Unregistered codec: " << it->second->GetName() << std::endl;
                codecs_.erase(it);
            }
        }

        ICompressionCodec* AssetCompressionSystem::GetCodec(CompressionFormat format) const {
            std::lock_guard<std::mutex> lock(codecMutex_);
            auto it = codecs_.find(format);
            return (it != codecs_.end()) ? it->second.get() : nullptr;
        }

        std::vector<CompressionFormat> AssetCompressionSystem::GetAvailableFormats() const {
            std::lock_guard<std::mutex> lock(codecMutex_);
            std::vector<CompressionFormat> formats;
            for (const auto& pair : codecs_) {
                formats.push_back(pair.first);
            }
            return formats;
        }

        void AssetCompressionSystem::InitializeBuiltinCodecs() {
            // Register built-in codecs
            RegisterCodec(std::make_unique<builtin_codecs::LZ4Codec>());
            RegisterCodec(std::make_unique<builtin_codecs::ZLIBCodec>());
            RegisterCodec(std::make_unique<builtin_codecs::DXT1Codec>());
            
            std::cout << "[Compression] Built-in codecs registered" << std::endl;
        }

        void AssetCompressionSystem::ShutdownCodecs() {
            std::lock_guard<std::mutex> lock(codecMutex_);
            codecs_.clear();
        }

        CompressionFormat AssetCompressionSystem::SelectOptimalFormat(
            AssetType assetType,
            TargetPlatform platform,
            const std::vector<uint8_t>& sampleData
        ) const {
            if (platform == TargetPlatform::Auto_Detect) {
                platform = DetectPlatform();
            }
            
            // Format selection logic based on asset type and platform
            switch (assetType) {
                case AssetType::Texture_Diffuse:
                case AssetType::Texture_Specular:
                    if (platform == TargetPlatform::PC_Desktop || platform == TargetPlatform::Console_Xbox) {
                        return CompressionFormat::DXT1;
                    } else if (platform == TargetPlatform::Mobile_Android) {
                        return CompressionFormat::ETC2_RGB;
                    } else if (platform == TargetPlatform::Mobile_iOS) {
                        return CompressionFormat::PVRTC_4BPP;
                    }
                    break;
                    
                case AssetType::Texture_Normal:
                    return CompressionFormat::BC5; // Best for normal maps
                    
                case AssetType::Audio_Music:
                    return CompressionFormat::OGG_VORBIS;
                    
                case AssetType::Audio_SFX:
                    return CompressionFormat::OGG_VORBIS;
                    
                case AssetType::Config_JSON:
                case AssetType::Shader_Source:
                    return CompressionFormat::ZLIB; // Good compression for text
                    
                case AssetType::Animation_Data:
                case AssetType::Config_Binary:
                    return CompressionFormat::LZ4; // Fast for frequently accessed data
                    
                default:
                    break;
            }
            
            // Default fallback based on data size
            if (!sampleData.empty()) {
                if (sampleData.size() < 1024) {
                    return CompressionFormat::LZ4; // Fast for small data
                } else {
                    return CompressionFormat::ZLIB; // Better ratio for larger data
                }
            }
            
            return CompressionFormat::LZ4; // Safe default
        }

        CompressionParams AssetCompressionSystem::OptimizeParameters(
            CompressionFormat format,
            AssetType assetType,
            TargetPlatform platform,
            const std::vector<uint8_t>& sampleData
        ) const {
            CompressionParams params(format);
            params.assetType = assetType;
            params.platform = platform;
            
            // Get codec and let it optimize parameters
            if (auto* codec = GetCodec(format)) {
                params = codec->OptimizeParams(params, sampleData);
            }
            
            // Apply platform-specific optimizations
            switch (platform) {
                case TargetPlatform::Mobile_Android:
                case TargetPlatform::Mobile_iOS:
                    // Mobile - prioritize speed and memory efficiency
                    params.quality = CompressionQuality::Fastest;
                    params.maxMemoryUsage = 16 * 1024 * 1024; // 16MB limit
                    params.chunkSize = 32 * 1024; // Smaller chunks
                    break;
                    
                case TargetPlatform::PC_Desktop:
                    // Desktop - can afford better quality
                    params.quality = CompressionQuality::Best;
                    params.enableMultithreading = true;
                    break;
                    
                case TargetPlatform::Web_Browser:
                    // Web - balance size and decode speed
                    params.quality = CompressionQuality::Balanced;
                    params.enableHardwareAccel = false; // Not always available
                    break;
                    
                default:
                    params.quality = CompressionQuality::Balanced;
                    break;
            }
            
            return params;
        }

        CompressionResult AssetCompressionSystem::Compress(
            const std::vector<uint8_t>& input,
            CompressedData& output,
            const CompressionParams& params
        ) {
            if (!initialized_.load()) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "Compression system not initialized";
                return result;
            }
            
            auto* codec = GetCodec(params.format);
            if (!codec) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "Codec not found for format: " + compression_utils::FormatToString(params.format);
                return result;
            }
            
            if (!codec->ValidateParams(params)) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "Invalid compression parameters";
                return result;
            }
            
            std::vector<uint8_t> compressedData;
            CompressionResult result = codec->Compress(input, compressedData, params);
            
            if (result.success) {
                output = CompressedData(std::move(compressedData), params.format, params);
                output.GetData(); // Access to set result
                
                // Record statistics
                stats_.RecordCompression(params.format, result);
                
                std::cout << "[Compression] Compressed " << result.originalSize 
                         << " bytes to " << result.compressedSize 
                         << " bytes (ratio: " << result.compressionRatio << ")" << std::endl;
            }
            
            return result;
        }

        CompressionResult AssetCompressionSystem::Decompress(
            const CompressedData& input,
            std::vector<uint8_t>& output,
            const CompressionParams& params
        ) {
            if (!initialized_.load()) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "Compression system not initialized";
                return result;
            }
            
            CompressionFormat format = input.GetFormat();
            auto* codec = GetCodec(format);
            if (!codec) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "Codec not found for format: " + compression_utils::FormatToString(format);
                return result;
            }
            
            CompressionResult result = codec->Decompress(input.GetData(), output, params);
            
            if (result.success) {
                // Record statistics
                stats_.RecordDecompression(format, result);
                
                std::cout << "[Compression] Decompressed " << result.compressedSize 
                         << " bytes to " << result.originalSize << " bytes" << std::endl;
            }
            
            return result;
        }

        std::future<CompressionResult> AssetCompressionSystem::CompressAsync(
            std::vector<uint8_t> input,
            CompressionParams params
        ) {
            return std::async(std::launch::async, [this, input = std::move(input), params]() mutable {
                CompressedData output;
                return Compress(input, output, params);
            });
        }

        std::future<CompressionResult> AssetCompressionSystem::DecompressAsync(
            CompressedData input,
            CompressionParams params
        ) {
            return std::async(std::launch::async, [this, input = std::move(input), params]() mutable {
                std::vector<uint8_t> output;
                return Decompress(input, output, params);
            });
        }

        CompressionResult AssetCompressionSystem::CompressFile(
            const std::string& inputPath,
            const std::string& outputPath,
            const CompressionParams& params
        ) {
            try {
                // Read input file
                std::ifstream file(inputPath, std::ios::binary);
                if (!file) {
                    CompressionResult result;
                    result.success = false;
                    result.errorMessage = "Failed to open input file: " + inputPath;
                    return result;
                }
                
                std::vector<uint8_t> inputData((std::istreambuf_iterator<char>(file)),
                                              std::istreambuf_iterator<char>());
                
                // Compress data
                CompressedData compressedData;
                CompressionResult result = Compress(inputData, compressedData, params);
                
                if (result.success) {
                    // Save compressed data
                    if (!compressedData.SaveToFile(outputPath)) {
                        result.success = false;
                        result.errorMessage = "Failed to save compressed file: " + outputPath;
                    }
                }
                
                return result;
            } catch (const std::exception& e) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "File compression failed: " + std::string(e.what());
                return result;
            }
        }

        CompressionResult AssetCompressionSystem::DecompressFile(
            const std::string& inputPath,
            const std::string& outputPath,
            const CompressionParams& params
        ) {
            try {
                // Load compressed data
                CompressedData compressedData;
                if (!compressedData.LoadFromFile(inputPath)) {
                    CompressionResult result;
                    result.success = false;
                    result.errorMessage = "Failed to load compressed file: " + inputPath;
                    return result;
                }
                
                // Decompress data
                std::vector<uint8_t> outputData;
                CompressionResult result = Decompress(compressedData, outputData, params);
                
                if (result.success) {
                    // Save decompressed data
                    std::ofstream file(outputPath, std::ios::binary);
                    if (file) {
                        file.write(reinterpret_cast<const char*>(outputData.data()), outputData.size());
                    } else {
                        result.success = false;
                        result.errorMessage = "Failed to save decompressed file: " + outputPath;
                    }
                }
                
                return result;
            } catch (const std::exception& e) {
                CompressionResult result;
                result.success = false;
                result.errorMessage = "File decompression failed: " + std::string(e.what());
                return result;
            }
        }

        TargetPlatform AssetCompressionSystem::DetectPlatform() const {
            // Platform detection logic
            #ifdef _WIN32
                return TargetPlatform::PC_Desktop;
            #elif defined(__ANDROID__)
                return TargetPlatform::Mobile_Android;
            #elif defined(__APPLE__)
                #ifdef TARGET_OS_IPHONE
                    return TargetPlatform::Mobile_iOS;
                #else
                    return TargetPlatform::PC_Desktop; // macOS
                #endif
            #elif defined(__linux__)
                return TargetPlatform::PC_Desktop;
            #elif defined(__EMSCRIPTEN__)
                return TargetPlatform::Web_Browser;
            #else
                return TargetPlatform::PC_Desktop; // Default
            #endif
        }

        AssetType AssetCompressionSystem::DetectAssetType(const std::vector<uint8_t>& data) const {
            if (data.empty()) return AssetType::Unknown;
            
            // Simple file format detection based on magic numbers
            if (data.size() >= 4) {
                // PNG signature
                if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
                    return AssetType::Texture_Diffuse;
                }
                
                // JPEG signature
                if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
                    return AssetType::Texture_Diffuse;
                }
                
                // OGG signature
                if (data[0] == 0x4F && data[1] == 0x67 && data[2] == 0x67 && data[3] == 0x53) {
                    return AssetType::Audio_Music;
                }
            }
            
            // Check for JSON
            if (data[0] == '{' || data[0] == '[') {
                return AssetType::Config_JSON;
            }
            
            return AssetType::Unknown;
        }

        // Utility functions implementation
        namespace compression_utils {
            std::string FormatToString(CompressionFormat format) {
                switch (format) {
                    case CompressionFormat::None: return "None";
                    case CompressionFormat::LZ4: return "LZ4";
                    case CompressionFormat::ZLIB: return "ZLIB";
                    case CompressionFormat::ZSTD: return "ZSTD";
                    case CompressionFormat::BROTLI: return "BROTLI";
                    case CompressionFormat::DXT1: return "DXT1";
                    case CompressionFormat::DXT3: return "DXT3";
                    case CompressionFormat::DXT5: return "DXT5";
                    case CompressionFormat::BC4: return "BC4";
                    case CompressionFormat::BC5: return "BC5";
                    case CompressionFormat::BC6H: return "BC6H";
                    case CompressionFormat::BC7: return "BC7";
                    case CompressionFormat::ASTC_4x4: return "ASTC_4x4";
                    case CompressionFormat::ASTC_6x6: return "ASTC_6x6";
                    case CompressionFormat::ASTC_8x8: return "ASTC_8x8";
                    case CompressionFormat::ETC2_RGB: return "ETC2_RGB";
                    case CompressionFormat::ETC2_RGBA: return "ETC2_RGBA";
                    case CompressionFormat::PVRTC_2BPP: return "PVRTC_2BPP";
                    case CompressionFormat::PVRTC_4BPP: return "PVRTC_4BPP";
                    case CompressionFormat::OGG_VORBIS: return "OGG_VORBIS";
                    case CompressionFormat::MP3: return "MP3";
                    case CompressionFormat::FLAC: return "FLAC";
                    case CompressionFormat::OPUS: return "OPUS";
                    case CompressionFormat::DRACO: return "DRACO";
                    case CompressionFormat::MESHOPT: return "MESHOPT";
                    default: return "Unknown";
                }
            }

            CompressionFormat StringToFormat(const std::string& formatStr) {
                if (formatStr == "LZ4") return CompressionFormat::LZ4;
                if (formatStr == "ZLIB") return CompressionFormat::ZLIB;
                if (formatStr == "ZSTD") return CompressionFormat::ZSTD;
                if (formatStr == "DXT1") return CompressionFormat::DXT1;
                if (formatStr == "DXT3") return CompressionFormat::DXT3;
                if (formatStr == "DXT5") return CompressionFormat::DXT5;
                if (formatStr == "OGG_VORBIS") return CompressionFormat::OGG_VORBIS;
                return CompressionFormat::None;
            }

            bool IsLossyFormat(CompressionFormat format) {
                switch (format) {
                    case CompressionFormat::DXT1:
                    case CompressionFormat::DXT3:
                    case CompressionFormat::DXT5:
                    case CompressionFormat::BC4:
                    case CompressionFormat::BC5:
                    case CompressionFormat::BC6H:
                    case CompressionFormat::BC7:
                    case CompressionFormat::ASTC_4x4:
                    case CompressionFormat::ASTC_6x6:
                    case CompressionFormat::ASTC_8x8:
                    case CompressionFormat::ETC2_RGB:
                    case CompressionFormat::ETC2_RGBA:
                    case CompressionFormat::PVRTC_2BPP:
                    case CompressionFormat::PVRTC_4BPP:
                    case CompressionFormat::OGG_VORBIS:
                    case CompressionFormat::MP3:
                    case CompressionFormat::OPUS:
                        return true;
                    default:
                        return false;
                }
            }

            bool IsTextureFormat(CompressionFormat format) {
                switch (format) {
                    case CompressionFormat::DXT1:
                    case CompressionFormat::DXT3:
                    case CompressionFormat::DXT5:
                    case CompressionFormat::BC4:
                    case CompressionFormat::BC5:
                    case CompressionFormat::BC6H:
                    case CompressionFormat::BC7:
                    case CompressionFormat::ASTC_4x4:
                    case CompressionFormat::ASTC_6x6:
                    case CompressionFormat::ASTC_8x8:
                    case CompressionFormat::ETC2_RGB:
                    case CompressionFormat::ETC2_RGBA:
                    case CompressionFormat::PVRTC_2BPP:
                    case CompressionFormat::PVRTC_4BPP:
                        return true;
                    default:
                        return false;
                }
            }

            bool IsAudioFormat(CompressionFormat format) {
                switch (format) {
                    case CompressionFormat::OGG_VORBIS:
                    case CompressionFormat::MP3:
                    case CompressionFormat::FLAC:
                    case CompressionFormat::OPUS:
                        return true;
                    default:
                        return false;
                }
            }

            float CalculateCompressionRatio(size_t originalSize, size_t compressedSize) {
                if (compressedSize == 0) return 0.0f;
                return static_cast<float>(originalSize) / static_cast<float>(compressedSize);
            }

            TargetPlatform GetCurrentPlatform() {
                #ifdef _WIN32
                    return TargetPlatform::PC_Desktop;
                #elif defined(__ANDROID__)
                    return TargetPlatform::Mobile_Android;
                #elif defined(__APPLE__)
                    #include "TargetConditionals.h"
                    #if TARGET_OS_IPHONE
                        return TargetPlatform::Mobile_iOS;
                    #else
                        return TargetPlatform::PC_Desktop;
                    #endif
                #elif defined(__linux__)
                    return TargetPlatform::PC_Desktop;
                #elif defined(__EMSCRIPTEN__)
                    return TargetPlatform::Web_Browser;
                #else
                    return TargetPlatform::PC_Desktop;
                #endif
            }
        }
        
    } // namespace compression
} // namespace assets