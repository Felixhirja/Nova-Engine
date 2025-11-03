#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <future>
#include <chrono>
#include <cstdint>

namespace assets {
    namespace compression {

        // Forward declarations
        class ICompressionCodec;
        class CompressionContext;
        class CompressionStats;

        // Compression formats supported
        enum class CompressionFormat {
            None = 0,
            
            // General purpose compression
            LZ4,         // Fast compression/decompression
            ZLIB,        // Good compression ratio
            ZSTD,        // Modern, excellent ratio and speed
            BROTLI,      // Web-optimized compression
            
            // Texture compression
            DXT1,        // BC1 - RGB, 1-bit alpha
            DXT3,        // BC2 - RGB + explicit alpha
            DXT5,        // BC3 - RGB + interpolated alpha
            BC4,         // Single channel
            BC5,         // Two channel (normal maps)
            BC6H,        // HDR compression
            BC7,         // High quality RGB/RGBA
            ASTC_4x4,    // ARM Adaptive Scalable Texture Compression
            ASTC_6x6,
            ASTC_8x8,
            ETC2_RGB,    // Ericsson Texture Compression 2
            ETC2_RGBA,
            PVRTC_2BPP,  // PowerVR Texture Compression
            PVRTC_4BPP,
            
            // Audio compression
            OGG_VORBIS,  // Ogg Vorbis audio
            MP3,         // MPEG Audio Layer 3
            FLAC,        // Free Lossless Audio Codec
            OPUS,        // Modern audio codec
            
            // Mesh compression
            DRACO,       // Google Draco mesh compression
            MESHOPT,     // Mesh optimization
            
            // Custom formats
            CUSTOM_1,
            CUSTOM_2,
            CUSTOM_3
        };

        // Compression quality levels
        enum class CompressionQuality {
            Fastest = 0,    // Prioritize speed
            Balanced = 1,   // Balance speed/quality
            Best = 2,       // Prioritize quality/compression ratio
            Custom = 3      // User-defined parameters
        };

        // Platform-specific compression preferences
        enum class TargetPlatform {
            PC_Desktop,
            PC_Mobile,
            Console_PlayStation,
            Console_Xbox,
            Console_Nintendo,
            Mobile_Android,
            Mobile_iOS,
            Web_Browser,
            VR_Headset,
            Auto_Detect
        };

        // Asset types for compression selection
        enum class AssetType {
            Unknown,
            Texture_Diffuse,
            Texture_Normal,
            Texture_Specular,
            Texture_HDR,
            Texture_UI,
            Audio_Music,
            Audio_SFX,
            Audio_Voice,
            Mesh_Static,
            Mesh_Skinned,
            Animation_Data,
            Config_JSON,
            Config_Binary,
            Shader_Source,
            Font_Data,
            Custom
        };

        // Compression parameters
        struct CompressionParams {
            CompressionFormat format = CompressionFormat::None;
            CompressionQuality quality = CompressionQuality::Balanced;
            TargetPlatform platform = TargetPlatform::Auto_Detect;
            AssetType assetType = AssetType::Unknown;
            
            // Quality/performance trade-offs
            int compressionLevel = -1;      // Codec-specific level (-1 = auto)
            bool enableMultithreading = true;
            bool enableHardwareAccel = true;
            
            // Memory constraints
            size_t maxMemoryUsage = 0;      // 0 = unlimited
            size_t chunkSize = 64 * 1024;   // Streaming chunk size
            
            // Custom parameters
            std::unordered_map<std::string, float> customParams;
            
            CompressionParams() = default;
            CompressionParams(CompressionFormat fmt, CompressionQuality qual = CompressionQuality::Balanced)
                : format(fmt), quality(qual) {}
        };

        // Compression result information
        struct CompressionResult {
            bool success = false;
            std::string errorMessage;
            
            // Size information
            size_t originalSize = 0;
            size_t compressedSize = 0;
            float compressionRatio = 0.0f;
            
            // Performance metrics
            std::chrono::milliseconds compressionTime{0};
            std::chrono::milliseconds decompressionTime{0};
            
            // Quality metrics (when applicable)
            float qualityScore = 0.0f;      // 0-1, higher is better
            float psnr = 0.0f;              // Peak Signal-to-Noise Ratio
            float ssim = 0.0f;              // Structural Similarity Index
            
            // Metadata
            CompressionFormat format = CompressionFormat::None;
            std::unordered_map<std::string, std::string> metadata;
            
