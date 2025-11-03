#include "AssetVersioningSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <regex>
#include <random>
#include <thread>
#include <ctime>
// #include <openssl/sha.h>  // Commented out - not available in this environment

namespace assets {
    namespace versioning {

        // Version implementation
        bool Version::FromString(const std::string& versionStr) {
            std::regex versionRegex(R"((\d+)\.(\d+)\.(\d+)(?:\.(\d+))?)");
            std::smatch matches;
            
            if (std::regex_match(versionStr, matches, versionRegex)) {
                major = std::stoul(matches[1].str());
                minor = std::stoul(matches[2].str());
                patch = std::stoul(matches[3].str());
                build = matches[4].matched ? std::stoul(matches[4].str()) : 0;
                return true;
            }
            return false;
        }

        // VersionHistory implementation
        void VersionHistory::AddVersion(const AssetVersionEntry& entry) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Insert in sorted order
            auto it = std::lower_bound(versions_.begin(), versions_.end(), entry,
                [](const AssetVersionEntry& a, const AssetVersionEntry& b) {
                    return a.version < b.version;
                });
            
            // Check if version already exists
            if (it != versions_.end() && it->version == entry.version) {
                // Replace existing version
                *it = entry;
                std::cout << "[Versioning] Updated existing version " << entry.version.ToString() 
                         << " for asset " << assetId_ << std::endl;
            } else {
                // Insert new version
                versions_.insert(it, entry);
                std::cout << "[Versioning] Added new version " << entry.version.ToString() 
                         << " for asset " << assetId_ << std::endl;
            }
        }

        const AssetVersionEntry* VersionHistory::GetVersion(const Version& version) const {
            std::lock_guard<std::mutex> lock(mutex_);
            
            auto it = std::find_if(versions_.begin(), versions_.end(),
                [&version](const AssetVersionEntry& entry) {
                    return entry.version == version;
                });
            
            return (it != versions_.end()) ? &(*it) : nullptr;
        }

