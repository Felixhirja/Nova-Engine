#include "AssetPipeline.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <cstring>

namespace fs = std::filesystem;

namespace AssetPipeline {

// ============================================================================
// Asset Validator Implementation
// ============================================================================

AssetValidator& AssetValidator::GetInstance() {
    static AssetValidator instance;
    return instance;
}

void AssetValidator::RegisterValidator(AssetType type, ValidatorFunc validator) {
    std::lock_guard<std::mutex> lock(mutex_);
    validators_[type] = validator;
}

AssetValidationResult AssetValidator::ValidateAsset(const AssetMetadata& metadata) {
    AssetValidationResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    if (!FileExists(metadata.path)) {
        result.is_valid = false;
        result.errors.push_back("File does not exist: " + metadata.path);
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = validators_.find(metadata.type);
    if (it != validators_.end()) {
        result = it->second(metadata);
    } else {
        result.warnings.push_back("No validator registered for asset type: " + 
                                 GetAssetTypeName(metadata.type));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

bool AssetValidator::ValidateAllAssets(std::vector<AssetValidationResult>& results) {
    bool all_valid = true;
    
    // Would iterate through all registered assets
    // For now, returns success
    (void)results; // Suppress unused parameter warning
    
    return all_valid;
}

// ============================================================================
// Dependency Tracker Implementation
// ============================================================================

DependencyTracker& DependencyTracker::GetInstance() {
    static DependencyTracker instance;
    return instance;
}

void DependencyTracker::RegisterDependency(const std::string& asset, const std::string& dependency) {
    std::lock_guard<std::mutex> lock(mutex_);
    dependencies_[asset].insert(dependency);
    dependents_[dependency].insert(asset);
}

void DependencyTracker::RemoveDependency(const std::string& asset, const std::string& dependency) {
    std::lock_guard<std::mutex> lock(mutex_);
    dependencies_[asset].erase(dependency);
    dependents_[dependency].erase(asset);
}

std::vector<std::string> DependencyTracker::GetDependencies(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dependencies_.find(asset);
    if (it != dependencies_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

std::vector<std::string> DependencyTracker::GetDependents(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dependents_.find(asset);
    if (it != dependents_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

std::vector<std::string> DependencyTracker::GetDependencyChain(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> chain;
    std::unordered_set<std::string> visited;
    
    std::function<void(const std::string&)> traverse = [&](const std::string& current) {
        if (visited.count(current)) return;
        visited.insert(current);
        chain.push_back(current);
        
        auto it = dependencies_.find(current);
        if (it != dependencies_.end()) {
            for (const auto& dep : it->second) {
                traverse(dep);
            }
        }
    };
    
    traverse(asset);
    return chain;
}

bool DependencyTracker::HasCircularDependency(const std::string& asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> stack;
    return HasCircularDependencyRecursive(asset, visited, stack);
}

bool DependencyTracker::HasCircularDependencyRecursive(const std::string& asset,
                                                       std::unordered_set<std::string>& visited,
                                                       std::unordered_set<std::string>& stack) const {
    if (stack.count(asset)) return true;
    if (visited.count(asset)) return false;
    
    visited.insert(asset);
    stack.insert(asset);
    
    auto it = dependencies_.find(asset);
    if (it != dependencies_.end()) {
        for (const auto& dep : it->second) {
            if (HasCircularDependencyRecursive(dep, visited, stack)) {
                return true;
            }
        }
    }
    
    stack.erase(asset);
    return false;
}

std::vector<std::string> DependencyTracker::GetLoadOrder(const std::vector<std::string>& assets) const {
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    
    std::function<void(const std::string&)> visit = [&](const std::string& asset) {
        if (visited.count(asset)) return;
        visited.insert(asset);
        
        auto deps = GetDependencies(asset);
        for (const auto& dep : deps) {
            visit(dep);
        }
        
        order.push_back(asset);
    };
    
    for (const auto& asset : assets) {
        visit(asset);
    }
    
    return order;
}

void DependencyTracker::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    dependencies_.clear();
    dependents_.clear();
}

void DependencyTracker::ExportDependencyGraph(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "digraph AssetDependencies {\n";
    for (const auto& [asset, deps] : dependencies_) {
        for (const auto& dep : deps) {
            file << "  \"" << asset << "\" -> \"" << dep << "\";\n";
        }
    }
    file << "}\n";
}

// ============================================================================
// Hot Reload Manager Implementation
// ============================================================================

HotReloadManager& HotReloadManager::GetInstance() {
    static HotReloadManager instance;
    return instance;
}

void HotReloadManager::WatchAsset(const std::string& path) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    watched_files_[path] = GetFileModificationTime(path);
}

void HotReloadManager::UnwatchAsset(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_files_.erase(path);
}

void HotReloadManager::WatchDirectory(const std::string& directory, bool recursive) {
    if (!enabled_) return;
    
    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    WatchAsset(entry.path().string());
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    WatchAsset(entry.path().string());
                }
            }
        }
    } catch (...) {
        // Directory doesn't exist or access denied
    }
}

void HotReloadManager::RegisterReloadCallback(const std::string& asset, ReloadCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_[asset] = callback;
}

void HotReloadManager::Update() {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [path, last_time] : watched_files_) {
        auto current_time = GetFileModificationTime(path);
        if (current_time > last_time) {
            last_time = current_time;
            pending_reloads_.push_back(path);
            
            auto callback_it = callbacks_.find(path);
            if (callback_it != callbacks_.end()) {
                callback_it->second(path);
            }
        }
    }
}

size_t HotReloadManager::GetPendingReloads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_reloads_.size();
}

