#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <functional>

namespace Nova {

// Asset streaming priority
enum class StreamPriority {
    Critical = 0,  // Must load immediately
    High = 1,      // Load ASAP
    Normal = 2,    // Load when convenient
    Low = 3,       // Load in background
    Prefetch = 4   // Load if bandwidth available
};

// Asset streaming state
enum class StreamState {
    Unloaded,
    Queued,
    Loading,
    Loaded,
    Failed
};

// Asset type for streaming
enum class AssetType {
    Texture,
    Mesh,
    Audio,
    Shader,
    Material,
    Animation,
    Other
};

// Streaming request
struct StreamRequest {
    std::string assetPath;
    AssetType type;
    StreamPriority priority;
    std::function<void(bool)> callback;
    float distanceFromCamera;
    size_t estimatedSize;
    double requestTime;
    int retryCount = 0;
};

// Streaming statistics
struct StreamStats {
    size_t totalRequests = 0;
    size_t completedRequests = 0;
    size_t failedRequests = 0;
    size_t activeStreams = 0;
    size_t queuedRequests = 0;
    double avgLoadTimeMs = 0.0;
    size_t totalBytesStreamed = 0;
    double currentBandwidthMBps = 0.0;
};

// LOD streaming level
struct LODStreamLevel {
    int level;
    float distance;
    size_t memorySize;
    bool loaded;
};

class AssetStreamer {
public:
    static AssetStreamer& GetInstance() {
        static AssetStreamer instance;
        return instance;
    }

    // === INITIALIZATION ===
    void Initialize(int numThreads = 2);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }
    
    // === STREAMING REQUESTS ===
    void RequestAsset(const std::string& assetPath, AssetType type,
                     StreamPriority priority = StreamPriority::Normal,
                     std::function<void(bool)> callback = nullptr);
    void RequestAssets(const std::vector<std::string>& assetPaths, AssetType type,
                      StreamPriority priority = StreamPriority::Normal);
    void CancelRequest(const std::string& assetPath);
    void ClearQueue();
    
    // === PRIORITY MANAGEMENT ===
    void SetPriority(const std::string& assetPath, StreamPriority priority);
    void UpdatePriorities(const std::vector<std::string>& visibleAssets);
    void BoostPriority(const std::string& assetPath);
    
    // === DISTANCE-BASED STREAMING ===
    void UpdateCameraPosition(float x, float y, float z);
    void SetStreamingDistance(float distance) { streamingDistance_ = distance; }
    float GetStreamingDistance() const { return streamingDistance_; }
    void RegisterAssetPosition(const std::string& assetPath, float x, float y, float z);
    void UpdateDistanceBasedPriorities();
    
    // === LOD STREAMING ===
    void EnableLODStreaming(bool enable) { lodStreamingEnabled_ = enable; }
    bool IsLODStreamingEnabled() const { return lodStreamingEnabled_; }
    void RequestLODLevel(const std::string& assetPath, int level);
    void UpdateLODLevels(float cameraDistance);
    int SelectOptimalLOD(const std::string& assetPath, float distance);
    
    // === BANDWIDTH MANAGEMENT ===
    void SetBandwidthLimit(size_t bytesPerSecond);
    size_t GetBandwidthLimit() const { return bandwidthLimit_; }
    void EnableBandwidthThrottling(bool enable) { bandwidthThrottlingEnabled_ = enable; }
    double GetCurrentBandwidth() const;
    void PauseBandwidthIntensiveStreaming(bool pause);
    
    // === MEMORY MANAGEMENT ===
    void SetMemoryBudget(size_t bytes);
    size_t GetMemoryBudget() const { return memoryBudget_; }
    size_t GetCurrentMemoryUsage() const;
    bool IsWithinMemoryBudget() const;
    void UnloadDistantAssets(float distance);
    void UnloadLeastRecentlyUsed(size_t targetBytes);
    
    // === PREFETCHING ===
    void PrefetchArea(float x, float y, float z, float radius);
    void PrefetchAssets(const std::vector<std::string>& assetPaths);
    void EnablePredictiveLoading(bool enable) { predictiveLoadingEnabled_ = enable; }
    void RegisterMovementVector(float vx, float vy, float vz);
    
