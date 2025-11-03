#include "AssetStreamingSystem.h"
#include "AssetProcessingPipeline.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>

namespace assets {
    namespace streaming {

        // AssetStreamingSystem Implementation
        bool AssetStreamingSystem::Initialize(const MemoryConstraints& constraints) {
            if (initialized_.load()) {
                std::cout << "[AssetStreaming] System already initialized" << std::endl;
                return true;
            }

            std::cout << "[AssetStreaming] Initializing Asset Streaming System..." << std::endl;

            memoryConstraints_ = constraints;
            lastUpdateTime_ = std::chrono::steady_clock::now();

            // Start worker threads
            int numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
            std::cout << "[AssetStreaming] Starting " << numThreads << " worker threads" << std::endl;

            for (int i = 0; i < numThreads; ++i) {
                workerThreads_.emplace_back(&AssetStreamingSystem::WorkerThreadMain, this);
            }

            RegisterConsoleCommands();
            initialized_.store(true);

            std::cout << "[AssetStreaming] System initialized successfully" << std::endl;
            std::cout << "[AssetStreaming] Memory limits: " 
                      << streaming_utils::FormatMemorySize(memoryConstraints_.maxTotalMemory) << std::endl;

            return true;
        }

        void AssetStreamingSystem::Shutdown() {
            if (!initialized_.load()) {
                return;
            }

            std::cout << "[AssetStreaming] Shutting down Asset Streaming System..." << std::endl;

            shouldExit_.store(true);
            requestCV_.notify_all();

            // Wait for worker threads
            for (auto& thread : workerThreads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            // Unload all assets
            std::lock_guard<std::mutex> lock(assetsMutex_);
            for (auto& [id, asset] : assets_) {
                UnloadAssetInternal(asset);
            }
            assets_.clear();

            initialized_.store(false);
            std::cout << "[AssetStreaming] Shutdown complete" << std::endl;
        }

        void AssetStreamingSystem::Update(float deltaTime) {
            if (!initialized_.load()) return;

            auto currentTime = std::chrono::steady_clock::now();
            float frameTime = std::chrono::duration<float>(currentTime - lastUpdateTime_).count();
            lastUpdateTime_ = currentTime;

            // Update distance-based priorities
            UpdateDistanceBasedPriorities();

            // Check memory pressure
            CheckMemoryPressure();

            // Update metrics
            {
                std::lock_guard<std::mutex> lock(metricsMutex_);
                metrics_.frameLoadTime = frameTime;
                metrics_.frameLoadsStarted = 0;
                metrics_.frameLoadsCompleted = 0;
            }

            // Update asset states and progress
            std::lock_guard<std::mutex> lock(assetsMutex_);
            for (auto& [id, asset] : assets_) {
                // Check for completed async loads
                if (asset.state == LoadingState::Loading && 
                    asset.loadFuture.valid() && 
                    asset.loadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    
                    bool success = asset.loadFuture.get();
                    asset.state = success ? LoadingState::Loaded : LoadingState::Failed;
                    
                    if (success) {
                        asset.loadProgress = 1.0f;
                        std::lock_guard<std::mutex> metricsLock(metricsMutex_);
                        metrics_.totalLoads++;
                        metrics_.frameLoadsCompleted++;
                    } else {
                        std::lock_guard<std::mutex> metricsLock(metricsMutex_);
                        metrics_.loadFailures++;
                    }
                }

                // Adaptive LOD updates
                if (adaptiveLOD_.load()) {
                    LODLevel optimalLOD = CalculateOptimalLOD(id);
                    if (optimalLOD != asset.currentLOD && asset.state == LoadingState::Loaded) {
                        RequestLODChange(id, optimalLOD);
                    }
                }
            }

            UpdateMemoryStats();
        }

        bool AssetStreamingSystem::RegisterAsset(const std::string& assetId, const std::string& filePath, 
                                                MemoryCategory category, size_t estimatedSize) {
            std::lock_guard<std::mutex> lock(assetsMutex_);

            if (assets_.find(assetId) != assets_.end()) {
                std::cout << "[AssetStreaming] Asset already registered: " << assetId << std::endl;
                return false;
            }

            StreamingAssetRef asset;
            asset.assetId = assetId;
            asset.filePath = filePath;
            asset.category = category;
            asset.priority = StreamingPriority::Medium;
            asset.currentLOD = LODLevel::Medium;
            asset.state = LoadingState::Unloaded;
            asset.targetMemoryUsage = estimatedSize > 0 ? estimatedSize : 1024 * 1024; // 1MB default
            asset.lastAccessed = std::chrono::steady_clock::now();

            assets_[assetId] = std::move(asset);

            std::cout << "[AssetStreaming] Registered asset: " << assetId 
                      << " (" << streaming_utils::CategoryToString(category) << ")" << std::endl;

            return true;
        }

        void AssetStreamingSystem::UnregisterAsset(const std::string& assetId) {
            std::lock_guard<std::mutex> lock(assetsMutex_);

            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                UnloadAssetInternal(it->second);
                assets_.erase(it);
                std::cout << "[AssetStreaming] Unregistered asset: " << assetId << std::endl;
            }
        }

