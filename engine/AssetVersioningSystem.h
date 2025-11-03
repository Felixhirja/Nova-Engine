#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <functional>
#include <queue>
#include <atomic>

namespace fs = std::filesystem;

namespace assets {
    namespace versioning {

        // Forward declarations
        class AssetVersioningSystem;
        class VersionHistory;
        class ChangeTracker;

        // Version information
        struct Version {
            uint32_t major = 1;
            uint32_t minor = 0;
            uint32_t patch = 0;
            uint32_t build = 0;

            // Constructors
            Version() = default;
            Version(uint32_t maj, uint32_t min, uint32_t pat, uint32_t bld = 0) 
                : major(maj), minor(min), patch(pat), build(bld) {}

            // Semantic version string (e.g., "1.2.3.456")
            std::string ToString() const {
                return std::to_string(major) + "." + std::to_string(minor) + "." + 
                       std::to_string(patch) + "." + std::to_string(build);
            }

            // Parse from string
            bool FromString(const std::string& versionStr);

            // Comparison operators
            bool operator==(const Version& other) const {
                return major == other.major && minor == other.minor && 
                       patch == other.patch && build == other.build;
            }

            bool operator<(const Version& other) const {
                if (major != other.major) return major < other.major;
                if (minor != other.minor) return minor < other.minor;
                if (patch != other.patch) return patch < other.patch;
                return build < other.build;
            }

            bool operator>(const Version& other) const { return other < *this; }
            bool operator<=(const Version& other) const { return !(other < *this); }
            bool operator>=(const Version& other) const { return !(*this < other); }
            bool operator!=(const Version& other) const { return !(*this == other); }
        };

        // Change types for tracking
        enum class ChangeType {
            Created,        // Asset newly created
            Modified,       // Asset content changed
            Deleted,        // Asset deleted
            Moved,          // Asset moved/renamed
            Metadata,       // Metadata updated
            Dependencies    // Dependencies changed
        };

        // Asset metadata
        struct AssetMetadata {
            std::string assetId;
            std::string filePath;
            std::string assetType;
            std::string author;
            std::string description;
            std::unordered_map<std::string, std::string> customProperties;
            std::chrono::system_clock::time_point createdTime;
            std::chrono::system_clock::time_point modifiedTime;  // Use system_clock for consistency
            size_t fileSize = 0;
            std::string checksum; // For integrity verification
        };

        // Change record
        struct ChangeRecord {
            std::string changeId;              // Unique change identifier
            std::string assetId;               // Asset that changed
            ChangeType changeType;             // Type of change
            Version version;                   // Version after this change
            std::chrono::system_clock::time_point timestamp; // When change occurred
            std::string author;                // Who made the change
            std::string description;           // Change description
            std::string previousFilePath;     // For moves/renames
            std::string currentFilePath;      // Current file path
            std::unordered_map<std::string, std::string> changeData; // Additional change data
            std::vector<std::string> affectedDependencies; // Dependencies affected
        };

        // Asset version entry
        struct AssetVersionEntry {
            Version version;
            std::string commitHash;           // Unique hash for this version
            std::string filePath;             // Path at this version
            AssetMetadata metadata;           // Metadata at this version
            std::chrono::system_clock::time_point timestamp;
            std::string author;
            std::string changeDescription;
            std::vector<std::string> dependencies; // Dependencies at this version
            bool isArchived = false;          // Whether this version is archived
            size_t dataSizeBytes = 0;         // Size of asset data
        };

        // Version history for an asset
        class VersionHistory {
        public:
            VersionHistory(const std::string& assetId) : assetId_(assetId) {}

            // Add new version
            void AddVersion(const AssetVersionEntry& entry);
            
            // Get specific version
            const AssetVersionEntry* GetVersion(const Version& version) const;
            
            // Get latest version
            const AssetVersionEntry* GetLatestVersion() const;
            
            // Get all versions (sorted by version)
            std::vector<AssetVersionEntry> GetAllVersions() const;
            
            // Get versions in range
            std::vector<AssetVersionEntry> GetVersionsInRange(const Version& from, const Version& to) const;
            
            // Check if version exists
            bool HasVersion(const Version& version) const;
            
            // Get next version number
            Version GetNextVersion(bool incrementMajor = false, bool incrementMinor = false) const;
            
            // Prune old versions (keep latest N versions)
            void PruneVersions(size_t maxVersionsToKeep);
            
            // Archive old versions
            void ArchiveVersions(const std::chrono::system_clock::time_point& olderThan);
            
            // Statistics
            size_t GetVersionCount() const { return versions_.size(); }
            size_t GetTotalDataSize() const;
            
