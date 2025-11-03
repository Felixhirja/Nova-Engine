#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>

namespace ContentManagement {

/**
 * ContentIntegration: Integration with external content tools
 * 
 * Features:
 * - Import/export to industry-standard formats
 * - Integration with version control systems (Git, Perforce)
 * - Plugin system for custom integrations
 * - REST API for external tools
 * - Webhook support for notifications
 * - Sync with external databases
 */
class ContentIntegration {
public:
    enum class IntegrationType {
        VersionControl,  // Git, SVN, Perforce
        AssetManagement, // Articy, Yarn Spinner
        Spreadsheet,     // Excel, Google Sheets
        Database,        // MySQL, PostgreSQL, MongoDB
        ContentPlatform, // Unity, Unreal, Godot
        CloudStorage,    // AWS S3, Google Cloud, Azure
        Custom          // Plugin-based
    };

    struct Integration {
        std::string id;
        std::string name;
        IntegrationType type;
        std::string endpoint;  // URL or path
        std::unordered_map<std::string, std::string> credentials;
        std::unordered_map<std::string, std::string> config;
        bool enabled;
        int syncIntervalSeconds;
        std::chrono::system_clock::time_point lastSync;
    };

    struct ImportExportFormat {
        std::string formatId;
        std::string name;
        std::string extension;  // e.g., ".json", ".xml", ".csv"
        std::string description;
        std::function<bool(const std::string&, std::vector<simplejson::JsonObject>&)> importer;
        std::function<bool(const std::vector<simplejson::JsonObject>&, const std::string&)> exporter;
        bool supportsImport;
        bool supportsExport;
    };

    struct WebhookConfig {
        std::string webhookId;
        std::string url;
        std::vector<std::string> events;  // "content.created", "content.updated", etc.
        std::string secret;  // For signature verification
        bool enabled;
    };

    struct SyncOperation {
        std::string operationId;
        std::string integrationId;
        std::string operation;  // "pull", "push", "sync"
        std::vector<std::string> affectedContent;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        bool success;
        std::string errorMessage;
        std::vector<std::string> logs;
    };

    ContentIntegration();
    ~ContentIntegration();

    // Integration Management
    void RegisterIntegration(const Integration& integration);
    void UpdateIntegration(const std::string& integrationId, const Integration& integration);
    void RemoveIntegration(const std::string& integrationId);
    const Integration* GetIntegration(const std::string& integrationId) const;
    std::vector<Integration> GetAllIntegrations() const;
    std::vector<Integration> GetIntegrationsByType(IntegrationType type) const;
    
    // Import/Export
    void RegisterFormat(const ImportExportFormat& format);
    std::vector<std::string> GetSupportedImportFormats() const;
    std::vector<std::string> GetSupportedExportFormats() const;
    
    bool ImportContent(const std::string& filePath, const std::string& formatId, std::vector<simplejson::JsonObject>& content);
    bool ExportContent(const std::vector<std::string>& contentIds, const std::string& outputPath, const std::string& formatId);
    bool BatchImport(const std::vector<std::string>& filePaths, const std::string& formatId);
    bool BatchExport(const std::string& outputDirectory, const std::string& formatId);
    
    // Version Control Integration
    bool ConnectToVCS(const std::string& integrationId, const std::string& repoUrl, const std::string& branch);
    bool PullFromVCS(const std::string& integrationId);
    bool PushToVCS(const std::string& integrationId, const std::vector<std::string>& contentIds, const std::string& commitMessage);
    bool CreateBranch(const std::string& integrationId, const std::string& branchName);
    bool MergeBranch(const std::string& integrationId, const std::string& sourceBranch, const std::string& targetBranch);
    std::vector<std::string> GetVCSChanges(const std::string& integrationId) const;
    
