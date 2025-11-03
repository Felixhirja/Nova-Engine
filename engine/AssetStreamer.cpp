#include "AssetStreamer.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace Nova {

// === INITIALIZATION ===

void AssetStreamer::Initialize(int numThreads) {
    if (initialized_) {
        std::cout << "AssetStreamer already initialized\n";
        return;
    }
    
    maxConcurrentStreams_ = numThreads;
    shutdownRequested_ = false;
    initialized_ = true;
    
    // Create worker threads
    for (int i = 0; i < numThreads; i++) {
        workerThreads_.push_back(std::thread(&AssetStreamer::StreamingWorker, this));
    }
    
    std::cout << "AssetStreamer initialized with " << numThreads << " worker threads\n";
}

AssetStreamer::~AssetStreamer() {
    Shutdown();
}

void AssetStreamer::Shutdown() {
    if (!initialized_) return;
    
    shutdownRequested_ = true;
    
    // Wake up all threads
    queueCondition_.notify_all();
    
    // Wait for threads to finish
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads_.clear();
    initialized_ = false;
    
    std::cout << "AssetStreamer shut down\n";
}

// === STREAMING REQUESTS ===

void AssetStreamer::RequestAsset(const std::string& assetPath, AssetType type,
                                 StreamPriority priority, std::function<void(bool)> callback) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // Check if already loaded or queued
    auto stateIt = assetStates_.find(assetPath);
    if (stateIt != assetStates_.end()) {
        if (stateIt->second == StreamState::Loaded) {
            if (callback) callback(true);
            return;
        }
        if (stateIt->second == StreamState::Loading || stateIt->second == StreamState::Queued) {
            return; // Already processing
        }
    }
    
    StreamRequest request;
    request.assetPath = assetPath;
    request.type = type;
    request.priority = priority;
    request.callback = callback;
    request.requestTime = GetCurrentTime();
    request.estimatedSize = 1024 * 1024; // Default 1MB
    
    // Calculate distance if position is registered
    auto posIt = assetPositions_.find(assetPath);
    if (posIt != assetPositions_.end()) {
        const auto& pos = posIt->second;
        float dx = pos[0] - cameraPosition_[0];
        float dy = pos[1] - cameraPosition_[1];
        float dz = pos[2] - cameraPosition_[2];
        request.distanceFromCamera = std::sqrt(dx*dx + dy*dy + dz*dz);
    } else {
        request.distanceFromCamera = 0.0f;
    }
    
    requestQueue_.push(request);
    assetStates_[assetPath] = StreamState::Queued;
    stats_.totalRequests++;
    stats_.queuedRequests++;
    
    queueCondition_.notify_one();
}

void AssetStreamer::RequestAssets(const std::vector<std::string>& assetPaths, AssetType type,
                                  StreamPriority priority) {
    for (const auto& path : assetPaths) {
        RequestAsset(path, type, priority);
    }
}

void AssetStreamer::CancelRequest(const std::string& assetPath) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    assetStates_[assetPath] = StreamState::Unloaded;
    
    // Note: Can't easily remove from priority queue, will skip in worker
    std::cout << "Cancelled streaming request: " << assetPath << "\n";
}

void AssetStreamer::ClearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // Clear the queue
    std::priority_queue<StreamRequest, std::vector<StreamRequest>, RequestComparator> emptyQueue;
    requestQueue_.swap(emptyQueue);
    
    // Reset states
    for (auto& [path, state] : assetStates_) {
        if (state == StreamState::Queued) {
            state = StreamState::Unloaded;
        }
    }
    
    stats_.queuedRequests = 0;
    
    std::cout << "Streaming queue cleared\n";
}

// === PRIORITY MANAGEMENT ===

void AssetStreamer::SetPriority(const std::string& assetPath, StreamPriority priority) {
    // In a real implementation, this would update priority in the queue
    std::cout << "Updated priority for " << assetPath << " to " << static_cast<int>(priority) << "\n";
}

void AssetStreamer::UpdatePriorities(const std::vector<std::string>& visibleAssets) {
    for (const auto& asset : visibleAssets) {
        SetPriority(asset, StreamPriority::High);
    }
}