void HotReloadManager::FlushReloads() {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_reloads_.clear();
}

// ============================================================================
// Compression Manager Implementation
// ============================================================================

CompressionManager& CompressionManager::GetInstance() {
    static CompressionManager instance;
    return instance;
}

bool CompressionManager::CompressAsset(const std::string& input_path,
                                      const std::string& output_path,
                                      CompressionType type) {
    std::ifstream input(input_path, std::ios::binary);
    if (!input) return false;
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
    
    auto compressed = CompressData(data, type);
    
    std::ofstream output(output_path, std::ios::binary);
    if (!output) return false;
    
    output.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    return true;
}

bool CompressionManager::DecompressAsset(const std::string& input_path,
                                        const std::string& output_path) {
    // Placeholder - would use actual compression library
    return false;
}

std::vector<uint8_t> CompressionManager::CompressData(const std::vector<uint8_t>& data,
                                                     CompressionType type) {
    // Placeholder - would use actual compression algorithms
    return data;
}

std::vector<uint8_t> CompressionManager::DecompressData(const std::vector<uint8_t>& data,
                                                       CompressionType type) {
    // Placeholder - would use actual decompression algorithms
    return data;
}

CompressionType CompressionManager::GetOptimalCompression(AssetType type) const {
    switch (type) {
        case AssetType::Texture: return CompressionType::LZ4;
        case AssetType::Model: return CompressionType::Zlib;
        case AssetType::Audio: return CompressionType::None;
        case AssetType::Script: return CompressionType::Zlib;
        default: return CompressionType::Auto;
    }
}

float CompressionManager::GetCompressionRatio(const std::string& path) const {
    auto it = compression_cache_.find(path);
    if (it != compression_cache_.end()) {
        return 1.0f; // Would calculate actual ratio
    }
    return 1.0f;
}

// ============================================================================
// Version Manager Implementation
// ============================================================================

VersionManager& VersionManager::GetInstance() {
    static VersionManager instance;
    return instance;
}

void VersionManager::SetAssetVersion(const std::string& path, int version) {
    std::lock_guard<std::mutex> lock(mutex_);
    versions_[path] = version;
}

int VersionManager::GetAssetVersion(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = versions_.find(path);
    return (it != versions_.end()) ? it->second : 1;
}

bool VersionManager::IsVersionCompatible(const std::string& path, int required_version) const {
    return GetAssetVersion(path) >= required_version;
}