        private:
            std::string assetId_;
            std::vector<AssetVersionEntry> versions_; // Sorted by version
            mutable std::mutex mutex_;
        };

        // Dependency graph for change tracking
        struct DependencyNode {
            std::string assetId;
            std::unordered_set<std::string> dependencies;   // Assets this depends on
            std::unordered_set<std::string> dependents;     // Assets that depend on this
            Version currentVersion;
            std::chrono::system_clock::time_point lastChecked;
        };

        // Change tracking configuration
        struct ChangeTrackingConfig {
            bool enableAutoVersioning = true;      // Auto-increment version on changes
            bool enableDependencyTracking = true;  // Track dependency changes
            bool enableChecksumValidation = true;  // Validate file checksums
            bool enableMetadataTracking = true;    // Track metadata changes
            size_t maxVersionHistory = 100;        // Max versions to keep per asset
            size_t maxChangeHistory = 1000;        // Max change records to keep
            std::chrono::hours archiveAfter{24 * 30}; // Archive versions after 30 days
            std::string versioningScheme = "semantic"; // "semantic" or "timestamp"
        };

        // Asset versioning statistics
        struct VersioningStats {
            size_t totalAssets = 0;
            size_t totalVersions = 0;
            size_t totalChanges = 0;
            size_t totalDependencies = 0;
            size_t archivedVersions = 0;
            float averageVersionsPerAsset = 0.0f;
            size_t totalStorageUsed = 0;
            std::chrono::system_clock::time_point oldestVersion;
            std::chrono::system_clock::time_point newestVersion;
        };

        // Asset Versioning System - main class
        class AssetVersioningSystem {
        public:
            static AssetVersioningSystem& Instance() {
                static AssetVersioningSystem instance;
                return instance;
            }

            // System lifecycle
            bool Initialize(const ChangeTrackingConfig& config = {});
            void Shutdown();
            void Update();

            // Configuration
            void SetConfig(const ChangeTrackingConfig& config) { config_ = config; }
            ChangeTrackingConfig GetConfig() const { return config_; }

            // Asset registration and management
            bool RegisterAsset(const std::string& assetId, const std::string& filePath, 
                             const AssetMetadata& metadata = {});
            void UnregisterAsset(const std::string& assetId);
            bool IsAssetRegistered(const std::string& assetId) const;

            // Version management
            Version CreateNewVersion(const std::string& assetId, const std::string& description = "",
                                   bool incrementMajor = false, bool incrementMinor = false);
            bool SetAssetVersion(const std::string& assetId, const Version& version);
            Version GetAssetVersion(const std::string& assetId) const;
            Version GetLatestVersion(const std::string& assetId) const;

            // Change tracking
            std::string RecordChange(const std::string& assetId, ChangeType changeType,
                                   const std::string& description = "", const std::string& author = "");
            std::vector<ChangeRecord> GetChangeHistory(const std::string& assetId) const;
            std::vector<ChangeRecord> GetRecentChanges(size_t maxCount = 50) const;
            std::vector<ChangeRecord> GetChangesSince(const std::chrono::system_clock::time_point& since) const;

            // Version history
            const VersionHistory* GetVersionHistory(const std::string& assetId) const;
            std::vector<AssetVersionEntry> GetAllVersions(const std::string& assetId) const;
            const AssetVersionEntry* GetSpecificVersion(const std::string& assetId, 
                                                       const Version& version) const;

            // Rollback capabilities
            bool RollbackToVersion(const std::string& assetId, const Version& version);
            bool RollbackToChange(const std::string& assetId, const std::string& changeId);
            std::vector<std::string> PreviewRollbackImpact(const std::string& assetId, 
                                                          const Version& version) const;

            // Dependency management
            void AddDependency(const std::string& assetId, const std::string& dependencyId);
            void RemoveDependency(const std::string& assetId, const std::string& dependencyId);
            std::vector<std::string> GetDependencies(const std::string& assetId) const;
            std::vector<std::string> GetDependents(const std::string& assetId) const;
            std::vector<std::string> GetTransitiveDependencies(const std::string& assetId) const;
            bool HasCircularDependency(const std::string& assetId) const;

            // Change propagation
            void PropagateChanges(const std::string& assetId);
            std::vector<std::string> GetAffectedAssets(const std::string& assetId) const;

            // Metadata management
            bool UpdateMetadata(const std::string& assetId, const AssetMetadata& metadata);
            AssetMetadata GetMetadata(const std::string& assetId) const;
            bool SetCustomProperty(const std::string& assetId, const std::string& key, 
                                 const std::string& value);
            std::string GetCustomProperty(const std::string& assetId, const std::string& key) const;

