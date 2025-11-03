#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>

namespace Nova {

// Performance profiling structures
struct AssetLoadingProfile {
    std::string assetPath;
    std::string assetType;
    double loadTimeMs = 0.0;
    size_t memoryBytes = 0;
    size_t gpuMemoryBytes = 0;
    int referenceCount = 0;
    std::chrono::steady_clock::time_point lastAccessTime;
};

struct RenderingProfile {
    std::string assetName;
    double avgRenderTimeMs = 0.0;
    int renderCount = 0;
    size_t triangleCount = 0;
    size_t drawCalls = 0;
};

// Memory optimization structures
struct MemoryUsageStats {
    size_t totalSystemMemory = 0;
    size_t usedSystemMemory = 0;
    size_t totalGPUMemory = 0;
    size_t usedGPUMemory = 0;
    size_t textureMemory = 0;
    size_t meshMemory = 0;
    size_t audioMemory = 0;
    size_t cachedMemory = 0;
};

// Quality level settings
enum class QualityLevel {
    Low,
    Medium,
    High,
    Ultra
};

struct QualitySettings {
    QualityLevel level = QualityLevel::High;
    
    // Texture settings
    int maxTextureSize = 4096;
    bool useTextureCompression = true;
    bool useMipmaps = true;
    int anisotropicFiltering = 16;
    
    // Model settings
    bool useLOD = true;
    int lodLevels = 4;
    float lodDistance = 100.0f;
    bool frustumCulling = true;
    bool occlusionCulling = false;
    
    // Effects settings
    int shadowQuality = 2;  // 0=off, 1=low, 2=med, 3=high
    bool postProcessing = true;
    int particleQuality = 2;
    bool bloom = true;
    bool ssao = true;
    
    // Performance settings
    int targetFPS = 60;
    bool vsync = true;
    bool dynamicResolution = false;
};

// Asset optimization manager
class AssetOptimizer {
public:
    static AssetOptimizer& GetInstance() {
        static AssetOptimizer instance;
        return instance;
    }

    // === PERFORMANCE PROFILING ===
    void StartLoadingProfile(const std::string& assetPath, const std::string& assetType);
    void EndLoadingProfile(const std::string& assetPath, size_t memoryBytes, size_t gpuMemoryBytes);
    void RecordRenderProfile(const std::string& assetName, double renderTimeMs, size_t triangles, size_t drawCalls);
    AssetLoadingProfile GetLoadingProfile(const std::string& assetPath) const;
    std::vector<AssetLoadingProfile> GetAllLoadingProfiles() const;
    RenderingProfile GetRenderingProfile(const std::string& assetName) const;
    void ClearProfiles();
    void ExportProfileReport(const std::string& outputPath);

    // === MEMORY OPTIMIZATION ===
    MemoryUsageStats GetMemoryStats() const;
    void UpdateMemoryStats();
    size_t GetAssetMemoryUsage(const std::string& assetPath) const;
    void SetMemoryBudget(size_t systemBytes, size_t gpuBytes);
    bool IsWithinMemoryBudget() const;
    void OptimizeMemoryUsage();
    std::vector<std::string> GetUnusedAssets(double timeoutSeconds = 60.0);
    void UnloadUnusedAssets(double timeoutSeconds = 60.0);
    void CompactMemory();

    // === LOADING OPTIMIZATION ===
    void EnableAsyncLoading(bool enable) { asyncLoadingEnabled_ = enable; }
    bool IsAsyncLoadingEnabled() const { return asyncLoadingEnabled_; }
    void SetLoadingPriority(const std::string& assetPath, int priority);
    void PreloadAssets(const std::vector<std::string>& assetPaths);
    void SetStreamingDistance(float distance) { streamingDistance_ = distance; }
    float GetStreamingDistance() const { return streamingDistance_; }
    void EnableBackgroundLoading(bool enable) { backgroundLoadingEnabled_ = enable; }
    
    // === QUALITY SETTINGS ===
    void SetQualityLevel(QualityLevel level);
    QualityLevel GetQualityLevel() const { return qualitySettings_.level; }
    QualitySettings& GetQualitySettings() { return qualitySettings_; }
    const QualitySettings& GetQualitySettings() const { return qualitySettings_; }
    void AutoDetectQualitySettings();
    void ApplyQualitySettings();
    
    // === PLATFORM OPTIMIZATION ===
    void DetectPlatformCapabilities();
    bool IsPlatformCapable(const std::string& feature) const;
    void SetPlatformProfile(const std::string& profile);  // "desktop", "mobile", "console"
    std::string GetPlatformProfile() const { return platformProfile_; }
    
    // === NETWORK OPTIMIZATION ===
    void EnableNetworkStreaming(bool enable) { networkStreamingEnabled_ = enable; }
    bool IsNetworkStreamingEnabled() const { return networkStreamingEnabled_; }
    void SetBandwidthLimit(size_t bytesPerSecond);
    size_t GetBandwidthLimit() const { return bandwidthLimit_; }
    void PrioritizeNetworkAsset(const std::string& assetPath);
    
    // === DIAGNOSTICS ===
    void EnableProfiling(bool enable) { profilingEnabled_ = enable; }
    bool IsProfilingEnabled() const { return profilingEnabled_; }
    void DumpOptimizationReport();
    std::string GetOptimizationStatus() const;
    void ResetStatistics();

private:
    AssetOptimizer() = default;
    ~AssetOptimizer() = default;
    
    AssetOptimizer(const AssetOptimizer&) = delete;
    AssetOptimizer& operator=(const AssetOptimizer&) = delete;

    // Internal state
    std::unordered_map<std::string, AssetLoadingProfile> loadingProfiles_;
    std::unordered_map<std::string, RenderingProfile> renderingProfiles_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> loadStartTimes_;
    
    MemoryUsageStats memoryStats_;
    size_t systemMemoryBudget_ = 2ULL * 1024 * 1024 * 1024;  // 2GB
    size_t gpuMemoryBudget_ = 1ULL * 1024 * 1024 * 1024;     // 1GB
    
    QualitySettings qualitySettings_;
    std::string platformProfile_ = "desktop";
    
    bool asyncLoadingEnabled_ = true;
    bool backgroundLoadingEnabled_ = true;
    float streamingDistance_ = 500.0f;
    
    bool networkStreamingEnabled_ = false;
    size_t bandwidthLimit_ = 10 * 1024 * 1024;  // 10MB/s
    
    bool profilingEnabled_ = true;
    
    std::unordered_map<std::string, bool> platformCapabilities_;
    std::unordered_map<std::string, int> assetPriorities_;
};

} // namespace Nova
