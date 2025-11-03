#include "AssetProcessingPipeline.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <condition_variable>
#include <regex>
#include <iomanip>

namespace assets {

namespace fs = std::filesystem;

// Implementation of AssetProcessingPipeline
void AssetProcessingPipeline::Initialize(const ProcessingConfig& config) {
    config_ = config;
    startTime_ = std::chrono::high_resolution_clock::now();
    
    // Create necessary directories
    CreateDirectories(config_.outputDirectory);
    CreateDirectories(config_.cacheDirectory);
    
    // Start worker threads
    shouldStop_.store(false);
    for (size_t i = 0; i < config_.maxThreads; ++i) {
        workerThreads_.emplace_back(&AssetProcessingPipeline::ProcessingWorker, this);
    }
    
    std::cout << "[AssetPipeline] Initialized with " << config_.maxThreads 
              << " worker threads" << std::endl;
    std::cout << "[AssetPipeline] Output directory: " << config_.outputDirectory << std::endl;
    std::cout << "[AssetPipeline] Cache directory: " << config_.cacheDirectory << std::endl;
}

void AssetProcessingPipeline::Shutdown() {
    shouldStop_.store(true);
    queueCondition_.notify_all();
    
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads_.clear();
    
    // Save asset database before shutdown
    SaveAssetDatabase();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime_);
    
    std::cout << "[AssetPipeline] Shutdown complete" << std::endl;
    std::cout << "[AssetPipeline] Session stats: " << totalProcessed_.load() << " processed, "
              << totalFailed_.load() << " failed, " << totalCached_.load() << " cached" << std::endl;
    std::cout << "[AssetPipeline] Total runtime: " << duration.count() << " seconds" << std::endl;
}

void AssetProcessingPipeline::RegisterProcessor(std::unique_ptr<IAssetProcessor> processor) {
    if (!processor) return;
    
    std::cout << "[AssetPipeline] Registered processor: " << processor->GetProcessorName() << std::endl;
    std::cout << "[AssetPipeline] Supported formats: ";
    auto formats = processor->GetSupportedFormats();
    for (const auto& format : formats) {
        std::cout << FormatToString(format) << " ";
    }
    std::cout << std::endl;
    
    processors_.push_back(std::move(processor));
}

void AssetProcessingPipeline::RegisterValidator(std::unique_ptr<IAssetValidator> validator) {
    if (!validator) return;
    
    std::cout << "[AssetPipeline] Registered validator: " << validator->GetValidatorName() << std::endl;
    validators_.push_back(std::move(validator));
}

