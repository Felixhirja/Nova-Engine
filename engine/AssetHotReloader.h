#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <filesystem>
#include <chrono>
#include <queue>

namespace fs = std::filesystem;

namespace assets {
    namespace hotreload {

        // Forward declarations
        class AssetHotReloader;
        class FileWatcher;

        // File change types
        enum class ChangeType {
            Created,
            Modified,
            Deleted,
            Renamed
        };

        // Hot reload event
        struct HotReloadEvent {
            std::string filePath;
            ChangeType changeType;
            std::chrono::steady_clock::time_point timestamp;
            std::string oldPath; // For rename events
        };

        // Hot reload callback signature
        using HotReloadCallback = std::function<void(const HotReloadEvent&)>;

        // File watcher configuration
        struct WatcherConfig {
            std::vector<std::string> watchDirectories = {"assets/"};
            std::vector<std::string> fileExtensions = {".png", ".jpg", ".obj", ".glsl", ".json", ".wav", ".ogg"};
            bool watchSubdirectories = true;
            float debounceTimeSeconds = 0.5f;  // Wait before processing changes
            int maxEventsPerFrame = 10;        // Limit processing per frame
            bool enableLogging = true;
        };

        // Asset dependency tracking
        struct AssetDependency {
            std::string assetId;
            std::string filePath;
            std::vector<std::string> dependencies; // Files this asset depends on
            std::vector<std::string> dependents;   // Assets that depend on this
            std::filesystem::file_time_type lastModified;
        };

        // Hot reload statistics
        struct HotReloadStats {
            int totalReloads = 0;
            int successfulReloads = 0;
            int failedReloads = 0;
            int filesWatched = 0;
            int directoriesWatched = 0;
            float averageReloadTime = 0.0f;
            std::chrono::steady_clock::time_point lastReload;
        };

        // File change event queue
        class ChangeEventQueue {
        public:
            void PushEvent(const HotReloadEvent& event);
            bool PopEvent(HotReloadEvent& event);
            void Clear();
            size_t Size() const;
            bool Empty() const;

        private:
            mutable std::mutex mutex_;
            std::queue<HotReloadEvent> events_;
        };

        // File system watcher (platform-specific implementation)
        class FileWatcher {
        public:
            FileWatcher(const WatcherConfig& config);
            ~FileWatcher();

            bool StartWatching();
            void StopWatching();
            bool IsWatching() const { return watching_.load(); }

            void SetEventCallback(HotReloadCallback callback) { eventCallback_ = callback; }

            // Manual file checking (fallback for platforms without native file watching)
            void CheckForChanges();

        private:
            void WatcherThreadMain();
            void ProcessDirectoryChanges(const std::string& directory);
            bool ShouldWatchFile(const std::string& filePath) const;

            WatcherConfig config_;
            std::atomic<bool> watching_{false};
            std::atomic<bool> shouldExit_{false};
            std::thread watcherThread_;
            HotReloadCallback eventCallback_;

            // File timestamp tracking for platforms without native watching
            std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps_;
            mutable std::mutex timestampMutex_;
        };

        // Debounce manager for file changes
        class DebounceManager {
        public:
            DebounceManager(float debounceTimeSeconds) : debounceTime_(debounceTimeSeconds) {}

            // Returns true if the event should be processed
            bool ShouldProcess(const std::string& filePath);
            void Clear();

        private:
            float debounceTime_;
            std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastEventTime_;
            mutable std::mutex mutex_;
        };

        // Asset Hot Reloader - main class
        class AssetHotReloader {
        public:
            static AssetHotReloader& Instance() {
                static AssetHotReloader instance;
                return instance;
            }

            // System lifecycle
            bool Initialize(const WatcherConfig& config = {});
            void Shutdown();
            void Update();

            // Configuration
            void SetConfig(const WatcherConfig& config);
            WatcherConfig GetConfig() const { return config_; }

            // Watch management
            bool StartWatching();
            void StopWatching();
            bool IsWatching() const;

            // Asset registration and dependency tracking
            void RegisterAsset(const std::string& assetId, const std::string& filePath);
            void UnregisterAsset(const std::string& assetId);
            void AddDependency(const std::string& assetId, const std::string& dependencyPath);
            void RemoveDependency(const std::string& assetId, const std::string& dependencyPath);

            // Callback registration
            void RegisterCallback(const std::string& name, HotReloadCallback callback);
            void UnregisterCallback(const std::string& name);

            // Manual reload triggers
            void ReloadAsset(const std::string& assetId);
            void ReloadFile(const std::string& filePath);
            void ReloadAll();

