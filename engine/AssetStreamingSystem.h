#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>
#include <future>

namespace assets {
    namespace streaming {

        // Forward declarations
        class StreamingAsset;
        class AssetStreamingSystem;

        // Streaming priority levels
        enum class StreamingPriority {
            Critical = 0,    // Must be loaded immediately (player view)
            High = 1,        // Should be loaded soon (nearby objects)
            Medium = 2,      // Load when convenient (medium distance)
            Low = 3,         // Load in background (far objects)
            Preload = 4      // Optional preload (distant objects)
        };

        // Level of Detail configurations
        enum class LODLevel {
            Highest = 0,     // Full quality
            High = 1,        // High quality
            Medium = 2,      // Medium quality  
            Low = 3,         // Low quality
            Lowest = 4       // Minimal quality
        };

        // Asset loading states
        enum class LoadingState {
            Unloaded,        // Not in memory
            Loading,         // Currently loading
            Loaded,          // Fully loaded
            Failed,          // Loading failed
            Unloading        // Currently unloading
        };

        // Memory usage categories
        enum class MemoryCategory {
            Texture,
            Mesh,
            Audio,
            Animation,
            Script,
            Other
        };

        // Distance-based loading configuration
        struct DistanceConfig {
            float criticalDistance = 10.0f;   // Must load within this distance
            float highDistance = 50.0f;       // High priority loading distance
            float mediumDistance = 200.0f;    // Medium priority loading distance
            float lowDistance = 500.0f;       // Low priority loading distance
            float preloadDistance = 1000.0f;  // Preload distance
            float unloadDistance = 1500.0f;   // Unload beyond this distance
        };

        // Memory constraints configuration
        struct MemoryConstraints {
            size_t maxTotalMemory = 512 * 1024 * 1024;      // 512MB default
            size_t maxTextureMemory = 256 * 1024 * 1024;    // 256MB for textures
            size_t maxMeshMemory = 128 * 1024 * 1024;       // 128MB for meshes
            size_t maxAudioMemory = 64 * 1024 * 1024;       // 64MB for audio
            size_t warningThreshold = 80;                   // Warn at 80% usage
            size_t emergencyThreshold = 95;                 // Emergency cleanup at 95%
        };

        // Progressive loading configuration
        struct ProgressiveConfig {
            bool enableProgressive = true;
            int initialChunkSize = 64 * 1024;     // 64KB initial chunks
            int maxChunkSize = 1024 * 1024;       // 1MB max chunks
            float chunkGrowthFactor = 1.5f;       // Chunk size growth
            int maxConcurrentLoads = 4;           // Max simultaneous loads
        };

        // LOD configuration
        struct LODConfig {
            std::unordered_map<LODLevel, float> distanceThresholds = {
                {LODLevel::Highest, 25.0f},
                {LODLevel::High, 75.0f},
                {LODLevel::Medium, 200.0f},
                {LODLevel::Low, 500.0f},
                {LODLevel::Lowest, 1000.0f}
            };
            
            std::unordered_map<LODLevel, float> qualityScales = {
                {LODLevel::Highest, 1.0f},
                {LODLevel::High, 0.75f},
                {LODLevel::Medium, 0.5f},
                {LODLevel::Low, 0.25f},
                {LODLevel::Lowest, 0.125f}
            };
        };

        // Asset reference with streaming metadata
        struct StreamingAssetRef {
            std::string assetId;
            std::string filePath;
            MemoryCategory category;
            StreamingPriority priority;
            LODLevel currentLOD;
            LoadingState state;
            
            float distance = 0.0f;              // Distance from viewer
            size_t memoryUsage = 0;              // Current memory usage
            size_t targetMemoryUsage = 0;        // Expected memory after loading
            
            std::chrono::steady_clock::time_point lastAccessed;
            std::chrono::steady_clock::time_point loadStartTime;
            
            int accessCount = 0;
            float loadProgress = 0.0f;           // 0.0 to 1.0
            
            // Runtime data
            std::shared_ptr<void> data;          // Actual asset data
            std::future<bool> loadFuture;        // Async loading future
            std::unique_ptr<std::mutex> dataMutex; // Thread safety for data access

            // Constructor
            StreamingAssetRef() : dataMutex(std::make_unique<std::mutex>()) {}
            
            // Move constructor
            StreamingAssetRef(StreamingAssetRef&& other) noexcept 
                : assetId(std::move(other.assetId))
                , filePath(std::move(other.filePath))
                , category(other.category)
                , priority(other.priority)
                , currentLOD(other.currentLOD)
                , state(other.state)
                , distance(other.distance)
                , memoryUsage(other.memoryUsage)
                , targetMemoryUsage(other.targetMemoryUsage)
                , lastAccessed(other.lastAccessed)
                , loadStartTime(other.loadStartTime)
                , accessCount(other.accessCount)
                , loadProgress(other.loadProgress)
                , data(std::move(other.data))
                , loadFuture(std::move(other.loadFuture))
                , dataMutex(std::move(other.dataMutex))
            {}
            