    // Database Sync
    bool ConnectToDatabase(const std::string& integrationId, const std::string& connectionString);
    bool SyncWithDatabase(const std::string& integrationId, bool bidirectional = false);
    bool PullFromDatabase(const std::string& integrationId, const std::string& query);
    bool PushToDatabase(const std::string& integrationId, const std::vector<std::string>& contentIds);
    
    // Cloud Storage
    bool ConnectToCloudStorage(const std::string& integrationId, const std::string& bucket);
    bool UploadToCloud(const std::string& integrationId, const std::vector<std::string>& contentIds);
    bool DownloadFromCloud(const std::string& integrationId, const std::string& path);
    bool SyncWithCloud(const std::string& integrationId);
    
    // Spreadsheet Integration
    bool ImportFromSpreadsheet(const std::string& filePath, const std::string& sheetName, std::vector<simplejson::JsonObject>& content);
    bool ExportToSpreadsheet(const std::vector<std::string>& contentIds, const std::string& outputPath, const std::string& sheetName);
    bool SyncWithGoogleSheets(const std::string& integrationId, const std::string& spreadsheetId);
    
    // REST API
    void StartAPIServer(int port = 8080);
    void StopAPIServer();
    bool IsAPIServerRunning() const;
    void RegisterAPIEndpoint(const std::string& path, std::function<std::string(const std::string&)> handler);
    
    // Webhooks
    void RegisterWebhook(const WebhookConfig& webhook);
    void RemoveWebhook(const std::string& webhookId);
    void TriggerWebhook(const std::string& event, const simplejson::JsonObject& payload);
    bool TestWebhook(const std::string& webhookId) const;
    
    // Plugin System
    bool LoadPlugin(const std::string& pluginPath);
    bool UnloadPlugin(const std::string& pluginId);
    std::vector<std::string> GetLoadedPlugins() const;
    void RegisterPluginHook(const std::string& hookName, std::function<void(const simplejson::JsonObject&)> callback);
    void TriggerPluginHook(const std::string& hookName, const simplejson::JsonObject& data);
    
    // Sync Management
    SyncOperation StartSync(const std::string& integrationId, const std::string& operation);
    bool CancelSync(const std::string& operationId);
    const SyncOperation* GetSyncOperation(const std::string& operationId) const;
    std::vector<SyncOperation> GetSyncHistory(const std::string& integrationId, int maxResults = 50) const;
    
    void EnableAutoSync(const std::string& integrationId, bool enabled);
    void SetSyncInterval(const std::string& integrationId, int seconds);
    void ProcessAutoSync();  // Called by update loop
    
    // Validation
    bool ValidateIntegration(const std::string& integrationId, std::vector<std::string>& errors) const;
    bool TestConnection(const std::string& integrationId, std::string& errorMessage) const;
    
    // Conflict Resolution
    std::vector<std::string> DetectConflicts(const std::string& integrationId) const;
    bool ResolveConflict(const std::string& contentId, const std::string& resolution);
    
    // Monitoring
    void SetOnImportComplete(std::function<void(int)> callback);
    void SetOnExportComplete(std::function<void(int)> callback);
    void SetOnSyncComplete(std::function<void(const std::string&, bool)> callback);
    
    // UI Integration
    void RenderIntegrationManager();
    void RenderImportExportDialog();
    void RenderSyncStatus();
    void RenderAPIDocumentation();
    
private:
    bool ExecuteSync(Integration& integration, const std::string& operation, SyncOperation& syncOp);
    bool ValidateCredentials(const Integration& integration) const;
    
    std::unordered_map<std::string, Integration> integrations_;
    std::unordered_map<std::string, ImportExportFormat> formats_;
    std::unordered_map<std::string, WebhookConfig> webhooks_;
    std::vector<SyncOperation> syncHistory_;
    
    std::function<void(int)> onImportComplete_;
    std::function<void(int)> onExportComplete_;
    std::function<void(const std::string&, bool)> onSyncComplete_;
    
    bool apiServerRunning_;
    int apiServerPort_;
};

} // namespace ContentManagement