        std::future<bool> AssetStreamingSystem::RequestAsset(const std::string& assetId, 
                                                           StreamingPriority priority, LODLevel targetLOD) {
            auto promise = std::make_shared<std::promise<bool>>();
            auto future = promise->get_future();

            RequestAssetAsync(assetId, priority, [promise](bool success) {
                promise->set_value(success);
            });

            return future;
        }

        void AssetStreamingSystem::RequestAssetAsync(const std::string& assetId, StreamingPriority priority,
                                                   std::function<void(bool)> callback) {
            StreamingRequest request;
            request.assetId = assetId;
            request.priority = priority;
            request.targetLOD = LODLevel::Medium;
            request.callback = callback;

            {
                std::lock_guard<std::mutex> lock(requestMutex_);
                requestQueue_.push(request);
            }

            requestCV_.notify_one();

            std::lock_guard<std::mutex> lock(metricsMutex_);
            metrics_.frameLoadsStarted++;
        }

        std::shared_ptr<void> AssetStreamingSystem::GetAsset(const std::string& assetId) {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            
            auto* asset = FindAsset(assetId);
            if (!asset || asset->state != LoadingState::Loaded) {
                return nullptr;
            }

            // Update access tracking
            asset->lastAccessed = std::chrono::steady_clock::now();
            asset->accessCount++;

            std::lock_guard<std::mutex> dataLock(*asset->dataMutex);
            return asset->data;
        }

        bool AssetStreamingSystem::IsAssetLoaded(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            const auto* asset = FindAsset(assetId);
            return asset && asset->state == LoadingState::Loaded;
        }

        LoadingState AssetStreamingSystem::GetAssetState(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            const auto* asset = FindAsset(assetId);
            return asset ? asset->state : LoadingState::Unloaded;
        }

        float AssetStreamingSystem::GetLoadProgress(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            const auto* asset = FindAsset(assetId);
            return asset ? asset->loadProgress : 0.0f;
        }

        void AssetStreamingSystem::UpdateAssetDistance(const std::string& assetId, float distance) {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto* asset = FindAsset(assetId);
            if (asset) {
                asset->distance = distance;
                // Update priority based on distance
                asset->priority = CalculatePriorityFromDistance(distance);
            }
        }

        void AssetStreamingSystem::UpdateViewerPosition(float x, float y, float z) {
            std::lock_guard<std::mutex> lock(viewerMutex_);
            viewerPos_[0] = x;
            viewerPos_[1] = y;
            viewerPos_[2] = z;
        }

        LODLevel AssetStreamingSystem::CalculateOptimalLOD(const std::string& assetId) const {
            const auto* asset = FindAsset(assetId);
            if (!asset) return LODLevel::Medium;

            float distance = asset->distance;
            
            for (const auto& [lod, threshold] : lodConfig_.distanceThresholds) {
                if (distance <= threshold) {
                    return lod;
                }
            }

            return LODLevel::Lowest;
        }

