#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace AssetPipeline {

/**
 * Asset Documentation System
 * Technical documentation generation and management for asset systems
 */

enum class DocumentationFormat {
    Markdown,
    HTML,
    JSON,
    XML,
    PDF,
    PlainText
};

struct DocumentationConfig {
    DocumentationFormat format = DocumentationFormat::Markdown;
    bool include_metadata = true;
    bool include_dependencies = true;
    bool include_metrics = true;
    bool include_examples = true;
    bool include_images = false;
    bool include_changelog = true;
    std::string template_path;
    std::string output_directory;
};

struct AssetDocumentation {
    std::string asset_path;
    std::string title;
    std::string description;
    std::string usage_example;
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> metadata_fields;
    std::string changelog;
    std::chrono::system_clock::time_point last_updated;
};

class AssetDocumentationGenerator {
public:
    static AssetDocumentationGenerator& GetInstance();

    // Configuration
    void SetConfig(const DocumentationConfig& config);
    DocumentationConfig GetConfig() const { return config_; }
    
    // Documentation generation
    std::string GenerateAssetDoc(const std::string& asset_path);
    std::string GenerateAssetDoc(const std::string& asset_path, DocumentationFormat format);
    
    std::vector<std::string> GenerateAllDocs();
    std::string GenerateIndexDoc();
    std::string GenerateTypeDoc(AssetType type);
    std::string GenerateDependencyDoc();
    
    // Custom documentation
    void SetAssetDescription(const std::string& asset_path, const std::string& description);
    void SetAssetUsageExample(const std::string& asset_path, const std::string& example);
    void AddToChangelog(const std::string& asset_path, const std::string& entry);
    
    // Documentation retrieval
    std::optional<AssetDocumentation> GetDocumentation(const std::string& asset_path);
    
    // Template system
    void RegisterTemplate(const std::string& name, const std::string& template_content);
    std::string ApplyTemplate(const std::string& template_name, const std::unordered_map<std::string, std::string>& vars);
    
    // Export
    bool ExportDocumentation(const std::string& output_dir);
    bool ExportAssetDoc(const std::string& asset_path, const std::string& output_path);
    
    // API documentation
    std::string GenerateAPIDoc();
    std::string GenerateQuickReference();
    std::string GenerateIntegrationGuide();
    std::string GenerateTroubleshootingGuide();
    
    // Auto-documentation from code
    void ScanForDocComments(const std::string& source_dir);
    void ExtractMetadataFromFiles();
    
    // Documentation validation
    struct DocValidationResult {
        bool is_complete = false;
        std::vector<std::string> missing_fields;
        std::vector<std::string> warnings;
    };
    
    DocValidationResult ValidateDocumentation(const std::string& asset_path);
    
    // Search documentation
    std::vector<std::string> SearchDocs(const std::string& query);
    
    // Statistics
    struct DocStats {
        size_t total_assets = 0;
        size_t documented_assets = 0;
        size_t complete_docs = 0;
        float documentation_coverage = 0.0f;
        std::unordered_map<AssetType, size_t> docs_by_type;
    };
    
    DocStats GetDocumentationStats();
    
    // HTML generation helpers
    std::string GenerateHTMLIndex(const std::vector<std::string>& asset_paths);
    std::string GenerateHTMLAssetPage(const std::string& asset_path);
    std::string GenerateHTMLSearchPage();
    
    // Markdown generation helpers
    std::string GenerateMarkdownIndex(const std::vector<std::string>& asset_paths);
    std::string GenerateMarkdownAssetPage(const std::string& asset_path);
    std::string GenerateMarkdownTable(const std::vector<std::vector<std::string>>& data);

private:
    AssetDocumentationGenerator() = default;
    
    std::string FormatAsMarkdown(const AssetDocumentation& doc);
    std::string FormatAsHTML(const AssetDocumentation& doc);
    std::string FormatAsJSON(const AssetDocumentation& doc);
    std::string FormatAsXML(const AssetDocumentation& doc);
    
    DocumentationConfig config_;
    std::unordered_map<std::string, AssetDocumentation> custom_docs_;
    std::unordered_map<std::string, std::string> templates_;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