            // Move assignment operator
            StreamingAssetRef& operator=(StreamingAssetRef&& other) noexcept {
                if (this != &other) {
                    assetId = std::move(other.assetId);
                    filePath = std::move(other.filePath);
                    category = other.category;
                    priority = other.priority;
                    currentLOD = other.currentLOD;
                    state = other.state;
                    distance = other.distance;
                    memoryUsage = other.memoryUsage;
                    targetMemoryUsage = other.targetMemoryUsage;
                    lastAccessed = other.lastAccessed;
                    loadStartTime = other.loadStartTime;
                    accessCount = other.accessCount;
                    loadProgress = other.loadProgress;
                    data = std::move(other.data);
                    loadFuture = std::move(other.loadFuture);
                    dataMutex = std::move(other.dataMutex);
                }
                return *this;
            }
            
            // Delete copy constructor and assignment
            StreamingAssetRef(const StreamingAssetRef&) = delete;
            StreamingAssetRef& operator=(const StreamingAssetRef&) = delete;
        };

        // Streaming request
        struct StreamingRequest {
            std::string assetId;
            StreamingPriority priority;
            LODLevel targetLOD;
            std::function<void(bool)> callback;  // Success callback
            
            bool operator<(const StreamingRequest& other) const {
                return priority > other.priority; // Higher priority first
            }
        };

        // Memory statistics
        struct MemoryStats {
            size_t totalUsed = 0;
            size_t totalAvailable = 0;
            std::unordered_map<MemoryCategory, size_t> categoryUsage;
            float utilizationPercent = 0.0f;
            int loadedAssets = 0;
            int loadingAssets = 0;
        };

        // Performance metrics
        struct StreamingMetrics {
            int totalLoads = 0;
            int totalUnloads = 0;
            int loadFailures = 0;
            float averageLoadTime = 0.0f;
            float peakMemoryUsage = 0.0f;
            int cacheHits = 0;
            int cacheMisses = 0;
            
            // Per-frame metrics
            float frameLoadTime = 0.0f;
            int frameLoadsStarted = 0;
            int frameLoadsCompleted = 0;
        };

        // Asset Streaming System - main class
        class AssetStreamingSystem {
        public:
            static AssetStreamingSystem& Instance() {
                static AssetStreamingSystem instance;
                return instance;
            }

            // System lifecycle
            bool Initialize(const MemoryConstraints& constraints = {});
            void Shutdown();
            void Update(float deltaTime);

            // Configuration
            void SetDistanceConfig(const DistanceConfig& config) { distanceConfig_ = config; }
            void SetMemoryConstraints(const MemoryConstraints& constraints) { memoryConstraints_ = constraints; }
            void SetProgressiveConfig(const ProgressiveConfig& config) { progressiveConfig_ = config; }
            void SetLODConfig(const LODConfig& config) { lodConfig_ = config; }

            // Asset registration and management
            bool RegisterAsset(const std::string& assetId, const std::string& filePath, 
                             MemoryCategory category, size_t estimatedSize = 0);
            void UnregisterAsset(const std::string& assetId);
            
            // Streaming requests
            std::future<bool> RequestAsset(const std::string& assetId, StreamingPriority priority = StreamingPriority::Medium,
                                         LODLevel targetLOD = LODLevel::Medium);
            void RequestAssetAsync(const std::string& assetId, StreamingPriority priority,
                                 std::function<void(bool)> callback);
            void CancelRequest(const std::string& assetId);

            // Asset access
            std::shared_ptr<void> GetAsset(const std::string& assetId);
            bool IsAssetLoaded(const std::string& assetId) const;
            LoadingState GetAssetState(const std::string& assetId) const;
            float GetLoadProgress(const std::string& assetId) const;

            // Distance-based streaming
            void UpdateAssetDistance(const std::string& assetId, float distance);
            void UpdateViewerPosition(float x, float y, float z);
            void SetViewerPosition(float x, float y, float z) { viewerPos_[0] = x; viewerPos_[1] = y; viewerPos_[2] = z; }

            // LOD management
            LODLevel CalculateOptimalLOD(const std::string& assetId) const;
            void RequestLODChange(const std::string& assetId, LODLevel newLOD);
            void EnableAdaptiveLOD(bool enable) { adaptiveLOD_ = enable; }

            // Memory management
            void ForceGarbageCollection();
            bool UnloadLeastRecentlyUsed(MemoryCategory category = MemoryCategory::Other);
            void SetMemoryPressureThreshold(float threshold) { memoryPressureThreshold_ = threshold; }

            // Statistics and monitoring
            MemoryStats GetMemoryStats() const;
            StreamingMetrics GetMetrics() const { return metrics_; }
            void ResetMetrics();

            // Debug and profiling
            std::vector<std::string> GetLoadedAssets() const;
            std::vector<std::string> GetLoadingAssets() const;
            void PrintDebugInfo() const;

            // Console commands
            void RegisterConsoleCommands();

