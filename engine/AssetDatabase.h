#pragma once

#include "AssetPipeline.h"
#include <sqlite3.h>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace AssetPipeline {

/**
 * Asset Database System
 * SQLite-backed persistent storage for asset metadata and relationships
 */
class AssetDatabase {
public:
    static AssetDatabase& GetInstance();

    // Database lifecycle
    bool Initialize(const std::string& db_path);
    void Shutdown();
    bool IsInitialized() const { return db_ != nullptr; }

    // Metadata operations
    bool StoreMetadata(const AssetMetadata& metadata);
    std::optional<AssetMetadata> LoadMetadata(const std::string& path);
    bool UpdateMetadata(const AssetMetadata& metadata);
    bool DeleteMetadata(const std::string& path);

    // Bulk operations
    std::vector<AssetMetadata> LoadAllMetadata();
    bool StoreAllMetadata(const std::vector<AssetMetadata>& metadata_list);
    
    // Query operations
    std::vector<AssetMetadata> QueryByType(AssetType type);
    std::vector<AssetMetadata> QueryByState(AssetState state);
    std::vector<AssetMetadata> QueryByTag(const std::string& key, const std::string& value);
    std::vector<AssetMetadata> QueryByPlatform(Platform platform);
    std::vector<std::string> QueryDependencies(const std::string& path);
    std::vector<std::string> QueryDependents(const std::string& path);

    // Relationship operations
    bool StoreDependency(const std::string& asset, const std::string& dependency);
    bool RemoveDependency(const std::string& asset, const std::string& dependency);
    bool ClearDependencies(const std::string& asset);

    // Statistics
    size_t GetTotalAssets() const;
    size_t GetAssetsByType(AssetType type) const;
    size_t GetDatabaseSize() const;
    
    // Maintenance
    bool Vacuum();
    bool Backup(const std::string& backup_path);
    bool Restore(const std::string& backup_path);

private:
    AssetDatabase() = default;
    ~AssetDatabase();
    AssetDatabase(const AssetDatabase&) = delete;
    AssetDatabase& operator=(const AssetDatabase&) = delete;

    bool CreateTables();
    bool ExecuteQuery(const std::string& query);
    
    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