std::vector<std::string> VersionManager::GetChangelog(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = changelogs_.find(path);
    return (it != changelogs_.end()) ? it->second : std::vector<std::string>();
}

void VersionManager::AddChangelogEntry(const std::string& path, const std::string& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    changelogs_[path].push_back(entry);
}

void VersionManager::ExportVersionManifest(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "# Asset Version Manifest\n\n";
    for (const auto& [path, version] : versions_) {
        file << path << " = v" << version << "\n";
        
        auto changelog_it = changelogs_.find(path);
        if (changelog_it != changelogs_.end() && !changelog_it->second.empty()) {
            for (const auto& entry : changelog_it->second) {
                file << "  - " << entry << "\n";
            }
        }
        file << "\n";
    }
}

// ============================================================================
// Optimization Manager Implementation
// ============================================================================

OptimizationManager& OptimizationManager::GetInstance() {
    static OptimizationManager instance;
    return instance;
}

void OptimizationManager::OptimizeAsset(const std::string& path, Platform platform) {
    auto type = GetAssetTypeFromExtension(path);
    
    switch (type) {
        case AssetType::Texture:
            OptimizeTexture(path, platform);
            break;
        case AssetType::Model:
            OptimizeModel(path, platform);
            break;
        case AssetType::Audio:
            OptimizeAudio(path, platform);
            break;
        default:
            break;
    }
}

void OptimizationManager::OptimizeDirectory(const std::string& directory, Platform platform) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                OptimizeAsset(entry.path().string(), platform);
            }
        }
    } catch (...) {
        // Directory error
    }
}

bool OptimizationManager::CanOptimize(AssetType type) const {
    return type == AssetType::Texture || 
           type == AssetType::Model || 
           type == AssetType::Audio;
}

size_t OptimizationManager::EstimateOptimizationSavings(const std::string& path) const {
    size_t size = GetFileSize(path);
    auto type = GetAssetTypeFromExtension(path);
    
    switch (type) {
        case AssetType::Texture: return size / 4;
        case AssetType::Model: return size / 3;
        case AssetType::Audio: return size / 2;
        default: return 0;
    }
}

void OptimizationManager::OptimizeTexture(const std::string& path, Platform platform) {
    // Placeholder - would perform actual texture optimization
}

void OptimizationManager::OptimizeModel(const std::string& path, Platform platform) {
    // Placeholder - would perform actual model optimization
}

void OptimizationManager::OptimizeAudio(const std::string& path, Platform platform) {
    // Placeholder - would perform actual audio optimization
}

// ============================================================================
// Streaming Manager Implementation
// ============================================================================

StreamingManager& StreamingManager::GetInstance() {
    static StreamingManager instance;
    return instance;
}

void StreamingManager::MarkStreamable(const std::string& path, int priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_queue_.push_back({path, priority, false});
    std::sort(stream_queue_.begin(), stream_queue_.end(),
             [](const StreamInfo& a, const StreamInfo& b) {
                 return a.priority > b.priority;
             });
}

void StreamingManager::UnmarkStreamable(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_queue_.erase(
        std::remove_if(stream_queue_.begin(), stream_queue_.end(),
                      [&path](const StreamInfo& info) { return info.path == path; }),
        stream_queue_.end());
}

bool StreamingManager::IsStreamable(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(stream_queue_.begin(), stream_queue_.end(),
                      [&path](const StreamInfo& info) { return info.path == path; });
}

void StreamingManager::StreamAsset(const std::string& path) {
    // Placeholder - would perform actual asset streaming
}

void StreamingManager::UnstreamAsset(const std::string& path) {
    // Placeholder - would unload streamed asset
}

void StreamingManager::Update() {
    if (!streaming_enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& info : stream_queue_) {
        if (!info.loaded && current_memory_usage_ < memory_budget_) {
            StreamAsset(info.path);
            info.loaded = true;
        }
    }
}

// ============================================================================
// Cache Manager Implementation
// ============================================================================

CacheManager& CacheManager::GetInstance() {
    static CacheManager instance;
    return instance;
}

