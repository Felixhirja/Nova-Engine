#include "AssetOptimizer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Nova {

// === Performance Profiling ===

void AssetOptimizer::StartLoadingProfile(const std::string& assetPath, const std::string& assetType) {
    if (!profilingEnabled_) return;
    
    AssetLoadingProfile profile;
    profile.assetPath = assetPath;
    profile.assetType = assetType;
    profile.lastAccessTime = std::chrono::steady_clock::now();
    
    loadingProfiles_[assetPath] = profile;
    loadStartTimes_[assetPath] = std::chrono::steady_clock::now();
}

void AssetOptimizer::EndLoadingProfile(const std::string& assetPath, size_t memoryBytes, size_t gpuMemoryBytes) {
    if (!profilingEnabled_) return;
    
    auto it = loadStartTimes_.find(assetPath);
    if (it != loadStartTimes_.end()) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second);
        
        auto& profile = loadingProfiles_[assetPath];
        profile.loadTimeMs = duration.count() / 1000.0;
        profile.memoryBytes = memoryBytes;
        profile.gpuMemoryBytes = gpuMemoryBytes;
        profile.referenceCount = 1;
        
        loadStartTimes_.erase(it);
    }
}

AssetLoadingProfile AssetOptimizer::GetLoadingProfile(const std::string& assetPath) const {
    auto it = loadingProfiles_.find(assetPath);
    if (it != loadingProfiles_.end()) {
        return it->second;
    }
    return AssetLoadingProfile();
}

void AssetOptimizer::RecordRenderProfile(const std::string& assetName, double renderTimeMs, 
                                        size_t triangleCount, size_t drawCalls) {
    if (!profilingEnabled_) return;
    
    auto& profile = renderingProfiles_[assetName];
    profile.assetName = assetName;
    profile.renderCount++;
    profile.avgRenderTimeMs = (profile.avgRenderTimeMs * (profile.renderCount - 1) + renderTimeMs) / profile.renderCount;
    profile.triangleCount = triangleCount;
    profile.drawCalls = drawCalls;
}

RenderingProfile AssetOptimizer::GetRenderingProfile(const std::string& assetName) const {
    auto it = renderingProfiles_.find(assetName);
    if (it != renderingProfiles_.end()) {
        return it->second;
    }
    return RenderingProfile();
}

void AssetOptimizer::ExportProfileReport(const std::string& outputPath) {
    // Implementation would write to file
    std::cout << "Performance Profile Report\n";
    std::cout << "=========================\n\n";
    
    std::cout << "Asset Loading Profiles:\n";
    for (const auto& [path, profile] : loadingProfiles_) {
        std::cout << "  " << path << ":\n";
        std::cout << "    Load Time: " << profile.loadTimeMs << "ms\n";
        std::cout << "    Memory: " << (profile.memoryBytes / 1024) << "KB\n";
        std::cout << "    GPU Memory: " << (profile.gpuMemoryBytes / 1024) << "KB\n";
    }
    
    std::cout << "\nRendering Profiles:\n";
    for (const auto& [name, profile] : renderingProfiles_) {
        std::cout << "  " << name << ":\n";
        std::cout << "    Avg Render Time: " << profile.avgRenderTimeMs << "ms\n";
        std::cout << "    Triangles: " << profile.triangleCount << "\n";
        std::cout << "    Draw Calls: " << profile.drawCalls << "\n";
    }
}

// === Memory Optimization ===

void AssetOptimizer::SetMemoryBudget(size_t systemMemory, size_t gpuMemory) {
    systemMemoryBudget_ = systemMemory;
    gpuMemoryBudget_ = gpuMemory;
}

MemoryUsageStats AssetOptimizer::GetMemoryStats() const {
    return memoryStats_;
}

void AssetOptimizer::UpdateMemoryStats() {
    // Calculate memory usage from profiles
    memoryStats_.usedSystemMemory = 0;
    memoryStats_.usedGPUMemory = 0;
    
    for (const auto& [path, profile] : loadingProfiles_) {
        memoryStats_.usedSystemMemory += profile.memoryBytes;
        memoryStats_.usedGPUMemory += profile.gpuMemoryBytes;
    }
}

