#pragma once

#include "AssetPipeline.h"
#include <string>
#include <memory>
#include <functional>

namespace AssetPipeline {

/**
 * Asset Integration System
 * Integration with external asset management tools and services
 */

// Supported external tools
enum class ExternalTool {
    Git,          // Git version control
    Perforce,     // Perforce VCS
    Unity,        // Unity asset database
    Unreal,       // Unreal asset registry
    Blender,      // Blender export
    Maya,         // Maya export
    SubstancePainter, // Substance Painter
    PhotoShop,    // Photoshop
    Audacity,     // Audacity audio
    Custom        // Custom integrations
};

// Integration configuration
struct IntegrationConfig {
    ExternalTool tool;
    std::string tool_path;
    std::string workspace_path;
    std::unordered_map<std::string, std::string> parameters;
    bool auto_sync = false;
    bool bidirectional = false;
};

// Integration result
struct IntegrationResult {
    bool success = false;
    std::string message;
    std::vector<std::string> affected_assets;
    std::vector<std::string> warnings;
};

class AssetIntegration {
public:
    static AssetIntegration& GetInstance();

    // Integration management
    bool RegisterIntegration(const std::string& name, const IntegrationConfig& config);
    bool UnregisterIntegration(const std::string& name);
    std::vector<std::string> GetActiveIntegrations();
    
    // Synchronization
    IntegrationResult SyncFromExternal(const std::string& integration_name);
    IntegrationResult SyncToExternal(const std::string& integration_name);
    IntegrationResult BidirectionalSync(const std::string& integration_name);
    
    // Git integration
    IntegrationResult GitCommit(const std::vector<std::string>& assets, const std::string& message);
    IntegrationResult GitPull();
    IntegrationResult GitPush();
    std::vector<std::string> GitStatus();
    
    // Asset import/export
    IntegrationResult ImportFromExternal(const std::string& tool_name, const std::string& source_path);
    IntegrationResult ExportToExternal(const std::string& tool_name, const std::vector<std::string>& assets);
    
    // Watch for external changes
    void EnableExternalWatch(const std::string& integration_name, bool enable);
    bool IsWatchingExternal(const std::string& integration_name);
    
    // Custom integration hooks
    using ImportHook = std::function<IntegrationResult(const std::string&)>;
    using ExportHook = std::function<IntegrationResult(const std::vector<std::string>&)>;
    
    void RegisterImportHook(const std::string& tool_name, ImportHook hook);
    void RegisterExportHook(const std::string& tool_name, ExportHook hook);
    
    // Pipeline integration
    void SetPreImportCallback(std::function<void(const std::string&)> callback);
    void SetPostImportCallback(std::function<void(const std::string&)> callback);
    void SetPreExportCallback(std::function<void(const std::string&)> callback);
    void SetPostExportCallback(std::function<void(const std::string&)> callback);

private:
    AssetIntegration() = default;
    
    std::unordered_map<std::string, IntegrationConfig> integrations_;
    std::unordered_map<std::string, ImportHook> import_hooks_;
    std::unordered_map<std::string, ExportHook> export_hooks_;
    
    std::function<void(const std::string&)> pre_import_callback_;
    std::function<void(const std::string&)> post_import_callback_;
    std::function<void(const std::string&)> pre_export_callback_;
    std::function<void(const std::string&)> post_export_callback_;
    
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