void CacheManager::CacheAsset(const std::string& path, const void* data, size_t size) {
    if (!cache_enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (current_cache_size_ + size > max_cache_size_ && !cache_.empty()) {
        auto oldest = std::min_element(cache_.begin(), cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.last_access < b.second.last_access;
            });
        current_cache_size_ -= oldest->second.data.size();
        cache_.erase(oldest);
    }
    
    CacheEntry entry;
    entry.data.resize(size);
    std::memcpy(entry.data.data(), data, size);
    entry.last_access = std::chrono::system_clock::now();
    
    cache_[path] = std::move(entry);
    current_cache_size_ += size;
}

bool CacheManager::GetCachedAsset(const std::string& path, void** data, size_t* size) {
    if (!cache_enabled_) {
        miss_count_++;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        it->second.last_access = std::chrono::system_clock::now();
        *data = it->second.data.data();
        *size = it->second.data.size();
        hit_count_++;
        return true;
    }
    
    miss_count_++;
    return false;
}

void CacheManager::InvalidateCache(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        current_cache_size_ -= it->second.data.size();
        cache_.erase(it);
    }
}

void CacheManager::ClearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    current_cache_size_ = 0;
}

CacheManager::CacheStats CacheManager::GetStats() const {
    CacheStats stats;
    stats.hit_count = hit_count_.load();
    stats.miss_count = miss_count_.load();
    stats.cache_size = current_cache_size_.load();
    
    size_t total = stats.hit_count + stats.miss_count;
    stats.hit_rate = total > 0 ? static_cast<float>(stats.hit_count) / total : 0.0f;
    
    return stats;
}

// ============================================================================
// Analytics Manager Implementation
// ============================================================================

AnalyticsManager& AnalyticsManager::GetInstance() {
    static AnalyticsManager instance;
    return instance;
}

void AnalyticsManager::RecordAssetLoad(const std::string& path, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& analytics = analytics_[path];
    analytics.asset_path = path;
    analytics.load_count++;
    analytics.total_load_time += duration;
    analytics.average_load_time = analytics.total_load_time / analytics.load_count;
    analytics.last_load_time = std::chrono::system_clock::now();
}

void AnalyticsManager::RecordAssetAccess(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    analytics_[path].access_count++;
}

void AnalyticsManager::RecordMemoryUsage(const std::string& path, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    analytics_[path].memory_usage = bytes;
}

AssetAnalytics AnalyticsManager::GetAnalytics(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = analytics_.find(path);
    return (it != analytics_.end()) ? it->second : AssetAnalytics{};
}

std::vector<AssetAnalytics> AnalyticsManager::GetTopAssets(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AssetAnalytics> sorted;
    
    for (const auto& [_, analytics] : analytics_) {
        sorted.push_back(analytics);
    }
    
    std::sort(sorted.begin(), sorted.end(),
             [](const AssetAnalytics& a, const AssetAnalytics& b) {
                 return a.access_count > b.access_count;
             });
    
    if (sorted.size() > count) {
        sorted.resize(count);
    }
    
    return sorted;
}

std::vector<std::string> AnalyticsManager::GetHotAssets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> hot_assets;
    
    for (const auto& [path, analytics] : analytics_) {
        if (analytics.is_hot_asset || analytics.access_count > 100) {
            hot_assets.push_back(path);
        }
    }
    
    return hot_assets;
}

void AnalyticsManager::ExportReport(const std::string& output_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "# Asset Analytics Report\n\n";
    file << "Total Assets Tracked: " << analytics_.size() << "\n\n";
    
    for (const auto& [path, analytics] : analytics_) {
        file << "## " << path << "\n";
        file << "- Load Count: " << analytics.load_count << "\n";
        file << "- Access Count: " << analytics.access_count << "\n";
        file << "- Average Load Time: " << analytics.average_load_time.count() << "ms\n";
        file << "- Memory Usage: " << (analytics.memory_usage / 1024) << " KB\n\n";
    }
}