bool AssetOptimizer::IsWithinMemoryBudget() const {
    return memoryStats_.usedSystemMemory <= systemMemoryBudget_ &&
           memoryStats_.usedGPUMemory <= gpuMemoryBudget_;
}

void AssetOptimizer::UnloadUnusedAssets(double timeThresholdSeconds) {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toUnload;
    
    for (const auto& [path, profile] : loadingProfiles_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - profile.lastAccessTime);
        if (elapsed.count() > timeThresholdSeconds && profile.referenceCount == 0) {
            toUnload.push_back(path);
        }
    }
    
    for (const auto& path : toUnload) {
        loadingProfiles_.erase(path);
        std::cout << "Unloaded unused asset: " << path << "\n";
    }
}

void AssetOptimizer::CompactMemory() {
    // Trigger memory compaction
    UpdateMemoryStats();
}

// === Quality Settings ===

void AssetOptimizer::SetQualityLevel(QualityLevel level) {
    qualitySettings_.level = level;
    ApplyQualitySettings();
}

void AssetOptimizer::ApplyQualitySettings() {
    QualityLevel level = qualitySettings_.level;
    switch (level) {
        case QualityLevel::Low:
            qualitySettings_.maxTextureSize = 1024;
            qualitySettings_.anisotropicFiltering = 2;
            qualitySettings_.lodLevels = 2;
            qualitySettings_.shadowQuality = 0;
            qualitySettings_.postProcessing = false;
            qualitySettings_.particleQuality = 0;
            qualitySettings_.bloom = false;
            qualitySettings_.ssao = false;
            break;
            
        case QualityLevel::Medium:
            qualitySettings_.maxTextureSize = 2048;
            qualitySettings_.anisotropicFiltering = 4;
            qualitySettings_.lodLevels = 3;
            qualitySettings_.shadowQuality = 1;
            qualitySettings_.postProcessing = true;
            qualitySettings_.particleQuality = 1;
            qualitySettings_.bloom = false;
            qualitySettings_.ssao = false;
            break;
            
        case QualityLevel::High:
            qualitySettings_.maxTextureSize = 4096;
            qualitySettings_.anisotropicFiltering = 8;
            qualitySettings_.lodLevels = 4;
            qualitySettings_.shadowQuality = 2;
            qualitySettings_.postProcessing = true;
            qualitySettings_.particleQuality = 2;
            qualitySettings_.bloom = true;
            qualitySettings_.ssao = true;
            break;
            
        case QualityLevel::Ultra:
            qualitySettings_.maxTextureSize = 8192;
            qualitySettings_.anisotropicFiltering = 16;
            qualitySettings_.lodLevels = 5;
            qualitySettings_.shadowQuality = 3;
            qualitySettings_.postProcessing = true;
            qualitySettings_.particleQuality = 3;
            qualitySettings_.bloom = true;
            qualitySettings_.ssao = true;
            break;
    }
}

void AssetOptimizer::AutoDetectQualitySettings() {
    // Simple auto-detection based on memory budget
    size_t totalMemoryGB = systemMemoryBudget_ / (1024ULL * 1024 * 1024);
    
    if (totalMemoryGB >= 8) {
        SetQualityLevel(QualityLevel::Ultra);
    } else if (totalMemoryGB >= 4) {
        SetQualityLevel(QualityLevel::High);
    } else if (totalMemoryGB >= 2) {
        SetQualityLevel(QualityLevel::Medium);
    } else {
        SetQualityLevel(QualityLevel::Low);
    }
}

// === Platform Optimization ===

void AssetOptimizer::DetectPlatformCapabilities() {
    // Detect hardware capabilities
    platformCapabilities_["texture_compression"] = true;
    platformCapabilities_["multithreading"] = true;
    platformCapabilities_["streaming"] = true;
    platformCapabilities_["hdr"] = true;
}

