#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <queue>
#include <condition_variable>
#include <iostream>

namespace assets {

// Asset types supported by the pipeline
enum class AssetType {
    Texture,
    Model,
    Material,
    Shader,
    Audio,
    Font,
    Config,
    Sprite,
    Unknown
};

// Asset formats
enum class AssetFormat {
    // Textures
    PNG, JPG, TGA, BMP, DDS, KTX, ASTC,
    // Models
    OBJ, FBX, GLTF, DAE,
    // Materials
    JSON, XML,
    // Shaders
    GLSL, HLSL, SPIRV,
    // Audio
    WAV, MP3, OGG,
    // Fonts
    TTF, OTF,
    // Vector Graphics
    SVG,
    // Config
    JSON_CONFIG, YAML, INI,
    Unknown
};

// Asset processing status
enum class ProcessingStatus {
    Pending,
    Processing,
    Completed,
    Failed,
    Cached
};

// Platform targets for optimization
enum class PlatformTarget {
    Desktop,
    Mobile,
    Console,
    Web,
    Universal
};

// Quality levels for optimization
enum class QualityLevel {
    Low,
    Medium,
    High,
    Ultra
};

// Asset metadata
struct AssetMetadata {
    std::string id;
    std::string name;
    std::string originalPath;
    std::string processedPath;
    AssetType type;
    AssetFormat format;
    ProcessingStatus status;
    PlatformTarget platform;
    QualityLevel quality;
    size_t originalSize;
    size_t processedSize;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point lastModified;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> properties;
    std::string checksum;
    
    AssetMetadata() : type(AssetType::Unknown), format(AssetFormat::Unknown), 
                      status(ProcessingStatus::Pending), platform(PlatformTarget::Universal),
                      quality(QualityLevel::Medium), originalSize(0), processedSize(0) {}
};

// Asset processor interface
class IAssetProcessor {
public:
    virtual ~IAssetProcessor() = default;
    virtual bool CanProcess(AssetFormat format) const = 0;
    virtual bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) = 0;
    virtual std::vector<AssetFormat> GetSupportedFormats() const = 0;
    virtual std::string GetProcessorName() const = 0;
};

// Asset validation result
struct ValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, std::string> properties;
    
    ValidationResult() : isValid(true) {}
};

// Asset validator interface
class IAssetValidator {
public:
    virtual ~IAssetValidator() = default;
    virtual bool CanValidate(AssetType type) const = 0;
    virtual ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) = 0;
    virtual std::string GetValidatorName() const = 0;
};

// Asset processing configuration
struct ProcessingConfig {
    PlatformTarget targetPlatform = PlatformTarget::Desktop;
    QualityLevel targetQuality = QualityLevel::Medium;
    bool enableCompression = true;
    bool enableOptimization = true;
    bool enableValidation = true;
    bool enableCaching = true;
    bool preserveOriginals = true;
    size_t maxThreads = 4;
    std::string outputDirectory = "assets/processed";
    std::string cacheDirectory = "assets/cache";
    std::unordered_map<std::string, std::string> customProperties;
};

// Advanced Asset Processing Pipeline
class AssetProcessingPipeline {
public:
    static AssetProcessingPipeline& Instance() {
        static AssetProcessingPipeline instance;
        return instance;
    }

    // Initialize the pipeline
    void Initialize(const ProcessingConfig& config = ProcessingConfig{});
    
    // Shutdown the pipeline
    void Shutdown();

    // Register processors and validators
    void RegisterProcessor(std::unique_ptr<IAssetProcessor> processor);
    void RegisterValidator(std::unique_ptr<IAssetValidator> validator);

    // Process single asset
    bool ProcessAsset(const std::string& inputPath, AssetMetadata& metadata);
    
    // Process batch of assets
    void ProcessAssetBatch(const std::vector<std::string>& inputPaths);
    
    // Get processing status
    ProcessingStatus GetAssetStatus(const std::string& assetId) const;
    
    // Asset discovery and scanning
    std::vector<std::string> ScanDirectory(const std::string& directory, bool recursive = true);
    
    // Asset format detection
    AssetFormat DetectFormat(const std::string& filePath) const;
    AssetType DetermineType(AssetFormat format) const;
    
    // Asset validation
    ValidationResult ValidateAsset(const std::string& filePath);
    
    // Optimization and conversion
    bool OptimizeAsset(AssetMetadata& metadata, const ProcessingConfig& config);
    bool ConvertAsset(AssetMetadata& metadata, AssetFormat targetFormat);
    
    // Compression
    bool CompressAsset(AssetMetadata& metadata);
    