void AssetStreamer::BoostPriority(const std::string& assetPath) {
    SetPriority(assetPath, StreamPriority::Critical);
}

// === DISTANCE-BASED STREAMING ===

void AssetStreamer::UpdateCameraPosition(float x, float y, float z) {
    cameraPosition_ = {x, y, z};
    UpdateDistanceBasedPriorities();
}

void AssetStreamer::RegisterAssetPosition(const std::string& assetPath, float x, float y, float z) {
    assetPositions_[assetPath] = {x, y, z};
}

void AssetStreamer::UpdateDistanceBasedPriorities() {
    std::vector<std::pair<std::string, float>> distances;
    
    for (const auto& [path, pos] : assetPositions_) {
        float dx = pos[0] - cameraPosition_[0];
        float dy = pos[1] - cameraPosition_[1];
        float dz = pos[2] - cameraPosition_[2];
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance <= streamingDistance_) {
            distances.push_back({path, distance});
        }
    }
    
    // Sort by distance
    std::sort(distances.begin(), distances.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Assign priorities based on distance
    for (size_t i = 0; i < distances.size(); i++) {
        const auto& [path, distance] = distances[i];
        
        auto stateIt = assetStates_.find(path);
        if (stateIt == assetStates_.end() || stateIt->second == StreamState::Unloaded) {
            StreamPriority priority = StreamPriority::Low;
            
            if (distance < streamingDistance_ * 0.3f) {
                priority = StreamPriority::High;
            } else if (distance < streamingDistance_ * 0.6f) {
                priority = StreamPriority::Normal;
            }
            
            RequestAsset(path, AssetType::Other, priority);
        }
    }
}

// === LOD STREAMING ===

void AssetStreamer::RequestLODLevel(const std::string& assetPath, int level) {
    std::string lodPath = assetPath + ".lod" + std::to_string(level);
    RequestAsset(lodPath, AssetType::Mesh, StreamPriority::Normal);
}

void AssetStreamer::UpdateLODLevels(float cameraDistance) {
    for (const auto& [path, levels] : lodLevels_) {
        int optimalLevel = SelectOptimalLOD(path, cameraDistance);
        RequestLODLevel(path, optimalLevel);
    }
}

int AssetStreamer::SelectOptimalLOD(const std::string& assetPath, float distance) {
    auto it = lodLevels_.find(assetPath);
    if (it == lodLevels_.end()) {
        return 0;
    }
    
    const auto& levels = it->second;
    for (size_t i = 0; i < levels.size(); i++) {
        if (distance < levels[i].distance) {
            return levels[i].level;
        }
    }
    
    return levels.empty() ? 0 : levels.back().level;
}

// === BANDWIDTH MANAGEMENT ===

void AssetStreamer::SetBandwidthLimit(size_t bytesPerSecond) {
    bandwidthLimit_ = bytesPerSecond;
    std::cout << "Bandwidth limit set to " << (bytesPerSecond / (1024*1024)) << " MB/s\n";
}

double AssetStreamer::GetCurrentBandwidth() const {
    return stats_.currentBandwidthMBps;
}

void AssetStreamer::PauseBandwidthIntensiveStreaming(bool pause) {
    bandwidthThrottlingEnabled_ = !pause;
    std::cout << "Bandwidth intensive streaming " << (pause ? "paused" : "resumed") << "\n";
}

// === PREFETCHING ===

void AssetStreamer::PrefetchAssets(const std::vector<std::string>& assetPaths) {
    for (const auto& path : assetPaths) {
        RequestAsset(path, AssetType::Other, StreamPriority::Prefetch);
    }
}

void AssetStreamer::PrefetchArea(float x, float y, float z, float radius) {
    std::vector<std::string> nearbyAssets;
    
    for (const auto& [path, pos] : assetPositions_) {
        float dx = pos[0] - x;
        float dy = pos[1] - y;
        float dz = pos[2] - z;
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance <= radius) {
            nearbyAssets.push_back(path);
        }
    }
    
    PrefetchAssets(nearbyAssets);
}

void AssetStreamer::RegisterMovementVector(float vx, float vy, float vz) {
    movementVector_ = {vx, vy, vz};
}

// === MEMORY MANAGEMENT ===

