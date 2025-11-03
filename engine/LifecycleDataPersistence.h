#pragma once

#include "LifecycleAnalytics.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace lifecycle {

// Data persistence and archiving system for lifecycle analytics
class LifecycleDataPersistence {
public:
    struct Config {
        std::string dataDirectory = "lifecycle_data";
        std::string csvExportPath = "lifecycle_analytics.csv";
        std::string jsonExportPath = "lifecycle_analytics.json";
        std::string archiveDirectory = "lifecycle_archives";
        bool enableAutoArchiving = true;
        bool enableCSVExport = true;
        bool enableJSONExport = true;
        size_t maxArchiveFiles = 100;
        
        Config() = default;
    };

    static LifecycleDataPersistence& Instance() {
        static LifecycleDataPersistence inst;
        return inst;
    }

    void Initialize() {
        Initialize(Config{});
    }
    
    void Initialize(const Config& config) {
        config_ = config;
        
        // Create directories if they don't exist
        try {
            std::filesystem::create_directories(config_.dataDirectory);
            std::filesystem::create_directories(config_.archiveDirectory);
        } catch (const std::exception& e) {
            std::cerr << "[LifecyclePersistence] Failed to create directories: " << e.what() << std::endl;
        }
        
        std::cout << "[LifecyclePersistence] Data persistence initialized" << std::endl;
    }

    // Export current analytics data to CSV format
    bool ExportToCSV(const std::string& filename = "") {
        std::string filepath = filename.empty() ? 
            config_.dataDirectory + "/" + config_.csvExportPath : filename;
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LifecyclePersistence] Failed to open CSV file: " << filepath << std::endl;
            return false;
        }
        
        // Write CSV header
        file << "timestamp,actor_type,total_created,avg_init_time,avg_active_time,event_counts\n";
        
        // Get analytics data
        auto& analytics = LifecycleAnalytics::Instance();
        auto report = analytics.GenerateReport();
        
        // Write timestamp and data
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        
        // For CSV, we'll write a simplified version
        file << ",summary,1,0.0,0.0,\"" << EscapeCSV(report) << "\"\n";
        
        file.close();
        std::cout << "[LifecyclePersistence] Exported analytics to CSV: " << filepath << std::endl;
        return true;
    }

    // Export current analytics data to JSON format
    bool ExportToJSON(const std::string& filename = "") {
        std::string filepath = filename.empty() ? 
            config_.dataDirectory + "/" + config_.jsonExportPath : filename;
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LifecyclePersistence] Failed to open JSON file: " << filepath << std::endl;
            return false;
        }
        
        // Create enhanced JSON export with metadata
        std::ostringstream json;
        json << "{\n";
        json << "  \"exportMetadata\": {\n";
        json << "    \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n";
        json << "    \"version\": \"1.0\",\n";
        json << "    \"exporter\": \"LifecycleDataPersistence\"\n";
        json << "  },\n";
        json << "  \"analyticsData\": " << LifecycleAnalytics::Instance().ExportJson();
        json << "\n}\n";
        
        file << json.str();
        file.close();
        
        std::cout << "[LifecyclePersistence] Exported analytics to JSON: " << filepath << std::endl;
        return true;
    }

    // Archive current analytics data with timestamp
    bool ArchiveCurrentData() {
        if (!config_.enableAutoArchiving) return false;
        
        std::string timestamp = GetFileTimestamp();
        std::string archiveBasename = config_.archiveDirectory + "/lifecycle_" + timestamp;
        
        bool success = true;
        
        if (config_.enableJSONExport) {
            success &= ExportToJSON(archiveBasename + ".json");
        }
        
        if (config_.enableCSVExport) {
            success &= ExportToCSV(archiveBasename + ".csv");
        }
        
        // Clean up old archives if we exceed the limit
        CleanupOldArchives();
        
        return success;
    }

    // Load and compare with previous analytics data
    std::string LoadAndCompareData(const std::string& archiveFile) {
        std::ifstream file(archiveFile);
        if (!file.is_open()) {
            return "Error: Could not open archive file: " + archiveFile;
        }
        
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string archiveData = buffer.str();
        file.close();
        
        // Get current data for comparison
        std::string currentData = LifecycleAnalytics::Instance().ExportJson();
        
        std::ostringstream comparison;
        comparison << "=== Analytics Data Comparison ===\n";
        comparison << "Archive file: " << archiveFile << "\n";
        comparison << "Archive size: " << archiveData.size() << " bytes\n";
        comparison << "Current size: " << currentData.size() << " bytes\n";
        comparison << "\nArchived data (first 500 chars):\n";
        comparison << archiveData.substr(0, 500) << "...\n";
        comparison << "\nCurrent data (first 500 chars):\n";
        comparison << currentData.substr(0, 500) << "...\n";
        comparison << "==============================\n";
        
        return comparison.str();
    }

    // Get list of available archive files
    std::vector<std::string> GetArchiveFiles() {
        std::vector<std::string> archives;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(config_.archiveDirectory)) {
                if (entry.is_regular_file() && 
                    (entry.path().extension() == ".json" || entry.path().extension() == ".csv")) {
                    archives.push_back(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[LifecyclePersistence] Error reading archive directory: " << e.what() << std::endl;
        }
        
        return archives;
    }

    // Generate historical analytics report
    std::string GenerateHistoricalReport() {
        auto archives = GetArchiveFiles();
        
        std::ostringstream report;
        report << "=== Historical Analytics Report ===\n";
        report << "Data directory: " << config_.dataDirectory << "\n";
        report << "Archive directory: " << config_.archiveDirectory << "\n";
        report << "Available archives: " << archives.size() << "\n\n";
        
        report << "Archive files:\n";
        for (const auto& archive : archives) {
            try {
                auto fileTime = std::filesystem::last_write_time(archive);
                auto size = std::filesystem::file_size(archive);
                
                report << "  " << std::filesystem::path(archive).filename().string() 
                       << " (size: " << size << " bytes)\n";
            } catch (const std::exception& e) {
                report << "  " << archive << " (error reading file info)\n";
            }
        }
        
        report << "\nCurrent analytics state:\n";
        report << LifecycleAnalytics::Instance().GenerateReport();
        report << "==================================\n";
        
        return report.str();
    }

    // Utilities
    const Config& GetConfig() const { return config_; }

private:
    LifecycleDataPersistence() = default;
    
    std::string GetCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    std::string GetFileTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        return ss.str();
    }
    
    std::string EscapeCSV(const std::string& str) const {
        std::string result;
        for (char c : str) {
            if (c == '"') result += "\"\"";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else result += c;
        }
        return result;
    }
    
    void CleanupOldArchives() {
        auto archives = GetArchiveFiles();
        
        if (archives.size() <= config_.maxArchiveFiles) return;
        
        // Sort by modification time (oldest first)
        std::sort(archives.begin(), archives.end(), [](const std::string& a, const std::string& b) {
            try {
                return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
            } catch (const std::exception&) {
                return false;
            }
        });
        
        // Remove oldest files
        size_t toRemove = archives.size() - config_.maxArchiveFiles;
        for (size_t i = 0; i < toRemove; ++i) {
            try {
                std::filesystem::remove(archives[i]);
                std::cout << "[LifecyclePersistence] Removed old archive: " << archives[i] << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[LifecyclePersistence] Failed to remove archive: " << e.what() << std::endl;
            }
        }
    }

    Config config_;
};