        void AssetStreamingSystem::RequestLODChange(const std::string& assetId, LODLevel newLOD) {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto* asset = FindAsset(assetId);
            if (asset && asset->currentLOD != newLOD) {
                // Queue for reload with new LOD
                StreamingRequest request;
                request.assetId = assetId;
                request.priority = asset->priority;
                request.targetLOD = newLOD;

                {
                    std::lock_guard<std::mutex> reqLock(requestMutex_);
                    requestQueue_.push(request);
                }

                requestCV_.notify_one();
            }
        }

        MemoryStats AssetStreamingSystem::GetMemoryStats() const {
            MemoryStats stats;
            
            std::lock_guard<std::mutex> lock(assetsMutex_);
            for (const auto& [id, asset] : assets_) {
                if (asset.state == LoadingState::Loaded) {
                    stats.totalUsed += asset.memoryUsage;
                    stats.categoryUsage[asset.category] += asset.memoryUsage;
                    stats.loadedAssets++;
                } else if (asset.state == LoadingState::Loading) {
                    stats.loadingAssets++;
                }
            }

            stats.totalAvailable = memoryConstraints_.maxTotalMemory;
            stats.utilizationPercent = (float)stats.totalUsed / stats.totalAvailable * 100.0f;

            return stats;
        }

        void AssetStreamingSystem::ForceGarbageCollection() {
            std::cout << "[AssetStreaming] Performing garbage collection..." << std::endl;

            std::lock_guard<std::mutex> lock(assetsMutex_);
            
            // Collect assets that can be unloaded
            std::vector<std::string> candidates;
            auto currentTime = std::chrono::steady_clock::now();

            for (const auto& [id, asset] : assets_) {
                if (asset.state == LoadingState::Loaded && 
                    asset.priority >= StreamingPriority::Low) {
                    
                    auto timeSinceAccess = std::chrono::duration<float>(currentTime - asset.lastAccessed).count();
                    if (timeSinceAccess > 30.0f) { // 30 seconds threshold
                        candidates.push_back(id);
                    }
                }
            }

            // Sort by access time (oldest first)
            std::sort(candidates.begin(), candidates.end(), [&](const std::string& a, const std::string& b) {
                return assets_.at(a).lastAccessed < assets_.at(b).lastAccessed;
            });

            // Unload candidates
            int unloaded = 0;
            for (const auto& assetId : candidates) {
                auto& asset = assets_.at(assetId);
                UnloadAssetInternal(asset);
                unloaded++;

                // Check if we've freed enough memory
                if (GetMemoryStats().utilizationPercent < memoryPressureThreshold_.load() * 100.0f) {
                    break;
                }
            }

            std::cout << "[AssetStreaming] Garbage collection complete, unloaded " << unloaded << " assets" << std::endl;
        }

        void AssetStreamingSystem::PrintDebugInfo() const {
            std::cout << "\n=== Asset Streaming System Debug Info ===" << std::endl;
            
            auto stats = GetMemoryStats();
            std::cout << "Memory Usage: " << streaming_utils::FormatMemorySize(stats.totalUsed) 
                      << " / " << streaming_utils::FormatMemorySize(stats.totalAvailable)
                      << " (" << std::fixed << std::setprecision(1) << stats.utilizationPercent << "%)" << std::endl;

            std::cout << "Assets: " << stats.loadedAssets << " loaded, " << stats.loadingAssets << " loading" << std::endl;

            std::cout << "\nMemory by category:" << std::endl;
            for (const auto& [category, usage] : stats.categoryUsage) {
                std::cout << "  " << streaming_utils::CategoryToString(category) << ": " 
                          << streaming_utils::FormatMemorySize(usage) << std::endl;
            }

            std::cout << "\nMetrics:" << std::endl;
            auto metrics = GetMetrics();
            std::cout << "  Total loads: " << metrics.totalLoads << std::endl;
            std::cout << "  Load failures: " << metrics.loadFailures << std::endl;
            std::cout << "  Cache hits: " << metrics.cacheHits << std::endl;
            std::cout << "  Cache misses: " << metrics.cacheMisses << std::endl;

            std::cout << "==========================================\n" << std::endl;
        }

