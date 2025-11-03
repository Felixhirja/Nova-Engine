#pragma once

#include "AssetProcessingPipeline.h"
#include "AssetProcessors.h"
#include "AssetValidators.h"
#include <iostream>

namespace assets {

// Asset Pipeline Integration Manager
class AssetPipelineManager {
public:
    static AssetPipelineManager& Instance() {
        static AssetPipelineManager instance;
        return instance;
    }

    // Initialize the complete asset pipeline system
    bool Initialize(const ProcessingConfig& config = ProcessingConfig{}) {
        std::cout << "[AssetPipelineManager] Initializing Asset Pipeline System..." << std::endl;
        
        try {
            // Initialize the core pipeline
            auto& pipeline = AssetProcessingPipeline::Instance();
            pipeline.Initialize(config);
            
            // Register all processors
            RegisterAllProcessors();
            
            // Register all validators
            RegisterAllValidators();
            
            // Register console commands
            AssetPipelineCommands::RegisterCommands();
            
            // Load existing asset database if available
            pipeline.LoadAssetDatabase();
            
            isInitialized_ = true;
            std::cout << "[AssetPipelineManager] Asset Pipeline System initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "[AssetPipelineManager] Failed to initialize: " << e.what() << std::endl;
            return false;
        }
    }

    // Shutdown the asset pipeline system
    void Shutdown() {
        if (!isInitialized_) return;
        
        std::cout << "[AssetPipelineManager] Shutting down Asset Pipeline System..." << std::endl;
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        pipeline.Shutdown();
        
        isInitialized_ = false;
        std::cout << "[AssetPipelineManager] Asset Pipeline System shutdown complete" << std::endl;
    }

    // Check if system is initialized
    bool IsInitialized() const {
        return isInitialized_;
    }

    // Quick asset processing interface
    bool ProcessSingleAsset(const std::string& filePath) {
        if (!isInitialized_) {
            std::cout << "[AssetPipelineManager] System not initialized" << std::endl;
            return false;
        }
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        AssetMetadata metadata;
        return pipeline.ProcessAsset(filePath, metadata);
    }

    // Batch process directory
    void ProcessDirectory(const std::string& directory, bool recursive = true) {
        if (!isInitialized_) {
            std::cout << "[AssetPipelineManager] System not initialized" << std::endl;
            return;
        }
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        auto assets = pipeline.ScanDirectory(directory, recursive);
        
        if (!assets.empty()) {
            pipeline.ProcessAssetBatch(assets);
        }
    }

    // Get processing statistics
    std::string GetSystemStatus() const {
        if (!isInitialized_) {
            return "Asset Pipeline System: Not Initialized";
        }
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        std::stringstream ss;
        ss << "Asset Pipeline System Status:\n";
        ss << "  Status: Initialized\n";
        ss << pipeline.GetAssetAnalytics();
        ss << "  Processors: " << processorCount_ << "\n";
        ss << "  Validators: " << validatorCount_ << "\n";
        
        return ss.str();
    }

    // Configuration management
    void UpdateConfig(const ProcessingConfig& config) {
        if (!isInitialized_) return;
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        pipeline.SetConfig(config);
        std::cout << "[AssetPipelineManager] Configuration updated" << std::endl;
    }

    ProcessingConfig GetConfig() const {
        if (!isInitialized_) return ProcessingConfig{};
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        return pipeline.GetConfig();
    }

    // Asset management helpers
    std::vector<AssetMetadata> GetAllAssets() const {
        if (!isInitialized_) return {};
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        return pipeline.GetAllAssets();
    }

    AssetMetadata* FindAsset(const std::string& assetId) {
        if (!isInitialized_) return nullptr;
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        return pipeline.FindAsset(assetId);
    }

    // Utility methods
    bool ValidateAsset(const std::string& filePath) {
        if (!isInitialized_) return false;
        
        auto& pipeline = AssetProcessingPipeline::Instance();
        auto result = pipeline.ValidateAsset(filePath);
        return result.isValid;
    }

    // Console command interface
    void ExecuteCommand(const std::string& command, const std::vector<std::string>& args = {}) {
        if (!isInitialized_) {
            std::cout << "[AssetPipelineManager] System not initialized" << std::endl;
            return;
        }
        
        AssetPipelineCommands::ExecuteCommand(command, args);
    }

private:
    AssetPipelineManager() = default;
    
