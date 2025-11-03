#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Nova {

// Texture compression formats
enum class TextureFormat {
    Unknown,
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    DXT1,       // BC1 - RGB compression
    DXT5,       // BC3 - RGBA compression
    BC4,        // Single channel compression
    BC5,        // Two channel compression
    BC6H,       // HDR compression
    BC7,        // High quality compression
    ETC2_RGB8,  // Mobile compression
    ETC2_RGBA8,
    ASTC_4x4,   // Adaptive compression
    ASTC_8x8
};

// Texture quality settings
struct TextureQualityConfig {
    int maxResolution = 4096;
    bool generateMipmaps = true;
    bool useCompression = true;
    TextureFormat preferredFormat = TextureFormat::RGBA8;
    int anisotropy = 16;
    bool sRGB = true;
    
    // Compression quality (0-100)
    int compressionQuality = 85;
};

// Texture metadata
struct TextureMetadata {
    std::string path;
    int width = 0;
    int height = 0;
    int channels = 0;
    TextureFormat format = TextureFormat::Unknown;
    size_t memorySize = 0;
    size_t compressedSize = 0;
    bool hasMipmaps = false;
    int mipmapLevels = 0;
    bool isCompressed = false;
    bool isStreaming = false;
};

class TextureOptimizer {
public:
    static TextureOptimizer& GetInstance() {
        static TextureOptimizer instance;
        return instance;
    }

    // === TEXTURE COMPRESSION ===
    bool CompressTexture(const std::string& inputPath, const std::string& outputPath, 
                        TextureFormat format, int quality = 85);
    bool DecompressTexture(const std::string& inputPath, const std::string& outputPath);
    TextureFormat SelectOptimalFormat(int channels, bool hasAlpha, bool isNormalMap, bool isHDR);
    size_t EstimateCompressedSize(int width, int height, TextureFormat format);
    
    // === MIPMAP GENERATION ===
    bool GenerateMipmaps(const std::string& texturePath, int levels = -1);
    int CalculateOptimalMipmapLevels(int width, int height);
    void SetMipmapFilter(const std::string& filter);  // "box", "triangle", "kaiser"
    
    // === TEXTURE RESIZING ===
    bool ResizeTexture(const std::string& inputPath, const std::string& outputPath,
                      int targetWidth, int targetHeight, bool maintainAspect = true);
    bool GenerateLODChain(const std::string& texturePath, int levels = 4);
    
    // === FORMAT CONVERSION ===
    bool ConvertFormat(const std::string& inputPath, const std::string& outputPath,
                      TextureFormat sourceFormat, TextureFormat targetFormat);
    bool ConvertToOptimalFormat(const std::string& texturePath);
    std::vector<TextureFormat> GetSupportedFormats() const;
    
    // === TEXTURE ATLAS ===
    struct AtlasEntry {
        std::string name;
        int x, y, width, height;
        int atlasId;
    };
    
    int CreateTextureAtlas(const std::vector<std::string>& texturePaths, 
                          const std::string& outputPath, int maxSize = 4096);
    std::vector<AtlasEntry> GetAtlasEntries(int atlasId) const;
    bool PackTextures(const std::vector<std::string>& textures, int& outWidth, int& outHeight);
    
    // === TEXTURE STREAMING ===
    void EnableStreaming(const std::string& texturePath, bool enable = true);
    bool IsStreaming(const std::string& texturePath) const;
    void SetStreamingMipLevel(const std::string& texturePath, int level);
    void UpdateStreamingPriorities(const std::vector<std::string>& visibleTextures);
    
    // === METADATA & ANALYSIS ===
    TextureMetadata GetMetadata(const std::string& texturePath) const;
    void CacheMetadata(const std::string& texturePath, const TextureMetadata& metadata);
    size_t AnalyzeMemoryUsage(const std::string& texturePath);
    std::vector<std::string> FindUnoptimizedTextures(size_t minSize = 1024 * 1024);
    
    // === QUALITY SETTINGS ===
    void SetQualityConfig(const TextureQualityConfig& config);
    TextureQualityConfig GetQualityConfig() const { return qualityConfig_; }
    void ApplyQualityPreset(const std::string& preset);  // "low", "medium", "high", "ultra"
    
    // === BATCH OPERATIONS ===
    void OptimizeDirectory(const std::string& directory, bool recursive = true);
    void CompressDirectory(const std::string& directory, TextureFormat format, bool recursive = true);
    void GenerateMipmapsForDirectory(const std::string& directory, bool recursive = true);
    int BatchConvert(const std::vector<std::string>& textures, TextureFormat targetFormat);
    
    // === DIAGNOSTICS ===
    void DumpTextureReport(const std::string& outputPath);
    size_t GetTotalTextureMemory() const;
    int GetTextureCount() const;
    void ClearCache();

private:
    TextureOptimizer() = default;
    ~TextureOptimizer() = default;
    
    TextureOptimizer(const TextureOptimizer&) = delete;
    TextureOptimizer& operator=(const TextureOptimizer&) = delete;

    // Internal helpers
    bool LoadTextureData(const std::string& path, std::vector<unsigned char>& data,
                        int& width, int& height, int& channels);
    bool SaveTextureData(const std::string& path, const std::vector<unsigned char>& data,
                        int width, int height, int channels);
    bool CompressTextureData(const std::vector<unsigned char>& input, std::vector<unsigned char>& output,
                            int width, int height, TextureFormat format, int quality);
    void GenerateMipmapLevel(const std::vector<unsigned char>& src, std::vector<unsigned char>& dst,
                            int srcWidth, int srcHeight, int channels);

    TextureQualityConfig qualityConfig_;
    std::unordered_map<std::string, TextureMetadata> metadataCache_;
    std::unordered_map<int, std::vector<AtlasEntry>> atlasData_;
    std::unordered_map<std::string, bool> streamingTextures_;
    int nextAtlasId_ = 1;
};

} // namespace Nova