            // File integrity
            std::string CalculateChecksum(const std::string& filePath) const;
            bool ValidateChecksum(const std::string& assetId) const;
            std::vector<std::string> FindCorruptedAssets() const;

            // History management
            void PruneHistory(const std::string& assetId, size_t maxVersions);
            void ArchiveOldVersions(const std::chrono::system_clock::time_point& olderThan);
            void CleanupOrphanedVersions();

            // Statistics and monitoring
            VersioningStats GetStats() const;
            void ResetStats();
            std::vector<std::string> GetMostActiveAssets(size_t count = 10) const;
            std::vector<std::string> GetLargestAssets(size_t count = 10) const;

            // Export/Import
            bool ExportVersionHistory(const std::string& assetId, const std::string& exportPath) const;
            bool ImportVersionHistory(const std::string& assetId, const std::string& importPath);
            bool ExportChangeLog(const std::string& exportPath, 
                               const std::chrono::system_clock::time_point& since = {}) const;

            // Console commands integration
            void RegisterConsoleCommands();

        private:
            AssetVersioningSystem() = default;
            ~AssetVersioningSystem() = default;
            AssetVersioningSystem(const AssetVersioningSystem&) = delete;
            AssetVersioningSystem& operator=(const AssetVersioningSystem&) = delete;

            // Internal helpers
            std::string GenerateChangeId() const;
            std::string GenerateCommitHash(const AssetVersionEntry& entry) const;
            void UpdateDependencyGraph(const std::string& assetId);
            void CheckForCircularDependencies();
            bool ValidateVersion(const Version& version) const;

            // Change detection
            bool HasFileChanged(const std::string& assetId) const;
            void DetectAndRecordChanges();

            // Internal change recording (assumes locks already held)
            std::string RecordChangeInternal(const std::string& assetId, ChangeType changeType,
                                           const std::string& description, const std::string& author,
                                           const Version& currentVersion, const std::string& filePath);

            // Configuration and state
            ChangeTrackingConfig config_;
            std::atomic<bool> initialized_{false};

            // Asset tracking
            std::unordered_map<std::string, std::unique_ptr<VersionHistory>> versionHistories_;
            std::unordered_map<std::string, AssetMetadata> assetMetadata_;
            std::unordered_map<std::string, DependencyNode> dependencyGraph_;
            mutable std::mutex assetMutex_;

            // Change tracking
            std::vector<ChangeRecord> changeHistory_;
            mutable std::mutex changeMutex_;

            // Statistics
            mutable VersioningStats stats_;
            mutable std::mutex statsMutex_;

            // File monitoring
            std::chrono::steady_clock::time_point lastUpdateTime_;
        };

        // Utility functions
        namespace versioning_utils {
            // Version string parsing and formatting
            Version ParseVersion(const std::string& versionStr);
            std::string FormatVersion(const Version& version);
            bool IsValidVersionString(const std::string& versionStr);

            // File utilities
            std::string CalculateFileChecksum(const std::string& filePath);
            size_t GetFileSize(const std::string& filePath);
            std::chrono::system_clock::time_point GetFileModificationTime(const std::string& filePath);

            // Change utilities
            std::string ChangeTypeToString(ChangeType changeType);
            ChangeType StringToChangeType(const std::string& changeTypeStr);

            // Time utilities
            std::string FormatTimestamp(const std::chrono::system_clock::time_point& timestamp);
            std::chrono::system_clock::time_point ParseTimestamp(const std::string& timestampStr);

            // Dependency utilities
            std::vector<std::string> TopologicalSort(const std::unordered_map<std::string, DependencyNode>& graph);
            bool DetectCircularDependency(const std::unordered_map<std::string, DependencyNode>& graph,
                                        const std::string& startNode);

            // Export/Import utilities
            bool ExportToJson(const VersionHistory& history, const std::string& filePath);
            bool ImportFromJson(VersionHistory& history, const std::string& filePath);
        }

        // Console commands for versioning system
        class VersioningConsoleCommands {
        public:
            static void RegisterCommands();
            static void HandleVersionList(const std::vector<std::string>& args);
            static void HandleVersionCreate(const std::vector<std::string>& args);
            static void HandleVersionRollback(const std::vector<std::string>& args);
            static void HandleVersionHistory(const std::vector<std::string>& args);
            static void HandleVersionStats(const std::vector<std::string>& args);
            static void HandleVersionDeps(const std::vector<std::string>& args);
            static void HandleVersionChanges(const std::vector<std::string>& args);
            static void HandleVersionValidate(const std::vector<std::string>& args);
            static void HandleVersionExport(const std::vector<std::string>& args);
            static void HandleVersionImport(const std::vector<std::string>& args);
        };

    } // namespace versioning
} // namespace assets