    // === STATE QUERIES ===
    StreamState GetAssetState(const std::string& assetPath) const;
    bool IsAssetLoaded(const std::string& assetPath) const;
    bool IsAssetQueued(const std::string& assetPath) const;
    int GetQueuePosition(const std::string& assetPath) const;
    float GetLoadProgress(const std::string& assetPath) const;
    
    // === STATISTICS ===
    StreamStats GetStatistics() const;
    void ResetStatistics();
    std::vector<std::string> GetActiveStreams() const;
    std::vector<std::string> GetQueuedAssets() const;
    size_t GetQueueSize() const;
    
    // === CONFIGURATION ===
    void SetMaxConcurrentStreams(int count) { maxConcurrentStreams_ = count; }
    int GetMaxConcurrentStreams() const { return maxConcurrentStreams_; }
    void SetRetryAttempts(int count) { maxRetryAttempts_ = count; }
    void SetTimeout(double seconds) { streamTimeout_ = seconds; }
    
    // === CALLBACKS ===
    void SetGlobalLoadCallback(std::function<void(const std::string&, bool)> callback);
    void SetProgressCallback(std::function<void(const std::string&, float)> callback);
    void SetErrorCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    // === DIAGNOSTICS ===
    void DumpStreamingReport(const std::string& outputPath);
    std::string GetStatusReport() const;
    void EnableDebugLogging(bool enable) { debugLogging_ = enable; }

private:
    AssetStreamer() = default;
    ~AssetStreamer();
    
    AssetStreamer(const AssetStreamer&) = delete;
    AssetStreamer& operator=(const AssetStreamer&) = delete;

    // Worker thread functions
    void StreamingWorker();
    void ProcessRequest(const StreamRequest& request);
    void UpdateBandwidthStats();
    
    // Helper functions
    bool SimulateAssetLoad(const StreamRequest& request);
    double GetCurrentTime() const;
    
    // Priority queue comparator
    struct RequestComparator {
        bool operator()(const StreamRequest& a, const StreamRequest& b) const {
            if (a.priority != b.priority)
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            if (a.distanceFromCamera != b.distanceFromCamera)
                return a.distanceFromCamera > b.distanceFromCamera;
            return a.requestTime > b.requestTime;
        }
    };

    // Internal state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdownRequested_{false};
    std::vector<std::thread> workerThreads_;
    
    std::priority_queue<StreamRequest, std::vector<StreamRequest>, RequestComparator> requestQueue_;
    std::mutex queueMutex_;
    std::mutex stateMutex_;
    std::condition_variable queueCondition_;
    
    std::unordered_map<std::string, StreamState> assetStates_;
    std::unordered_map<std::string, float> loadProgress_;
    std::unordered_map<std::string, std::array<float, 3>> assetPositions_;
    std::unordered_map<std::string, std::vector<LODStreamLevel>> lodLevels_;
    
    StreamStats stats_;
    std::array<float, 3> cameraPosition_{0, 0, 0};
    std::array<float, 3> movementVector_{0, 0, 0};
    
    // Configuration
    int maxConcurrentStreams_ = 4;
    int maxRetryAttempts_ = 3;
    double streamTimeout_ = 30.0;
    size_t bandwidthLimit_ = 50 * 1024 * 1024;  // 50 MB/s
    size_t memoryBudget_ = 512 * 1024 * 1024;   // 512 MB
    float streamingDistance_ = 500.0f;
    
    bool bandwidthThrottlingEnabled_ = true;
    bool lodStreamingEnabled_ = true;
    bool predictiveLoadingEnabled_ = true;
    bool debugLogging_ = false;
    
    // Callbacks
    std::function<void(const std::string&, bool)> globalLoadCallback_;
    std::function<void(const std::string&, float)> progressCallback_;
    std::function<void(const std::string&, const std::string&)> errorCallback_;
    
    // Bandwidth tracking
    std::vector<std::pair<double, size_t>> bandwidthHistory_;  // (time, bytes)
    std::mutex bandwidthMutex_;
};

} // namespace Nova