bool AssetProcessingPipeline::ProcessAsset(const std::string& inputPath, AssetMetadata& metadata) {
    if (!pipeline_utils::FileExists(inputPath)) {
        std::cout << "[AssetPipeline] File not found: " << inputPath << std::endl;
        return false;
    }
    
    // Initialize metadata
    metadata.originalPath = inputPath;
    metadata.name = pipeline_utils::GetFileName(inputPath);
    metadata.format = DetectFormat(inputPath);
    metadata.type = DetermineType(metadata.format);
    metadata.originalSize = pipeline_utils::GetFileSize(inputPath);
    metadata.creationTime = std::chrono::system_clock::now();
    metadata.platform = config_.targetPlatform;
    metadata.quality = config_.targetQuality;
    metadata.checksum = CalculateChecksum(inputPath);
    metadata.id = GenerateAssetId(inputPath);
    
    // Check if cached
    if (config_.enableCaching && IsCached(metadata)) {
        if (LoadFromCache(metadata)) {
            metadata.status = ProcessingStatus::Cached;
            totalCached_.fetch_add(1);
            return true;
        }
    }
    
    // Validate if enabled
    if (config_.enableValidation) {
        auto validation = ValidateAsset(inputPath);
        if (!validation.isValid) {
            std::cout << "[AssetPipeline] Validation failed for: " << inputPath << std::endl;
            for (const auto& error : validation.errors) {
                std::cout << "[AssetPipeline]   Error: " << error << std::endl;
            }
            metadata.status = ProcessingStatus::Failed;
            totalFailed_.fetch_add(1);
            return false;
        }
        
        // Store validation properties
        for (const auto& prop : validation.properties) {
            metadata.properties[prop.first] = prop.second;
        }
    }
    
    // Find appropriate processor
    IAssetProcessor* processor = nullptr;
    for (auto& proc : processors_) {
        if (proc->CanProcess(metadata.format)) {
            processor = proc.get();
            break;
        }
    }
    
    if (!processor) {
        std::cout << "[AssetPipeline] No processor found for format: " 
                  << FormatToString(metadata.format) << std::endl;
        metadata.status = ProcessingStatus::Failed;
        totalFailed_.fetch_add(1);
        return false;
    }
    
    // Generate output path
    std::string outputDir = config_.outputDirectory + "/" + 
                           pipeline_utils::GetPlatformString(metadata.platform) + "/" +
                           pipeline_utils::GetQualityString(metadata.quality);
    CreateDirectories(outputDir);
    
    metadata.processedPath = outputDir + "/" + metadata.name;
    
    // Process the asset
    metadata.status = ProcessingStatus::Processing;
    if (processor->Process(metadata, inputPath, metadata.processedPath)) {
        metadata.status = ProcessingStatus::Completed;
        metadata.processedSize = pipeline_utils::GetFileSize(metadata.processedPath);
        metadata.lastModified = std::chrono::system_clock::now();
        
        // Apply optimization if enabled
        if (config_.enableOptimization) {
            OptimizeAsset(metadata, config_);
        }
        
        // Apply compression if enabled
        if (config_.enableCompression) {
            CompressAsset(metadata);
        }
        
        // Update dependencies
        UpdateDependencies(metadata);
        
        // Cache the result
        if (config_.enableCaching) {
            CacheAsset(metadata);
        }
        
        // Store in database
        {
            std::lock_guard<std::mutex> lock(databaseMutex_);
            assetDatabase_[metadata.id] = metadata;
        }
        
        totalProcessed_.fetch_add(1);
        std::cout << "[AssetPipeline] Successfully processed: " << inputPath << std::endl;
        return true;
    } else {
        metadata.status = ProcessingStatus::Failed;
        totalFailed_.fetch_add(1);
        std::cout << "[AssetPipeline] Failed to process: " << inputPath << std::endl;
        return false;
    }
}

void AssetProcessingPipeline::ProcessAssetBatch(const std::vector<std::string>& inputPaths) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        for (const auto& path : inputPaths) {
            processingQueue_.push(path);
        }
    }
    queueCondition_.notify_all();
    
    std::cout << "[AssetPipeline] Queued " << inputPaths.size() << " assets for batch processing" << std::endl;
}

ProcessingStatus AssetProcessingPipeline::GetAssetStatus(const std::string& assetId) const {
    std::lock_guard<std::mutex> lock(databaseMutex_);
    auto it = assetDatabase_.find(assetId);
    return (it != assetDatabase_.end()) ? it->second.status : ProcessingStatus::Pending;
}