    void RegisterAllProcessors() {
        auto& pipeline = AssetProcessingPipeline::Instance();
        auto processors = ProcessorFactory::CreateAllProcessors();
        
        processorCount_ = processors.size();
        
        for (auto& processor : processors) {
            pipeline.RegisterProcessor(std::move(processor));
        }
        
        std::cout << "[AssetPipelineManager] Registered " << processorCount_ << " processors" << std::endl;
    }
    
    void RegisterAllValidators() {
        auto& pipeline = AssetProcessingPipeline::Instance();
        auto validators = ValidatorFactory::CreateAllValidators();
        
        validatorCount_ = validators.size();
        
        for (auto& validator : validators) {
            pipeline.RegisterValidator(std::move(validator));
        }
        
        std::cout << "[AssetPipelineManager] Registered " << validatorCount_ << " validators" << std::endl;
    }

    bool isInitialized_ = false;
    size_t processorCount_ = 0;
    size_t validatorCount_ = 0;
};

// Convenience functions for easy integration
namespace pipeline_integration {

    // Initialize with default configuration
    inline bool Initialize() {
        return AssetPipelineManager::Instance().Initialize();
    }

    // Initialize with custom configuration
    inline bool Initialize(const ProcessingConfig& config) {
        return AssetPipelineManager::Instance().Initialize(config);
    }

    // Shutdown the system
    inline void Shutdown() {
        AssetPipelineManager::Instance().Shutdown();
    }

    // Process a single asset
    inline bool ProcessAsset(const std::string& filePath) {
        return AssetPipelineManager::Instance().ProcessSingleAsset(filePath);
    }

    // Process all assets in a directory
    inline void ProcessDirectory(const std::string& directory, bool recursive = true) {
        AssetPipelineManager::Instance().ProcessDirectory(directory, recursive);
    }

    // Get system status
    inline std::string GetStatus() {
        return AssetPipelineManager::Instance().GetSystemStatus();
    }

    // Execute pipeline command
    inline void ExecuteCommand(const std::string& command, const std::vector<std::string>& args = {}) {
        AssetPipelineManager::Instance().ExecuteCommand(command, args);
    }

    // Quick asset validation
    inline bool ValidateAsset(const std::string& filePath) {
        return AssetPipelineManager::Instance().ValidateAsset(filePath);
    }

    // Configuration helpers
    inline ProcessingConfig CreateMobileConfig() {
        ProcessingConfig config;
        config.targetPlatform = PlatformTarget::Mobile;
        config.targetQuality = QualityLevel::Medium;
        config.enableCompression = true;
        config.enableOptimization = true;
        config.maxThreads = 2;
        return config;
    }

    inline ProcessingConfig CreateDesktopConfig() {
        ProcessingConfig config;
        config.targetPlatform = PlatformTarget::Desktop;
        config.targetQuality = QualityLevel::High;
        config.enableCompression = true;
        config.enableOptimization = true;
        config.maxThreads = 4;
        return config;
    }

    inline ProcessingConfig CreateWebConfig() {
        ProcessingConfig config;
        config.targetPlatform = PlatformTarget::Web;
        config.targetQuality = QualityLevel::Medium;
        config.enableCompression = true;
        config.enableOptimization = true;
        config.maxThreads = 2;
        config.outputDirectory = "assets/web";
        return config;
    }

    inline ProcessingConfig CreateDevelopmentConfig() {
        ProcessingConfig config;
        config.targetPlatform = PlatformTarget::Desktop;
        config.targetQuality = QualityLevel::Medium;
        config.enableCompression = false;
        config.enableOptimization = false;
        config.enableValidation = true;
        config.preserveOriginals = true;
        config.maxThreads = 2;
        return config;
    }

    inline ProcessingConfig CreateProductionConfig() {
        ProcessingConfig config;
        config.targetPlatform = PlatformTarget::Desktop;
        config.targetQuality = QualityLevel::High;
        config.enableCompression = true;
        config.enableOptimization = true;
        config.enableValidation = true;
        config.enableCaching = true;
        config.preserveOriginals = false;
        config.maxThreads = 6;
        return config;
    }

} // namespace pipeline_integration

} // namespace assets