            CompressionResult() = default;
            CompressionResult(bool success) : success(success) {}
        };

        // Compressed data container
        class CompressedData {
        public:
            CompressedData() = default;
            CompressedData(std::vector<uint8_t> data, CompressionFormat format, CompressionParams params);
            
            // Data access
            const std::vector<uint8_t>& GetData() const { return data_; }
            std::vector<uint8_t>& GetData() { return data_; }
            size_t GetSize() const { return data_.size(); }
            bool IsEmpty() const { return data_.empty(); }
            
            // Compression info
            CompressionFormat GetFormat() const { return format_; }
            const CompressionParams& GetParams() const { return params_; }
            const CompressionResult& GetResult() const { return result_; }
            
            // Metadata
            void SetMetadata(const std::string& key, const std::string& value);
            std::string GetMetadata(const std::string& key) const;
            
            // Serialization
            bool SaveToFile(const std::string& filePath) const;
            bool LoadFromFile(const std::string& filePath);
            
            // Streaming support
            class StreamReader {
            public:
                StreamReader(const CompressedData& data, size_t chunkSize = 64 * 1024);
                bool ReadChunk(std::vector<uint8_t>& chunk);
                bool IsAtEnd() const;
                void Reset();
                
            private:
                const CompressedData& data_;
                size_t position_;
                size_t chunkSize_;
            };
            
            StreamReader CreateStreamReader(size_t chunkSize = 64 * 1024) const;
            
        private:
            std::vector<uint8_t> data_;
            CompressionFormat format_ = CompressionFormat::None;
            CompressionParams params_;
            CompressionResult result_;
            std::unordered_map<std::string, std::string> metadata_;
        };

        // Abstract compression codec interface
        class ICompressionCodec {
        public:
            virtual ~ICompressionCodec() = default;
            
            // Codec information
            virtual CompressionFormat GetFormat() const = 0;
            virtual std::string GetName() const = 0;
            virtual std::vector<AssetType> GetSupportedAssetTypes() const = 0;
            virtual std::vector<TargetPlatform> GetSupportedPlatforms() const = 0;
            
            // Capability queries
            virtual bool SupportsStreaming() const = 0;
            virtual bool SupportsHardwareAcceleration() const = 0;
            virtual bool SupportsMultithreading() const = 0;
            virtual bool SupportsQualityLevels() const = 0;
            
            // Compression operations
            virtual CompressionResult Compress(
                const std::vector<uint8_t>& input,
                std::vector<uint8_t>& output,
                const CompressionParams& params = {}
            ) = 0;
            
            virtual CompressionResult Decompress(
                const std::vector<uint8_t>& input,
                std::vector<uint8_t>& output,
                const CompressionParams& params = {}
            ) = 0;
            
            // Streaming operations (optional)
            virtual bool SupportsStreamingCompression() const { return false; }
            virtual bool SupportsStreamingDecompression() const { return false; }
            
            virtual std::unique_ptr<CompressionContext> CreateCompressionContext(
                const CompressionParams& params = {}
            ) { return nullptr; }
            
            virtual std::unique_ptr<CompressionContext> CreateDecompressionContext(
                const CompressionParams& params = {}
            ) { return nullptr; }
            
            // Parameter validation and optimization
            virtual CompressionParams OptimizeParams(
                const CompressionParams& params,
                const std::vector<uint8_t>& sampleData = {}
            ) const;
            
            virtual bool ValidateParams(const CompressionParams& params) const = 0;
            
            // Quality estimation (for lossy codecs)
            virtual float EstimateQuality(
                const std::vector<uint8_t>& original,
                const std::vector<uint8_t>& compressed,
                const CompressionParams& params = {}
            ) const { return 0.0f; }
        };

        // Streaming compression context
        class CompressionContext {
        public:
            virtual ~CompressionContext() = default;
            
            // Streaming operations
            virtual bool ProcessChunk(
                const std::vector<uint8_t>& inputChunk,
                std::vector<uint8_t>& outputChunk,
                bool isLastChunk = false
            ) = 0;
            
            virtual bool Finish(std::vector<uint8_t>& finalOutput) = 0;
            virtual void Reset() = 0;
            
            // State queries
            virtual size_t GetTotalInputSize() const = 0;
            virtual size_t GetTotalOutputSize() const = 0;
            virtual float GetProgress() const = 0;
            virtual bool IsFinished() const = 0;
        };