        // Private implementation methods

        StreamingAssetRef* AssetStreamingSystem::FindAsset(const std::string& assetId) {
            auto it = assets_.find(assetId);
            return it != assets_.end() ? &it->second : nullptr;
        }

        const StreamingAssetRef* AssetStreamingSystem::FindAsset(const std::string& assetId) const {
            auto it = assets_.find(assetId);
            return it != assets_.end() ? &it->second : nullptr;
        }

        bool AssetStreamingSystem::LoadAssetInternal(StreamingAssetRef& asset, LODLevel targetLOD) {
            std::cout << "[AssetStreaming] Loading asset: " << asset.assetId 
                      << " (LOD: " << streaming_utils::LODToString(targetLOD) << ")" << std::endl;

            asset.state = LoadingState::Loading;
            asset.loadStartTime = std::chrono::steady_clock::now();
            asset.loadProgress = 0.0f;

            // Load asset data (this would integrate with your actual asset loading system)
            auto data = LoadAssetData(asset.filePath, asset.category, targetLOD);
            
            if (data) {
            {
                std::lock_guard<std::mutex> lock(*asset.dataMutex);
                asset.data = data;
            }
                asset.currentLOD = targetLOD;
                asset.state = LoadingState::Loaded;
                asset.loadProgress = 1.0f;

                // Estimate memory usage (simplified)
                asset.memoryUsage = asset.targetMemoryUsage * lodConfig_.qualityScales.at(targetLOD);

                std::cout << "[AssetStreaming] Successfully loaded: " << asset.assetId << std::endl;
                return true;
            } else {
                asset.state = LoadingState::Failed;
                std::cout << "[AssetStreaming] Failed to load: " << asset.assetId << std::endl;
                return false;
            }
        }

        void AssetStreamingSystem::UnloadAssetInternal(StreamingAssetRef& asset) {
            if (asset.state == LoadingState::Unloaded) return;

            std::cout << "[AssetStreaming] Unloading asset: " << asset.assetId << std::endl;

            asset.state = LoadingState::Unloading;
            
            {
                std::lock_guard<std::mutex> lock(*asset.dataMutex);
                asset.data.reset();
            }

            asset.state = LoadingState::Unloaded;
            asset.loadProgress = 0.0f;
            asset.memoryUsage = 0;

            std::lock_guard<std::mutex> lock(metricsMutex_);
            metrics_.totalUnloads++;
        }

        std::shared_ptr<void> AssetStreamingSystem::LoadAssetData(const std::string& filePath, 
                                                                MemoryCategory category, LODLevel lod) {
            // This is a simplified implementation
            // In a real system, this would integrate with your asset loading pipeline
            
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                return nullptr;
            }

            auto fileSize = file.tellg();
            file.seekg(0);

            // Apply LOD scaling to file size
            float qualityScale = lodConfig_.qualityScales.at(lod);
            size_t loadSize = static_cast<size_t>(fileSize * qualityScale);

            auto buffer = std::make_shared<std::vector<uint8_t>>(loadSize);
            file.read(reinterpret_cast<char*>(buffer->data()), loadSize);

            return std::static_pointer_cast<void>(buffer);
        }