std::vector<std::string> AssetProcessingPipeline::ScanDirectory(const std::string& directory, bool recursive) {
    std::vector<std::string> assets;
    
    if (!fs::exists(directory)) {
        std::cout << "[AssetPipeline] Directory not found: " << directory << std::endl;
        return assets;
    }
    
    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    auto format = DetectFormat(entry.path().string());
                    if (format != AssetFormat::Unknown) {
                        assets.push_back(entry.path().string());
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    auto format = DetectFormat(entry.path().string());
                    if (format != AssetFormat::Unknown) {
                        assets.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << "[AssetPipeline] Filesystem error: " << e.what() << std::endl;
    }
    
    std::cout << "[AssetPipeline] Scanned " << directory 
              << (recursive ? " (recursive)" : "") 
              << ", found " << assets.size() << " assets" << std::endl;
    
    return assets;
}

AssetFormat AssetProcessingPipeline::DetectFormat(const std::string& filePath) const {
    auto extension = pipeline_utils::GetFileExtension(filePath);
    auto format = DetectFormatByExtension(extension);
    
    if (format == AssetFormat::Unknown) {
        format = DetectFormatByContent(filePath);
    }
    
    return format;
}

AssetType AssetProcessingPipeline::DetermineType(AssetFormat format) const {
    switch (format) {
        case AssetFormat::PNG:
        case AssetFormat::JPG:
        case AssetFormat::TGA:
        case AssetFormat::BMP:
        case AssetFormat::DDS:
        case AssetFormat::KTX:
        case AssetFormat::ASTC:
            return AssetType::Texture;
            
        case AssetFormat::OBJ:
        case AssetFormat::FBX:
        case AssetFormat::GLTF:
        case AssetFormat::DAE:
            return AssetType::Model;
            
        case AssetFormat::GLSL:
        case AssetFormat::HLSL:
        case AssetFormat::SPIRV:
            return AssetType::Shader;
            
        case AssetFormat::WAV:
        case AssetFormat::MP3:
        case AssetFormat::OGG:
            return AssetType::Audio;
            
        case AssetFormat::TTF:
        case AssetFormat::OTF:
            return AssetType::Font;
            
        case AssetFormat::SVG:
            return AssetType::Sprite;
            
        case AssetFormat::JSON:
        case AssetFormat::XML:
        case AssetFormat::JSON_CONFIG:
        case AssetFormat::YAML:
        case AssetFormat::INI:
            return AssetType::Config;
            
        default:
            return AssetType::Unknown;
    }
}

ValidationResult AssetProcessingPipeline::ValidateAsset(const std::string& filePath) {
    auto format = DetectFormat(filePath);
    auto type = DetermineType(format);
    
    ValidationResult result;
    
    // Find appropriate validator
    for (auto& validator : validators_) {
        if (validator->CanValidate(type)) {
            AssetMetadata tempMetadata;
            tempMetadata.originalPath = filePath;
            tempMetadata.format = format;
            tempMetadata.type = type;
            
            auto validationResult = validator->Validate(tempMetadata, filePath);
            
            // Combine results
            result.isValid = result.isValid && validationResult.isValid;
            result.errors.insert(result.errors.end(), validationResult.errors.begin(), validationResult.errors.end());
            result.warnings.insert(result.warnings.end(), validationResult.warnings.begin(), validationResult.warnings.end());
            
            for (const auto& prop : validationResult.properties) {
                result.properties[prop.first] = prop.second;
            }
        }
    }
    
    return result;
}

bool AssetProcessingPipeline::OptimizeAsset(AssetMetadata& metadata, const ProcessingConfig& config) {
    // Platform-specific optimization
    switch (config.targetPlatform) {
        case PlatformTarget::Mobile:
            // Reduce texture sizes, compress meshes
            metadata.properties["optimization"] = "mobile";
            break;
        case PlatformTarget::Console:
            // Balance between quality and performance
            metadata.properties["optimization"] = "console";
            break;
        case PlatformTarget::Web:
            // Minimize file sizes for web delivery
            metadata.properties["optimization"] = "web";
            break;
        default:
            metadata.properties["optimization"] = "desktop";
            break;
    }
    
    return true;
}

bool AssetProcessingPipeline::ConvertAsset(AssetMetadata& metadata, AssetFormat targetFormat) {
    // Find converter for the target format
    for (auto& processor : processors_) {
        auto supportedFormats = processor->GetSupportedFormats();
        if (std::find(supportedFormats.begin(), supportedFormats.end(), targetFormat) != supportedFormats.end()) {
            return processor->Process(metadata, metadata.originalPath, metadata.processedPath);
        }
    }
    
    return false;
}

bool AssetProcessingPipeline::CompressAsset(AssetMetadata& metadata) {
    // Asset-specific compression
    switch (metadata.type) {
        case AssetType::Texture:
            metadata.properties["compression"] = "texture_compressed";
            break;
        case AssetType::Model:
            metadata.properties["compression"] = "mesh_compressed";
            break;
        case AssetType::Audio:
            metadata.properties["compression"] = "audio_compressed";
            break;
        default:
            metadata.properties["compression"] = "generic";
            break;
    }
    
    return true;
}

bool AssetProcessingPipeline::IsCached(const AssetMetadata& metadata) const {
    std::string cacheFile = config_.cacheDirectory + "/" + metadata.id + ".cache";
    return pipeline_utils::FileExists(cacheFile);
}

bool AssetProcessingPipeline::CacheAsset(const AssetMetadata& metadata) {
    std::string cacheFile = config_.cacheDirectory + "/" + metadata.id + ".cache";
    
    std::ofstream file(cacheFile);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple cache format (could be improved with binary format)
    file << "id=" << metadata.id << "\n";
    file << "name=" << metadata.name << "\n";
    file << "originalPath=" << metadata.originalPath << "\n";
    file << "processedPath=" << metadata.processedPath << "\n";
    file << "checksum=" << metadata.checksum << "\n";
    file << "originalSize=" << metadata.originalSize << "\n";
    file << "processedSize=" << metadata.processedSize << "\n";
    
    return true;
}

bool AssetProcessingPipeline::LoadFromCache(AssetMetadata& metadata) {
    std::string cacheFile = config_.cacheDirectory + "/" + metadata.id + ".cache";
    
    std::ifstream file(cacheFile);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "processedPath") {
            metadata.processedPath = value;
        } else if (key == "processedSize") {
            metadata.processedSize = std::stoull(value);
        }
    }
    
    return pipeline_utils::FileExists(metadata.processedPath);
}