void AnalyticsManager::ClearAnalytics() {
    std::lock_guard<std::mutex> lock(mutex_);
    analytics_.clear();
}

// ============================================================================
// Documentation Generator Implementation
// ============================================================================

DocumentationGenerator& DocumentationGenerator::GetInstance() {
    static DocumentationGenerator instance;
    return instance;
}

void DocumentationGenerator::GenerateDocumentation(const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "# Nova Engine Asset Pipeline Documentation\n\n";
    file << "Generated: " << std::time(nullptr) << "\n\n";
    
    auto& manager = AssetPipelineManager::GetInstance();
    auto status = manager.GetStatus();
    
    file << "## Pipeline Status\n\n";
    file << "- Total Assets: " << status.total_assets << "\n";
    file << "- Loaded Assets: " << status.loaded_assets << "\n";
    file << "- Failed Assets: " << status.failed_assets << "\n";
    file << "- Cached Assets: " << status.cached_assets << "\n";
    file << "- Memory Usage: " << (status.memory_usage / 1024 / 1024) << " MB\n\n";
    
    for (const auto& [title, content] : custom_sections_) {
        file << "## " << title << "\n\n";
        file << content << "\n\n";
    }
}

void DocumentationGenerator::GenerateAssetDoc(const std::string& asset_path, 
                                             const std::string& output_path) {
    auto* metadata = AssetPipelineManager::GetInstance().GetAssetMetadata(asset_path);
    if (!metadata) return;
    
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "# Asset: " << metadata->name << "\n\n";
    file << "**Path:** " << metadata->path << "\n";
    file << "**Type:** " << GetAssetTypeName(metadata->type) << "\n";
    file << "**Size:** " << (metadata->size_bytes / 1024) << " KB\n";
    file << "**Version:** " << metadata->version << "\n\n";
    
    if (!metadata->documentation.empty()) {
        file << "## Description\n\n";
        file << metadata->documentation << "\n\n";
    }
    
    auto deps = DependencyTracker::GetInstance().GetDependencies(asset_path);
    if (!deps.empty()) {
        file << "## Dependencies\n\n";
        for (const auto& dep : deps) {
            file << "- " << dep << "\n";
        }
        file << "\n";
    }
}

void DocumentationGenerator::AddCustomSection(const std::string& title, const std::string& content) {
    custom_sections_[title] = content;
}

// ============================================================================
// Asset Pipeline Manager Implementation
// ============================================================================

AssetPipelineManager& AssetPipelineManager::GetInstance() {
    static AssetPipelineManager instance;
    return instance;
}

bool AssetPipelineManager::Initialize(const std::string& asset_root) {
    asset_root_ = asset_root;
    DiscoverAssets(asset_root);
    return true;
}

void AssetPipelineManager::Shutdown() {
    GetCache().ClearCache();
    GetHotReload().FlushReloads();
    assets_.clear();
}

void AssetPipelineManager::RegisterAsset(const std::string& path, AssetType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AssetMetadata metadata;
    metadata.path = path;
    metadata.name = fs::path(path).filename().string();
    metadata.type = type;
    metadata.size_bytes = GetFileSize(path);
    metadata.checksum = CalculateChecksum(path);
    metadata.last_modified = GetFileModificationTime(path);
    
    assets_[path] = metadata;
}

void AssetPipelineManager::DiscoverAssets(const std::string& directory) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto path = entry.path().string();
                auto type = GetAssetTypeFromExtension(path);
                RegisterAsset(path, type);
            }
        }
    } catch (...) {
        // Directory error
    }
}

AssetMetadata* AssetPipelineManager::GetAssetMetadata(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = assets_.find(path);
    return (it != assets_.end()) ? &it->second : nullptr;
}

bool AssetPipelineManager::ValidateAllAssets() {
    std::lock_guard<std::mutex> lock(mutex_);
    bool all_valid = true;
    
    for (auto& [path, metadata] : assets_) {
        auto result = GetValidator().ValidateAsset(metadata);
        if (!result.is_valid) {
            all_valid = false;
        }
    }
    
    return all_valid;
}

