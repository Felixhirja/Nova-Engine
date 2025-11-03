#include "AssetHotReloader.h"
#include "AssetProcessingPipeline.h"
#include "AssetStreamingSystem.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace assets {
    namespace hotreload {

        // ChangeEventQueue Implementation
        void ChangeEventQueue::PushEvent(const HotReloadEvent& event) {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.push(event);
        }

        bool ChangeEventQueue::PopEvent(HotReloadEvent& event) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (events_.empty()) {
                return false;
            }
            event = events_.front();
            events_.pop();
            return true;
        }

        void ChangeEventQueue::Clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            while (!events_.empty()) {
                events_.pop();
            }
        }

        size_t ChangeEventQueue::Size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return events_.size();
        }

        bool ChangeEventQueue::Empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return events_.empty();
        }

        // FileWatcher Implementation
        FileWatcher::FileWatcher(const WatcherConfig& config) : config_(config) {
            if (config_.enableLogging) {
                std::cout << "[HotReload] FileWatcher created for " << config_.watchDirectories.size() 
                          << " directories" << std::endl;
            }
        }

        FileWatcher::~FileWatcher() {
            StopWatching();
        }

        bool FileWatcher::StartWatching() {
            if (watching_.load()) {
                return true;
            }

            if (config_.enableLogging) {
                std::cout << "[HotReload] Starting file watcher..." << std::endl;
            }

            shouldExit_.store(false);
            watching_.store(true);
            
            // Start watcher thread
            watcherThread_ = std::thread(&FileWatcher::WatcherThreadMain, this);

            if (config_.enableLogging) {
                std::cout << "[HotReload] File watcher started successfully" << std::endl;
            }

            return true;
        }

        void FileWatcher::StopWatching() {
            if (!watching_.load()) {
                return;
            }

            if (config_.enableLogging) {
                std::cout << "[HotReload] Stopping file watcher..." << std::endl;
            }

            shouldExit_.store(true);
            watching_.store(false);

            if (watcherThread_.joinable()) {
                watcherThread_.join();
            }

            if (config_.enableLogging) {
                std::cout << "[HotReload] File watcher stopped" << std::endl;
            }
        }

        void FileWatcher::CheckForChanges() {
            for (const auto& directory : config_.watchDirectories) {
                ProcessDirectoryChanges(directory);
            }
        }

        void FileWatcher::WatcherThreadMain() {
            while (!shouldExit_.load()) {
                CheckForChanges();
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check every 100ms
            }
        }

        void FileWatcher::ProcessDirectoryChanges(const std::string& directory) {
            if (!fs::exists(directory) || !fs::is_directory(directory)) {
                return;
            }

            std::lock_guard<std::mutex> lock(timestampMutex_);

            auto processFile = [this](const fs::directory_entry& entry) {
                if (!entry.is_regular_file()) return;

                std::string filePath = entry.path().string();
                if (!ShouldWatchFile(filePath)) return;

                try {
                    auto currentTime = entry.last_write_time();
                    auto it = fileTimestamps_.find(filePath);

                    if (it == fileTimestamps_.end()) {
                        // New file
                        fileTimestamps_[filePath] = currentTime;
                        if (eventCallback_) {
                            HotReloadEvent event;
                            event.filePath = filePath;
                            event.changeType = ChangeType::Created;
                            event.timestamp = std::chrono::steady_clock::now();
                            eventCallback_(event);
                        }
                    } else if (it->second != currentTime) {
                        // Modified file
                        it->second = currentTime;
                        if (eventCallback_) {
                            HotReloadEvent event;
                            event.filePath = filePath;
                            event.changeType = ChangeType::Modified;
                            event.timestamp = std::chrono::steady_clock::now();
                            eventCallback_(event);
                        }
                    }
                } catch (const std::exception& e) {
                    if (config_.enableLogging) {
                        std::cout << "[HotReload] Error processing file " << filePath 
                                  << ": " << e.what() << std::endl;
                    }
                }
            };

            if (config_.watchSubdirectories) {
                for (const auto& entry : fs::recursive_directory_iterator(directory, 
                    fs::directory_options::skip_permission_denied)) {
                    processFile(entry);
                }
            } else {
                for (const auto& entry : fs::directory_iterator(directory, 
                    fs::directory_options::skip_permission_denied)) {
                    processFile(entry);
                }
            }
        }

        bool FileWatcher::ShouldWatchFile(const std::string& filePath) const {
            if (config_.fileExtensions.empty()) {
                return true;
            }

            std::string extension = hotreload_utils::GetFileExtension(filePath);
            return std::find(config_.fileExtensions.begin(), config_.fileExtensions.end(), extension) 
                   != config_.fileExtensions.end();
        }

        // DebounceManager Implementation
        bool DebounceManager::ShouldProcess(const std::string& filePath) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            auto now = std::chrono::steady_clock::now();
            auto it = lastEventTime_.find(filePath);
            
            if (it == lastEventTime_.end()) {
                lastEventTime_[filePath] = now;
                return true;
            }
            
            auto elapsed = std::chrono::duration<float>(now - it->second).count();
            if (elapsed >= debounceTime_) {
                it->second = now;
                return true;
            }
            
            return false;
        }

        void DebounceManager::Clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            lastEventTime_.clear();
        }

        // AssetHotReloader Implementation
        bool AssetHotReloader::Initialize(const WatcherConfig& config) {
            if (initialized_.load()) {
                std::cout << "[HotReload] System already initialized" << std::endl;
                return true;
            }

            std::cout << "[HotReload] Initializing Hot Reload System..." << std::endl;

            config_ = config;
            
            // Create file watcher
            fileWatcher_ = std::make_unique<FileWatcher>(config_);
            fileWatcher_->SetEventCallback([this](const HotReloadEvent& event) {
                eventQueue_.PushEvent(event);
            });

            // Create debounce manager
            debounceManager_ = std::make_unique<DebounceManager>(config_.debounceTimeSeconds);

            // Register console commands
            HotReloadConsoleCommands::RegisterCommands();

            initialized_.store(true);

            std::cout << "[HotReload] System initialized successfully" << std::endl;
            std::cout << "[HotReload] Watching " << config_.watchDirectories.size() 
                      << " directories for " << config_.fileExtensions.size() << " file types" << std::endl;

            return true;
        }

        void AssetHotReloader::Shutdown() {
            if (!initialized_.load()) {
                return;
            }

            std::cout << "[HotReload] Shutting down Hot Reload System..." << std::endl;

            StopWatching();
            
            fileWatcher_.reset();
            debounceManager_.reset();
            eventQueue_.Clear();

            {
                std::lock_guard<std::mutex> lock(assetMutex_);
                assets_.clear();
                fileToAsset_.clear();
            }

            {
                std::lock_guard<std::mutex> lock(callbackMutex_);
                callbacks_.clear();
            }

            initialized_.store(false);
            std::cout << "[HotReload] Shutdown complete" << std::endl;
        }

        void AssetHotReloader::Update() {
            if (!initialized_.load()) return;

            ProcessEvents();
        }

        void AssetHotReloader::SetConfig(const WatcherConfig& config) {
            config_ = config;
            if (fileWatcher_) {
                StopWatching();
                fileWatcher_ = std::make_unique<FileWatcher>(config_);
                fileWatcher_->SetEventCallback([this](const HotReloadEvent& event) {
                    eventQueue_.PushEvent(event);
                });
            }
            if (debounceManager_) {
                debounceManager_ = std::make_unique<DebounceManager>(config_.debounceTimeSeconds);
            }
        }

        bool AssetHotReloader::StartWatching() {
            if (!initialized_.load() || !fileWatcher_) {
                return false;
            }
            return fileWatcher_->StartWatching();
        }

        void AssetHotReloader::StopWatching() {
            if (fileWatcher_) {
                fileWatcher_->StopWatching();
            }
        }

        bool AssetHotReloader::IsWatching() const {
            return fileWatcher_ && fileWatcher_->IsWatching();
        }

        void AssetHotReloader::RegisterAsset(const std::string& assetId, const std::string& filePath) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            AssetDependency dependency;
            dependency.assetId = assetId;
            dependency.filePath = hotreload_utils::NormalizePath(filePath);
            
            if (fs::exists(dependency.filePath)) {
                dependency.lastModified = fs::last_write_time(dependency.filePath);
            }
            
            assets_[assetId] = std::move(dependency);
            fileToAsset_[dependency.filePath] = assetId;
            
            if (loggingEnabled_.load()) {
                std::cout << "[HotReload] Registered asset: " << assetId 
                          << " -> " << dependency.filePath << std::endl;
            }
        }

        void AssetHotReloader::UnregisterAsset(const std::string& assetId) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                fileToAsset_.erase(it->second.filePath);
                assets_.erase(it);
                
                if (loggingEnabled_.load()) {
                    std::cout << "[HotReload] Unregistered asset: " << assetId << std::endl;
                }
            }
        }

        void AssetHotReloader::AddDependency(const std::string& assetId, const std::string& dependencyPath) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                std::string normalizedPath = hotreload_utils::NormalizePath(dependencyPath);
                auto& deps = it->second.dependencies;
                if (std::find(deps.begin(), deps.end(), normalizedPath) == deps.end()) {
                    deps.push_back(normalizedPath);
                }
                
                // Add reverse dependency
                auto depIt = fileToAsset_.find(normalizedPath);
                if (depIt != fileToAsset_.end()) {
                    auto depAssetIt = assets_.find(depIt->second);
                    if (depAssetIt != assets_.end()) {
                        auto& dependents = depAssetIt->second.dependents;
                        if (std::find(dependents.begin(), dependents.end(), assetId) == dependents.end()) {
                            dependents.push_back(assetId);
                        }
                    }
                }
            }
        }

        void AssetHotReloader::RemoveDependency(const std::string& assetId, const std::string& dependencyPath) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                std::string normalizedPath = hotreload_utils::NormalizePath(dependencyPath);
                auto& deps = it->second.dependencies;
                deps.erase(std::remove(deps.begin(), deps.end(), normalizedPath), deps.end());
                
                // Remove reverse dependency
                auto depIt = fileToAsset_.find(normalizedPath);
                if (depIt != fileToAsset_.end()) {
                    auto depAssetIt = assets_.find(depIt->second);
                    if (depAssetIt != assets_.end()) {
                        auto& dependents = depAssetIt->second.dependents;
                        dependents.erase(std::remove(dependents.begin(), dependents.end(), assetId), 
                                       dependents.end());
                    }
                }
            }
        }

        void AssetHotReloader::RegisterCallback(const std::string& name, HotReloadCallback callback) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            callbacks_[name] = callback;
            
            if (loggingEnabled_.load()) {
                std::cout << "[HotReload] Registered callback: " << name << std::endl;
            }
        }

        void AssetHotReloader::UnregisterCallback(const std::string& name) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            callbacks_.erase(name);
            
            if (loggingEnabled_.load()) {
                std::cout << "[HotReload] Unregistered callback: " << name << std::endl;
            }
        }

        void AssetHotReloader::ReloadAsset(const std::string& assetId) {
            RecordReloadStart();
            
            try {
                ReloadAssetInternal(assetId);
                RecordReloadEnd(true);
                
                if (loggingEnabled_.load()) {
                    std::cout << "[HotReload] Successfully reloaded asset: " << assetId << std::endl;
                }
            } catch (const std::exception& e) {
                RecordReloadEnd(false);
                
                if (loggingEnabled_.load()) {
                    std::cout << "[HotReload] Failed to reload asset " << assetId 
                              << ": " << e.what() << std::endl;
                }
            }
        }

        void AssetHotReloader::ReloadFile(const std::string& filePath) {
            std::string normalizedPath = hotreload_utils::NormalizePath(filePath);
            
            std::lock_guard<std::mutex> lock(assetMutex_);
            auto it = fileToAsset_.find(normalizedPath);
            if (it != fileToAsset_.end()) {
                ReloadAsset(it->second);
            }
        }

        void AssetHotReloader::ReloadAll() {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            std::cout << "[HotReload] Reloading all " << assets_.size() << " assets..." << std::endl;
            
            for (const auto& [assetId, dependency] : assets_) {
                ReloadAsset(assetId);
            }
            
            std::cout << "[HotReload] All assets reloaded" << std::endl;
        }

        HotReloadStats AssetHotReloader::GetStats() const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            return stats_;
        }

        void AssetHotReloader::ResetStats() {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_ = HotReloadStats{};
        }

        std::vector<std::string> AssetHotReloader::GetWatchedFiles() const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            std::vector<std::string> files;
            files.reserve(assets_.size());
            
            for (const auto& [assetId, dependency] : assets_) {
                files.push_back(dependency.filePath);
            }
            
            return files;
        }

        std::vector<std::string> AssetHotReloader::GetWatchedDirectories() const {
            return config_.watchDirectories;
        }

        std::vector<std::string> AssetHotReloader::GetDependencies(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            auto it = assets_.find(assetId);
            return it != assets_.end() ? it->second.dependencies : std::vector<std::string>{};
        }

        std::vector<std::string> AssetHotReloader::GetDependents(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            auto it = assets_.find(assetId);
            return it != assets_.end() ? it->second.dependents : std::vector<std::string>{};
        }

        bool AssetHotReloader::HasCircularDependency(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            std::unordered_set<std::string> visited;
            return HasCircularDependencyInternal(assetId, visited);
        }

        void AssetHotReloader::PrintDebugInfo() const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            std::cout << "\n=== Hot Reload Debug Info ===" << std::endl;
            std::cout << "Assets: " << assets_.size() << std::endl;
            std::cout << "File mappings: " << fileToAsset_.size() << std::endl;
            std::cout << "Watching: " << (IsWatching() ? "Yes" : "No") << std::endl;
            
            auto stats = GetStats();
            std::cout << "Total reloads: " << stats.totalReloads << std::endl;
            std::cout << "Successful: " << stats.successfulReloads << std::endl;
            std::cout << "Failed: " << stats.failedReloads << std::endl;
            
            std::cout << "\nRegistered assets:" << std::endl;
            for (const auto& [assetId, dependency] : assets_) {
                std::cout << "  " << assetId << " -> " << dependency.filePath << std::endl;
                if (!dependency.dependencies.empty()) {
                    std::cout << "    Dependencies: ";
                    for (const auto& dep : dependency.dependencies) {
                        std::cout << dep << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }

        // Private methods implementation
        void AssetHotReloader::ProcessEvents() {
            int eventsProcessed = 0;
            HotReloadEvent event;
            
            while (eventsProcessed < config_.maxEventsPerFrame && eventQueue_.PopEvent(event)) {
                if (debounceManager_ && debounceManager_->ShouldProcess(event.filePath)) {
                    ProcessFileChange(event);
                    eventsProcessed++;
                }
            }
        }

        void AssetHotReloader::ProcessFileChange(const HotReloadEvent& event) {
            if (loggingEnabled_.load()) {
                hotreload_utils::LogHotReloadEvent(event);
            }

            // Notify callbacks
            {
                std::lock_guard<std::mutex> lock(callbackMutex_);
                for (const auto& [name, callback] : callbacks_) {
                    try {
                        callback(event);
                    } catch (const std::exception& e) {
                        if (loggingEnabled_.load()) {
                            std::cout << "[HotReload] Callback " << name << " failed: " 
                                      << e.what() << std::endl;
                        }
                    }
                }
            }

            // Process asset changes
            if (event.changeType == ChangeType::Modified || event.changeType == ChangeType::Created) {
                ReloadFile(event.filePath);
                ReloadDependents(event.filePath);
            }

            // Notify other systems
            hotreload_utils::NotifyAssetProcessingPipeline(event.filePath, event.changeType);
        }

        void AssetHotReloader::ReloadAssetInternal(const std::string& assetId) {
            // This would integrate with the asset processing pipeline
            // For now, just notify the systems
            std::lock_guard<std::mutex> lock(assetMutex_);
            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                hotreload_utils::NotifyAssetStreamingSystem(assetId, ChangeType::Modified);
            }
        }

        void AssetHotReloader::ReloadDependents(const std::string& filePath) {
            std::string normalizedPath = hotreload_utils::NormalizePath(filePath);
            
            std::lock_guard<std::mutex> lock(assetMutex_);
            auto fileIt = fileToAsset_.find(normalizedPath);
            if (fileIt != fileToAsset_.end()) {
                auto assetIt = assets_.find(fileIt->second);
                if (assetIt != assets_.end()) {
                    const auto& dependency = assetIt->second;
                    for (const auto& dependent : dependency.dependents) {
                        ReloadAsset(dependent);
                    }
                }
            }
        }

        bool AssetHotReloader::HasCircularDependencyInternal(const std::string& assetId, 
                                                           std::unordered_set<std::string>& visited) const {
            if (visited.find(assetId) != visited.end()) {
                return true; // Circular dependency found
            }
            
            visited.insert(assetId);
            
            auto it = assets_.find(assetId);
            if (it != assets_.end()) {
                for (const auto& dep : it->second.dependencies) {
                    auto depIt = fileToAsset_.find(dep);
                    if (depIt != fileToAsset_.end()) {
                        if (HasCircularDependencyInternal(depIt->second, visited)) {
                            return true;
                        }
                    }
                }
            }
            
            visited.erase(assetId);
            return false;
        }

        void AssetHotReloader::RecordReloadStart() {
            reloadStartTime_ = std::chrono::steady_clock::now();
        }

        void AssetHotReloader::RecordReloadEnd(bool success) {
            auto endTime = std::chrono::steady_clock::now();
            float reloadTime = std::chrono::duration<float>(endTime - reloadStartTime_).count();
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.totalReloads++;
            if (success) {
                stats_.successfulReloads++;
            } else {
                stats_.failedReloads++;
            }
            
            // Update average reload time
            stats_.averageReloadTime = (stats_.averageReloadTime * (stats_.totalReloads - 1) + reloadTime) 
                                     / stats_.totalReloads;
            stats_.lastReload = endTime;
        }

        // Utility functions implementation
        namespace hotreload_utils {
            std::string GetFileExtension(const std::string& filePath) {
                size_t dotPos = filePath.find_last_of('.');
                if (dotPos == std::string::npos) {
                    return "";
                }
                return filePath.substr(dotPos);
            }

            bool IsAssetFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                static const std::unordered_set<std::string> assetExtensions = {
                    ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds",  // Images
                    ".obj", ".fbx", ".gltf", ".glb", ".dae",          // Models
                    ".glsl", ".vert", ".frag", ".hlsl",               // Shaders
                    ".wav", ".ogg", ".mp3", ".flac",                  // Audio
                    ".json", ".xml", ".yaml", ".yml",                 // Config
                    ".ttf", ".otf"                                    // Fonts
                };
                
                return assetExtensions.find(ext) != assetExtensions.end();
            }

            bool IsImageFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || 
                       ext == ".tga" || ext == ".dds";
            }

            bool IsModelFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                return ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae";
            }

            bool IsShaderFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                return ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".hlsl";
            }

            bool IsAudioFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                return ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac";
            }

            bool IsConfigFile(const std::string& filePath) {
                std::string ext = GetFileExtension(filePath);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                return ext == ".json" || ext == ".xml" || ext == ".yaml" || ext == ".yml";
            }

            std::string NormalizePath(const std::string& path) {
                try {
                    return fs::canonical(fs::path(path)).string();
                } catch (const std::exception&) {
                    // Fallback to absolute path if canonical fails
                    return fs::absolute(fs::path(path)).string();
                }
            }

            std::string GetRelativePath(const std::string& path, const std::string& basePath) {
                try {
                    return fs::relative(fs::path(path), fs::path(basePath)).string();
                } catch (const std::exception&) {
                    return path;
                }
            }

            bool IsPathInDirectory(const std::string& filePath, const std::string& directory) {
                try {
                    auto fileCanonical = fs::canonical(fs::path(filePath));
                    auto dirCanonical = fs::canonical(fs::path(directory));
                    auto relative = fs::relative(fileCanonical, dirCanonical);
                    return !relative.empty() && relative.string().find("..") != 0;
                } catch (const std::exception&) {
                    return false;
                }
            }

            void NotifyAssetProcessingPipeline(const std::string& filePath, ChangeType changeType) {
                // Integration point with AssetProcessingPipeline
                if (IsAssetFile(filePath)) {
                    std::cout << "[HotReload] Notifying processing pipeline: " << filePath << std::endl;
                    // Could call AssetProcessingPipeline::Instance().ProcessAsset(filePath);
                }
            }

            void NotifyAssetStreamingSystem(const std::string& assetId, ChangeType changeType) {
                // Integration point with AssetStreamingSystem
                std::cout << "[HotReload] Notifying streaming system: " << assetId << std::endl;
                // Could call AssetStreamingSystem::Instance().ReloadAsset(assetId);
            }

            void ShowHotReloadNotification(const std::string& message) {
                std::cout << "[HotReload] " << message << std::endl;
            }

            void LogHotReloadEvent(const HotReloadEvent& event) {
                std::string changeTypeStr;
                switch (event.changeType) {
                    case ChangeType::Created: changeTypeStr = "CREATED"; break;
                    case ChangeType::Modified: changeTypeStr = "MODIFIED"; break;
                    case ChangeType::Deleted: changeTypeStr = "DELETED"; break;
                    case ChangeType::Renamed: changeTypeStr = "RENAMED"; break;
                }
                
                std::cout << "[HotReload] " << changeTypeStr << ": " << event.filePath;
                if (!event.oldPath.empty()) {
                    std::cout << " (from " << event.oldPath << ")";
                }
                std::cout << std::endl;
            }
        }

        // Console commands implementation
        void HotReloadConsoleCommands::RegisterCommands() {
            std::cout << "[HotReload] Console commands available:" << std::endl;
            std::cout << "  hotreload.start - Start file watching" << std::endl;
            std::cout << "  hotreload.stop - Stop file watching" << std::endl;
            std::cout << "  hotreload.status - Show current status" << std::endl;
            std::cout << "  hotreload.config - Show configuration" << std::endl;
            std::cout << "  hotreload.reload <assetId> - Reload specific asset" << std::endl;
            std::cout << "  hotreload.stats - Show statistics" << std::endl;
            std::cout << "  hotreload.list - List watched assets" << std::endl;
            std::cout << "  hotreload.deps <assetId> - Show dependencies" << std::endl;
        }

        void HotReloadConsoleCommands::HandleHotReloadStart(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            if (reloader.StartWatching()) {
                std::cout << "Hot reload watching started" << std::endl;
            } else {
                std::cout << "Failed to start hot reload watching" << std::endl;
            }
        }

        void HotReloadConsoleCommands::HandleHotReloadStop(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            reloader.StopWatching();
            std::cout << "Hot reload watching stopped" << std::endl;
        }

        void HotReloadConsoleCommands::HandleHotReloadStatus(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            std::cout << "Hot reload status: " << (reloader.IsWatching() ? "Watching" : "Stopped") << std::endl;
            
            auto stats = reloader.GetStats();
            std::cout << "Total reloads: " << stats.totalReloads << std::endl;
            std::cout << "Success rate: " << (stats.totalReloads > 0 ? 
                (100.0f * stats.successfulReloads / stats.totalReloads) : 0.0f) << "%" << std::endl;
        }

        void HotReloadConsoleCommands::HandleHotReloadConfig(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            auto config = reloader.GetConfig();
            
            std::cout << "Hot reload configuration:" << std::endl;
            std::cout << "  Watch directories:" << std::endl;
            for (const auto& dir : config.watchDirectories) {
                std::cout << "    " << dir << std::endl;
            }
            std::cout << "  File extensions:" << std::endl;
            for (const auto& ext : config.fileExtensions) {
                std::cout << "    " << ext << std::endl;
            }
            std::cout << "  Debounce time: " << config.debounceTimeSeconds << "s" << std::endl;
            std::cout << "  Max events per frame: " << config.maxEventsPerFrame << std::endl;
        }

        void HotReloadConsoleCommands::HandleHotReloadReload(const std::vector<std::string>& args) {
            if (args.size() < 2) {
                std::cout << "Usage: hotreload.reload <assetId>" << std::endl;
                return;
            }
            
            auto& reloader = AssetHotReloader::Instance();
            reloader.ReloadAsset(args[1]);
        }

        void HotReloadConsoleCommands::HandleHotReloadStats(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            auto stats = reloader.GetStats();
            
            std::cout << "Hot reload statistics:" << std::endl;
            std::cout << "  Total reloads: " << stats.totalReloads << std::endl;
            std::cout << "  Successful: " << stats.successfulReloads << std::endl;
            std::cout << "  Failed: " << stats.failedReloads << std::endl;
            std::cout << "  Average reload time: " << stats.averageReloadTime << "s" << std::endl;
            std::cout << "  Files watched: " << stats.filesWatched << std::endl;
            std::cout << "  Directories watched: " << stats.directoriesWatched << std::endl;
        }

        void HotReloadConsoleCommands::HandleHotReloadList(const std::vector<std::string>& args) {
            auto& reloader = AssetHotReloader::Instance();
            auto files = reloader.GetWatchedFiles();
            
            std::cout << "Watched assets (" << files.size() << "):" << std::endl;
            for (const auto& file : files) {
                std::cout << "  " << file << std::endl;
            }
        }

        void HotReloadConsoleCommands::HandleHotReloadDeps(const std::vector<std::string>& args) {
            if (args.size() < 2) {
                std::cout << "Usage: hotreload.deps <assetId>" << std::endl;
                return;
            }
            
            auto& reloader = AssetHotReloader::Instance();
            std::string assetId = args[1];
            
            auto deps = reloader.GetDependencies(assetId);
            auto dependents = reloader.GetDependents(assetId);
            
            std::cout << "Asset: " << assetId << std::endl;
            std::cout << "Dependencies (" << deps.size() << "):" << std::endl;
            for (const auto& dep : deps) {
                std::cout << "  " << dep << std::endl;
            }
            std::cout << "Dependents (" << dependents.size() << "):" << std::endl;
            for (const auto& dependent : dependents) {
                std::cout << "  " << dependent << std::endl;
            }
            
            if (reloader.HasCircularDependency(assetId)) {
                std::cout << "WARNING: Circular dependency detected!" << std::endl;
            }
        }

        // AssetChangeNotifier Implementation
        void AssetChangeNotifier::NotifyAssetChanged(const std::string& assetId, const std::string& filePath) {
            std::lock_guard<std::mutex> lock(listenerMutex_);
            for (const auto& [name, listener] : listeners_) {
                try {
                    listener(assetId, filePath, ChangeType::Modified);
                } catch (const std::exception& e) {
                    std::cout << "[HotReload] Listener " << name << " failed: " << e.what() << std::endl;
                }
            }
        }

        void AssetChangeNotifier::NotifyAssetCreated(const std::string& assetId, const std::string& filePath) {
            std::lock_guard<std::mutex> lock(listenerMutex_);
            for (const auto& [name, listener] : listeners_) {
                try {
                    listener(assetId, filePath, ChangeType::Created);
                } catch (const std::exception& e) {
                    std::cout << "[HotReload] Listener " << name << " failed: " << e.what() << std::endl;
                }
            }
        }

        void AssetChangeNotifier::NotifyAssetDeleted(const std::string& assetId, const std::string& filePath) {
            std::lock_guard<std::mutex> lock(listenerMutex_);
            for (const auto& [name, listener] : listeners_) {
                try {
                    listener(assetId, filePath, ChangeType::Deleted);
                } catch (const std::exception& e) {
                    std::cout << "[HotReload] Listener " << name << " failed: " << e.what() << std::endl;
                }
            }
        }

        void AssetChangeNotifier::RegisterListener(const std::string& name, 
                    std::function<void(const std::string&, const std::string&, ChangeType)> listener) {
            std::lock_guard<std::mutex> lock(listenerMutex_);
            listeners_[name] = listener;
            std::cout << "[HotReload] Registered change listener: " << name << std::endl;
        }

        void AssetChangeNotifier::UnregisterListener(const std::string& name) {
            std::lock_guard<std::mutex> lock(listenerMutex_);
            listeners_.erase(name);
            std::cout << "[HotReload] Unregistered change listener: " << name << std::endl;
        }

    } // namespace hotreload
} // namespace assets