// Console commands for data persistence
class LifecyclePersistenceCommands {
public:
    static void RegisterCommands() {
        std::cout << "[LifecyclePersistence] Persistence commands available:" << std::endl;
        std::cout << "  lifecycle.export.csv - Export analytics to CSV" << std::endl;
        std::cout << "  lifecycle.export.json - Export analytics to JSON" << std::endl;
        std::cout << "  lifecycle.archive - Archive current analytics data" << std::endl;
        std::cout << "  lifecycle.history - Show historical analytics report" << std::endl;
        std::cout << "  lifecycle.list.archives - List available archive files" << std::endl;
    }
    
    static void ExecuteCommand(const std::string& command) {
        auto& persistence = LifecycleDataPersistence::Instance();
        
        if (command == "lifecycle.export.csv") {
            persistence.ExportToCSV();
        } else if (command == "lifecycle.export.json") {
            persistence.ExportToJSON();
        } else if (command == "lifecycle.archive") {
            persistence.ArchiveCurrentData();
        } else if (command == "lifecycle.history") {
            std::cout << persistence.GenerateHistoricalReport() << std::endl;
        } else if (command == "lifecycle.list.archives") {
            auto archives = persistence.GetArchiveFiles();
            std::cout << "Available archives (" << archives.size() << "):" << std::endl;
            for (const auto& archive : archives) {
                std::cout << "  " << archive << std::endl;
            }
        } else {
            std::cout << "Unknown persistence command: " << command << std::endl;
        }
    }
};

// Utility functions for persistence integration
namespace persistence_utils {
    // Initialize complete persistence system
    inline void InitializePersistenceSystem() {
        LifecycleDataPersistence::Config config;
        config.dataDirectory = "artifacts/lifecycle_data";
        config.archiveDirectory = "artifacts/lifecycle_archives";
        config.enableAutoArchiving = true;
        config.enableCSVExport = true;
        config.enableJSONExport = true;
        config.maxArchiveFiles = 50;
        
        LifecycleDataPersistence::Instance().Initialize(config);
        LifecyclePersistenceCommands::RegisterCommands();
        
        std::cout << "[LifecyclePersistence] Complete persistence system initialized" << std::endl;
    }
    
    // Shutdown persistence system with final data export
    inline void ShutdownPersistenceSystem() {
        auto& persistence = LifecycleDataPersistence::Instance();
        
        // Archive final analytics data
        persistence.ArchiveCurrentData();
        
        // Export final reports
        persistence.ExportToJSON();
        persistence.ExportToCSV();
        
        std::cout << "[LifecyclePersistence] Final analytics data exported and archived" << std::endl;
        std::cout << "[LifecyclePersistence] Persistence system shutdown complete" << std::endl;
    }
    
    // Quick export function for on-demand analytics export
    inline void QuickExport() {
        auto& persistence = LifecycleDataPersistence::Instance();
        persistence.ExportToJSON();
        persistence.ExportToCSV();
        std::cout << "[LifecyclePersistence] Quick export completed" << std::endl;
    }
}

} // namespace lifecycle