        const AssetVersionEntry* VersionHistory::GetLatestVersion() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return versions_.empty() ? nullptr : &versions_.back();
        }

        std::vector<AssetVersionEntry> VersionHistory::GetAllVersions() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return versions_;
        }

        std::vector<AssetVersionEntry> VersionHistory::GetVersionsInRange(const Version& from, const Version& to) const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<AssetVersionEntry> result;
            
            for (const auto& entry : versions_) {
                if (entry.version >= from && entry.version <= to) {
                    result.push_back(entry);
                }
            }
            
            return result;
        }

        bool VersionHistory::HasVersion(const Version& version) const {
            return GetVersion(version) != nullptr;
        }

        Version VersionHistory::GetNextVersion(bool incrementMajor, bool incrementMinor) const {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (versions_.empty()) {
                return Version{1, 0, 0, 0};
            }
            
            Version next = versions_.back().version;
            
            if (incrementMajor) {
                next.major++;
                next.minor = 0;
                next.patch = 0;
                next.build = 0;
            } else if (incrementMinor) {
                next.minor++;
                next.patch = 0;
                next.build = 0;
            } else {
                next.patch++;
                next.build = 0;
            }
            
            return next;
        }

        void VersionHistory::PruneVersions(size_t maxVersionsToKeep) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (versions_.size() > maxVersionsToKeep) {
                size_t toRemove = versions_.size() - maxVersionsToKeep;
                versions_.erase(versions_.begin(), versions_.begin() + toRemove);
                std::cout << "[Versioning] Pruned " << toRemove << " old versions for asset " 
                         << assetId_ << std::endl;
            }
        }

        void VersionHistory::ArchiveVersions(const std::chrono::system_clock::time_point& olderThan) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            size_t archivedCount = 0;
            for (auto& entry : versions_) {
                if (entry.timestamp < olderThan && !entry.isArchived) {
                    entry.isArchived = true;
                    archivedCount++;
                }
            }
            
            if (archivedCount > 0) {
                std::cout << "[Versioning] Archived " << archivedCount << " versions for asset " 
                         << assetId_ << std::endl;
            }
        }

        size_t VersionHistory::GetTotalDataSize() const {
            std::lock_guard<std::mutex> lock(mutex_);
            
            size_t totalSize = 0;
            for (const auto& entry : versions_) {
                totalSize += entry.dataSizeBytes;
            }
            return totalSize;
        }

        // AssetVersioningSystem implementation
        bool AssetVersioningSystem::Initialize(const ChangeTrackingConfig& config) {
            if (initialized_.load()) {
                std::cout << "[Versioning] System already initialized" << std::endl;
                return true;
            }

            config_ = config;
            
            // Initialize statistics
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_ = VersioningStats{};
            }

            lastUpdateTime_ = std::chrono::steady_clock::now();
            
            // Register console commands
            RegisterConsoleCommands();
            
            initialized_.store(true);
            std::cout << "[Versioning] Asset Versioning System initialized successfully" << std::endl;
            return true;
        }

        void AssetVersioningSystem::Shutdown() {
            if (!initialized_.load()) {
                return;
            }

            // Clear all data
            {
                std::lock_guard<std::mutex> lock(assetMutex_);
                versionHistories_.clear();
                assetMetadata_.clear();
                dependencyGraph_.clear();
            }

            {
                std::lock_guard<std::mutex> lock(changeMutex_);
                changeHistory_.clear();
            }

            initialized_.store(false);
            std::cout << "[Versioning] Asset Versioning System shut down" << std::endl;
        }

        void AssetVersioningSystem::Update() {
            if (!initialized_.load()) {
                return;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime_);
            
            // Update every 5 seconds
            if (elapsed.count() >= 5) {
                DetectAndRecordChanges();
                CheckForCircularDependencies();
                lastUpdateTime_ = now;
            }
        }

        bool AssetVersioningSystem::RegisterAsset(const std::string& assetId, 
                                                 const std::string& filePath, 
                                                 const AssetMetadata& metadata) {
            if (!initialized_.load()) {
                std::cout << "[Versioning] System not initialized" << std::endl;
                return false;
            }

            std::lock_guard<std::mutex> lock(assetMutex_);
            
            // Create version history if it doesn't exist
            if (versionHistories_.find(assetId) == versionHistories_.end()) {
                versionHistories_[assetId] = std::make_unique<VersionHistory>(assetId);
            }

            // Set up metadata
            AssetMetadata fullMetadata = metadata;
            fullMetadata.assetId = assetId;
            fullMetadata.filePath = filePath;
            
            if (fs::exists(filePath)) {
                fullMetadata.fileSize = fs::file_size(filePath);
                // Convert filesystem time to system_clock time
                auto fsTime = fs::last_write_time(filePath);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    fsTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                fullMetadata.modifiedTime = sctp;
                if (config_.enableChecksumValidation) {
                    fullMetadata.checksum = CalculateChecksum(filePath);
                }
            }
            
            fullMetadata.createdTime = std::chrono::system_clock::now();
            assetMetadata_[assetId] = fullMetadata;

            // Create initial version
            AssetVersionEntry initialVersion;
            initialVersion.version = Version{1, 0, 0, 0};
            initialVersion.filePath = filePath;
            initialVersion.metadata = fullMetadata;
            initialVersion.timestamp = std::chrono::system_clock::now();
            initialVersion.author = metadata.author.empty() ? "System" : metadata.author;
            initialVersion.changeDescription = "Initial version";
            initialVersion.commitHash = GenerateCommitHash(initialVersion);
            initialVersion.dataSizeBytes = fullMetadata.fileSize;

            versionHistories_[assetId]->AddVersion(initialVersion);

            // Add to dependency graph
            DependencyNode node;
            node.assetId = assetId;
            node.currentVersion = initialVersion.version;
            node.lastChecked = std::chrono::system_clock::now();
            dependencyGraph_[assetId] = node;

            // Record creation change using internal method (avoid deadlock)
            {
                std::lock_guard<std::mutex> changeLock(changeMutex_);
                RecordChangeInternal(assetId, ChangeType::Created, "Asset registered with versioning system",
                                   "System", initialVersion.version, filePath);
            }

            std::cout << "[Versioning] Registered asset: " << assetId << " -> " << filePath << std::endl;
            return true;
        }

        void AssetVersioningSystem::UnregisterAsset(const std::string& assetId) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            versionHistories_.erase(assetId);
            assetMetadata_.erase(assetId);
            dependencyGraph_.erase(assetId);

            // Remove from other assets' dependencies
            for (auto& [id, node] : dependencyGraph_) {
                node.dependencies.erase(assetId);
                node.dependents.erase(assetId);
            }

            std::cout << "[Versioning] Unregistered asset: " << assetId << std::endl;
        }

        bool AssetVersioningSystem::IsAssetRegistered(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            return versionHistories_.find(assetId) != versionHistories_.end();
        }

        Version AssetVersioningSystem::CreateNewVersion(const std::string& assetId, 
                                                       const std::string& description,
                                                       bool incrementMajor, 
                                                       bool incrementMinor) {
            std::cout << "[Versioning] CreateNewVersion called for: " << assetId << std::endl;
            std::lock_guard<std::mutex> lock(assetMutex_);
            std::cout << "[Versioning] Lock acquired" << std::endl;
            
            auto it = versionHistories_.find(assetId);
            if (it == versionHistories_.end()) {
                std::cout << "[Versioning] Asset not found: " << assetId << std::endl;
                return Version{};
            }
            std::cout << "[Versioning] Asset found in version histories" << std::endl;

            // Get next version number
            std::cout << "[Versioning] Getting next version..." << std::endl;
            Version newVersion = it->second->GetNextVersion(incrementMajor, incrementMinor);
            std::cout << "[Versioning] Next version: " << newVersion.ToString() << std::endl;
            
            // Create version entry
            std::cout << "[Versioning] Creating version entry..." << std::endl;
            AssetVersionEntry entry;
            entry.version = newVersion;
            entry.filePath = assetMetadata_[assetId].filePath;
            std::cout << "[Versioning] File path: " << entry.filePath << std::endl;
            entry.metadata = assetMetadata_[assetId];
            entry.timestamp = std::chrono::system_clock::now();
            entry.author = assetMetadata_[assetId].author;
            entry.changeDescription = description.empty() ? "Version created" : description;
            std::cout << "[Versioning] Generating commit hash..." << std::endl;
            entry.commitHash = GenerateCommitHash(entry);
            std::cout << "[Versioning] Commit hash generated" << std::endl;
            
            std::cout << "[Versioning] Checking if file exists..." << std::endl;
            if (fs::exists(entry.filePath)) {
                std::cout << "[Versioning] File exists, getting size..." << std::endl;
                entry.dataSizeBytes = fs::file_size(entry.filePath);
                std::cout << "[Versioning] File size: " << entry.dataSizeBytes << " bytes" << std::endl;
                if (config_.enableChecksumValidation) {
                    std::cout << "[Versioning] Checksum validation enabled, calculating..." << std::endl;
                    entry.metadata.checksum = CalculateChecksum(entry.filePath);
                    std::cout << "[Versioning] Checksum calculated" << std::endl;
                }
            } else {
                std::cout << "[Versioning] File does not exist: " << entry.filePath << std::endl;
            }

            std::cout << "[Versioning] Checking dependencies..." << std::endl;
            // Add dependencies
            auto depIt = dependencyGraph_.find(assetId);
            if (depIt != dependencyGraph_.end()) {
                std::cout << "[Versioning] Found dependencies, adding..." << std::endl;
                for (const auto& dep : depIt->second.dependencies) {
                    entry.dependencies.push_back(dep);
                }
                depIt->second.currentVersion = newVersion;
            }
            std::cout << "[Versioning] Dependencies processed" << std::endl;

            std::cout << "[Versioning] Adding version to history..." << std::endl;
            it->second->AddVersion(entry);
            std::cout << "[Versioning] Version added to history" << std::endl;
            
            // Record change using internal method (no additional locks needed)
            std::cout << "[Versioning] Recording change..." << std::endl;
            {
                std::lock_guard<std::mutex> changeLock(changeMutex_);
                RecordChangeInternal(assetId, ChangeType::Modified, "New version created: " + description,
                                   "System", newVersion, entry.filePath);
            }
            std::cout << "[Versioning] Change recorded" << std::endl;

            std::cout << "[Versioning] Created version " << newVersion.ToString() 
                     << " for asset " << assetId << std::endl;
            return newVersion;
        }

        bool AssetVersioningSystem::SetAssetVersion(const std::string& assetId, const Version& version) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto depIt = dependencyGraph_.find(assetId);
            if (depIt != dependencyGraph_.end()) {
                depIt->second.currentVersion = version;
                return true;
            }
            return false;
        }

        Version AssetVersioningSystem::GetAssetVersion(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = dependencyGraph_.find(assetId);
            return (it != dependencyGraph_.end()) ? it->second.currentVersion : Version{};
        }

        Version AssetVersioningSystem::GetLatestVersion(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = versionHistories_.find(assetId);
            if (it != versionHistories_.end()) {
                auto* latest = it->second->GetLatestVersion();
                return latest ? latest->version : Version{};
            }
            return Version{};
        }

        std::string AssetVersioningSystem::RecordChange(const std::string& assetId, 
                                                       ChangeType changeType,
                                                       const std::string& description, 
                                                       const std::string& author) {
            std::lock_guard<std::mutex> lock(changeMutex_);
            
            ChangeRecord record;
            record.changeId = GenerateChangeId();
            record.assetId = assetId;
            record.changeType = changeType;
            record.timestamp = std::chrono::system_clock::now();
            record.author = author.empty() ? "System" : author;
            record.description = description;
            
            // Get current version
            {
                std::lock_guard<std::mutex> assetLock(assetMutex_);
                auto it = dependencyGraph_.find(assetId);
                if (it != dependencyGraph_.end()) {
                    record.version = it->second.currentVersion;
                }
                
                auto metaIt = assetMetadata_.find(assetId);
                if (metaIt != assetMetadata_.end()) {
                    record.currentFilePath = metaIt->second.filePath;
                }
            }

            changeHistory_.push_back(record);
            
            // Limit change history size
            if (changeHistory_.size() > config_.maxChangeHistory) {
                changeHistory_.erase(changeHistory_.begin(), 
                                   changeHistory_.begin() + (changeHistory_.size() - config_.maxChangeHistory));
            }

            std::cout << "[Versioning] Recorded change " << record.changeId << " for asset " 
                     << assetId << ": " << description << std::endl;
            return record.changeId;
        }

        std::vector<ChangeRecord> AssetVersioningSystem::GetChangeHistory(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(changeMutex_);
            
            std::vector<ChangeRecord> result;
            for (const auto& record : changeHistory_) {
                if (record.assetId == assetId) {
                    result.push_back(record);
                }
            }
            
            // Sort by timestamp (newest first)
            std::sort(result.begin(), result.end(),
                [](const ChangeRecord& a, const ChangeRecord& b) {
                    return a.timestamp > b.timestamp;
                });
            
            return result;
        }

        std::vector<ChangeRecord> AssetVersioningSystem::GetRecentChanges(size_t maxCount) const {
            std::lock_guard<std::mutex> lock(changeMutex_);
            
            std::vector<ChangeRecord> result = changeHistory_;
            
            // Sort by timestamp (newest first)
            std::sort(result.begin(), result.end(),
                [](const ChangeRecord& a, const ChangeRecord& b) {
                    return a.timestamp > b.timestamp;
                });
            
            if (result.size() > maxCount) {
                result.resize(maxCount);
            }
            
            return result;
        }

        std::vector<ChangeRecord> AssetVersioningSystem::GetChangesSince(
            const std::chrono::system_clock::time_point& since) const {
            std::lock_guard<std::mutex> lock(changeMutex_);
            
            std::vector<ChangeRecord> result;
            for (const auto& record : changeHistory_) {
                if (record.timestamp >= since) {
                    result.push_back(record);
                }
            }
            
            // Sort by timestamp (newest first)
            std::sort(result.begin(), result.end(),
                [](const ChangeRecord& a, const ChangeRecord& b) {
                    return a.timestamp > b.timestamp;
                });
            
            return result;
        }

        // Additional implementation methods...
        
        std::string AssetVersioningSystem::GenerateChangeId() const {
            // Generate unique change ID using timestamp and random component
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);
            
            return "CHG_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
        }

        std::string AssetVersioningSystem::GenerateCommitHash(const AssetVersionEntry& entry) const {
            // Create hash from version info, timestamp, and file path
            std::stringstream ss;
            ss << entry.version.ToString() << "_" 
               << std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count()
               << "_" << entry.filePath << "_" << entry.changeDescription;
            
            // Simple hash for now (in production, use proper cryptographic hash)
            std::hash<std::string> hasher;
            size_t hash = hasher(ss.str());
            
            std::stringstream hexStream;
            hexStream << std::hex << hash;
            return hexStream.str();
        }

        std::string AssetVersioningSystem::CalculateChecksum(const std::string& filePath) const {
            std::cout << "[Versioning] Calculating checksum for: " << filePath << std::endl;
            
            if (!fs::exists(filePath)) {
                std::cout << "[Versioning] File does not exist: " << filePath << std::endl;
                return "";
            }

            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                std::cout << "[Versioning] Failed to open file: " << filePath << std::endl;
                return "";
            }

            // Simple checksum using hash (in production, use SHA-256 or similar)
            std::stringstream buffer;
            try {
                std::cout << "[Versioning] Reading file content..." << std::endl;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                std::cout << "[Versioning] File content size: " << content.size() << " bytes" << std::endl;
                
                std::hash<std::string> hasher;
                size_t hash = hasher(content);
                
                std::stringstream hexStream;
                hexStream << std::hex << hash;
                std::string result = hexStream.str();
                std::cout << "[Versioning] Checksum calculated: " << result << std::endl;
                return result;
            } catch (const std::exception& e) {
                std::cout << "[Versioning] Exception during checksum calculation: " << e.what() << std::endl;
                return "";
            }
        }

        bool AssetVersioningSystem::HasFileChanged(const std::string& assetId) const {
            auto metaIt = assetMetadata_.find(assetId);
            if (metaIt == assetMetadata_.end()) {
                return false;
            }

            const std::string& filePath = metaIt->second.filePath;
            if (!fs::exists(filePath)) {
                return true; // File was deleted
            }

            // Check file modification time
            auto currentModTime = fs::last_write_time(filePath);
            // Convert to system_clock for comparison
            auto currentSystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                currentModTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            if (currentSystemTime != metaIt->second.modifiedTime) {
                return true;
            }

            // Check checksum if enabled
            if (config_.enableChecksumValidation) {
                std::string currentChecksum = CalculateChecksum(filePath);
                if (currentChecksum != metaIt->second.checksum) {
                    return true;
                }
            }

            return false;
        }

        void AssetVersioningSystem::DetectAndRecordChanges() {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            for (auto& [assetId, metadata] : assetMetadata_) {
                if (HasFileChanged(assetId)) {
                    // Update metadata
                    if (fs::exists(metadata.filePath)) {
                        auto fsTime = fs::last_write_time(metadata.filePath);
                        metadata.modifiedTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            fsTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                        metadata.fileSize = fs::file_size(metadata.filePath);
                        if (config_.enableChecksumValidation) {
                            metadata.checksum = CalculateChecksum(metadata.filePath);
                        }
                    }

                    // Create new version if auto-versioning is enabled
                    if (config_.enableAutoVersioning) {
                        // Release lock temporarily to avoid deadlock
                        assetMutex_.unlock();
                        CreateNewVersion(assetId, "Auto-detected file change");
                        assetMutex_.lock();
                    } else {
                        // Just record the change
                        changeMutex_.unlock();
                        RecordChange(assetId, ChangeType::Modified, "File modification detected");
                        changeMutex_.lock();
                    }
                }
            }
        }

        void AssetVersioningSystem::CheckForCircularDependencies() {
            // Implementation for circular dependency detection
            // This would use graph algorithms to detect cycles
        }

        void AssetVersioningSystem::RegisterConsoleCommands() {
            VersioningConsoleCommands::RegisterCommands();
        }

        // Utility functions implementation
        namespace versioning_utils {
            Version ParseVersion(const std::string& versionStr) {
                Version version;
                if (version.FromString(versionStr)) {
                    return version;
                }
                return Version{0, 0, 0, 0}; // Invalid version
            }

            std::string FormatVersion(const Version& version) {
                return version.ToString();
            }

            bool IsValidVersionString(const std::string& versionStr) {
                Version version;
                return version.FromString(versionStr);
            }

            std::string CalculateFileChecksum(const std::string& filePath) {
                // Same implementation as in AssetVersioningSystem
                if (!fs::exists(filePath)) {
                    return "";
                }

                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    return "";
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                
                std::hash<std::string> hasher;
                size_t hash = hasher(content);
                
                std::stringstream hexStream;
                hexStream << std::hex << hash;
                return hexStream.str();
            }

            size_t GetFileSize(const std::string& filePath) {
                if (fs::exists(filePath)) {
                    return fs::file_size(filePath);
                }
                return 0;
            }

            std::chrono::system_clock::time_point GetFileModificationTime(const std::string& filePath) {
                if (fs::exists(filePath)) {
                    auto fsTime = fs::last_write_time(filePath);
                    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        fsTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                }
                return std::chrono::system_clock::time_point{};
            }

            std::string ChangeTypeToString(ChangeType changeType) {
                switch (changeType) {
                    case ChangeType::Created: return "Created";
                    case ChangeType::Modified: return "Modified";
                    case ChangeType::Deleted: return "Deleted";
                    case ChangeType::Moved: return "Moved";
                    case ChangeType::Metadata: return "Metadata";
                    case ChangeType::Dependencies: return "Dependencies";
                    default: return "Unknown";
                }
            }

            ChangeType StringToChangeType(const std::string& changeTypeStr) {
                if (changeTypeStr == "Created") return ChangeType::Created;
                if (changeTypeStr == "Modified") return ChangeType::Modified;
                if (changeTypeStr == "Deleted") return ChangeType::Deleted;
                if (changeTypeStr == "Moved") return ChangeType::Moved;
                if (changeTypeStr == "Metadata") return ChangeType::Metadata;
                if (changeTypeStr == "Dependencies") return ChangeType::Dependencies;
                return ChangeType::Modified; // Default
            }

            std::string FormatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
                auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                return ss.str();
            }

            std::chrono::system_clock::time_point ParseTimestamp(const std::string& timestampStr) {
                // Basic timestamp parsing implementation
                std::tm tm = {};
                std::istringstream ss(timestampStr);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                return std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
        }

        // Console commands implementation
        void VersioningConsoleCommands::RegisterCommands() {
            std::cout << "[Versioning] Console commands registered" << std::endl;
            // Integration with console system would go here
        }

        void VersioningConsoleCommands::HandleVersionList(const std::vector<std::string>& args) {
            if (args.size() < 2) {
                std::cout << "Usage: version_list <asset_id>" << std::endl;
                return;
            }

            auto& system = AssetVersioningSystem::Instance();
            std::string assetId = args[1];
            
            auto versions = system.GetAllVersions(assetId);
            if (versions.empty()) {
                std::cout << "No versions found for asset: " << assetId << std::endl;
                return;
            }

            std::cout << "Versions for asset " << assetId << ":" << std::endl;
            for (const auto& version : versions) {
                std::cout << "  " << version.version.ToString() 
                         << " - " << version.changeDescription 
                         << " (" << versioning_utils::FormatTimestamp(version.timestamp) << ")" << std::endl;
            }
        }

        void VersioningConsoleCommands::HandleVersionStats(const std::vector<std::string>& args) {
            auto& system = AssetVersioningSystem::Instance();
            auto stats = system.GetStats();
            
            std::cout << "Asset Versioning Statistics:" << std::endl;
            std::cout << "  Total Assets: " << stats.totalAssets << std::endl;
            std::cout << "  Total Versions: " << stats.totalVersions << std::endl;
            std::cout << "  Total Changes: " << stats.totalChanges << std::endl;
            std::cout << "  Total Dependencies: " << stats.totalDependencies << std::endl;
            std::cout << "  Archived Versions: " << stats.archivedVersions << std::endl;
            std::cout << "  Average Versions per Asset: " << stats.averageVersionsPerAsset << std::endl;
            std::cout << "  Total Storage Used: " << stats.totalStorageUsed << " bytes" << std::endl;
        }

        // Placeholder implementations for other console commands
        void VersioningConsoleCommands::HandleVersionCreate(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionRollback(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionHistory(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionDeps(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionChanges(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionValidate(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionExport(const std::vector<std::string>& args) { /* TODO */ }
        void VersioningConsoleCommands::HandleVersionImport(const std::vector<std::string>& args) { /* TODO */ }

        // Missing method implementations
        std::vector<AssetVersionEntry> AssetVersioningSystem::GetAllVersions(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = versionHistories_.find(assetId);
            if (it != versionHistories_.end()) {
                return it->second->GetAllVersions();
            }
            return {};
        }

        AssetMetadata AssetVersioningSystem::GetMetadata(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = assetMetadata_.find(assetId);
            if (it != assetMetadata_.end()) {
                return it->second;
            }
            return {};
        }

        void AssetVersioningSystem::AddDependency(const std::string& assetId, const std::string& dependencyId) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto& node = dependencyGraph_[assetId];
            node.assetId = assetId;
            node.dependencies.insert(dependencyId);
            
            auto& depNode = dependencyGraph_[dependencyId];
            depNode.assetId = dependencyId;
            depNode.dependents.insert(assetId);
            
            std::cout << "[Versioning] Added dependency: " << assetId << " -> " << dependencyId << std::endl;
        }

        void AssetVersioningSystem::RemoveDependency(const std::string& assetId, const std::string& dependencyId) {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto assetIt = dependencyGraph_.find(assetId);
            if (assetIt != dependencyGraph_.end()) {
                assetIt->second.dependencies.erase(dependencyId);
            }
            
            auto depIt = dependencyGraph_.find(dependencyId);
            if (depIt != dependencyGraph_.end()) {
                depIt->second.dependents.erase(assetId);
            }
            
            std::cout << "[Versioning] Removed dependency: " << assetId << " -> " << dependencyId << std::endl;
        }

        std::vector<std::string> AssetVersioningSystem::GetDependencies(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = dependencyGraph_.find(assetId);
            if (it != dependencyGraph_.end()) {
                return std::vector<std::string>(it->second.dependencies.begin(), it->second.dependencies.end());
            }
            return {};
        }

        std::vector<std::string> AssetVersioningSystem::GetDependents(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            auto it = dependencyGraph_.find(assetId);
            if (it != dependencyGraph_.end()) {
                return std::vector<std::string>(it->second.dependents.begin(), it->second.dependents.end());
            }
            return {};
        }

        std::vector<std::string> AssetVersioningSystem::GetTransitiveDependencies(const std::string& assetId) const {
            std::lock_guard<std::mutex> lock(assetMutex_);
            
            std::vector<std::string> result;
            std::unordered_set<std::string> visited;
            
            std::function<void(const std::string&)> collectDeps = [&](const std::string& id) {
                if (visited.find(id) != visited.end()) return;
                visited.insert(id);
                
                auto it = dependencyGraph_.find(id);
                if (it != dependencyGraph_.end()) {
                    for (const auto& dep : it->second.dependencies) {
                        result.push_back(dep);
                        collectDeps(dep);
                    }
                }
            };
            
            collectDeps(assetId);
            return result;
        }

        VersioningStats AssetVersioningSystem::GetStats() const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            
            VersioningStats stats;
            stats.totalAssets = assetMetadata_.size();
            stats.totalDependencies = 0;
            
            for (const auto& [id, node] : dependencyGraph_) {
                stats.totalDependencies += node.dependencies.size();
            }
            
            stats.totalVersions = 0;
            for (const auto& [id, history] : versionHistories_) {
                stats.totalVersions += history->GetVersionCount();
            }
            
            stats.totalChanges = changeHistory_.size();
            stats.averageVersionsPerAsset = stats.totalAssets > 0 ? 
                static_cast<float>(stats.totalVersions) / stats.totalAssets : 0.0f;
            
            return stats;
        }

        std::string AssetVersioningSystem::RecordChangeInternal(const std::string& assetId, 
                                                              ChangeType changeType,
                                                              const std::string& description, 
                                                              const std::string& author,
                                                              const Version& currentVersion,
                                                              const std::string& filePath) {
            // This method assumes changeMutex_ is already locked
            ChangeRecord record;
            record.changeId = GenerateChangeId();
            record.assetId = assetId;
            record.changeType = changeType;
            record.timestamp = std::chrono::system_clock::now();
            record.author = author.empty() ? "System" : author;
            record.description = description;
            record.version = currentVersion;
            record.currentFilePath = filePath;

            changeHistory_.push_back(record);
            return record.changeId;
        }

    } // namespace versioning
} // namespace assets