        private:
            AssetStreamingSystem() = default;
            ~AssetStreamingSystem() = default;
            AssetStreamingSystem(const AssetStreamingSystem&) = delete;
            AssetStreamingSystem& operator=(const AssetStreamingSystem&) = delete;

            // Internal asset management
            StreamingAssetRef* FindAsset(const std::string& assetId);
            const StreamingAssetRef* FindAsset(const std::string& assetId) const;
            
            // Loading/unloading implementation
            bool LoadAssetInternal(StreamingAssetRef& asset, LODLevel targetLOD);
            void UnloadAssetInternal(StreamingAssetRef& asset);
            std::shared_ptr<void> LoadAssetData(const std::string& filePath, MemoryCategory category, LODLevel lod);

            // Memory management
            void CheckMemoryPressure();
            void PerformEmergencyCleanup();
            size_t GetCategoryMemoryUsage(MemoryCategory category) const;
            void UpdateMemoryStats();

            // Worker thread management
            void WorkerThreadMain();
            void ProcessStreamingRequests();

            // Distance and LOD calculations
            float CalculateDistance(const std::string& assetId) const;
            StreamingPriority CalculatePriorityFromDistance(float distance) const;
            void UpdateDistanceBasedPriorities();

            // Configuration
            DistanceConfig distanceConfig_;
            MemoryConstraints memoryConstraints_;
            ProgressiveConfig progressiveConfig_;
            LODConfig lodConfig_;

            // Asset storage
            std::unordered_map<std::string, StreamingAssetRef> assets_;
            mutable std::mutex assetsMutex_;

            // Request queue
            std::priority_queue<StreamingRequest> requestQueue_;
            std::mutex requestMutex_;
            std::condition_variable requestCV_;

            // Worker threads
            std::vector<std::thread> workerThreads_;
            std::atomic<bool> shouldExit_{false};
            std::atomic<bool> initialized_{false};

            // Viewer position for distance calculations
            float viewerPos_[3] = {0.0f, 0.0f, 0.0f};
            std::mutex viewerMutex_;

            // Settings
            std::atomic<bool> adaptiveLOD_{true};
            std::atomic<float> memoryPressureThreshold_{0.8f};

            // Statistics
            mutable StreamingMetrics metrics_;
            mutable std::mutex metricsMutex_;

            // Frame timing
            std::chrono::steady_clock::time_point lastUpdateTime_;
        };

        // Utility functions
        namespace streaming_utils {
            // Convert enums to strings for debugging
            std::string PriorityToString(StreamingPriority priority);
            std::string LODToString(LODLevel lod);
            std::string StateToString(LoadingState state);
            std::string CategoryToString(MemoryCategory category);

            // Memory utilities
            std::string FormatMemorySize(size_t bytes);
            float CalculateMemoryPressure(size_t used, size_t available);

            // Distance utilities
            float CalculateDistance3D(float x1, float y1, float z1, float x2, float y2, float z2);
            
            // Performance utilities
            void WarmupAssetCache(AssetStreamingSystem& system, const std::vector<std::string>& assetIds);
            void PreloadAssetsByDistance(AssetStreamingSystem& system, float maxDistance);
        }

        // Streaming asset handle for easy access
        class StreamingAssetHandle {
        public:
            StreamingAssetHandle(const std::string& assetId) : assetId_(assetId) {}

            std::shared_ptr<void> Get() const {
                return AssetStreamingSystem::Instance().GetAsset(assetId_);
            }

            bool IsLoaded() const {
                return AssetStreamingSystem::Instance().IsAssetLoaded(assetId_);
            }

            LoadingState GetState() const {
                return AssetStreamingSystem::Instance().GetAssetState(assetId_);
            }

            float GetProgress() const {
                return AssetStreamingSystem::Instance().GetLoadProgress(assetId_);
            }

            void UpdateDistance(float distance) {
                AssetStreamingSystem::Instance().UpdateAssetDistance(assetId_, distance);
            }

            std::future<bool> Request(StreamingPriority priority = StreamingPriority::Medium) {
                return AssetStreamingSystem::Instance().RequestAsset(assetId_, priority);
            }

        private:
            std::string assetId_;
        };

        // Console commands for streaming system
        class StreamingConsoleCommands {
        public:
            static void RegisterCommands();
            static void HandleStreamingList(const std::vector<std::string>& args);
            static void HandleStreamingLoad(const std::vector<std::string>& args);
            static void HandleStreamingUnload(const std::vector<std::string>& args);
            static void HandleStreamingStats(const std::vector<std::string>& args);
            static void HandleStreamingLOD(const std::vector<std::string>& args);
            static void HandleStreamingMemory(const std::vector<std::string>& args);
            static void HandleStreamingDistance(const std::vector<std::string>& args);
            static void HandleStreamingPreload(const std::vector<std::string>& args);
            static void HandleStreamingConfig(const std::vector<std::string>& args);
        };

    } // namespace streaming
} // namespace assets