        void AssetStreamingSystem::CheckMemoryPressure() {
            // Calculate stats without additional locking to avoid deadlock
            MemoryStats stats;
            {
                std::lock_guard<std::mutex> lock(assetsMutex_);
                for (const auto& [id, asset] : assets_) {
                    if (asset.state == LoadingState::Loaded) {
                        stats.totalUsed += asset.memoryUsage;
                        stats.categoryUsage[asset.category] += asset.memoryUsage;
                        stats.loadedAssets++;
                    } else if (asset.state == LoadingState::Loading) {
                        stats.loadingAssets++;
                    }
                }
                stats.totalAvailable = memoryConstraints_.maxTotalMemory;
                stats.utilizationPercent = (float)stats.totalUsed / stats.totalAvailable * 100.0f;
                
                // Check thresholds while we have the lock to avoid race conditions
                if (stats.utilizationPercent > memoryConstraints_.emergencyThreshold) {
                    std::cout << "[AssetStreaming] Emergency memory cleanup triggered!" << std::endl;
                    // Unload all low priority assets (already have lock)
                    for (auto& [id, asset] : assets_) {
                        if (asset.state == LoadingState::Loaded && asset.priority >= StreamingPriority::Low) {
                            UnloadAssetInternal(asset);
                        }
                    }
                }
            }
            
            // Force garbage collection outside the lock if needed
            if (stats.utilizationPercent > memoryConstraints_.warningThreshold && 
                stats.utilizationPercent <= memoryConstraints_.emergencyThreshold) {
                ForceGarbageCollection();
            }
        }

        void AssetStreamingSystem::PerformEmergencyCleanup() {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            
            // Unload all low priority assets
            for (auto& [id, asset] : assets_) {
                if (asset.state == LoadingState::Loaded && asset.priority >= StreamingPriority::Low) {
                    UnloadAssetInternal(asset);
                }
            }
        }

        void AssetStreamingSystem::UpdateMemoryStats() {
            // Update peak memory usage
            auto stats = GetMemoryStats();
            
            std::lock_guard<std::mutex> lock(metricsMutex_);
            metrics_.peakMemoryUsage = std::max(metrics_.peakMemoryUsage, 
                                              (float)stats.totalUsed / (1024.0f * 1024.0f));
        }

        void AssetStreamingSystem::WorkerThreadMain() {
            while (!shouldExit_.load()) {
                std::unique_lock<std::mutex> lock(requestMutex_);
                requestCV_.wait(lock, [this] { return !requestQueue_.empty() || shouldExit_.load(); });

                if (shouldExit_.load()) break;

                if (requestQueue_.empty()) continue;

                auto request = requestQueue_.top();
                requestQueue_.pop();
                lock.unlock();

                // Process the request
                bool success = false;
                {
                    std::lock_guard<std::mutex> assetLock(assetsMutex_);
                    auto* asset = FindAsset(request.assetId);
                    if (asset) {
                        success = LoadAssetInternal(*asset, request.targetLOD);
                    }
                }

                // Call callback if provided
                if (request.callback) {
                    request.callback(success);
                }
            }
        }

        float AssetStreamingSystem::CalculateDistance(const std::string& assetId) const {
            const auto* asset = FindAsset(assetId);
            return asset ? asset->distance : std::numeric_limits<float>::max();
        }

        StreamingPriority AssetStreamingSystem::CalculatePriorityFromDistance(float distance) const {
            if (distance <= distanceConfig_.criticalDistance) return StreamingPriority::Critical;
            if (distance <= distanceConfig_.highDistance) return StreamingPriority::High;
            if (distance <= distanceConfig_.mediumDistance) return StreamingPriority::Medium;
            if (distance <= distanceConfig_.lowDistance) return StreamingPriority::Low;
            return StreamingPriority::Preload;
        }

        void AssetStreamingSystem::UpdateDistanceBasedPriorities() {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            for (auto& [id, asset] : assets_) {
                auto newPriority = CalculatePriorityFromDistance(asset.distance);
                if (newPriority != asset.priority) {
                    asset.priority = newPriority;
                    
                    // Auto-unload if too far
                    if (asset.distance > distanceConfig_.unloadDistance && 
                        asset.state == LoadingState::Loaded) {
                        UnloadAssetInternal(asset);
                    }
                }
            }
        }

        void AssetStreamingSystem::RegisterConsoleCommands() {
            StreamingConsoleCommands::RegisterCommands();
        }