    // Caching
    bool IsCached(const AssetMetadata& metadata) const;
    bool CacheAsset(const AssetMetadata& metadata);
    bool LoadFromCache(AssetMetadata& metadata);
    void ClearCache();
    
    // Asset database
    void SaveAssetDatabase(const std::string& databasePath = "assets/asset_database.json");
    bool LoadAssetDatabase(const std::string& databasePath = "assets/asset_database.json");
    
    // Analytics and reporting
    void GenerateProcessingReport();
    std::string GetAssetAnalytics() const;
    
    // Configuration
    void SetConfig(const ProcessingConfig& config) { config_ = config; }
    const ProcessingConfig& GetConfig() const { return config_; }
    
    // Asset management
    std::vector<AssetMetadata> GetAllAssets() const;
    AssetMetadata* FindAsset(const std::string& assetId);
    bool RemoveAsset(const std::string& assetId);
    
    // Dependency management
    void UpdateDependencies(AssetMetadata& metadata);
    std::vector<std::string> GetDependents(const std::string& assetId) const;

private:
    AssetProcessingPipeline() = default;
    
    // Internal processing methods
    std::string GenerateAssetId(const std::string& filePath) const;
    std::string CalculateChecksum(const std::string& filePath) const;
    bool CreateDirectories(const std::string& path) const;
    
    // Processing worker thread
    void ProcessingWorker();
    
    // Format detection helpers
    AssetFormat DetectFormatByExtension(const std::string& extension) const;
    AssetFormat DetectFormatByContent(const std::string& filePath) const;
    
    // Utility methods
    std::string FormatToString(AssetFormat format) const;
    std::string TypeToString(AssetType type) const;
    std::string StatusToString(ProcessingStatus status) const;

    // Member variables
    ProcessingConfig config_;
    
    std::vector<std::unique_ptr<IAssetProcessor>> processors_;
    std::vector<std::unique_ptr<IAssetValidator>> validators_;
    
    std::unordered_map<std::string, AssetMetadata> assetDatabase_;
    mutable std::mutex databaseMutex_;
    
    std::vector<std::thread> workerThreads_;
    std::queue<std::string> processingQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shouldStop_{false};
    
    // Processing statistics
    std::atomic<size_t> totalProcessed_{0};
    std::atomic<size_t> totalFailed_{0};
    std::atomic<size_t> totalCached_{0};
    std::chrono::high_resolution_clock::time_point startTime_;
};

// Utility functions
namespace pipeline_utils {
    // File system utilities
    bool FileExists(const std::string& path);
    size_t GetFileSize(const std::string& path);
    std::string     GetFileExtension(const std::string& path);
    std::string GetFileName(const std::string& path);
    std::string GetDirectory(const std::string& path);
    std::string JoinPath(const std::string& dir, const std::string& file);
    
    // String utilities
    std::string ToLower(const std::string& str);
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
    
    // Hash utilities
    std::string CalculateMD5(const std::string& filePath);
    std::string CalculateSHA256(const std::string& filePath);
    
    // Platform utilities
    std::string GetPlatformString(PlatformTarget platform);
    std::string GetQualityString(QualityLevel quality);
    
    // Asset ID generation
    std::string GenerateUniqueId();
    std::string GenerateAssetId(const std::string& filePath, PlatformTarget platform, QualityLevel quality);
    
    // Format conversion utilities (inline for header availability)
    inline std::string FormatToString(AssetFormat format) {
        switch (format) {
            case AssetFormat::PNG: return "PNG";
            case AssetFormat::JPG: return "JPG";
            case AssetFormat::TGA: return "TGA";
            case AssetFormat::BMP: return "BMP";
            case AssetFormat::DDS: return "DDS";
            case AssetFormat::KTX: return "KTX";
            case AssetFormat::ASTC: return "ASTC";
            case AssetFormat::OBJ: return "OBJ";
            case AssetFormat::FBX: return "FBX";
            case AssetFormat::GLTF: return "GLTF";
            case AssetFormat::DAE: return "DAE";
            case AssetFormat::JSON: return "JSON";
            case AssetFormat::XML: return "XML";
            case AssetFormat::GLSL: return "GLSL";
            case AssetFormat::HLSL: return "HLSL";
            case AssetFormat::SPIRV: return "SPIRV";
            case AssetFormat::WAV: return "WAV";
            case AssetFormat::MP3: return "MP3";
            case AssetFormat::OGG: return "OGG";
            case AssetFormat::TTF: return "TTF";
            case AssetFormat::OTF: return "OTF";
            case AssetFormat::SVG: return "SVG";
            case AssetFormat::JSON_CONFIG: return "JSON_CONFIG";
            case AssetFormat::YAML: return "YAML";
            case AssetFormat::INI: return "INI";
            default: return "Unknown";
        }
    }
    