        // Compression statistics and monitoring
        class CompressionStats {
        public:
            // Global statistics
            struct GlobalStats {
                uint64_t totalCompressions = 0;
                uint64_t totalDecompressions = 0;
                uint64_t totalBytesCompressed = 0;
                uint64_t totalBytesDecompressed = 0;
                uint64_t totalCompressionTime = 0;      // milliseconds
                uint64_t totalDecompressionTime = 0;    // milliseconds
                
                double averageCompressionRatio = 0.0;
                double averageCompressionSpeed = 0.0;   // MB/s
                double averageDecompressionSpeed = 0.0; // MB/s
                
                std::unordered_map<CompressionFormat, uint64_t> formatUsage;
                std::unordered_map<AssetType, uint64_t> assetTypeUsage;
            };
            
            // Per-format statistics
            struct FormatStats {
                CompressionFormat format;
                uint64_t compressions = 0;
                uint64_t decompressions = 0;
                uint64_t totalInputBytes = 0;
                uint64_t totalOutputBytes = 0;
                uint64_t totalTime = 0;
                
                double averageRatio = 0.0;
                double averageSpeed = 0.0;
                double averageQuality = 0.0;
                
                size_t minSize = SIZE_MAX;
                size_t maxSize = 0;
                float bestRatio = 0.0f;
                float worstRatio = 0.0f;
            };
            
            void RecordCompression(CompressionFormat format, const CompressionResult& result);
            void RecordDecompression(CompressionFormat format, const CompressionResult& result);
            
            GlobalStats GetGlobalStats() const;
            FormatStats GetFormatStats(CompressionFormat format) const;
            std::vector<FormatStats> GetAllFormatStats() const;
            
            void Reset();
            void ResetFormat(CompressionFormat format);
            
        private:
            mutable std::mutex statsMutex_;
            GlobalStats globalStats_;
            std::unordered_map<CompressionFormat, FormatStats> formatStats_;
        };

        // Main Asset Compression System
        class AssetCompressionSystem {
        public:
            // Singleton access
            static AssetCompressionSystem& Instance();
            
            // System lifecycle
            bool Initialize();
            void Shutdown();
            bool IsInitialized() const { return initialized_.load(); }
            
            // Codec management
            void RegisterCodec(std::unique_ptr<ICompressionCodec> codec);
            void UnregisterCodec(CompressionFormat format);
            ICompressionCodec* GetCodec(CompressionFormat format) const;
            std::vector<CompressionFormat> GetAvailableFormats() const;
            std::vector<CompressionFormat> GetSupportedFormats(AssetType assetType, TargetPlatform platform = TargetPlatform::Auto_Detect) const;
            
            // Format selection and optimization
            CompressionFormat SelectOptimalFormat(
                AssetType assetType,
                TargetPlatform platform = TargetPlatform::Auto_Detect,
                const std::vector<uint8_t>& sampleData = {}
            ) const;
            
            CompressionParams OptimizeParameters(
                CompressionFormat format,
                AssetType assetType,
                TargetPlatform platform = TargetPlatform::Auto_Detect,
                const std::vector<uint8_t>& sampleData = {}
            ) const;
            
            // Synchronous compression/decompression
            CompressionResult Compress(
                const std::vector<uint8_t>& input,
                CompressedData& output,
                const CompressionParams& params
            );
            
            CompressionResult Decompress(
                const CompressedData& input,
                std::vector<uint8_t>& output,
                const CompressionParams& params = {}
            );
            
            // Asynchronous operations
            std::future<CompressionResult> CompressAsync(
                std::vector<uint8_t> input,
                CompressionParams params
            );
            
            std::future<CompressionResult> DecompressAsync(
                CompressedData input,
                CompressionParams params = {}
            );
            
            // Batch operations
            struct BatchItem {
                std::string id;
                std::vector<uint8_t> data;
                CompressionParams params;
                CompressionResult result;
            };
            
            void CompressBatch(std::vector<BatchItem>& items, bool parallel = true);
            void DecompressBatch(std::vector<std::pair<CompressedData, std::vector<uint8_t>&>>& items, bool parallel = true);
            
            // File operations
            CompressionResult CompressFile(
                const std::string& inputPath,
                const std::string& outputPath,
                const CompressionParams& params
            );
            
            CompressionResult DecompressFile(
                const std::string& inputPath,
                const std::string& outputPath,
                const CompressionParams& params = {}
            );
            
            // Streaming operations
            std::unique_ptr<CompressionContext> CreateCompressionStream(const CompressionParams& params);
            std::unique_ptr<CompressionContext> CreateDecompressionStream(const CompressionParams& params);
            