            // Statistics and monitoring
            HotReloadStats GetStats() const;
            void ResetStats();
            std::vector<std::string> GetWatchedFiles() const;
            std::vector<std::string> GetWatchedDirectories() const;

            // Dependency queries
            std::vector<std::string> GetDependencies(const std::string& assetId) const;
            std::vector<std::string> GetDependents(const std::string& assetId) const;
            bool HasCircularDependency(const std::string& assetId) const;

            // Debug and profiling
            void PrintDebugInfo() const;
            void EnableLogging(bool enable) { loggingEnabled_ = enable; }

        private:
            AssetHotReloader() = default;
            ~AssetHotReloader() = default;
            AssetHotReloader(const AssetHotReloader&) = delete;
            AssetHotReloader& operator=(const AssetHotReloader&) = delete;

            // Event processing
            void ProcessEvents();
            void ProcessFileChange(const HotReloadEvent& event);
            void ReloadAssetInternal(const std::string& assetId);
            void ReloadDependents(const std::string& filePath);

            // Dependency management
            void UpdateDependencyGraph();
            bool HasCircularDependencyInternal(const std::string& assetId, 
                                             std::unordered_set<std::string>& visited) const;

            // Statistics tracking
            void RecordReloadStart();
            void RecordReloadEnd(bool success);

            // Configuration and state
            WatcherConfig config_;
            std::atomic<bool> initialized_{false};
            std::atomic<bool> loggingEnabled_{true};

            // File watching
            std::unique_ptr<FileWatcher> fileWatcher_;
            std::unique_ptr<DebounceManager> debounceManager_;
            ChangeEventQueue eventQueue_;

            // Asset tracking
            std::unordered_map<std::string, AssetDependency> assets_;
            std::unordered_map<std::string, std::string> fileToAsset_; // filePath -> assetId
            mutable std::mutex assetMutex_;

            // Callbacks
            std::unordered_map<std::string, HotReloadCallback> callbacks_;
            mutable std::mutex callbackMutex_;

            // Statistics
            mutable HotReloadStats stats_;
            mutable std::mutex statsMutex_;
            std::chrono::steady_clock::time_point reloadStartTime_;
        };

        // Integration utilities
        namespace hotreload_utils {
            // File type detection
            std::string GetFileExtension(const std::string& filePath);
            bool IsAssetFile(const std::string& filePath);
            bool IsImageFile(const std::string& filePath);
            bool IsModelFile(const std::string& filePath);
            bool IsShaderFile(const std::string& filePath);
            bool IsAudioFile(const std::string& filePath);
            bool IsConfigFile(const std::string& filePath);

            // Path utilities
            std::string NormalizePath(const std::string& path);
            std::string GetRelativePath(const std::string& path, const std::string& basePath);
            bool IsPathInDirectory(const std::string& filePath, const std::string& directory);

            // Asset processing integration
            void NotifyAssetProcessingPipeline(const std::string& filePath, ChangeType changeType);
            void NotifyAssetStreamingSystem(const std::string& assetId, ChangeType changeType);

            // Development helpers
            void ShowHotReloadNotification(const std::string& message);
            void LogHotReloadEvent(const HotReloadEvent& event);
        }

        // Hot reload console commands
        class HotReloadConsoleCommands {
        public:
            static void RegisterCommands();
            static void HandleHotReloadStart(const std::vector<std::string>& args);
            static void HandleHotReloadStop(const std::vector<std::string>& args);
            static void HandleHotReloadStatus(const std::vector<std::string>& args);
            static void HandleHotReloadConfig(const std::vector<std::string>& args);
            static void HandleHotReloadReload(const std::vector<std::string>& args);
            static void HandleHotReloadStats(const std::vector<std::string>& args);
            static void HandleHotReloadList(const std::vector<std::string>& args);
            static void HandleHotReloadDeps(const std::vector<std::string>& args);
        };

        // Asset change notification system
        class AssetChangeNotifier {
        public:
            static AssetChangeNotifier& Instance() {
                static AssetChangeNotifier instance;
                return instance;
            }

            void NotifyAssetChanged(const std::string& assetId, const std::string& filePath);
            void NotifyAssetCreated(const std::string& assetId, const std::string& filePath);
            void NotifyAssetDeleted(const std::string& assetId, const std::string& filePath);

            void RegisterListener(const std::string& name, 
                                std::function<void(const std::string&, const std::string&, ChangeType)> listener);
            void UnregisterListener(const std::string& name);

        private:
            std::unordered_map<std::string, std::function<void(const std::string&, const std::string&, ChangeType)>> listeners_;
            mutable std::mutex listenerMutex_;
        };

    } // namespace hotreload
} // namespace assets