void AssetStreamer::SetMemoryBudget(size_t bytes) {
    memoryBudget_ = bytes;
    std::cout << "Streaming memory budget set to " << (bytes / (1024*1024)) << " MB\n";
}

size_t AssetStreamer::GetCurrentMemoryUsage() const {
    size_t total = 0;
    for (const auto& [path, state] : assetStates_) {
        if (state == StreamState::Loaded) {
            total += 1024 * 1024; // Placeholder estimate
        }
    }
    return total;
}

bool AssetStreamer::IsWithinMemoryBudget() const {
    return GetCurrentMemoryUsage() <= memoryBudget_;
}

void AssetStreamer::UnloadLeastRecentlyUsed(size_t targetBytes) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    size_t freedBytes = 0;
    std::vector<std::string> toUnload;
    
    for (const auto& [path, state] : assetStates_) {
        if (state == StreamState::Loaded && freedBytes < targetBytes) {
            toUnload.push_back(path);
            freedBytes += 1024 * 1024; // Placeholder estimate
        }
    }
    
    for (const auto& path : toUnload) {
        assetStates_[path] = StreamState::Unloaded;
        std::cout << "Unloaded LRU asset: " << path << "\n";
    }
}

void AssetStreamer::UnloadDistantAssets(float distance) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    std::vector<std::string> toUnload;
    
    for (const auto& [path, pos] : assetPositions_) {
        float dx = pos[0] - cameraPosition_[0];
        float dy = pos[1] - cameraPosition_[1];
        float dz = pos[2] - cameraPosition_[2];
        float assetDistance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (assetDistance > distance) {
            auto stateIt = assetStates_.find(path);
            if (stateIt != assetStates_.end() && stateIt->second == StreamState::Loaded) {
                toUnload.push_back(path);
            }
        }
    }
    
    for (const auto& path : toUnload) {
        assetStates_[path] = StreamState::Unloaded;
        std::cout << "Unloaded distant asset: " << path << "\n";
    }
}

// === STATISTICS ===

StreamStats AssetStreamer::GetStatistics() const {
    return stats_;
}

void AssetStreamer::ResetStatistics() {
    stats_ = StreamStats();
}

StreamState AssetStreamer::GetAssetState(const std::string& assetPath) const {
    auto it = assetStates_.find(assetPath);
    if (it != assetStates_.end()) {
        return it->second;
    }
    return StreamState::Unloaded;
}

bool AssetStreamer::IsAssetLoaded(const std::string& assetPath) const {
    return GetAssetState(assetPath) == StreamState::Loaded;
}

bool AssetStreamer::IsAssetQueued(const std::string& assetPath) const {
    auto state = GetAssetState(assetPath);
    return state == StreamState::Queued || state == StreamState::Loading;
}

int AssetStreamer::GetQueuePosition(const std::string& assetPath) const {
    // Simple implementation - would need queue traversal for accurate position
    return IsAssetQueued(assetPath) ? 0 : -1;
}

std::vector<std::string> AssetStreamer::GetActiveStreams() const {
    std::vector<std::string> active;
    for (const auto& [path, state] : assetStates_) {
        if (state == StreamState::Loading) {
            active.push_back(path);
        }
    }
    return active;
}

std::vector<std::string> AssetStreamer::GetQueuedAssets() const {
    std::vector<std::string> queued;
    for (const auto& [path, state] : assetStates_) {
        if (state == StreamState::Queued) {
            queued.push_back(path);
        }
    }
    return queued;
}

// === WORKER THREAD ===

void AssetStreamer::StreamingWorker() {
    std::cout << "Worker thread started\n";
    
    while (!shutdownRequested_) {
        StreamRequest request;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for work
            queueCondition_.wait(lock, [this] {
                return shutdownRequested_ || !requestQueue_.empty();
            });
            
            if (shutdownRequested_) break;
            
            if (requestQueue_.empty()) continue;
            
            request = requestQueue_.top();
            requestQueue_.pop();
            stats_.queuedRequests--;
        }
        
        // Check if request was cancelled
        auto stateIt = assetStates_.find(request.assetPath);
        if (stateIt == assetStates_.end() || stateIt->second != StreamState::Queued) {
            continue;
        }
        
        // Process request
        ProcessRequest(request);
    }
    
    std::cout << "Worker thread stopped\n";
}