            // Quality analysis
            CompressionResult AnalyzeQuality(
                const std::vector<uint8_t>& original,
                const CompressedData& compressed
            );
            
            std::vector<CompressionResult> CompareFormats(
                const std::vector<uint8_t>& input,
                const std::vector<CompressionFormat>& formats,
                AssetType assetType = AssetType::Unknown
            );
            
            // Statistics and monitoring
            const CompressionStats& GetStats() const { return stats_; }
            CompressionStats& GetStats() { return stats_; }
            
            // Configuration
            void SetMaxThreads(size_t maxThreads);
            size_t GetMaxThreads() const { return maxThreads_; }
            
            void SetMemoryLimit(size_t memoryLimit);
            size_t GetMemoryLimit() const { return memoryLimit_; }
            
            void EnableHardwareAcceleration(bool enable) { hardwareAccelEnabled_ = enable; }
            bool IsHardwareAccelerationEnabled() const { return hardwareAccelEnabled_; }
            
        private:
            AssetCompressionSystem() = default;
            ~AssetCompressionSystem() = default;
            AssetCompressionSystem(const AssetCompressionSystem&) = delete;
            AssetCompressionSystem& operator=(const AssetCompressionSystem&) = delete;
            
            // Internal helper methods
            CompressionResult CompressInternal(
                const std::vector<uint8_t>& input,
                std::vector<uint8_t>& output,
                const CompressionParams& params
            );
            
            CompressionResult DecompressInternal(
                const std::vector<uint8_t>& input,
                std::vector<uint8_t>& output,
                const CompressionParams& params
            );
            
            void InitializeBuiltinCodecs();
            void ShutdownCodecs();
            
            TargetPlatform DetectPlatform() const;
            AssetType DetectAssetType(const std::vector<uint8_t>& data) const;
            
            // State
            std::atomic<bool> initialized_{false};
            mutable std::mutex codecMutex_;
            std::unordered_map<CompressionFormat, std::unique_ptr<ICompressionCodec>> codecs_;
            
            // Configuration
            size_t maxThreads_ = std::thread::hardware_concurrency();
            size_t memoryLimit_ = 0; // 0 = unlimited
            bool hardwareAccelEnabled_ = true;
            
            // Statistics
            CompressionStats stats_;
            
            // Thread pool for async operations
            std::vector<std::thread> threadPool_;
            std::atomic<bool> shutdownThreads_{false};
        };

        // Utility functions
        namespace compression_utils {
            // Format information
            std::string FormatToString(CompressionFormat format);
            CompressionFormat StringToFormat(const std::string& formatStr);
            bool IsLossyFormat(CompressionFormat format);
            bool IsTextureFormat(CompressionFormat format);
            bool IsAudioFormat(CompressionFormat format);
            bool IsMeshFormat(CompressionFormat format);
            
            // Platform detection
            TargetPlatform GetCurrentPlatform();
            std::vector<CompressionFormat> GetPreferredFormats(TargetPlatform platform, AssetType assetType);
            
            // Quality metrics
            float CalculatePSNR(const std::vector<uint8_t>& original, const std::vector<uint8_t>& compressed);
            float CalculateSSIM(const std::vector<uint8_t>& original, const std::vector<uint8_t>& compressed);
            float CalculateCompressionRatio(size_t originalSize, size_t compressedSize);
            
            // Data validation
            bool ValidateCompressedData(const CompressedData& data);
            bool ValidateCompressionParams(const CompressionParams& params);
            
            // Memory management
            size_t EstimateMemoryUsage(CompressionFormat format, size_t inputSize, const CompressionParams& params);
            size_t GetOptimalChunkSize(CompressionFormat format, size_t totalSize);
            
            // File format detection
            AssetType DetectAssetTypeFromPath(const std::string& filePath);
            AssetType DetectAssetTypeFromData(const std::vector<uint8_t>& data);
            
            // Compression suggestions
            struct CompressionSuggestion {
                CompressionFormat format;
                CompressionParams params;
                float estimatedRatio;
                float estimatedQuality;
                std::string reasoning;
            };
            
            std::vector<CompressionSuggestion> SuggestCompressionOptions(
                AssetType assetType,
                size_t dataSize,
                TargetPlatform platform = TargetPlatform::Auto_Detect,
                CompressionQuality targetQuality = CompressionQuality::Balanced
            );
        }

    } // namespace compression
} // namespace assets