bool AssetOptimizer::IsPlatformCapable(const std::string& feature) const {
    auto it = platformCapabilities_.find(feature);
    return it != platformCapabilities_.end() && it->second;
}

void AssetOptimizer::SetPlatformProfile(const std::string& platform) {
    platformProfile_ = platform;
    
    if (platform == "mobile") {
        SetQualityLevel(QualityLevel::Medium);
        qualitySettings_.maxTextureSize = 2048;
    } else if (platform == "console") {
        SetQualityLevel(QualityLevel::High);
    } else if (platform == "desktop") {
        AutoDetectQualitySettings();
    }
}

// === Network Optimization ===

void AssetOptimizer::SetBandwidthLimit(size_t bytesPerSecond) {
    bandwidthLimit_ = bytesPerSecond;
}

void AssetOptimizer::PrioritizeNetworkAsset(const std::string& assetPath) {
    assetPriorities_[assetPath] = 100;
}

// === Utility ===

std::string AssetOptimizer::GetOptimizationStatus() const {
    std::string status = "Asset Optimization Status:\n";
    status += "  Profiling: " + std::string(profilingEnabled_ ? "Enabled" : "Disabled") + "\n";
    status += "  Quality Level: ";
    switch (qualitySettings_.level) {
        case QualityLevel::Low: status += "Low\n"; break;
        case QualityLevel::Medium: status += "Medium\n"; break;
        case QualityLevel::High: status += "High\n"; break;
        case QualityLevel::Ultra: status += "Ultra\n"; break;
    }
    status += "  Memory Budget: " + std::to_string(systemMemoryBudget_ / (1024*1024)) + "MB / ";
    status += std::to_string(gpuMemoryBudget_ / (1024*1024)) + "MB GPU\n";
    status += "  Memory Used: " + std::to_string(memoryStats_.usedSystemMemory / (1024*1024)) + "MB / ";
    status += std::to_string(memoryStats_.usedGPUMemory / (1024*1024)) + "MB GPU\n";
    status += "  Assets Loaded: " + std::to_string(loadingProfiles_.size()) + "\n";
    
    return status;
}

void AssetOptimizer::ResetStatistics() {
    loadingProfiles_.clear();
    renderingProfiles_.clear();
    loadStartTimes_.clear();
    memoryStats_ = MemoryUsageStats();
}

std::vector<AssetLoadingProfile> AssetOptimizer::GetAllLoadingProfiles() const {
    std::vector<AssetLoadingProfile> profiles;
    for (const auto& [path, profile] : loadingProfiles_) {
        profiles.push_back(profile);
    }
    return profiles;
}

void AssetOptimizer::ClearProfiles() {
    loadingProfiles_.clear();
    renderingProfiles_.clear();
}

size_t AssetOptimizer::GetAssetMemoryUsage(const std::string& assetPath) const {
    auto it = loadingProfiles_.find(assetPath);
    if (it != loadingProfiles_.end()) {
        return it->second.memoryBytes + it->second.gpuMemoryBytes;
    }
    return 0;
}

void AssetOptimizer::OptimizeMemoryUsage() {
    UnloadUnusedAssets(60.0);
    CompactMemory();
}

std::vector<std::string> AssetOptimizer::GetUnusedAssets(double timeoutSeconds) {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> unused;
    
    for (const auto& [path, profile] : loadingProfiles_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - profile.lastAccessTime);
        if (elapsed.count() > timeoutSeconds) {
            unused.push_back(path);
        }
    }
    
    return unused;
}

void AssetOptimizer::SetLoadingPriority(const std::string& assetPath, int priority) {
    assetPriorities_[assetPath] = priority;
}

void AssetOptimizer::PreloadAssets(const std::vector<std::string>& assetPaths) {
    for (const auto& path : assetPaths) {
        assetPriorities_[path] = 100;
    }
}

void AssetOptimizer::DumpOptimizationReport() {
    std::cout << GetOptimizationStatus() << std::endl;
}

} // namespace Nova