    inline std::string TypeToString(AssetType type) {
        switch (type) {
            case AssetType::Texture: return "Texture";
            case AssetType::Model: return "Model";
            case AssetType::Material: return "Material";
            case AssetType::Shader: return "Shader";
            case AssetType::Audio: return "Audio";
            case AssetType::Font: return "Font";
            case AssetType::Config: return "Config";
            case AssetType::Sprite: return "Sprite";
            default: return "Unknown";
        }
    }
    
    inline std::string StatusToString(ProcessingStatus status) {
        switch (status) {
            case ProcessingStatus::Pending: return "Pending";
            case ProcessingStatus::Processing: return "Processing";
            case ProcessingStatus::Completed: return "Completed";
            case ProcessingStatus::Failed: return "Failed";
            case ProcessingStatus::Cached: return "Cached";
            default: return "Unknown";
        }
    }
}

// Console commands for asset pipeline
class AssetPipelineCommands {
public:
    static void RegisterCommands() {
        std::cout << "[AssetPipeline] Pipeline commands available:" << std::endl;
        std::cout << "  asset.scan <directory> - Scan directory for assets" << std::endl;
        std::cout << "  asset.process <file> - Process single asset" << std::endl;
        std::cout << "  asset.batch <directory> - Process all assets in directory" << std::endl;
        std::cout << "  asset.validate <file> - Validate asset" << std::endl;
        std::cout << "  asset.optimize <file> - Optimize asset" << std::endl;
        std::cout << "  asset.compress <file> - Compress asset" << std::endl;
        std::cout << "  asset.cache.clear - Clear asset cache" << std::endl;
        std::cout << "  asset.database.save - Save asset database" << std::endl;
        std::cout << "  asset.database.load - Load asset database" << std::endl;
        std::cout << "  asset.report - Generate processing report" << std::endl;
        std::cout << "  asset.analytics - Show asset analytics" << std::endl;
        std::cout << "  asset.list - List all assets" << std::endl;
    }
    
    static void ExecuteCommand(const std::string& command, const std::vector<std::string>& args = {}) {
        auto& pipeline = AssetProcessingPipeline::Instance();
        
        if (command == "asset.scan" && !args.empty()) {
            auto assets = pipeline.ScanDirectory(args[0]);
            std::cout << "Found " << assets.size() << " assets in " << args[0] << std::endl;
        } else if (command == "asset.process" && !args.empty()) {
            AssetMetadata metadata;
            if (pipeline.ProcessAsset(args[0], metadata)) {
                std::cout << "Successfully processed: " << args[0] << std::endl;
            } else {
                std::cout << "Failed to process: " << args[0] << std::endl;
            }
        } else if (command == "asset.batch" && !args.empty()) {
            auto assets = pipeline.ScanDirectory(args[0]);
            pipeline.ProcessAssetBatch(assets);
            std::cout << "Batch processing started for " << assets.size() << " assets" << std::endl;
        } else if (command == "asset.validate" && !args.empty()) {
            auto result = pipeline.ValidateAsset(args[0]);
            std::cout << "Validation " << (result.isValid ? "passed" : "failed") 
                      << " for " << args[0] << std::endl;
            for (const auto& error : result.errors) {
                std::cout << "  Error: " << error << std::endl;
            }
        } else if (command == "asset.cache.clear") {
            pipeline.ClearCache();
            std::cout << "Asset cache cleared" << std::endl;
        } else if (command == "asset.database.save") {
            pipeline.SaveAssetDatabase();
            std::cout << "Asset database saved" << std::endl;
        } else if (command == "asset.database.load") {
            if (pipeline.LoadAssetDatabase()) {
                std::cout << "Asset database loaded" << std::endl;
            } else {
                std::cout << "Failed to load asset database" << std::endl;
            }
        } else if (command == "asset.report") {
            pipeline.GenerateProcessingReport();
        } else if (command == "asset.analytics") {
            std::cout << pipeline.GetAssetAnalytics() << std::endl;
        } else if (command == "asset.list") {
            auto assets = pipeline.GetAllAssets();
            std::cout << "Total assets: " << assets.size() << std::endl;
            for (const auto& asset : assets) {
                std::cout << "  " << asset.name << " (" << asset.id << ")" << std::endl;
            }
        } else {
            std::cout << "Unknown asset command: " << command << std::endl;
        }
    }
};

} // namespace assets