void AssetPipelineManager::OptimizeAllAssets(Platform platform) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [path, _] : assets_) {
        GetOptimization().OptimizeAsset(path, platform);
    }
}

void AssetPipelineManager::GenerateAllDocumentation() {
    GetDocumentation().GenerateDocumentation("docs/asset_pipeline.md");
}

void AssetPipelineManager::Update() {
    GetHotReload().Update();
    GetStreaming().Update();
}

void AssetPipelineManager::FlushAll() {
    GetHotReload().FlushReloads();
    GetCache().ClearCache();
}

AssetPipelineManager::PipelineStatus AssetPipelineManager::GetStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PipelineStatus status;
    status.total_assets = assets_.size();
    status.cache_usage = GetCache().GetCacheUsage();
    status.memory_usage = GetStreaming().GetMemoryUsage();
    
    for (const auto& [_, metadata] : assets_) {
        if (metadata.state == AssetState::Loaded) {
            status.loaded_assets++;
        } else if (metadata.state == AssetState::Failed) {
            status.failed_assets++;
        }
        if (metadata.is_streaming) {
            status.streamed_assets++;
        }
    }
    
    return status;
}

void AssetPipelineManager::ExportStatusReport(const std::string& output_path) const {
    auto status = GetStatus();
    
    std::ofstream file(output_path);
    if (!file) return;
    
    file << "# Asset Pipeline Status Report\n\n";
    file << "## Summary\n\n";
    file << "- Total Assets: " << status.total_assets << "\n";
    file << "- Loaded: " << status.loaded_assets << "\n";
    file << "- Failed: " << status.failed_assets << "\n";
    file << "- Cached: " << status.cached_assets << "\n";
    file << "- Streamed: " << status.streamed_assets << "\n";
    file << "- Memory Usage: " << (status.memory_usage / 1024 / 1024) << " MB\n";
    file << "- Cache Usage: " << (status.cache_usage / 1024 / 1024) << " MB\n\n";
    
    auto cache_stats = GetCache().GetStats();
    file << "## Cache Statistics\n\n";
    file << "- Hit Rate: " << (cache_stats.hit_rate * 100) << "%\n";
    file << "- Hit Count: " << cache_stats.hit_count << "\n";
    file << "- Miss Count: " << cache_stats.miss_count << "\n\n";
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

AssetType GetAssetTypeFromExtension(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || 
        ext == ".svg" || ext == ".tga") {
        return AssetType::Texture;
    } else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
        return AssetType::Model;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::Audio;
    } else if (ext == ".lua" || ext == ".js" || ext == ".py") {
        return AssetType::Script;
    } else if (ext == ".json" || ext == ".xml" || ext == ".ini") {
        return AssetType::Config;
    } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag") {
        return AssetType::Shader;
    } else if (ext == ".mtl" || ext == ".mat") {
        return AssetType::Material;
    } else if (ext == ".ttf" || ext == ".otf") {
        return AssetType::Font;
    }
    
    return AssetType::Unknown;
}

std::string GetAssetTypeName(AssetType type) {
    switch (type) {
        case AssetType::Texture: return "Texture";
        case AssetType::Model: return "Model";
        case AssetType::Audio: return "Audio";
        case AssetType::Script: return "Script";
        case AssetType::Config: return "Config";
        case AssetType::Shader: return "Shader";
        case AssetType::Material: return "Material";
        case AssetType::Font: return "Font";
        case AssetType::Video: return "Video";
        case AssetType::Data: return "Data";
        default: return "Unknown";
    }
}

uint64_t CalculateChecksum(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;
    
    uint64_t checksum = 0;
    char byte;
    while (file.get(byte)) {
        checksum = checksum * 31 + static_cast<uint8_t>(byte);
    }
    
    return checksum;
}

bool FileExists(const std::string& path) {
    return fs::exists(path);
}

size_t GetFileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

std::chrono::system_clock::time_point GetFileModificationTime(const std::string& path) {
    try {
        auto ftime = fs::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return sctp;
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

} // namespace AssetPipeline