void AssetProcessingPipeline::ClearCache() {
    try {
        if (fs::exists(config_.cacheDirectory)) {
            fs::remove_all(config_.cacheDirectory);
            CreateDirectories(config_.cacheDirectory);
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << "[AssetPipeline] Error clearing cache: " << e.what() << std::endl;
    }
}

void AssetProcessingPipeline::SaveAssetDatabase(const std::string& databasePath) {
    std::lock_guard<std::mutex> lock(databaseMutex_);
    
    std::ofstream file(databasePath);
    if (!file.is_open()) {
        std::cout << "[AssetPipeline] Failed to save asset database: " << databasePath << std::endl;
        return;
    }
    
    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"assets\": [\n";
    
    bool first = true;
    for (const auto& [id, metadata] : assetDatabase_) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"id\": \"" << metadata.id << "\",\n";
        file << "      \"name\": \"" << metadata.name << "\",\n";
        file << "      \"originalPath\": \"" << metadata.originalPath << "\",\n";
        file << "      \"processedPath\": \"" << metadata.processedPath << "\",\n";
        file << "      \"type\": \"" << TypeToString(metadata.type) << "\",\n";
        file << "      \"format\": \"" << FormatToString(metadata.format) << "\",\n";
        file << "      \"status\": \"" << StatusToString(metadata.status) << "\",\n";
        file << "      \"originalSize\": " << metadata.originalSize << ",\n";
        file << "      \"processedSize\": " << metadata.processedSize << ",\n";
        file << "      \"checksum\": \"" << metadata.checksum << "\"\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    std::cout << "[AssetPipeline] Saved asset database with " << assetDatabase_.size() << " assets" << std::endl;
}

bool AssetProcessingPipeline::LoadAssetDatabase(const std::string& databasePath) {
    if (!pipeline_utils::FileExists(databasePath)) {
        std::cout << "[AssetPipeline] Asset database not found: " << databasePath << std::endl;
        return false;
    }
    
    // Simple JSON parsing (could be improved with proper JSON library)
    std::ifstream file(databasePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(databaseMutex_);
    assetDatabase_.clear();
    
    std::cout << "[AssetPipeline] Asset database loaded (simplified parsing)" << std::endl;
    return true;
}

void AssetProcessingPipeline::GenerateProcessingReport() {
    std::cout << "\n=== Asset Processing Report ===" << std::endl;
    std::cout << "Total assets processed: " << totalProcessed_.load() << std::endl;
    std::cout << "Total assets failed: " << totalFailed_.load() << std::endl;
    std::cout << "Total assets cached: " << totalCached_.load() << std::endl;
    
    std::lock_guard<std::mutex> lock(databaseMutex_);
    std::cout << "Assets in database: " << assetDatabase_.size() << std::endl;
    
    // Statistics by type
    std::unordered_map<AssetType, size_t> typeCount;
    for (const auto& [id, metadata] : assetDatabase_) {
        typeCount[metadata.type]++;
    }
    
    std::cout << "\nAssets by type:" << std::endl;
    for (const auto& [type, count] : typeCount) {
        std::cout << "  " << TypeToString(type) << ": " << count << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime_);
    std::cout << "\nProcessing time: " << duration.count() << " seconds" << std::endl;
    std::cout << "================================\n" << std::endl;
}

std::string AssetProcessingPipeline::GetAssetAnalytics() const {
    std::stringstream ss;
    ss << "Asset Pipeline Analytics:\n";
    ss << "  Processed: " << totalProcessed_.load() << "\n";
    ss << "  Failed: " << totalFailed_.load() << "\n";
    ss << "  Cached: " << totalCached_.load() << "\n";
    ss << "  Database entries: " << assetDatabase_.size() << "\n";
    ss << "  Worker threads: " << config_.maxThreads << "\n";
    return ss.str();
}

std::vector<AssetMetadata> AssetProcessingPipeline::GetAllAssets() const {
    std::lock_guard<std::mutex> lock(databaseMutex_);
    std::vector<AssetMetadata> assets;
    assets.reserve(assetDatabase_.size());
    
    for (const auto& [id, metadata] : assetDatabase_) {
        assets.push_back(metadata);
    }
    
    return assets;
}

AssetMetadata* AssetProcessingPipeline::FindAsset(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(databaseMutex_);
    auto it = assetDatabase_.find(assetId);
    return (it != assetDatabase_.end()) ? &it->second : nullptr;
}

bool AssetProcessingPipeline::RemoveAsset(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(databaseMutex_);
    return assetDatabase_.erase(assetId) > 0;
}

void AssetProcessingPipeline::UpdateDependencies(AssetMetadata& metadata) {
    // Scan processed file for dependencies (simplified implementation)
    metadata.dependencies.clear();
    
    // TODO: Implement proper dependency scanning based on asset type
    switch (metadata.type) {
        case AssetType::Material:
            // Scan for texture references
            break;
        case AssetType::Model:
            // Scan for material references
            break;
        default:
            break;
    }
}

std::vector<std::string> AssetProcessingPipeline::GetDependents(const std::string& assetId) const {
    std::vector<std::string> dependents;
    std::lock_guard<std::mutex> lock(databaseMutex_);
    
    for (const auto& [id, metadata] : assetDatabase_) {
        auto& deps = metadata.dependencies;
        if (std::find(deps.begin(), deps.end(), assetId) != deps.end()) {
            dependents.push_back(id);
        }
    }
    
    return dependents;
}

// Private methods
std::string AssetProcessingPipeline::GenerateAssetId(const std::string& filePath) const {
    return pipeline_utils::GenerateAssetId(filePath, config_.targetPlatform, config_.targetQuality);
}

std::string AssetProcessingPipeline::CalculateChecksum(const std::string& filePath) const {
    return pipeline_utils::CalculateMD5(filePath);
}

bool AssetProcessingPipeline::CreateDirectories(const std::string& path) const {
    try {
        return fs::create_directories(path);
    } catch (const fs::filesystem_error& e) {
        std::cout << "[AssetPipeline] Error creating directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

void AssetProcessingPipeline::ProcessingWorker() {
    while (!shouldStop_.load()) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        queueCondition_.wait(lock, [this] { return !processingQueue_.empty() || shouldStop_.load(); });
        
        if (shouldStop_.load()) break;
        
        if (!processingQueue_.empty()) {
            std::string filePath = processingQueue_.front();
            processingQueue_.pop();
            lock.unlock();
            
            AssetMetadata metadata;
            ProcessAsset(filePath, metadata);
        }
    }
}

AssetFormat AssetProcessingPipeline::DetectFormatByExtension(const std::string& extension) const {
    std::string ext = pipeline_utils::ToLower(extension);
    
    // Texture formats
    if (ext == ".png") return AssetFormat::PNG;
    if (ext == ".jpg" || ext == ".jpeg") return AssetFormat::JPG;
    if (ext == ".tga") return AssetFormat::TGA;
    if (ext == ".bmp") return AssetFormat::BMP;
    if (ext == ".dds") return AssetFormat::DDS;
    if (ext == ".ktx") return AssetFormat::KTX;
    if (ext == ".astc") return AssetFormat::ASTC;
    
    // Model formats
    if (ext == ".obj") return AssetFormat::OBJ;
    if (ext == ".fbx") return AssetFormat::FBX;
    if (ext == ".gltf" || ext == ".glb") return AssetFormat::GLTF;
    if (ext == ".dae") return AssetFormat::DAE;
    
    // Shader formats
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom") return AssetFormat::GLSL;
    if (ext == ".hlsl") return AssetFormat::HLSL;
    if (ext == ".spirv") return AssetFormat::SPIRV;
    
    // Audio formats
    if (ext == ".wav") return AssetFormat::WAV;
    if (ext == ".mp3") return AssetFormat::MP3;
    if (ext == ".ogg") return AssetFormat::OGG;
    
    // Font formats
    if (ext == ".ttf") return AssetFormat::TTF;
    if (ext == ".otf") return AssetFormat::OTF;
    
    // Vector graphics
    if (ext == ".svg") return AssetFormat::SVG;
    
    // Config formats
    if (ext == ".json") return AssetFormat::JSON;
    if (ext == ".xml") return AssetFormat::XML;
    if (ext == ".yaml" || ext == ".yml") return AssetFormat::YAML;
    if (ext == ".ini") return AssetFormat::INI;
    
    return AssetFormat::Unknown;
}

AssetFormat AssetProcessingPipeline::DetectFormatByContent(const std::string& filePath) const {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return AssetFormat::Unknown;
    
    // Read magic bytes
    char magic[16] = {0};
    file.read(magic, sizeof(magic));
    
    // PNG magic bytes
    if (magic[0] == '\x89' && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        return AssetFormat::PNG;
    }
    
    // JPEG magic bytes
    if (magic[0] == '\xFF' && magic[1] == '\xD8') {
        return AssetFormat::JPG;
    }
    
    // Check for text-based formats
    file.seekg(0);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    if (content.find("{") != std::string::npos) {
        return AssetFormat::JSON;
    }
    
    if (content.find("<?xml") != std::string::npos) {
        return AssetFormat::XML;
    }
    
    return AssetFormat::Unknown;
}

std::string AssetProcessingPipeline::FormatToString(AssetFormat format) const {
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

std::string AssetProcessingPipeline::TypeToString(AssetType type) const {
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

std::string AssetProcessingPipeline::StatusToString(ProcessingStatus status) const {
    switch (status) {
        case ProcessingStatus::Pending: return "Pending";
        case ProcessingStatus::Processing: return "Processing";
        case ProcessingStatus::Completed: return "Completed";
        case ProcessingStatus::Failed: return "Failed";
        case ProcessingStatus::Cached: return "Cached";
        default: return "Unknown";
    }
}

// Utility functions implementation
namespace pipeline_utils {

bool FileExists(const std::string& path) {
    return fs::exists(path);
}

size_t GetFileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (const fs::filesystem_error&) {
        return 0;
    }
}

std::string GetFileExtension(const std::string& path) {
    return fs::path(path).extension().string();
}

std::string GetFileName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string GetDirectory(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string JoinPath(const std::string& dir, const std::string& file) {
    return (fs::path(dir) / file).string();
}

std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string Trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string CalculateMD5(const std::string& filePath) {
    // Simplified checksum (would use proper MD5 in production)
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";
    
    size_t hash = 0;
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        for (size_t i = 0; i < file.gcount(); ++i) {
            hash ^= std::hash<char>{}(buffer[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
    }
    
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string CalculateSHA256(const std::string& filePath) {
    // Placeholder implementation
    return CalculateMD5(filePath) + "_sha256";
}

std::string GetPlatformString(PlatformTarget platform) {
    switch (platform) {
        case PlatformTarget::Desktop: return "desktop";
        case PlatformTarget::Mobile: return "mobile";
        case PlatformTarget::Console: return "console";
        case PlatformTarget::Web: return "web";
        case PlatformTarget::Universal: return "universal";
        default: return "unknown";
    }
}

std::string GetQualityString(QualityLevel quality) {
    switch (quality) {
        case QualityLevel::Low: return "low";
        case QualityLevel::Medium: return "medium";
        case QualityLevel::High: return "high";
        case QualityLevel::Ultra: return "ultra";
        default: return "medium";
    }
}

std::string GenerateUniqueId() {
    static std::atomic<size_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "asset_" << timestamp << "_" << counter.fetch_add(1);
    return ss.str();
}

std::string GenerateAssetId(const std::string& filePath, PlatformTarget platform, QualityLevel quality) {
    std::string name = GetFileName(filePath);
    std::string platformStr = GetPlatformString(platform);
    std::string qualityStr = GetQualityString(quality);
    
    std::stringstream ss;
    ss << name << "_" << platformStr << "_" << qualityStr;
    
    // Replace invalid characters
    std::string result = ss.str();
    std::replace(result.begin(), result.end(), '.', '_');
    std::replace(result.begin(), result.end(), ' ', '_');
    
    return result;
}

} // namespace pipeline_utils

} // namespace assets