void AssetStreamer::ProcessRequest(const StreamRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        assetStates_[request.assetPath] = StreamState::Loading;
        stats_.activeStreams++;
    }
    
    std::cout << "Loading asset: " << request.assetPath 
              << " (Priority: " << static_cast<int>(request.priority) << ")\n";
    
    // Simulate loading
    bool success = SimulateAssetLoad(request);
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (success) {
            assetStates_[request.assetPath] = StreamState::Loaded;
            stats_.completedRequests++;
            stats_.totalBytesStreamed += request.estimatedSize;
        } else {
            assetStates_[request.assetPath] = StreamState::Failed;
            stats_.failedRequests++;
        }
        
        stats_.activeStreams--;
        stats_.avgLoadTimeMs = (stats_.avgLoadTimeMs * (stats_.completedRequests - 1) + duration.count()) / 
                               stats_.completedRequests;
    }
    
    // Call callback if provided
    if (request.callback) {
        request.callback(success);
    }
    
    // Check memory budget
    if (!IsWithinMemoryBudget()) {
        UnloadDistantAssets(streamingDistance_ * 0.5f);
    }
}

bool AssetStreamer::SimulateAssetLoad(const StreamRequest& request) {
    // In a real implementation, this would actually load the asset
    // For now, just simulate the time it takes
    
    // Bandwidth throttling
    if (bandwidthThrottlingEnabled_ && bandwidthLimit_ > 0) {
        double loadTimeMs = (request.estimatedSize / static_cast<double>(bandwidthLimit_)) * 1000.0;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(loadTimeMs)));
    }
    
    return true; // Success
}

// === UTILITY ===

double AssetStreamer::GetCurrentTime() const {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void AssetStreamer::UpdateBandwidthStats() {
    // Calculate current bandwidth usage
    double now = GetCurrentTime();
    
    // Clean old history
    bandwidthHistory_.erase(
        std::remove_if(bandwidthHistory_.begin(), bandwidthHistory_.end(),
            [now](const auto& entry) { return now - entry.first > 1.0; }),
        bandwidthHistory_.end()
    );
    
    // Calculate bandwidth
    size_t totalBytes = 0;
    for (const auto& [time, bytes] : bandwidthHistory_) {
        totalBytes += bytes;
    }
    
    stats_.currentBandwidthMBps = totalBytes / (1024.0 * 1024.0);
}

std::string AssetStreamer::GetStatusReport() const {
    std::string report = "Asset Streaming Status:\n";
    report += "  Total Requests: " + std::to_string(stats_.totalRequests) + "\n";
    report += "  Completed: " + std::to_string(stats_.completedRequests) + "\n";
    report += "  Failed: " + std::to_string(stats_.failedRequests) + "\n";
    report += "  Active: " + std::to_string(stats_.activeStreams) + "\n";
    report += "  Queued: " + std::to_string(stats_.queuedRequests) + "\n";
    report += "  Avg Load Time: " + std::to_string(stats_.avgLoadTimeMs) + "ms\n";
    report += "  Bandwidth: " + std::to_string(stats_.currentBandwidthMBps) + " MB/s\n";
    return report;
}

void AssetStreamer::DumpStreamingReport(const std::string& outputPath) {
    std::cout << GetStatusReport() << std::endl;
}

void AssetStreamer::SetGlobalLoadCallback(std::function<void(const std::string&, bool)> callback) {
    globalLoadCallback_ = callback;
}

void AssetStreamer::SetProgressCallback(std::function<void(const std::string&, float)> callback) {
    progressCallback_ = callback;
}

void AssetStreamer::SetErrorCallback(std::function<void(const std::string&, const std::string&)> callback) {
    errorCallback_ = callback;
}

float AssetStreamer::GetLoadProgress(const std::string& assetPath) const {
    auto it = loadProgress_.find(assetPath);
    return it != loadProgress_.end() ? it->second : 0.0f;
}

size_t AssetStreamer::GetQueueSize() const {
    return stats_.queuedRequests;
}

} // namespace Nova