        // Utility functions implementation
        namespace streaming_utils {
            std::string PriorityToString(StreamingPriority priority) {
                switch (priority) {
                    case StreamingPriority::Critical: return "Critical";
                    case StreamingPriority::High: return "High";
                    case StreamingPriority::Medium: return "Medium";
                    case StreamingPriority::Low: return "Low";
                    case StreamingPriority::Preload: return "Preload";
                    default: return "Unknown";
                }
            }

            std::string LODToString(LODLevel lod) {
                switch (lod) {
                    case LODLevel::Highest: return "Highest";
                    case LODLevel::High: return "High";
                    case LODLevel::Medium: return "Medium";
                    case LODLevel::Low: return "Low";
                    case LODLevel::Lowest: return "Lowest";
                    default: return "Unknown";
                }
            }

            std::string StateToString(LoadingState state) {
                switch (state) {
                    case LoadingState::Unloaded: return "Unloaded";
                    case LoadingState::Loading: return "Loading";
                    case LoadingState::Loaded: return "Loaded";
                    case LoadingState::Failed: return "Failed";
                    case LoadingState::Unloading: return "Unloading";
                    default: return "Unknown";
                }
            }

            std::string CategoryToString(MemoryCategory category) {
                switch (category) {
                    case MemoryCategory::Texture: return "Texture";
                    case MemoryCategory::Mesh: return "Mesh";
                    case MemoryCategory::Audio: return "Audio";
                    case MemoryCategory::Animation: return "Animation";
                    case MemoryCategory::Script: return "Script";
                    case MemoryCategory::Other: return "Other";
                    default: return "Unknown";
                }
            }

            std::string FormatMemorySize(size_t bytes) {
                const char* units[] = {"B", "KB", "MB", "GB"};
                int unit = 0;
                double size = static_cast<double>(bytes);

                while (size >= 1024.0 && unit < 3) {
                    size /= 1024.0;
                    unit++;
                }

                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
                return oss.str();
            }

            float CalculateMemoryPressure(size_t used, size_t available) {
                return available > 0 ? static_cast<float>(used) / available : 1.0f;
            }

            float CalculateDistance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
                float dx = x2 - x1;
                float dy = y2 - y1;
                float dz = z2 - z1;
                return std::sqrt(dx*dx + dy*dy + dz*dz);
            }

            void WarmupAssetCache(AssetStreamingSystem& system, const std::vector<std::string>& assetIds) {
                std::cout << "[AssetStreaming] Warming up cache with " << assetIds.size() << " assets..." << std::endl;
                
                for (const auto& assetId : assetIds) {
                    system.RequestAssetAsync(assetId, StreamingPriority::Preload, nullptr);
                }
            }

            void PreloadAssetsByDistance(AssetStreamingSystem& system, float maxDistance) {
                // This would need access to the internal asset list
                // Implementation would iterate through assets and preload those within distance
                std::cout << "[AssetStreaming] Preloading assets within " << maxDistance << " units..." << std::endl;
            }
        }

        // Console commands implementation
        void StreamingConsoleCommands::RegisterCommands() {
            std::cout << "[AssetStreaming] Console commands available:" << std::endl;
            std::cout << "  streaming.list - List all streaming assets" << std::endl;
            std::cout << "  streaming.load <assetId> - Load specific asset" << std::endl;
            std::cout << "  streaming.unload <assetId> - Unload specific asset" << std::endl;
            std::cout << "  streaming.stats - Show streaming statistics" << std::endl;
            std::cout << "  streaming.memory - Show memory usage" << std::endl;
            std::cout << "  streaming.lod <assetId> <level> - Change LOD level" << std::endl;
            std::cout << "  streaming.distance <assetId> <distance> - Set asset distance" << std::endl;
            std::cout << "  streaming.preload <distance> - Preload assets within distance" << std::endl;
            std::cout << "  streaming.config - Show current configuration" << std::endl;
        }

        void StreamingConsoleCommands::HandleStreamingStats(const std::vector<std::string>& args) {
            auto& system = AssetStreamingSystem::Instance();
            system.PrintDebugInfo();
        }

        // Additional console command implementations would go here...

    } // namespace streaming
} // namespace assets